/*
 * demo_particles.c — Scene 6: Particle system showcase
 *
 * Demonstrates: hit sparks, dust, explosions, slash trails, smoke,
 * magic sparks, priority eviction when pool is full.
 *
 * All tile_base values are 0 — the particle system uses its own
 * sprite slots and tiles.  Passing tile_base=0 to spawn helpers
 * uses the default particle tiles already loaded in C ROM.
 *
 * https://eaglesoftware.biz
 */

#include "demo_particles.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_engine.h"
#include "sdk/2d_engine/ng_particles.h"
#include <stdint.h>

void NEOGEO_USER clearFix(void);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER playSFX(uint8_t n);

/* Particle palette: use FX base palette 0x40 */
#define PART_PAL  0x40u

/* Particle first sprite slot (above character range) */
#define PART_SLOT_START   50u

/* ------------------------------------------------------------------ */
/*  Sub-scene: hit sparks burst                                          */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER part_hit_sparks(void)
{
    uint16_t t;
    uint8_t  burst;

    clearFix();
    demo_fix_puts(2u, 0u, "PARTICLE SYSTEM", 2u);
    demo_fix_puts(2u, 1u, "HIT SPARKS  8-WAY BURST", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    ng_particles_init();
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    for (t = 0u; t < 240u; t++) {
        /* Burst every 60 frames */
        if ((t % 60u) == 0u) {
            int16_t cx = (int16_t)(80 + (int16_t)((t >> 2) & 0x3Fu));
            int16_t cy = 112;
            for (burst = 0u; burst < 8u; burst++) {
                ng_spawn_hit_spark(cx, cy, 0u, PART_PAL);
            }
            playSFX(SOUND_SFX_IMPACT_HIT);
        }

        ng_particles_update();
        ng_particles_draw(PART_SLOT_START, 0u);

        if (demo_frame()) break;
    }
    ng_particles_init();
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: dust + smoke                                              */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER part_dust_smoke(void)
{
    uint16_t t;

    clearFix();
    demo_fix_puts(2u, 0u, "PARTICLE SYSTEM", 2u);
    demo_fix_puts(2u, 1u, "DUST + SMOKE  AMBIENT PARTICLES", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    ng_particles_init();
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    for (t = 0u; t < 240u; t++) {
        /* Continuous dust spawns */
        if ((t % 8u) == 0u) {
            int16_t dx = (int16_t)(40 + (int16_t)((t * 3u) & 0xFFu));
            ng_spawn_dust(dx, 160, 0u, PART_PAL);
            if ((t % 24u) == 0u) {
                ng_spawn_smoke((int16_t)(dx + 16), 150, 0u, PART_PAL);
            }
            playSFX(SOUND_SFX_FOOTSTEP);
        }

        ng_particles_update();
        ng_particles_draw(PART_SLOT_START, 0u);

        if (demo_frame()) break;
    }
    ng_particles_init();
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: explosions + slash trails                                 */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER part_explosions_slash(void)
{
    uint16_t t;
    uint8_t  hit;

    clearFix();
    demo_fix_puts(2u, 0u, "PARTICLE SYSTEM", 2u);
    demo_fix_puts(2u, 1u, "EXPLOSIONS + SLASH TRAILS", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    ng_particles_init();
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    for (t = 0u; t < 300u; t++) {
        if ((t % 45u) == 0u) {
            int16_t ex = (int16_t)(60 + (int16_t)((t * 5u) & 0x7Fu));
            int16_t ey = (int16_t)(80 + (int16_t)((t * 3u) & 0x3Fu));

            /* Explosion at center */
            ng_spawn_explosion(ex, ey, 0u, PART_PAL);

            /* Slash trails fanning out */
            for (hit = 0u; hit < 4u; hit++) {
                ng_spawn_slash_trail((int16_t)(ex + (int16_t)(hit * 8)), ey,
                                     (int8_t)((hit & 1u) ? 1 : -1),
                                     0u, PART_PAL);
            }

            /* Spark burst */
            ng_spawn_hit_spark(ex, ey, 0u, PART_PAL);
            ng_spawn_hit_spark((int16_t)(ex + 8), (int16_t)(ey - 4), 0u, PART_PAL);

            playSFX(SOUND_SFX_BLADE_WHOOSH);
        }

        ng_particles_update();
        ng_particles_draw(PART_SLOT_START, 0u);

        if (demo_frame()) break;
    }
    ng_particles_init();
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: magic sparks + priority eviction                          */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER part_magic_eviction(void)
{
    uint16_t t;

    clearFix();
    demo_fix_puts(2u, 0u, "PARTICLE SYSTEM", 2u);
    demo_fix_puts(2u, 1u, "PRIORITY EVICTION  POOL FILL", 1u);
    demo_fix_puts(2u, 3u, "OPTIONAL PARTICLES DROP GRACEFULLY", 0u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    ng_particles_init();
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    for (t = 0u; t < 240u; t++) {
        /* Flood with magic sparks (low priority) + critical hit sparks */
        if ((t % 4u) == 0u) {
            int16_t mx = (int16_t)(160 + (int16_t)((int16_t)(t & 0x3Fu) - 32));
            int16_t my = (int16_t)(112 + (int16_t)((int16_t)(t & 0x1Fu) - 16));
            ng_spawn_magic_spark(mx, my, 0u, PART_PAL);
        }
        /* Critical sparks always succeed */
        if ((t % 30u) == 0u) {
            ng_spawn_hit_spark(160, 112, 0u, PART_PAL);
            playSFX(SOUND_SFX_STRING_PHRASE);
        }

        {
            char cnt[16];
            uint8_t n = ng_particles_count();
            cnt[0] = 'P';
            cnt[1] = 'A';
            cnt[2] = 'R';
            cnt[3] = 'T';
            cnt[4] = ':';
            cnt[5] = (char)('0' + (n / 10u));
            cnt[6] = (char)('0' + (n % 10u));
            cnt[7] = '/';
            cnt[8] = '3';
            cnt[9] = '2';
            cnt[10] = '\0';
            demo_fix_puts(2u, 5u, cnt, 2u);
        }

        ng_particles_update();
        ng_particles_draw(PART_SLOT_START, 0u);

        if (demo_frame()) break;
    }
    ng_particles_init();
}

/* ------------------------------------------------------------------ */
/*  Public: particles scene                                              */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_particles_run(void)
{
    demo_clear_scene();
    soundSceneReset();
    soundSetADPCMAVolume(0x3Cu);

    part_hit_sparks();
    part_dust_smoke();
    part_explosions_slash();
    part_magic_eviction();

    soundStopAll();
    demo_clear_scene();
}
