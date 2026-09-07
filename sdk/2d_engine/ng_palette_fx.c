/* Palette effects use the Neo Geo split-component color encoding.
 * Index zero is preserved; uploads are deferred to the render queue. */

#include "ng_palette_fx.h"
#include "macro.h"
#include "neogeo.h"

static NGPalFxSlot ng_palfx_slots[NG_PALFX_MAX_SLOTS];

#include "ng_palette_math.h"

static void NEOGEO_USER palfx_blend_to(uint16_t *out, const uint16_t *base,
                                      uint8_t r, uint8_t g, uint8_t b, uint8_t blend)
{
    ng_palette_tint_colors(out, base, r, g, b, blend);
}

static void NEOGEO_USER palfx_scale(uint16_t *out, const uint16_t *base, uint8_t brightness)
{
    ng_palette_scale_colors(out, base, brightness);
}

static NGPalFxSlot * NEOGEO_USER palfx_find_or_alloc(uint8_t palette_slot)
{
    uint8_t i;
    uint8_t free_i = 0xff;

    /* First check if the slot is already being managed */
    for (i = 0; i < NG_PALFX_MAX_SLOTS; i++) {
        if (ng_palfx_slots[i].active && ng_palfx_slots[i].palette_slot == palette_slot)
            return &ng_palfx_slots[i];
        if (!ng_palfx_slots[i].active && free_i == 0xff)
            free_i = i;
    }

    if (free_i == 0xff) return 0; /* pool full */

    ng_palfx_slots[free_i].active       = 1;
    ng_palfx_slots[free_i].palette_slot = palette_slot;
    return &ng_palfx_slots[free_i];
}

void NEOGEO_USER ng_palette_fx_init(void)
{
    uint8_t i;

    for (i = 0; i < NG_PALFX_MAX_SLOTS; i++) {
        ng_palfx_slots[i].active = 0;
    }
}

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

void NEOGEO_USER ng_palfx_upload_base(uint8_t palette_slot, const uint16_t *pal)
{
    if (!pal) return;
    ng_rq_palette_upload(palette_slot, pal);
}

void NEOGEO_USER ng_palfx_fade_in(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = base_pal ? palfx_find_or_alloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type   = NG_PALFX_FADE_IN;
    s->base_pal  = base_pal;
    s->timer     = 0;
    s->duration  = duration ? duration : 1;
}

void NEOGEO_USER ng_palfx_fade_out(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = base_pal ? palfx_find_or_alloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type   = NG_PALFX_FADE_OUT;
    s->base_pal  = base_pal;
    s->timer     = 0;
    s->duration  = duration ? duration : 1;
}

void NEOGEO_USER ng_palfx_flash_white(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = base_pal ? palfx_find_or_alloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type   = NG_PALFX_FLASH_WHITE;
    s->base_pal  = base_pal;
    s->timer     = duration ? duration : 4;
    s->duration  = s->timer;
}

void NEOGEO_USER ng_palfx_flash_red(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = base_pal ? palfx_find_or_alloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type   = NG_PALFX_FLASH_RED;
    s->base_pal  = base_pal;
    s->timer     = duration ? duration : 6;
    s->duration  = s->timer;
}

void NEOGEO_USER ng_palfx_flash_blue(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration)
{
    NGPalFxSlot *s = base_pal ? palfx_find_or_alloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type   = NG_PALFX_FLASH_BLUE;
    s->base_pal  = base_pal;
    s->timer     = duration ? duration : 6;
    s->duration  = s->timer;
}

void NEOGEO_USER ng_palfx_pulse(uint8_t palette_slot, const uint16_t *base_pal, uint8_t period)
{
    NGPalFxSlot *s = base_pal ? palfx_find_or_alloc(palette_slot) : 0;
    if (!s) return;
    s->fx_type   = NG_PALFX_PULSE;
    s->base_pal  = base_pal;
    s->timer     = 0;
    s->duration  = period ? (period < 2u ? 2u : period) : 16;
}

void NEOGEO_USER ng_palfx_cycle(uint8_t palette_slot, const uint16_t *base_pal,
                                  uint8_t start, uint8_t end)
{
    NGPalFxSlot *s = base_pal ? palfx_find_or_alloc(palette_slot) : 0;
    if (!s) return;
    if (start == 0u || start > 15u) start = 1u;
    if (end >= 16) end = 15;
    if (end < start) end = start;
    s->fx_type    = NG_PALFX_CYCLE;
    s->base_pal   = base_pal;
    s->cycle_start = start;
    s->cycle_end   = end;
    s->cycle_pos   = 0;
    s->timer       = 0;
    s->duration    = 1;
}

void NEOGEO_USER ng_palfx_stop(uint8_t palette_slot)
{
    uint8_t i;

    for (i = 0; i < NG_PALFX_MAX_SLOTS; i++) {
        if (ng_palfx_slots[i].active && ng_palfx_slots[i].palette_slot == palette_slot) {
            /* Restore base palette */
            if (ng_palfx_slots[i].base_pal)
                ng_rq_palette_upload(palette_slot, ng_palfx_slots[i].base_pal);
            ng_palfx_slots[i].active = 0;
            return;
        }
    }
}

uint8_t NEOGEO_USER ng_palfx_active(uint8_t palette_slot)
{
    uint8_t i;
    for (i = 0; i < NG_PALFX_MAX_SLOTS; i++) {
        if (ng_palfx_slots[i].active && ng_palfx_slots[i].palette_slot == palette_slot)
            return 1;
    }
    return 0;
}

