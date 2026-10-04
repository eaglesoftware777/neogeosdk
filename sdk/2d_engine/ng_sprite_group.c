#include "macro.h"
#include "neogeo.h"
#include "ng_sprite_group.h"
#include "ng_sprite_pool.h"
#include "ng_vram.h"
#include "ng_sprite_hw.h"
#include "ng_perf.h"
#include "ng_fix.h"
#ifdef NG_VRAM_DEFER
#include "ng_render_queue.h"
#endif

/*
 * Built at -O2 while the rest of the tree may be -O0.
 *
 * These are the routines a frame runs most: the map fill below runs over
 * up to sixteen rows for every strip of every sprite whose art changed,
 * and the position writes run for every sprite that moved.  The writes to
 * the video RAM ports are in the assembly helpers below, so the compiler
 * can neither reorder, merge nor elide one.
 */
#pragma GCC optimize ("O2")

/*
 * VRAM writes, straight at the LSPC's ports (VRAM_ADDR, VRAM_RW, VRAM_INC
 * at +0, +2, +4).  Each write to VRAM_RW lands at least 12 clocks after the
 * write before it, the pace the SDK's other VRAM loops keep (vram_SCB1 is a
 * memory-to-memory move); the compiler would be free to pack two register
 * stores into back-to-back 8-clock moves, which is why these are assembly.
 *
 * A run of sprites sits at consecutive addresses in each of SCB1 (64 words
 * a sprite), SCB2, SCB3 and SCB4, so a run is one address and a stream of
 * words with the auto-increment at 1.
 *
 * A host build (the unit tests) makes the same writes through the vram_*
 * calls, which the tests stand in for.
 */
#ifdef __m68k__

/* One word at `addr`. */
static inline void NEOGEO_USER ngsg_put(uint16_t addr, uint16_t v)
{
    NG_PERF_VRAM(1);
    __asm__ volatile (
        "move.w %[addr],(%[p])\n\t"    /* VRAM_ADDR                 */
        "move.w %[v],2(%[p])"            /* VRAM_RW, 12 clocks on     */
        :
        : [p] "a" (VRAM_ADDR), [addr] "d" (addr), [v] "d" (v)
        : "memory");
}

/* `first` at `addr`, then n - 1 copies of `rest` at the words after it. */
static inline void NEOGEO_USER ngsg_fill(uint16_t addr, uint16_t first, uint16_t rest, uint16_t n)
{
    NG_PERF_VRAM(n);
    if (!n) return;
    __asm__ volatile (
        "move.w %[addr],(%[p])\n\t"    /* VRAM_ADDR                 */
        "move.w #1,4(%[p])\n\t"        /* VRAM_INC = 1              */
        "move.w %[first],2(%[p])\n\t"  /* VRAM_RW                   */
        "subq.w #1,%[n]\n\t"
        "bra.s 2f\n"
        "1:\n\t"
        "move.w %[rest],2(%[p])\n"      /* 12 clocks, then the dbf   */
        "2:\n\t"
        "dbf %[n],1b"
        : [n] "+&d" (n)
        : [p] "a" (VRAM_ADDR), [addr] "d" (addr), [first] "d" (first), [rest] "d" (rest)
        : "cc", "memory");
}

/*
 * One strip's SCB1 map at `addr`: `art` rows of (tile, attr), the
 * tile stepping by `step` a row, then `blank` rows of the transparent tile.
 * With a bank map `pm`, stepping with the tile, each row's attribute takes
 * its palette bank from the map over attr's low byte.
 */
/* (always inlined: called a strip at a time, the call itself -- seven
 * words pushed -- cost four times a short strip's writes) */
static inline __attribute__((always_inline)) void ngsg_put_strip(uint16_t addr, uint16_t tile, int16_t step,
                                                                 uint16_t attr, const uint8_t *pm,
                                                                 uint16_t art, uint16_t blank)
{
    volatile uint16_t *p = (volatile uint16_t *)VRAM_ADDR;
    uint16_t w;
    NG_PERF_VRAM(2u * (art + blank));

    __asm__ volatile (
        "move.w %[addr],(%[p])\n\t"    /* VRAM_ADDR                 */
        "move.w #1,4(%[p])\n\t"        /* VRAM_INC = 1              */
        "addq.l #2,%[p]\n\t"           /* VRAM_RW from here on      */
        "subq.w #1,%[art]\n\t"
        "bmi.s 3f\n\t"                   /* no art: all blank         */
        "cmpa.w #0,%[pm]\n\t"
        "beq.s 2f\n"
        "1:\n\t"                         /* banked rows               */
        "move.w %[tile],(%[p])\n\t"
        "move.b (%[pm]),%[w]\n\t"
        "lsl.w #8,%[w]\n\t"
        "or.b %[attr],%[w]\n\t"
        "move.w %[w],(%[p])\n\t"
        "add.w %[step],%[tile]\n\t"
        "adda.w %[step],%[pm]\n\t"
        "dbf %[art],1b\n\t"
        "bra.s 3f\n"
        "2:\n\t"                         /* one bank                  */
        "move.w %[tile],(%[p])\n\t"
        "add.w %[step],%[tile]\n\t"      /* 12 clocks to the attr     */
        "move.w %[attr],(%[p])\n\t"
        "dbf %[art],2b\n"
        "3:\n\t"
        "subq.w #1,%[blank]\n\t"
        "bmi.s 5f\n"
        "4:\n\t"                         /* the map's unused rows     */
        "move.w %[bt],(%[p])\n\t"
        "move.w %[ba],(%[p])\n\t"
        "dbf %[blank],4b\n"
        "5:"
        : [p] "+&a" (p), [pm] "+&a" (pm), [tile] "+&d" (tile), [art] "+&d" (art),
          [blank] "+&d" (blank), [w] "=&d" (w)
        : [addr] "d" (addr), [step] "d" (step), [attr] "d" (attr),
          [bt] "i" (NG_SPRITE_BLANK_TILE), [ba] "i" (NG_SPRITE_BLANK_ATTR)
        : "cc", "memory");
}
#else
static void ngsg_put(uint16_t addr, uint16_t v)
{
    vram_SCB234(addr, v);
}

static void ngsg_fill(uint16_t addr, uint16_t first, uint16_t rest, uint16_t n)
{
    if (!n) return;
    vram_init(addr, 1);
    vram_sfix1(first);
    while (--n) vram_sfix1(rest);
}

static void ngsg_put_strip(uint16_t addr, uint16_t tile, int16_t step, uint16_t attr,
                           const uint8_t *pm, uint16_t art, uint16_t blank)
{
    vram_init(addr, 1);
    for (; art; art--) {
        vram_sfix1(tile);
        vram_sfix1(pm ? (uint16_t)(((uint16_t)*pm << 8) | (attr & 0x00ffu)) : attr);
        tile = (uint16_t)(tile + step);
        if (pm) pm += step;
    }
    for (; blank; blank--) {
        vram_sfix1(NG_SPRITE_BLANK_TILE);
        vram_sfix1(NG_SPRITE_BLANK_ATTR);
    }
}
#endif

