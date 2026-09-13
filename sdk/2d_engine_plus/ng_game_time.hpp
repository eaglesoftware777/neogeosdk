#ifndef NG_GAME_TIME_HPP
#define NG_GAME_TIME_HPP

#include "ng_defs.hpp"

#ifdef __cplusplus
extern "C" {
#endif


void NEOGEO_USER ng_game_time_init(void);
void NEOGEO_USER ng_game_time_tick(void);
void NEOGEO_USER ng_game_time_reset_stage(void);
uint32_t NEOGEO_USER ng_game_time_frame(void);
uint16_t NEOGEO_USER ng_game_time_second(void);
uint32_t NEOGEO_USER ng_game_time_stage_frame(void);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif