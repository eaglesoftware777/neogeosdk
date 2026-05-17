#include "ng_palette_fx.hpp"
#include "ng_hw.hpp"

#define PAL_R(c)    (uint8_t)(((c) >> 10) & 0x1F)
#define PAL_G(c)    (uint8_t)(((c) >>  5) & 0x1F)
#define PAL_B(c)    (uint8_t)((c) & 0x1F)
#define PAL_PACK(r,g,b) (uint16_t)(((uint16_t)(r) << 10) | ((uint16_t)(g) << 5) | (uint16_t)(b))

/* --- PaletteFxSystem singleton --- */

PaletteFxSystem& PaletteFxSystem::instance()
{
    static PaletteFxSystem pfs;
    return pfs;
}

/* --- PaletteFxSystem private helpers --- */

uint8_t PaletteFxSystem::scaleChannel(uint8_t v, uint8_t factor)
{
    return (uint8_t)(((uint16_t)v * (uint16_t)factor) >> 8);
}

void PaletteFxSystem::blendTo(uint16_t *out, const uint16_t *base,
                               uint8_t tr, uint8_t tg, uint8_t tb, uint8_t blend)
{
    uint8_t i, inv = (uint8_t)(255 - blend);
    out[0] = 0x8000;
    for (i = 1; i < 16; i++) {
        uint16_t src = base[i];
        uint8_t r, g, b;
        uint16_t rb, gb, bb;
        if (src == 0x8000) { out[i] = 0x8000; continue; }
        rb = (uint16_t)(scaleChannel(PAL_R(src), inv) + scaleChannel((uint8_t)(tr >> 3), blend));
        gb = (uint16_t)(scaleChannel(PAL_G(src), inv) + scaleChannel((uint8_t)(tg >> 3), blend));
        bb = (uint16_t)(scaleChannel(PAL_B(src), inv) + scaleChannel((uint8_t)(tb >> 3), blend));
        r = (uint8_t)(rb >> 1); if (r > 31) r = 31;
        g = (uint8_t)(gb >> 1); if (g > 31) g = 31;
        b = (uint8_t)(bb >> 1); if (b > 31) b = 31;
        out[i] = PAL_PACK(r, g, b);
    }
}

void PaletteFxSystem::scalePal(uint16_t *out, const uint16_t *base, uint8_t brightness)
{
    uint8_t i;
    out[0] = 0x8000;
    for (i = 1; i < 16; i++) {
        uint16_t src = base[i];
        uint8_t r, g, b;
        if (src == 0x8000) { out[i] = 0x8000; continue; }
        r = scaleChannel(PAL_R(src), brightness);
        g = scaleChannel(PAL_G(src), brightness);
        b = scaleChannel(PAL_B(src), brightness);
        out[i] = PAL_PACK(r, g, b);
    }
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
    NGPalFxSlot *s = findOrAlloc(palette_slot);
    if (!s) return;
    s->fx_type = NG_PALFX_FADE_IN; s->base_pal = base_pal;
    s->timer = 0; s->duration = duration ? duration : 1;
}

void PaletteFxSystem::fadeOut(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = findOrAlloc(palette_slot);
    if (!s) return;
    s->fx_type = NG_PALFX_FADE_OUT; s->base_pal = base_pal;
    s->timer = 0; s->duration = duration ? duration : 1;
}

void PaletteFxSystem::flashWhite(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = findOrAlloc(palette_slot);
    if (!s) return;
    s->fx_type = NG_PALFX_FLASH_WHITE; s->base_pal = base_pal;
    s->timer = duration ? duration : 4; s->duration = s->timer;
}

void PaletteFxSystem::flashRed(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = findOrAlloc(palette_slot);
    if (!s) return;
    s->fx_type = NG_PALFX_FLASH_RED; s->base_pal = base_pal;
    s->timer = duration ? duration : 6; s->duration = s->timer;
}

void PaletteFxSystem::flashBlue(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = findOrAlloc(palette_slot);
    if (!s) return;
    s->fx_type = NG_PALFX_FLASH_BLUE; s->base_pal = base_pal;
    s->timer = duration ? duration : 6; s->duration = s->timer;
}

void PaletteFxSystem::pulse(uint8_t palette_slot, const uint16_t *base_pal, uint8_t period)
{
    NGPalFxSlot *s = findOrAlloc(palette_slot);
    if (!s) return;
    s->fx_type = NG_PALFX_PULSE; s->base_pal = base_pal;
    s->timer = 0; s->duration = period ? period : 16;
}

void PaletteFxSystem::cycle(uint8_t palette_slot, const uint16_t *base_pal,
                             uint8_t start, uint8_t end)
{
    NGPalFxSlot *s = findOrAlloc(palette_slot);
    if (!s) return;
    if (start >= 16) start = 1;
    if (end >= 16)   end   = 15;
    if (end <= start) end  = (uint8_t)(start + 1);
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
            if (brightness > 255) brightness = 255;
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
            blendTo(s->work_pal, s->base_pal, 255, 255, 255, (uint8_t)(255 - blend));
            ng_rq_palette_upload(s->palette_slot, s->work_pal);
            if (s->timer == 0) { ng_rq_palette_upload(s->palette_slot, s->base_pal); s->active = 0; }
            else s->timer--;
            break;
        }
        case NG_PALFX_FLASH_RED: {
            uint8_t blend = (uint8_t)(((uint16_t)s->timer * 200u) / (uint16_t)s->duration);
            blendTo(s->work_pal, s->base_pal, 255, 0, 0, (uint8_t)(200 - blend));
            ng_rq_palette_upload(s->palette_slot, s->work_pal);
            if (s->timer == 0) { ng_rq_palette_upload(s->palette_slot, s->base_pal); s->active = 0; }
            else s->timer--;
            break;
        }
        case NG_PALFX_FLASH_BLUE: {
            uint8_t blend = (uint8_t)(((uint16_t)s->timer * 180u) / (uint16_t)s->duration);
            blendTo(s->work_pal, s->base_pal, 0, 64, 255, (uint8_t)(180 - blend));
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
                phase = (uint8_t)(127u - (uint16_t)(s->timer - half) * 127u / (uint16_t)half);
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
