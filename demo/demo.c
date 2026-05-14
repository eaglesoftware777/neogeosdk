#include "demo.h"
#include "demo_screen.h"
#include "demo_sound.h"
#include "demo_fix.h"
#include "demo_2d_engine.h"
#include "demo_3d.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include <stdint.h>

void NEOGEO_USER showEyeCatcherMVS(void);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER soundSetSSGVolume(uint8_t v);
void NEOGEO_USER soundSetFMVolume(uint8_t v);
void NEOGEO_USER soundFadeOutSpeed(uint8_t speed);
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playSFXB(uint8_t n);
void NEOGEO_USER playVoiceCue(uint8_t n);
void NEOGEO_USER setpal(uint16_t *pal_tile, uint16_t t0, uint16_t t1, uint16_t t2, uint16_t t3, uint16_t t4, uint16_t t5, uint16_t t6, uint16_t t7, uint16_t t8, uint16_t t9, uint16_t t10, uint16_t t11, uint16_t t12, uint16_t t13, uint16_t t14, uint16_t t15);
void NEOGEO_USER load_palettes(uint16_t *p_palette, uintptr_t palette_offset);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER clearSprs(void);
void NEOGEO_USER cycle1s(void);
void NEOGEO_USER cyclexms(int cycxms);
void NEOGEO_USER waitVbl(void);
void NEOGEO_USER fixtext_out(uint16_t x, uint16_t y, char *mess, short pal);

#define PAL_BLUE 1
#define PAL_RED  2

void NEOGEO_USER showGameOver(void)
{
    soundStopAll();
    demo_clear_scene();
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_ENDING_SCENE);
    fixtext_out(14, 10, "DEMO COMPLETE", 0);
    fixtext_out(8,  13, "NEO GEO SDK 2D ENGINE", 2);
    demo_wait_frames_or_a(210);
    soundFadeOutSpeed(6);
    demo_wait_frames_or_a(45);
    soundStopAll();
    demo_clear_scene();
}

void NEOGEO_USER showEagleIntro(void)
{
    uint16_t fix_pal[16];
    int i;

    clearFix();
    clearSprs();
    soundSceneReset();
    soundSetADPCMAVolume(0x3F);
    soundSetADPCMBVolume(0xB8);
    soundSetSSGVolume(0x00);
    soundSetFMVolume(0x00);

    setpal(fix_pal, 0x8000, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES);
    setpal(fix_pal, 0x8000, CYAN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES + PALOFFSET);
    setpal(fix_pal, 0x8000, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES + PALOFFSET * 2);

    /* Instant full reveal */
    fixtext_out(17, 13, "EAGLE", 0);
    fixtext_out(16, 15, "SOFTWARE", 1);
    playVoiceCue(SOUND_VOICE_GET_READY);
    cyclexms(400);

    /* Four quick color hits */
    for (i = 0; i < 4; i++) {
        fixtext_out(17, 13, "EAGLE",    (short)((i & 1) ? PAL_BLUE : PAL_RED));
        fixtext_out(16, 15, "SOFTWARE", (short)((i & 1) ? PAL_RED  : PAL_BLUE));
        if (i == 0) playSFX(SOUND_SFX_STRING_PHRASE);
        cyclexms(60);
    }

    /* Logo snap */
    clearFix();
    clearSprs();
    playSFX(SOUND_SFX_TITLE_GONG);
    demo_safe_show(showScreen107, 16, 24, 0xF, 0xAF, 16, BLACK, DEMO_SHOWSCREEN_BASE);
    cycle1s();

    soundFadeOutSpeed(6);
    cyclexms(120);
    soundStopAll();
    demo_clear_scene();
}

void NEOGEO_USER showCharacterParade(void)
{
    uint8_t i;
    static const DemoShowScreenFn frames[] = {
        showScreen79, showScreen80, showScreen81, showScreen82, showScreen83,
        showScreen84, showScreen85, showScreen86, showScreen87, showScreen88,
        showScreen89, showScreen90, showScreen91, showScreen92
    };

    demo_clear_scene();
    soundSceneReset();
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);
    demo_scene_caption("MASCOT PARADE", "EYECATCHER ANIMATION FRAMES", "SLOW FRAME TIMING");
    playVoiceCue(SOUND_VOICE_GET_READY);

    for (i = 0; i < (uint8_t)(sizeof(frames) / sizeof(frames[0])); i++) {
        /* No sprite clear inside the loop: palette-only frames (90-92) rely on
         * the previous frame's sprite tile data remaining in VRAM. */
        demo_scene_caption("MASCOT PARADE", "EYECATCHER ANIMATION FRAMES", "SLOW FRAME TIMING");
        demo_safe_show(frames[i], 72, 62, 0xF, 0xAF, 10, BLACK, DEMO_SHOWSCREEN_BASE);
        if ((i & 3u) == 0u) playSFX(SOUND_SFX_STRING_PHRASE);
        if (demo_wait_frames_or_a(28)) break;
    }

    soundStopAll();
    demo_clear_scene();
}

static void NEOGEO_USER demo_show_mvs_eyecatcher_block(void)
{
#ifndef NG_AES
    demo_clear_scene();
    demo_scene_caption("MVS EYECATCHER", "CABINET BOOT STYLE", "CENTERED / PACED");
    playSFX(SOUND_SFX_TITLE_GONG);
    demo_wait_frames_or_a(45);
    showEyeCatcherMVS();
    demo_clear_scene();
#endif
}

void NEOGEO_USER demo_run_attract(void)
{
    /* Attract mode is deliberately short.  Full tech showcase starts only
       after START_GAME, avoiding boring long loops before coin/start. */
    soundSceneReset();
    soundSetADPCMAVolume(0x00);
    soundSetADPCMBVolume(0xBC);
    playSFXB(SOUND_BED_EYECATCHER);
    showEyeCatcherMVS();
    soundStopAll();
    showTitleScreen();
}

void NEOGEO_USER demo_run_full_flow(void)
{
    showEagleIntro();

    soundSceneReset();
    soundSetADPCMAVolume(0x3C);
    soundSetADPCMBVolume(0xB8);
    soundSetSSGVolume(0x08);
    soundSetFMVolume(0x08);

    demo_screen_showcase();
    demo_sound_showcase();
    demo_fix_showcase();
    demo_2d_engine_run();
    demo_3d_showcase();
    demo_show_mvs_eyecatcher_block();
    showCharacterParade();
    demo_2d_engine_advanced_animation();
    showGameOver();
}
