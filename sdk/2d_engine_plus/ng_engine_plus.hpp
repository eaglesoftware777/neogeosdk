#ifndef NG_ENGINE_PLUS_HPP
#define NG_ENGINE_PLUS_HPP

/*
 * ng_engine_plus.hpp — C++ umbrella header for sdk/2d_engine_plus.
 *
 * Drop-in replacement for ng_engine.h / ng_engine.hpp when compiling game
 * code as C++.  All public functions retain C linkage (extern "C") so
 * existing .c game files can link against the C++ engine without changes.
 *
 * Compiler requirements: c++14, -fno-exceptions, -fno-rtti,
 *   -fno-threadsafe-statics (embedded bare-metal target).
 *
 * Usage:
 *   C++ game file:  #include "ng_engine_plus.hpp"
 *   C game file:    #include "ng_engine.h"   (links against same objects)
 */

#include "ng_defs.hpp"
#include "sound_ids.h"
#include "ng_properties.hpp"
#include "ng_game_time.hpp"
#include "ng_timers.hpp"
#include "ng_progress.hpp"
#include "ng_status.hpp"
#include "ng_game_events.hpp"
#include "ng_level.hpp"
#include "ng_sprite_pool.hpp"
#include "ng_bg.hpp"
#include "ng_fix.hpp"
#include "ng_sprite_group.hpp"
#include "ng_chars.hpp"
#include "ng_actions.hpp"
#include "ng_npcs.hpp"
#include "ng_physics.hpp"
#include "ng_border_constraints.hpp"
#include "ng_game_interupt.hpp"
#include "ng_scene.hpp"
#include "ng_render_queue.hpp"
#include "ng_fixed.hpp"
#include "ng_camera.hpp"
#include "ng_palette_fx.hpp"
#include "ng_particles.hpp"
#include "ng_feedback.hpp"
#include "ng_depthfx.hpp"
#include "ng_debug.hpp"
#include "ng_joystick.hpp"

#endif
