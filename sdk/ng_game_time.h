#ifndef NG_GAME_TIME_H
#define NG_GAME_TIME_H

#include "ng_defs.h"

void game_time_init(void);
void game_time_tick(void);
void game_time_reset_stage(void);
uint32_t game_time_frame(void);
uint16_t game_time_second(void);
uint32_t game_time_stage_frame(void);

#endif
