#include "ng_sprite_window.h"
#include "ng_vram.h"

static uint8_t NEOGEO_USER ngsw_clamp_count(uint8_t value, uint8_t max_value)
{
    if (value == 0u) return 1u;
    if (value > max_value) return max_value;
    return value;
}

void NEOGEO_USER ng_sprite_window_init(NGSpriteWindow *window,
                                       uint8_t owner,
                                       uint16_t first_slot,
                                       uint8_t max_strips)
{
    if (!window) return;

    window->owner = owner;
    window->first_slot = first_slot;
    window->max_strips = max_strips ? max_strips : 1u;
    window->current_strips = 0u;
    window->previous_strips = 0u;
    window->visible = 0u;
}

void NEOGEO_USER ng_sprite_window_reset(NGSpriteWindow *window)
{
    if (!window) return;

    window->current_strips = 0u;
    window->previous_strips = 0u;
    window->visible = 0u;
}

void NEOGEO_USER ng_sprite_window_set_current(NGSpriteWindow *window,
                                              uint8_t current_strips)
{
    if (!window) return;

    window->previous_strips = window->current_strips;
    window->current_strips = ngsw_clamp_count(current_strips, window->max_strips);
    window->visible = 1u;
}

void NEOGEO_USER ng_sprite_window_clear(NGSpriteWindow *window)
{
    if (!window) return;

    ng_vram_clear_sprite_range(window->first_slot, window->max_strips);
    window->previous_strips = 0u;
    window->current_strips = 0u;
    window->visible = 0u;
}

void NEOGEO_USER ng_sprite_window_clear_tail(NGSpriteWindow *window)
{
    if (!window) return;

    if (window->previous_strips == 0u) {
        ng_vram_clear_sprite_range(window->first_slot, window->max_strips);
        return;
    }

    if (window->previous_strips > window->current_strips) {
        ng_vram_clear_sprite_range(
            (uint16_t)(window->first_slot + window->current_strips),
            (uint16_t)(window->previous_strips - window->current_strips));
    }
}

void NEOGEO_USER ng_sprite_window_hide(NGSpriteWindow *window)
{
    if (!window) return;

    ng_vram_clear_sprite_range(window->first_slot, window->max_strips);
    window->visible = 0u;
    window->previous_strips = window->current_strips;
    window->current_strips = 0u;
}
