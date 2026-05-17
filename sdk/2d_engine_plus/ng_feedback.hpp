/*
 * ng_feedback.h — Hitstop, timing, and feedback helpers (Stage 9)
 *
 * Provides a coordinated set of game-feel effects:
 *   - Hitstop: pause character logic for N frames (sword clash, heavy hit)
 *   - Screen shake: forward to ng_camera_shake()
 *   - Flash: palette FX trigger on a target slot
 *   - Slow motion: half-speed update flag (optional cheap implementation)
 *   - Impact event: single call to trigger all of the above + sound + particles
 *
 * NeoGeo notes:
 *   - Hitstop is implemented by a global skip counter checked in ng_chars_update.
 *   - Screen shake amplitude > 4 pixels tends to look bad on a CRT; keep it 2-4.
 *   - Sound triggers via the registered SFX hook from ng_actions.
 */

#ifndef NG_FEEDBACK_HPP
#define NG_FEEDBACK_HPP

#include "ng_defs.hpp"
#include "ng_camera.hpp"
#include "ng_particles.hpp"

#ifdef __cplusplus
extern "C" {
#endif


/* Maximum simultaneous feedback events */
#define NG_FEEDBACK_MAX_EVENTS  4

/* Impact kind presets */
#define NG_IMPACT_LIGHT    0   /* 3 hitstop, no shake, white flash 4 frames */
#define NG_IMPACT_MEDIUM   1   /* 5 hitstop, 2px shake 6 frames, red flash */
#define NG_IMPACT_HEAVY    2   /* 8 hitstop, 3px shake 8 frames, white+red flash */
#define NG_IMPACT_BOSS     3   /* 12 hitstop, 4px shake 12 frames, screen-wide fx */

/* Sound trigger hook — matches NGActionSfxHook */
typedef void(*NGFeedbackSfxHook)(uint16_t id);

/* Initialise feedback system. */
void NEOGEO_USER ng_feedback_init(void);

/* Register the SFX trigger hook (usually playSFX wrapper). */
void NEOGEO_USER ng_feedback_set_sfx_hook(NGFeedbackSfxHook hook);

/*
 * Per-frame update.  Call once per frame after game logic.
 * Decrements hitstop, shake, flash timers.
 */
void NEOGEO_USER ng_feedback_update(void);

/* --- Individual controls --- */

/* Start hitstop: pause character/NPC logic for `frames` frames. */
void NEOGEO_USER ng_feedback_hitstop(uint8_t frames);

/* Query: is hitstop active? (non-zero = freeze character logic) */
uint8_t NEOGEO_USER ng_feedback_is_hitstop(void);

/* Start screen shake (forwarded to provided camera). */
void NEOGEO_USER ng_feedback_shake(NGCamera *cam, uint8_t amp, uint8_t frames);

/* Trigger a palette flash on a slot. */
void NEOGEO_USER ng_feedback_flash_white(uint8_t palette_slot, const uint16_t *base_pal, uint8_t frames);
void NEOGEO_USER ng_feedback_flash_red(uint8_t palette_slot, const uint16_t *base_pal, uint8_t frames);

/* Slow motion: half-speed for `frames` frames (character velocity scaled by 0.5). */
void NEOGEO_USER ng_feedback_slow_motion(uint8_t frames);
uint8_t NEOGEO_USER ng_feedback_is_slow_motion(void);

/* --- Composite impact event --- */

/*
 * ng_impact_event — trigger a full hit response in one call.
 *
 * impact_kind: NG_IMPACT_LIGHT / MEDIUM / HEAVY / BOSS
 * palette_slot: which palette to flash (usually the hit character's palette)
 * base_pal:     the character's base palette (for flash restore)
 * cam:          the active camera (may be NULL if no shake desired)
 * sfx_id:       sound effect to play (0 = no sound)
 * spark_x/y:    world position to spawn hit spark (use tile_base=0 to skip)
 * spark_tile:   tile base for the hit spark animation (0 = skip particles)
 * spark_pal:    palette for the spark
 */
void NEOGEO_USER ng_impact_event(uint8_t impact_kind,
                                  uint8_t palette_slot,
                                  const uint16_t *base_pal,
                                  NGCamera *cam,
                                  uint16_t sfx_id,
                                  int16_t spark_x, int16_t spark_y,
                                  uint16_t spark_tile, uint8_t spark_pal);

/* Get remaining hitstop frames (for debug HUD). */
uint8_t NEOGEO_USER ng_feedback_hitstop_remaining(void);

/* Get remaining slow motion frames. */
uint8_t NEOGEO_USER ng_feedback_slow_motion_remaining(void);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif