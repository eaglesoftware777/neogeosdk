/*
 * demo_plus_main.c
 *
 * Smoke-test for the C++ 2D engine (sdk/2d_engine_plus).
 *
 * Three short scenes that link only against sdk/2d_engine_plus
 * (USE_2D_PLUS=1) without using any legacy demo content:
 *
 *   1. TITLE        — animated title card with palette-FX flashes
 *   2. PARTICLES    — emits sparks from the screen centre and lets
 *                     the engine's particle pool age them
 *   3. MARQUEE      — scrolling FIX-layer banner driven entirely
 *                     by per-frame fix_puts updates
 *
 * Press A to advance through the scenes.  Last scene loops until A.
 *
 * Build: make GAME=demo_plus p1 USE_2D_PLUS=1
 */

#include "demo_plus_main.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_engine.h"
#include "sdk/2d_engine/ng_sprite_pool.h"
#include <stdint.h>

void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER setBACKDROP(uint16_t bd);
void NEOGEO_USER fixtext_out(uint16_t x, uint16_t y, char *mess, short pal);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundPlayGameLoop(uint8_t track);
void NEOGEO_USER soundFadeOutSpeed(uint8_t speed);
void NEOGEO_USER playSFX(uint8_t n);
uint16_t NEOGEO_USER poll_joystick(void);

/* The shared games/demo/main.c references demo_run_full_flow /
 * demo_run_attract from games/demo/scenes/demo.c.  demo_plus does
 * not link those scene files, so stub the symbols here. */
void NEOGEO_USER demo_run_full_flow(void);
void NEOGEO_USER demo_run_attract(void);
void NEOGEO_USER demo_run_full_flow(void) { demo_plus_main_run(); }
void NEOGEO_USER demo_run_attract(void)   { demo_plus_main_run(); }

/* ----- helpers ---------------------------------------------------------- */

static void NEOGEO_USER puts_fix(uint8_t x, uint8_t y, const char *s, uint8_t pal)
{
    char buf[40];
    uint8_t i;
    for (i = 0u; s[i] && i < 38u; i++) buf[i] = s[i];
    buf[i] = '\0';
    fixtext_out(x, y, buf, (short)pal);
}

/* Returns 1 when A was pressed since last call (edge-triggered). */
static uint8_t NEOGEO_USER advance_pressed(void)
{
    static uint16_t prev = 0u;
    uint16_t now = poll_joystick();
    uint16_t edge = (uint16_t)(now & (uint16_t)~prev);
    prev = now;
    return (edge & BUTTON_A) ? 1u : 0u;
}

/*
 * One frame of engine pump.  Returns 1 if A was pressed (so the
 * caller can short-circuit out of its scene loop).
 */
static uint8_t NEOGEO_USER step(void)
{
    waitVbl();
    ng_render_queue_flush();
    ng_palette_fx_update();
    ng_particles_update();
    ng_feedback_update();
    return advance_pressed();
}

static void NEOGEO_USER scene_clear(void)
{
    clearFix();
    setBACKDROP(0x8002);
}

/* ----- Scene 1: TITLE --------------------------------------------------- */

static void NEOGEO_USER scene_title(void)
{
    static const uint16_t palfx_base[16] = {
        0x0000,
        0x7000, 0x7800, 0x7C00, 0x7E00, 0x7F00, 0x7F80, 0x7FC0,
        0x7FE0, 0x07E0, 0x03E0, 0x001F, 0x021F, 0x041F, 0x081F, 0x7FFF
    };
    const uint8_t fx_slot = 15u;
    uint16_t t;

    scene_clear();

    puts_fix(8u,  4u, "NEOGEO SDK DEMO PLUS",  2u);
    puts_fix(8u,  5u, "C++14 2D ENGINE BUILD", 1u);
    puts_fix(2u,  8u, "BUILT WITH USE_2D_PLUS=1",       1u);
    puts_fix(2u,  9u, "GAME ID 778                ",   0u);
    puts_fix(2u, 11u, "SCENES IN THIS SMOKE TEST:",    2u);
    puts_fix(4u, 12u, "1. TITLE      (THIS SCREEN)",   1u);
    puts_fix(4u, 13u, "2. PARTICLES  (POOL DEMO)",     1u);
    puts_fix(4u, 14u, "3. MARQUEE    (FIX SCROLL)",    1u);
    puts_fix(2u, 17u, "EAGLESOFTWARE.BIZ",             0u);
    puts_fix(2u, 27u, "A: NEXT SCENE",                 2u);

    ng_palfx_upload_base(fx_slot, palfx_base);

    for (t = 0u; t < 240u; t++) {
        /* Periodic flash on the sandbox palette — visible if you draw
         * anything tagged with palette 15.  For this title we just
         * exercise the API and let the user see palette FX is alive. */
        if (t == 30u)  ng_palfx_flash_white(fx_slot, palfx_base, 18u);
        if (t == 90u)  ng_palfx_flash_red  (fx_slot, palfx_base, 18u);
        if (t == 150u) ng_palfx_pulse      (fx_slot, palfx_base, 20u);

        if (step()) break;
    }
    ng_palfx_stop(fx_slot);
}

/* ----- Scene 2: PARTICLES ----------------------------------------------- */

static void NEOGEO_USER scene_particles(void)
{
    uint16_t t;
    char count_buf[6];
    /*
     * Particles use whatever tile happens to live at NG_SPR_PART_FIRST.
     * The demo_plus build does NOT preload a dedicated spark tile, so
     * the visual is whatever is there.  Treat it as a STRESS / LIFETIME
     * demo: the active count rises, then ages back down to zero.
     */

    scene_clear();
    puts_fix(2u,  2u, "SCENE 2 / 3  PARTICLES",          2u);
    puts_fix(2u,  4u, "POOL FILLS, AGES, EMPTIES.",      1u);
    puts_fix(2u,  5u, "BURSTS EVERY 20 FRAMES.",         0u);
    puts_fix(2u,  7u, "ACTIVE: ",                        1u);
    puts_fix(2u, 27u, "A: NEXT SCENE",                   2u);

    for (t = 0u; t < 360u; t++) {
        if ((t % 20u) == 0u && t < 200u) {
            uint8_t i;
            for (i = 0u; i < 4u; i++) {
                int8_t  vx = (int8_t)((int8_t)i - 2);
                ng_particle_spawn(NG_PART_HIT_SPARK,
                                  NG_PART_PRI_CRITICAL,
                                  160, 112,
                                  ((int32_t)vx) << NG_FP_SHIFT,
                                  -(2L << NG_FP_SHIFT),
                                  30u, 0u, 1u, 1u, 1u);
            }
            playSFX(SOUND_SFX_FOOTSTEP);
        }

        /* Active count display — 3 digits */
        {
            uint16_t n = (uint16_t)ng_particles_count();
            if (n > 999u) n = 999u;
            count_buf[0] = (char)('0' + (n / 100u));
            count_buf[1] = (char)('0' + ((n / 10u) % 10u));
            count_buf[2] = (char)('0' + (n % 10u));
            count_buf[3] = '\0';
            puts_fix(10u, 7u, count_buf, 2u);
        }

        ng_particles_draw(NG_SPR_PART_FIRST, 0u);

        if (step()) break;
    }
}

/* ----- Scene 3: MARQUEE ------------------------------------------------- */

static void NEOGEO_USER scene_marquee(void)
{
    static const char message[] =
        "  *  NEOGEO SDK 2D ENGINE PLUS  *  PRESS A TO RESTART  *  "
        "MADE WITH SDK/2D_ENGINE_PLUS C++14  *  EAGLESOFTWARE.BIZ  *  ";
    const uint16_t msg_len = (uint16_t)(sizeof(message) - 1u);
    const uint16_t MARQUEE_ROW = 14u;
    const uint16_t MARQUEE_W   = 36u;
    uint16_t offset = 0u;
    uint16_t t;

    scene_clear();
    puts_fix(2u,  2u, "SCENE 3 / 3  MARQUEE",            2u);
    puts_fix(2u,  4u, "FIX SCROLLS A LONG STRING",       1u);
    puts_fix(2u,  5u, "ONE CELL PER FRAME, NO TEAR.",    0u);
    puts_fix(2u, 11u, "+----------------------------------+", 2u);
    puts_fix(2u, MARQUEE_ROW + 1u,
             "+----------------------------------+", 2u);
    puts_fix(2u, 18u, "ALL FRAMES RENDERED VIA NG_FIX_PUTS", 1u);
    puts_fix(2u, 27u, "A: BACK TO TITLE",                2u);

    for (t = 0u; ; t++) {
        char row[40];
        uint16_t i;
        for (i = 0u; i < MARQUEE_W; i++) {
            row[i] = message[(offset + i) % msg_len];
        }
        row[MARQUEE_W] = '\0';
        puts_fix(3u, MARQUEE_ROW, row, (uint8_t)(1u + ((t >> 4) & 1u)));
        if ((t & 1u) == 0u) offset = (uint16_t)((offset + 1u) % msg_len);
        if (step()) break;
    }
}

/* ----- Entry ------------------------------------------------------------ */

void NEOGEO_USER demo_plus_main_run(void)
{
    /* Engine init — one-shot for the whole flow */
    ng_render_queue_init();
    ng_palette_fx_init();
    ng_particles_init();
    ng_feedback_init();
    ng_chars_init();

    soundSceneReset();
    soundPlayGameLoop(SOUND_MUSIC_EAGLE_FANFARE);

    /* Loop the scenes forever; A advances through them. */
    for (;;) {
        scene_title();
        scene_particles();
        scene_marquee();
    }
}
