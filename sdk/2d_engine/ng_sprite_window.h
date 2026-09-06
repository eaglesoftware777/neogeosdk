#ifndef NG_SPRITE_WINDOW_H
#define NG_SPRITE_WINDOW_H

#include "ng_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t owner;
    uint16_t first_slot;
    uint8_t max_strips;
    uint8_t current_strips;
    uint8_t previous_strips;
    uint8_t current_rows;
    uint8_t previous_rows;
    uint8_t visible;
    /* High-water mark of strips this window has ever occupied.
     * Bounds the full-window teardown in ng_sprite_window_clear() and
     * ng_sprite_window_hide() to the VRAM this sprite has actually
     * touched, rather than the whole reservation - parking the
     * reservation takes the neighbouring group's slots with it (e.g.
     * HERO at slot 64 parking ENEMY at slot 80). */
    uint8_t max_used_strips;
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


#ifdef __cplusplus
}
#endif
#endif
