#include "demo_screen.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include <stdint.h>

void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER playVoiceCue(uint8_t n);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playSFXB(uint8_t n);
void NEOGEO_USER cyclexs(int cyc1xs);
void NEOGEO_USER cyclexms(int cycxms);
void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER clearSprs(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);
void NEOGEO_USER fixtext_out(uint16_t x, uint16_t y, char *mess, short pal);
void NEOGEO_USER vram_SCB234(uint16_t SCBADDR, uint16_t SCB234);

uint8_t NEOGEO_USER demo_advance_requested(void)
{
    uint8_t p1_change;
    uint8_t stat_change;

    p1_change = NEO_REGISTER8(BIOS_P1CHANGE);
    stat_change = NEO_REGISTER8(BIOS_STATCHANGE);

    if (p1_change & (uint8_t)(1u << CNT_A)) return 1;
    if (stat_change & (uint8_t)(1u << CNT_START1)) return 1;
    if (NEO_REGISTER8(BIOS_START_FLAG) & 1u) return 1;
    return 0;
}

uint8_t NEOGEO_USER demo_wait_frames_or_a(uint16_t frames)
{
    uint16_t i;

    for (i = 0; i < frames; i++) {
        waitVbl();
        if (demo_advance_requested()) {
            /* Debounce the manual advance so one press does not skip all scenes. */
            waitVbl();
            return 1;
        }
    }
    return 0;
}

void NEOGEO_USER demo_clear_all_sprites(void)
{
    uint16_t sprite;

    clearSprs();
    for (sprite = 0; sprite < 384; sprite++) {
        vram_SCB234((uint16_t)(SCB3_ADDR + sprite), 0);
    }
}

void NEOGEO_USER demo_clear_scene(void)
{
    soundStopAll();
    clearFix();
    demo_clear_all_sprites();
    setBACKDROP(BLACK);
    waitVbl();
    demo_clear_all_sprites();
    waitVbl();
}

void NEOGEO_USER demo_scene_caption(const char *title, const char *line1, const char *line2)
{
    clearFix();
    if (title) fixtext_out(2, 1, (char *)title, 0);
    if (line1) fixtext_out(2, 3, (char *)line1, 1);
    if (line2) fixtext_out(2, 5, (char *)line2, 2);
    fixtext_out(25, 27, "A: NEXT", 1);
}

static void NEOGEO_USER demo_wait_labeled(uint16_t frames)
{
    (void)demo_wait_frames_or_a(frames);
}

void NEOGEO_USER showWalkDemo(int loops, int delay_ms)
{
    int i;
    int frame;
    static const DemoShowScreenFn walk_frames[] = {
        showScreen2, showScreen3, showScreen4, showScreen5,
        showScreen6, showScreen7, showScreen8
    };

    demo_clear_scene();
    demo_scene_caption("SPRITE FRAME WALK", "RAW SHOWSCREEN TEST", "CENTERED / HARD-CLEARED");
    playVoiceCue(SOUND_VOICE_GET_READY);

    frame = 0;
    for (i = 0; i < loops; i++) {
        demo_clear_all_sprites();
        /* Keep these raw generated sprites high enough to avoid bottom clipping. */
        walk_frames[frame](88, 78, 0xF, 0xAF, 9, 0x0FFF, 0);
        if (i == 0) playSFX(SOUND_SFX_FOOTSTEP);
        if (demo_wait_frames_or_a((uint16_t)(delay_ms > 0 ? delay_ms : 6))) break;
        frame++;
        if (frame >= 7) frame = 0;
    }

    demo_clear_scene();
}

/* Title screen — stable title 1 plus a safe fallback instead of broken title 2. */
void NEOGEO_USER showTitleScreen(void)
{
    int i;

    demo_clear_scene();
    soundSceneReset();
    setBACKDROP(BLACK);

    demo_scene_caption("TITLE SCREEN", "TITLE ART 1 / BOOT REVEAL", "PRESS A TO ADVANCE");
    showScreen108(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
    playSFX(SOUND_SFX_TITLE_GONG);
    if (demo_wait_frames_or_a(180)) {
        demo_clear_scene();
        return;
    }

    /* showScreen109 is currently a bad art export on some builds: it shows one
       vertical strip at the left. Use the Eagle logo reveal as safe title 2. */
    demo_clear_scene();
    demo_scene_caption("TITLE SCREEN", "TITLE ART 2 SAFE FALLBACK", "BROKEN STRIP ASSET BYPASSED");
    showScreen107(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
    playSFX(SOUND_SFX_LOW_DRUM);

    for (i = 0; i < 240; i++) {
        waitVbl();
        if (demo_advance_requested()) break;
        if (i % 60 < 30)
            fixtext_out(14, 26, "PRESS START", 0);
        else
            fixtext_out(14, 26, "           ", 0);
    }

    demo_clear_scene();
}

void NEOGEO_USER demo_screen_showcase(void)
{
    demo_clear_scene();
    soundSceneReset();

    demo_scene_caption("SHOWSCREEN ASSET DEMO", "1. TITLE / BOOT IMAGE", "GENERATED SHOWSCREEN DATA");
    showScreen108(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
    playSFX(SOUND_SFX_TITLE_GONG);
    demo_wait_labeled(160);

    demo_clear_scene();
    demo_scene_caption("SHOWSCREEN ASSET DEMO", "2. SAFE LOGO FALLBACK", "TITLE 2 STRIP BUG AVOIDED");
    showScreen107(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
    playSFX(SOUND_SFX_LOW_DRUM);
    demo_wait_labeled(140);

    showWalkDemo(32, 7);
    demo_clear_scene();
}
