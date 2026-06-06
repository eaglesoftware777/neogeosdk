#ifndef NG_SPRITE_WINDOW_HPP
#define NG_SPRITE_WINDOW_HPP

#include "ng_defs.hpp"

struct NGSpriteWindow {
    uint8_t owner;
    uint16_t first_slot;
    uint8_t max_strips;
    uint8_t current_strips;
    uint8_t previous_strips;
    uint8_t current_rows;
    uint8_t previous_rows;
    uint8_t visible;
    /* High-water mark of strips this window has ever held — used by
     * clearTail to bound full-window wipes to the actual VRAM range
     * this sprite has ever written, instead of nuking the whole
     * max_strips window and stomping on adjacent sprite groups. */
    uint8_t max_used_strips;

    void NEOGEO_USER init(uint8_t owner, uint16_t first_slot, uint8_t max_strips);
    void NEOGEO_USER reset();
    void NEOGEO_USER setCurrent(uint8_t current_strips);
    void NEOGEO_USER setShape(uint8_t current_strips, uint8_t current_rows);
    void NEOGEO_USER clear();
    void NEOGEO_USER clearTail();
    void NEOGEO_USER hide();
};

#ifdef __cplusplus
extern "C" {
#endif

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
