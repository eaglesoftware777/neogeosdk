#include "macro.h"
#include "neogeo.h"
#include "ng_sprite_group.h"
#include "ng_sprite_pool.h"
#include "ng_vram.h"
#include "ng_sprite_hw.h"

/*
 * Built at -O2 while the rest of the tree is -O0.
 *
 * These are the routines a frame runs most: the map fill below is a
 * tight loop over up to sixteen rows for every strip of every sprite
 * that moved, and at -O0 it reloads its bounds from the stack on each
 * iteration.  Raising it here is safe because nothing in this file
 * depends on -O0 to be correct - every hardware access goes through the
 * vram_* helpers in another translation unit, so the compiler can
 * neither reorder nor elide one, and there is no inline asm holding an
 * opinion about register allocation.
 */
#pragma GCC optimize ("O2")

static uint16_t ngsg_tiles[NG_SPRITE_MAX_HEIGHT_TILES];
static uint16_t ngsg_attrs[NG_SPRITE_MAX_HEIGHT_TILES];

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

static uint16_t NEOGEO_USER ngsg_tile_for(NGSpriteGroup *g, uint8_t strip, uint8_t row)
{
    uint8_t sourceStrip = strip;
    uint8_t sourceRow = row;

    if (g->hflip) sourceStrip = (uint8_t)((g->strips - 1u) - sourceStrip);
    if (g->vflip) sourceRow = (uint8_t)((g->heightTiles - 1u) - sourceRow);

    return (uint16_t)(g->tileBase + ((uint16_t)sourceRow * g->tileStride) + sourceStrip);
}

static void NEOGEO_USER ng_sprite_kill_slot(uint16_t spr)
{
    uint16_t i;
    uint16_t scb1_base;

    if (spr >= NG_SPR_TOTAL) return;

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
    vram_SCB234((uint16_t)(SCB3_ADDR + spr), NG_SPRITE_DISABLED_SCB3);
    vram_SCB234((uint16_t)(SCB2_ADDR + spr), 0x0FFFu);
    vram_SCB234((uint16_t)(SCB4_ADDR + spr), setSCB4(NG_SPRITE_DISABLED_X));
}

void NEOGEO_USER ng_sprite_park_off_range(uint16_t first, uint16_t count)
{
    uint16_t end;
    uint16_t i;

    if (first == 0xffffu) return;
    if (first >= NG_SPR_TOTAL) return;

    end = (uint16_t)(first + count);
    if (end > NG_SPR_TOTAL || end < first) end = NG_SPR_TOTAL;

    for (i = first; i < end; i++) {
        ng_sprite_park_off(i);
    }
}

/* Per-frame hide path: hot, must fit in vblank.  Uses the quick
 * park (just SCB234, no SCB1 wipe).  ng_sprite_hide_all() goes
 * through ng_vram_clear_all_sprites() -> heavy disable instead,
 * so scene boundaries still get the full SCB1 teardown. */
void NEOGEO_USER ng_sprite_hide_range(uint16_t firstSprite, uint16_t count)
{
    ng_sprite_park_off_range(firstSprite, count);
}

void NEOGEO_USER ng_sprite_hide_vram_base(uint16_t spriteBase, uint16_t count)
{
    ng_sprite_park_off_range(ng_vram_scb1_to_sprite_slot(spriteBase), count);
}

