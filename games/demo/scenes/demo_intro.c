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
#include "sdk/2d_engine/ng_progress.h"
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
#define PAL_BLUE    4 

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

    setpal(fix_pal, 0x8000u, BLUE,   BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
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
    demo_fix_puts(16u, 15u, "SOFTWARE", PAL_WHITE);

    playVoiceCue(SOUND_VOICE_1);
    cyclexms(400);

    /* Four quick color alternation hits */
    for (i = 0; i < 4u; i++) {
        demo_fix_puts(17u, 13u, "EAGLE",    (uint8_t)((i & 1u) ? PAL_BLUE  : PAL_RED));
        demo_fix_puts(16u, 15u, "SOFTWARE", (uint8_t)((i & 1u) ? PAL_RED    : PAL_BLUE));
        if (i == 0u) playSFX(SOUND_SFX_9);
        cyclexms(60);
    }

    /* Logo snap — show Eagle logo screen */
    clearFix();
    clearSprs();
    playSFX(SOUND_SFX_3);
    demo_safe_show(showScreen107, 32, 24, 0xF, 0xAF, 16, BLACK, DEMO_SHOWSCREEN_BASE);

    if (demo_wait(60u)) goto intro_done;
    if (demo_wait(60u)) goto intro_done;

    soundFadeOutSpeed(6u);
    demo_wait(45u);

intro_done:
    soundStopAll();
    demo_clear_scene();
}

/* ------------------------------------------------------------------ */
/*  Scene 0.2 — System Banner                                           */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_intro_system_banner(void)
{
    demo_clear_scene();
    setBACKDROP(BLACK);

    {
        uint16_t fix_pal[16];
        setpal(fix_pal, 0x8000u, WHITE,  BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
               BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
        load_palettes(fix_pal, PALETTES);
        setpal(fix_pal, 0x8000u, CYAN,   BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
               BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
        load_palettes(fix_pal, PALETTES + PALOFFSET);
        setpal(fix_pal, 0x8000u, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
               BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
        load_palettes(fix_pal, PALETTES + PALOFFSET * 2u);
    }

#ifdef NG_AES
    demo_fix_puts(10u, 9u,  "AES CONSOLE MODE", 2u);
#else
    demo_fix_puts(10u, 9u,  "MVS ARCADE MODE",  2u);
#endif
    demo_fix_puts(10u, 11u, "ROM: NEOGEOSDK V1.3.0",  1u);
    demo_fix_puts(4u,  15u, "CPU: MC68000 / SOUND: YM2610", 0u);
    demo_fix_puts(2u,  27u, "A: NEXT", 1u);

    demo_wait(175u);
    demo_clear_scene();
}

/* ------------------------------------------------------------------ */
/*  Scene 7.0 — Loading scene                                           */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_intro_loading(void)
{
    static const char *const s_labels[8] = {
        "SPRITE BANKS",
        "PALETTE DATA",
        "FIX TILESET",
        "SOUND SAMPLES",
        "FM PATCHES",
        "LEVEL DATA",
        "PHYSICS WORLD",
        "PARTICLE POOL"
    };
    uint8_t i;
    char bar[14];

    demo_clear_scene();
    setBACKDROP(BLACK);

    {
        uint16_t fix_pal[16];
        setpal(fix_pal, 0x8000u, WHITE,  BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
               BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
        load_palettes(fix_pal, PALETTES);
        setpal(fix_pal, 0x8000u, CYAN,   BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
               BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
        load_palettes(fix_pal, PALETTES + PALOFFSET);
        setpal(fix_pal, 0x8000u, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
               BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
        load_palettes(fix_pal, PALETTES + PALOFFSET * 2u);
    }

    demo_fix_puts(10u, 5u, "LOADING ASSETS...", 2u);
    demo_fix_puts(8u,  7u, "[          ]  0/8",  1u);

    ng_progress_start(0u, 8u);

    for (i = 0u; i < 8u; i++) {
        uint8_t pct;
        uint8_t filled;
        uint8_t j;

        ng_progress_add(0u, 1u);
        pct    = ng_progress_percent(0u);
        filled = (uint8_t)(pct / 10u);
        if (filled > 10u) filled = 10u;

        bar[0]  = '[';
        for (j = 0u; j < 10u; j++) {
            bar[1u + j] = (j < filled) ? '#' : ' ';
        }
        bar[11] = ']';
        bar[12] = ' ';
        bar[13] = '\0';
        demo_fix_puts(8u, 7u, bar, 1u);

        {
            char num[6];
            num[0] = (char)('0' + (uint8_t)(i + 1u));
            num[1] = '/';
            num[2] = '8';
            num[3] = '\0';
            demo_fix_puts(22u, 7u, num, 1u);
        }

        demo_fix_puts(4u, 9u, "                            ", 0u);
        demo_fix_puts(4u, 9u, s_labels[i], 0u);

        demo_wait(18u);
    }

    demo_fix_puts(10u, 12u, "READY!", 2u);
    demo_wait(60u);
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
	
	   setpal(fix_pal, 0x8000u, RED,   BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES + PALOFFSET * 3u);
	
		   setpal(fix_pal, 0x8000u, BLUE,   BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES + PALOFFSET * 4u);


    /* Character-by-character scroll in — title line */
    {
        const char *title = "NEO GEO SDK V1.3.0";
        const char *sub1  = "POWERED BY 2D ENGINE";
        const char *sub2  = "60 FPS  REAL HARDWARE + MAME";
        uint8_t col;

        /* Show title character by character */
        for (col = 0u; title[col] != '\0'; col++) {
            char ch[2];
            ch[0] = title[col];
            ch[1] = '\0';
            demo_fix_puts((uint8_t)(14u + col), 11u, ch, PAL_BLUE);
            waitVbl();
            waitVbl();
            if (demo_advance_requested()) goto title_done;
        }

        playSFX(SOUND_SFX_10);
        demo_wait(12u);

        demo_fix_puts(7u, 13u, sub1, PAL_RED);
        demo_wait(8u);
        demo_fix_puts(6u, 15u, sub2, PAL_WHITE);
        demo_wait(8u);

        playSFX(SOUND_SFX_9);
    }

    demo_fix_puts(2u, 27u, "A: NEXT", 1u);

    /* Pulse the title with color cycling */
    for (t = 0u; t < 150u; t++) {
        uint8_t pal = (uint8_t)((t >> 4) % 3u);
        demo_fix_puts(14u, 11u, "NEO GEO SDK V1.3.0", pal);
        if (demo_frame()) goto title_done;
    }

title_done:
     demo_wait(170u);
    soundStopAll();
    demo_clear_scene();
}
