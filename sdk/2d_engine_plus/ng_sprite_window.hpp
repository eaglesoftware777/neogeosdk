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
     * clear() and hide() to bound full-window teardown to the VRAM
     * range this sprite has actually written, instead of parking the
     * whole max_strips reservation and taking the adjacent sprite's
     * slots with it. */
    uint8_t max_used_strips;

    void NEOGEO_USER init(uint8_t owner, uint16_t first_slot, uint8_t max_strips);
    void NEOGEO_USER reset();
    void NEOGEO_USER setCurrent(uint8_t current_strips);
    void NEOGEO_USER setShape(uint8_t current_strips, uint8_t current_rows);
    uint8_t NEOGEO_USER footprint() const;
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
