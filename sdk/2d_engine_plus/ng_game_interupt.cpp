#include "ng_game_interupt.hpp"
#include "ng_properties.hpp"
#include "ng_game_time.hpp"
#include "ng_timers.hpp"
#include "ng_progress.hpp"
#include "ng_status.hpp"
#include "ng_game_events.hpp"
#include "ng_level.hpp"
#include "ng_bg.hpp"
#include "ng_fix.hpp"
#include "ng_border_constraints.hpp"
#include "ng_chars.hpp"
#include "ng_npcs.hpp"
#include "ng_physics.hpp"
#include "ng_actions.hpp"
#include "ng_render_queue.hpp"
#include "ng_particles.hpp"
#include "ng_palette_fx.hpp"
#include "ng_feedback.hpp"
#include "ng_depthfx.hpp"
#include "ng_joystick.hpp"

extern "C" {


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
    ng_render_queue_init();
    ng_particles_init();
    ng_palette_fx_init();
    ng_feedback_init();
    ng_depthfx_init();
    ng_joystick_init();

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
    ng_joystick_update();

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

    ng_particles_update();
    ng_palette_fx_update();
    ng_feedback_update();
    ng_render_queue_flush();
}

void NEOGEO_USER ng_game_interupt(void)
{
    ng_game_engine_frame();
}


} /* extern "C" */
