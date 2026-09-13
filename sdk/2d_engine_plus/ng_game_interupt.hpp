#ifndef NG_GAME_INTERUPT_HPP
#define NG_GAME_INTERUPT_HPP

#include "ng_defs.hpp"

#ifdef __cplusplus
extern "C" {
#endif


typedef void(*NGInteruptHook)(void);

void NEOGEO_USER ng_game_engine_init(void);
void NEOGEO_USER ng_game_runtime_init(void);
void NEOGEO_USER ng_game_engine_set_hooks(
    NGInteruptHook before_logic,
    NGInteruptHook collision_logic,
    NGInteruptHook after_events,
    NGInteruptHook before_draw,
    NGInteruptHook after_draw
);
void NEOGEO_USER ng_game_interupt_set_hooks(
    NGInteruptHook before_logic,
    NGInteruptHook collision_logic,
    NGInteruptHook after_events,
    NGInteruptHook before_draw,
    NGInteruptHook after_draw
);
void NEOGEO_USER ng_game_engine_frame(void);
void NEOGEO_USER ng_game_interupt(void);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif