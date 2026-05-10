#include "ng_game_interupt.h"
#include "ng_properties.h"
#include "ng_game_time.h"
#include "ng_timers.h"
#include "ng_progress.h"
#include "ng_status.h"
#include "ng_game_events.h"
#include "ng_level.h"
#include "ng_fix.h"
#include "ng_border_constraints.h"
#include "ng_chars.h"
#include "ng_npcs.h"
#include "ng_physics.h"
#include "ng_actions.h"

static NGInteruptHook ng_before_logic;
static NGInteruptHook ng_collision_logic;
static NGInteruptHook ng_after_events;
static NGInteruptHook ng_before_draw;
static NGInteruptHook ng_after_draw;

void NEOGEO_USER game_engine_init(void)
{
    properties_init();
    game_time_init();
    timers_init();
    progress_init();
    status_init();
    game_events_init();
    level_init();
    ng_fix_init();
    border_constraints_init();
    actions_init();
    chars_init();
    npcs_init();
    physics_init();

    ng_before_logic = 0;
    ng_collision_logic = 0;
    ng_after_events = 0;
    ng_before_draw = 0;
    ng_after_draw = 0;
}

void NEOGEO_USER game_runtime_init(void)
{
    game_engine_init();
}

void NEOGEO_USER game_engine_set_hooks(
    NGInteruptHook before_logic,
    NGInteruptHook collision_logic,
    NGInteruptHook after_events,
    NGInteruptHook before_draw,
    NGInteruptHook after_draw
) {
    ng_before_logic = before_logic;
    ng_collision_logic = collision_logic;
    ng_after_events = after_events;
    ng_before_draw = before_draw;
    ng_after_draw = after_draw;
}

void NEOGEO_USER game_interupt_set_hooks(
    NGInteruptHook before_logic,
    NGInteruptHook collision_logic,
    NGInteruptHook after_events,
    NGInteruptHook before_draw,
    NGInteruptHook after_draw
) {
    game_engine_set_hooks(
        before_logic,
        collision_logic,
        after_events,
        before_draw,
        after_draw
    );
}

void NEOGEO_USER game_engine_frame(void)
{
    game_time_tick();

    if (ng_before_logic) ng_before_logic();

    timers_update();
    level_update();
    npcs_update();
    physics_update_pre();
    border_constraints_update();
    chars_update();
    physics_resolve();

    if (ng_collision_logic) ng_collision_logic();

    game_events_update();

    if (ng_after_events) ng_after_events();

    progress_update();

    if (ng_before_draw) ng_before_draw();

    chars_draw();

    if (ng_after_draw) ng_after_draw();
}

void NEOGEO_USER game_interupt(void)
{
    game_engine_frame();
}
