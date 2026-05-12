#include "demo_2d_engine.h"
#include "demo_screen.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include <stdint.h>

void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER soundSetSSGVolume(uint8_t v);
void NEOGEO_USER soundSetFMVolume(uint8_t v);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playSFXB(uint8_t n);
void NEOGEO_USER playVoiceCue(uint8_t n);
void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);

/* Direct generated NPC frames.  These are real showScreen exports for
 * artbox/in/npcs (z_npc_84..z_npc_95), so use them directly instead
 * of guessing C-ROM tile bases. */
void NEOGEO_USER showScreen94(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base);
void NEOGEO_USER showScreen95(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base);
void NEOGEO_USER showScreen96(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base);
void NEOGEO_USER showScreen97(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base);
void NEOGEO_USER showScreen98(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base);
void NEOGEO_USER showScreen99(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base);
void NEOGEO_USER showScreen100(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base);
void NEOGEO_USER showScreen101(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base);
void NEOGEO_USER showScreen102(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base);
void NEOGEO_USER showScreen103(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base);
void NEOGEO_USER showScreen104(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base);
void NEOGEO_USER showScreen105(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base);

/*
 * The generated character canvases are wide 16-strip C-ROM blocks.  The
 * visible body is not at the left edge of the canvas, so the driver X must
 * be lower than a normal centered sprite.  These values put the visible
 * body around the middle/lower play area instead of the bottom edge.
 *
 * Do not clear sprites every frame: doing so creates visible flicker on real
 * hardware and in MAME.  Each scene clears once, then reuses the same slots.
 */
#define DEMO_MAIN_X            20
#define DEMO_MAIN_Y           (-34)
#define DEMO_MAIN_ATTACK_X     18
#define DEMO_MAIN_ATTACK_Y    (-38)
#define DEMO_FX_X              94
#define DEMO_FX_Y             (-38)
#define DEMO_NPC_SCREEN_BASE   94
#define DEMO_NPC_FRAME_COUNT   12
#define DEMO_NPC_FRAME(n) ((uint8_t)(DEMO_NPC_SCREEN_BASE + ((n) % DEMO_NPC_FRAME_COUNT)))

static void NEOGEO_USER demo_engine_header(const char *phase, const char *line)
{
    clearFix();
    demo_fix_puts(2, 1, "NEO 2D ENGINE FULL MODE", 2);
    if (phase) demo_fix_puts(2, 3, phase, 0);
    if (line) demo_fix_puts(2, 5, line, 1);
    demo_fix_puts(2, 27, "A: NEXT   START: GAME FLOW ONLY", 1);
}

static uint8_t NEOGEO_USER demo_wait_short(uint16_t frames)
{
    return demo_wait_frames_or_a(frames);
}

static void NEOGEO_USER demo_main_frame(uint8_t screen_id, int16_t x, int16_t y, uint8_t sx, uint8_t sy)
{
    demo_draw_sprite_screen(screen_id, 1, x, y, 16, 16, sx, sy);
}

static void NEOGEO_USER demo_fx_frame(uint8_t screen_id, int16_t x, int16_t y)
{
    demo_draw_sprite_screen(screen_id, 24, x, y, 16, 16, 0xFF, 0xFF);
}

static void NEOGEO_USER demo_npc_frame(uint8_t screen_id, uint16_t slot, int16_t x, int16_t y)
{
    uint16_t base = (uint16_t)(slot * 64u);

    switch (screen_id) {
        case 94:  showScreen94 (x, y, 0x0F, 0xFF, 16, BLACK, base); break;
        case 95:  showScreen95 (x, y, 0x0F, 0xFF, 16, BLACK, base); break;
        case 96:  showScreen96 (x, y, 0x0F, 0xFF, 16, BLACK, base); break;
        case 97:  showScreen97 (x, y, 0x0F, 0xFF, 16, BLACK, base); break;
        case 98:  showScreen98 (x, y, 0x0F, 0xFF, 16, BLACK, base); break;
        case 99:  showScreen99 (x, y, 0x0F, 0xFF, 16, BLACK, base); break;
        case 100: showScreen100(x, y, 0x0F, 0xFF, 16, BLACK, base); break;
        case 101: showScreen101(x, y, 0x0F, 0xFF, 16, BLACK, base); break;
        case 102: showScreen102(x, y, 0x0F, 0xFF, 16, BLACK, base); break;
        case 103: showScreen103(x, y, 0x0F, 0xFF, 16, BLACK, base); break;
        case 104: showScreen104(x, y, 0x0F, 0xFF, 16, BLACK, base); break;
        case 105: showScreen105(x, y, 0x0F, 0xFF, 16, BLACK, base); break;
        default: break;
    }
}

