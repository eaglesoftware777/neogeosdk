#include "macro.h"
#include "neogeo.h"
#include "ng_sprite_group.h"

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

void NEOGEO_USER ng_sprite_hide_range(uint16_t firstSprite, uint8_t count)
{
    uint8_t i;

    if (firstSprite == 0xffff) return;
    if (count > NG_SPRITE_MAX_STRIPS) count = NG_SPRITE_MAX_STRIPS;

    for (i = 0; i < count; i++) {
        uint16_t spriteIndex = (uint16_t)(firstSprite + i);
        /*
         * Clear SCB3 to turn the sprite off (zero height = invisible).
         * No waitVbl() here: VRAM writes are effective immediately; waiting
         * per-sprite would stall the CPU for count full frames.
         */
        vram_SCB234((uint16_t)(SCB3_ADDR + spriteIndex), 0);
    }
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
}

void NEOGEO_USER ng_sprite_group_set_tile_base(NGSpriteGroup *g, uint16_t tileBase)
{
    if (g) g->tileBase = tileBase;
}

void NEOGEO_USER ng_sprite_group_set_tile_stride(NGSpriteGroup *g, uint16_t tileStride)
{
    if (g) g->tileStride = tileStride ? tileStride : g->strips;
}

void NEOGEO_USER ng_sprite_group_set_palette(NGSpriteGroup *g, uint8_t palette)
{
    if (g) g->palette = palette;
}

void NEOGEO_USER ng_sprite_group_set_active_rows(NGSpriteGroup *g, uint8_t activeRows)
{
    if (!g) return;

    if (activeRows < 1) activeRows = 1;
    if (activeRows > g->heightTiles) activeRows = g->heightTiles;
    if (activeRows > NG_SPRITE_MAX_HEIGHT_TILES) activeRows = NG_SPRITE_MAX_HEIGHT_TILES;

    g->activeRows = activeRows;
}

void NEOGEO_USER ng_sprite_group_set_pos(NGSpriteGroup *g, int16_t x, int16_t y)
{
    if (g) {
        g->x = x;
        g->y = y;
    }
}

void NEOGEO_USER ng_sprite_group_move(NGSpriteGroup *g, int16_t dx, int16_t dy)
{
    if (g) {
        g->x += dx;
        g->y += dy;
    }
}

void NEOGEO_USER ng_sprite_group_set_scale(NGSpriteGroup *g, uint8_t xScale, uint8_t yScale)
{
    if (g) {
        g->xScale = xScale;
        g->yScale = yScale;
    }
}

void NEOGEO_USER ng_sprite_group_set_flip(NGSpriteGroup *g, uint8_t hflip, uint8_t vflip)
{
    if (g) {
        g->hflip = hflip ? 1 : 0;
        g->vflip = vflip ? 1 : 0;
    }
}

void NEOGEO_USER ng_sprite_group_set_auto_anim(NGSpriteGroup *g, uint8_t autoAnim4, uint8_t autoAnim8)
{
    if (g) {
        g->autoAnim4 = autoAnim4 ? 1 : 0;
        g->autoAnim8 = autoAnim8 ? 1 : 0;
    }
}

void NEOGEO_USER ng_sprite_group_set_visible(NGSpriteGroup *g, uint8_t visible)
{
    if (g) g->visible = visible ? 1 : 0;
}

void NEOGEO_USER ng_sprite_group_upload(NGSpriteGroup *g)
{
    uint8_t strip;
    uint8_t row;
    uint8_t activeRows;
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

        for (row = 0; row < g->heightTiles; row++) {
            ngsg_tiles[row] = ngsg_tile_for(g, strip, row);
            ngsg_attrs[row] = attr;
        }

        if (strip == 0) {
            /*
             * Driving strip: owns X, Y, height and vertical shrink.
             */
            scb3 = driverScb3;
            scb4 = driverScb4;
        } else {
            /*
             * Sticky/chain strip: SCB3 must be sticky bit only.
             * Hardware glues this strip directly to the right of the previous
             * strip and inherits Y, height and vertical shrink from the driver.
             * SCB4 is ignored while sticky is set, but keep it deterministic.
             */
            scb3 = 0x0040;
            scb4 = 0;
        }

        vram_sprite(
            scb1Addr,
            1,
            spriteIndex,
            ngsg_tiles,
            ngsg_attrs,
            g->heightTiles,
            scb2,
            scb3,
            scb4
        );
    }
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
