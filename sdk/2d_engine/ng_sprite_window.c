#include "ng_sprite_window.h"
#include "ng_sprite_group.h"
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
    window->current_rows = 0u;
    window->previous_rows = 0u;
    window->visible = 0u;
    window->max_used_strips = 0u;
}

void NEOGEO_USER ng_sprite_window_reset(NGSpriteWindow *window)
{
    if (!window) return;

    window->current_strips = 0u;
    window->previous_strips = 0u;
    window->current_rows = 0u;
    window->previous_rows = 0u;
    window->visible = 0u;
    window->max_used_strips = 0u;
}

void NEOGEO_USER ng_sprite_window_set_current(NGSpriteWindow *window,
                                              uint8_t current_strips)
{
    ng_sprite_window_set_shape(window, current_strips, 0u);
}

void NEOGEO_USER ng_sprite_window_set_shape(NGSpriteWindow *window,
                                            uint8_t current_strips,
                                            uint8_t current_rows)
{
    if (!window) return;

    window->previous_strips = window->current_strips;
    window->previous_rows = window->current_rows;
    window->current_strips = ngsw_clamp_count(current_strips, window->max_strips);
    window->current_rows = current_rows ? current_rows : 1u;
    window->visible = 1u;

    if (window->current_strips > window->max_used_strips) {
        window->max_used_strips = window->current_strips;
    }
}

void NEOGEO_USER ng_sprite_window_clear(NGSpriteWindow *window)
{
    if (!window) return;

    ng_sprite_park_off_range(window->first_slot, window->max_strips);
    window->previous_strips = 0u;
    window->current_strips = 0u;
    window->previous_rows = 0u;
    window->current_rows = 0u;
    window->visible = 0u;
}

void NEOGEO_USER ng_sprite_window_clear_tail(NGSpriteWindow *window)
{
    uint8_t footprint;

    if (!window) return;

    /* Only wipe the range this window has ever actually written to.
     * Using max_strips (typically NG_SPRITE_MAX_STRIPS = 32) here was
     * trashing neighbouring sprite groups: e.g. HERO at slot 64 with
     * 5 strips would zero slots 64..95, taking the ENEMY at slot 80
     * with it.  The neighbour then had to re-upload from scratch,
     * which manifested as split sprites and a one-frame blink. */
    footprint = window->max_used_strips;
    if (footprint == 0u) footprint = window->max_strips;

    if (window->previous_strips == 0u) {
        ng_sprite_park_off_range(window->first_slot, footprint);
        return;
    }

    if (window->previous_rows != window->current_rows) {
        ng_sprite_park_off_range(window->first_slot, footprint);
        return;
    }

    if (window->previous_strips > window->current_strips) {
        ng_sprite_park_off_range(
            (uint16_t)(window->first_slot + window->current_strips),
            (uint16_t)(window->previous_strips - window->current_strips));
    }
}

void NEOGEO_USER ng_sprite_window_hide(NGSpriteWindow *window)
{
    if (!window) return;

    ng_sprite_park_off_range(window->first_slot, window->max_strips);
    window->visible = 0u;
    window->previous_strips = window->current_strips;
    window->current_strips = 0u;
    window->previous_rows = window->current_rows;
    window->current_rows = 0u;
}
