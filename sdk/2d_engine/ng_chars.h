#ifndef NG_CHARS_H
#define NG_CHARS_H

#include "ng_defs.h"
#include "ng_sprite_group.h"

/*
 * Public character handle.
 *
 * Several SDK modules use NGCharacter directly in their public prototypes:
 * ng_npcs.h, ng_physics.h, ng_actions.h, and game code.  Therefore the
 * typedef must exist before those headers are parsed.
 */
typedef struct NGCharacter NGCharacter;

typedef void(*NGCharInterupt)(NGCharacter *c);

struct NGCharacter {
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
    uint8_t sprite_active_rows;
    uint8_t palette;
    uint8_t scale_x;
    uint8_t scale_y;
    uint8_t flip_x;
    uint8_t flip_y;
    uint8_t sprite_dirty;

    /*
     * Per-frame draw offsets from the generated Artbox metadata.
     * They keep cropped sprite frames aligned to the original source canvas.
     */
    int16_t sprite_offset_x;
    int16_t sprite_offset_y;

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
};

void NEOGEO_USER ng_chars_init(void);
NGCharacter* NEOGEO_USER chars_add(uint8_t kind, int16_t x, int16_t y);
void NEOGEO_USER ng_chars_remove(NGCharacter *c);
void NEOGEO_USER ng_chars_clear_kind(uint8_t kind);
NGCharacter* NEOGEO_USER chars_find(uint8_t kind);
NGCharacter* NEOGEO_USER chars_at(uint8_t index);
uint8_t NEOGEO_USER ng_chars_count(void);
uint8_t NEOGEO_USER ng_chars_index(NGCharacter *c);
void NEOGEO_USER ng_chars_set_game_interupt(uint8_t kind, NGCharInterupt fn);
void NEOGEO_USER ng_chars_update(void);
void NEOGEO_USER ng_chars_draw(void);

void NEOGEO_USER ng_char_set_sprite(NGCharacter *c, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette);
void NEOGEO_USER ng_char_set_body(NGCharacter *c, int16_t x, int16_t y, int16_t w, int16_t h);
void NEOGEO_USER ng_char_set_pos(NGCharacter *c, int16_t x, int16_t y);
void NEOGEO_USER ng_char_set_speed(NGCharacter *c, int16_t vx_px, int16_t vy_px);
void NEOGEO_USER ng_char_damage(NGCharacter *c, uint8_t amount);
void NEOGEO_USER ng_char_heal(NGCharacter *c, uint8_t amount);
NGRect NEOGEO_USER ng_char_body_rect(NGCharacter *c);
NGRect NEOGEO_USER ng_char_hit_rect(NGCharacter *c);

#endif
