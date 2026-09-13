/*
 * ng_feedback.hpp — Hitstop, slow motion, screen shake, palette flash.
 *
 * All effects are coordinated via FeedbackSystem singleton.
 * ng_impact_event() is the one-call compositor for hit responses.
 */

#ifndef NG_FEEDBACK_HPP
#define NG_FEEDBACK_HPP

#include "ng_defs.hpp"
#include "ng_camera.hpp"
#include "ng_particles.hpp"

#define NG_FEEDBACK_MAX_EVENTS  4

#define NG_IMPACT_LIGHT    0
#define NG_IMPACT_MEDIUM   1
#define NG_IMPACT_HEAVY    2
#define NG_IMPACT_BOSS     3

typedef void(*NGFeedbackSfxHook)(uint16_t id);

/*
 * FeedbackSystem — singleton for all game-feel effects.
 */
class FeedbackSystem {
public:
    static FeedbackSystem& instance();

    void    NEOGEO_USER init();
    void    NEOGEO_USER setSfxHook(NGFeedbackSfxHook hook);
    void    NEOGEO_USER update();

    void    NEOGEO_USER hitstop(uint8_t frames);
    uint8_t NEOGEO_USER isHitstop() const;
    uint8_t NEOGEO_USER hitstopRemaining() const;

    void    NEOGEO_USER shake(NGCamera *cam, uint8_t amp, uint8_t frames);
    void    NEOGEO_USER flashWhite(uint8_t palette_slot, const uint16_t *base_pal, uint8_t frames);
    void    NEOGEO_USER flashRed(uint8_t palette_slot, const uint16_t *base_pal, uint8_t frames);

    void    NEOGEO_USER slowMotion(uint8_t frames);
    uint8_t NEOGEO_USER isSlowMotion() const;
    uint8_t NEOGEO_USER slowMotionRemaining() const;

    void NEOGEO_USER impactEvent(uint8_t impact_kind,
                                  uint8_t palette_slot, const uint16_t *base_pal,
                                  NGCamera *cam, uint16_t sfx_id,
                                  int16_t spark_x, int16_t spark_y,
                                  uint16_t spark_tile, uint8_t spark_pal);

private:
    FeedbackSystem() {}

    uint8_t           fb_hitstop;
    uint8_t           fb_slow_motion;
    NGFeedbackSfxHook fb_sfx_hook;
};

#ifdef __cplusplus
extern "C" {
#endif

void    NEOGEO_USER ng_feedback_init(void);
void    NEOGEO_USER ng_feedback_set_sfx_hook(NGFeedbackSfxHook hook);
void    NEOGEO_USER ng_feedback_update(void);
void    NEOGEO_USER ng_feedback_hitstop(uint8_t frames);
uint8_t NEOGEO_USER ng_feedback_is_hitstop(void);
void    NEOGEO_USER ng_feedback_shake(NGCamera *cam, uint8_t amp, uint8_t frames);
void    NEOGEO_USER ng_feedback_flash_white(uint8_t palette_slot, const uint16_t *base_pal, uint8_t frames);
void    NEOGEO_USER ng_feedback_flash_red(uint8_t palette_slot, const uint16_t *base_pal, uint8_t frames);
void    NEOGEO_USER ng_feedback_slow_motion(uint8_t frames);
uint8_t NEOGEO_USER ng_feedback_is_slow_motion(void);
void    NEOGEO_USER ng_impact_event(uint8_t impact_kind,
                                     uint8_t palette_slot, const uint16_t *base_pal,
                                     NGCamera *cam, uint16_t sfx_id,
                                     int16_t spark_x, int16_t spark_y,
                                     uint16_t spark_tile, uint8_t spark_pal);
uint8_t NEOGEO_USER ng_feedback_hitstop_remaining(void);
uint8_t NEOGEO_USER ng_feedback_slow_motion_remaining(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
