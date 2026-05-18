#include "ng_sprite_window.hpp"
#include "ng_vram.hpp"

static uint8_t ngsw_clamp_count(uint8_t value, uint8_t max_value)
{
    if (value == 0u) return 1u;
    if (value > max_value) return max_value;
    return value;
}

void NGSpriteWindow::init(uint8_t owner_kind, uint16_t first, uint8_t max_count)
{
    owner = owner_kind;
    first_slot = first;
    max_strips = max_count ? max_count : 1u;
    current_strips = 0u;
    previous_strips = 0u;
    visible = 0u;
}

void NGSpriteWindow::reset()
{
    current_strips = 0u;
    previous_strips = 0u;
    visible = 0u;
}

void NGSpriteWindow::setCurrent(uint8_t count)
{
    previous_strips = current_strips;
    current_strips = ngsw_clamp_count(count, max_strips);
    visible = 1u;
}

void NGSpriteWindow::clear()
{
    ng_vram_clear_sprite_range(first_slot, max_strips);
    previous_strips = 0u;
    current_strips = 0u;
    visible = 0u;
}

void NGSpriteWindow::clearTail()
{
    if (previous_strips == 0u) {
        ng_vram_clear_sprite_range(first_slot, max_strips);
        return;
    }

    if (previous_strips > current_strips) {
        ng_vram_clear_sprite_range(
            (uint16_t)(first_slot + current_strips),
            (uint16_t)(previous_strips - current_strips));
    }
}

void NGSpriteWindow::hide()
{
    ng_vram_clear_sprite_range(first_slot, max_strips);
    visible = 0u;
    previous_strips = current_strips;
    current_strips = 0u;
}

void NEOGEO_USER ng_sprite_window_init(NGSpriteWindow *window,
                                       uint8_t owner,
                                       uint16_t first_slot,
                                       uint8_t max_strips)
{
    if (window) window->init(owner, first_slot, max_strips);
}

void NEOGEO_USER ng_sprite_window_reset(NGSpriteWindow *window)
{
    if (window) window->reset();
}

void NEOGEO_USER ng_sprite_window_set_current(NGSpriteWindow *window,
                                              uint8_t current_strips)
{
    if (window) window->setCurrent(current_strips);
}

void NEOGEO_USER ng_sprite_window_clear(NGSpriteWindow *window)
{
    if (window) window->clear();
}

void NEOGEO_USER ng_sprite_window_clear_tail(NGSpriteWindow *window)
{
    if (window) window->clearTail();
}

void NEOGEO_USER ng_sprite_window_hide(NGSpriteWindow *window)
{
    if (window) window->hide();
}
