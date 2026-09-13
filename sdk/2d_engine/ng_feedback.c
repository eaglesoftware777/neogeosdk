/*
 * ng_feedback.c — Hitstop, timing, and feedback helpers (Stage 9)
 */

#include "ng_feedback.h"
#include "ng_palette_fx.h"

static uint8_t           ng_fb_hitstop;
static uint8_t           ng_fb_slow_motion;
static NGFeedbackSfxHook ng_fb_sfx_hook;

void NEOGEO_USER ng_feedback_init(void)
{
    ng_fb_hitstop    = 0;
    ng_fb_slow_motion = 0;
    ng_fb_sfx_hook   = 0;
}

void NEOGEO_USER ng_feedback_set_sfx_hook(NGFeedbackSfxHook hook)
{
    ng_fb_sfx_hook = hook;
}

void NEOGEO_USER ng_feedback_update(void)
{
    if (ng_fb_hitstop > 0)     ng_fb_hitstop--;
    if (ng_fb_slow_motion > 0) ng_fb_slow_motion--;

    /* Palette FX is updated separately in ng_palette_fx_update() */
}

void NEOGEO_USER ng_feedback_hitstop(uint8_t frames)
{
    if (frames > ng_fb_hitstop) ng_fb_hitstop = frames;
}

uint8_t NEOGEO_USER ng_feedback_is_hitstop(void)
{
    return ng_fb_hitstop > 0 ? 1 : 0;
}

void NEOGEO_USER ng_feedback_shake(NGCamera *cam, uint8_t amp, uint8_t frames)
{
    if (cam) ng_camera_shake(cam, amp, frames);
}

void NEOGEO_USER ng_feedback_flash_white(uint8_t palette_slot, const uint16_t *base_pal, uint8_t frames)
{
    ng_palfx_flash_white(palette_slot, base_pal, frames);
}

void NEOGEO_USER ng_feedback_flash_red(uint8_t palette_slot, const uint16_t *base_pal, uint8_t frames)
{
    ng_palfx_flash_red(palette_slot, base_pal, frames);
}

void NEOGEO_USER ng_feedback_slow_motion(uint8_t frames)
{
    if (frames > ng_fb_slow_motion) ng_fb_slow_motion = frames;
}

uint8_t NEOGEO_USER ng_feedback_is_slow_motion(void)
{
    return ng_fb_slow_motion > 0 ? 1 : 0;
}

void NEOGEO_USER ng_impact_event(uint8_t impact_kind,
                                  uint8_t palette_slot,
                                  const uint16_t *base_pal,
                                  NGCamera *cam,
                                  uint16_t sfx_id,
                                  int16_t spark_x, int16_t spark_y,
                                  uint16_t spark_tile, uint8_t spark_pal)
{
    uint8_t hitstop = 0;
    uint8_t shake_amp = 0;
    uint8_t shake_frames = 0;
    uint8_t flash_frames = 0;

    /*
     * Impact presets — tuned for arcade feel on 60Hz hardware.
     * Heavy impacts use both white and red flashes: white first (peak),
     * red lingers (damage read).
     */
    switch (impact_kind) {
    case NG_IMPACT_LIGHT:
        hitstop      = 3;
        shake_amp    = 1;
        shake_frames = 4;
        flash_frames = 4;
        if (base_pal) ng_palfx_flash_white(palette_slot, base_pal, flash_frames);
        break;

    case NG_IMPACT_MEDIUM:
        hitstop      = 5;
        shake_amp    = 2;
        shake_frames = 6;
        flash_frames = 6;
        if (base_pal) ng_palfx_flash_red(palette_slot, base_pal, flash_frames);
        break;

    case NG_IMPACT_HEAVY:
        hitstop      = 8;
        shake_amp    = 3;
        shake_frames = 8;
        flash_frames = 8;
        /* White flash only: both effects share the same palette slot so a
         * subsequent flash_red call would immediately overwrite the white. */
        if (base_pal) ng_palfx_flash_white(palette_slot, base_pal, flash_frames);
        break;

    case NG_IMPACT_BOSS:
        hitstop      = 12;
        shake_amp    = 4;
        shake_frames = 12;
        flash_frames = 12;
        if (base_pal) ng_palfx_flash_white(palette_slot, base_pal, flash_frames);
        break;

    default:
        hitstop      = 3;
        shake_amp    = 1;
        shake_frames = 4;
        break;
    }

    ng_feedback_hitstop(hitstop);

    if (cam && shake_amp > 0) {
        ng_camera_shake(cam, shake_amp, shake_frames);
    }

    /* Sound */
    if (sfx_id > 0 && ng_fb_sfx_hook) {
        ng_fb_sfx_hook(sfx_id);
    }

    /* Particles */
    if (spark_tile > 0) {
        ng_spawn_hit_spark(spark_x, spark_y, spark_tile, spark_pal);
    }
}

uint8_t NEOGEO_USER ng_feedback_hitstop_remaining(void)
{
    return ng_fb_hitstop;
}

uint8_t NEOGEO_USER ng_feedback_slow_motion_remaining(void)
{
    return ng_fb_slow_motion;
}
