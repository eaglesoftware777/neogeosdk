#include "demo_screen.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include <stdint.h>

void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER playVoiceCue(uint8_t n);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playSFXB(uint8_t n);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER soundSetSSGVolume(uint8_t v);
void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER clearSprs(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);
void NEOGEO_USER fixtext_out(uint16_t x, uint16_t y, char *mess, short pal);
void NEOGEO_USER vram_SCB234(uint16_t SCBADDR, uint16_t SCB234);

uint8_t NEOGEO_USER demo_advance_requested(void)
{
    return (NEO_REGISTER8(BIOS_P1CHANGE) & (uint8_t)(1u << CNT_A)) ? 1u : 0u;
}

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

void NEOGEO_USER demo_fix_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    char buf[39];
    uint8_t i = 0;

    if (!text || x >= 40 || y >= 28) return;

    while (text[i] && i < (uint8_t)(38u - x)) {
        buf[i] = text[i];
        i++;
    }
    buf[i] = '\0';
    fixtext_out(x, y, buf, pal);
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
    clearFix();
    demo_clear_all_sprites();
    waitVbl();
}

void NEOGEO_USER demo_scene_caption(const char *title, const char *line1, const char *line2)
{
    clearFix();
    if (title) demo_fix_puts(2, 1, title, 2);
    if (line1) demo_fix_puts(2, 3, line1, 0);
    if (line2) demo_fix_puts(2, 5, line2, 1);
    demo_fix_puts(2, 27, "A: NEXT", 1);
}

void NEOGEO_USER demo_safe_show(DemoShowScreenFn fn, int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base)
{
    if (!fn) return;
    if (sprite_base == 0) sprite_base = DEMO_SHOWSCREEN_BASE;
    fn(x0, y0, xr, yr, min_crt_sz, backdrop, sprite_base);
}

void NEOGEO_USER demo_load_screen_palette(uint8_t screen_id)
{
    switch (screen_id) {
        case 1: showScreen1(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 2: showScreen2(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 3: showScreen3(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 4: showScreen4(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 5: showScreen5(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 6: showScreen6(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 7: showScreen7(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 8: showScreen8(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 9: showScreen9(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 10: showScreen10(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 11: showScreen11(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 12: showScreen12(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 13: showScreen13(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 14: showScreen14(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 15: showScreen15(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 16: showScreen16(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 17: showScreen17(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 18: showScreen18(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 19: showScreen19(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 20: showScreen20(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 21: showScreen21(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 22: showScreen22(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 23: showScreen23(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 24: showScreen24(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 25: showScreen25(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 26: showScreen26(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 27: showScreen27(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 28: showScreen28(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 29: showScreen29(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 30: showScreen30(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 31: showScreen31(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 32: showScreen32(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 33: showScreen33(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 34: showScreen34(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 35: showScreen35(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 36: showScreen36(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 37: showScreen37(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 38: showScreen38(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 39: showScreen39(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 40: showScreen40(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 41: showScreen41(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 42: showScreen42(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 43: showScreen43(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 44: showScreen44(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 45: showScreen45(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 46: showScreen46(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 47: showScreen47(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 48: showScreen48(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 49: showScreen49(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 50: showScreen50(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 51: showScreen51(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 52: showScreen52(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 53: showScreen53(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 54: showScreen54(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 55: showScreen55(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 56: showScreen56(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 57: showScreen57(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 58: showScreen58(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 59: showScreen59(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 60: showScreen60(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 61: showScreen61(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 62: showScreen62(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 63: showScreen63(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 64: showScreen64(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 65: showScreen65(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 66: showScreen66(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 67: showScreen67(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 68: showScreen68(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 69: showScreen69(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 70: showScreen70(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 71: showScreen71(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 72: showScreen72(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 73: showScreen73(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 74: showScreen74(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 75: showScreen75(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 76: showScreen76(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 77: showScreen77(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 78: showScreen78(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 79: showScreen79(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 80: showScreen80(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 81: showScreen81(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 82: showScreen82(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 83: showScreen83(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 84: showScreen84(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 85: showScreen85(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 86: showScreen86(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 87: showScreen87(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 88: showScreen88(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 89: showScreen89(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 90: showScreen90(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 91: showScreen91(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 92: showScreen92(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 93: showScreen93(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 94: showScreen94(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 95: showScreen95(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 96: showScreen96(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 97: showScreen97(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 98: showScreen98(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 99: showScreen99(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 100: showScreen100(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 101: showScreen101(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 102: showScreen102(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 103: showScreen103(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 104: showScreen104(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 105: showScreen105(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 106: showScreen106(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 107: showScreen107(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 108: showScreen108(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        case 109: showScreen109(-320, 1024, 0, 0, 0, BLACK, DEMO_PRELOAD_BASE); break;
        default: break;
    }
}

static uint8_t NEOGEO_USER demo_normalize_x_scale(uint8_t scale_x)
{
    /*
     * Many generated showScreen() calls use a 4-bit horizontal reduction
     * value: 0x0F means full width.  NGSpriteGroup expects the wider
     * engine scale byte where 0xFF means full width, and internally turns
     * that back into the hardware nibble.  Passing 0x0F directly makes the
     * object one pixel wide, which is the "vertical stick" bug.
     */
    if (scale_x <= 0x0F) {
        return (uint8_t)((scale_x << 4) | scale_x);
    }
    return scale_x;
}

void NEOGEO_USER demo_draw_sprite_screen(uint8_t screen_id, uint16_t first_sprite, int16_t x, int16_t y, uint8_t strips, uint8_t rows, uint8_t scale_x, uint8_t scale_y)
{
    NGSpriteGroup g;
    if (screen_id == 0) return;
    if (strips == 0) strips = 1;
    if (rows == 0) rows = 1;
    if (strips > 16) strips = 16;
    if (rows > 16) rows = 16;

    demo_load_screen_palette(screen_id);

    ng_sprite_group_init(&g, first_sprite, strips, 16, DEMO_SCREEN_TILE(screen_id), DEMO_SCREEN_PALETTE(screen_id));
    ng_sprite_group_set_tile_stride(&g, 16);
    ng_sprite_group_set_active_rows(&g, rows);
    ng_sprite_group_set_pos(&g, x, y);
    ng_sprite_group_set_scale(&g, demo_normalize_x_scale(scale_x), scale_y);
    ng_sprite_group_upload(&g);
}

void NEOGEO_USER showWalkDemo(int loops, int delay_frames)
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

void NEOGEO_USER showTitleScreen(void)
{
    int i;

    demo_clear_scene();
    soundSceneReset();
    setBACKDROP(BLACK);
    soundSetADPCMAVolume(0x3C);
    soundSetADPCMBVolume(0xB0);
    soundSetSSGVolume(0x00);

    demo_scene_caption("TITLE SCREEN", "NEO GEO SDK DEMO", "SAFE SHOWSCREEN BASE 0040H");
    demo_safe_show(showScreen108, 16, 24, 0xF, 0xAF, 16, BLACK, DEMO_SHOWSCREEN_BASE);
    playSFX(SOUND_SFX_TITLE_GONG);
    if (demo_wait_frames_or_a(180)) {
        demo_clear_scene();
        return;
    }

    demo_clear_scene();
    demo_scene_caption("TITLE SCREEN", "EAGLE SOFTWARE FALLBACK", "SECOND TITLE ASSET BYPASSED");
    demo_safe_show(showScreen107, 16, 24, 0xF, 0xAF, 16, BLACK, DEMO_SHOWSCREEN_BASE);
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
    showTitleScreen();
    showWalkDemo(18, 30);
}
