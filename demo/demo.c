#include "demo.h"
#include "demo_screen.h"
#include "demo_sound.h"
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
void NEOGEO_USER soundFadeOutSpeed(uint8_t speed);
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playVoiceCue(uint8_t n);
void NEOGEO_USER setpal(uint16_t *pal_tile, uint16_t t0, uint16_t t1, uint16_t t2, uint16_t t3, uint16_t t4, uint16_t t5, uint16_t t6, uint16_t t7, uint16_t t8, uint16_t t9, uint16_t t10, uint16_t t11, uint16_t t12, uint16_t t13, uint16_t t14, uint16_t t15);
void NEOGEO_USER load_palettes(uint16_t *p_palette, uintptr_t palette_offset);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER clearSprs(void);
void NEOGEO_USER cycle1s(void);
void NEOGEO_USER cyclexs(int cyc1xs);
void NEOGEO_USER cyclexms(int cycxms);
void NEOGEO_USER waitVbl(void);
void NEOGEO_USER fixtext_out(uint16_t x, uint16_t y, char *mess, short pal);

#define PAL_BLUE 1
#define PAL_RED  2

void NEOGEO_USER showGameOver(void) {
    soundStopAll();
    clearFix();
    clearSprs();
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_ENDING_SCENE);
    fixtext_out(15, 10, "GAME OVER", 0);
    cyclexs(4);
    soundStopAll();
}

/* Boot intro: typewriter "EAGLE SOFTWARE" with voice cues, then Eagle Soft logo. */

void NEOGEO_USER showEagleIntro(void) {
    static const char eagle_word[] = "EAGLE";
    static const char soft_word[] = "SOFTWARE";
    uint16_t fix_pal[16];
    char ch[2];
    int i;

    clearFix();
    clearSprs();
    soundSceneReset();
    soundSetADPCMAVolume(0x3F);
    soundSetADPCMBVolume(0xB8);
    soundSetSSGVolume(0x00);

    /* FIX palettes: white/cyan/yellow text on black for typewriter and shimmer. */
    setpal(fix_pal, 0x8000, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES);
    setpal(fix_pal, 0x8000, CYAN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES + PALOFFSET);
    setpal(fix_pal, 0x8000, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES + PALOFFSET * 2);

    /* Eagle fanfare boot melody */
   /* soundPlayTitleMusic(0);*/

    ch[1] = '\0';

    /* "Get Ready" voice + typewriter "EAGLE" — centered at tile col 17 row 13 */
    playVoiceCue(SOUND_VOICE_GET_READY);
    for (i = 0; i < 5; i++) {
        ch[0] = eagle_word[i];
        fixtext_out(17 + i, 13, ch, 0);
        cyclexms(70);
    }

    cyclexms(20);

    /* "Attack" voice + typewriter "SOFTWARE" — centered at tile col 16 row 15 */
    playVoiceCue(SOUND_VOICE_ATTACK);
    for (i = 0; i < 8; i++) {
        ch[0] = soft_word[i];
        fixtext_out(16 + i, 15, ch, 0);
        cyclexms(60);
    }

for (i = 0; i < 18; i++) {
    short eaglePal    = (short)((i & 1) ? PAL_RED  : PAL_BLUE);
    short softwarePal = (short)((i & 1) ? PAL_BLUE : PAL_RED);

    fixtext_out(17, 13, "EAGLE", eaglePal);
    fixtext_out(16, 15, "SOFTWARE", softwarePal);

    if ((i & 3) == 0) {
        playSFX(SOUND_SFX_STRING_PHRASE);
    }

    cyclexms(35);
}

    /* Eagle Soft title reveal */
    clearFix();
    clearSprs();
    playSFX(SOUND_SFX_TITLE_GONG);
    showScreen107(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
    cycle1s();
    playSFX(SOUND_SFX_LOW_DRUM);
    cycle1s();

    soundFadeOutSpeed(4);
    cyclexms(170);
    soundStopAll();
    clearFix();
    clearSprs();
}

/* Slide through sprite asset screens 2–8 as a character parade. */

void NEOGEO_USER showCharacterParade(void)
{
    uint8_t i;
    static const DemoShowScreenFn frames[] = {
        showScreen79, showScreen80, showScreen81, showScreen82, showScreen83,
        showScreen84, showScreen85, showScreen86, showScreen87, showScreen88,
        showScreen89, showScreen90, showScreen91, showScreen92
    };

    demo_clear_scene();
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);
    demo_scene_caption("EYECATCHER ANIMATION", "MASCOT SPRITE FRAME PARADE", "RAW SHOWSCREEN FRAMES");
    playVoiceCue(SOUND_VOICE_GET_READY);

    for (i = 0; i < (uint8_t)(sizeof(frames) / sizeof(frames[0])); i++) {
        demo_clear_all_sprites();
        demo_scene_caption("EYECATCHER ANIMATION", "MASCOT SPRITE FRAME PARADE", "RAW SHOWSCREEN FRAMES");
        frames[i](72, 70, 0xF, 0xAF, 9, 0x0000, 0);
        if ((i & 3u) == 0u) playSFX(SOUND_SFX_STRING_PHRASE);
        if (demo_wait_frames_or_a(36)) break;
    }

    soundStopAll();
    demo_clear_scene();
}

static void NEOGEO_USER demo_show_mvs_eyecatcher_block(void)
{
#ifndef NG_AES
    demo_clear_scene();
    demo_scene_caption("MVS EYECATCHER", "ORIGINAL BOOT / CABINET STYLE", "SCENE KEPT IN FLOW");
    playSFX(SOUND_SFX_TITLE_GONG);
    demo_wait_frames_or_a(45);
    showEyeCatcherMVS();
    demo_clear_scene();
#endif
}

void NEOGEO_USER demo_run_attract(void)
{
    showEagleIntro();
    demo_screen_showcase();
    demo_sound_showcase();
    demo_2d_engine_run();
    demo_3d_showcase();
    demo_show_mvs_eyecatcher_block();
    showCharacterParade();
    demo_2d_engine_advanced_animation();
}

void NEOGEO_USER demo_run_full_flow(void)
{
    demo_screen_showcase();
    demo_sound_showcase();
    demo_2d_engine_run();
    demo_3d_showcase();
    demo_show_mvs_eyecatcher_block();
    showCharacterParade();
    demo_2d_engine_advanced_animation();
    showGameOver();
}
