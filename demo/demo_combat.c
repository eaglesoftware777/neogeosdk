/*
 * demo_combat.c — Scene 7: Impact feedback system showcase
 *
 * Demonstrates: hitstop freeze, red palette flash, camera shake,
 * hit spark particles, escalating impact levels.
 *
 * https://eaglesoftware.biz
 */

#include "demo_combat.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_engine.h"
#include "sdk/2d_engine/ng_feedback.h"
#include "sdk/2d_engine/ng_camera.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "sdk/2d_engine/ng_particles.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include <stdint.h>

void NEOGEO_USER clearFix(void);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER playSFX(uint8_t n);

/* Attacker: warrior walk/idle = screen 11, slot 1 */
#define ATKER_SCREEN   20u   /* attack pose */
#define ATKER_SLOT      1u

/* Defender: NPC = screen 109, slot 20 */
#define DEFDR_SCREEN  109u
#define DEFDR_SLOT     20u

/* Base palette slots */
#define ATKER_PAL  DEMO_SCREEN_PALETTE(ATKER_SCREEN)
#define DEFDR_PAL  DEMO_SCREEN_PALETTE(DEFDR_SCREEN)

static const uint16_t s_atker_pal[16] = {
    0x0000u, 0x7FFFu, 0x4F00u, 0x2422u, 0x3747u, 0x7551u,
    0x7001u, 0x7011u, 0x4e82u, 0x2a82u, 0x5341u, 0x3113u,
    0x1448u, 0x1b55u, 0x6FF0u, 0x30FFu
};

/* SFX hook for ng_feedback */
static void NEOGEO_USER combat_sfx_hook(uint16_t id)
{
    if (id < 32u) playSFX((uint8_t)id);
}

/* ------------------------------------------------------------------ */
/*  Combat sub-scene: escalating hit sequence                            */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER combat_hit_sequence(void)
{
    NGCamera cam;
    uint8_t  hit;
    uint16_t t;

    clearFix();
    demo_fix_puts(2u, 0u, "IMPACT FEEDBACK SYSTEM", 2u);
    demo_fix_puts(2u, 1u, "HITSTOP + SHAKE + FLASH + PARTICLES", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 0, 0);
    ng_camera_snap(&cam, 0, 0);

    ng_feedback_init();
    ng_feedback_set_sfx_hook(combat_sfx_hook);

    ng_particles_init();

    /* Draw both characters once (static for hitstop demo) */
    demo_load_screen_palette(11u);
    demo_draw_sprite_screen(11u, ATKER_SLOT,  60, (int16_t)(-34), 16u, 16u, 0xFFu, 0xFFu);
    demo_load_screen_palette(109u);
    demo_draw_sprite_screen(109u, DEFDR_SLOT, 200, (int16_t)(-60), 6u, 6u, 0x70u, 0x70u);

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    for (t = 0u; t < 360u; t++) {
        /* Trigger a hit at frames 60, 120, 180, 240 — escalating */
        if ((t == 60u) || (t == 120u) || (t == 180u) || (t == 240u)) {
            hit = (uint8_t)((t / 60u) - 1u);  /* 0=light, 1=medium, 2=heavy, 3=boss */
            if (hit > NG_IMPACT_BOSS) hit = NG_IMPACT_BOSS;

            ng_impact_event(hit, DEFDR_PAL, s_atker_pal,
                            &cam,
                            (uint16_t)SOUND_SFX_IMPACT_HIT,
                            200, 112, 0u, 0x40u);

            {
                const char *labels[4] = {
                    "LIGHT IMPACT  HITSTOP 3F",
                    "MEDIUM IMPACT  SHAKE 6F",
                    "HEAVY IMPACT  SHAKE 8F",
                    "BOSS HIT  MAX SHAKE 12F"
                };
                demo_fix_puts(2u, 3u, labels[hit], (uint8_t)(hit + 1u));
            }
        }

        /* Update systems */
        ng_camera_update(&cam, 0, 0, 0);
        ng_particles_update();
        ng_particles_draw(40u, 40u);
        ng_feedback_update();
        ng_palette_fx_update();

        if (demo_frame()) break;
    }

    ng_particles_init();
}

/* ------------------------------------------------------------------ */
/*  Public: combat scene                                                 */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_combat_run(void)
{
    demo_clear_scene();
    soundSceneReset();
    soundSetADPCMAVolume(0x3Cu);

    combat_hit_sequence();

    soundStopAll();
    demo_clear_scene();
}
