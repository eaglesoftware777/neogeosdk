#ifndef NG_GAME_INTERUPT_H
#define NG_GAME_INTERUPT_H

#include "ng_defs.h"

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
/* Opt in to a real hitstop: while ng_feedback_is_hitstop(), the engine frame
 * skips logic, timers, physics and character movement but still draws and
 * counts the hitstop down. Off by default, so existing games keep their
 * behaviour. */
void NEOGEO_USER ng_game_engine_set_hitstop_freeze(uint8_t on);
void NEOGEO_USER ng_game_interupt(void);


#ifdef __cplusplus
}
#endif
#endif
