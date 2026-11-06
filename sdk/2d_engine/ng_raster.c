#include "ng_raster.h"
#include "ng_sprite_group.h"
#include "neogeo.h"

/*
 * The LSPC timer counts pixels (6 MHz, 384 a line) down from its load value
 * and interrupts (IRQ2) when it reaches 0. REG_LSPCMODE bit 4 turns the
 * interrupt on; bit 5 loads the counter as soon as REG_TIMERLOW is written;
 * bit 6 loads it at the start of each frame's blanking (line 240); bit 7
 * loads it again each time it reaches 0. A load value must stay above 4:
 * with the interrupt on, a smaller one floods the CPU. Bit 1 of REG_IRQACK
 * acknowledges it. (neogeodev wiki: "Timer interrupt", "Memory mapped
 * registers".)
 *
 * The timer runs only over the bands in use, so a frame costs one interrupt
 * a band:
 * - the frame's load (bit 6, line 240) holds the delay to the first band;
 * - that band's interrupt loads the band period at once and repeats it
 *   (bits 5 and 7);
 * - the last band's interrupt parks the counter (loaded with its largest
 *   value) and leaves the next frame's delay to be loaded at the blank.
 *
 * The handler works out its band from the raster line counter rather than
 * by counting interrupts, so one held back or missed costs nothing after it.
 */

#define RASTER_IRQ_ON      0x0010u
#define RASTER_LOAD_WRITE  0x0020u
#define RASTER_LOAD_FRAME  0x0040u
#define RASTER_LOAD_ZERO   0x0080u
#define RASTER_ACK_TIMER   0x0002u

typedef struct {                                /* 32 bytes: indexed by a shift */
    uint8_t ncolors, nwords;
    uint16_t spare;
    uint16_t color_at[NG_RASTER_MAX_COLORS];   /* byte offset into palette RAM */
    uint16_t color[NG_RASTER_MAX_COLORS];
    uint16_t word_at[NG_RASTER_MAX_WORDS];     /* VRAM address */
    uint16_t word[NG_RASTER_MAX_WORDS];
    uint16_t spare2[2];
} NGRasterBand;

static NGRasterBand ng_raster_table[2][NG_RASTER_BANDS];
static uint8_t ng_raster_first[2], ng_raster_used[2];   /* bands in use: first .. used-1 */
static uint8_t ng_raster_back;                  /* the table being built */

/* What the interrupt reads. */
static NGRasterBand *volatile ng_raster_front;
static volatile uint8_t ng_raster_front_first, ng_raster_front_last;
static volatile uint16_t ng_raster_delay_hi, ng_raster_delay_lo;   /* the next frame's load */
static volatile uint8_t ng_raster_starting;     /* the next interrupt is a frame's first */
static uint16_t ng_raster_period_hi, ng_raster_period_lo;
static uint16_t ng_raster_mode_off, ng_raster_mode_once, ng_raster_mode_repeat, ng_raster_mode_write;
static volatile uint16_t ng_raster_irqs, ng_raster_skips;
/* The band of each value of the raster line counter (bits 15-7 of
 * REG_LSPCMODE, $F8..$1FF; line 0 is $100): the lines since the frame's
 * load on line 240, to the nearest band. Made by ng_raster_start(). */
static uint8_t ng_raster_band_of[512];

static uint8_t ng_raster_lines;                 /* 8 or 16, 0 = off */
static uint8_t ng_raster_shift;
static uint8_t ng_raster_max;                   /* the last band: 263 / lines */
static uint8_t ng_raster_running;
static uint8_t ng_raster_anim;

static void NEOGEO_USER raster_modes(void)
{
    uint16_t a = (uint16_t)((uint16_t)ng_raster_anim << 8);
    ng_raster_mode_off = a;
    ng_raster_mode_once = (uint16_t)(a | RASTER_IRQ_ON | RASTER_LOAD_FRAME);
    ng_raster_mode_repeat = (uint16_t)(a | RASTER_IRQ_ON | RASTER_LOAD_FRAME | RASTER_LOAD_ZERO);
    ng_raster_mode_write = (uint16_t)(a | RASTER_IRQ_ON | RASTER_LOAD_FRAME | RASTER_LOAD_WRITE);
}

void NEOGEO_USER ng_raster_set_anim_speed(uint8_t speed)
{
    ng_raster_anim = speed;
    raster_modes();
    if (!ng_raster_running) NEO_REGISTER(REG_LSPCMODE) = ng_raster_mode_off;
}

void NEOGEO_USER ng_raster_stop(void)
{
    ng_raster_front = 0;
    ng_raster_running = 0;
    ng_raster_lines = 0;
    raster_modes();
    NEO_REGISTER(REG_LSPCMODE) = ng_raster_mode_off;
    NEO_REGISTER(REG_IRQACK) = RASTER_ACK_TIMER;
}

