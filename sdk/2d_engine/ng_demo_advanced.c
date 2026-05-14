/*
 * ng_demo_advanced.c — Demonstrations of Stages 2..11 features (Stage 12)
 *
 * Each demo is a self-contained function that can be called from main.c.
 * All demos use the ng_game_engine_frame() loop so existing VBlank / sound
 * infrastructure is not bypassed.
 *
 * IMPORTANT: These demos are designed to show system features.
 *            They reference tile numbers and palette slots as placeholders.
 *            Adapt tile/palette constants to match your actual artbox output.
 *
 * Included demos:
 *   demo_sticky_character()    — large sticky-bit group moving without split
 *   demo_smooth_camera()       — smooth follow camera with border constraints
 *   demo_parallax_scene()      — two-layer parallax background (requires screens)
 *   demo_palette_flash_cycle() — palette flash, fade, pulse, colour cycle
 *   demo_hitstop_and_shake()   — hit spark + camera shake + hitstop
 *   demo_particle_stress()     — spawn many particles to test priority drop
 *   demo_depth_starfield()     — flying starfield using ng_depthfx
 *   demo_boss_shrink()         — large boss sprite with depth scale
 *   demo_perf_hud()            — performance HUD overlay
 */

#include "ng_engine.h"
#include "macro.h"
#include "neogeo.h"

/* ---------------------------------------------------------------------------
 * Shared helpers
 * -------------------------------------------------------------------------- */

/* Placeholder tile/palette constants — replace with artbox output values */
#define DEMO_TILE_CHAR_BASE    0x0010   /* first tile of walk animation strip */
#define DEMO_TILE_EFFECT_BASE  0x0200   /* hit spark tiles */
#define DEMO_PAL_CHAR          0x02
#define DEMO_PAL_BG            0x10
#define DEMO_PAL_FX            0x40

/* A minimal 16-colour palette used for flash demos */
static const uint16_t demo_base_pal[16] = {
    0x8000,   /* 0: transparent */
    0x7FFF,   /* 1: white */
    0x001F,   /* 2: blue */
    0x4F00,   /* 3: red */
    0x20F0,   /* 4: green */
    0x6FF0,   /* 5: yellow */
    0x5F0F,   /* 6: magenta */
    0x30FF,   /* 7: cyan */
    0x5555,   /* 8: mid-grey */
    0x7777,   /* 9: light-grey */
    0x2200,   /* 10: dark-green */
    0x0800,   /* 11: dark-red */
    0x0006,   /* 12: dark-blue */
    0x6600,   /* 13: dark-yellow */
    0x4400,   /* 14: dark-magenta */
    0x1800    /* 15: dark-cyan */
};

/* ---------------------------------------------------------------------------
 * Demo 1: Large sticky-bit character — 4 strips wide, moves smoothly
 * -------------------------------------------------------------------------- */
void NEOGEO_USER demo_sticky_character(void)
{
    NGSpriteGroup grp;
    uint16_t frame_count = 0;
    int16_t  x = 32;
    int16_t  dx = 1;
    int16_t  y = 100;

    ng_sprite_group_init(&grp, 0, 4, 8, DEMO_TILE_CHAR_BASE, DEMO_PAL_CHAR);
    ng_sprite_group_set_pos(&grp, x, y);
    ng_sprite_group_upload(&grp);

    /*
     * Move back and forth.  Because strips 1..3 are sticky-chain, the whole
     * group moves together without tearing.
     * dirty flag NG_SGF_DIRTY_POS is set by ng_sprite_group_set_pos();
     * ng_sprite_group_flush() writes only SCB3/4 — no redundant SCB1 writes.
     */
    while (frame_count < 300) {
        waitVbl();
        kickWatchDog();

        x = (int16_t)(x + dx);
        if (x > 200 || x < 16) dx = (int16_t)(-dx);

        ng_sprite_group_set_pos(&grp, x, y);
        ng_sprite_group_flush(&grp);   /* dirty-flag aware: only writes pos */

        frame_count++;
    }

    ng_sprite_group_hide(&grp);
}

/* ---------------------------------------------------------------------------
 * Demo 2: Smooth camera follow with border constraints
 * -------------------------------------------------------------------------- */
void NEOGEO_USER demo_smooth_camera(void)
{
    NGCamera cam;
    int16_t  target_x = 160;
    int16_t  target_y = 112;
    int16_t  target_vx = 2;
    uint16_t frame_count = 0;

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 639, 224);
    ng_camera_set_follow_speed(&cam, 0x20);   /* gentle, ~12.5% per frame */
    ng_camera_set_dead_zone(&cam, 40, 20);
    ng_camera_set_look_ahead(&cam, 48, 0, 2);

    ng_level_set_world_bounds(0, 0, 639, 224);

    while (frame_count < 360) {
        waitVbl();
        kickWatchDog();

        /* Simulate a target walking back and forth */
        target_x = (int16_t)(target_x + target_vx);
        if (target_x > 580 || target_x < 60) {
            target_vx = (int16_t)(-target_vx);
        }

        ng_camera_apply(&cam, target_x, target_y, target_vx);
        frame_count++;
    }
}

