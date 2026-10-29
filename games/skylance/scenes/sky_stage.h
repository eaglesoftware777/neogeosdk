#ifndef SKY_STAGE_H
#define SKY_STAGE_H

#include <stdint.h>
#include "sky.h"
#include "sdk/2d_engine/ng_chars.h"

/* Character kinds.  ng_chars keys its per-kind helpers off these. */
#define SKY_KIND_PLAYER   1u
#define SKY_KIND_ENEMY    2u
#define SKY_KIND_BOSS     3u
#define SKY_KIND_PSHOT    4u
#define SKY_KIND_ESHOT    5u
#define SKY_KIND_BLAST    6u
#define SKY_KIND_FACE     7u
#define SKY_KIND_PICKUP   8u
#define SKY_KIND_SMOKE    9u
#define SKY_KIND_SUPER   10u
#define SKY_KIND_SCENERY 11u
#define SKY_KIND_CHAFF   12u
#define SKY_KIND_RADAR   13u
#define SKY_KIND_MODULE  14u
#define SKY_KIND_CANOPY  15u
#define SKY_KIND_WRECK   16u

void NEOGEO_USER sky_stage_camera_shift(int16_t delta);
void NEOGEO_USER sky_stage_parts_update(void);

#define SKY_STAGE_COUNT   7u

/* Pool caps.  ng_chars holds NG_MAX_CHARS objects and ng_chars_draw has
 * NG_SPR_CHAR_FIRST..LAST (128) hardware strips to hand out; an 8-strip
 * enemy eats eight of them, so these are the numbers that keep a boss
 * fight inside both budgets instead of silently dropping sprites. */
#define SKY_MAX_ENEMIES   4u
#define SKY_MAX_PSHOTS    10u
#define SKY_MAX_ESHOTS    12u   /* pinned slots; each stage's live cap is in k_difficulty */
#define SKY_MAX_BLASTS    4u
#define SKY_MAX_PICKUPS   3u

void NEOGEO_USER sky_stage_begin(uint8_t stage);
uint8_t NEOGEO_USER sky_stage_bg(uint8_t stage);
uint8_t NEOGEO_USER sky_stage_has_boss(uint8_t stage);
const char * NEOGEO_USER sky_stage_boss_name(uint8_t stage);
const char * NEOGEO_USER sky_stage_subtitle(uint8_t stage);
const char * NEOGEO_USER sky_stage_objective(uint8_t stage);
const char * NEOGEO_USER sky_stage_intel(uint8_t stage);
const char * NEOGEO_USER sky_stage_tactic(uint8_t stage);

/* One director step: enemy/boss behaviour, wave spawning, enemy fire.
 * Returns 1 on the frame the stage's boss dies. */
uint8_t NEOGEO_USER sky_stage_tick(int16_t player_x, int16_t player_y);

uint8_t  NEOGEO_USER sky_stage_boss_active(void);
uint8_t  NEOGEO_USER sky_stage_boss_bar(void);   /* 0..24 cells */
uint16_t NEOGEO_USER sky_stage_kills(void);
uint8_t NEOGEO_USER sky_stage_targets(void);
uint8_t NEOGEO_USER sky_stage_target_alert(void);
uint8_t NEOGEO_USER sky_stage_paused(void);
uint8_t NEOGEO_USER sky_stage_encounter(void);
uint8_t NEOGEO_USER sky_stage_radar_alert(void);
uint8_t NEOGEO_USER sky_stage_chaff_stock(void);
uint8_t NEOGEO_USER sky_stage_chaff(int16_t x, int16_t y);
uint8_t NEOGEO_USER sky_stage_cruising(void);  /* waves spent, flying on to the arena */
uint8_t NEOGEO_USER sky_stage_seekers(void);   /* guided missiles still tracking the jet */
/* Record a defeated target before removal and return its score award. */
uint16_t NEOGEO_USER sky_stage_defeated(const NGCharacter *c);
/* Turn a defeated enemy (or the Crimson Keep) into a wreck that plays its
 * destruction frames.
 * Returns 0 when it has none; the caller then removes it. */
uint8_t NEOGEO_USER sky_stage_wreck(NGCharacter *c);
void NEOGEO_USER sky_stage_damage(NGCharacter *c, uint16_t damage);
void NEOGEO_USER sky_stage_effects_tick(void);

/* Shared spawns used by both the director and the player code. */
NGCharacter * NEOGEO_USER sky_spawn_blast(int16_t x, int16_t y);
NGCharacter * NEOGEO_USER sky_spawn_smoke(int16_t x, int16_t y);
uint8_t NEOGEO_USER sky_count_kind(uint8_t kind);

#endif /* SKY_STAGE_H */
