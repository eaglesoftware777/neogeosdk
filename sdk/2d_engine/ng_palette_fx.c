/* Palette effects use the Neo Geo split-component color encoding.
 * Index zero is preserved; uploads are deferred to the render queue. */

#include "ng_palette_fx.h"
#include "macro.h"
#include "neogeo.h"

static NGPalFxSlot ng_palfx_slots[NG_PALFX_MAX_SLOTS];

#include "ng_palette_math.h"

#if NG_PALFX_SCREEN
/* ------------------------------------------------------------------ */
/*  The screen (see ng_palette_fx.h)                                  */
/* ------------------------------------------------------------------ */
static struct {
    uint16_t *base;          /* the colours as loaded, count * 16 words     */
    uint16_t *out;           /* the colours to show at the next blank       */
    uint8_t   count;         /* banks 0 .. count-1                          */
    uint8_t   target;        /* NG_PALFX_WHITE or NG_PALFX_BLACK            */
    uint8_t   level;         /* 0 the true colours .. 16 all target         */
    uint8_t   fade;          /* NG_PALFX_FADE_IN, NG_PALFX_FADE_OUT or 0    */
    uint8_t   frames;        /* frames the fade has still to run            */
    uint8_t   all_dirty;     /* every bank changed since the last blank     */
    uint8_t   backdrop_set;
    uint16_t  backdrop;
    uint16_t  progress;      /* 8.8, 0 .. 16: how far the fade has gone     */
    uint16_t  step;          /* added to progress each frame                */
    uint8_t   dirty[32];     /* a bit per bank changed since the last blank */
    /* Each channel's 5-bit value at `level`, already in its place in the
     * palette word: red at lut[0..31], green at [32..63], blue at [64..95]. */
    uint16_t  lut[96];
} ng_palfx_scr;

/*
 * Word copy, kept in assembly on purpose: as a C loop GCC can fold it into
 * "move.w (a0)+,(0,a0,d0.l)", whose destination a 68000 works out with the
 * already incremented a0, so every word lands one entry along. `n` >= 1.
 */
__attribute__((noinline))
static void NEOGEO_USER palfx_copy_words(volatile uint16_t *dst, const volatile uint16_t *src, uint16_t n)
{
    n = (uint16_t)(n - 1u);
    __asm__ volatile (
        "1:\n\t"
        "move.w (%0)+,(%1)+\n\t"
        "dbf %2,1b"
        : "+a" (src), "+a" (dst), "+d" (n)
        :
        : "memory");
}

/*
 * n colours through the level's tables. Each channel's 5-bit value (four
 * bits in its nibble, the lowest in bit 14, 13 or 12) is found by shifts
 * and a bit test, looked up, and the three results ORed: no multiply or
 * divide per colour. `n` >= 1.
 */
__attribute__((noinline))
static void NEOGEO_USER palfx_screen_blend(uint16_t *dst, const uint16_t *src, uint16_t n)
{
    const uint16_t *green = &ng_palfx_scr.lut[32];
    uint16_t c, i, w;
    n = (uint16_t)(n - 1u);
    __asm__ volatile (
        "1:\n\t"
        "move.w (%[src])+,%[c]\n\t"
        "move.w %[c],%[i]\n\t"          /* red: bits 11..8 and 14 */
        "lsr.w #6,%[i]\n\t"
        "andi.w #0x3C,%[i]\n\t"
        "btst #14,%[c]\n\t"
        "beq.s 2f\n\t"
        "addq.w #2,%[i]\n"
        "2:\n\t"
        "move.w -64(%[t],%[i].w),%[w]\n\t"
        "move.w %[c],%[i]\n\t"          /* green: bits 7..4 and 13 */
        "lsr.w #2,%[i]\n\t"
        "andi.w #0x3C,%[i]\n\t"
        "btst #13,%[c]\n\t"
        "beq.s 3f\n\t"
        "addq.w #2,%[i]\n"
        "3:\n\t"
        "or.w 0(%[t],%[i].w),%[w]\n\t"
        "move.w %[c],%[i]\n\t"          /* blue: bits 3..0 and 12 */
        "lsl.w #2,%[i]\n\t"
        "andi.w #0x3C,%[i]\n\t"
        "btst #12,%[c]\n\t"
        "beq.s 4f\n\t"
        "addq.w #2,%[i]\n"
        "4:\n\t"
        "or.w 64(%[t],%[i].w),%[w]\n\t"
        "move.w %[w],(%[dst])+\n\t"
        "dbf %[n],1b"
        : [src] "+a" (src), [dst] "+a" (dst), [n] "+d" (n),
          [c] "=&d" (c), [i] "=&d" (i), [w] "=&d" (w)
        : [t] "a" (green)
        : "cc", "memory");
}

