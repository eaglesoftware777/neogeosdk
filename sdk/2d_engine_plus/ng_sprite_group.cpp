#include "ng_hw.hpp"
#include "ng_sprite_group.hpp"
#include "ng_sprite_pool.hpp"
#include "ng_vram.hpp"

static uint16_t ngsg_tiles[NG_SPRITE_MAX_HEIGHT_TILES];
static uint16_t ngsg_attrs[NG_SPRITE_MAX_HEIGHT_TILES];

/* --- NGSpriteGroup private helpers --- */

uint8_t NGSpriteGroup::clampU8(uint8_t v, uint8_t mn, uint8_t mx)
{
    if (v < mn) return mn;
    if (v > mx) return mx;
    return v;
}

uint8_t NGSpriteGroup::xShrinkNibble(uint8_t xScale)
{
    if (xScale >= NG_SPRITE_FULL_XSCALE) return 0x0f;
    return (uint8_t)((xScale >> 4) & 0x0f);
}

uint16_t NGSpriteGroup::tileFor(uint8_t strip, uint8_t row) const
{
    uint8_t s = strip;
    uint8_t r = row;
    if (hflip) s = (uint8_t)((strips - 1u) - s);
    if (vflip) r = (uint8_t)((heightTiles - 1u) - r);
    return (uint16_t)(tileBase + ((uint16_t)r * tileStride) + s);
}

/* --- NGSpriteGroup static methods --- */

