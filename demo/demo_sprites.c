/*
 * demo_sprites.c — Scene 3: Sprite group showcase
 *
 * Demonstrates sticky-bit sprite chaining, dirty-flag efficiency,
 * large character groups moving without split artefacts,
 * hardware scale matrix, and animated NPC cast.
 *
 * https://eaglesoftware.biz
 */

#include "demo_sprites.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_engine.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include "sdk/2d_engine/ng_chars.h"
#include <stdint.h>

void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playVoiceCue(uint8_t n);

/* ------------------------------------------------------------------ */
/*  Asset layout constants                                               */
/* ------------------------------------------------------------------ */
/* Main warrior walk/idle: screens 11-19 (meta[10..18])                */
#define SPR_WALK_FIRST     11u
#define SPR_WALK_COUNT      9u
#define SPR_MAIN_X         20
#define SPR_MAIN_Y        (-34)
#define SPR_MAIN_ATK_X     18
#define SPR_MAIN_ATK_Y    (-38)

/* NPC: z_npc_84..z_npc_95 = screens 109-120, meta[108..119]           */
#define NPC_FRAME_COUNT    12u
#define NPC_TILE(n)        ((uint16_t)(27648u + (uint16_t)((uint8_t)(n) % 12u) * 256u + 165u))
#define NPC_PALETTE        124u
#define NPC_STRIPS          6u
#define NPC_ROWS            6u
#define NPC_STRIDE         16u

/* Eyecatcher character parade: screens 79-92 */
static const uint8_t s_parade_screens[] = {
    79u, 80u, 81u, 82u, 83u, 84u, 85u, 86u,
    87u, 88u, 89u, 90u, 91u, 92u
};
#define PARADE_COUNT ((uint8_t)(sizeof(s_parade_screens) / sizeof(s_parade_screens[0])))

/* Scale matrix */
static const uint8_t  s_scales[4]   = { 0xFFu, 0xC8u, 0x96u, 0x64u };
static const int16_t  s_xpos[4]     = { 12,    76,    148,   216  };
static const uint16_t s_slots[4]    = { 300u,  306u,  312u,  318u };

/* ------------------------------------------------------------------ */
/*  Helpers                                                              */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER spr_depth_scale(int16_t y)
{
    /* Map y from range [60..-50] → scale [0x80..0xFF] */
    int16_t span = (int16_t)(60 - (-50));
    int16_t dist = (int16_t)(60 - y);
    if (dist < 0) dist = 0;
    if (dist > span) dist = span;
    return (uint8_t)(0x80u + (uint16_t)((uint16_t)127u * (uint16_t)dist) / (uint16_t)span);
}

