#include "ng_game_interupt.h"
#include "ng_properties.h"
#include "ng_game_time.h"
#include "ng_timers.h"
#include "ng_progress.h"
#include "ng_status.h"
#include "ng_game_events.h"
#include "ng_level.h"
#include "ng_bg.h"
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

void NEOGEO_USER ng_game_engine_init(void)
{
    ng_properties_init();
    ng_game_time_init();
    ng_timers_init();
    ng_progress_init();
    ng_status_init();
    ng_game_events_init();
    ng_level_init();
    ng_fix_init();
    ng_border_constraints_init();
    ng_actions_init();
    ng_chars_init();
    ng_npcs_init();
    ng_physics_init();

    ng_before_logic = 0;
    ng_collision_logic = 0;
    ng_after_events = 0;
    ng_before_draw = 0;
    ng_after_draw = 0;
    ng_bg_init();
}

void NEOGEO_USER ng_game_runtime_init(void)
{
    ng_game_engine_init();
}

void NEOGEO_USER ng_game_engine_set_hooks(
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

void NEOGEO_USER ng_game_interupt_set_hooks(
    NGInteruptHook before_logic,
    NGInteruptHook collision_logic,
    NGInteruptHook after_events,
    NGInteruptHook before_draw,
    NGInteruptHook after_draw
) {
    ng_game_engine_set_hooks(
        before_logic,
        collision_logic,
        after_events,
        before_draw,
        after_draw
    );
}

void NEOGEO_USER ng_game_engine_frame(void)
{
    ng_game_time_tick();

    if (ng_before_logic) ng_before_logic();

    ng_timers_update();
    ng_level_update();
    ng_npcs_update();
    ng_physics_update_pre();
    ng_border_constraints_update();
    ng_chars_update();
    ng_physics_resolve();

    if (ng_collision_logic) ng_collision_logic();

    ng_game_events_update();

    if (ng_after_events) ng_after_events();

    ng_progress_update();

    if (ng_before_draw) ng_before_draw();

    {
        const NGLevelState *_level = level_state();
        int16_t _cam_x = _level ? _level->scroll_x : 0;
        int16_t _cam_y = _level ? _level->scroll_y : 0;
        ng_bg_draw(_cam_x, _cam_y);
    }
    ng_chars_draw();

    if (ng_after_draw) ng_after_draw();
}

void NEOGEO_USER ng_game_interupt(void)
{
    ng_game_engine_frame();
}