void NEOGEO_USER ng_sprite_hide_all(void)
{
    /* Slot zero is the hardware's empty-list filler. */
    ng_sprite_disable_hw(0u);
    ng_sprite_park_off_range(1u, NG_SPR_TOTAL - 1u);
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

void NEOGEO_USER ng_sprite_group_upload(NGSpriteGroup *g)
{
    uint8_t strip;
    uint8_t row;
    uint8_t activeRows;
    uint8_t mapRows;
    uint8_t xNibble;
    uint16_t driverScb3;
    uint16_t driverScb4;
    uint16_t scb2;
    uint16_t attr;

    if (!g) return;

    if (!g->visible) {
        ng_sprite_group_hide(g);
        return;
    }

    activeRows = g->activeRows ? g->activeRows : g->heightTiles;
    if (activeRows > g->heightTiles) activeRows = g->heightTiles;
    if (activeRows > NG_SPRITE_MAX_HEIGHT_TILES) activeRows = NG_SPRITE_MAX_HEIGHT_TILES;

    activeRows = ng_sprite_display_rows(activeRows, g->yScale);
    mapRows = ng_sprite_map_rows(activeRows);
    xNibble = ngsg_x_shrink_nibble(g->xScale);
    scb2 = setSCB2(xNibble, g->yScale);
    driverScb3 = setSCB3((uint16_t)(496 - g->y), 0, activeRows);
    driverScb4 = setSCB4((uint16_t)g->x);
    attr = setSCB1_2(
        g->palette,
        0,
        g->autoAnim8,
        g->autoAnim4,
        g->vflip,
        g->hflip
    );

    for (strip = 0; strip < g->strips; strip++) {
        uint16_t spriteIndex = (uint16_t)(g->firstSprite + strip);
        uint16_t scb1Addr = (uint16_t)(64u * spriteIndex);
        uint16_t scb3;
        uint16_t scb4;

        for (row = 0; row < mapRows; row++) {
            ngsg_tiles[row] = row < g->heightTiles ? ngsg_tile_for(g, strip, row) : NG_SPRITE_BLANK_TILE;
            ngsg_attrs[row] = row < g->heightTiles ? attr : NG_SPRITE_BLANK_ATTR;
        }

        if (strip == 0) {
            /*
             * Driving strip: owns X, Y, height and vertical shrink.
             */
            scb3 = driverScb3;
            scb4 = driverScb4;
        } else {
            /*
             * Sticky/chain strip: bit 6 = chain bit, bits[5:0] = height.
             * Hardware ignores height on chained strips but emulators may read
             * it; keep consistent with update_transform/flush paths.
             */
            scb3 = (uint16_t)(0x0040 | activeRows);
            scb4 = 0;
        }

        vram_sprite(
            scb1Addr,
            1,
            spriteIndex,
            ngsg_tiles,
            ngsg_attrs,
            mapRows,
            scb2,
            scb3,
            scb4
        );
    }
    g->dirty = 0u;
}

void NEOGEO_USER ng_sprite_group_update_transform(NGSpriteGroup *g)
{
    uint8_t strip;
    uint8_t activeRows;
    uint8_t xNibble;
    uint16_t driverScb3;
    uint16_t driverScb4;
    uint16_t scb2;

    if (!g) return;

    if (!g->visible) {
        ng_sprite_group_hide(g);
        return;
    }

    activeRows = g->activeRows ? g->activeRows : g->heightTiles;
    if (activeRows > g->heightTiles) activeRows = g->heightTiles;
    if (activeRows > NG_SPRITE_MAX_HEIGHT_TILES) activeRows = NG_SPRITE_MAX_HEIGHT_TILES;

    activeRows = ng_sprite_display_rows(activeRows, g->yScale);
    xNibble = ngsg_x_shrink_nibble(g->xScale);
    scb2 = setSCB2(xNibble, g->yScale);
    /* Guide specifies 496-Y is the internal coordinate system for vertical pos */
    driverScb3 = setSCB3((uint16_t)(496 - g->y), 0, activeRows);
    driverScb4 = setSCB4((uint16_t)g->x);

    /*
     * VRAM Access Optimization: Write driver registers first.
     * Use individual VRAM calls to ensure hardware timing compliance.
     */
    vram_SCB234((uint16_t)(SCB2_ADDR + g->firstSprite), scb2);
    vram_SCB234((uint16_t)(SCB3_ADDR + g->firstSprite), driverScb3);
    vram_SCB234((uint16_t)(SCB4_ADDR + g->firstSprite), driverScb4);

    for (strip = 1; strip < g->strips; strip++) {
        uint16_t spriteIndex = (uint16_t)(g->firstSprite + strip);

        /*
         * Sticky group movement: bit 6 (0x40) of SCB3 is the chain bit.
         * Horizontal reduction (SCB2) must match driver for consistent width.
         */
        vram_SCB234((uint16_t)(SCB2_ADDR + spriteIndex), scb2);
        vram_SCB234((uint16_t)(SCB3_ADDR + spriteIndex), 0x0040 | activeRows);
    }
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
void NEOGEO_USER ng_sprite_group_flush(NGSpriteGroup *g)
{
    uint8_t strip;
    uint8_t activeRows;
    uint8_t mapRows;
    uint8_t xNibble;
    uint16_t scb2;
    uint16_t driverScb3;
    uint16_t driverScb4;
    uint16_t attr;
    uint8_t row;

    if (!g) return;
    if (!g->dirty) return;

    /* Visibility change: hide and return if not visible */
    if (!g->visible) {
        if (g->dirty & NG_SGF_DIRTY_VIS) ng_sprite_group_hide(g);
        g->dirty = 0;
        return;
    }

    activeRows = g->activeRows ? g->activeRows : g->heightTiles;
    if (activeRows > g->heightTiles) activeRows = g->heightTiles;
    if (activeRows > NG_SPRITE_MAX_HEIGHT_TILES) activeRows = NG_SPRITE_MAX_HEIGHT_TILES;

    activeRows = ng_sprite_display_rows(activeRows, g->yScale);
    mapRows = ng_sprite_map_rows(activeRows);
    /* Compute values once even if some are not needed — branch avoidance */
    xNibble    = ngsg_x_shrink_nibble(g->xScale);
    scb2       = setSCB2(xNibble, g->yScale);
    driverScb3 = setSCB3((uint16_t)(496 - g->y), 0, activeRows);
    driverScb4 = setSCB4((uint16_t)g->x);
    attr       = setSCB1_2(g->palette, 0, g->autoAnim8, g->autoAnim4, g->vflip, g->hflip);

    /* SCB1 tile + attribute upload — only when tile or palette changed */
    if (g->dirty & (NG_SGF_DIRTY_TILE | NG_SGF_DIRTY_PALETTE)) {
        for (strip = 0; strip < g->strips; strip++) {
            uint16_t scb1Addr = (uint16_t)(64u * (uint16_t)(g->firstSprite + strip));

            for (row = 0; row < mapRows; row++) {
                ngsg_tiles[row] = row < g->heightTiles ? ngsg_tile_for(g, strip, row) : NG_SPRITE_BLANK_TILE;
                ngsg_attrs[row] = row < g->heightTiles ? attr : NG_SPRITE_BLANK_ATTR;
            }

            vram_init(scb1Addr, 1);
            vram_SCB1(ngsg_tiles, ngsg_attrs, mapRows);
        }
    }

    /* SCB2 shrink upload */
    if (g->dirty & NG_SGF_DIRTY_SHRINK) {
        for (strip = 0; strip < g->strips; strip++) {
            uint16_t spriteIndex = (uint16_t)(g->firstSprite + strip);
            vram_SCB234((uint16_t)(SCB2_ADDR + spriteIndex), scb2);
        }
    }

    /*
     * SCB3/4.  The driver strip carries X, Y and the active-character count,
     * so it is rewritten for a move, a show, or a scale change - the count is
     * derived from the vertical shrink and goes stale with it.
     */
    if (g->dirty & (NG_SGF_DIRTY_POS | NG_SGF_DIRTY_VIS | NG_SGF_DIRTY_SHRINK)) {
        vram_SCB234((uint16_t)(SCB3_ADDR + g->firstSprite), driverScb3);
        vram_SCB234((uint16_t)(SCB4_ADDR + g->firstSprite), driverScb4);
    }

    /*
     * Chained strips hold only the chain bit and a copy of the count; the
     * hardware reads neither position from them.  A move therefore leaves
     * them alone - re-stamping thirty-odd slots every time a background
     * scrolls one pixel was most of what a scroll cost.  A show still has to
     * write them, because parking a slot clears its chain bit.
     */
    if (g->dirty & (NG_SGF_DIRTY_VIS | NG_SGF_DIRTY_SHRINK)) {
        for (strip = 1; strip < g->strips; strip++) {
            uint16_t spriteIndex = (uint16_t)(g->firstSprite + strip);
            vram_SCB234((uint16_t)(SCB3_ADDR + spriteIndex), (uint16_t)(0x0040 | activeRows));
            vram_SCB234((uint16_t)(SCB4_ADDR + spriteIndex), 0);
        }
    }

    g->dirty = 0;   /* all flushed */
}
