#include "ng_scene.h"
#include "ng_chars.h"
#include "ng_physics.h"
#include "ng_game_events.h"
#include "ng_timers.h"
#include "ng_progress.h"
#include "ng_palette_fx.h"
#include "ng_sprite_pool.h"
#include "ng_sprite_group.h"
#include "neogeo.h"

void NEOGEO_USER ng_scene_clean(uint8_t flags)
{
    uint8_t i;

    if (flags & NG_SCENE_CLEAN_FIX) {
        clearFix();
    }

    if (flags & NG_SCENE_CLEAN_SPRITES) {
        clearSprs();
        ng_sprite_hide_all();
    }

    if (flags & NG_SCENE_CLEAN_CHARS) {
        for (i = 0u; i < NG_MAX_CHARS; i++) {
            ng_chars_reset_slot(i);
        }
        ng_chars_defrag_slots();
    }

    if (flags & NG_SCENE_CLEAN_PHYSICS) {
        ng_physics_init();
    }

    if (flags & NG_SCENE_CLEAN_EVENTS) {
        ng_game_events_init();
    }

    if (flags & NG_SCENE_CLEAN_TIMERS) {
        ng_timers_init();
    }

    if (flags & NG_SCENE_CLEAN_PROGRESS) {
        ng_progress_init();
    }

    if (flags & NG_SCENE_CLEAN_PALETTEFX) {
        ng_palette_fx_init();
    }
}

void NEOGEO_USER ng_scene_clean_default(void)
{
    ng_scene_clean(NG_SCENE_CLEAN_DEFAULT);
}
