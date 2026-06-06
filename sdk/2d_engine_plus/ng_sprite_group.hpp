#ifndef NG_SPRITE_GROUP_HPP
#define NG_SPRITE_GROUP_HPP

#include "ng_defs.hpp"

#define NG_SGF_DIRTY_POS      0x01
#define NG_SGF_DIRTY_TILE     0x02
#define NG_SGF_DIRTY_PALETTE  0x04
#define NG_SGF_DIRTY_SHRINK   0x08
#define NG_SGF_DIRTY_VIS      0x10
#define NG_SGF_DIRTY_ALL      0x1F

/*
 * NGSpriteGroup — hardware sprite group with member-method API.
 *
 * A group is N adjacent 16-px-wide strips sharing tile data.
 * Strip 0 is the driver (owns Y, height, vertical shrink);
 * strips 1..N-1 carry the sticky/chain bit.
 * All data is public so C game code can read fields directly.
 */
struct NGSpriteGroup {
    uint16_t firstSprite;
    uint8_t  strips;
    uint8_t  heightTiles;
    uint8_t  activeRows;
    uint16_t tileBase;
    uint16_t tileStride;
    uint8_t  palette;
    int16_t  x;
    int16_t  y;
    uint8_t  xScale;
    uint8_t  yScale;
    uint8_t  hflip;
    uint8_t  vflip;
    uint8_t  autoAnim4;
    uint8_t  autoAnim8;
    uint8_t  visible;
    uint8_t  dirty;

    void NEOGEO_USER init(uint16_t firstSprite, uint8_t strips, uint8_t heightTiles,
                          uint16_t tileBase, uint8_t palette);
    void NEOGEO_USER markDirty(uint8_t flags);
    void NEOGEO_USER flush();
    void NEOGEO_USER setTileBase(uint16_t tileBase);
    void NEOGEO_USER setTileStride(uint16_t tileStride);
    void NEOGEO_USER setPalette(uint8_t palette);
    void NEOGEO_USER setActiveRows(uint8_t rows);
    void NEOGEO_USER setPos(int16_t x, int16_t y);
    void NEOGEO_USER move(int16_t dx, int16_t dy);
    void NEOGEO_USER setScale(uint8_t xScale, uint8_t yScale);
    void NEOGEO_USER setFlip(uint8_t hflip, uint8_t vflip);
    void NEOGEO_USER setAutoAnim(uint8_t aa4, uint8_t aa8);
    void NEOGEO_USER setVisible(uint8_t v);
    void NEOGEO_USER upload();
    void NEOGEO_USER updateTransform();
    void NEOGEO_USER hide();

    static void NEOGEO_USER hideRange(uint16_t firstSprite, uint16_t count);
    static void NEOGEO_USER hideVramBase(uint16_t spriteBase, uint16_t count);
    static void NEOGEO_USER hideAll();
    static void NEOGEO_USER initHardware(uint16_t transparentTile);

private:
    static uint8_t  clampU8(uint8_t v, uint8_t mn, uint8_t mx);
    static uint8_t  xShrinkNibble(uint8_t xScale);
    uint16_t tileFor(uint8_t strip, uint8_t row) const;
};

#ifdef __cplusplus
extern "C" {
#endif

void NEOGEO_USER ng_sprite_group_init(NGSpriteGroup *g, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette);
void NEOGEO_USER ng_sprite_group_mark_dirty(NGSpriteGroup *g, uint8_t dirty_flags);
void NEOGEO_USER ng_sprite_group_flush(NGSpriteGroup *g);
void NEOGEO_USER ng_sprite_group_set_tile_base(NGSpriteGroup *g, uint16_t tileBase);
void NEOGEO_USER ng_sprite_group_set_tile_stride(NGSpriteGroup *g, uint16_t tileStride);
void NEOGEO_USER ng_sprite_group_set_palette(NGSpriteGroup *g, uint8_t palette);
void NEOGEO_USER ng_sprite_group_set_active_rows(NGSpriteGroup *g, uint8_t activeRows);
void NEOGEO_USER ng_sprite_group_set_pos(NGSpriteGroup *g, int16_t x, int16_t y);
void NEOGEO_USER ng_sprite_group_move(NGSpriteGroup *g, int16_t dx, int16_t dy);
void NEOGEO_USER ng_sprite_group_set_scale(NGSpriteGroup *g, uint8_t xScale, uint8_t yScale);
void NEOGEO_USER ng_sprite_group_set_flip(NGSpriteGroup *g, uint8_t hflip, uint8_t vflip);
void NEOGEO_USER ng_sprite_group_set_auto_anim(NGSpriteGroup *g, uint8_t autoAnim4, uint8_t autoAnim8);
void NEOGEO_USER ng_sprite_group_set_visible(NGSpriteGroup *g, uint8_t visible);
void NEOGEO_USER ng_sprite_group_upload(NGSpriteGroup *g);
void NEOGEO_USER ng_sprite_group_update_transform(NGSpriteGroup *g);
void NEOGEO_USER ng_sprite_group_hide(NGSpriteGroup *g);
void NEOGEO_USER ng_engine_init_hardware(uint16_t transparentTile);
void NEOGEO_USER ng_sprite_hide_range(uint16_t firstSprite, uint16_t count);
void NEOGEO_USER ng_sprite_hide_vram_base(uint16_t spriteBase, uint16_t count);
void NEOGEO_USER ng_sprite_hide_all(void);

/* Blank-tile convention for hardware sprite teardown.  See the
 * C engine companion (sdk/2d_engine/ng_sprite_group.h) for the
 * full rationale: writing literal tile=0 / attr=0 would render
 * monitor-sync black through palette 0; use a reserved blank C-ROM
 * tile and a safe attribute instead. */
#ifndef NG_SPRITE_BLANK_TILE
#define NG_SPRITE_BLANK_TILE  0x00FFu
#endif
#ifndef NG_SPRITE_BLANK_ATTR
#define NG_SPRITE_BLANK_ATTR  0x0000u
#endif

/* Fully disable a single hardware sprite slot.  See the C engine
 * companion for the per-field rationale: ACT=0, chain=0,
 * off-screen Y=496, full scale, every SCB1 row set to
 * NG_SPRITE_BLANK_TILE/NG_SPRITE_BLANK_ATTR.  Use for every unused
 * strip / freed slot, including at scene boundaries. */
void NEOGEO_USER ng_sprite_disable_hw(uint16_t spr);
void NEOGEO_USER ng_sprite_disable_hw_range(uint16_t first, uint16_t count);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
