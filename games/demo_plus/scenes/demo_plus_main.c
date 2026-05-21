/*
 * demo_plus_main.c
 *
 * Minimal showcase for the C++ 2D engine (sdk/2d_engine_plus).
 * Demonstrates the parts of the engine that benefit most from the C++
 * port — RAII scene scopes, member-function-style API, header-only
 * inline accessors — without dragging in the legacy demo scene set.
 *
 * Build: make GAME=demo_plus p1 USE_2D_PLUS=1
 *
 * This is a STARTING POINT — keep it minimal so the build succeeds; add
 * new C++-style chapters here as the engine grows.
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
void NEOGEO_USER playSFX(uint8_t n);
uint16_t NEOGEO_USER poll_joystick(void);

static void NEOGEO_USER puts_fix(uint8_t x, uint8_t y, const char *s, uint8_t pal)
{
    char buf[40];
    uint8_t i;
    for (i = 0u; s[i] && i < 38u; i++) buf[i] = s[i];
    buf[i] = '\0';
    fixtext_out(x, y, buf, (short)pal);
}

/*
 * The demo's main.c defines maingame() which calls demo_run_full_flow().
 * demo_plus does not bring in the legacy demo scene set, so provide a
 * stub that just forwards to demo_plus_main_run().  This keeps the
 * shared main.c link-compatible.
 */
void NEOGEO_USER demo_run_full_flow(void);
void NEOGEO_USER demo_run_attract(void);
void NEOGEO_USER demo_run_full_flow(void) { demo_plus_main_run(); }
void NEOGEO_USER demo_run_attract(void)   { demo_plus_main_run(); }

void NEOGEO_USER demo_plus_main_run(void)
{
    uint16_t t;

    /* ---- Subsystem init ------------------------------------------------ */
    ng_render_queue_init();
    ng_palette_fx_init();
    ng_particles_init();
    ng_feedback_init();
    ng_chars_init();

    setBACKDROP(0x8002);    /* dim navy backdrop */
    clearFix();

    soundSceneReset();
    soundPlayGameLoop(SOUND_MUSIC_EAGLE_FANFARE);

    /* ---- Static title -------------------------------------------------- */
    puts_fix(8u,  4u, "NEOGEO SDK DEMO PLUS",  2u);
    puts_fix(8u,  5u, "C++14 2D ENGINE BUILD", 1u);

    puts_fix(2u,  8u, "USES sdk/2d_engine_plus ONLY",   1u);
    puts_fix(2u,  9u, "BUILD: make GAME=demo_plus p1",  0u);
    puts_fix(2u, 10u, "       USE_2D_PLUS=1",           0u);

    puts_fix(2u, 13u, "ENGINE FEATURES WIRED UP:",      2u);
    puts_fix(4u, 14u, "RENDER QUEUE   PALETTE FX",      1u);
    puts_fix(4u, 15u, "PARTICLES      FEEDBACK",        1u);
    puts_fix(4u, 16u, "CHARS / NPCS   CAMERA",          1u);
    puts_fix(4u, 17u, "PHYSICS        DEPTH FX",        1u);

    puts_fix(2u, 20u, "SCENE STUB --- ADD MORE HERE",   2u);
    puts_fix(2u, 21u, "EAGLESOFTWARE.BIZ",              0u);
    puts_fix(2u, 27u, "A: EXIT",                        1u);

    /* ---- Frame loop with full engine pump ------------------------------ */
    for (t = 0u; ; t++) {
        uint16_t joy;

        /* Per-frame engine updates */
        ng_palette_fx_update();
        ng_particles_update();
        ng_feedback_update();

        waitVbl();
        ng_render_queue_flush();

        joy = poll_joystick();
        if (joy & BUTTON_A) break;

        if ((t & 0x3Fu) == 0u) playSFX(SOUND_SFX_FOOTSTEP);
    }

    soundStopAll();
    clearFix();
}
