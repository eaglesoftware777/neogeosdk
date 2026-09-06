#ifndef NG_ENGINE_H
#define NG_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Core engine */
#include "ng_defs.h"
#include "sound_ids.h"
#include "ng_properties.h"
#include "ng_game_time.h"
#include "ng_timers.h"
#include "ng_progress.h"
#include "ng_status.h"
#include "ng_game_events.h"
#include "ng_level.h"
#include "ng_sprite_pool.h"
#include "ng_vram.h"
#include "ng_sprite_window.h"
#include "ng_art_asset.h"
#include "ng_palette_assets.h"
#include "ng_bg.h"
#include "ng_fix.h"
#include "ng_sprite_group.h"
#include "ng_chars.h"
#include "ng_actions.h"
#include "ng_npcs.h"
#include "ng_physics.h"
#include "ng_border_constraints.h"
#include "ng_game_interupt.h"
#include "ng_scene.h"

/* New subsystems (Stages 3..11) */
#include "ng_render_queue.h"
#include "ng_fixed.h"
#include "ng_camera.h"
#include "ng_palette_fx.h"
#include "ng_particles.h"
#include "ng_feedback.h"
#include "ng_depthfx.h"
#include "ng_debug.h"
#include "ng_joystick.h"


#ifdef __cplusplus
}
#endif
#endif
