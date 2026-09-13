#include "ng_palette_fx.hpp"
#include "ng_hw.hpp"

#include "../2d_engine/ng_palette_math.h"

/* --- PaletteFxSystem singleton --- */

PaletteFxSystem& PaletteFxSystem::instance()
{
    static PaletteFxSystem pfs;
    return pfs;
}

/* --- PaletteFxSystem private helpers --- */

void PaletteFxSystem::blendTo(uint16_t *out, const uint16_t *base,
                               uint8_t r, uint8_t g, uint8_t b, uint8_t blend)
{
    ng_palette_tint_colors(out, base, r, g, b, blend);
}

void PaletteFxSystem::scalePal(uint16_t *out, const uint16_t *base, uint8_t brightness)
{
    ng_palette_scale_colors(out, base, brightness);
}

NGPalFxSlot* PaletteFxSystem::findOrAlloc(uint8_t palette_slot)
{
    uint8_t i, free_i = 0xff;
    for (i = 0; i < NG_PALFX_MAX_SLOTS; i++) {
        if (slots[i].active && slots[i].palette_slot == palette_slot) return &slots[i];
        if (!slots[i].active && free_i == 0xff) free_i = i;
    }
    if (free_i == 0xff) return 0;
    slots[free_i].active       = 1;
    slots[free_i].palette_slot = palette_slot;
    return &slots[free_i];
}

/* --- PaletteFxSystem public methods --- */

void PaletteFxSystem::init()
{
    uint8_t i;
    for (i = 0; i < NG_PALFX_MAX_SLOTS; i++) slots[i].active = 0;
}

void PaletteFxSystem::uploadBase(uint8_t palette_slot, const uint16_t *pal)
{
    if (pal) ng_rq_palette_upload(palette_slot, pal);
}

void PaletteFxSystem::fadeIn(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = base_pal ? findOrAlloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type = NG_PALFX_FADE_IN; s->base_pal = base_pal;
    s->timer = 0; s->duration = duration ? duration : 1;
}

void PaletteFxSystem::fadeOut(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = base_pal ? findOrAlloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type = NG_PALFX_FADE_OUT; s->base_pal = base_pal;
    s->timer = 0; s->duration = duration ? duration : 1;
}

void PaletteFxSystem::flashWhite(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = base_pal ? findOrAlloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type = NG_PALFX_FLASH_WHITE; s->base_pal = base_pal;
    s->timer = duration ? duration : 4; s->duration = s->timer;
}

void PaletteFxSystem::flashRed(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = base_pal ? findOrAlloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type = NG_PALFX_FLASH_RED; s->base_pal = base_pal;
    s->timer = duration ? duration : 6; s->duration = s->timer;
}

void PaletteFxSystem::flashBlue(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = base_pal ? findOrAlloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type = NG_PALFX_FLASH_BLUE; s->base_pal = base_pal;
    s->timer = duration ? duration : 6; s->duration = s->timer;
}

void PaletteFxSystem::pulse(uint8_t palette_slot, const uint16_t *base_pal, uint8_t period)
{
    NGPalFxSlot *s = base_pal ? findOrAlloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type = NG_PALFX_PULSE; s->base_pal = base_pal;
    s->timer = 0; s->duration = period ? (period < 2u ? 2u : period) : 16;
}

void PaletteFxSystem::cycle(uint8_t palette_slot, const uint16_t *base_pal,
                             uint8_t start, uint8_t end)
{
    NGPalFxSlot *s = base_pal ? findOrAlloc(palette_slot) : 0;
    if (!s) return;
    if (start == 0u || start > 15u) start = 1u;
    if (end >= 16)   end   = 15;
    if (end < start) end = start;
    s->fx_type    = NG_PALFX_CYCLE; s->base_pal = base_pal;
    s->cycle_start = start; s->cycle_end = end; s->cycle_pos = 0;
    s->timer = 0; s->duration = 1;
}