/* The tables for the current level and target: each channel moves
 * level/16 of the way from its value to the target's (31 or 0). */
static void NEOGEO_USER palfx_screen_tables(void)
{
    uint8_t v, l;
    uint8_t k = ng_palfx_scr.level;
    int16_t p = ng_palfx_scr.target == NG_PALFX_WHITE ? (int16_t)(((int16_t)k << 5) - k) : 0;
    for (v = 0; v < 32u; v++) {
        /* p = (target - v) * level, stepped down by `level` each time */
        l = (uint8_t)(v + (p >> 4));
        ng_palfx_scr.lut[v]       = (uint16_t)(((uint16_t)(l & 1u) << 14) | ((uint16_t)(l >> 1) << 8));
        ng_palfx_scr.lut[32u + v] = (uint16_t)(((uint16_t)(l & 1u) << 13) | ((uint16_t)(l >> 1) << 4));
        ng_palfx_scr.lut[64u + v] = (uint16_t)(((uint16_t)(l & 1u) << 12) | (uint16_t)(l >> 1));
        p = (int16_t)(p - k);
    }
}

/* One bank's colours, at the fade's level, ready for the next blank. */
static void NEOGEO_USER palfx_screen_show(uint8_t bank, const uint16_t *colors)
{
    uint16_t *out = ng_palfx_scr.out + ((uint16_t)bank << 4);
    if (ng_palfx_scr.level) palfx_screen_blend(out, colors, 16u);
    else palfx_copy_words(out, colors, 16u);
    ng_palfx_scr.dirty[bank >> 3] |= (uint8_t)(1u << (bank & 7u));
}

/* Every bank again at `level`: all of them go on screen at the next blank,
 * so the picture never shows two brightnesses at once. */
static void NEOGEO_USER palfx_screen_level(uint8_t level, uint8_t force)
{
    uint16_t n = (uint16_t)((uint16_t)ng_palfx_scr.count << 4);
    if (level > 16u) level = 16u;
    if (level == ng_palfx_scr.level && !force) return;
    ng_palfx_scr.level = level;
    if (level) {
        palfx_screen_tables();
        palfx_screen_blend(ng_palfx_scr.out, ng_palfx_scr.base, n);
    } else {
        palfx_copy_words(ng_palfx_scr.out, ng_palfx_scr.base, n);
    }
    ng_palfx_scr.all_dirty = 1;
}

/* The fade's next frame; run by ng_palette_fx_update() before the effects,
 * so an effect's colours are laid over the fade's, not under them. */
static void NEOGEO_USER palfx_screen_step(void)
{
    uint8_t part;
    if (!ng_palfx_scr.fade) return;
    if (--ng_palfx_scr.frames == 0) {
        part = 16u;
    } else {
        ng_palfx_scr.progress = (uint16_t)(ng_palfx_scr.progress + ng_palfx_scr.step);
        part = (uint8_t)(ng_palfx_scr.progress >> 8);
        if (part > 16u) part = 16u;
    }
    palfx_screen_level(ng_palfx_scr.fade == NG_PALFX_FADE_OUT ? part : (uint8_t)(16u - part), 0);
    if (!ng_palfx_scr.frames) ng_palfx_scr.fade = 0;
}

static void NEOGEO_USER palfx_screen_start(uint8_t fade, uint8_t duration)
{
    if (!duration) duration = 1;
    ng_palfx_scr.fade = fade;
    ng_palfx_scr.frames = duration;
    ng_palfx_scr.progress = 0;
    /* 16 levels over `duration` frames, rounded up (one divide, here only) */
    ng_palfx_scr.step = (uint16_t)((uint16_t)(4096u + duration - 1u) / (uint16_t)duration);
}

