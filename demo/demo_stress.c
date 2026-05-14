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
#include "sdk/2d_engine/ng_debug.h"
#include "sdk/2d_engine/ng_render_queue.h"
#include <stdint.h>

void NEOGEO_USER clearFix(void);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER playSFX(uint8_t n);

/* NPC data */
#define STRESS_NPC_TILE(n)   ((uint16_t)(27648u + (uint16_t)((uint8_t)(n) % 12u) * 256u + 165u))
#define STRESS_NPC_PAL       124u
#define STRESS_NPC_STRIPS     6u
#define STRESS_NPC_ROWS       6u
#define STRESS_NPC_STRIDE    16u
#define STRESS_NPC_COUNT     12u  /* use 12 NGCharacter objects */

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

    demo_load_screen_palette(109u);
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
                               STRESS_NPC_TILE(fn), STRESS_NPC_PAL);
            ng_char_set_tile_stride(chars[i], STRESS_NPC_STRIDE);
            chars[i]->sprite_offset_y = -96;
            chars[i]->scale_x = 0x70u;
            chars[i]->scale_y = 0x70u;
        }
    }

    for (t = 0u; t < 360u; t++) {
        uint8_t anim = (uint8_t)((t / 12u) % 12u);
        uint16_t tile = STRESS_NPC_TILE(anim);
        uint16_t spr_used;

        /* Update NPC positions + animation */
        for (i = 0u; i < STRESS_NPC_COUNT; i++) {
            NGCharacter *c = chars[i];
            if (!c) continue;
            if (c->sprite_tile != tile) { c->sprite_tile = tile; c->sprite_dirty = 1u; }
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
        spr_used = ng_particles_draw(STRESS_PART_SLOT, STRESS_PART_SLOT);

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
