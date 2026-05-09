#include "macro.h"
#include "neogeo.h"
#include "ng_sprite_group.h"

static uint16_t ngsg_tiles[NG_SPRITE_MAX_HEIGHT_TILES];
static uint16_t ngsg_attrs[NG_SPRITE_MAX_HEIGHT_TILES];

static uint8_t ngsg_clamp_u8(uint8_t v, uint8_t min, uint8_t max)
{
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static uint8_t ngsg_x_nibble_from_accum(uint16_t xScale15, uint16_t *accum)
{
    uint8_t base = (uint8_t)((xScale15 >> 8) & 0x0f);
    uint8_t frac = (uint8_t)(xScale15 & 0x00ff);
    uint8_t out = base;

    *accum += frac;
    if (*accum & 0x0100) {
        out++;
        *accum &= 0x00ff;
    }
    if (out > 0x0f) out = 0x0f;
    return out;
}

static uint16_t ngsg_tile_for(NGSpriteGroup *g, uint8_t strip, uint8_t row)
{
    if (g->hflip) strip = (uint8_t)((g->strips - 1) - strip);
    if (g->vflip) row = (uint8_t)((g->heightTiles - 1) - row);
    return (uint16_t)(g->tileBase + ((uint16_t)row * g->tileStride) + strip);
}

void NEOGEO_USER ngSpriteGroupInit(NGSpriteGroup *g, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette)
{
    if (!g) return;
    strips = ngsg_clamp_u8(strips, 1, NG_SPRITE_MAX_STRIPS);
    heightTiles = ngsg_clamp_u8(heightTiles, 1, NG_SPRITE_MAX_HEIGHT_TILES);

    g->firstSprite = firstSprite;
    g->strips = strips;
    g->heightTiles = heightTiles;
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

void NEOGEO_USER ngSpriteGroupSetTileBase(NGSpriteGroup *g, uint16_t tileBase) { if (g) g->tileBase = tileBase; }
void NEOGEO_USER ngSpriteGroupSetTileStride(NGSpriteGroup *g, uint16_t tileStride) { if (g) g->tileStride = tileStride; }
void NEOGEO_USER ngSpriteGroupSetPalette(NGSpriteGroup *g, uint8_t palette) { if (g) g->palette = palette; }
void NEOGEO_USER ngSpriteGroupSetPos(NGSpriteGroup *g, int16_t x, int16_t y) { if (g) { g->x = x; g->y = y; } }
void NEOGEO_USER ngSpriteGroupMove(NGSpriteGroup *g, int16_t dx, int16_t dy) { if (g) { g->x += dx; g->y += dy; } }
void NEOGEO_USER ngSpriteGroupSetScale(NGSpriteGroup *g, uint8_t xScale, uint8_t yScale) { if (g) { g->xScale = xScale; g->yScale = yScale; } }
void NEOGEO_USER ngSpriteGroupSetFlip(NGSpriteGroup *g, uint8_t hflip, uint8_t vflip) { if (g) { g->hflip = hflip ? 1 : 0; g->vflip = vflip ? 1 : 0; } }
void NEOGEO_USER ngSpriteGroupSetAutoAnim(NGSpriteGroup *g, uint8_t autoAnim4, uint8_t autoAnim8) { if (g) { g->autoAnim4 = autoAnim4 ? 1 : 0; g->autoAnim8 = autoAnim8 ? 1 : 0; } }
void NEOGEO_USER ngSpriteGroupSetVisible(NGSpriteGroup *g, uint8_t visible) { if (g) g->visible = visible ? 1 : 0; }

void NEOGEO_USER ngSpriteGroupUpload(NGSpriteGroup *g)
{
    uint8_t strip, row;
    uint16_t xScale15, accum;

    if (!g) return;
    if (!g->visible) { ngSpriteGroupHide(g); return; }

    xScale15 = (uint16_t)g->xScale * 15;
    accum = 0;

    for (strip = 0; strip < g->strips; strip++) {
        uint16_t spriteIndex = (uint16_t)(g->firstSprite + strip);
        uint16_t scb1Addr = (uint16_t)(64 * spriteIndex);
        uint8_t sticky = (strip == 0) ? 0 : 1;
        uint8_t xNibble = ngsg_x_nibble_from_accum(xScale15, &accum);
        uint16_t scb2 = setSCB2(xNibble, g->yScale);
        uint16_t scb3 = setSCB3((uint16_t)(496 - g->y), sticky, g->heightTiles);
        uint16_t scb4 = setSCB4((uint16_t)(g->x + (16 * strip)));

        for (row = 0; row < g->heightTiles; row++) {
            ngsg_tiles[row] = ngsg_tile_for(g, strip, row);
            ngsg_attrs[row] = setSCB1_2(g->palette, 0, g->autoAnim8, g->autoAnim4, g->vflip, g->hflip);
        }

        vram_sprite(scb1Addr, 1, spriteIndex, ngsg_tiles, ngsg_attrs, g->heightTiles, scb2, scb3, scb4);
    }
}

void NEOGEO_USER ngSpriteGroupUpdateTransform(NGSpriteGroup *g)
{
    uint8_t strip;
    uint16_t xScale15, accum;

    if (!g) return;
    if (!g->visible) { ngSpriteGroupHide(g); return; }

    xScale15 = (uint16_t)g->xScale * 15;
    accum = 0;

    for (strip = 0; strip < g->strips; strip++) {
        uint16_t spriteIndex = (uint16_t)(g->firstSprite + strip);
        uint8_t sticky = (strip == 0) ? 0 : 1;
        uint8_t xNibble = ngsg_x_nibble_from_accum(xScale15, &accum);
        uint16_t scb2 = setSCB2(xNibble, g->yScale);
        uint16_t scb3 = setSCB3((uint16_t)(496 - g->y), sticky, g->heightTiles);
        uint16_t scb4 = setSCB4((uint16_t)(g->x + (16 * strip)));

        vram_SCB234((uint16_t)(SCB2_ADDR + spriteIndex), scb2);
        vram_SCB234((uint16_t)(SCB3_ADDR + spriteIndex), scb3);
        vram_SCB234((uint16_t)(SCB4_ADDR + spriteIndex), scb4);
    }
}

void NEOGEO_USER ngSpriteGroupHide(NGSpriteGroup *g)
{
    uint8_t strip;
    if (!g) return;
    for (strip = 0; strip < g->strips; strip++) {
        uint16_t spriteIndex = (uint16_t)(g->firstSprite + strip);
        vram_SCB234((uint16_t)(SCB3_ADDR + spriteIndex), 0);
    }
}
