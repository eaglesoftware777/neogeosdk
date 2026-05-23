/*
 * demo_palette.c — Scene 5: Palette FX showcase
 *
 * Demonstrates: fade in/out, white flash, red damage flash, blue magic flash,
 * brightness pulse, and color cycle — all via ng_palette_fx.
 *
 * https://eaglesoftware.biz
 */

#include "demo_palette.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_engine.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include <stdint.h>

void NEOGEO_USER clearFix(void);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER playSFX(uint8_t n);

/* Use warrior walk frame 11 for palette demo character */
#define PALDEMO_SCREEN     11u
#define PALDEMO_SLOT        1u

/* Sample palette for the character (built from screen 11 colors) */
static const uint16_t s_char_pal[16] = {
    0x0000u, 0x7FFFu, 0x4F00u, 0x2422u, 0x3747u, 0x7551u,
    0x7001u, 0x7011u, 0x4e82u, 0x2a82u, 0x5341u, 0x3113u,
    0x1448u, 0x1b55u, 0x6FF0u, 0x30FFu
};

/* ------------------------------------------------------------------ */
/*  Draw the demo character and wait with palette FX active              */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER pal_show_char_with_fx(const char *label,
                                              uint8_t fx_type,
                                              uint16_t wait_frames)
{
    uint16_t t;
    uint8_t  pal_slot;

    clearFix();
    demo_fix_puts(2u, 0u, "PALETTE FX SHOWCASE", 2u);
    if (label) demo_fix_puts(2u, 2u, label, 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    /* Draw character */
    demo_draw_sprite_screen(PALDEMO_SCREEN, PALDEMO_SLOT,
                            100, (int16_t)(-34), 16u, 16u, 0xFFu, 0xFFu);

    pal_slot = DEMO_SCREEN_PALETTE(PALDEMO_SCREEN);

    switch (fx_type) {
        case NG_PALFX_FADE_IN:
            ng_palfx_fade_in(pal_slot, s_char_pal, 30u);
            break;
        case NG_PALFX_FADE_OUT:
            ng_palfx_fade_out(pal_slot, s_char_pal, 30u);
            break;
        case NG_PALFX_FLASH_WHITE:
            ng_palfx_flash_white(pal_slot, s_char_pal, 12u);
            break;
        case NG_PALFX_FLASH_RED:
            ng_palfx_flash_red(pal_slot, s_char_pal, 12u);
            break;
        case NG_PALFX_FLASH_BLUE:
            ng_palfx_flash_blue(pal_slot, s_char_pal, 12u);
            break;
        case NG_PALFX_PULSE:
            ng_palfx_pulse(pal_slot, s_char_pal, 30u);
            break;
        case NG_PALFX_CYCLE:
            ng_palfx_cycle(pal_slot, s_char_pal, 1u, 14u);
            break;
        default:
            break;
    }

    for (t = 0u; t < wait_frames; t++) {
        if (demo_frame()) { ng_palfx_stop(pal_slot); return; }
    }
    ng_palfx_stop(pal_slot);
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: rapid flash sequence                                      */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER pal_flash_sequence(void)
{
    uint8_t i;
    uint8_t pal_slot = DEMO_SCREEN_PALETTE(PALDEMO_SCREEN);

    clearFix();
    demo_fix_puts(2u, 0u, "RAPID FLASH SEQUENCE", 2u);
    demo_fix_puts(2u, 1u, "WHITE  RED  BLUE  MULTI-HIT", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    demo_draw_sprite_screen(PALDEMO_SCREEN, PALDEMO_SLOT,
                            100, (int16_t)(-34), 16u, 16u, 0xFFu, 0xFFu);

    for (i = 0u; i < 6u; i++) {
        uint8_t fx;
        /* Cycle through white, red, blue */
        switch (i % 3u) {
            case 0u: fx = NG_PALFX_FLASH_WHITE; break;
            case 1u: fx = NG_PALFX_FLASH_RED;   break;
            default: fx = NG_PALFX_FLASH_BLUE;  break;
        }

        ng_palfx_stop(pal_slot);
        switch (fx) {
            case NG_PALFX_FLASH_WHITE: ng_palfx_flash_white(pal_slot, s_char_pal, 8u); break;
            case NG_PALFX_FLASH_RED:   ng_palfx_flash_red  (pal_slot, s_char_pal, 8u); break;
            default:                   ng_palfx_flash_blue (pal_slot, s_char_pal, 8u); break;
        }

        playSFX(SOUND_SFX_8);
        if (demo_wait(20u)) { ng_palfx_stop(pal_slot); return; }
    }
    ng_palfx_stop(pal_slot);
}

/* ------------------------------------------------------------------ */
/*  Public: palette scene                                                */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_palette_run(void)
{
    demo_clear_scene();
    soundSceneReset();
    soundSetADPCMAVolume(0x3Cu);

    soundPlayGameLoop(SOUND_MUSIC_B);

    /* Fade in */
    pal_show_char_with_fx("FADE IN  FROM BLACK", NG_PALFX_FADE_IN, 50u);

    /* Rapid flash trio */
    pal_flash_sequence();

    /* White flash */
    pal_show_char_with_fx("WHITE HIT FLASH", NG_PALFX_FLASH_WHITE, 40u);

    /* Red damage */
    pal_show_char_with_fx("RED DAMAGE FLASH", NG_PALFX_FLASH_RED, 40u);

    /* Blue magic */
    pal_show_char_with_fx("BLUE MAGIC FLASH", NG_PALFX_FLASH_BLUE, 40u);

    /* Pulse */
    pal_show_char_with_fx("BRIGHTNESS PULSE  LOOPING", NG_PALFX_PULSE, 90u);

    /* Color cycle */
    pal_show_char_with_fx("COLOR CYCLE  PALETTE ROTATION", NG_PALFX_CYCLE, 90u);

    /* Fade out */
    pal_show_char_with_fx("FADE OUT  TO BLACK", NG_PALFX_FADE_OUT, 50u);

    soundStopAll();
    demo_clear_scene();
}
