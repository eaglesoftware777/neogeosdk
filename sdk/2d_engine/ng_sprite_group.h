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
} NGSpriteGroup;

void NEOGEO_USER ng_sprite_group_init(NGSpriteGroup *g, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette);
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

/* Hide a raw hardware-sprite range by clearing SCB3 height. */
void NEOGEO_USER ng_sprite_hide_range(uint16_t firstSprite, uint8_t count);

#endif