/* ---------------------------------------------------------------------------
 * Demo 3: Palette flash, fade, pulse, colour cycle
 * -------------------------------------------------------------------------- */
void NEOGEO_USER demo_palette_flash_cycle(void)
{
    uint16_t frame_count = 0;

    ng_palette_fx_init();
    ng_palfx_upload_base(DEMO_PAL_CHAR, demo_base_pal);

    /* Phase 1: white flash on impact */
    ng_palfx_flash_white(DEMO_PAL_CHAR, demo_base_pal, 8);
    while (frame_count < 60) {
        waitVbl(); kickWatchDog();
        ng_palette_fx_update();
        ng_render_queue_flush();
        frame_count++;
    }

    /* Phase 2: red damage flash */
    ng_palfx_flash_red(DEMO_PAL_CHAR, demo_base_pal, 12);
    while (frame_count < 120) {
        waitVbl(); kickWatchDog();
        ng_palette_fx_update();
        ng_render_queue_flush();
        frame_count++;
    }

    /* Phase 3: pulse effect */
    ng_palfx_pulse(DEMO_PAL_CHAR, demo_base_pal, 30);
    while (frame_count < 240) {
        waitVbl(); kickWatchDog();
        ng_palette_fx_update();
        ng_render_queue_flush();
        frame_count++;
    }

    /* Phase 4: colour cycle (colours 8..14) */
    ng_palfx_stop(DEMO_PAL_CHAR);
    ng_palfx_cycle(DEMO_PAL_CHAR, demo_base_pal, 8, 14);
    while (frame_count < 360) {
        waitVbl(); kickWatchDog();
        ng_palette_fx_update();
        ng_render_queue_flush();
        frame_count++;
    }

    /* Phase 5: full fade out */
    ng_palfx_stop(DEMO_PAL_CHAR);
    ng_palfx_fade_out(DEMO_PAL_CHAR, demo_base_pal, 60);
    while (frame_count < 420) {
        waitVbl(); kickWatchDog();
        ng_palette_fx_update();
        ng_render_queue_flush();
        frame_count++;
    }
}

/* ---------------------------------------------------------------------------
 * Demo 4: Hit spark + camera shake + hitstop
 * -------------------------------------------------------------------------- */
void NEOGEO_USER demo_hitstop_and_shake(void)
{
    NGCamera cam;
    uint16_t frame_count = 0;
    uint8_t  phase = 0;

    ng_camera_init(&cam);
    ng_camera_snap(&cam, 0, 0);
    ng_feedback_init();
    ng_feedback_set_sfx_hook(0);   /* no sound in this demo */

    ng_palfx_upload_base(DEMO_PAL_CHAR, demo_base_pal);

    while (frame_count < 360) {
        waitVbl();
        kickWatchDog();

        /* Trigger a medium impact every 90 frames */
        if ((frame_count % 90) == 45) {
            ng_impact_event(NG_IMPACT_MEDIUM,
                            DEMO_PAL_CHAR,
                            demo_base_pal,
                            &cam,
                            0,                    /* no SFX */
                            160, 112,             /* world position */
                            DEMO_TILE_EFFECT_BASE, DEMO_PAL_FX);
        }

        /* Heavy impact at frame 180 */
        if (frame_count == 180) {
            ng_impact_event(NG_IMPACT_HEAVY,
                            DEMO_PAL_CHAR,
                            demo_base_pal,
                            &cam,
                            0,
                            160, 112,
                            DEMO_TILE_EFFECT_BASE, DEMO_PAL_FX);
        }

        ng_feedback_update();
        ng_camera_update(&cam, 160, 112, 0);
        ng_level_set_scroll(cam.x, cam.y);

        ng_palette_fx_update();
        ng_particles_update();
        ng_render_queue_flush();

        frame_count++;
        NG_UNUSED(phase);
    }
}

/* ---------------------------------------------------------------------------
 * Demo 5: Particle stress test — spawn many particles, test priority drop
 * -------------------------------------------------------------------------- */
void NEOGEO_USER demo_particle_stress(void)
{
    uint16_t frame_count = 0;
    uint8_t  spawn_timer = 0;
    uint16_t first_slot = 10;

    ng_particles_init();

    while (frame_count < 480) {
        waitVbl();
        kickWatchDog();

        /* Spawn a mix of critical and optional particles every 3 frames */
        spawn_timer++;
        if (spawn_timer >= 3) {
            spawn_timer = 0;

            /* Critical hit sparks */
            ng_spawn_hit_spark(
                (int16_t)(80 + (frame_count & 0x3F)),
                (int16_t)(100 + (frame_count & 0x1F)),
                DEMO_TILE_EFFECT_BASE, DEMO_PAL_FX);

            /* Optional dust */
            ng_spawn_dust(
                (int16_t)(160 + (frame_count & 0x7F)),
                (int16_t)(112 + (frame_count & 0x3F)),
                DEMO_TILE_EFFECT_BASE + 4, DEMO_PAL_FX);

            /* Optional smoke — will be dropped when budget is tight */
            ng_spawn_smoke(
                (int16_t)(240 + (frame_count & 0x3F)),
                (int16_t)(80 + (frame_count & 0x1F)),
                DEMO_TILE_EFFECT_BASE + 8, DEMO_PAL_FX);
        }

        ng_particles_update();
        first_slot = ng_particles_draw(first_slot, 0);
        first_slot = 10;  /* reset for next frame */

        frame_count++;
    }

    ng_particles_init();   /* clear pool */
}