void PaletteFxSystem::stop(uint8_t palette_slot)
{
    uint8_t i;
    for (i = 0; i < NG_PALFX_MAX_SLOTS; i++) {
        if (slots[i].active && slots[i].palette_slot == palette_slot) {
            if (slots[i].base_pal) ng_rq_palette_upload(palette_slot, slots[i].base_pal);
            slots[i].active = 0;
            return;
        }
    }
}

uint8_t PaletteFxSystem::active(uint8_t palette_slot) const
{
    uint8_t i;
    for (i = 0; i < NG_PALFX_MAX_SLOTS; i++)
        if (slots[i].active && slots[i].palette_slot == palette_slot) return 1;
    return 0;
}

void PaletteFxSystem::update()
{
    uint8_t i;
    for (i = 0; i < NG_PALFX_MAX_SLOTS; i++) {
        NGPalFxSlot *s = &slots[i];
        if (!s->active || !s->base_pal) continue;

        switch (s->fx_type) {
        case NG_PALFX_FADE_IN: {
            uint8_t brightness;
            s->timer++;
            brightness = (uint8_t)((uint16_t)((uint16_t)s->timer * 255u) / (uint16_t)s->duration);
            scalePal(s->work_pal, s->base_pal, brightness);
            ng_rq_palette_upload(s->palette_slot, s->work_pal);
            if (s->timer >= s->duration) { ng_rq_palette_upload(s->palette_slot, s->base_pal); s->active = 0; }
            break;
        }
        case NG_PALFX_FADE_OUT: {
            uint8_t brightness;
            s->timer++;
            brightness = (s->timer >= s->duration) ? 0 :
                         (uint8_t)((uint16_t)((uint16_t)(s->duration - s->timer) * 255u) / (uint16_t)s->duration);
            scalePal(s->work_pal, s->base_pal, brightness);
            ng_rq_palette_upload(s->palette_slot, s->work_pal);
            if (s->timer >= s->duration) s->active = 0;
            break;
        }
        case NG_PALFX_FLASH_WHITE: {
            uint8_t blend = (uint8_t)(((uint16_t)s->timer * 255u) / (uint16_t)s->duration);
            blendTo(s->work_pal, s->base_pal, 255, 255, 255, blend);
            ng_rq_palette_upload(s->palette_slot, s->work_pal);
            if (s->timer == 0) { ng_rq_palette_upload(s->palette_slot, s->base_pal); s->active = 0; }
            else s->timer--;
            break;
        }
        case NG_PALFX_FLASH_RED: {
            uint8_t blend = (uint8_t)(((uint16_t)s->timer * 200u) / (uint16_t)s->duration);
            blendTo(s->work_pal, s->base_pal, 255, 0, 0, blend);
            ng_rq_palette_upload(s->palette_slot, s->work_pal);
            if (s->timer == 0) { ng_rq_palette_upload(s->palette_slot, s->base_pal); s->active = 0; }
            else s->timer--;
            break;
        }
        case NG_PALFX_FLASH_BLUE: {
            uint8_t blend = (uint8_t)(((uint16_t)s->timer * 180u) / (uint16_t)s->duration);
            blendTo(s->work_pal, s->base_pal, 0, 64, 255, blend);
            ng_rq_palette_upload(s->palette_slot, s->work_pal);
            if (s->timer == 0) { ng_rq_palette_upload(s->palette_slot, s->base_pal); s->active = 0; }
            else s->timer--;
            break;
        }
        case NG_PALFX_PULSE: {
            uint8_t half = (uint8_t)(s->duration >> 1);
            uint8_t phase, brightness;
            s->timer++;
            if (s->timer >= s->duration) s->timer = 0;
            if (s->timer < half)
                phase = (uint8_t)((uint16_t)s->timer * 127u / (uint16_t)half);
            else
                phase = (uint8_t)(127u - (uint16_t)(s->timer - half) * 127u / (uint16_t)(s->duration - half));
            brightness = (uint8_t)(128 + phase);
            scalePal(s->work_pal, s->base_pal, brightness);
            ng_rq_palette_upload(s->palette_slot, s->work_pal);
            break;
        }
        case NG_PALFX_CYCLE: {
            uint8_t j, len = (uint8_t)(s->cycle_end - s->cycle_start + 1);
            for (j = 0; j < 16; j++) s->work_pal[j] = s->base_pal[j];
            for (j = 0; j < len; j++) {
                uint8_t src = (uint8_t)(s->cycle_start + ((j + s->cycle_pos) % len));
                s->work_pal[s->cycle_start + j] = s->base_pal[src];
            }
            ng_rq_palette_upload(s->palette_slot, s->work_pal);
            if (++s->cycle_pos >= len) s->cycle_pos = 0;
            break;
        }
        default:
            s->active = 0;
            break;
        }
    }
}