static void NEOGEO_USER spr_header(const char *title, const char *sub)
{
    clearFix();
    if (title) demo_fix_puts(2u, 0u, title, 2u);
    if (sub)   demo_fix_puts(2u, 1u, sub,   1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: walk showcase                                             */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER spr_walk_loop(void)
{
    uint16_t t;

    spr_header("SPRITE GROUP CHAINS", "WALK CYCLE  ZERO SPLIT  DIRTY-FLAG");
    demo_fix_puts(2u, 7u, "16-STRIP GROUP  STICKY-BIT CHAINED", 1u);
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);
    playVoiceCue(SOUND_VOICE_GET_READY);

    for (t = 0u; t < (uint16_t)(SPR_WALK_COUNT * 30u); t++) {
        uint8_t frame = (uint8_t)(SPR_WALK_FIRST + (t / 30u));
        demo_draw_sprite_screen(frame, 1u, SPR_MAIN_X, SPR_MAIN_Y, 16u, 16u, 0xFFu, 0xFFu);
        if ((t % 60u) == 0u) playSFX(SOUND_SFX_FOOTSTEP);
        if (demo_wait(1u)) return;
    }
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: scale matrix — 4 sprites at 4 sizes simultaneously       */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER spr_scale_matrix(void)
{
    NGSpriteGroup g[4];
    uint8_t i;
    uint16_t t;

    demo_load_screen_palette(109u);  /* NPC palette preload */

    spr_header("HARDWARE SCALE MATRIX", "4 SIZES LIVE  SCB2 XSCALE/YSCALE");
    demo_fix_puts(3u,  9u, "100%", 1u);
    demo_fix_puts(10u, 9u, " 78%", 1u);
    demo_fix_puts(18u, 9u, " 59%", 2u);
    demo_fix_puts(27u, 9u, " 39%", 2u);
    demo_fix_puts(1u, 10u, "0xFF 0xC8 0x96 0x64", 0u);

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);
    playSFX(SOUND_SFX_LOW_DRUM);

    for (t = 0u; t < 300u; t++) {
        uint8_t frame = (uint8_t)((t / 18u) % NPC_FRAME_COUNT);
        uint16_t tile  = NPC_TILE(frame);

        for (i = 0u; i < 4u; i++) {
            ng_sprite_group_init(&g[i], s_slots[i], NPC_STRIPS, NPC_ROWS,
                                 tile, NPC_PALETTE);
            ng_sprite_group_set_tile_stride(&g[i], NPC_STRIDE);
            ng_sprite_group_set_active_rows(&g[i], NPC_ROWS);
            ng_sprite_group_set_scale(&g[i], s_scales[i], s_scales[i]);
            ng_sprite_group_set_pos(&g[i], s_xpos[i], (int16_t)(-40));
            ng_sprite_group_upload(&g[i]);
        }

        if ((t % 120u) == 0u) playSFX(SOUND_SFX_BLADE_WHOOSH);
        if (demo_wait(1u)) return;
    }
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: NPC depth-sorted crowd                                    */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER spr_crowd(void)
{
#define CROWD_N 5u
    NGCharacter *chars[CROWD_N];
    int16_t cy[CROWD_N];
    uint8_t i;
    uint16_t t;

    static const int16_t start_x[CROWD_N] = { 20, 68, 116, 164, 212 };
    static const int16_t start_y[CROWD_N] = { -50, -23, 5, 32, 60 };

    spr_header("BRAWLER PERSPECTIVE CROWD", "5 NPCS  DEPTH SCALE  Y-SORT  LIVE");
    demo_fix_puts(2u, 7u, "BOTTOM=NEAR=LARGE   TOP=FAR=SMALL", 1u);
    demo_fix_puts(2u, 8u, "380 SPRITES / ZERO SPLIT ALLOWED", 2u);

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);
    playSFX(SOUND_SFX_STRING_PHRASE);
    demo_load_screen_palette(109u);
    ng_chars_init();

    for (i = 0u; i < CROWD_N; i++) {
        uint8_t fn = (uint8_t)((i * 2u) % NPC_FRAME_COUNT);
        cy[i] = start_y[i];
        chars[i] = chars_add(0u, start_x[i], cy[i]);
        if (chars[i]) {
            ng_char_set_sprite(chars[i], 0u, NPC_STRIPS, NPC_ROWS,
                               NPC_TILE(fn), NPC_PALETTE);
            ng_char_set_tile_stride(chars[i], NPC_STRIDE);
            chars[i]->sprite_offset_y = -96;
        }
    }

    for (t = 0u; t < 360u; t++) {
        uint8_t anim = (uint8_t)((t / 18u) % NPC_FRAME_COUNT);

        for (i = 0u; i < CROWD_N; i++) {
            NGCharacter *c = chars[i];
            uint16_t tile;
            uint8_t sc;
            if (!c) continue;

            tile = NPC_TILE(anim);
            if (c->sprite_tile != tile) { c->sprite_tile = tile; c->sprite_dirty = 1u; }

            cy[i] = (int16_t)(cy[i] - 1);
            if (cy[i] < -50) cy[i] = 60;
            c->y = cy[i];
            sc = spr_depth_scale(cy[i]);
            c->scale_x = sc;
            c->scale_y = sc;
        }

        ng_chars_draw();

        if ((t % 90u) == 0u) playSFX(SOUND_SFX_FOOTSTEP);
        if (demo_wait(1u)) break;
    }

    ng_chars_init();
#undef CROWD_N
}

/* ------------------------------------------------------------------ */
/*  Public: full sprites scene                                           */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_sprites_run(void)
{
    demo_clear_scene();
    soundSceneReset();
    soundSetADPCMAVolume(0x3Fu);
    soundSetADPCMBVolume(0xBCu);
    soundSetSSGVolume(0x08u);
    soundSetFMVolume(0x08u);

    spr_walk_loop();
    spr_scale_matrix();
    spr_crowd();

    soundStopAll();
    demo_clear_scene();
}

/* ------------------------------------------------------------------ */
/*  Parade (eyecatcher animation frames)                                 */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_sprites_parade(void)
{
    uint8_t i;

    demo_clear_scene();
    soundSceneReset();
    soundSetADPCMAVolume(0x38u);
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);
    demo_caption("MASCOT PARADE", "EYECATCHER ANIMATION FRAMES", "SLOW FRAME TIMING");
    playVoiceCue(SOUND_VOICE_GET_READY);

    for (i = 0u; i < PARADE_COUNT; i++) {
        demo_caption("MASCOT PARADE", "EYECATCHER ANIMATION FRAMES", "SLOW FRAME TIMING");
        demo_safe_show(0, 72, 62, 0xF, 0xAF, 10, BLACK, DEMO_SHOWSCREEN_BASE);
        demo_load_screen_palette(s_parade_screens[i]);
        demo_draw_sprite_screen(s_parade_screens[i], 1u,
                                72, 62, 16u, 16u, 0xFFu, 0xFFu);
        if ((i & 3u) == 0u) playSFX(SOUND_SFX_STRING_PHRASE);
        if (demo_wait(28u)) break;
    }

    soundStopAll();
    demo_clear_scene();
}

/* ------------------------------------------------------------------ */
/*  Walk demo (legacy compat)                                            */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_sprites_walk(int loops, int delay_frames)
{
    int i;
    uint8_t frame;

    demo_clear_scene();
    soundSceneReset();
    soundSetADPCMAVolume(0x38u);
    demo_caption("SPRITE TILE PREVIEW", "CENTERED NGSpriteGroup DRAW", "NO PER-FRAME HARD CLEAR");
    playVoiceCue(SOUND_VOICE_GET_READY);

    frame = 0u;
    for (i = 0; i < loops; i++) {
        demo_draw_sprite_screen((uint8_t)(2u + frame), 1u,
                                SPR_MAIN_X, SPR_MAIN_Y,
                                16u, 16u, 0xFFu, 0xFFu);
        if ((frame & 3u) == 0u) playSFX(SOUND_SFX_FOOTSTEP);
        if (demo_wait((uint16_t)(delay_frames > 0 ? delay_frames : 24))) break;
        frame++;
        if (frame >= 7u) frame = 0u;
    }

    demo_clear_scene();
}
