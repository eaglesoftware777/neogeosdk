/*
 * demo_intro.c — Scene 1: Eagle Software cinematic intro
 *                Scene 2: NEO GEO SDK title card
 *
 * https://eaglesoftware.biz
 */

#include "demo_intro.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include <stdint.h>

void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER clearSprs(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);
void NEOGEO_USER load_palettes(uint16_t *p_palette, uintptr_t palette_offset);
void NEOGEO_USER setpal(uint16_t *pal_tile,
    uint16_t t0, uint16_t t1, uint16_t t2, uint16_t t3,
    uint16_t t4, uint16_t t5, uint16_t t6, uint16_t t7,
    uint16_t t8, uint16_t t9, uint16_t t10, uint16_t t11,
    uint16_t t12, uint16_t t13, uint16_t t14, uint16_t t15);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundFadeOutSpeed(uint8_t speed);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER soundSetSSGVolume(uint8_t v);
void NEOGEO_USER soundSetFMVolume(uint8_t v);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playSFXB(uint8_t n);
void NEOGEO_USER playFMTrack(uint8_t n);
void NEOGEO_USER playVoiceCue(uint8_t n);
void NEOGEO_USER cyclexms(int ms);
void NEOGEO_USER showScreen107(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen108(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);

#define PAL_WHITE   0
#define PAL_CYAN    1
#define PAL_YELLOW  2
#define PAL_RED     3

/* ------------------------------------------------------------------ */
/*  Scene 1 — Eagle Software intro                                       */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_intro_eagle(void)
{
    uint16_t fix_pal[16];
    uint16_t i;

    demo_clear_scene();
    soundSceneReset();
    soundSetADPCMAVolume(0x3Fu);
    soundSetADPCMBVolume(0xB8u);
    soundSetSSGVolume(0x00u);
    soundSetFMVolume(0x00u);

    setBACKDROP(BLACK);

    /* FIX palette 0 = white, 1 = cyan, 2 = yellow, 3 = red */
    setpal(fix_pal, 0x8000u, WHITE,  BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES);

    setpal(fix_pal, 0x8000u, CYAN,   BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES + PALOFFSET);

    setpal(fix_pal, 0x8000u, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES + PALOFFSET * 2u);

    setpal(fix_pal, 0x8000u, RED,    BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES + PALOFFSET * 3u);

    /* Flash in the text */
    demo_fix_puts(17u, 13u, "EAGLE",    PAL_WHITE);
    demo_fix_puts(16u, 15u, "SOFTWARE", PAL_CYAN);

    playVoiceCue(SOUND_VOICE_GET_READY);
    cyclexms(400);

    /* Four quick color alternation hits */
    for (i = 0; i < 4u; i++) {
        demo_fix_puts(17u, 13u, "EAGLE",    (uint8_t)((i & 1u) ? PAL_CYAN   : PAL_RED));
        demo_fix_puts(16u, 15u, "SOFTWARE", (uint8_t)((i & 1u) ? PAL_RED    : PAL_CYAN));
        if (i == 0u) playSFX(SOUND_SFX_STRING_PHRASE);
        cyclexms(60);
    }

    /* Logo snap — show Eagle logo screen */
    clearFix();
    clearSprs();
    playSFX(SOUND_SFX_TITLE_GONG);
    demo_safe_show(showScreen107, 16, 24, 0xF, 0xAF, 16, BLACK, DEMO_SHOWSCREEN_BASE);

    if (demo_wait(60u)) goto intro_done;
    if (demo_wait(60u)) goto intro_done;

    soundFadeOutSpeed(6u);
    demo_wait(45u);

intro_done:
    soundStopAll();
    demo_clear_scene();
}

/* ------------------------------------------------------------------ */
/*  Scene 2 — SDK Title Card                                             */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_intro_sdk_title(void)
{
    uint16_t fix_pal[16];
    uint16_t t;

    demo_clear_scene();
    soundSceneReset();
    soundSetADPCMAVolume(0x3Cu);
    soundSetADPCMBVolume(0xB8u);
    soundSetSSGVolume(0x00u);
    soundSetFMVolume(0x0Cu);

    setBACKDROP(BLACK);

    setpal(fix_pal, 0x8000u, WHITE,  BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES);

    setpal(fix_pal, 0x8000u, CYAN,   BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES + PALOFFSET);

    setpal(fix_pal, 0x8000u, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES + PALOFFSET * 2u);

    /* Character-by-character scroll in — title line */
    {
        const char *title = "NEO GEO SDK";
        const char *sub1  = "HIGH-PERFORMANCE 2D ENGINE";
        const char *sub2  = "60 FPS  REAL HARDWARE  MAME";
        uint8_t col;

        /* Show title character by character */
        for (col = 0u; title[col] != '\0'; col++) {
            char ch[2];
            ch[0] = title[col];
            ch[1] = '\0';
            demo_fix_puts((uint8_t)(14u + col), 11u, ch, PAL_CYAN);
            waitVbl();
            waitVbl();
            if (demo_advance_requested()) goto title_done;
        }

        playSFX(SOUND_SFX_TITLE_GONG);
        demo_wait(12u);

        demo_fix_puts(7u, 13u, sub1, PAL_WHITE);
        demo_wait(8u);
        demo_fix_puts(6u, 15u, sub2, PAL_YELLOW);
        demo_wait(8u);

        playFMTrack(SOUND_FM_VICTORY_JINGLE);
    }

    demo_fix_puts(2u, 27u, "A: NEXT", 1u);

    /* Pulse the title with color cycling */
    for (t = 0u; t < 150u; t++) {
        uint8_t pal = (uint8_t)((t >> 4) % 3u);
        demo_fix_puts(14u, 11u, "NEO GEO SDK", pal);
        if (demo_frame()) goto title_done;
    }

title_done:
    soundStopAll();
    demo_clear_scene();
}
