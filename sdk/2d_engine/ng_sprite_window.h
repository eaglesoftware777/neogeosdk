#ifndef NG_SPRITE_WINDOW_H
#define NG_SPRITE_WINDOW_H

#include "ng_defs.h"

typedef struct {
    uint8_t owner;
    uint16_t first_slot;
    uint8_t max_strips;
    uint8_t current_strips;
    uint8_t previous_strips;
    uint8_t current_rows;
    uint8_t previous_rows;
    uint8_t visible;
} NGSpriteWindow;

void NEOGEO_USER ng_sprite_window_init(NGSpriteWindow *window,
                                       uint8_t owner,
                                       uint16_t first_slot,
                                       uint8_t max_strips);
void NEOGEO_USER ng_sprite_window_reset(NGSpriteWindow *window);
void NEOGEO_USER ng_sprite_window_set_current(NGSpriteWindow *window,
                                              uint8_t current_strips);
void NEOGEO_USER ng_sprite_window_set_shape(NGSpriteWindow *window,
                                            uint8_t current_strips,
                                            uint8_t current_rows);
void NEOGEO_USER ng_sprite_window_clear(NGSpriteWindow *window);
void NEOGEO_USER ng_sprite_window_clear_tail(NGSpriteWindow *window);
void NEOGEO_USER ng_sprite_window_hide(NGSpriteWindow *window);

#endif
