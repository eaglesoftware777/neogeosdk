#include "ng_fix.h"
#include "ng_sprite_group.h"
#include "ng_perf.h"
#include "neogeo.h"

/*
 * What each cell holds, as the map word itself, in the map's own order:
 * cell (x, y) is word x * 32 + y, at FIXMAP + 2 + that. One compare tells
 * whether a write would change anything. NG_FIX_STALE never matches a word
 * a cell can be given, so a stale cell is always written.
 */
#define NG_FIX_STALE 0xFFFFu
#define NG_FIX_BLANK 0x00FFu
#define NG_FIX_CELL(x, y) ((uint16_t)(((uint16_t)(x) << 5) + (y)))
static uint16_t ng_fix_words[NG_FIX_WIDTH * NG_FIX_HEIGHT];
static uint16_t ng_fix_ascii_base;

#ifdef NG_VRAM_DEFER
/*
 * NG_VRAM_DEFER: what changed waits for ng_fix_commit() in the vertical
 * blank (ng_vram_commit() calls it), as runs: cells 32 words apart -- a
 * row of text, left to right -- each run one entry. The commit writes a
 * run's cells as they are then (a cell changed twice in a frame shows its
 * last word), and only while the blank has time (ng_vram_lines_left): the
 * rest wait, in order, for the next one. When the list is full a change
 * marks its text row instead, and a marked row is written whole from what
 * the cells hold -- nothing is ever written at once while the picture is
 * drawn. A clear of the whole layer drops what was waiting.
 */
#define NG_FIX_QUEUE 64u
typedef struct { uint16_t first, next, n, spare; } NGFixRun;   /* 8 bytes: a shift to index */
static NGFixRun ng_fix_q[NG_FIX_QUEUE];
static NGFixRun *ng_fix_tail;    /* one past the last run (ng_fix_queue_drop sets it) */
static uint32_t ng_fix_rows;     /* text rows to write whole: a bit a row */

static uint8_t ng_fix_last_n;
uint8_t NEOGEO_USER ng_fix_last_commit_cells(void) { return ng_fix_last_n; }

/* A run's address is set once and the chip steps it after each word. */
void NEOGEO_USER ng_fix_commit(void)
{
    NGFixRun *r = ng_fix_q, *end, *keep = ng_fix_q;
    uint16_t cells = 0;
    if (ng_fix_tail == 0) ng_fix_tail = ng_fix_q;
    end = ng_fix_tail;
    if ((r == end && !ng_fix_rows) || !ng_vram_lines_left()) { ng_fix_last_n = 0; return; }
    NEO_REGISTER(VRAM_INC) = 0x20;
    for (; r < end; r++) {
        const uint16_t *w = &ng_fix_words[r->first];
        uint16_t n = r->n;
        if (ng_vram_lines_left() < (uint8_t)(1u + (n >> 4))) {   /* some 20 cells a line */
            while (r < end) *keep++ = *r++;                       /* the next blank, in order */
            break;
        }
        cells = (uint16_t)(cells + n);
        NEO_REGISTER(VRAM_ADDR) = (uint16_t)(FIXMAP + 2u + r->first);
        while (n--) { NEO_REGISTER(VRAM_RW) = *w; w += 32; }
    }
    ng_fix_tail = keep;
    if (ng_fix_rows) {
        uint8_t y;
        uint16_t rows = 0;
        for (y = 0; y < NG_FIX_HEIGHT; y++) {
            const uint16_t *w;
            uint8_t x;
            if (!(ng_fix_rows & ((uint32_t)1u << y))) continue;
            if (ng_vram_lines_left() < 3u) break;                 /* (a row is 40 cells) */
            ng_fix_rows &= ~((uint32_t)1u << y);
            w = &ng_fix_words[y];
            NEO_REGISTER(VRAM_ADDR) = (uint16_t)(FIXMAP + 2u + y);
            for (x = 0; x < NG_FIX_WIDTH; x++, w += 32) NEO_REGISTER(VRAM_RW) = *w;
            cells = (uint16_t)(cells + NG_FIX_WIDTH);
            rows++;
        }
        NG_PERF_FIX_ROWS(rows);
        (void)rows;
    }
    NG_PERF_VRAM(cells);
    ng_fix_last_n = (uint8_t)(cells > 255u ? 255u : cells);
}