/* An effect's colours for a bank: into the screen when it holds the bank. */
static void NEOGEO_USER palfx_put(uint8_t bank, const uint16_t *pal)
{
    if (bank < ng_palfx_scr.count) palfx_screen_show(bank, pal);
    else ng_rq_palette_upload(bank, pal);
}

void NEOGEO_USER ng_palfx_screen_init(uint16_t *base, uint16_t *out, uint8_t count)
{
    uint8_t i;
    uint16_t n = (uint16_t)((uint16_t)count << 4);
    ng_palfx_scr.count = 0;
    if (!base || !out || !count) return;
    ng_palfx_scr.base = base;
    ng_palfx_scr.out = out;
    palfx_copy_words(base, (const volatile uint16_t *)PALETTES, n);
    palfx_copy_words(out, base, n);
    ng_palfx_scr.count = count;
    ng_palfx_scr.target = NG_PALFX_WHITE;
    ng_palfx_scr.level = 0;
    ng_palfx_scr.fade = 0;
    ng_palfx_scr.all_dirty = 0;
    ng_palfx_scr.backdrop_set = 0;
    for (i = 0; i < 32u; i++) ng_palfx_scr.dirty[i] = 0;
}

void NEOGEO_USER ng_palfx_screen_load(uint8_t bank, const uint16_t *colors)
{
    uint16_t *base;
    if (bank >= ng_palfx_scr.count || !colors) return;
    base = ng_palfx_scr.base + ((uint16_t)bank << 4);
    palfx_copy_words(base, colors, 16u);
    palfx_screen_show(bank, base);
}

const uint16_t * NEOGEO_USER ng_palfx_screen_colors(uint8_t bank)
{
    return bank < ng_palfx_scr.count ? ng_palfx_scr.base + ((uint16_t)bank << 4) : 0;
}

void NEOGEO_USER ng_palfx_screen_backdrop(uint16_t color)
{
    ng_palfx_scr.backdrop = color;
    ng_palfx_scr.backdrop_set = 1;
}

void NEOGEO_USER ng_palfx_screen_fade_out(uint8_t target, uint8_t duration)
{
    if (!ng_palfx_scr.count) return;
    palfx_screen_start(NG_PALFX_FADE_OUT, duration);
    if (target != ng_palfx_scr.target) {
        ng_palfx_scr.target = target;
        if (ng_palfx_scr.level) palfx_screen_level(ng_palfx_scr.level, 1);
    }
}

void NEOGEO_USER ng_palfx_screen_fade_in(uint8_t target, uint8_t duration)
{
    if (!ng_palfx_scr.count) return;
    palfx_screen_start(NG_PALFX_FADE_IN, duration);
    ng_palfx_scr.target = target;
    palfx_screen_level(16u, 1);
}

void NEOGEO_USER ng_palfx_screen_stop(void)
{
    if (!ng_palfx_scr.count) return;
    ng_palfx_scr.fade = 0;
    palfx_screen_level(0, 0);
}

uint8_t NEOGEO_USER ng_palfx_screen_fading(void)
{
    return (uint8_t)(ng_palfx_scr.fade != 0);
}

void NEOGEO_USER ng_palfx_vblank(void)
{
    uint8_t i, bits, bank;
    uint8_t bytes = (uint8_t)((ng_palfx_scr.count + 7u) >> 3);
    if (ng_palfx_scr.backdrop_set) {
        ng_palfx_scr.backdrop_set = 0;
        setBACKDROP(ng_palfx_scr.backdrop);
    }
    if (ng_palfx_scr.all_dirty) {
        ng_palfx_scr.all_dirty = 0;
        for (i = 0; i < bytes; i++) ng_palfx_scr.dirty[i] = 0;
        palfx_copy_words((volatile uint16_t *)PALETTES, ng_palfx_scr.out,
                         (uint16_t)((uint16_t)ng_palfx_scr.count << 4));
        return;
    }
    for (i = 0; i < bytes; i++) {
        bits = ng_palfx_scr.dirty[i];
        if (!bits) continue;
        ng_palfx_scr.dirty[i] = 0;
        for (bank = (uint8_t)(i << 3); bits; bits >>= 1, bank++) {
            if (bits & 1u)
                palfx_copy_words((volatile uint16_t *)PALETTES + ((uint16_t)bank << 4),
                                 ng_palfx_scr.out + ((uint16_t)bank << 4), 16u);
        }
    }
}
#else
/* Without the screen an effect's colours go straight to the render queue. */
#define palfx_put(bank, pal) ng_rq_palette_upload((bank), (pal))
#endif

