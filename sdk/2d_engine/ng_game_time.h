#ifndef NG_GAME_TIME_H
#define NG_GAME_TIME_H

#include "ng_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

void NEOGEO_USER ng_game_time_init(void);
void NEOGEO_USER ng_game_time_tick(void);
void NEOGEO_USER ng_game_time_reset_stage(void);
/*
 * The stage clock (ng_game_time_stage_frame) counts only while running;
 * it runs from ng_game_time_init(). A game stops it for what it doesn't
 * want timed -- a mission card, a scene change -- and starts it again. Like
 * every clock here it counts frames, not wall time: see docs/game_time.md.
 */
void NEOGEO_USER ng_game_time_stage_run(uint8_t on);
uint32_t NEOGEO_USER ng_game_time_frame(void);
uint16_t NEOGEO_USER ng_game_time_second(void);
uint32_t NEOGEO_USER ng_game_time_stage_frame(void);


#ifdef __cplusplus
}
#endif
#endif