void NEOGEO_USER ng_fix_queue_drop(void)
{
    ng_fix_tail = ng_fix_q;
    ng_fix_rows = 0;
}

/* Cell i changed: it extends the last run when it is that run's next cell;
 * with the list full, its row is marked to be written whole. */
static void NEOGEO_USER ng_fix_write(uint16_t i)
{
    NGFixRun *r = ng_fix_tail;
    if (r == 0) r = ng_fix_tail = ng_fix_q;
    if (r != ng_fix_q && r[-1].next == i) {
        r[-1].n++;
        r[-1].next = (uint16_t)(i + 32u);
        return;
    }
    if (r == &ng_fix_q[NG_FIX_QUEUE]) {
        ng_fix_rows |= (uint32_t)1u << (i & 31u);
        return;
    }
    r->first = i;
    r->next = (uint16_t)(i + 32u);
    r->n = 1;
    ng_fix_tail = r + 1;
}
#else
#define ng_fix_write(i) do { NG_PERF_VRAM(1); vram_sfix(0x20, (uint16_t)(FIXMAP + 2u + (i)), ng_fix_words[i]); } while (0)
#endif

/* Give cell i its word unless it already has it. */
#define NG_FIX_SET(i, word) do { \
        uint16_t w_ = (word); \
        if (ng_fix_words[i] != w_) { \
            ng_fix_words[i] = w_; \
            ng_fix_write(i); \
        } \
    } while (0)

void NEOGEO_USER ng_fix_set_ascii_base(uint16_t tile_base)
{
    if (tile_base > 0xF00u || (tile_base & 0xFFu)) return;
    ng_fix_ascii_base = tile_base;
    ng_fix_invalidate_all();
}

void NEOGEO_USER ng_fix_blank_cell(uint8_t x, uint8_t y)
{
    uint16_t i;

    if (x >= NG_FIX_WIDTH || y >= NG_FIX_HEIGHT) return;

    /*
     * The FIX layer is transparent at color index 0, but ASCII ' ' is still
     * a real tile number.  Some S-ROMs have visible pixels in that tile.
     * Use tile $00FF for an explicit blank cell, as BIOS clear routines do.
     * (+2 in the address: rows 0/1 of the 32-row FIX map are in vertical
     * blanking, so visible row y is map row y + 2, as fixtext_out() has it.)
     */
    i = NG_FIX_CELL(x, y);
    NG_FIX_SET(i, NG_FIX_BLANK);
}

void NEOGEO_USER ng_fix_init(void)
{
#ifdef NG_VRAM_DEFER
    ng_fix_queue_drop();
#endif
    ng_fix_invalidate_all();
}

void NEOGEO_USER ng_fix_invalidate_all(void)
{
    uint16_t i;
    for (i = 0; i < NG_FIX_WIDTH * NG_FIX_HEIGHT; i++) ng_fix_words[i] = NG_FIX_STALE;
}

void NEOGEO_USER ng_fix_clear(void)
{
    uint16_t i;
#ifdef NG_VRAM_DEFER
    ng_fix_queue_drop();   /* (a scene-change clear: behind a fade) */
#endif
    clearFix();
    for (i = 0; i < NG_FIX_WIDTH * NG_FIX_HEIGHT; i++) ng_fix_words[i] = NG_FIX_BLANK;
}

void NEOGEO_USER ng_fix_clear_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pal)
{
    uint8_t ix;

    NG_UNUSED(pal);

    if (x >= NG_FIX_WIDTH || y >= NG_FIX_HEIGHT) return;
    if (w > (uint8_t)(NG_FIX_WIDTH - x)) w = (uint8_t)(NG_FIX_WIDTH - x);
    if (h > (uint8_t)(NG_FIX_HEIGHT - y)) h = (uint8_t)(NG_FIX_HEIGHT - y);

    /* Row by row, as before: a cell's write still lands in the same order. */
    for (; h; h--, y++) {
        uint16_t i = NG_FIX_CELL(x, y);
        for (ix = w; ix; ix--, i += 32u) NG_FIX_SET(i, NG_FIX_BLANK);
    }
}

