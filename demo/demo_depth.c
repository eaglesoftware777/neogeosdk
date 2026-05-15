/*
 * demo_depth.c — Scene 8: 2.5D projection engine showcase
 *
 * Demonstrates: starfield warp (Z-based shrink), NGVec3 perspective
 * projection, fog palette bands, depth-sorted sprites.
 *
 * https://eaglesoftware.biz
 */

#include "demo_depth.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_engine.h"
#include "sdk/2d_engine/ng_depthfx.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include "sdk/2d_engine/ng_fixed.h"
#include <stdint.h>

void NEOGEO_USER clearFix(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER playSFX(uint8_t n);

/* Use 16 star objects — each is a 1-strip sprite */
#define STAR_COUNT   16u
#define STAR_SLOT_0   2u   /* slots 2..17 */

/* NPC tile for flying objects */
#define DEPTH_NPC_TILE    ((uint16_t)(27648u + 165u))
#define DEPTH_NPC_PAL     124u
#define DEPTH_NPC_STRIPS   6u
#define DEPTH_NPC_ROWS     6u

/* xorshift noise for star reset positions */
static uint16_t s_lfsr = 0xACE1u;
static uint8_t NEOGEO_USER depth_rand8(void)
{
    s_lfsr ^= (uint16_t)(s_lfsr << 7);
    s_lfsr ^= (uint16_t)(s_lfsr >> 9);
    s_lfsr ^= (uint16_t)(s_lfsr << 8);
    return (uint8_t)(s_lfsr & 0xFFu);
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: starfield warp                                            */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER depth_starfield(void)
{
    NGVec3       stars[STAR_COUNT];
    NGSpriteGroup star_grp[STAR_COUNT];
    uint8_t      i;
    uint16_t     t;

    clearFix();
    setBACKDROP(BLACK);
    demo_fix_puts(2u, 0u, "STARFIELD WARP", 2u);
    demo_fix_puts(2u, 1u, "Z-PROJECTION  SHRINK TABLE  NO DIVISION", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    ng_depthfx_init();

    /* Init stars spread across screen */
    for (i = 0u; i < STAR_COUNT; i++) {
        stars[i].x = (int16_t)((int16_t)(depth_rand8() & 0x7Fu) - 64 + NG_DEPTH_CX);
        stars[i].y = (int16_t)((int16_t)(depth_rand8() & 0x7Fu) - 64 + NG_DEPTH_CY);
        stars[i].z = (int16_t)(depth_rand8() & 0x7Fu);

        ng_sprite_group_init(&star_grp[i], (uint16_t)(STAR_SLOT_0 + i),
                             1u, 1u, 0u, 0u);
        star_grp[i].visible = 1u;
    }

    demo_load_screen_palette(109u);
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    for (t = 0u; t < 300u; t++) {
        for (i = 0u; i < STAR_COUNT; i++) {
            NGProjected proj;
            uint8_t sz;

            /* Move star toward camera */
            ng_depthfx_advance_star(&stars[i], 2, 127, 120, 80);

            proj = ng_depthfx_project(stars[i], DEPTH_NPC_PAL);

            if (proj.visible) {
                /* Size the group tile based on depth */
                sz = proj.shrink_y;
                if (sz < 0x20u) sz = 0x20u;

                ng_sprite_group_init(&star_grp[i], (uint16_t)(STAR_SLOT_0 + i),
                                     DEPTH_NPC_STRIPS, DEPTH_NPC_ROWS,
                                     DEPTH_NPC_TILE, proj.palette);
                ng_sprite_group_set_tile_stride(&star_grp[i], 16u);
                ng_sprite_group_set_active_rows(&star_grp[i], DEPTH_NPC_ROWS);
                ng_sprite_group_set_scale(&star_grp[i], sz, sz);
                ng_sprite_group_set_pos(&star_grp[i], proj.screen_x, proj.screen_y);
                ng_sprite_group_set_visible(&star_grp[i], 1u);
                ng_sprite_group_upload(&star_grp[i]);
            } else {
                ng_sprite_group_hide(&star_grp[i]);
            }
        }

        if (demo_frame()) break;
    }

    for (i = 0u; i < STAR_COUNT; i++) {
        ng_sprite_group_hide(&star_grp[i]);
    }
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: 2.5D perspective depth display using FIX lines           */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER depth_perspective_floor(void)
{
    uint16_t t;
    uint8_t  phase;

    clearFix();
    demo_fix_puts(2u, 0u, "2.5D PROJECTION ENGINE", 2u);
    demo_fix_puts(2u, 1u, "VANISHING POINT  CONVERGENCE LINES", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    for (t = 0u; t < 300u; t++) {
        phase = (uint8_t)((t >> 4) & 1u);

        /* Redraw perspective lines each frame */
        demo_fix_puts(1u,  9u, "                                      ", 0u);
        demo_fix_puts(1u, 10u, "                                      ", 0u);
        demo_fix_puts(1u, 11u, "                                      ", 0u);
        demo_fix_puts(1u, 12u, "                                      ", 0u);
        demo_fix_puts(1u, 13u, "                                      ", 0u);
        demo_fix_puts(1u, 14u, "                                      ", 0u);
        demo_fix_puts(1u, 15u, "                                      ", 0u);
        demo_fix_puts(1u, 16u, "                                      ", 0u);

        demo_fix_puts(17u, 9u, "-",  2u);
        demo_fix_puts(16u, 10u, "---", 1u);
        if (phase) {
            demo_fix_puts(14u, 11u, "-------", 1u);
        } else {
            demo_fix_puts(13u, 11u, "---------", 1u);
        }
        demo_fix_puts(10u, 12u, "================", 0u);
        demo_fix_puts(6u,  14u, "========================", 0u);
        demo_fix_puts(2u,  16u, "====================================", 1u);

        /* Converging side lines */
        demo_fix_puts(7u,  10u, "/",  1u);
        demo_fix_puts(31u, 10u, "\\", 1u);
        demo_fix_puts(4u,  13u, "/",  1u);
        demo_fix_puts(34u, 13u, "\\", 1u);

        if ((t % 90u) == 0u) playSFX(SOUND_SFX_LOW_DRUM);
        if (demo_frame()) break;
    }
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: 8 Z-sorted objects                                        */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER depth_zsort(void)
{
#define ZSORT_N  8u
    NGVec3       objs[ZSORT_N];
    NGSpriteGroup grps[ZSORT_N];
    uint8_t       order[ZSORT_N];
    uint8_t       i, j, tmp8;
    uint16_t      t;

    static const int16_t start_x[ZSORT_N] = { -60, -40, -20, 0, 20, 40, 60, -10 };
    static const int16_t start_y[ZSORT_N] = { 0, -10, 10, -5, 5, 0, -8, 12 };

    clearFix();
    setBACKDROP(BLACK);
    demo_fix_puts(2u, 0u, "Z-SORTED DEPTH: 8 OBJECTS", 2u);
    demo_fix_puts(2u, 1u, "PAINTER ALGORITHM  Z-FOG  8-STEP ORBIT", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    ng_depthfx_init();
    demo_load_screen_palette(109u);

    static const uint8_t s_angle_tab[8] = {
        0, 32, 64, 96, 128, 160, 192, 224
    };

    for (i = 0u; i < ZSORT_N; i++) {
        objs[i].x = start_x[i];
        objs[i].y = start_y[i];
        objs[i].z = (int16_t)(8 + i * 10);
        order[i]  = i;
        ng_sprite_group_init(&grps[i], (uint16_t)(STAR_SLOT_0 + i),
                             DEPTH_NPC_STRIPS, DEPTH_NPC_ROWS,
                             DEPTH_NPC_TILE, DEPTH_NPC_PAL);
        ng_sprite_group_set_tile_stride(&grps[i], 16u);
        ng_sprite_group_set_active_rows(&grps[i], DEPTH_NPC_ROWS);
    }

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    for (t = 0u; t < 300u; t++) {
        uint8_t phase = (uint8_t)(t & 7u);

        /* Animate objects in circular orbit using step table */
        for (i = 0u; i < ZSORT_N; i++) {
            uint8_t angle = (uint8_t)((s_angle_tab[i] + phase * 8u) & 0xFFu);
            /* cos-like approximation: 0..7 → {64,45,0,-45,-64,-45,0,45} */
            static const int8_t cstep[8] = { 64, 45, 0, -45, -64, -45, 0, 45 };
            static const int8_t sstep[8] = { 0,  45, 64, 45,  0,  -45,-64,-45 };
            uint8_t ai = (uint8_t)((angle >> 5) & 7u);
            objs[i].x = (int16_t)(start_x[i] + cstep[ai]);
            objs[i].z = (int16_t)(8 + i * 10 + sstep[ai] + 64);
        }

        /* Bubble sort by Z descending (far first = painter) */
        for (i = 0u; i < ZSORT_N - 1u; i++) {
            for (j = 0u; j < ZSORT_N - 1u - i; j++) {
                if (objs[order[j]].z < objs[order[j+1u]].z) {
                    tmp8 = order[j];
                    order[j] = order[j+1u];
                    order[j+1u] = tmp8;
                }
            }
        }

        /* Draw back-to-front */
        for (i = 0u; i < ZSORT_N; i++) {
            uint8_t idx  = order[i];
            NGProjected proj = ng_depthfx_project(objs[idx], DEPTH_NPC_PAL);
            if (proj.visible) {
                uint8_t sz = proj.shrink_y;
                if (sz < 0x18u) sz = 0x18u;
                ng_sprite_group_init(&grps[idx],
                                     (uint16_t)(STAR_SLOT_0 + idx),
                                     DEPTH_NPC_STRIPS, DEPTH_NPC_ROWS,
                                     DEPTH_NPC_TILE, proj.palette);
                ng_sprite_group_set_tile_stride(&grps[idx], 16u);
                ng_sprite_group_set_active_rows(&grps[idx], DEPTH_NPC_ROWS);
                ng_sprite_group_set_scale(&grps[idx], sz, sz);
                ng_sprite_group_set_pos(&grps[idx], proj.screen_x, proj.screen_y);
                ng_sprite_group_upload(&grps[idx]);
            } else {
                ng_sprite_group_hide(&grps[idx]);
            }
        }

        if (demo_frame()) break;
    }

    for (i = 0u; i < ZSORT_N; i++) {
        ng_sprite_group_hide(&grps[i]);
    }
#undef ZSORT_N
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: depth parallax ground plane                               */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER depth_ground_plane(void)
{
#define GPLANE_STRIPS  4u
    NGSpriteGroup strips[GPLANE_STRIPS];
    uint8_t       i;
    uint16_t      t;
    uint16_t      tile_off;

    static const int16_t  strip_y[GPLANE_STRIPS]  = { 160, 180, 196, 208 };
    static const uint8_t  strip_sc[GPLANE_STRIPS]  = { 0x40u, 0x60u, 0x90u, 0xC0u };

    clearFix();
    setBACKDROP(BLACK);
    demo_fix_puts(2u, 0u, "DEPTH PARALLAX / Z-SCALE TABLE", 2u);
    demo_fix_puts(2u, 1u, "4 GROUND STRIPS  NG_SHRINK_TAB SCALE", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    demo_load_screen_palette(1u);

    for (i = 0u; i < GPLANE_STRIPS; i++) {
        ng_sprite_group_init(&strips[i],
                             (uint16_t)(NG_SPR_BG0_FIRST + (uint16_t)i * 16u),
                             16u, 1u,
                             DEMO_SCREEN_TILE(1u), DEMO_SCREEN_PALETTE(1u));
        ng_sprite_group_set_tile_stride(&strips[i], 1u);
        ng_sprite_group_set_active_rows(&strips[i], 1u);
        ng_sprite_group_set_scale(&strips[i], strip_sc[i], strip_sc[i]);
        ng_sprite_group_set_pos(&strips[i], 0, strip_y[i]);
    }

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    tile_off = 0u;
    for (t = 0u; t < 300u; t++) {
        for (i = 0u; i < GPLANE_STRIPS; i++) {
            uint16_t scroll = (uint16_t)(tile_off * (uint16_t)(i + 1u));
            ng_sprite_group_set_pos(&strips[i],
                                    (int16_t)(-(scroll & 0xFFu)),
                                    strip_y[i]);
            ng_sprite_group_mark_dirty(&strips[i], NG_SGF_DIRTY_POS);
            ng_sprite_group_flush(&strips[i]);
        }
        tile_off = (uint16_t)(tile_off + 1u);

        if (demo_frame()) break;
    }

    for (i = 0u; i < GPLANE_STRIPS; i++) {
        ng_sprite_group_hide(&strips[i]);
    }
#undef GPLANE_STRIPS
}

/* ------------------------------------------------------------------ */
/*  Public: depth scene                                                  */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_depth_run(void)
{
    demo_clear_scene();
    soundSceneReset();
    soundSetADPCMAVolume(0x3Cu);

    depth_starfield();
    depth_perspective_floor();
    depth_zsort();
    depth_ground_plane();

    soundStopAll();
    demo_clear_scene();
}
