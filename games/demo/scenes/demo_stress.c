/*
 * demo_stress.c — Scene 11: Hardware stress test
 *
 * Spawns maximum sprites + maximum particles simultaneously.
 * Shows the performance HUD overlay on FIX layer.
 * Demonstrates the SDK stays within 60 FPS budget.
 *
 * https://eaglesoftware.biz
 */

#include "demo_stress.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_engine.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include "sdk/2d_engine/ng_particles.h"
#include "sdk/2d_engine/ng_chars.h"
#include "sdk/2d_engine/ng_npcs.h"
#include "sdk/2d_engine/ng_camera.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "sdk/2d_engine/ng_feedback.h"
#include "sdk/2d_engine/ng_debug.h"
#include "sdk/2d_engine/ng_render_queue.h"
#include <stdint.h>

void NEOGEO_USER clearFix(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER playSFX(uint8_t n);

/* NPC data */
#define STRESS_NPC_FIRST_SCREEN 110u
#define STRESS_NPC_SCREEN(n) ((uint8_t)(STRESS_NPC_FIRST_SCREEN + ((uint8_t)(n) % STRESS_NPC_COUNT)))
#define STRESS_NPC_TILE(n)   DEMO_SCREEN_TILE(STRESS_NPC_SCREEN(n))
#define STRESS_NPC_PAL(n)    DEMO_SCREEN_PALETTE(STRESS_NPC_FIRST_SCREEN)
#define STRESS_NPC_STRIPS    12u
#define STRESS_NPC_ROWS      15u
#define STRESS_NPC_STRIDE    16u
#define STRESS_NPC_COUNT     12u  /* use 12 NGCharacter objects */
#define STRESS_NPC_OFFSET_Y (-240)

/* Particle palette */
#define STRESS_PART_PAL    0x40u
#define STRESS_PART_SLOT   80u

/* xorshift for pseudo-random positions */
static uint16_t s_stress_lfsr = 0xBEEFu;
static uint8_t NEOGEO_USER stress_rand8(void)
{
    s_stress_lfsr ^= (uint16_t)(s_stress_lfsr << 7);
    s_stress_lfsr ^= (uint16_t)(s_stress_lfsr >> 9);
    s_stress_lfsr ^= (uint16_t)(s_stress_lfsr << 8);
    return (uint8_t)(s_stress_lfsr & 0xFFu);
}

/* ------------------------------------------------------------------ */
/*  HUD update on FIX (always visible, used as perf overlay)            */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER stress_draw_hud(uint8_t part_count,
                                        uint16_t spr_used,
                                        uint8_t rq_free)
{
    char buf[20];

    /* Particles */
    buf[0] = 'P';
    buf[1] = ':';
    buf[2] = (char)('0' + (part_count / 10u));
    buf[3] = (char)('0' + (part_count % 10u));
    buf[4] = '/';
    buf[5] = '3';
    buf[6] = '2';
    buf[7] = '\0';
    demo_fix_puts(22u, 2u, buf, 2u);

    /* Sprites */
    buf[0] = 'S';
    buf[1] = 'P';
    buf[2] = 'R';
    buf[3] = ':';
    buf[4] = (char)('0' + ((spr_used / 100u) % 10u));
    buf[5] = (char)('0' + ((spr_used / 10u)  % 10u));
    buf[6] = (char)('0' + (spr_used % 10u));
    buf[7] = '\0';
    demo_fix_puts(22u, 3u, buf, 2u);

    /* RQ free */
    buf[0] = 'R';
    buf[1] = 'Q';
    buf[2] = ':';
    buf[3] = (char)('0' + ((rq_free / 100u) % 10u));
    buf[4] = (char)('0' + ((rq_free / 10u)  % 10u));
    buf[5] = (char)('0' + (rq_free % 10u));
    buf[6] = '\0';
    demo_fix_puts(22u, 4u, buf, 1u);
}

/* SFX hook for feedback */
static void NEOGEO_USER stress_sfx_hook(uint16_t id)
{
    if (id < 32u) playSFX((uint8_t)id);
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: full combined scene                                        */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER stress_full_scene(void)
{
    NGCamera cam;
    NGCharacter *player;
    NGNpc *npcs[4];
    NGCharacter *nc;
    uint8_t i;
    uint16_t t;
    static const int16_t npc_sx[4] = { 40, 100, 200, 270 };

    static const uint16_t s_player_pal[16] = {
        0x0000u, 0x7FFFu, 0x4F00u, 0x2422u, 0x3747u, 0x7551u,
        0x7001u, 0x7011u, 0x4e82u, 0x2a82u, 0x5341u, 0x3113u,
        0x1448u, 0x1b55u, 0x6FF0u, 0x30FFu
    };

    clearFix();
    setBACKDROP(BLACK);
    demo_fix_puts(2u, 0u, "FULL SCENE / ALL MODULES LIVE", 2u);
    demo_fix_puts(2u, 1u, "CAM + NPCs + PARTICLES + PULSE + MUSIC", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    ng_feedback_init();
    ng_feedback_set_sfx_hook(stress_sfx_hook);
    ng_particles_init();
    ng_debug_init();

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 640, 0);
    ng_camera_set_follow_speed(&cam, 20u);
    ng_camera_set_dead_zone(&cam, 40u, 0u);

    ng_palfx_pulse(0u, s_player_pal, 60u);

    demo_load_screen_palette(11u);
    demo_load_screen_palette(STRESS_NPC_FIRST_SCREEN);
    ng_chars_init();
    ng_npcs_init();

    player = chars_add(0u, 160, 160);
    if (player) {
        ng_char_set_sprite(player, 0u, STRESS_NPC_STRIPS, STRESS_NPC_ROWS,
                           STRESS_NPC_TILE(0u), STRESS_NPC_PAL(0u));
        ng_char_set_tile_stride(player, STRESS_NPC_STRIDE);
        player->sprite_offset_y = STRESS_NPC_OFFSET_Y;
        player->scale_x = 0xA0u;
        player->scale_y = 0xA0u;
    }

    for (i = 0u; i < 4u; i++) {
        npcs[i] = npc_spawn(0u, 0u, npc_sx[i], 160);
        if (npcs[i]) {
            nc = npc_char(npcs[i]);
            if (nc) {
                ng_char_set_sprite(nc, 0u, STRESS_NPC_STRIPS, STRESS_NPC_ROWS,
                                   STRESS_NPC_TILE((uint8_t)(i * 3u)), STRESS_NPC_PAL((uint8_t)(i * 3u)));
                ng_char_set_tile_stride(nc, STRESS_NPC_STRIDE);
                nc->sprite_offset_y = STRESS_NPC_OFFSET_Y;
                nc->scale_x = 0x70u;
                nc->scale_y = 0x70u;
            }
            ng_npc_set_home(npcs[i], npc_sx[i], 160);
            ng_npc_set_patrol_bounds(npcs[i],
                (int16_t)(npc_sx[i] - 60), (int16_t)(npc_sx[i] + 60), 130, 190);
            ng_npc_set_think(npcs[i], ng_npc_think_patrol, 8u);
        }
    }

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    for (t = 0u; t < 300u; t++) {
        int16_t px = 160;
        if (player) px = player->x;

        px = (int16_t)(160 + (int16_t)((t & 0xFFu) - 128));
        if (player) {
            player->x = px;
            uint8_t anim = (uint8_t)((t / 12u) % 12u);
            uint16_t tile = STRESS_NPC_TILE(anim);
            uint8_t pal = STRESS_NPC_PAL(anim);
            demo_load_screen_palette(STRESS_NPC_FIRST_SCREEN);
            if (player->sprite_tile != tile || player->palette != pal) {
                player->sprite_tile = tile;
                player->palette = pal;
                player->sprite_dirty = 1u;
            }
        }

        ng_camera_update(&cam, px, 160, 0);

        for (i = 0u; i < 4u; i++) {
            if (npcs[i]) {
                nc = npc_char(npcs[i]);
                if (nc) {
                    uint8_t anim = (uint8_t)((t / 12u) % 12u);
                    uint16_t tile = STRESS_NPC_TILE(anim);
                    uint8_t pal = STRESS_NPC_PAL(anim);
                    demo_load_screen_palette(STRESS_NPC_FIRST_SCREEN);
                    if (nc->sprite_tile != tile || nc->palette != pal) {
                        nc->sprite_tile = tile;
                        nc->palette = pal;
                        nc->sprite_dirty = 1u;
                    }
                }
            }
        }

        if ((t % 30u) == 0u) {
            int16_t bx = (int16_t)((stress_rand8() & 0x7Fu) + 80);
            int16_t by = (int16_t)((stress_rand8() & 0x3Fu) + 80);
            ng_spawn_magic_spark(bx, by, 0u, STRESS_PART_PAL);
        }

        ng_npcs_update();
        ng_chars_draw();
        ng_particles_update();
        ng_particles_draw(STRESS_PART_SLOT, STRESS_PART_SLOT);
        ng_palette_fx_update();
        ng_feedback_update();
        ng_debug_draw();

        if (demo_frame()) break;
    }

    ng_palfx_stop(0u);
    ng_chars_init();
    ng_npcs_init();
    ng_particles_init();
    ng_debug_clear();
    soundStopAll();
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: boss encounter                                             */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER stress_boss(void)
{
    NGCharacter *boss;
    NGCamera cam;
    uint8_t  boss_hp;
    uint8_t  player_hp;
    uint16_t t;
    uint8_t  phase2;
    char     bar[14];
    uint8_t  i;

    static const uint16_t s_boss_pal[16] = {
        0x0000u, 0x7FFFu, 0x4F00u, 0x2422u, 0x3747u, 0x7551u,
        0x7001u, 0x7011u, 0x4e82u, 0x2a82u, 0x5341u, 0x3113u,
        0x1448u, 0x1b55u, 0x6FF0u, 0x30FFu
    };

    clearFix();
    setBACKDROP(BLACK);
    demo_fix_puts(2u, 0u, "BOSS ENCOUNTER", 2u);
    demo_fix_puts(2u, 1u, "PHASE 2 AT 50% HP  HITSTOP + SHAKE", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 0, 0);
    ng_camera_snap(&cam, 0, 0);

    ng_feedback_init();
    ng_feedback_set_sfx_hook(stress_sfx_hook);
    ng_particles_init();

    demo_load_screen_palette(STRESS_NPC_FIRST_SCREEN);
    ng_chars_init();

    boss = chars_add(0u, 200, 140);
    if (boss) {
        ng_char_set_sprite(boss, 0u, STRESS_NPC_STRIPS, STRESS_NPC_ROWS,
                           STRESS_NPC_TILE(0u), STRESS_NPC_PAL(0u));
        ng_char_set_tile_stride(boss, STRESS_NPC_STRIDE);
        boss->sprite_offset_y = STRESS_NPC_OFFSET_Y;
        boss->scale_x = 0xD0u;
        boss->scale_y = 0xD0u;
        boss->hp     = 10u;
        boss->max_hp = 10u;
    }

    boss_hp   = 10u;
    player_hp = 10u;
    phase2    = 0u;

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    for (t = 0u; t < 500u; t++) {
        if (boss_hp == 0u) break;

        /* Player attacks boss every 60 frames */
        if ((t % 60u) == 0u && t > 0u && boss && boss_hp > 0u) {
            boss_hp--;
            ng_char_damage(boss, 1u);
            ng_impact_event(NG_IMPACT_HEAVY, STRESS_NPC_PAL(0u), s_boss_pal,
                            &cam, (uint16_t)SOUND_SFX_IMPACT_HIT,
                            200, 110, 0u, STRESS_PART_PAL);
            ng_spawn_hit_spark(200, 110, 0u, STRESS_PART_PAL);

            if (!phase2 && boss_hp <= 5u) {
                phase2 = 1u;
                ng_camera_shake(&cam, 4u, 20u);
                ng_palfx_flash_white(0u, s_boss_pal, 12u);
                demo_fix_puts(14u, 5u, "PHASE 2!", 2u);
            }
        }

        if (boss) {
            uint8_t anim = (uint8_t)((t / 8u) % 12u);
            uint16_t tile = STRESS_NPC_TILE(anim);
            uint8_t pal = STRESS_NPC_PAL(anim);
            demo_load_screen_palette(STRESS_NPC_FIRST_SCREEN);
            if (boss->sprite_tile != tile || boss->palette != pal) {
                boss->sprite_tile = tile;
                boss->palette = pal;
                boss->sprite_dirty = 1u;
            }
        }

        ng_camera_update(&cam, 0, 0, 0);

        if (boss) {
            if (boss->scale_x < 0xD0u) boss->scale_x = 0xD0u;
            ng_chars_draw();
        }

        ng_particles_update();
        ng_particles_draw(STRESS_PART_SLOT, STRESS_PART_SLOT);
        ng_palette_fx_update();
        ng_feedback_update();

        /* Boss HP bar */
        bar[0] = '[';
        for (i = 0u; i < 10u; i++) bar[1u + i] = (i < boss_hp) ? '#' : ' ';
        bar[11] = ']';
        bar[12] = '\0';
        demo_fix_puts(2u,  24u, "BOSS:", 0u);
        demo_fix_puts(8u,  24u, bar, boss_hp > 5u ? 2u : 1u);

        bar[0] = '[';
        for (i = 0u; i < 10u; i++) bar[1u + i] = (i < player_hp) ? '#' : ' ';
        bar[11] = ']';
        bar[12] = '\0';
        demo_fix_puts(2u,  25u, "PLYR:", 0u);
        demo_fix_puts(8u,  25u, bar, 1u);

        if (demo_frame()) break;
    }

    ng_palfx_stop(0u);
    ng_chars_init();
    ng_particles_init();
    soundStopAll();
    demo_clear_scene();
}

/* ------------------------------------------------------------------ */
/*  Public: stress test scene                                            */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_stress_run(void)
{
    NGCharacter *chars[STRESS_NPC_COUNT];
    int16_t cy[STRESS_NPC_COUNT];
    uint8_t i;
    uint16_t t;

    demo_clear_scene();
    soundSceneReset();
    soundSetADPCMAVolume(0x3Cu);

    stress_full_scene();
    stress_boss();

    clearFix();
    demo_fix_puts(2u, 0u, "PERFORMANCE HUD / HARDWARE STRESS",  2u);
    demo_fix_puts(2u, 1u, "MAX SPRITES + MAX PARTICLES  60 FPS", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    /* HUD column labels */
    demo_fix_puts(22u, 1u, "-- PERF HUD --", 2u);
    demo_fix_puts(22u, 6u, "60 FPS TARGET", 0u);

    ng_debug_init();
    ng_particles_init();
    ng_chars_init();

    demo_load_screen_palette(STRESS_NPC_FIRST_SCREEN);
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    /* Populate 12 NPCs at various depth positions */
    for (i = 0u; i < STRESS_NPC_COUNT; i++) {
        uint8_t fn = (uint8_t)((i * 3u) % 12u);
        int16_t sx = (int16_t)(16 + (int16_t)(i * 24));
        cy[i] = (int16_t)(-50 + (int16_t)(i * 10));
        chars[i] = chars_add(0u, sx, cy[i]);
        if (chars[i]) {
            ng_char_set_sprite(chars[i], 0u,
                               STRESS_NPC_STRIPS, STRESS_NPC_ROWS,
                               STRESS_NPC_TILE(fn), STRESS_NPC_PAL(fn));
            ng_char_set_tile_stride(chars[i], STRESS_NPC_STRIDE);
            chars[i]->sprite_offset_y = STRESS_NPC_OFFSET_Y;
            chars[i]->scale_x = 0x70u;
            chars[i]->scale_y = 0x70u;
        }
    }

    for (t = 0u; t < 360u; t++) {
        uint8_t anim = (uint8_t)((t / 12u) % 12u);
        uint16_t tile = STRESS_NPC_TILE(anim);
        uint8_t pal = STRESS_NPC_PAL(anim);
        uint16_t spr_used;
        demo_load_screen_palette(STRESS_NPC_FIRST_SCREEN);

        /* Update NPC positions + animation */
        for (i = 0u; i < STRESS_NPC_COUNT; i++) {
            NGCharacter *c = chars[i];
            if (!c) continue;
            if (c->sprite_tile != tile || c->palette != pal) {
                c->sprite_tile = tile;
                c->palette = pal;
                c->sprite_dirty = 1u;
            }
            cy[i] = (int16_t)(cy[i] - 1);
            if (cy[i] < -50) cy[i] = 60;
            c->y = cy[i];
        }
        ng_chars_draw();

        /* Continuous particle burst */
        if ((t % 6u) == 0u) {
            int16_t px = (int16_t)((stress_rand8() & 0x7Fu) + 80);
            int16_t py = (int16_t)((stress_rand8() & 0x3Fu) + 80);
            ng_spawn_magic_spark(px, py, 0u, STRESS_PART_PAL);
            if ((t % 30u) == 0u) {
                ng_spawn_hit_spark(px, py, 0u, STRESS_PART_PAL);
                ng_spawn_dust(px, (int16_t)(py + 10), 0u, STRESS_PART_PAL);
            }
        }

        ng_particles_update();
        spr_used = (uint16_t)(ng_particles_draw(STRESS_PART_SLOT, STRESS_PART_SLOT) - STRESS_PART_SLOT);

        /* Update HUD */
        stress_draw_hud(ng_particles_count(),
                        spr_used,
                        ng_rq_free());

        ng_debug_draw();

        if ((t % 90u) == 0u) playSFX(SOUND_SFX_STRING_PHRASE);
        if (demo_frame()) break;
    }

    ng_chars_init();
    ng_particles_init();
    ng_debug_clear();
    soundStopAll();
    demo_clear_scene();
}
