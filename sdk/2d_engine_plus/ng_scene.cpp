#include "ng_scene.hpp"
#include "ng_chars.hpp"
#include "ng_physics.hpp"
#include "ng_game_events.hpp"
#include "ng_timers.hpp"
#include "ng_progress.hpp"
#include "ng_palette_fx.hpp"
#include "ng_sprite_pool.hpp"
#include "ng_sprite_group.hpp"
#include "ng_hw.hpp"

extern "C" {

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

} /* extern "C" */
