#ifndef NG_SPRITE_GROUP_H
#define NG_SPRITE_GROUP_H

#include "ng_defs.h"

/*
 * Hardware sprite group.
 *
 * Neo Geo sprites are 16px-wide vertical strips.  A large object is a group
 * of adjacent strips, normally using the SCB3 sticky/chain bit for strips
 * 1..N.  The first strip is the driver; chained strips inherit Y, height and
 * vertical shrink from the driver, while each strip still needs its own tile
 * map, palette attributes and horizontal shrink.
 */
/*
 * Dirty flag bits for NGSpriteGroup.
 * Only the bits that are set get written to VRAM on the next update.
 * This avoids re-uploading all SCB fields every frame for static objects.
 */
#define NG_SGF_DIRTY_POS      0x01  /* x/y changed → write SCB3/SCB4 */
#define NG_SGF_DIRTY_TILE     0x02  /* tileBase/stride changed → write SCB1 */
#define NG_SGF_DIRTY_PALETTE  0x04  /* palette changed → update SCB1 attrs */
#define NG_SGF_DIRTY_SHRINK   0x08  /* scale changed → write SCB2 */
#define NG_SGF_DIRTY_VIS      0x10  /* visibility changed */
#define NG_SGF_DIRTY_ALL      0x1F  /* force full upload */

typedef struct {
    uint16_t firstSprite;
    uint8_t strips;
    uint8_t heightTiles;
    uint8_t activeRows;
    uint16_t tileBase;
    uint16_t tileStride;
    uint8_t palette;
    int16_t x;
    int16_t y;
    uint8_t xScale;
    uint8_t yScale;
    uint8_t hflip;
    uint8_t vflip;
    uint8_t autoAnim4;
    uint8_t autoAnim8;
    uint8_t visible;
    uint8_t dirty;      /* bitmask of NG_SGF_DIRTY_* flags */
} NGSpriteGroup;

void NEOGEO_USER ng_sprite_group_init(NGSpriteGroup *g, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette);
/* Mark specific attributes dirty so the next flush only uploads changed data. */
void NEOGEO_USER ng_sprite_group_mark_dirty(NGSpriteGroup *g, uint8_t dirty_flags);
/* Dirty-aware flush: only writes VRAM regions flagged in g->dirty. */
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

/*
 * Blank-tile convention for hardware sprite teardown.
 *
 * ng_sprite_disable_hw() fills every SCB1 row of a slot with a
 * (tile, attr) pair.  If we wrote (0, 0) the LSPC would still
 * happily render C-ROM tile 0 through palette bank 0 — and on
 * NeoGeo, palette[0][0] is the monitor-sync reference colour
 * (pure black).  Combined with any non-transparent pixel in
 * tile 0, that paints a black rectangle wherever the slot's Y
 * happens to fall on-screen.
 *
 * Convention used by the engine and by the artbox C-ROM packer:
 *   - Tile 0x00FF is reserved as an always-transparent C-ROM tile
 *     (matches NGFIX_DEFAULT_BLANK_TILE for the FIX layer).
 *   - Attribute 0x0000 keeps palette/flip bits zero.  With a fully
 *     transparent tile the chosen palette is irrelevant, but using
 *     0 keeps the disabled slot identifiable in VRAM dumps.
 *
 * If a project ships its own C-ROM, ensure tile 0x00FF is empty
 * (all pixel indices = 0) or override these constants before
 * including this header.
 */
#ifndef NG_SPRITE_BLANK_TILE
#define NG_SPRITE_BLANK_TILE  0x00FFu
#endif
#ifndef NG_SPRITE_BLANK_ATTR
#define NG_SPRITE_BLANK_ATTR  0x0000u
#endif

/* Hide a raw hardware-sprite range and clear stale chain/position state. */
void NEOGEO_USER ng_sprite_hide_range(uint16_t firstSprite, uint16_t count);
void NEOGEO_USER ng_sprite_hide_vram_base(uint16_t spriteBase, uint16_t count);
void NEOGEO_USER ng_sprite_hide_all(void);

/*
 * Fully disable a single hardware sprite slot.
 *
 * Neo Geo sprites are 16px-wide vertical strips chained via the SCB3
 * sticky bit; a wide sprite is N adjacent strips with strip[0] as the
 * driver and strip[1..N-1] inheriting position/scale via the chain.
 * If the chain bit on an old strip is left set after a shrink, that
 * strip stays attached to the new driver and shows leftover tile
 * data at the new sprite's X position — the "old object stuck to new
 * char" symptom.
 *
 * ng_sprite_disable_hw() performs the complete teardown of a slot:
 *   - ACT (SCB3 height field) = 0
 *   - chain (SCB3 bit 6)      = 0
 *   - position                = off-screen (Y_pos=496, X=0)
 *   - scale                   = full size (SCB2 = 0x0FFF)
 *   - tile / attr (SCB1[0..1])= 0
 *
 * Use it for every unused strip and every freed slot, including at
 * scene boundaries.
 */
void NEOGEO_USER ng_sprite_disable_hw(uint16_t spr);
void NEOGEO_USER ng_sprite_disable_hw_range(uint16_t first, uint16_t count);

#endif
