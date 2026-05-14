/*
 * demo_title.c — Title screen and end credits
 *
 * Title screen: shown in attract mode + after START_GAME prompt.
 * End card:     credits scroll, branding, loop-back to eyecatcher.
 *
 * https://eaglesoftware.biz
 */

#include "demo_title.h"
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
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playFMTrack(uint8_t n);
void NEOGEO_USER showScreen107(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen108(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);

/* ------------------------------------------------------------------ */
/*  Title screen                                                         */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_title_screen(void)
{
    uint16_t fix_pal[16];
    uint16_t i;

    demo_clear_scene();
    soundSceneReset();
    setBACKDROP(BLACK);

    soundSetADPCMAVolume(0x3Cu);
    soundSetADPCMBVolume(0xB0u);
    soundSetSSGVolume(0x00u);
    soundSetFMVolume(0x00u);

    /* FIX palette 0 = white, 1 = cyan, 2 = yellow */
    setpal(fix_pal, 0x8000u, WHITE,  BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES);

    setpal(fix_pal, 0x8000u, CYAN,   BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES + PALOFFSET);

    setpal(fix_pal, 0x8000u, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES + PALOFFSET * 2u);

    demo_caption("TITLE SCREEN", "NEO GEO SDK DEMO", "SAFE SHOWSCREEN BASE 0040H");
    demo_safe_show(showScreen108, 16, 24, 0xF, 0xAF, 16, BLACK, DEMO_SHOWSCREEN_BASE);
    playSFX(SOUND_SFX_TITLE_GONG);

    if (demo_wait(180u)) {
        demo_clear_scene();
        return;
    }

    /* Second title with PRESS START blink */
    demo_clear_scene();
    demo_caption("TITLE SCREEN", "EAGLE SOFTWARE", "PRESS START");
    demo_safe_show(showScreen107, 16, 24, 0xF, 0xAF, 16, BLACK, DEMO_SHOWSCREEN_BASE);
    playSFX(SOUND_SFX_LOW_DRUM);

    for (i = 0u; i < 180u; i++) {
        waitVbl();
        if (demo_advance_requested()) break;
        if ((i % 60u) < 30u) {
            demo_fix_puts(13u, 26u, "PRESS START", 0u);
        } else {
            demo_fix_puts(13u, 26u, "           ", 0u);
        }
    }

    demo_clear_scene();
}

/* ------------------------------------------------------------------ */
/*  End card / credits                                                   */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_title_end_card(void)
{
    uint16_t fix_pal[16];
    uint16_t i;

    soundStopAll();
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

    playFMTrack(SOUND_FM_VICTORY_JINGLE);

    demo_fix_puts(14u, 4u,  "DEMO COMPLETE", 2u);
    demo_fix_puts(8u,  7u,  "NEO GEO SDK 2D ENGINE",     1u);
    demo_fix_puts(4u,  9u,  "HIGH-PERFORMANCE 2D ENGINE", 0u);
    demo_fix_puts(3u,  11u, "EAGLESOFTWARE.BIZ",          2u);
    demo_fix_puts(1u,  13u, "GITHUB.COM/EAGLESOFTWARE777/NEOGEOSDK", 1u);
    demo_fix_puts(7u,  16u, "PROGRAMMING",  0u);
    demo_fix_puts(10u, 17u, "EAGLESOFTWARE777", 2u);
    demo_fix_puts(6u,  19u, "ALL RIGHTS RESERVED", 0u);
    demo_fix_puts(5u,  21u, "2025  EAGLE SOFTWARE", 1u);

    if (demo_wait(210u)) goto end_done;

    /* Color cycle on title line */
    for (i = 0u; i < 120u; i++) {
        demo_fix_puts(14u, 4u, "DEMO COMPLETE",
                      (uint8_t)((i >> 4) % 3u));
        waitVbl();
        if (demo_advance_requested()) break;
    }

end_done:
    soundFadeOutSpeed(6u);
    demo_wait(45u);
    soundStopAll();
    demo_clear_scene();
}
