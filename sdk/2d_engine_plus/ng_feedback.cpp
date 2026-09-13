#include "ng_feedback.hpp"
#include "ng_palette_fx.hpp"

/* --- FeedbackSystem singleton --- */

FeedbackSystem& FeedbackSystem::instance()
{
    static FeedbackSystem fs;
    return fs;
}

/* --- FeedbackSystem public methods --- */

void FeedbackSystem::init()
{
    fb_hitstop    = 0;
    fb_slow_motion = 0;
    fb_sfx_hook   = 0;
}

void FeedbackSystem::setSfxHook(NGFeedbackSfxHook hook)
{
    fb_sfx_hook = hook;
}

void FeedbackSystem::update()
{
    if (fb_hitstop    > 0) fb_hitstop--;
    if (fb_slow_motion > 0) fb_slow_motion--;
}

void FeedbackSystem::hitstop(uint8_t frames)
{
    if (frames > fb_hitstop) fb_hitstop = frames;
}

uint8_t FeedbackSystem::isHitstop() const  { return fb_hitstop > 0 ? 1 : 0; }
uint8_t FeedbackSystem::hitstopRemaining() const { return fb_hitstop; }

void FeedbackSystem::shake(NGCamera *cam, uint8_t amp, uint8_t frames)
{
    if (cam) cam->shake(amp, frames);
}

void FeedbackSystem::flashWhite(uint8_t palette_slot, const uint16_t *base_pal, uint8_t frames)
{
    ng_palfx_flash_white(palette_slot, base_pal, frames);
}

void FeedbackSystem::flashRed(uint8_t palette_slot, const uint16_t *base_pal, uint8_t frames)
{
    ng_palfx_flash_red(palette_slot, base_pal, frames);
}

void FeedbackSystem::slowMotion(uint8_t frames)
{
    if (frames > fb_slow_motion) fb_slow_motion = frames;
}

uint8_t FeedbackSystem::isSlowMotion() const  { return fb_slow_motion > 0 ? 1 : 0; }
uint8_t FeedbackSystem::slowMotionRemaining() const { return fb_slow_motion; }

void FeedbackSystem::impactEvent(uint8_t impact_kind,
                                  uint8_t palette_slot, const uint16_t *base_pal,
                                  NGCamera *cam, uint16_t sfx_id,
                                  int16_t spark_x, int16_t spark_y,
                                  uint16_t spark_tile, uint8_t spark_pal)
{
    uint8_t hs = 0, shake_amp = 0, shake_frames = 0, flash_f = 0;

    switch (impact_kind) {
    case NG_IMPACT_LIGHT:
        hs = 3; shake_amp = 1; shake_frames = 4; flash_f = 4;
        if (base_pal) flashWhite(palette_slot, base_pal, flash_f);
        break;
    case NG_IMPACT_MEDIUM:
        hs = 5; shake_amp = 2; shake_frames = 6; flash_f = 6;
        if (base_pal) flashRed(palette_slot, base_pal, flash_f);
        break;
    case NG_IMPACT_HEAVY:
        hs = 8; shake_amp = 3; shake_frames = 8; flash_f = 8;
        if (base_pal) flashWhite(palette_slot, base_pal, flash_f);
        break;
    case NG_IMPACT_BOSS:
        hs = 12; shake_amp = 4; shake_frames = 12; flash_f = 12;
        if (base_pal) flashWhite(palette_slot, base_pal, flash_f);
        break;
    default:
        hs = 3; shake_amp = 1; shake_frames = 4;
        break;
    }

    hitstop(hs);
    if (cam && shake_amp > 0) cam->shake(shake_amp, shake_frames);
    if (sfx_id > 0 && fb_sfx_hook) fb_sfx_hook(sfx_id);
    if (spark_tile > 0) ng_spawn_hit_spark(spark_x, spark_y, spark_tile, spark_pal);
}

/* --- extern "C" wrappers --- */

extern "C" {

void NEOGEO_USER ng_feedback_init(void)                 { FeedbackSystem::instance().init(); }
void NEOGEO_USER ng_feedback_set_sfx_hook(NGFeedbackSfxHook hook) { FeedbackSystem::instance().setSfxHook(hook); }
void NEOGEO_USER ng_feedback_update(void)               { FeedbackSystem::instance().update(); }
void NEOGEO_USER ng_feedback_hitstop(uint8_t frames)    { FeedbackSystem::instance().hitstop(frames); }
uint8_t NEOGEO_USER ng_feedback_is_hitstop(void)        { return FeedbackSystem::instance().isHitstop(); }
void NEOGEO_USER ng_feedback_shake(NGCamera *cam, uint8_t amp, uint8_t frames) { FeedbackSystem::instance().shake(cam, amp, frames); }
void NEOGEO_USER ng_feedback_flash_white(uint8_t ps, const uint16_t *b, uint8_t f) { FeedbackSystem::instance().flashWhite(ps, b, f); }
void NEOGEO_USER ng_feedback_flash_red(uint8_t ps, const uint16_t *b, uint8_t f)   { FeedbackSystem::instance().flashRed(ps, b, f); }
void NEOGEO_USER ng_feedback_slow_motion(uint8_t frames){ FeedbackSystem::instance().slowMotion(frames); }
uint8_t NEOGEO_USER ng_feedback_is_slow_motion(void)    { return FeedbackSystem::instance().isSlowMotion(); }
uint8_t NEOGEO_USER ng_feedback_hitstop_remaining(void) { return FeedbackSystem::instance().hitstopRemaining(); }
uint8_t NEOGEO_USER ng_feedback_slow_motion_remaining(void) { return FeedbackSystem::instance().slowMotionRemaining(); }

void NEOGEO_USER ng_impact_event(uint8_t impact_kind,
                                  uint8_t palette_slot, const uint16_t *base_pal,
                                  NGCamera *cam, uint16_t sfx_id,
                                  int16_t spark_x, int16_t spark_y,
                                  uint16_t spark_tile, uint8_t spark_pal)
{
    FeedbackSystem::instance().impactEvent(impact_kind, palette_slot, base_pal,
                                           cam, sfx_id, spark_x, spark_y,
                                           spark_tile, spark_pal);
}

} /* extern "C" */
