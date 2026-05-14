/*
 * demo_camera.c — Scene 4: Smooth camera system showcase
 *
 * Demonstrates: follow mode, dead zone, look-ahead, camera shake,
 * cinematic pan, parallax background scrolling.
 *
 * https://eaglesoftware.biz
 */

#include "demo_camera.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_engine.h"
#include "sdk/2d_engine/ng_camera.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include <stdint.h>

void NEOGEO_USER clearFix(void);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER playSFX(uint8_t n);

/* Camera demo uses two sprite groups: a "player" character and a
 * "background" reference bar to show relative parallax. */
#define CAM_PLAYER_SLOT   1u
#define CAM_BG_SLOT       20u

/* Player tile: warrior walk frame 1 (screen 11 = meta[10]) */
#define CAM_PLAYER_TILE        ((uint16_t)((11u - 1u) * 256u))
#define CAM_PLAYER_PALETTE     ((uint8_t)(0x10u + 10u))
#define CAM_PLAYER_STRIPS       6u
#define CAM_PLAYER_ROWS         10u
#define CAM_PLAYER_STRIDE       16u

/* World size — virtual 1024 px wide world */
#define CAM_WORLD_W   1024
#define CAM_WORLD_H    224

/* ------------------------------------------------------------------ */
/*  Sub-scene: follow + dead zone                                        */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER cam_follow_demo(void)
{
    NGCamera cam;
    NGSpriteGroup player;
    int16_t px;           /* player world X */
    int16_t pvx;          /* player velocity X */
    uint16_t t;

    clearFix();
    demo_fix_puts(2u, 0u, "SMOOTH CAMERA / FOLLOW / DEAD ZONE", 2u);
    demo_fix_puts(2u, 1u, "PLAYER MOVES - CAMERA LAGS BEHIND", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    demo_load_screen_palette(11u);

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, CAM_WORLD_W - NG_SCREEN_W, CAM_WORLD_H - NG_SCREEN_H);
    ng_camera_set_follow_speed(&cam, 24u);
    ng_camera_set_dead_zone(&cam, 32u, 24u);
    ng_camera_set_look_ahead(&cam, 40, 0, 4u);

    ng_sprite_group_init(&player, CAM_PLAYER_SLOT,
                         CAM_PLAYER_STRIPS, CAM_PLAYER_ROWS,
                         CAM_PLAYER_TILE, CAM_PLAYER_PALETTE);
    ng_sprite_group_set_tile_stride(&player, CAM_PLAYER_STRIDE);
    ng_sprite_group_set_active_rows(&player, CAM_PLAYER_ROWS);
    ng_sprite_group_set_scale(&player, 0xFFu, 0xFFu);

    px  = 160;
    pvx = 1;

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    for (t = 0u; t < 360u; t++) {
        int16_t screen_x;
        uint8_t anim_frame;

        /* Bounce player across world */
        px = (int16_t)(px + pvx);
        if (px >= CAM_WORLD_W - 32) { pvx = -1; px = (int16_t)(CAM_WORLD_W - 32); }
        if (px < 32)                { pvx =  1; px = 32; }

        /* Camera follow */
        ng_camera_update(&cam, px, 96, pvx);

        /* World-to-screen: player draws at its screen position */
        screen_x = ng_camera_world_to_screen_x(&cam, px);

        /* Advance walk animation */
        anim_frame = (uint8_t)(11u + ((t / 8u) % 9u));
        ng_sprite_group_set_tile_base(&player, DEMO_SCREEN_TILE(anim_frame));
        ng_sprite_group_mark_dirty(&player, NG_SGF_DIRTY_TILE | NG_SGF_DIRTY_POS);
        ng_sprite_group_set_pos(&player, screen_x, (int16_t)(-34));
        ng_sprite_group_flush(&player);

        /* Show camera position on FIX */
        {
            char buf[20];
            uint16_t cx = (uint16_t)(cam.x >= 0 ? cam.x : 0);
            buf[0] = 'C';
            buf[1] = 'A';
            buf[2] = 'M';
            buf[3] = 'X';
            buf[4] = ':';
            buf[5] = (char)('0' + ((cx / 100u) % 10u));
            buf[6] = (char)('0' + ((cx / 10u) % 10u));
            buf[7] = (char)('0' + (cx % 10u));
            buf[8] = '\0';
            demo_fix_puts(2u, 3u, buf, 0u);
        }

        if ((t % 90u) == 0u) playSFX(SOUND_SFX_FOOTSTEP);
        if (demo_frame()) break;
    }

    ng_sprite_group_hide(&player);
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: shake                                                     */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER cam_shake_demo(void)
{
    NGCamera cam;
    NGSpriteGroup player;
    uint16_t t;

    clearFix();
    demo_fix_puts(2u, 0u, "CAMERA SHAKE", 2u);
    demo_fix_puts(2u, 1u, "IMPACT SHAKE  ESCALATING AMPLITUDE", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    demo_load_screen_palette(11u);

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 0, 0);
    ng_camera_snap(&cam, 0, 0);

    ng_sprite_group_init(&player, CAM_PLAYER_SLOT,
                         CAM_PLAYER_STRIPS, CAM_PLAYER_ROWS,
                         CAM_PLAYER_TILE, CAM_PLAYER_PALETTE);
    ng_sprite_group_set_tile_stride(&player, CAM_PLAYER_STRIDE);
    ng_sprite_group_set_active_rows(&player, CAM_PLAYER_ROWS);
    ng_sprite_group_set_scale(&player, 0xFFu, 0xFFu);
    ng_sprite_group_set_pos(&player, 140, (int16_t)(-34));
    ng_sprite_group_upload(&player);

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    for (t = 0u; t < 240u; t++) {
        int16_t sx;
        uint8_t amp;

        /* Trigger shake at intervals with escalating amplitude */
        if ((t % 60u) == 0u) {
            amp = (uint8_t)(2u + (t / 60u) * 2u);
            if (amp > 8u) amp = 8u;
            ng_camera_shake(&cam, amp, 12u);
            playSFX(SOUND_SFX_IMPACT_HIT);
        }

        ng_camera_update(&cam, 0, 0, 0);

        /* Apply shake offset to player sprite X */
        sx = (int16_t)(140 + cam.shake_offset_x);
        ng_sprite_group_set_pos(&player, sx, (int16_t)(-34 + cam.shake_offset_y));
        ng_sprite_group_mark_dirty(&player, NG_SGF_DIRTY_POS);
        ng_sprite_group_flush(&player);

        if (demo_frame()) break;
    }

    ng_sprite_group_hide(&player);
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: parallax layers via alternating sprite groups             */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER cam_parallax_demo(void)
{
    NGSpriteGroup near_layer, far_layer;
    int16_t scroll;
    uint16_t t;

    clearFix();
    demo_fix_puts(2u, 0u, "PARALLAX DEPTH", 2u);
    demo_fix_puts(2u, 1u, "FAR=HALF SPEED  NEAR=FULL SPEED", 1u);
    demo_fix_puts(2u, 3u, "FAR  BG LAYER  MOVES AT 0.5X", 0u);
    demo_fix_puts(2u, 4u, "NEAR FG LAYER  MOVES AT 1.0X", 0u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    /* Far layer: background screen 1 at half scroll speed */
    demo_load_screen_palette(1u);
    ng_sprite_group_init(&far_layer, NG_SPR_BG0_FIRST, 16u, 16u,
                         DEMO_SCREEN_TILE(1u), DEMO_SCREEN_PALETTE(1u));
    ng_sprite_group_set_tile_stride(&far_layer, 16u);
    ng_sprite_group_set_scale(&far_layer, 0xFFu, 0xFFu);

    /* Near layer: background screen 2 at full scroll speed */
    demo_load_screen_palette(2u);
    ng_sprite_group_init(&near_layer, NG_SPR_BG1_FIRST, 16u, 16u,
                         DEMO_SCREEN_TILE(2u), DEMO_SCREEN_PALETTE(2u));
    ng_sprite_group_set_tile_stride(&near_layer, 16u);
    ng_sprite_group_set_scale(&near_layer, 0xFFu, 0xFFu);

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    scroll = 0;
    for (t = 0u; t < 300u; t++) {
        /* Far layer scrolls at half rate */
        ng_sprite_group_set_pos(&far_layer,  (int16_t)(-(scroll >> 1)), (int16_t)0);
        ng_sprite_group_mark_dirty(&far_layer, NG_SGF_DIRTY_POS);
        ng_sprite_group_flush(&far_layer);

        /* Near layer scrolls at full rate */
        ng_sprite_group_set_pos(&near_layer, (int16_t)(-scroll), (int16_t)0);
        ng_sprite_group_mark_dirty(&near_layer, NG_SGF_DIRTY_POS);
        ng_sprite_group_flush(&near_layer);

        scroll++;
        if (scroll > 319) scroll = 0;

        if (demo_frame()) break;
    }

    ng_sprite_group_hide(&far_layer);
    ng_sprite_group_hide(&near_layer);
}

/* ------------------------------------------------------------------ */
/*  Public: camera scene                                                 */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_camera_run(void)
{
    demo_clear_scene();
    soundSceneReset();
    soundSetADPCMAVolume(0x3Cu);

    cam_parallax_demo();
    cam_follow_demo();
    cam_shake_demo();

    soundStopAll();
    demo_clear_scene();
}