void NEOGEO_USER ng_fix_putc(uint8_t x, uint8_t y, char ch, uint8_t pal)
{
    uint16_t i;

    if (x >= NG_FIX_WIDTH || y >= NG_FIX_HEIGHT) return;

    i = NG_FIX_CELL(x, y);
    /* FIX can only address the first 16 palettes. A space is the blank
     * tile, whatever its palette. */
    NG_FIX_SET(i, ch == ' ' ? NG_FIX_BLANK
                            : (uint16_t)(((uint16_t)(pal & 0x0f) << 12) | (uint16_t)(ng_fix_ascii_base + (uint8_t)ch)));
}

/*
 * Any FIX tile by its full 12-bit number, not just the first 256 a char
 * can name -- for HUD pieces drawn into the free space of a game's own S1
 * ROM.
 */
void NEOGEO_USER ng_fix_put_tile(uint8_t x, uint8_t y, uint16_t tile, uint8_t pal)
{
    uint16_t i;
    if (x >= NG_FIX_WIDTH || y >= NG_FIX_HEIGHT) return;
    i = NG_FIX_CELL(x, y);
    NG_FIX_SET(i, (uint16_t)(((uint16_t)(pal & 0x0f) << 12) | (tile & 0x0fffu)));
}

void NEOGEO_USER ng_fix_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    uint16_t i, base;
    char ch;

    if (!text || y >= NG_FIX_HEIGHT || x >= NG_FIX_WIDTH) return;

    i = NG_FIX_CELL(x, y);
    base = (uint16_t)(((uint16_t)(pal & 0x0f) << 12) + ng_fix_ascii_base);
    while ((ch = *text++) != 0) {
        NG_FIX_SET(i, ch == ' ' ? NG_FIX_BLANK : (uint16_t)(base + (uint8_t)ch));
        if (++x >= NG_FIX_WIDTH) break;
        i += 32u;
    }
}

static void NEOGEO_USER ng_fix_put_digit(uint8_t x, uint8_t y, uint8_t digit, uint8_t pal)
{
    ng_fix_putc(x, y, (char)('0' + digit), pal);
}

static uint8_t NEOGEO_USER ng_fix_emit_dec32(uint8_t x, uint8_t y, uint32_t value, uint8_t pal)
{
    static const uint32_t pow10[10] = {
        1000000000UL, 100000000UL, 10000000UL, 1000000UL, 100000UL,
        10000UL, 1000UL, 100UL, 10UL, 1UL
    };

    uint8_t i;
    uint8_t started = 0;
    uint8_t cursor = x;

    for (i = 0; i < 10; i++) {
        uint8_t digit = 0;

        while (value >= pow10[i]) {
            value -= pow10[i];
            digit++;
        }

        if (digit || started || i == 9) {
            if (cursor >= NG_FIX_WIDTH) break;
            ng_fix_put_digit(cursor, y, digit, pal);
            cursor++;
            started = 1;
        }
    }

    return (uint8_t)(cursor - x);
}

void NEOGEO_USER ng_fix_put_u16(uint8_t x, uint8_t y, uint16_t value, uint8_t pal, uint16_t tile_offset)
{
    uint8_t used;
    uint8_t i;

    NG_UNUSED(tile_offset);

    if (x >= NG_FIX_WIDTH || y >= NG_FIX_HEIGHT) return;

    used = ng_fix_emit_dec32(x, y, (uint32_t)value, pal);

    for (i = used; i < 5; i++) {
        if ((uint8_t)(x + i) >= NG_FIX_WIDTH) break;
        ng_fix_blank_cell((uint8_t)(x + i), y);
    }
}

void NEOGEO_USER ng_fix_put_u32(uint8_t x, uint8_t y, uint32_t value, uint8_t pal, uint16_t tile_offset)
{
    uint8_t used;
    uint8_t i;

    NG_UNUSED(tile_offset);

    if (x >= NG_FIX_WIDTH || y >= NG_FIX_HEIGHT) return;

    used = ng_fix_emit_dec32(x, y, value, pal);

    for (i = used; i < 10; i++) {
        if ((uint8_t)(x + i) >= NG_FIX_WIDTH) break;
        ng_fix_blank_cell((uint8_t)(x + i), y);
    }
}