static void NEOGEO_USER palfx_blend_to(uint16_t *out, const uint16_t *base,
                                      uint8_t r, uint8_t g, uint8_t b, uint8_t blend)
{
    ng_palette_tint_colors(out, base, r, g, b, blend);
}

static void NEOGEO_USER palfx_scale(uint16_t *out, const uint16_t *base, uint8_t brightness)
{
    ng_palette_scale_colors(out, base, brightness);
}

static NGPalFxSlot * NEOGEO_USER palfx_find_or_alloc(uint8_t palette_slot)
{
    uint8_t i;
    uint8_t free_i = 0xff;

    /* First check if the slot is already being managed */
    for (i = 0; i < NG_PALFX_MAX_SLOTS; i++) {
        if (ng_palfx_slots[i].active && ng_palfx_slots[i].palette_slot == palette_slot)
            return &ng_palfx_slots[i];
        if (!ng_palfx_slots[i].active && free_i == 0xff)
            free_i = i;
    }

    if (free_i == 0xff) return 0; /* pool full */

    ng_palfx_slots[free_i].active       = 1;
    ng_palfx_slots[free_i].palette_slot = palette_slot;
    return &ng_palfx_slots[free_i];
}

void NEOGEO_USER ng_palette_fx_init(void)
{
    uint8_t i;

    for (i = 0; i < NG_PALFX_MAX_SLOTS; i++) {
        ng_palfx_slots[i].active = 0;
    }
}

void NEOGEO_USER ng_palette_load_bank(uint8_t palette_slot, const uint16_t *pal)
{
    if (!pal) return;
    load_palettes((uint16_t *)pal,
                  (uintptr_t)((uint32_t)PALETTES +
                              ((uint32_t)PALOFFSET * (uint32_t)palette_slot)));
}

uint8_t NEOGEO_USER ng_palette_load_asset(const NGPaletteAsset *assets,
                                          uint16_t count,
                                          uint16_t asset_id)
{
    uint16_t i;

    if (!assets) return 0u;

    if (asset_id != 0u && asset_id <= count &&
        assets[asset_id - 1u].asset_id == asset_id) {
        ng_palette_load_bank(assets[asset_id - 1u].palette_slot,
                              assets[asset_id - 1u].colors);
        return 1u;
    }

    for (i = 0u; i < count; i++) {
        if (assets[i].asset_id == asset_id) {
            ng_palette_load_bank(assets[i].palette_slot, assets[i].colors);
            return 1u;
        }
    }

    return 0u;
}

void NEOGEO_USER ng_palfx_upload_base(uint8_t palette_slot, const uint16_t *pal)
{
    if (!pal) return;
#if NG_PALFX_SCREEN
    if (palette_slot < ng_palfx_scr.count) {
        ng_palfx_screen_load(palette_slot, pal);
        return;
    }
#endif
    ng_rq_palette_upload(palette_slot, pal);
}

void NEOGEO_USER ng_palfx_fade_in(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = base_pal ? palfx_find_or_alloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type   = NG_PALFX_FADE_IN;
    s->base_pal  = base_pal;
    s->timer     = 0;
    s->duration  = duration ? duration : 1;
}

void NEOGEO_USER ng_palfx_fade_out(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = base_pal ? palfx_find_or_alloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type   = NG_PALFX_FADE_OUT;
    s->base_pal  = base_pal;
    s->timer     = 0;
    s->duration  = duration ? duration : 1;
}