static void NEOGEO_USER demo_prepare_scene(const char *phase, const char *line)
{
    demo_clear_scene();
    demo_engine_header(phase, line);
}

static void NEOGEO_USER demo_walk_showcase(void)
{
    uint16_t t;

    demo_prepare_scene("MAIN CHARACTER", "WALK / IDLE TILES FROM C ROM");
    demo_fix_puts(2, 7, "CENTERED / NO PER-FRAME CLEAR", 1);
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);
    playVoiceCue(SOUND_VOICE_GET_READY);

    for (t = 0; t < 9u * 30u; t++) {
        uint8_t frame = (uint8_t)(11u + (t / 30u));
        demo_main_frame(frame, DEMO_MAIN_X, DEMO_MAIN_Y, 0xFF, 0xFF);
        if ((t % 60u) == 0u) playSFX(SOUND_SFX_FOOTSTEP);
        if (demo_wait_short(1)) return;
    }
}

static void NEOGEO_USER demo_blade_combo(void)
{
    uint16_t t;

    demo_prepare_scene("SPECIAL MOVE 1", "BLADE COMBO: WHOOSH + IMPACT");
    demo_fix_puts(2, 7, "NO FLICKER: SAME SPRITE SLOTS", 1);
    playVoiceCue(SOUND_VOICE_ATTACK);

    for (t = 0; t < 15u * 24u; t++) {
        uint8_t frame = (uint8_t)(20u + (t / 24u));
        demo_main_frame(frame, DEMO_MAIN_ATTACK_X, DEMO_MAIN_ATTACK_Y, 0xFF, 0xFF);
        if (t == 72u) playSFX(SOUND_SFX_BLADE_WHOOSH);
        if (t == 216u) playSFX(SOUND_SFX_IMPACT_HIT);
        if (demo_wait_short(1)) return;
    }
}

static void NEOGEO_USER demo_energy_combo(void)
{
    uint16_t t;

    demo_prepare_scene("SPECIAL MOVE 2", "ENERGY / SLASH FX OVERLAY");
    demo_fix_puts(2, 7, "MAIN SPRITE + FX SPRITE GROUP", 1);
    playSFX(SOUND_SFX_START_SLASH);

    for (t = 0; t < 13u * 24u; t++) {
        uint8_t f = (uint8_t)(50u + (t / 24u));
        uint8_t main_f = (uint8_t)(20u + ((t / 24u) % 12u));
        demo_main_frame(main_f, DEMO_MAIN_ATTACK_X, DEMO_MAIN_ATTACK_Y, 0xFF, 0xFF);
        demo_fx_frame(f, DEMO_FX_X, DEMO_FX_Y);
        if ((t % 96u) == 0u) playSFX(SOUND_SFX_STRING_PHRASE);
        if (demo_wait_short(1)) return;
    }
}

static void NEOGEO_USER demo_npc_scene(void)
{
    uint16_t t;

    demo_prepare_scene("NPC SCENE", "MAIN CHARACTER WITH MOVING CAST");
    demo_fix_puts(2, 7, "DIRECT NPC EXPORTS: z_npc_84..95", 1);
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);
    playSFX(SOUND_SFX_STRING_PHRASE);

    for (t = 0; t < 420u; t++) {
        uint8_t hero = (uint8_t)(11u + ((t / 26u) % 9u));
        uint8_t npc_a = DEMO_NPC_FRAME((uint8_t)(t / 18u));
        uint8_t npc_b = DEMO_NPC_FRAME((uint8_t)(6u + (t / 24u)));
        int16_t ax = (int16_t)(-26 + ((t / 3u) % 92u));
        int16_t bx = (int16_t)(132 - ((t / 4u) % 88u));

        demo_npc_frame(npc_a, 40, ax, -24);
        demo_main_frame(hero, DEMO_MAIN_X, DEMO_MAIN_Y, 0xFF, 0xFF);
        demo_npc_frame(npc_b, 64, bx, -28);
        if ((t % 108u) == 0u) playSFX(SOUND_SFX_FOOTSTEP);
        if (demo_wait_short(1)) return;
    }
}