void NEOGEO_USER ng_raster_start(uint8_t lines)
{
    uint32_t period;
    uint16_t c;
    ng_raster_stop();
    if (lines != 8u && lines != 16u) return;
    ng_raster_lines = lines;
    ng_raster_shift = (uint8_t)(lines == 8u ? 3u : 4u);
    ng_raster_max = (uint8_t)(263u >> ng_raster_shift);   /* a later one would be the next frame's */
    ng_raster_used[0] = ng_raster_used[1] = 0;
    ng_raster_first[0] = ng_raster_first[1] = NG_RASTER_BANDS;
    ng_raster_back = 0;
    ng_raster_irqs = ng_raster_skips = 0;
    period = (uint32_t)lines * 384u - 1u;
    ng_raster_period_hi = (uint16_t)(period >> 16);
    ng_raster_period_lo = (uint16_t)period;
    for (c = 0; c < 512u; c++) {
        uint16_t r = (uint16_t)(c >= 0x100u ? c - 0x100u + 24u : c - 0xF8u + 280u);
        if (c < 0xF8u) r = 0;                       /* never read */
        if (r >= 264u) r = (uint16_t)(r - 264u);
        ng_raster_band_of[c] = (uint8_t)((r + (lines >> 1)) >> ng_raster_shift);
    }
}

uint8_t NEOGEO_USER ng_raster_band_at(int16_t y)
{
    /* band b starts on line 240 + b * lines of the blank before, which is
     * screen line b * lines - 40 */
    int16_t v = (int16_t)(y + 40);
    uint8_t b;
    if (!ng_raster_lines) return 0;
    if (v < 0) v = 0;
    b = (uint8_t)(((uint16_t)v + ng_raster_lines - 1u) >> ng_raster_shift);
    if (b < 1u) b = 1u;                              /* band 0 is the load itself */
    return b < ng_raster_max ? b : ng_raster_max;
}

uint8_t NEOGEO_USER ng_raster_last_band(void)
{
    return ng_raster_max;
}

int16_t NEOGEO_USER ng_raster_band_line(uint8_t band)
{
    return (int16_t)(((int16_t)band << ng_raster_shift) - 40);
}

void NEOGEO_USER ng_raster_clear(void)
{
    uint8_t i;
    NGRasterBand *t = ng_raster_table[ng_raster_back];
    for (i = ng_raster_first[ng_raster_back]; i < ng_raster_used[ng_raster_back]; i++)
        t[i].ncolors = t[i].nwords = 0;
    ng_raster_used[ng_raster_back] = 0;
    ng_raster_first[ng_raster_back] = NG_RASTER_BANDS;
}

/* Bands first .. end-1 join the table, emptied as they join. */
static NGRasterBand *NEOGEO_USER raster_take(uint8_t first, uint8_t end)
{
    NGRasterBand *t = ng_raster_table[ng_raster_back];
    uint8_t lo = ng_raster_first[ng_raster_back], hi = ng_raster_used[ng_raster_back], i;
    if (!hi) {                                  /* an empty table */
        for (i = first; i < end; i++) t[i].ncolors = t[i].nwords = 0;
        ng_raster_first[ng_raster_back] = first;
        ng_raster_used[ng_raster_back] = end;
        return t + first;
    }
    for (i = first; i < lo; i++) t[i].ncolors = t[i].nwords = 0;
    for (i = hi; i < end; i++) t[i].ncolors = t[i].nwords = 0;
    if (first < lo) ng_raster_first[ng_raster_back] = first;
    if (end > hi) ng_raster_used[ng_raster_back] = end;
    return t + first;
}

void NEOGEO_USER ng_raster_color(uint8_t band, uint16_t color_index, uint16_t color)
{
    NGRasterBand *b;
    if (!ng_raster_lines || !band || band > ng_raster_max || color_index > 4095u) return;
    b = raster_take(band, (uint8_t)(band + 1u));
    if (b->ncolors >= NG_RASTER_MAX_COLORS) return;
    b->color_at[b->ncolors] = (uint16_t)(color_index << 1);
    b->color[b->ncolors] = color;
    b->ncolors++;
}

void NEOGEO_USER ng_raster_vram(uint8_t band, uint16_t vram_addr, uint16_t value)
{
    NGRasterBand *b;
    if (!ng_raster_lines || !band || band > ng_raster_max) return;
    b = raster_take(band, (uint8_t)(band + 1u));
    if (b->nwords >= NG_RASTER_MAX_WORDS) return;
    b->word_at[b->nwords] = vram_addr;
    b->word[b->nwords] = value;
    b->nwords++;
}