void NEOGEO_USER ng_palfx_flash_white(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = base_pal ? palfx_find_or_alloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type   = NG_PALFX_FLASH_WHITE;
    s->base_pal  = base_pal;
    s->timer     = duration ? duration : 4;
    s->duration  = s->timer;
}

void NEOGEO_USER ng_palfx_flash_red(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = base_pal ? palfx_find_or_alloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type   = NG_PALFX_FLASH_RED;
    s->base_pal  = base_pal;
    s->timer     = duration ? duration : 6;
    s->duration  = s->timer;
}

void NEOGEO_USER ng_palfx_flash_blue(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = base_pal ? palfx_find_or_alloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type   = NG_PALFX_FLASH_BLUE;
    s->base_pal  = base_pal;
    s->timer     = duration ? duration : 6;
    s->duration  = s->timer;
}

void NEOGEO_USER ng_palfx_pulse(uint8_t palette_slot, const uint16_t *base_pal, uint8_t period)
{
    NGPalFxSlot *s = base_pal ? palfx_find_or_alloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type   = NG_PALFX_PULSE;
    s->base_pal  = base_pal;
    s->timer     = 0;
    s->duration  = period ? (period < 2u ? 2u : period) : 16;
}

void NEOGEO_USER ng_palfx_cycle(uint8_t palette_slot, const uint16_t *base_pal,
                                  uint8_t start, uint8_t end)
{
    NGPalFxSlot *s = base_pal ? palfx_find_or_alloc(palette_slot) : 0;
    if (!s) return;
    if (start == 0u || start > 15u) start = 1u;
    if (end >= 16) end = 15;
    if (end < start) end = start;
    s->fx_type    = NG_PALFX_CYCLE;
    s->base_pal   = base_pal;
    s->cycle_start = start;
    s->cycle_end   = end;
    s->cycle_pos   = 0;
    s->timer       = 0;
    s->duration    = 1;
}

void NEOGEO_USER ng_palfx_stop(uint8_t palette_slot)
{
    uint8_t i;

    for (i = 0; i < NG_PALFX_MAX_SLOTS; i++) {
        if (ng_palfx_slots[i].active && ng_palfx_slots[i].palette_slot == palette_slot) {
            /* Restore base palette */
            if (ng_palfx_slots[i].base_pal)
                palfx_put(palette_slot, ng_palfx_slots[i].base_pal);
            ng_palfx_slots[i].active = 0;
            return;
        }
    }
}

uint8_t NEOGEO_USER ng_palfx_active(uint8_t palette_slot)
{
    uint8_t i;
    for (i = 0; i < NG_PALFX_MAX_SLOTS; i++) {
        if (ng_palfx_slots[i].active && ng_palfx_slots[i].palette_slot == palette_slot)
            return 1;
    }
    return 0;
}

