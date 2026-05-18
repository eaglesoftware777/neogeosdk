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

/* Hide a raw hardware-sprite range and clear stale chain/position state. */
void NEOGEO_USER ng_sprite_hide_range(uint16_t firstSprite, uint16_t count);
void NEOGEO_USER ng_sprite_hide_vram_base(uint16_t spriteBase, uint16_t count);
void NEOGEO_USER ng_sprite_hide_all(void);

#endif