void NEOGEO_USER ng_sprite_disable_hw(uint16_t spr)
{
    uint16_t i;
    uint16_t scb1_base;

    if (spr >= NG_SPR_TOTAL) return;

    /* 1. Kill display first: ACT=0, chain=0, Y_field=256 so
     *    screen_y=240 (past the 224-line visible window).  Note
     *    Y_field=496 would resolve to screen_y=0 / top of screen
     *    — visible, not off-screen. */
    vram_SCB234((uint16_t)(SCB3_ADDR + spr), NG_SPRITE_DISABLED_SCB3);

    /* 2. Normalise scale (full size) and park X off-screen right. */
    vram_SCB234((uint16_t)(SCB2_ADDR + spr), 0x0FFFu);
    vram_SCB234((uint16_t)(SCB4_ADDR + spr), NG_SPRITE_DISABLED_X);

    /* 3. FULL SCB1 clear — 32 rows × (tile, attr) per slot.
     *    Writing tile=0/attr=0 would render C-ROM tile 0 through
     *    palette bank 0 (monitor-sync black) and paint a black
     *    rectangle on any slot whose SCB3 ever bumps off 0.  Use
     *    the project's reserved blank tile so the worst case is
     *    fully transparent. */
    scb1_base = (uint16_t)(64u * spr);
    vram_init(scb1_base, 1u);
    for (i = 0u; i < 32u; i++) {
        vram_sfix1(NG_SPRITE_BLANK_TILE);
        vram_sfix1(NG_SPRITE_BLANK_ATTR);
    }
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
    uint16_t scb1_base;

    if (spr >= NG_SPR_TOTAL) return;

    /* Per-frame hot path: kill SCB3, normalise SCB2/SCB4, blank
     * SCB1 row 0.  The row-0 wipe is required because some real
     * boards treat SCB3 height=0 as "32 rows with Y-wrap" rather
     * than "0 rows", which would let leftover tile data render as
     * a horizontal strip across the screen. */
    vram_SCB234((uint16_t)(SCB3_ADDR + spr), NG_SPRITE_DISABLED_SCB3);
    vram_SCB234((uint16_t)(SCB2_ADDR + spr), 0x0FFFu);
    vram_SCB234((uint16_t)(SCB4_ADDR + spr), NG_SPRITE_DISABLED_X);

    scb1_base = (uint16_t)(64u * spr);
    vram_init(scb1_base, 1u);
    vram_sfix1(NG_SPRITE_BLANK_TILE);
    vram_sfix1(NG_SPRITE_BLANK_ATTR);
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

void NGSpriteGroup::hideRange(uint16_t first, uint16_t count)
{
    /* Per-frame hot path: light park, not full SCB1 wipe.  Scene
     * boundaries still go through hideAll -> heavy disable. */
    ng_sprite_park_off_range(first, count);
}

void NGSpriteGroup::hideVramBase(uint16_t spriteBase, uint16_t count)
{
    ng_vram_clear_sprite_vram_base(spriteBase, count);
}

void NGSpriteGroup::hideAll()
{
    ng_vram_clear_all_sprites();
}

void NGSpriteGroup::initHardware(uint16_t transparentTile)
{
    uint16_t i;
    vram_init(0, 1);
    for (i = 0; i < 0x40; i++) vram_sfix1(transparentTile);
    vram_SCB234(0x8200, 0);
}

/* --- NGSpriteGroup member methods --- */

void NGSpriteGroup::init(uint16_t first, uint8_t s, uint8_t h, uint16_t tb, uint8_t pal)
{
    s = clampU8(s, 1, NG_SPRITE_MAX_STRIPS);
    h = clampU8(h, 1, NG_SPRITE_MAX_HEIGHT_TILES);
    firstSprite = first;
    strips      = s;
    heightTiles = h;
    activeRows  = h;
    tileBase    = tb;
    tileStride  = s;
    palette     = pal;
    x = 0; y = 0;
    xScale     = NG_SPRITE_FULL_XSCALE;
    yScale     = NG_SPRITE_FULL_YSCALE;
    hflip      = 0;
    vflip      = 0;
    autoAnim4  = 0;
    autoAnim8  = 0;
    visible    = 1;
    dirty      = NG_SGF_DIRTY_ALL;
}

void NGSpriteGroup::markDirty(uint8_t flags) { dirty |= flags; }

void NGSpriteGroup::setTileBase(uint16_t tb)
{
    tileBase = tb;
    dirty |= NG_SGF_DIRTY_TILE;
}

void NGSpriteGroup::setTileStride(uint16_t ts)
{
    tileStride = ts ? ts : strips;
}

void NGSpriteGroup::setPalette(uint8_t pal)
{
    palette = pal;
    dirty |= NG_SGF_DIRTY_PALETTE;
}

void NGSpriteGroup::setActiveRows(uint8_t rows)
{
    if (rows < 1) rows = 1;
    if (rows > heightTiles) rows = heightTiles;
    if (rows > NG_SPRITE_MAX_HEIGHT_TILES) rows = NG_SPRITE_MAX_HEIGHT_TILES;
    activeRows = rows;
}

void NGSpriteGroup::setPos(int16_t px, int16_t py)
{
    x = px; y = py;
    dirty |= NG_SGF_DIRTY_POS;
}

void NGSpriteGroup::move(int16_t dx, int16_t dy)
{
    x += dx; y += dy;
    dirty |= NG_SGF_DIRTY_POS;
}

void NGSpriteGroup::setScale(uint8_t sx, uint8_t sy)
{
    xScale = sx; yScale = sy;
    dirty |= NG_SGF_DIRTY_SHRINK;
}

void NGSpriteGroup::setFlip(uint8_t h, uint8_t v)
{
    hflip = h ? 1 : 0;
    vflip = v ? 1 : 0;
}

void NGSpriteGroup::setAutoAnim(uint8_t aa4, uint8_t aa8)
{
    autoAnim4 = aa4 ? 1 : 0;
    autoAnim8 = aa8 ? 1 : 0;
}

void NGSpriteGroup::setVisible(uint8_t v)
{
    visible = v ? 1 : 0;
    dirty |= NG_SGF_DIRTY_VIS;
}

void NGSpriteGroup::hide()
{
    hideRange(firstSprite, strips);
}

void NGSpriteGroup::upload()
{
    uint8_t strip, row, ar;
    uint8_t xn;
    uint16_t scb2, driverScb3, driverScb4, attr;

    if (!visible) { hide(); return; }

    ar = activeRows ? activeRows : heightTiles;
    if (ar > heightTiles) ar = heightTiles;
    if (ar > NG_SPRITE_MAX_HEIGHT_TILES) ar = NG_SPRITE_MAX_HEIGHT_TILES;

    xn          = xShrinkNibble(xScale);
    scb2        = setSCB2(xn, yScale);
    driverScb3  = setSCB3((uint16_t)(496 - y), 0, ar);
    driverScb4  = setSCB4((uint16_t)x);
    attr        = setSCB1_2(palette, 0, autoAnim8, autoAnim4, vflip, hflip);

    for (strip = 0; strip < strips; strip++) {
        uint16_t spriteIndex = (uint16_t)(firstSprite + strip);
        uint16_t scb1Addr    = (uint16_t)(64u * spriteIndex);
        uint16_t scb3, scb4;

        for (row = 0; row < heightTiles; row++) {
            ngsg_tiles[row] = tileFor(strip, row);
            ngsg_attrs[row] = attr;
        }

        if (strip == 0) {
            scb3 = driverScb3;
            scb4 = driverScb4;
        } else {
            scb3 = (uint16_t)(0x0040 | ar);
            scb4 = 0;
        }

        vram_sprite(scb1Addr, 1, spriteIndex,
                    ngsg_tiles, ngsg_attrs, heightTiles,
                    scb2, scb3, scb4);
    }
}

void NGSpriteGroup::updateTransform()
{
    uint8_t strip, ar, xn;
    uint16_t scb2, driverScb3, driverScb4;

    if (!visible) { hide(); return; }

    ar = activeRows ? activeRows : heightTiles;
    if (ar > heightTiles) ar = heightTiles;
    if (ar > NG_SPRITE_MAX_HEIGHT_TILES) ar = NG_SPRITE_MAX_HEIGHT_TILES;

    xn          = xShrinkNibble(xScale);
    scb2        = setSCB2(xn, yScale);
    driverScb3  = setSCB3((uint16_t)(496 - y), 0, ar);
    driverScb4  = setSCB4((uint16_t)x);

    vram_SCB234((uint16_t)(SCB2_ADDR + firstSprite), scb2);
    vram_SCB234((uint16_t)(SCB3_ADDR + firstSprite), driverScb3);
    vram_SCB234((uint16_t)(SCB4_ADDR + firstSprite), driverScb4);

    for (strip = 1; strip < strips; strip++) {
        uint16_t si = (uint16_t)(firstSprite + strip);
        vram_SCB234((uint16_t)(SCB2_ADDR + si), scb2);
        vram_SCB234((uint16_t)(SCB3_ADDR + si), (uint16_t)(0x0040 | ar));
    }
}

void NGSpriteGroup::flush()
{
    uint8_t strip, row, ar, xn;
    uint16_t scb2, driverScb3, driverScb4, attr;

    if (!dirty) return;

    if ((dirty & NG_SGF_DIRTY_VIS) && !visible) {
        hide();
        dirty = 0;
        return;
    }

    ar = activeRows ? activeRows : heightTiles;
    if (ar > heightTiles) ar = heightTiles;
    if (ar > NG_SPRITE_MAX_HEIGHT_TILES) ar = NG_SPRITE_MAX_HEIGHT_TILES;

    xn         = xShrinkNibble(xScale);
    scb2       = setSCB2(xn, yScale);
    driverScb3 = setSCB3((uint16_t)(496 - y), 0, ar);
    driverScb4 = setSCB4((uint16_t)x);
    attr       = setSCB1_2(palette, 0, autoAnim8, autoAnim4, vflip, hflip);

    if (dirty & (NG_SGF_DIRTY_TILE | NG_SGF_DIRTY_PALETTE)) {
        for (strip = 0; strip < strips; strip++) {
            uint16_t scb1Addr = (uint16_t)(64u * (uint16_t)(firstSprite + strip));
            for (row = 0; row < heightTiles; row++) {
                ngsg_tiles[row] = tileFor(strip, row);
                ngsg_attrs[row] = attr;
            }
            vram_init(scb1Addr, 1);
            vram_SCB1(ngsg_tiles, ngsg_attrs, heightTiles);
        }
    }

    if (dirty & NG_SGF_DIRTY_SHRINK) {
        for (strip = 0; strip < strips; strip++) {
            uint16_t si = (uint16_t)(firstSprite + strip);
            vram_SCB234((uint16_t)(SCB2_ADDR + si), scb2);
        }
    }

    if (dirty & (NG_SGF_DIRTY_POS | NG_SGF_DIRTY_VIS)) {
        vram_SCB234((uint16_t)(SCB3_ADDR + firstSprite), driverScb3);
        vram_SCB234((uint16_t)(SCB4_ADDR + firstSprite), driverScb4);
        for (strip = 1; strip < strips; strip++) {
            uint16_t si = (uint16_t)(firstSprite + strip);
            vram_SCB234((uint16_t)(SCB3_ADDR + si), (uint16_t)(0x0040 | ar));
            vram_SCB234((uint16_t)(SCB4_ADDR + si), 0);
        }
    }

    dirty = 0;
}

/* --- extern "C" wrappers — identical signatures, delegate to methods --- */

extern "C" {

void NEOGEO_USER ng_sprite_hide_range(uint16_t firstSprite, uint16_t count)
{
    NGSpriteGroup::hideRange(firstSprite, count);
}

void NEOGEO_USER ng_sprite_hide_vram_base(uint16_t spriteBase, uint16_t count)
{
    NGSpriteGroup::hideVramBase(spriteBase, count);
}

void NEOGEO_USER ng_sprite_hide_all(void)
{
    NGSpriteGroup::hideAll();
}

void NEOGEO_USER ng_engine_init_hardware(uint16_t transparentTile)
{
    NGSpriteGroup::initHardware(transparentTile);
}

void NEOGEO_USER ng_sprite_group_init(NGSpriteGroup *g, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette)
{
    if (g) g->init(firstSprite, strips, heightTiles, tileBase, palette);
}

void NEOGEO_USER ng_sprite_group_mark_dirty(NGSpriteGroup *g, uint8_t dirty_flags)
{
    if (g) g->markDirty(dirty_flags);
}

void NEOGEO_USER ng_sprite_group_flush(NGSpriteGroup *g)
{
    if (g) g->flush();
}

void NEOGEO_USER ng_sprite_group_set_tile_base(NGSpriteGroup *g, uint16_t tileBase)
{
    if (g) g->setTileBase(tileBase);
}

void NEOGEO_USER ng_sprite_group_set_tile_stride(NGSpriteGroup *g, uint16_t tileStride)
{
    if (g) g->setTileStride(tileStride);
}

void NEOGEO_USER ng_sprite_group_set_palette(NGSpriteGroup *g, uint8_t palette)
{
    if (g) g->setPalette(palette);
}

void NEOGEO_USER ng_sprite_group_set_active_rows(NGSpriteGroup *g, uint8_t activeRows)
{
    if (g) g->setActiveRows(activeRows);
}

void NEOGEO_USER ng_sprite_group_set_pos(NGSpriteGroup *g, int16_t x, int16_t y)
{
    if (g) g->setPos(x, y);
}

void NEOGEO_USER ng_sprite_group_move(NGSpriteGroup *g, int16_t dx, int16_t dy)
{
    if (g) g->move(dx, dy);
}

void NEOGEO_USER ng_sprite_group_set_scale(NGSpriteGroup *g, uint8_t xScale, uint8_t yScale)
{
    if (g) g->setScale(xScale, yScale);
}

void NEOGEO_USER ng_sprite_group_set_flip(NGSpriteGroup *g, uint8_t hflip, uint8_t vflip)
{
    if (g) g->setFlip(hflip, vflip);
}

void NEOGEO_USER ng_sprite_group_set_auto_anim(NGSpriteGroup *g, uint8_t autoAnim4, uint8_t autoAnim8)
{
    if (g) g->setAutoAnim(autoAnim4, autoAnim8);
}

void NEOGEO_USER ng_sprite_group_set_visible(NGSpriteGroup *g, uint8_t visible)
{
    if (g) g->setVisible(visible);
}

void NEOGEO_USER ng_sprite_group_upload(NGSpriteGroup *g)
{
    if (g) g->upload();
}

void NEOGEO_USER ng_sprite_group_update_transform(NGSpriteGroup *g)
{
    if (g) g->updateTransform();
}

void NEOGEO_USER ng_sprite_group_hide(NGSpriteGroup *g)
{
    if (g) g->hide();
}

} /* extern "C" */