static uint8_t NEOGEO_USER ngsg_clamp_u8(uint8_t v, uint8_t min, uint8_t max)
{
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static uint8_t NEOGEO_USER ngsg_x_shrink_nibble(uint8_t xScale)
{
    /*
     * SCB2 horizontal shrink:
     *   F = full 16px width, 0 = one pixel.
     */
    if (xScale >= NG_SPRITE_FULL_XSCALE) return 0x0f;
    return (uint8_t)((xScale >> 4) & 0x0f);
}

/* The rows a group shows: its active rows, bounded by its height, then as
 * many as its vertical shrink leaves on screen (SCB3's count). */
static uint8_t NEOGEO_USER ngsg_rows(const NGSpriteGroup *g)
{
    uint8_t rows = g->activeRows ? g->activeRows : g->heightTiles;
    if (rows > g->heightTiles) rows = g->heightTiles;
    if (rows > NG_SPRITE_MAX_HEIGHT_TILES) rows = NG_SPRITE_MAX_HEIGHT_TILES;
    if (g->yScale == NG_SPRITE_FULL_YSCALE && rows) return rows;   /* no multiply */
    return ng_sprite_display_rows(rows, g->yScale);
}

/* The SCB words, as setSCB2/3/4 and setSCB1_2 build them. */
static uint16_t NEOGEO_USER ngsg_scb2(const NGSpriteGroup *g)
{
    return (uint16_t)(((uint16_t)ngsg_x_shrink_nibble(g->xScale) << 8) | g->yScale);
}

static uint16_t NEOGEO_USER ngsg_scb3(const NGSpriteGroup *g, uint8_t rows)
{
    return (uint16_t)(((uint16_t)(496 - g->y) << 7) | rows);
}

static uint16_t NEOGEO_USER ngsg_scb4(const NGSpriteGroup *g)
{
    return (uint16_t)((uint16_t)g->x << 7);
}

static uint16_t NEOGEO_USER ngsg_attr(const NGSpriteGroup *g)
{
    return (uint16_t)(((uint16_t)g->palette << 8) | (g->autoAnim8 << 3) | (g->autoAnim4 << 2) |
                      (g->vflip << 1) | g->hflip);
}

/*
 * Every strip's SCB1 map.  Rows past the art hold the transparent tile, up
 * to the rows the hardware can reach (ng_sprite_map_rows).  A strip reads
 * its page column from the far side when mirrored, and its rows bottom up
 * when flipped: the tile and the bank map then step back a row at a time.
 */
static void NEOGEO_USER ngsg_put_map(NGSpriteGroup *g, uint8_t rows, uint8_t force)
{
    uint8_t mapRows = ng_sprite_map_rows(rows);
    uint8_t art = g->heightTiles < mapRows ? g->heightTiles : mapRows;
    uint8_t blank = (uint8_t)(mapRows - art);
    uint16_t attr = ngsg_attr(g);
    int16_t step = (int16_t)g->tileStride;
    uint16_t start = 0;
    uint16_t addr = (uint16_t)(64u * g->firstSprite);
    uint8_t strip;

    if (!force && g->mapRows >= mapRows && g->mapFirst == g->firstSprite &&
        g->mapStrips == g->strips && g->mapHeight == g->heightTiles)
        blank = 0u;

    if (g->vflip) {
        start = (uint16_t)((uint16_t)(g->heightTiles - 1u) * g->tileStride);
        step = (int16_t)-step;
    }
    for (strip = 0; strip < g->strips; strip++, addr = (uint16_t)(addr + 64u)) {
        uint16_t first = (uint16_t)(start + (g->hflip ? (uint8_t)(g->strips - 1u - strip) : strip));
        ngsg_put_strip(addr, (uint16_t)(g->tileBase + first), step, attr,
                       g->tilePalettes ? g->tilePalettes + first : 0,
                       art, blank);
    }
    g->mapFirst = g->firstSprite;
    g->mapStrips = g->strips;
    g->mapHeight = g->heightTiles;
    g->mapRows = mapRows;
}

/*
 * One strip pointed at another column of the group's art: `column` counts
 * 16-pixel columns from tileBase, rows stepping by tileStride, the bank map
 * stepping with them. A layer wider than the 512 pixels the hardware wraps
 * in streams its columns this way, each strip rewritten while it is off
 * screen. Written at once.
 */
static void NEOGEO_USER ngsg_write_strip(NGSpriteGroup *g, uint8_t strip, uint16_t column)
{
    uint8_t mapRows = ng_sprite_map_rows(ngsg_rows(g));
    uint8_t art = g->heightTiles < mapRows ? g->heightTiles : mapRows;
    ngsg_put_strip((uint16_t)(64u * (uint16_t)(g->firstSprite + strip)), (uint16_t)(g->tileBase + column),
                   (int16_t)g->tileStride, ngsg_attr(g), g->tilePalettes ? g->tilePalettes + column : 0,
                   art, 0u);
}

#ifdef NG_VRAM_DEFER
static void NEOGEO_USER ngsg_list_strip(NGSpriteGroup *g, uint8_t strip, uint16_t column);
#endif
static void NEOGEO_USER ngsg_note_ready(NGSpriteGroup *g, uint8_t rows);

void NEOGEO_USER ng_sprite_group_set_strip_column(NGSpriteGroup *g, uint8_t strip, uint16_t column)
{
    if (!g || strip >= g->strips) return;
#ifdef NG_VRAM_DEFER
    ngsg_list_strip(g, strip, column);
#else
    ngsg_write_strip(g, strip, column);
#endif
}

void NEOGEO_USER ng_sprite_group_set_palette_map(NGSpriteGroup *g, const uint8_t *banks)
{
    if (g && g->tilePalettes != banks) {
        g->tilePalettes = banks;
        g->dirty |= NG_SGF_DIRTY_PALETTE;
    }
}

static void NEOGEO_USER ng_sprite_kill_slot(uint16_t spr)
{
    uint16_t i;
    uint16_t scb1_base;

    if (spr >= NG_SPR_TOTAL) return;
    NG_PERF_VRAM(3u + 64u);

    /* 1. Kill display FIRST: ACT=0, chain=0, Y_field=256 ->
     *    screen_y=240 (past visible).  Y_field=496 would resolve
     *    to screen_y=0 — visible, not off-screen. */
    vram_SCB234((uint16_t)(SCB3_ADDR + spr), NG_SPRITE_DISABLED_SCB3);

    /* 2. Normalise scale (full size) and park X off-screen right. */
    vram_SCB234((uint16_t)(SCB2_ADDR + spr), 0x0FFFu);
    vram_SCB234((uint16_t)(SCB4_ADDR + spr), setSCB4(NG_SPRITE_DISABLED_X));

    /* 3. FULL SCB1 clear — every one of the 32 (tile, attr) rows.
     *    If hardware ever wraps the height field or some later
     *    write resurrects ACT, rows 1..31 must be safe.  Using
     *    NG_SPRITE_BLANK_TILE (a tile slot the artbox guarantees
     *    is all-zero pixel data) makes the worst-case render a
     *    fully transparent 16x32 area instead of last frame's
     *    artwork. */
    /*    Streamed straight at VRAM_RW rather than through 64
     *    vram_sfix1() calls.  The SDK builds at -O0, so each of those
     *    was a real out-of-line call and a stack frame for a single
     *    word store, and this runs for all 381 slots on every scene
     *    clear - PC sampling put a third of the ~3s black gap between
     *    the eyecatcher and the first intro frame right here.  The
     *    write sequence is identical (VRAM_ADDR, VRAM_INC=1, then 64
     *    consecutive words); only the call overhead is gone. */
    scb1_base = (uint16_t)(64u * spr);
    NEO_REGISTER(VRAM_ADDR) = scb1_base;
    NEO_REGISTER(VRAM_INC)  = 1u;
    {
        volatile uint16_t *rw = (volatile uint16_t *)VRAM_RW;

        /* Eight rows per pass: at -O0 the loop counter arithmetic costs
         * as much as the stores themselves, so unrolling is most of the
         * win here. */
        for (i = 0u; i < 4u; i++) {
            rw[0] = NG_SPRITE_BLANK_TILE; rw[0] = NG_SPRITE_BLANK_ATTR;
            rw[0] = NG_SPRITE_BLANK_TILE; rw[0] = NG_SPRITE_BLANK_ATTR;
            rw[0] = NG_SPRITE_BLANK_TILE; rw[0] = NG_SPRITE_BLANK_ATTR;
            rw[0] = NG_SPRITE_BLANK_TILE; rw[0] = NG_SPRITE_BLANK_ATTR;
            rw[0] = NG_SPRITE_BLANK_TILE; rw[0] = NG_SPRITE_BLANK_ATTR;
            rw[0] = NG_SPRITE_BLANK_TILE; rw[0] = NG_SPRITE_BLANK_ATTR;
            rw[0] = NG_SPRITE_BLANK_TILE; rw[0] = NG_SPRITE_BLANK_ATTR;
            rw[0] = NG_SPRITE_BLANK_TILE; rw[0] = NG_SPRITE_BLANK_ATTR;
        }
    }
}

void NEOGEO_USER ng_sprite_disable_hw(uint16_t spr)
{
    ng_sprite_kill_slot(spr);
}

void NEOGEO_USER ng_sprite_disable_hw_range(uint16_t first, uint16_t count)
{
    uint16_t end;
    uint16_t i;

    if (first == 0xffffu) return;
    if (first >= NG_SPR_TOTAL) return;

    end = (uint16_t)(first + count);
    if (end > NG_SPR_TOTAL || end < first) end = NG_SPR_TOTAL;

    for (i = first; i < end; i++) {
        ng_sprite_disable_hw(i);
    }
}

void NEOGEO_USER ng_sprite_park_off(uint16_t spr)
{
    if (spr >= NG_SPR_TOTAL) return;
    /* Break the chain and disable height before touching its transform.
     * Map padding is guaranteed by upload/flush, not by a per-frame wipe. */
    ngsg_put((uint16_t)(SCB3_ADDR + spr), NG_SPRITE_DISABLED_SCB3);
    ngsg_put((uint16_t)(SCB2_ADDR + spr), 0x0FFFu);
    ngsg_put((uint16_t)(SCB4_ADDR + spr), (uint16_t)(NG_SPRITE_DISABLED_X << 7));
}

void NEOGEO_USER ng_sprite_park_off_range(uint16_t first, uint16_t count)
{
    uint16_t end;
    uint16_t i;

    if (first == 0xffffu) return;
    if (first >= NG_SPR_TOTAL) return;

    end = (uint16_t)(first + count);
    if (end > NG_SPR_TOTAL || end < first) end = NG_SPR_TOTAL;
    if (end == first) return;

    /* ng_sprite_park_off for the whole run: every chain broken first. */
    i = (uint16_t)(end - first);
    ngsg_fill((uint16_t)(SCB3_ADDR + first), NG_SPRITE_DISABLED_SCB3, NG_SPRITE_DISABLED_SCB3, i);
    ngsg_fill((uint16_t)(SCB2_ADDR + first), 0x0FFFu, 0x0FFFu, i);
    ngsg_fill((uint16_t)(SCB4_ADDR + first), (uint16_t)(NG_SPRITE_DISABLED_X << 7),
              (uint16_t)(NG_SPRITE_DISABLED_X << 7), i);
}

/* Per-frame hide path: hot, must fit in vblank.  Uses the quick
 * park (just SCB234, no SCB1 wipe).  ng_sprite_hide_all() goes
 * through ng_vram_clear_all_sprites() -> heavy disable instead,
 * so scene boundaries still get the full SCB1 teardown. */
void NEOGEO_USER ng_sprite_hide_range(uint16_t firstSprite, uint16_t count)
{
#ifdef NG_VRAM_DEFER
    ng_vram_busy++;
#endif
    ng_sprite_park_off_range(firstSprite, count);
#ifdef NG_VRAM_DEFER
    ng_vram_busy--;
#endif
}

void NEOGEO_USER ng_sprite_hide_vram_base(uint16_t spriteBase, uint16_t count)
{
    ng_sprite_park_off_range(ng_vram_scb1_to_sprite_slot(spriteBase), count);
}

#ifdef NG_VRAM_DEFER
static void NEOGEO_USER ngsg_queue_drop(void);
#endif

void NEOGEO_USER ng_sprite_hide_all(void)
{
#ifdef NG_VRAM_DEFER
    /* Everything goes: what was listed for the next blank was flushed
     * before this and would bring its strips back over the next screen. */
    ngsg_queue_drop();
    ng_vram_busy++;
#endif
    /* Slot zero is the hardware's empty-list filler. */
    ng_sprite_disable_hw(0u);
    ng_sprite_park_off_range(1u, NG_SPR_TOTAL - 1u);
#ifdef NG_VRAM_DEFER
    ng_vram_busy--;
#endif
}

void NEOGEO_USER ng_sprite_group_init(NGSpriteGroup *g, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette)
{
    if (!g) return;

    strips = ngsg_clamp_u8(strips, 1, NG_SPRITE_MAX_STRIPS);
    heightTiles = ngsg_clamp_u8(heightTiles, 1, NG_SPRITE_MAX_HEIGHT_TILES);

    g->firstSprite = firstSprite;
    g->strips = strips;
    g->heightTiles = heightTiles;
    g->activeRows = heightTiles;
    g->tileBase = tileBase;
    g->tileStride = strips;
    g->palette = palette;
    g->tilePalettes = 0;
    g->x = 0;
    g->y = 0;
    g->xScale = NG_SPRITE_FULL_XSCALE;
    g->yScale = NG_SPRITE_FULL_YSCALE;
    g->hflip = 0;
    g->vflip = 0;
    g->autoAnim4 = 0;
    g->autoAnim8 = 0;
    g->visible = 1;
    g->dirty = NG_SGF_DIRTY_ALL;   /* force full upload on first draw */
    g->mapRows = 0u;
    g->mapFirst = 0xffffu;
    g->mapStrips = g->mapHeight = 0u;
    g->prio = NG_SG_PRIO_NORMAL;
    g->readyRows = 0u;
}

void NEOGEO_USER ng_sprite_group_set_priority(NGSpriteGroup *g, uint8_t prio)
{
    if (g) g->prio = prio <= NG_SG_PRIO_LOW ? prio : NG_SG_PRIO_NORMAL;
}

void NEOGEO_USER ng_sprite_group_mark_dirty(NGSpriteGroup *g, uint8_t dirty_flags)
{
    if (g) g->dirty |= dirty_flags;
}

void NEOGEO_USER ng_sprite_group_set_tile_base(NGSpriteGroup *g, uint16_t tileBase)
{
    if (g && g->tileBase != tileBase) { g->tileBase = tileBase; g->dirty |= NG_SGF_DIRTY_TILE; }
}

void NEOGEO_USER ng_sprite_group_set_tile_stride(NGSpriteGroup *g, uint16_t tileStride)
{
    if (g) {
        uint16_t new_stride = tileStride ? tileStride : g->strips;
        if (g->tileStride != new_stride) {
            g->tileStride = new_stride;
            /* tile_for() multiplies the row index by tileStride, so the
             * computed tile id for every cell changes — SCB1 must be
             * re-uploaded for the new stride to take effect. */
            g->dirty |= NG_SGF_DIRTY_TILE;
        }
    }
}

void NEOGEO_USER ng_sprite_group_set_palette(NGSpriteGroup *g, uint8_t palette)
{
    if (g && g->palette != palette) { g->palette = palette; g->dirty |= NG_SGF_DIRTY_PALETTE; }
}

void NEOGEO_USER ng_sprite_group_set_active_rows(NGSpriteGroup *g, uint8_t activeRows)
{
    if (!g) return;

    if (activeRows < 1) activeRows = 1;
    if (activeRows > g->heightTiles) activeRows = g->heightTiles;
    if (activeRows > NG_SPRITE_MAX_HEIGHT_TILES) activeRows = NG_SPRITE_MAX_HEIGHT_TILES;

    if (g->activeRows != activeRows) {
        g->activeRows = activeRows;
        /* The driver strip and every chained strip carry a copy of the
         * active-character count, so this has to reach both.  SHRINK is
         * the flag that means "the count moved" - the vertical shrink is
         * the other thing it is derived from. */
        g->dirty |= NG_SGF_DIRTY_POS | NG_SGF_DIRTY_SHRINK;
    }
}

void NEOGEO_USER ng_sprite_group_set_pos(NGSpriteGroup *g, int16_t x, int16_t y)
{
    if (g && (g->x != x || g->y != y)) {
        g->x = x;
        g->y = y;
        g->dirty |= NG_SGF_DIRTY_POS;
    }
}

void NEOGEO_USER ng_sprite_group_move(NGSpriteGroup *g, int16_t dx, int16_t dy)
{
    if (g) {
        g->x += dx;
        g->y += dy;
        g->dirty |= NG_SGF_DIRTY_POS;
    }
}

void NEOGEO_USER ng_sprite_group_set_scale(NGSpriteGroup *g, uint8_t xScale, uint8_t yScale)
{
    if (g && (g->xScale != xScale || g->yScale != yScale)) {
        g->xScale = xScale;
        g->yScale = yScale;
        g->dirty |= NG_SGF_DIRTY_SHRINK;
    }
}

void NEOGEO_USER ng_sprite_group_set_flip(NGSpriteGroup *g, uint8_t hflip, uint8_t vflip)
{
    if (g) {
        uint8_t nh = hflip ? 1 : 0;
        uint8_t nv = vflip ? 1 : 0;
        if (g->hflip != nh || g->vflip != nv) {
            g->hflip = nh;
            g->vflip = nv;
            /* hflip mirrors the per-strip tile id lookup AND the SCB1
             * attribute word's hflip bit; vflip does the same for rows.
             * Both pieces live behind the TILE/PALETTE flush path, so
             * mark both so the next flush re-emits SCB1. */
            g->dirty |= NG_SGF_DIRTY_TILE | NG_SGF_DIRTY_PALETTE;
        }
    }
}

void NEOGEO_USER ng_sprite_group_set_auto_anim(NGSpriteGroup *g, uint8_t autoAnim4, uint8_t autoAnim8)
{
    if (g) {
        uint8_t na4 = autoAnim4 ? 1 : 0;
        uint8_t na8 = autoAnim8 ? 1 : 0;
        if (g->autoAnim4 != na4 || g->autoAnim8 != na8) {
            g->autoAnim4 = na4;
            g->autoAnim8 = na8;
            /* autoAnim bits live in SCB1's attribute word (bits 3/2);
             * re-emit via the PALETTE flush path which owns SCB1[attr]. */
            g->dirty |= NG_SGF_DIRTY_PALETTE;
        }
    }
}

void NEOGEO_USER ng_sprite_group_set_visible(NGSpriteGroup *g, uint8_t visible)
{
    if (g && g->visible != (visible ? 1 : 0)) {
        g->visible = visible ? 1 : 0;
        g->dirty |= visible ? NG_SGF_DIRTY_ALL : NG_SGF_DIRTY_VIS;
    }
}

void NEOGEO_USER ng_sprite_group_show_at(NGSpriteGroup *g, uint16_t tileBase, uint8_t palette,
                                         int16_t x, int16_t y)
{
    uint8_t d = 0;
    if (!g) return;
    if (g->tileBase != tileBase) { g->tileBase = tileBase; d |= NG_SGF_DIRTY_TILE; }
    if (g->palette != palette) { g->palette = palette; d |= NG_SGF_DIRTY_PALETTE; }
    if (g->x != x || g->y != y) { g->x = x; g->y = y; d |= NG_SGF_DIRTY_POS; }
    if (!g->visible) { g->visible = 1; d |= NG_SGF_DIRTY_ALL; }
    g->dirty |= d;
    ng_sprite_group_flush(g);
}

void NEOGEO_USER ng_sprite_groups_hide_all(NGSpriteGroup *g, uint8_t count)
{
    if (!g) return;
    for (; count; count--, g++) {
        if (g->visible) { g->visible = 0; g->dirty |= NG_SGF_DIRTY_VIS; }
        if (g->dirty & NG_SGF_DIRTY_ALL) ng_sprite_group_flush(g);
    }
}

#ifdef NG_VRAM_DEFER
static void NEOGEO_USER ngsg_upload_now(NGSpriteGroup *g);
void NEOGEO_USER ng_sprite_group_upload(NGSpriteGroup *g)
{
    ng_vram_busy++;
    ngsg_upload_now(g);
    ng_vram_busy--;
}
static void NEOGEO_USER ngsg_upload_now(NGSpriteGroup *g)
#else
void NEOGEO_USER ng_sprite_group_upload(NGSpriteGroup *g)
#endif
{
    uint8_t rows;
    uint16_t n;

    if (!g) return;

    if (!g->visible) {
        ng_sprite_group_hide(g);
        return;
    }

    rows = ngsg_rows(g);
    n = g->strips;
    ngsg_put_map(g, rows, 1u);
    /*
     * The driving strip owns X, Y, the height and the vertical shrink; the
     * chained (sticky) strips carry the chain bit (0x40) and a copy of the
     * height, which hardware ignores on them but emulators may read.
     */
    ngsg_fill((uint16_t)(SCB2_ADDR + g->firstSprite), ngsg_scb2(g), ngsg_scb2(g), n);
    ngsg_fill((uint16_t)(SCB3_ADDR + g->firstSprite), ngsg_scb3(g, rows), (uint16_t)(0x0040u | rows), n);
    ngsg_fill((uint16_t)(SCB4_ADDR + g->firstSprite), ngsg_scb4(g), 0u, n);
    g->dirty = 0u;
    ngsg_note_ready(g, rows);
}

void NEOGEO_USER ng_sprite_group_update_transform(NGSpriteGroup *g)
{
    uint8_t rows;

    if (!g) return;

    if (!g->visible) {
        ng_sprite_group_hide(g);
        return;
    }

    rows = ngsg_rows(g);
    /* Horizontal shrink on every strip, for a consistent width; the
     * driver's Y and height, and the chain bit on the rest; the driver's X. */
    ngsg_fill((uint16_t)(SCB2_ADDR + g->firstSprite), ngsg_scb2(g), ngsg_scb2(g), g->strips);
    ngsg_fill((uint16_t)(SCB3_ADDR + g->firstSprite), ngsg_scb3(g, rows), (uint16_t)(0x0040u | rows), g->strips);
    ngsg_put((uint16_t)(SCB4_ADDR + g->firstSprite), ngsg_scb4(g));
}

/**
 * Hardware Initialization as per Spec:
 * [1] Write transparency character to VRAM 0-3FH
 * [2] Write 0000H to VRAM 8200H
 */
void NEOGEO_USER ng_engine_init_hardware(uint16_t transparentTile)
{
    uint16_t i;
    vram_init(0, 1);
    for (i = 0; i < 0x40; i++) {
        vram_sfix1(transparentTile);
    }
    vram_SCB234(0x8200, 0);
}

void NEOGEO_USER ng_sprite_group_hide(NGSpriteGroup *g)
{
    if (!g) return;
    ng_sprite_hide_range(g->firstSprite, g->strips);
    g->mapRows = 0u;
    g->readyRows = 0u;
}

/* After a full write: whether a later move can be only the driving strip's
 * position (its map, footprint and shrink as written), and with how many rows. */
static void NEOGEO_USER ngsg_note_ready(NGSpriteGroup *g, uint8_t rows)
{
    g->readyRows = (uint8_t)((g->visible && rows && g->yScale == NG_SPRITE_FULL_YSCALE &&
                              g->mapRows >= ng_sprite_map_rows(rows) && g->mapFirst == g->firstSprite &&
                              g->mapStrips == g->strips && g->mapHeight == g->heightTiles) ? rows : 0u);
}

/*
 * ng_sprite_group_flush — dirty-aware VRAM update.
 *
 * Only writes the VRAM regions corresponding to set dirty flags.
 * Clears all dirty flags after writing.
 *
 * Rules:
 *   NG_SGF_DIRTY_TILE or PALETTE → full SCB1 upload (tile+attr per row).
 *   NG_SGF_DIRTY_SHRINK           → write SCB2 for all strips.
 *   NG_SGF_DIRTY_POS              → write SCB3 (driver) + SCB4 (all strips).
 *   NG_SGF_DIRTY_VIS              → hide or show as appropriate.
 *
 * If nothing is dirty, the function returns immediately — zero VRAM writes.
 */
/* With NG_VRAM_DEFER this is the commit's writer, called with the group's
 * listing already cleared; ng_sprite_group_flush() then only lists. */
#ifdef NG_VRAM_DEFER
static void NEOGEO_USER ngsg_flush_now(NGSpriteGroup *g)
#else
void NEOGEO_USER ng_sprite_group_flush(NGSpriteGroup *g)
#endif
{
    uint8_t dirty;
    uint8_t rows;

    if (!g) return;
    dirty = g->dirty;
    if (!dirty) return;
    g->dirty = 0;   /* all flushed */

    /* Visibility change: hide and return if not visible */
    if (!g->visible) {
        if (dirty & NG_SGF_DIRTY_VIS) ng_sprite_group_hide(g);
        return;
    }

    rows = ngsg_rows(g);

    /* SCB1 tile + attribute upload — only when tile or palette changed */
    if ((dirty & (NG_SGF_DIRTY_TILE | NG_SGF_DIRTY_PALETTE)) ||
        g->mapRows < ng_sprite_map_rows(rows) || g->mapFirst != g->firstSprite ||
        g->mapStrips != g->strips || g->mapHeight != g->heightTiles)
        ngsg_put_map(g, rows, (uint8_t)(dirty == NG_SGF_DIRTY_ALL));

    /* SCB2 shrink upload */
    if (dirty & NG_SGF_DIRTY_SHRINK)
        ngsg_fill((uint16_t)(SCB2_ADDR + g->firstSprite), ngsg_scb2(g), ngsg_scb2(g), g->strips);

    /*
     * SCB3/4.  The driver strip carries X, Y and the active-character count,
     * so it is rewritten for a move, a show, or a scale change - the count is
     * derived from the vertical shrink and goes stale with it.
     *
     * Chained strips hold only the chain bit and a copy of the count; the
     * hardware reads neither position from them.  A move therefore leaves
     * them alone - re-stamping thirty-odd slots every time a background
     * scrolls one pixel was most of what a scroll cost.  A show still has to
     * write them, because parking a slot clears its chain bit.
     */
    if (dirty & (NG_SGF_DIRTY_VIS | NG_SGF_DIRTY_SHRINK)) {
        ngsg_fill((uint16_t)(SCB3_ADDR + g->firstSprite), ngsg_scb3(g, rows), (uint16_t)(0x0040u | rows), g->strips);
        ngsg_fill((uint16_t)(SCB4_ADDR + g->firstSprite), ngsg_scb4(g), 0u, g->strips);
    } else if (dirty & NG_SGF_DIRTY_POS) {
        ngsg_put((uint16_t)(SCB3_ADDR + g->firstSprite), ngsg_scb3(g, rows));
        ngsg_put((uint16_t)(SCB4_ADDR + g->firstSprite), ngsg_scb4(g));
    }
    ngsg_note_ready(g, rows);
}

#ifdef NG_VRAM_DEFER
/*
 * The vertical blank's video work, with a deadline (ng_sprite_group.h).
 *
 * The blank is 40 lines, 240 to the line before the picture (16). A
 * commit that ran on past it wrote sprites while they were being drawn;
 * measured on a busy road, most commits did. Each job now looks at the
 * line first; what doesn't fit stays listed, in order, for the next blank.
 */
volatile uint8_t ng_vram_busy;

static NGSpriteGroup *ngsg_high[NG_VRAM_QUEUE_HIGH];
static NGSpriteGroup *ngsg_norm[NG_VRAM_QUEUE_GROUPS];
static NGSpriteGroup *ngsg_low[NG_VRAM_QUEUE_LOW];
static uint16_t ngsg_high_n, ngsg_norm_n, ngsg_low_n;
/* Character hides: a bit a sprite slot, so a hide never waits for room. */
static uint16_t ngsg_hide_bits[(NG_SPR_TOTAL + 15u) >> 4];
static uint8_t ngsg_hide_any;
/* A streaming layer's strips pointed at new columns (each off screen). */
static struct { NGSpriteGroup *g; uint16_t column; uint8_t strip; } ngsg_strips[NG_VRAM_QUEUE_STRIPS];
static uint8_t ngsg_strips_n;
static uint16_t ngsg_done;      /* groups written by the commit under way (ng_perf) */

/* The raster line, 0..263 (the counter runs $F8..$1FF: $100 is line 0). */
static uint16_t NEOGEO_USER ngsg_line(void)
{
    uint16_t c = (uint16_t)(*(volatile uint16_t *)0x3C0006u >> 7);
    return (uint16_t)(c >= 0x100u ? c - 0x100u : c + 8u);
}

uint8_t NEOGEO_USER ng_vram_lines_left(void)
{
    uint16_t l = ngsg_line();
    if (l >= 240u) return (uint8_t)(264u - l + NG_VRAM_DEADLINE);
    if (l < NG_VRAM_DEADLINE) return (uint8_t)(NG_VRAM_DEADLINE - l);
    return 0u;
}

uint8_t NEOGEO_USER ng_vram_window_open(void)
{
    return (uint8_t)(ng_vram_lines_left() != 0u);
}

void NEOGEO_USER ng_sprite_group_flush(NGSpriteGroup *g)
{
    if (!g || !(g->dirty & NG_SGF_DIRTY_ALL)) return;
    if (g->dirty & NG_SGF_QUEUED) return;          /* already listed: written as it is then */
    if (g->prio == NG_SG_PRIO_HIGH && ngsg_high_n < NG_VRAM_QUEUE_HIGH) {
        ngsg_high[ngsg_high_n++] = g;
    } else if (g->prio == NG_SG_PRIO_LOW && ngsg_low_n < NG_VRAM_QUEUE_LOW) {
        ngsg_low[ngsg_low_n++] = g;
    } else if (ngsg_norm_n < NG_VRAM_QUEUE_GROUPS) {
        ngsg_norm[ngsg_norm_n++] = g;
    } else {
        /* Full: never written here. It stays dirty, and its next flush
         * lists it (each group is listed once a frame, so this needs more
         * groups changing in one frame than the lists hold). */
        NG_PERF_QUEUE_FULL();
        return;
    }
    g->dirty = (uint8_t)(g->dirty | NG_SGF_QUEUED);
}

static void NEOGEO_USER ngsg_list_strip(NGSpriteGroup *g, uint8_t strip, uint16_t column)
{
    uint8_t i;
    for (i = 0; i < ngsg_strips_n; i++) {
        if (ngsg_strips[i].g == g && ngsg_strips[i].strip == strip) {
            ngsg_strips[i].column = column;        /* the last column wins */
            return;
        }
    }
    if (ngsg_strips_n < NG_VRAM_QUEUE_STRIPS) {
        ngsg_strips[ngsg_strips_n].g = g;
        ngsg_strips[ngsg_strips_n].strip = strip;
        ngsg_strips[ngsg_strips_n].column = column;
        ngsg_strips_n++;
    } else {
        NG_PERF_QUEUE_FULL();                      /* (two layers' 32 strips each fit) */
    }
}

/* The lists emptied: each group listed loses its changes, as cancelled
 * (ng_sprite_group_cancel), and no hide or strip waits either. */
static void NEOGEO_USER ngsg_queue_drop(void)
{
    uint16_t i;
    for (i = 0; i < ngsg_high_n; i++) ngsg_high[i]->dirty = 0;
    for (i = 0; i < ngsg_norm_n; i++) ngsg_norm[i]->dirty = 0;
    for (i = 0; i < ngsg_low_n; i++) ngsg_low[i]->dirty = 0;
    ngsg_high_n = ngsg_norm_n = ngsg_low_n = 0;
    for (i = 0; i < (uint16_t)((NG_SPR_TOTAL + 15u) >> 4); i++) ngsg_hide_bits[i] = 0;
    ngsg_hide_any = 0;
    ngsg_strips_n = 0;
}

void NEOGEO_USER ng_sprite_group_cancel(NGSpriteGroup *g)
{
    /* Its changes are dropped; the listing, if any, stays and finds nothing. */
    if (g) g->dirty = (uint8_t)(g->dirty & NG_SGF_QUEUED);
}

void NEOGEO_USER ng_sprite_hide_range_queued(uint16_t firstSprite, uint16_t count)
{
    uint16_t s, end = (uint16_t)(firstSprite + count);
    if (firstSprite >= NG_SPR_TOTAL) return;
    if (end > NG_SPR_TOTAL || end < firstSprite) end = NG_SPR_TOTAL;
    for (s = firstSprite; s < end; s++) ngsg_hide_bits[s >> 4] |= (uint16_t)(1u << (s & 15u));
    if (end > firstSprite) ngsg_hide_any = 1;
}

/*
 * Nine flushes in ten (measured in play) only move a group: its map,
 * height and shrink are as last written, and the driving strip's two
 * position words are all that change. Written here straight, without the
 * general routine's checks; anything else takes ngsg_flush_now(). Returns
 * 0 when the group isn't that case.
 */
static uint8_t NEOGEO_USER ngsg_flush_moved(NGSpriteGroup *g)
{
    uint8_t rows;
    if ((g->dirty & NG_SGF_DIRTY_ALL) != NG_SGF_DIRTY_POS || !g->visible ||
        g->yScale != NG_SPRITE_FULL_YSCALE || g->mapFirst != g->firstSprite ||
        g->mapStrips != g->strips || g->mapHeight != g->heightTiles)
        return 0u;
    rows = g->activeRows ? g->activeRows : g->heightTiles;          /* as ngsg_rows() */
    if (rows > g->heightTiles) rows = g->heightTiles;
    if (rows > NG_SPRITE_MAX_HEIGHT_TILES) rows = NG_SPRITE_MAX_HEIGHT_TILES;
    if (!rows || g->mapRows < ng_sprite_map_rows(rows)) return 0u;
    ngsg_put((uint16_t)(SCB3_ADDR + g->firstSprite), ngsg_scb3(g, rows));
    ngsg_put((uint16_t)(SCB4_ADDR + g->firstSprite), ngsg_scb4(g));
    g->dirty = (uint8_t)(g->dirty & NG_SGF_QUEUED);
    return 1u;
}

/* About how many lines a listed group's write takes, rounded up, as
 * measured in play: a new tile map 2 lines of set-up, three quarters of a
 * line a strip and some 12 row pairs a line; a show, a hide or a rescale 2
 * lines a small group; a move (most of them) well under one. Never more
 * than a blank holds, so a big job can always go first in the next one. */
#define NGSG_COST_MAX 30u
static uint8_t NEOGEO_USER ngsg_cost(const NGSpriteGroup *g)
{
    uint8_t d = g->dirty;
    if (!g->visible) return (uint8_t)(2u + (g->strips >> 3));
    if ((d & (NG_SGF_DIRTY_TILE | NG_SGF_DIRTY_PALETTE)) || g->mapFirst != g->firstSprite ||
        g->mapStrips != g->strips || g->mapHeight != g->heightTiles) {
        uint16_t pairs = (uint16_t)((uint16_t)g->strips * (g->heightTiles < 16u ? g->heightTiles : 16u));
        uint16_t lines;
        if (g->mapRows < ng_sprite_map_rows(ngsg_rows(g)) || g->mapFirst != g->firstSprite)
            pairs = (uint16_t)((uint16_t)g->strips * ng_sprite_map_rows(ngsg_rows(g)));   /* padding too */
        lines = (uint16_t)(2u + (((uint16_t)g->strips * 3u) >> 2) + ((pairs * 5u) >> 6));   /* set-up, strips, rows */
        return (uint8_t)(lines > NGSG_COST_MAX ? NGSG_COST_MAX : lines);
    }
    if (d & (NG_SGF_DIRTY_VIS | NG_SGF_DIRTY_SHRINK)) return (uint8_t)(2u + (g->strips >> 3));
    return 1u;
}

/*
 * A rescale (the vertical or horizontal shrink, maybe a move with it) of a
 * group whose map still fits: every strip's SCB2, the driving strip's
 * Y/count with the count copied to the chained strips, the driving X --
 * without the general routine's checks. Sparks and petals shrink a little
 * every frame, and through the general path each cost two lines. Returns 0
 * when the group isn't that case.
 */
static uint8_t NEOGEO_USER ngsg_flush_rescaled(NGSpriteGroup *g)
{
    uint8_t rows;
    if ((g->dirty & NG_SGF_DIRTY_ALL & (uint8_t)~(NG_SGF_DIRTY_POS | NG_SGF_DIRTY_SHRINK)) || !g->visible ||
        g->mapFirst != g->firstSprite || g->mapStrips != g->strips || g->mapHeight != g->heightTiles)
        return 0u;
    rows = ngsg_rows(g);
    if (!rows || g->mapRows < ng_sprite_map_rows(rows)) return 0u;
    ngsg_fill((uint16_t)(SCB2_ADDR + g->firstSprite), ngsg_scb2(g), ngsg_scb2(g), g->strips);
    ngsg_fill((uint16_t)(SCB3_ADDR + g->firstSprite), ngsg_scb3(g, rows), (uint16_t)(0x0040u | rows), g->strips);
    ngsg_put((uint16_t)(SCB4_ADDR + g->firstSprite), ngsg_scb4(g));
    g->dirty = (uint8_t)(g->dirty & NG_SGF_QUEUED);
    ngsg_note_ready(g, rows);
    return 1u;
}

/* Phase B: one list's groups, as many as the blank has room for; the rest
 * stay listed, in order, at its head. A job starts only with half its
 * estimate again to spare; the line is read after every job but the
 * cheapest, whose cost is taken off instead (rounded up, so it errs early). */
static void NEOGEO_USER ngsg_commit_list(NGSpriteGroup **q, uint16_t *count)
{
    uint16_t i, kept = 0, n = *count;
    uint8_t moves = 0;
    uint8_t left;
    if (!n) return;
    left = ng_vram_lines_left();
    for (i = 0; i < n; i++) {
        NGSpriteGroup *g = q[i];
        uint8_t cost;
        if (!left) {
            /* the blank is spent: the rest wait, in order, untouched (one
             * moved only is dropped from the list next time round) */
            if (kept != i) while (i < n) q[kept++] = q[i++];
            else kept = n;
            break;
        }
        if (!(g->dirty & NG_SGF_DIRTY_ALL)) {       /* only moved: done in phase A */
            g->dirty = (uint8_t)(g->dirty & (uint8_t)~NG_SGF_QUEUED);
            continue;
        }
        /* A move phase A had no time for: the driving strip's Y/count and X. */
        if ((g->dirty & NG_SGF_DIRTY_ALL) == NG_SGF_DIRTY_POS && g->readyRows && g->visible) {
            if (left < 2u) { q[kept++] = g; left = ng_vram_lines_left(); continue; }
            ngsg_put((uint16_t)(SCB3_ADDR + g->firstSprite),
                     (uint16_t)(((uint16_t)(496 - g->y) << 7) | g->readyRows));
            ngsg_put((uint16_t)(SCB4_ADDR + g->firstSprite), (uint16_t)((uint16_t)g->x << 7));
            g->dirty = 0;
            ngsg_done++;
            if (left < 10u || !(++moves & 7u)) left = ng_vram_lines_left();
            continue;
        }
        cost = ngsg_cost(g);
        if ((uint8_t)(cost + (cost >> 1)) > left) {   /* (a half again: the estimate can run short) */
            q[kept++] = g;                          /* next blank: still listed */
            left = ng_vram_lines_left();
            continue;
        }
        g->dirty = (uint8_t)(g->dirty & (uint8_t)~NG_SGF_QUEUED);
        if (!ngsg_flush_moved(g) && !ngsg_flush_rescaled(g)) ngsg_flush_now(g);
        ngsg_done++;
        if (left <= 8u || cost > 1u || (i & 7u) == 7u) left = ng_vram_lines_left();
        else left = (uint8_t)(left - cost);
    }
    *count = kept;
}

#ifdef NG_DEBUG_PERF
/* (measurement builds) Content -- maps, shrinks, shows -- a commit left for
 * the next blank. Counted at its end, so the count takes a little of the
 * blank: a measurement build defers a little more than a release does. */
static void NEOGEO_USER ngsg_perf_left(void)
{
    const uint8_t content = NG_SGF_DIRTY_TILE | NG_SGF_DIRTY_PALETTE | NG_SGF_DIRTY_SHRINK | NG_SGF_DIRTY_VIS;
    uint16_t k, left = 0;
    for (k = 0; k < ngsg_high_n; k++) if (ngsg_high[k]->dirty & content) left++;
    for (k = 0; k < ngsg_norm_n; k++) if (ngsg_norm[k]->dirty & content) left++;
    for (k = 0; k < ngsg_low_n; k++) if (ngsg_low[k]->dirty & content) left++;
    NG_PERF_DEFERRED(left);
}
#endif

/*
 * Phase A: every listed group that only moved since its last full write
 * gets its new position now -- the driving strip's Y/count and X, two
 * words, a fraction of a line -- in every list. Scenery that slipped a
 * frame against the scrolling layers would show. Groups with new content
 * (maps, shrinks, shows) are phase B, whole, by priority.
 */
static void NEOGEO_USER ngsg_commit_moves(NGSpriteGroup **q, uint16_t n)
{
    uint16_t i;
    uint8_t left, moves = 0;
    if (!n) return;
    left = ng_vram_lines_left();
#ifdef __m68k__
    /* SCB3 and SCB4 are $200 words apart: with the step at $200, one
     * address and two words make a move (the other writers set their own
     * step). */
    NEO_REGISTER(VRAM_INC) = 0x200u;
#endif
    for (i = 0; i < n; i++) {
        NGSpriteGroup *g = q[i];
        uint8_t rows = g->readyRows;
        /* a move alone: one with new content as well (a pool block given
         * another piece, a creature's next frame) is written whole in
         * phase B, never its place now and its look a frame later */
        if ((g->dirty & NG_SGF_DIRTY_ALL) != NG_SGF_DIRTY_POS || !rows || !g->visible) continue;
        /* the line every eighth move (each is a fraction of a line), every
         * move near the end; what's left keeps its move for phase B or the
         * next blank */
        if (left < 6u || !(++moves & 7u)) {
            left = ng_vram_lines_left();
            if (left < 2u) break;
        }
#ifdef __m68k__
        NG_PERF_VRAM(2);
        __asm__ volatile (
            "move.w %[a],(%[p])\n\t"      /* VRAM_ADDR: SCB3 of the driving strip */
            "move.w %[y],2(%[p])\n\t"     /* Y and the count                     */
            "move.w %[x],2(%[p])"         /* (step $200) SCB4: X, 12 clocks on   */
            :
            : [p] "a" (VRAM_ADDR), [a] "d" ((uint16_t)(SCB3_ADDR + g->firstSprite)),
              [y] "d" ((uint16_t)(((uint16_t)(496 - g->y) << 7) | rows)),
              [x] "d" ((uint16_t)((uint16_t)g->x << 7))
            : "memory");
#else
        ngsg_put((uint16_t)(SCB3_ADDR + g->firstSprite), (uint16_t)(((uint16_t)(496 - g->y) << 7) | rows));
        ngsg_put((uint16_t)(SCB4_ADDR + g->firstSprite), (uint16_t)((uint16_t)g->x << 7));
#endif
        g->dirty = (uint8_t)(g->dirty & (uint8_t)~NG_SGF_DIRTY_POS);
        ngsg_done++;
    }
#ifdef __m68k__
    NEO_REGISTER(VRAM_INC) = 1u;   /* (back to the step every other writer expects) */
#endif
}

static void NEOGEO_USER ngsg_commit_hides(void)
{
    uint16_t w, s = 0, run = 0;
    for (w = 0; w < (uint16_t)((NG_SPR_TOTAL + 15u) >> 4); w++) {
        uint16_t bits = ngsg_hide_bits[w];
        uint8_t b;
        if (!bits && !run) { s = (uint16_t)(s + 16u); continue; }
        ngsg_hide_bits[w] = 0;
        for (b = 0; b < 16u; b++, s++) {
            if (bits & (uint16_t)(1u << b)) {
                run++;
            } else if (run) {
                ng_sprite_park_off_range((uint16_t)(s - run), run);
                run = 0;
            }
        }
    }
    if (run) ng_sprite_park_off_range((uint16_t)(s - run), run);
    ngsg_hide_any = 0;
}

static void NEOGEO_USER ngsg_commit_strips(void)
{
    uint8_t i, kept = 0;
    for (i = 0; i < ngsg_strips_n; i++) {
        if (ng_vram_lines_left() < 2u) {
            ngsg_strips[kept++] = ngsg_strips[i];   /* (a strip is well under a line) */
            continue;
        }
        ngsg_write_strip(ngsg_strips[i].g, ngsg_strips[i].strip, ngsg_strips[i].column);
    }
    ngsg_strips_n = kept;
}

void NEOGEO_USER ng_vram_commit(void)
{
    ng_vram_busy++;
    NG_PERF_COMMIT(1);
    NG_PERF_QUEUE((uint16_t)(ngsg_high_n + ngsg_norm_n + ngsg_low_n));
    ngsg_done = 0;
    ng_render_queue_flush();         /* anything posted for the blank */
    /* Hides first: a character that moved to other slots is hidden where it
     * was before it is drawn where it is. */
    if (ngsg_hide_any) ngsg_commit_hides();
    /* Every listed group's move first (phase A), so nothing slips against
     * the scrolling layers; then content -- maps, shrinks, shows -- by
     * priority (phase B): the player and the layers, the cast, the text,
     * the scenery. Content is what waits when the blank is short. */
    ngsg_commit_moves(ngsg_high, ngsg_high_n);
    ngsg_commit_moves(ngsg_norm, ngsg_norm_n);
    ngsg_commit_moves(ngsg_low, ngsg_low_n);
    ngsg_commit_list(ngsg_high, &ngsg_high_n);
    if (ngsg_strips_n) ngsg_commit_strips();
    ngsg_commit_list(ngsg_norm, &ngsg_norm_n);
    {
        uint16_t from = NG_PERF_LINE();
        (void)from;
        ng_fix_commit();             /* then the text cells */
        NG_PERF_COMMIT_PART(ngsg_done, ng_fix_last_commit_cells(), from);
    }
    ngsg_commit_list(ngsg_low, &ngsg_low_n);
    NG_PERF_COMMIT(0);
#ifdef NG_DEBUG_PERF
    ngsg_perf_left();                /* (after the commit's end is taken) */
#endif
    ng_vram_busy--;
}
#endif
