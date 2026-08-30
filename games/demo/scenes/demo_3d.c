#include "demo_3d.h"
#include "demo_screen.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include <stdint.h>

void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundCancelFade(void);
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER waitVbl(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);

static void NEOGEO_USER demo_vbl_delay(uint8_t frames)
{
    uint8_t i;
    for (i = 0; i < frames; i++) waitVbl();
}

static void NEOGEO_USER demo_3d_audio(uint8_t music)
{
    soundCancelFade();
    soundStopAll();
    demo_vbl_delay(6);
    soundSceneReset();
    demo_vbl_delay(8);
    soundCancelFade();
    soundPlayGameLoop(music);
    demo_vbl_delay(4);
}

static void NEOGEO_USER demo_blank_rows(uint8_t y0, uint8_t y1)
{
    uint8_t y;
    for (y = y0; y <= y1; y++) {
        demo_fix_puts(1, y, "                                      ", 0);
    }
}

static void NEOGEO_USER demo_center_line(uint8_t y, uint8_t width, char ch, uint8_t pal)
{
    char line[39];
    uint8_t i;
    uint8_t x;

    if (width > 38u) width = 38u;
    for (i = 0; i < 38u; i++) line[i] = ' ';
    line[38] = '\0';

    x = (uint8_t)(19u - (width >> 1));
    for (i = 0; i < width && (uint8_t)(x + i) < 38u; i++) {
        line[x + i] = ch;
    }
    demo_fix_puts(1, y, line, pal);
}

static void NEOGEO_USER demo_perspective_floor(uint16_t frame)
{
    uint8_t phase = (uint8_t)((frame >> 4) & 1u);

    demo_blank_rows(8, 26);
    demo_fix_puts(2, 8, "HORIZON", 2);
    demo_fix_puts(11, 9, "VANISHING POINT", 1);
    demo_center_line(11, 6,  '-', 1);
    demo_center_line((uint8_t)(13u + phase), 12, '-', 1);
    demo_center_line(16, 20, '=', 0);
    demo_center_line((uint8_t)(19u + phase), 28, '=', 0);
    demo_center_line(23, 38, '=', 1);

    demo_fix_puts(7, 12, "/", 1);
    demo_fix_puts(31, 12, "\\", 1);
    demo_fix_puts(5, 17, "/", 1);
    demo_fix_puts(33, 17, "\\", 1);
    demo_fix_puts(3, 23, "/", 1);
    demo_fix_puts(35, 23, "\\", 1);
}

void NEOGEO_USER showPseudo3DLoop(void)
{
    uint16_t frame;

    demo_clear_scene();
    setBACKDROP(DEMO_BG);
    demo_scene_caption("OPTICAL 3D IDEA", "FIX FLOOR ONLY / NO SCB SHRINK", "STABLE PREVIEW - NO SPRITE FLICKER");
    demo_3d_audio(SOUND_MUSIC_A);
    playSFX(SOUND_SFX_9);

    for (frame = 0; frame < 360u; frame++) {
        demo_clear_all_sprites();
        demo_fix_puts(2, 1, "OPTICAL 3D IDEA", 2);
        demo_fix_puts(2, 3, "2D LINES CREATE DEPTH ILLUSION", 0);
        demo_fix_puts(2, 5, "NO SCB SHRINK IN THIS SCENE", 1);
        demo_fix_puts(2, 27, "A: NEXT   START: GAME FLOW ONLY", 1);
        demo_perspective_floor(frame);
        if ((frame % 120u) == 0u) playSFX(SOUND_SFX_10);
        if (demo_wait_frames_or_a(1)) break;
    }

    soundStopAll();
    demo_clear_scene();
}

static void NEOGEO_USER demo_depth_columns(uint16_t frame)
{
    uint8_t i;
    uint8_t shift = (uint8_t)((frame >> 5) & 1u);

    demo_blank_rows(8, 26);
    demo_fix_puts(2, 8, "DEPTH SLICES DESIGN", 2);
    demo_fix_puts(2, 10, "FAR SMALL -> NEAR LARGE", 1);

    for (i = 0; i < 7u; i++) {
        uint8_t y = (uint8_t)(12u + i * 2u);
        uint8_t w = (uint8_t)(6u + i * 5u + shift);
        if (w > 38u) w = 38u;
        demo_center_line(y, w, (i < 2u) ? '-' : '=', (uint8_t)(i & 1u));
    }
}

void NEOGEO_USER show3DRaycaster(void)
{
    uint16_t frame;

    demo_clear_scene();
    setBACKDROP(DEMO_BG);
    demo_scene_caption("DEPTH COLUMN IDEA", "DESIGN PREVIEW ONLY", "NO SPRITE WALLS / NO SHRINK YET");
    demo_3d_audio(SOUND_MUSIC_B);
    playSFX(SOUND_SFX_3);

    for (frame = 0; frame < 300u; frame++) {
        demo_clear_all_sprites();
        demo_fix_puts(2, 1, "DEPTH COLUMN IDEA", 2);
        demo_fix_puts(2, 3, "READABLE LINES - NO GARBAGE", 0);
        demo_fix_puts(2, 5, "FUTURE WALLS NEED STABLE SLOTS", 1);
        demo_fix_puts(2, 27, "A: NEXT   START: GAME FLOW ONLY", 1);
        demo_depth_columns(frame);
        if (demo_wait_frames_or_a(1)) break;
    }

    soundStopAll();
    demo_clear_scene();
}

void NEOGEO_USER demo_3d_showcase(void)
{
    showPseudo3DLoop();
    show3DRaycaster();
    demo_clear_scene();
}
