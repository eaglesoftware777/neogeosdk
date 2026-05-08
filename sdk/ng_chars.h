#ifndef NG_CHARS_H
#define NG_CHARS_H

#include "ng_defs.h"
#include "ng_sprite_group.h"

/*
 * Character: one active entry in the game table.
 * It can be Maiya, enemy, NPC, chest, platform, item, FX, boss, etc.
 */
typedef struct NGCharacter {
    uint8_t active;
    uint8_t kind;
    uint8_t state;
    uint8_t facing;
    uint8_t visible;

    int16_t x;
    int16_t y;
    int32_t x_fp;
    int32_t y_fp;
    int32_t vx_fp;
    int32_t vy_fp;

    uint16_t action;
    uint16_t action_pos;
    uint16_t action_timer;

    uint16_t sprite_first;
    uint16_t sprite_tile;
    uint16_t sprite_stride;
    uint8_t sprite_strips;
    uint8_t sprite_height;
    uint8_t palette;
    uint8_t scale_x;
    uint8_t scale_y;
    uint8_t flip_x;
    uint8_t flip_y;
    uint8_t sprite_dirty;

    int16_t body_x;
    int16_t body_y;
    int16_t body_w;
    int16_t body_h;

    int16_t hit_x;
    int16_t hit_y;
    int16_t hit_w;
    int16_t hit_h;

    uint8_t hp;
    uint8_t max_hp;
    uint16_t flags;

    uint16_t data0;
    uint16_t data1;
    uint16_t data2;
} NGCharacter;

typedef void (*NGCharInterupt)(NGCharacter *c);

void chars_init(void);
NGCharacter* chars_add(uint8_t kind, int16_t x, int16_t y);
void chars_remove(NGCharacter *c);
void chars_clear_kind(uint8_t kind);
NGCharacter* chars_find(uint8_t kind);
NGCharacter* chars_at(uint8_t index);
uint8_t chars_count(void);

void chars_set_game_interupt(uint8_t kind, NGCharInterupt fn);
void chars_update(void);
void chars_draw(void);

void char_set_sprite(NGCharacter *c, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette);
void char_set_body(NGCharacter *c, int16_t x, int16_t y, int16_t w, int16_t h);
void char_set_pos(NGCharacter *c, int16_t x, int16_t y);
void char_set_speed(NGCharacter *c, int16_t vx_px, int16_t vy_px);
void char_damage(NGCharacter *c, uint8_t amount);
void char_heal(NGCharacter *c, uint8_t amount);
NGRect char_body_rect(NGCharacter *c);
NGRect char_hit_rect(NGCharacter *c);

#endif
