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
/*  Public: depth scene                                                  */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_depth_run(void)
{
    demo_clear_scene();
    soundSceneReset();
    soundSetADPCMAVolume(0x3Cu);

    depth_starfield();
    depth_perspective_floor();

    soundStopAll();
    demo_clear_scene();
}
