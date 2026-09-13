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

#define SKY_STAGE_COUNT   7u

/* Pool caps.  ng_chars holds NG_MAX_CHARS objects and ng_chars_draw has
 * NG_SPR_CHAR_FIRST..LAST (128) hardware strips to hand out; an 8-strip
 * enemy eats eight of them, so these are the numbers that keep a boss
 * fight inside both budgets instead of silently dropping sprites. */
#define SKY_MAX_ENEMIES   6u
#define SKY_MAX_PSHOTS    6u
#define SKY_MAX_ESHOTS    10u
#define SKY_MAX_BLASTS    4u

void NEOGEO_USER sky_stage_begin(uint8_t stage);
uint8_t NEOGEO_USER sky_stage_bg(uint8_t stage);
const char * NEOGEO_USER sky_stage_boss_name(uint8_t stage);

/* One director step: enemy/boss AI, wave spawning, enemy fire.
 * Returns 1 on the frame the stage's boss dies. */
uint8_t NEOGEO_USER sky_stage_tick(int16_t player_x, int16_t player_y);

uint8_t  NEOGEO_USER sky_stage_boss_active(void);
uint8_t  NEOGEO_USER sky_stage_boss_bar(void);   /* 0..24 cells */
uint16_t NEOGEO_USER sky_stage_kills(void);

/* Shared spawns used by both the director and the player code. */
NGCharacter * NEOGEO_USER sky_spawn_blast(int16_t x, int16_t y);
uint8_t NEOGEO_USER sky_count_kind(uint8_t kind);

#endif /* SKY_STAGE_H */