void NEOGEO_USER ng_raster_vram_bands(uint8_t first, uint8_t count, uint16_t vram_addr, const uint16_t *values)
{
    NGRasterBand *t;
    if (!ng_raster_lines || !values || !first || first > ng_raster_max || !count) return;
    if (count > (uint8_t)(ng_raster_max + 1u - first)) count = (uint8_t)(ng_raster_max + 1u - first);
    t = raster_take(first, (uint8_t)(first + count));
    for (; count; count--, t++, values++) {
        if (t->nwords >= NG_RASTER_MAX_WORDS) continue;
        t->word_at[t->nwords] = vram_addr;
        t->word[t->nwords] = *values;
        t->nwords++;
    }
}

void NEOGEO_USER ng_raster_vblank(void)
{
    uint8_t used, first;
    uint32_t delay;
    if (!ng_raster_lines) return;
    used = ng_raster_used[ng_raster_back];
    first = ng_raster_first[ng_raster_back];
    ng_raster_front = 0;                        /* (the interrupt reads these) */
    if (used) {
        ng_raster_front_first = first;
        ng_raster_front_last = (uint8_t)(used - 1u);
        delay = ((uint32_t)first * ng_raster_lines) * 384u - 1u;
        ng_raster_delay_hi = (uint16_t)(delay >> 16);
        ng_raster_delay_lo = (uint16_t)delay;
        ng_raster_front = ng_raster_table[ng_raster_back];
        if (!ng_raster_running) {
            /* from the next frame's load on */
            ng_raster_starting = 1;
            NEO_REGISTER(REG_LSPCMODE) = ng_raster_mode_once;
            NEO_REGISTER(REG_TIMERHIGH) = ng_raster_delay_hi;
            NEO_REGISTER(REG_TIMERLOW) = ng_raster_delay_lo;     /* bit 5 clear: not now */
            ng_raster_running = 1;
        }
    } else if (ng_raster_running) {
        NEO_REGISTER(REG_LSPCMODE) = ng_raster_mode_off;
        NEO_REGISTER(REG_IRQACK) = RASTER_ACK_TIMER;
        ng_raster_running = 0;
    }
    ng_raster_back ^= 1u;
    ng_raster_clear();
}

static inline void raster_apply(void)
{
    const NGRasterBand *t;
    uint8_t b, i;
    NEO_REGISTER(REG_IRQACK) = RASTER_ACK_TIMER;
    ng_raster_irqs++;
    b = ng_raster_band_of[NEO_REGISTER(REG_LSPCMODE) >> 7];
    if (ng_raster_starting) {
        /* the frame's first: a band at a time from here */
        ng_raster_starting = 0;
        NEO_REGISTER(REG_LSPCMODE) = ng_raster_mode_write;
        NEO_REGISTER(REG_TIMERHIGH) = ng_raster_period_hi;
        NEO_REGISTER(REG_TIMERLOW) = ng_raster_period_lo;       /* loaded now */
        NEO_REGISTER(REG_LSPCMODE) = ng_raster_mode_repeat;
    }
    if (b >= ng_raster_front_last) {
        /* the last: park the counter, and leave the next frame's delay
         * for the load at the blank */
        NEO_REGISTER(REG_LSPCMODE) = ng_raster_mode_write;
        NEO_REGISTER(REG_TIMERHIGH) = 0xFFFFu;
        NEO_REGISTER(REG_TIMERLOW) = 0xFFFFu;                   /* loaded now */
        NEO_REGISTER(REG_LSPCMODE) = ng_raster_mode_once;
        NEO_REGISTER(REG_TIMERHIGH) = ng_raster_delay_hi;
        NEO_REGISTER(REG_TIMERLOW) = ng_raster_delay_lo;        /* at the blank */
        ng_raster_starting = 1;
    }
    t = ng_raster_front;
    if (!t || b < ng_raster_front_first || b > ng_raster_front_last) return;
    t += b;
    for (i = 0; i < t->ncolors; i++)
        *(volatile uint16_t *)(PALETTES + t->color_at[i]) = t->color[i];
    if (!t->nwords) return;
#ifdef NG_VRAM_DEFER
    if (ng_vram_busy) { ng_raster_skips++; return; }
    NEO_REGISTER(VRAM_ADDR) = t->word_at[0];
    NEO_REGISTER(VRAM_RW) = t->word[0];
    if (t->nwords > 1u) {
        NEO_REGISTER(VRAM_ADDR) = t->word_at[1];
        NEO_REGISTER(VRAM_RW) = t->word[1];
    }
#else
    ng_raster_skips++;
#endif
}

/* From a game's own IRQ2 handler. */
void NEOGEO_USER ng_raster_irq(void)
{
    raster_apply();
}

/* The handler itself, for a game whose IRQ2 is just a jump here
 * (games/maiya/user.c): no second call and register save. */
NEOGEO_INTERRUPT void NEOGEO_USER ng_raster_irq_handler(void)
{
    raster_apply();
}

uint16_t NEOGEO_USER ng_raster_irq_count(void) { return ng_raster_irqs; }
uint16_t NEOGEO_USER ng_raster_vram_skips(void) { return ng_raster_skips; }
