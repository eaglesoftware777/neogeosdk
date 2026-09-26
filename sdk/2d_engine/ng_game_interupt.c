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
#include "ng_render_queue.h"
#include "ng_particles.h"
#include "ng_palette_fx.h"
#include "ng_feedback.h"
#include "ng_pause.h"
#include "ng_depthfx.h"
#include "ng_joystick.h"

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

static uint8_t ng_hitstop_freeze = 0;

void NEOGEO_USER ng_game_engine_set_hitstop_freeze(uint8_t on)
{
    ng_hitstop_freeze = on ? 1u : 0u;
}

/* Everything that puts the frame on screen, shared by a normal frame and a
 * frozen hitstop frame. */
static void NEOGEO_USER ng_game_engine_draw(void)
{
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

void NEOGEO_USER ng_game_engine_frame(void)
{
    /* Paused (ng_pause): nothing moves -- no timers, characters, palette
     * effects, particles or camera -- but the input is still read. */
    if (ng_freeze.paused) {
        ng_joystick_update();
        return;
    }

    ng_joystick_update();

    /*
     * Hitstop, for a game that opts in: the world holds still for the few
     * frames of a heavy blow -- no logic, timers, physics or character
     * movement -- while the screen keeps drawing (so a shake still shows)
     * and the hitstop counter runs down. Slow motion holds it the same way
     * on every other frame (both are ng_pause's freeze).
     */
    if (ng_hitstop_freeze && NG_FREEZE_LOGIC()) {
        ng_game_engine_draw();
        ng_palette_fx_update();
        ng_feedback_update();
        ng_render_queue_flush();
        return;
    }

    ng_game_time_tick();   /* after the hitstop: game time holds through one too */
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

    ng_game_engine_draw();

    ng_particles_update();
    ng_palette_fx_update();
    ng_feedback_update();
    ng_render_queue_flush();
}

void NEOGEO_USER ng_game_interupt(void)
{
    ng_game_engine_frame();
}