void NEOGEO_USER ng_palette_fx_update(void)
{
    uint8_t i;

    for (i = 0; i < NG_PALFX_MAX_SLOTS; i++) {
        NGPalFxSlot *s = &ng_palfx_slots[i];

        if (!s->active || !s->base_pal) continue;

        switch (s->fx_type) {

        case NG_PALFX_FADE_IN: {
            /*
             * Fade in: brightness goes from 0 to 255 over `duration` frames.
             * brightness = (timer * 255) / duration — use shift approximation.
             */
            uint16_t bright_n;
            uint8_t  brightness;

            s->timer++;
            bright_n   = (uint16_t)((uint16_t)s->timer * 255u);
            brightness = (uint8_t)(bright_n / (uint16_t)s->duration);

            palfx_scale(s->work_pal, s->base_pal, brightness);
            ng_rq_palette_upload(s->palette_slot, s->work_pal);

            if (s->timer >= s->duration) {
                ng_rq_palette_upload(s->palette_slot, s->base_pal);
                s->active = 0;
            }
            break;
        }

        case NG_PALFX_FADE_OUT: {
            uint16_t bright_n;
            uint8_t  brightness;

            s->timer++;
            bright_n   = (uint16_t)((uint16_t)(s->duration - s->timer) * 255u);
            if (s->timer >= s->duration) {
                brightness = 0;
            } else {
                brightness = (uint8_t)(bright_n / (uint16_t)s->duration);
            }

            palfx_scale(s->work_pal, s->base_pal, brightness);
            ng_rq_palette_upload(s->palette_slot, s->work_pal);

            if (s->timer >= s->duration) {
                s->active = 0;
            }
            break;
        }

        case NG_PALFX_FLASH_WHITE: {
            /* Blend from base towards white; intensity decays over duration */
            uint8_t blend;

            blend = (uint8_t)(((uint16_t)s->timer * 255u) / (uint16_t)s->duration);
            /* White: R=31, G=31, B=31 → pass as scaled 0..255: 255 */
            palfx_blend_to(s->work_pal, s->base_pal, 255, 255, 255, blend);
            ng_rq_palette_upload(s->palette_slot, s->work_pal);

            if (s->timer == 0) {
                ng_rq_palette_upload(s->palette_slot, s->base_pal);
                s->active = 0;
            } else {
                s->timer--;
            }
            break;
        }

        case NG_PALFX_FLASH_RED: {
            uint8_t blend;

            blend = (uint8_t)(((uint16_t)s->timer * 200u) / (uint16_t)s->duration);
            palfx_blend_to(s->work_pal, s->base_pal, 255, 0, 0, blend);
            ng_rq_palette_upload(s->palette_slot, s->work_pal);

            if (s->timer == 0) {
                ng_rq_palette_upload(s->palette_slot, s->base_pal);
                s->active = 0;
            } else {
                s->timer--;
            }
            break;
        }

        case NG_PALFX_FLASH_BLUE: {
            uint8_t blend;

            blend = (uint8_t)(((uint16_t)s->timer * 180u) / (uint16_t)s->duration);
            palfx_blend_to(s->work_pal, s->base_pal, 0, 64, 255, blend);
            ng_rq_palette_upload(s->palette_slot, s->work_pal);

            if (s->timer == 0) {
                ng_rq_palette_upload(s->palette_slot, s->base_pal);
                s->active = 0;
            } else {
                s->timer--;
            }
            break;
        }

        case NG_PALFX_PULSE: {
            /*
             * Brightness pulse: sine-wave brightness using triangle approximation.
             * sin_tab index: (timer * 64) / duration maps 0..duration → 0..64 (quarter period).
             * Amplitude oscillates 128..255.
             */
            uint8_t phase;
            uint8_t brightness;
            uint8_t half = (uint8_t)(s->duration >> 1);

            s->timer++;
            if (s->timer >= s->duration) s->timer = 0;

            /* Triangle wave: ramp up then ramp down */
            if (s->timer < half) {
                phase = (uint8_t)((uint16_t)s->timer * 127u / (uint16_t)half);
            } else {
                phase = (uint8_t)(127u - (uint16_t)(s->timer - half) * 127u / (uint16_t)(s->duration - half));
            }
            brightness = (uint8_t)(128 + phase);

            palfx_scale(s->work_pal, s->base_pal, brightness);
            ng_rq_palette_upload(s->palette_slot, s->work_pal);
            /* Pulse is perpetual — slot stays active */
            break;
        }

        case NG_PALFX_CYCLE: {
            /*
             * Colour cycle: copy base palette, then rotate entries [start..end] by cycle_pos.
             * One entry rotated per frame for smooth cycling without bandwidth spike.
             */
            uint8_t j;
            uint8_t len = (uint8_t)(s->cycle_end - s->cycle_start + 1);

            for (j = 0; j < 16; j++) s->work_pal[j] = s->base_pal[j];

            /* Rotate: work_pal[start + k] = base_pal[start + (k + pos) % len] */
            for (j = 0; j < len; j++) {
                uint8_t src = (uint8_t)(s->cycle_start + ((j + s->cycle_pos) % len));
                s->work_pal[s->cycle_start + j] = s->base_pal[src];
            }

            ng_rq_palette_upload(s->palette_slot, s->work_pal);

            s->cycle_pos++;
            if (s->cycle_pos >= len) s->cycle_pos = 0;
            break;
        }

        default:
            s->active = 0;
            break;
        }
    }
}