static void NEOGEO_USER demo_optical_floor(uint16_t t)
{
    uint8_t row;
    demo_fix_puts(2, 9, "HORIZON", 2);
    for (row = 0; row < 7; row++) {
        uint8_t y = (uint8_t)(11 + row * 2);
        uint8_t left = (uint8_t)(16 - row * 2);
        uint8_t width = (uint8_t)(8 + row * 4);
        uint8_t i;
        char line[37];
        for (i = 0; i < 36; i++) line[i] = ' ';
        line[36] = '\0';
        if (left > 35) left = 0;
        for (i = 0; i < width && (uint8_t)(left + i) < 36; i++) {
            line[left + i] = ((t >> 4) & 1u) ? '-' : '=';
        }
        demo_fix_puts(2, y, line, (uint8_t)(row & 1u));
    }
}

static void NEOGEO_USER demo_optical_background_scene(void)
{
    uint16_t t;

    demo_prepare_scene("OPTICAL BACKGROUND", "POSITION DEPTH ONLY - NO SCB SHRINK");
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);
    playSFX(SOUND_SFX_LOW_DRUM);

    for (t = 0; t < 420u; t++) {
        uint8_t phase = (uint8_t)((t / 105u) & 3u);
        uint8_t frame = (uint8_t)(20u + ((t / 24u) % 15u));
        int16_t y;
        uint8_t sx;
        uint8_t sy;

        if (phase == 0) { y = -62; sx = 0xFF; sy = 0xFF; }
        else if (phase == 1) { y = -52; sx = 0xFF; sy = 0xFF; }
        else if (phase == 2) { y = -42; sx = 0xFF; sy = 0xFF; }
        else { y = -32; sx = 0xFF; sy = 0xFF; }

        demo_optical_floor(t);
        demo_main_frame(frame, DEMO_MAIN_X, y, sx, sy);
        if ((t % 132u) == 0u) playSFX(SOUND_SFX_BLADE_WHOOSH);
        if (demo_wait_short(1)) return;
    }
}

void NEOGEO_USER demo_2d_engine_run(void)
{
    demo_clear_scene();
    soundSceneReset();
    soundSetADPCMAVolume(0x3F);
    soundSetADPCMBVolume(0xBC);
    soundSetSSGVolume(0x08);
    soundSetFMVolume(0x08);

    demo_walk_showcase();
    demo_blade_combo();
    demo_energy_combo();
    demo_npc_scene();
    demo_optical_background_scene();

    soundStopAll();
    demo_clear_scene();
}

void NEOGEO_USER demo_2d_engine_advanced_animation(void)
{
    uint16_t t;

    demo_prepare_scene("ADVANCED ANIMATION", "FINISHER FRAME TIMING");
    demo_fix_puts(2, 7, "SLOW HOLD / IMPACT CUES", 1);
    soundSceneReset();
    soundSetADPCMAVolume(0x3F);
    soundSetADPCMBVolume(0xB8);
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    for (t = 0; t < 360u; t++) {
        uint8_t frame = (uint8_t)(30u + ((t / 24u) % 20u));
        demo_main_frame(frame, DEMO_MAIN_ATTACK_X, DEMO_MAIN_ATTACK_Y, 0xFF, 0xFF);
        if ((t % 120u) == 0u) playSFX(SOUND_SFX_IMPACT_HIT);
        if (demo_wait_short(1)) break;
    }

    soundStopAll();
    demo_clear_scene();
}