void NEOGEO_USER ng_palette_fx_update(void)
{
    uint8_t i;

#if NG_PALFX_SCREEN
    palfx_screen_step();
#endif

    for (i = 0; i < NG_PALFX_MAX_SLOTS; i++) {
        NGPalFxSlot *s = &ng_palfx_slots[i];

        if (!s->active || !s->base_pal) continue;

        switch (s->fx_type) {

        case NG_PALFX_FADE_IN: {
            /*
             * Fade in: brightness goes from 0 to 255 over `duration` frames.
             * brightness = (timer * 255) / duration — use shift approximation.
             */
            uint16_t bright_n;
            uint8_t  brightness;

            s->timer++;
            bright_n   = (uint16_t)((uint16_t)s->timer * 255u);
            brightness = (uint8_t)(bright_n / (uint16_t)s->duration);

            palfx_scale(s->work_pal, s->base_pal, brightness);
            palfx_put(s->palette_slot, s->work_pal);

            if (s->timer >= s->duration) {
                palfx_put(s->palette_slot, s->base_pal);
                s->active = 0;
            }
            break;
        }

        case NG_PALFX_FADE_OUT: {
            uint16_t bright_n;
            uint8_t  brightness;

            s->timer++;
            bright_n   = (uint16_t)((uint16_t)(s->duration - s->timer) * 255u);
            if (s->timer >= s->duration) {
                brightness = 0;
            } else {
                brightness = (uint8_t)(bright_n / (uint16_t)s->duration);
            }

            palfx_scale(s->work_pal, s->base_pal, brightness);
            palfx_put(s->palette_slot, s->work_pal);

            if (s->timer >= s->duration) {
                s->active = 0;
            }
            break;
        }

        case NG_PALFX_FLASH_WHITE: {
            /* Blend from base towards white; intensity decays over duration */
            uint8_t blend;

            blend = (uint8_t)(((uint16_t)s->timer * 255u) / (uint16_t)s->duration);
            /* White: R=31, G=31, B=31 → pass as scaled 0..255: 255 */
            palfx_blend_to(s->work_pal, s->base_pal, 255, 255, 255, blend);
            palfx_put(s->palette_slot, s->work_pal);

            if (s->timer == 0) {
                palfx_put(s->palette_slot, s->base_pal);
                s->active = 0;
            } else {
                s->timer--;
            }
            break;
        }

        case NG_PALFX_FLASH_RED: {
            uint8_t blend;

            blend = (uint8_t)(((uint16_t)s->timer * 200u) / (uint16_t)s->duration);
            palfx_blend_to(s->work_pal, s->base_pal, 255, 0, 0, blend);
            palfx_put(s->palette_slot, s->work_pal);

            if (s->timer == 0) {
                palfx_put(s->palette_slot, s->base_pal);
                s->active = 0;
            } else {
                s->timer--;
            }
            break;
        }

        case NG_PALFX_FLASH_BLUE: {
            uint8_t blend;

            blend = (uint8_t)(((uint16_t)s->timer * 180u) / (uint16_t)s->duration);
            palfx_blend_to(s->work_pal, s->base_pal, 0, 64, 255, blend);
            palfx_put(s->palette_slot, s->work_pal);

            if (s->timer == 0) {
                palfx_put(s->palette_slot, s->base_pal);
                s->active = 0;
            } else {
                s->timer--;
            }
            break;
        }

        case NG_PALFX_PULSE: {
            /*
             * Brightness pulse: sine-wave brightness using triangle approximation.
             * sin_tab index: (timer * 64) / duration maps 0..duration → 0..64 (quarter period).
             * Amplitude oscillates 128..255.
             */
            uint8_t phase;
            uint8_t brightness;
            uint8_t half = (uint8_t)(s->duration >> 1);

            s->timer++;
            if (s->timer >= s->duration) s->timer = 0;

            /* Triangle wave: ramp up then ramp down */
            if (s->timer < half) {
                phase = (uint8_t)((uint16_t)s->timer * 127u / (uint16_t)half);
            } else {
                phase = (uint8_t)(127u - (uint16_t)(s->timer - half) * 127u / (uint16_t)(s->duration - half));
            }
            brightness = (uint8_t)(128 + phase);

            palfx_scale(s->work_pal, s->base_pal, brightness);
            palfx_put(s->palette_slot, s->work_pal);
            /* Pulse is perpetual — slot stays active */
            break;
        }

        case NG_PALFX_CYCLE: {
            /*
             * Colour cycle: copy base palette, then rotate entries [start..end] by cycle_pos.
             * One entry rotated per frame for smooth cycling without bandwidth spike.
             */
            uint8_t j;
            uint8_t len = (uint8_t)(s->cycle_end - s->cycle_start + 1);

            for (j = 0; j < 16; j++) s->work_pal[j] = s->base_pal[j];

            /* Rotate: work_pal[start + k] = base_pal[start + (k + pos) % len] */
            for (j = 0; j < len; j++) {
                uint8_t src = (uint8_t)(s->cycle_start + ((j + s->cycle_pos) % len));
                s->work_pal[s->cycle_start + j] = s->base_pal[src];
            }

            palfx_put(s->palette_slot, s->work_pal);

            s->cycle_pos++;
            if (s->cycle_pos >= len) s->cycle_pos = 0;
            break;
        }

        default:
            s->active = 0;
            break;
        }
    }
}