/* ---------------------------------------------------------------------------
 * Demo 6: Depth FX starfield
 * -------------------------------------------------------------------------- */
#define DEMO_STAR_COUNT  16

void NEOGEO_USER demo_depth_starfield(void)
{
    NGVec3       stars[DEMO_STAR_COUNT];
    NGSpriteGroup star_groups[DEMO_STAR_COUNT];
    uint16_t frame_count = 0;
    uint8_t  i;

    ng_depthfx_init();

    /* Initialise stars at random Z depths */
    for (i = 0; i < DEMO_STAR_COUNT; i++) {
        stars[i].x = (int16_t)(NG_DEPTH_CX + ((i * 23 + 7) % 160) - 80);
        stars[i].y = (int16_t)(NG_DEPTH_CY + ((i * 17 + 3) % 120) - 60);
        stars[i].z = (int16_t)((i * 8) & 0x7F);

        ng_sprite_group_init(&star_groups[i], (uint16_t)(32 + i), 1, 1,
                              DEMO_TILE_EFFECT_BASE, DEMO_PAL_FX);
    }

    while (frame_count < 300) {
        waitVbl();
        kickWatchDog();

        for (i = 0; i < DEMO_STAR_COUNT; i++) {
            /*
             * Advance star towards camera; reset to far plane when it passes.
             * Spread 200x100 around screen centre.
             */
            ng_depthfx_advance_star(&stars[i], 1, 120, 200, 100);
            ng_depthfx_draw_group(&star_groups[i], stars[i], DEMO_PAL_FX);
        }

        frame_count++;
    }

    for (i = 0; i < DEMO_STAR_COUNT; i++) {
        ng_sprite_group_hide(&star_groups[i]);
    }
}

/* ---------------------------------------------------------------------------
 * Demo 7: Boss sprite group with depth shrink effect
 * -------------------------------------------------------------------------- */
void NEOGEO_USER demo_boss_depth_shrink(void)
{
    NGSpriteGroup boss;
    NGVec3        boss_pos;
    uint16_t      frame_count = 0;

    ng_depthfx_init();
    ng_sprite_group_init(&boss, 0, 8, 16, DEMO_TILE_CHAR_BASE, DEMO_PAL_CHAR);

    boss_pos.x = NG_DEPTH_CX;
    boss_pos.y = NG_DEPTH_CY;
    boss_pos.z = 0;

    while (frame_count < 360) {
        waitVbl();
        kickWatchDog();

        /*
         * Boss approaches from depth Z=120 to Z=0 then retreats.
         * The depth system applies the shrink table automatically.
         */
        if (frame_count < 180) {
            boss_pos.z = (int16_t)(120 - frame_count / 2);
        } else {
            boss_pos.z = (int16_t)((frame_count - 180) / 2);
        }

        if (boss_pos.z < 0)   boss_pos.z = 0;
        if (boss_pos.z > 120) boss_pos.z = 120;

        ng_depthfx_draw_group(&boss, boss_pos, DEMO_PAL_CHAR);

        frame_count++;
    }

    ng_sprite_group_hide(&boss);
}

/* ---------------------------------------------------------------------------
 * Demo 8: Performance HUD
 * -------------------------------------------------------------------------- */
#ifdef NG_DEBUG_PERF
void NEOGEO_USER demo_perf_hud(void)
{
    NGCamera cam;
    uint16_t frame_count = 0;

    ng_camera_init(&cam);
    ng_feedback_init();
    ng_particles_init();
    ng_debug_init();

    while (frame_count < 600) {
        waitVbl();
        kickWatchDog();

        /* Simulate some activity */
        if ((frame_count & 7) == 0) {
            ng_spawn_dust(
                (int16_t)(80 + (frame_count & 0x3F)),
                112,
                DEMO_TILE_EFFECT_BASE, DEMO_PAL_FX);
        }

        ng_particles_update();
        ng_camera_update(&cam, 160, 112, 1);

        /* Fill debug counters */
        ng_dbg_sprites_used    = (uint16_t)(10 + ng_particles_count());
        ng_dbg_particles       = ng_particles_count();
        ng_dbg_camera_x        = cam.x;
        ng_dbg_camera_y        = cam.y;
        ng_dbg_hitstop         = ng_feedback_hitstop_remaining();
        ng_dbg_rq_used         = (uint8_t)(NG_RQ_MAX_CMDS - ng_rq_free());

        ng_debug_draw();
        ng_render_queue_flush();

        frame_count++;
    }

    ng_debug_clear();
}
#endif /* NG_DEBUG_PERF */
