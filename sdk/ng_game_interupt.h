#ifndef NG_GAME_INTERUPT_H
#define NG_GAME_INTERUPT_H

#include "ng_defs.h"

typedef void (*NGInteruptHook)(void);

void game_engine_init(void);
void game_runtime_init(void);
void game_engine_set_hooks(
    NGInteruptHook before_logic,
    NGInteruptHook collision_logic,
    NGInteruptHook after_events,
    NGInteruptHook before_draw,
    NGInteruptHook after_draw
);
void game_interupt_set_hooks(
    NGInteruptHook before_logic,
    NGInteruptHook collision_logic,
    NGInteruptHook after_events,
    NGInteruptHook before_draw,
    NGInteruptHook after_draw
);
void game_engine_frame(void);
void game_interupt(void);

#endif