/* --- extern "C" wrappers --- */

extern "C" {

void NEOGEO_USER ng_palette_load_bank(uint8_t palette_slot, const uint16_t *pal)
{
    if (!pal) return;
    load_palettes((uint16_t *)pal,
                  (uintptr_t)((uint32_t)PALETTES +
                              ((uint32_t)PALOFFSET * (uint32_t)palette_slot)));
}

uint8_t NEOGEO_USER ng_palette_load_asset(const NGPaletteAsset *assets,
                                          uint16_t count,
                                          uint16_t asset_id)
{
    uint16_t i;

    if (!assets) return 0u;

    if (asset_id != 0u && asset_id <= count &&
        assets[asset_id - 1u].asset_id == asset_id) {
        ng_palette_load_bank(assets[asset_id - 1u].palette_slot,
                              assets[asset_id - 1u].colors);
        return 1u;
    }

    for (i = 0u; i < count; i++) {
        if (assets[i].asset_id == asset_id) {
            ng_palette_load_bank(assets[i].palette_slot, assets[i].colors);
            return 1u;
        }
    }

    return 0u;
}

void NEOGEO_USER ng_palette_fx_init(void)           { PaletteFxSystem::instance().init(); }
void NEOGEO_USER ng_palette_fx_update(void)         { PaletteFxSystem::instance().update(); }
void NEOGEO_USER ng_palfx_upload_base(uint8_t ps, const uint16_t *pal) { PaletteFxSystem::instance().uploadBase(ps, pal); }
void NEOGEO_USER ng_palfx_fade_in(uint8_t ps, const uint16_t *b, uint8_t d)    { PaletteFxSystem::instance().fadeIn(ps, b, d); }
void NEOGEO_USER ng_palfx_fade_out(uint8_t ps, const uint16_t *b, uint8_t d)   { PaletteFxSystem::instance().fadeOut(ps, b, d); }
void NEOGEO_USER ng_palfx_flash_white(uint8_t ps, const uint16_t *b, uint8_t d){ PaletteFxSystem::instance().flashWhite(ps, b, d); }
void NEOGEO_USER ng_palfx_flash_red(uint8_t ps, const uint16_t *b, uint8_t d)  { PaletteFxSystem::instance().flashRed(ps, b, d); }
void NEOGEO_USER ng_palfx_flash_blue(uint8_t ps, const uint16_t *b, uint8_t d) { PaletteFxSystem::instance().flashBlue(ps, b, d); }
void NEOGEO_USER ng_palfx_pulse(uint8_t ps, const uint16_t *b, uint8_t p)      { PaletteFxSystem::instance().pulse(ps, b, p); }
void NEOGEO_USER ng_palfx_cycle(uint8_t ps, const uint16_t *b, uint8_t s, uint8_t e) { PaletteFxSystem::instance().cycle(ps, b, s, e); }
void NEOGEO_USER ng_palfx_stop(uint8_t ps)          { PaletteFxSystem::instance().stop(ps); }
uint8_t NEOGEO_USER ng_palfx_active(uint8_t ps)     { return PaletteFxSystem::instance().active(ps); }

} /* extern "C" */
