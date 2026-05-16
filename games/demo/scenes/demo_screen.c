#include "demo_screen.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include <stdint.h>

void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER soundSetSSGVolume(uint8_t v);
void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playVoiceCue(uint8_t n);

/* demo_wait_frames_or_a — thin VBlank-based wait, no render-queue flush.
 * Distinct from demo_wait() in demo.c which calls the full demo_frame pump. */
uint8_t NEOGEO_USER demo_wait_frames_or_a(uint16_t frames)
{
    uint16_t i;
    for (i = 0; i < frames; i++) {
        waitVbl();
        if (demo_advance_requested()) {
            waitVbl();
            return 1;
        }
    }
    return 0;
}

/* demo_scene_caption — three-line caption with a fixed "A: NEXT" footer.
 * Different signature from demo_caption() in demo.c (title + 2 body lines). */
void NEOGEO_USER demo_scene_caption(const char *title, const char *line1, const char *line2)
{
    clearFix();
    if (title) demo_fix_puts(2, 1, title, 2);
    if (line1) demo_fix_puts(2, 3, line1, 0);
    if (line2) demo_fix_puts(2, 5, line2, 1);
    demo_fix_puts(2, 27, "A: NEXT", 1);
}


static void NEOGEO_USER screen_walk_showcase(int loops, int delay_frames)
{
    int i;
    uint8_t frame;

    demo_clear_scene();
    soundSceneReset();
    soundSetADPCMAVolume(0x38);
    demo_scene_caption("SPRITE TILE PREVIEW", "CENTERED NGSpriteGroup DRAW", "NO PER-FRAME HARD CLEAR");
    playVoiceCue(SOUND_VOICE_GET_READY);

    frame = 0;
    for (i = 0; i < loops; i++) {
        demo_draw_sprite_screen((uint8_t)(2u + frame), 1, 20, -34, 16, 16, 0xFF, 0xFF);
        if ((frame & 3u) == 0u) playSFX(SOUND_SFX_FOOTSTEP);
        if (demo_wait_frames_or_a((uint16_t)(delay_frames > 0 ? delay_frames : 24))) break;
        frame++;
        if (frame >= 7) frame = 0;
    }

    demo_clear_scene();
}

static void NEOGEO_USER screen_title_showcase(void)
{
    int i;

    demo_clear_scene();
    soundSceneReset();
    setBACKDROP(BLACK);
    soundSetADPCMAVolume(0x3C);
    soundSetADPCMBVolume(0xB0);
    soundSetSSGVolume(0x00);

    demo_scene_caption("TITLE SCREEN", "NEO GEO SDK DEMO", "SAFE SHOWSCREEN BASE 0040H");
    demo_safe_show(showScreen108, 32, 24, 0xF, 0xAF, 16, BLACK, DEMO_SHOWSCREEN_BASE);
    playSFX(SOUND_SFX_TITLE_GONG);
    if (demo_wait_frames_or_a(180)) {
        demo_clear_scene();
        return;
    }

    demo_clear_scene();
    demo_scene_caption("TITLE SCREEN", "EAGLE SOFTWARE FALLBACK", "SECOND TITLE ASSET BYPASSED");
    demo_safe_show(showScreen107, 32, 24, 0xF, 0xAF, 16, BLACK, DEMO_SHOWSCREEN_BASE);
    playSFX(SOUND_SFX_LOW_DRUM);

    for (i = 0; i < 180; i++) {
        waitVbl();
        if (demo_advance_requested()) break;
        if ((i % 60) < 30) demo_fix_puts(13, 26, "PRESS START", 0);
        else demo_fix_puts(13, 26, "           ", 0);
    }

    demo_clear_scene();
}

void NEOGEO_USER demo_screen_showcase(void)
{
    screen_title_showcase();
    screen_walk_showcase(18, 30);
}
