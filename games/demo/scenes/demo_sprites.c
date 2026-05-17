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
#include "sdk/2d_engine/ng_physics.h"
#include "sdk/2d_engine/ng_fixed.h"
#include <stdint.h>

void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playVoiceCue(uint8_t n);
void NEOGEO_USER ng_clear_screen_full(void);

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

/* NPC: cat_01..cat_12 = screens 110-121 in the generated manifest. */
#define NPC_FRAME_COUNT    12u
#define NPC_FIRST_SCREEN  110u
#define NPC_SCREEN(n)     ((uint8_t)(NPC_FIRST_SCREEN + ((uint8_t)(n) % NPC_FRAME_COUNT)))
#define NPC_TILE(n)       DEMO_SCREEN_TILE(NPC_SCREEN(n))
#define NPC_PALETTE(n)    DEMO_SCREEN_PALETTE(NPC_FIRST_SCREEN)
#define NPC_STRIPS         12u
#define NPC_ROWS           15u
#define NPC_STRIDE         16u
#define NPC_OFFSET_Y      (-240)

/* Eyecatcher character parade: screens 79-92 */
static const uint8_t s_parade_screens[] = {
    79u, 80u, 81u, 82u, 83u, 84u, 85u, 86u,
    87u, 88u, 89u, 90u, 91u, 92u
};
#define PARADE_COUNT ((uint8_t)(sizeof(s_parade_screens) / sizeof(s_parade_screens[0])))

/* Scale matrix */
static const uint8_t  s_scales[4]   = { 0xFFu, 0xC8u, 0x96u, 0x64u };
static const int16_t  s_xpos[4]     = { 12,    76,    148,   216  };
static const uint16_t s_slots[4]    = { 300u,  312u,  324u,  336u };

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
        uint8_t pal = NPC_PALETTE(frame);
        demo_load_screen_palette(NPC_FIRST_SCREEN);

        for (i = 0u; i < 4u; i++) {
            ng_sprite_group_init(&g[i], s_slots[i], NPC_STRIPS, NPC_ROWS,
                                 tile, pal);
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
    ng_chars_init();

    for (i = 0u; i < CROWD_N; i++) {
        uint8_t fn = (uint8_t)((i * 2u) % NPC_FRAME_COUNT);
        cy[i] = start_y[i];
        chars[i] = chars_add(0u, start_x[i], cy[i]);
        if (chars[i]) {
            demo_load_screen_palette(NPC_FIRST_SCREEN);
            ng_char_set_sprite(chars[i], 0u, NPC_STRIPS, NPC_ROWS,
                               NPC_TILE(fn), NPC_PALETTE(fn));
            ng_char_set_tile_stride(chars[i], NPC_STRIDE);
            chars[i]->sprite_offset_y = NPC_OFFSET_Y;
        }
    }

    for (t = 0u; t < 360u; t++) {
        uint8_t anim = (uint8_t)((t / 18u) % NPC_FRAME_COUNT);
        demo_load_screen_palette(NPC_FIRST_SCREEN);

        for (i = 0u; i < CROWD_N; i++) {
            NGCharacter *c = chars[i];
            uint16_t tile;
            uint8_t pal;
            uint8_t sc;
            if (!c) continue;

            tile = NPC_TILE(anim);
            pal = NPC_PALETTE(anim);
            if (c->sprite_tile != tile || c->palette != pal) {
                c->sprite_tile = tile;
                c->palette = pal;
                c->sprite_dirty = 1u;
            }

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
/*  Sub-scene: raw SCB API demo                                          */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER spr_raw_api(void)
{
    NGSpriteGroup g;
    uint16_t t;

    enum {
        SCREEN_W = 320,
        SCREEN_H = 224,
        SPR_COLS = 6,
        SPR_ROWS = 10,
        TILE_SIZE = 16,
        SPR_W = SPR_COLS * TILE_SIZE,
        SPR_H = SPR_ROWS * TILE_SIZE,
        CENTER_X = (SCREEN_W - SPR_W) / 2,
        CENTER_Y = (SCREEN_H - SPR_H) / 2
    };

    clearFix();
    setBACKDROP(BLACK);

    demo_fix_puts(2u, 0u,  "RAW NEO GEO API / DIRECT SCB WRITES", 2u);
    demo_fix_puts(2u, 1u,  "NO NG_* WRAPPERS  PURE HARDWARE", 1u);
    demo_fix_puts(2u, 3u,  "SCB1=TILE+PAL  SCB2=SCALE  SCB3=Y+STICKY  SCB4=X", 0u);
    demo_fix_puts(2u, 5u,  "CENTERED CHARACTER", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    demo_load_screen_palette(11u);
    soundSceneReset();
    soundSetADPCMAVolume(0x38u);
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);
    ng_sprite_group_init(&g, 1u, SPR_COLS, SPR_ROWS,
                         DEMO_SCREEN_TILE(11u),
                         DEMO_SCREEN_PALETTE(11u));

    ng_sprite_group_set_tile_stride(&g, 16u);
    ng_sprite_group_set_active_rows(&g, SPR_ROWS);
    ng_sprite_group_set_scale(&g, 0xFFu, 0xFFu);

    for (t = 0u; t < 180u; t++) {
        ng_sprite_group_set_pos(&g, CENTER_X, CENTER_Y);
        ng_sprite_group_upload(&g);

        if (demo_frame()) break;
    }

    ng_sprite_group_hide(&g);
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: animation state machine                                   */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER spr_action_fsm(void)
{
    NGCharacter *c;
    uint16_t t;
    uint8_t state;
    uint16_t state_timer;
    int16_t base_x;
    int16_t base_y;
    int16_t draw_y;

    enum {
        SCREEN_W = 320,
        SCREEN_H = 224,

        SPR_COLS = 14,
        SPR_ROWS = 10,
        TILE_SIZE = 16,
        SPR_W = SPR_COLS * TILE_SIZE,
        SPR_H = SPR_ROWS * TILE_SIZE,

        CENTER_X = (SCREEN_W - SPR_W) / 2,
        BASE_Y = ((SCREEN_H - SPR_H) / 2) + SPR_H,

        JUMP_H = 32
    };

    enum {
        STATE_IDLE = 0,
        STATE_RUN = 1,
        STATE_JUMP = 2,
        STATE_ATTACK = 3
    };

    static const char *const s_state_names[4] = {
        "IDLE  ", "RUN   ", "JUMP  ", "ATTACK"
    };

    static const uint16_t s_state_dur[4] = {
        90u, 180u, 70u, 90u
    };

    static const uint8_t s_state_speed[4] = {
        12u, 8u, 7u, 7u
    };

    static const uint8_t s_idle_frames[1] = { 13u };
    static const uint8_t s_run_frames[4] = { 15u, 16u, 17u, 18u };
    static const uint8_t s_jump_frames[4] = { 14u, 19u, 20u, 21u };
    static const uint8_t s_attack_frames[5] = { 26u, 27u, 28u, 29u, 30u };

    clearFix();
    setBACKDROP(BLACK);

    demo_fix_puts(2u, 0u,  "ANIMATION STATE MACHINE", 2u);
    demo_fix_puts(2u, 1u,  "RUNNING FULL SEQUENCE", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    ng_chars_init();

    base_x = CENTER_X;
    base_y = BASE_Y;

    demo_load_screen_palette(s_run_frames[0]);

    c = chars_add(0u, base_x, base_y);
    if (c) {
        ng_char_set_sprite(c, 0u, SPR_COLS, SPR_ROWS,
                           DEMO_SCREEN_TILE(s_run_frames[0]),
                           DEMO_SCREEN_PALETTE(s_run_frames[0]));

        ng_char_set_tile_stride(c, 16u);

        c->sprite_offset_y = -SPR_H;
        c->scale_x = 0xFFu;
        c->scale_y = 0xFFu;
    }

    state = STATE_RUN;
    state_timer = 0u;

    for (t = 0u; t < ((90u + 180u + 70u + 90u) * 2u); t++) {
        uint8_t frame_id;

        if (state_timer >= s_state_dur[state]) {
            state = (uint8_t)((state + 1u) & 3u);
            state_timer = 0u;
        }

        draw_y = base_y;

        if (state == STATE_JUMP) {
            uint16_t half = s_state_dur[STATE_JUMP] / 2u;

            if (state_timer < half) {
                draw_y = (int16_t)(base_y -
                    (((int16_t)state_timer * JUMP_H) / (int16_t)half));
            } else {
                uint16_t down = (uint16_t)(state_timer - half);

                draw_y = (int16_t)(base_y - JUMP_H +
                    (((int16_t)down * JUMP_H) / (int16_t)half));
            }
        }

        if (state == STATE_IDLE) {
            frame_id = s_idle_frames[0];
        } else if (state == STATE_RUN) {
            frame_id = s_run_frames[
                (state_timer / s_state_speed[STATE_RUN]) % 4u
            ];
        } else if (state == STATE_JUMP) {
            frame_id = s_jump_frames[
                (state_timer / s_state_speed[STATE_JUMP]) % 4u
            ];
        } else {
            frame_id = s_attack_frames[
                (state_timer / s_state_speed[STATE_ATTACK]) % 5u
            ];
        }

        if (c) {
            uint16_t tile = DEMO_SCREEN_TILE(frame_id);
            uint8_t pal = DEMO_SCREEN_PALETTE(frame_id);

            demo_load_screen_palette(frame_id);

            if (c->sprite_tile != tile || c->palette != pal) {
                c->sprite_tile = tile;
                c->palette = pal;
                c->sprite_dirty = 1u;
            }

            c->x = base_x;
            c->y = draw_y;
        }

        demo_fix_puts(2u, 3u, "STATE:", 0u);
        demo_fix_puts(9u, 3u, s_state_names[state], 2u);

        ng_chars_draw();
        state_timer++;

        if (demo_frame()) {
            break;
        }
    }

    ng_clear_screen_full();
}
/* ------------------------------------------------------------------ */
/*  Sub-scene: physics with gravity and solid platform                   */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER spr_physics(void)
{
    NGCharacter *eagle = 0;
    uint16_t t;
    uint8_t frame_id;
    uint8_t grounded;
    char buf[8];

    enum {
        EAGLE_ROWS = 6,
        EAGLE_STRIDE = 16,
        EAGLE_H = EAGLE_ROWS * 8,

        PLATFORM_X = 40,
        PLATFORM_Y = 160,
        PLATFORM_W = 240,
        PLATFORM_H = 8,

        EAGLE_X = 82,
        EAGLE_START_Y = 118,

        FLY_TIME = 90,
        DEMO_END = 330
    };

    static const uint8_t s_eagle_frames[3] = {
        76u, 77u, 78u
    };
    static const uint8_t s_eagle_ground_frames[3] = {
        73u, 74u, 75u
    };

    ng_clear_screen_full();

    demo_fix_puts(2u, 0u,  "EAGLE PHYSICS TEST", 2u);
    demo_fix_puts(2u, 1u,  "FLY -> FALL -> LAND", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    demo_fix_puts(5u, 20u, "==============================", 2u);

    ng_physics_init();
    ng_physics_add_solid(PLATFORM_X, PLATFORM_Y,
                         PLATFORM_W, PLATFORM_H, 0u);

    frame_id = s_eagle_frames[0];
    demo_load_screen_palette(frame_id);

    eagle = chars_add(0u, EAGLE_X, EAGLE_START_Y);
    if (eagle) {
        ng_char_set_sprite(eagle, 0u,
                           demo_screen_strips(frame_id),
                           EAGLE_ROWS,
                           DEMO_SCREEN_TILE(frame_id),
                           DEMO_SCREEN_PALETTE(frame_id));

        ng_char_set_tile_stride(eagle, EAGLE_STRIDE);

        /*
         * Bottom anchor for physics.
         */
        eagle->sprite_offset_y = -EAGLE_H;

        /*
         * Shrink so the full eagle fits and landing is readable.
         */
        eagle->scale_x = 0x80u;
        eagle->scale_y = 0x80u;

        ng_physics_attach(eagle,
            (uint16_t)(NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS));

        ng_physics_set_gravity(eagle,
            NG_FP_FROM_FRAC(1, 4),
            NG_TO_FP(4));
    }

    for (t = 0u; t < DEMO_END; t++) {
        if (eagle) {
            if (t < FLY_TIME) {
                eagle->x = EAGLE_X;
                eagle->y = (int16_t)(EAGLE_START_Y +
                    (((t & 15u) < 8u) ? 0 : 3));

                frame_id = s_eagle_frames[(t / 10u) % 3u];

                demo_fix_puts(2u, 23u, "STATE: FLYING  ", 1u);
            } else {
                if (t == FLY_TIME) {
                    eagle->x = EAGLE_X;
                    eagle->y = EAGLE_START_Y;
                }

                ng_physics_update_pre();
                ng_chars_update();
                ng_physics_resolve();

                grounded = ng_physics_is_grounded(eagle);

                frame_id = grounded
                    ? s_eagle_ground_frames[(t / 6u) % 3u]
                    : s_eagle_frames[(t / 6u) % 3u];

                demo_fix_puts(2u, 23u,
                              grounded ? "STATE: GROUNDED" : "STATE: FALLING ",
                              grounded ? 2u : 1u);
            }

            demo_load_screen_palette(frame_id);

            if (eagle->sprite_tile   != DEMO_SCREEN_TILE(frame_id)     ||
                eagle->palette       != DEMO_SCREEN_PALETTE(frame_id)  ||
                eagle->sprite_strips != demo_screen_strips(frame_id)) {
                eagle->sprite_tile   = DEMO_SCREEN_TILE(frame_id);
                eagle->palette       = DEMO_SCREEN_PALETTE(frame_id);
                eagle->sprite_strips = demo_screen_strips(frame_id);
                eagle->sprite_dirty  = 1u;
            }

            grounded = ng_physics_is_grounded(eagle);

            buf[0] = 'G';
            buf[1] = 'R';
            buf[2] = 'D';
            buf[3] = ':';
            buf[4] = grounded ? '1' : '0';
            buf[5] = '\0';

            demo_fix_puts(27u, 23u, buf, grounded ? 2u : 1u);
        }

        ng_chars_draw();

        if (demo_frame()) {
            break;
        }
    }

    ng_clear_screen_full();
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

    spr_raw_api();
    spr_action_fsm();
    spr_physics();
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

/* Call this to clear all graphics, sprites, and physics state */
void NEOGEO_USER ng_clear_screen_full(void)
{
    /* Remove all physics solids */
    ng_physics_clear_solids();

    /* Reset software character pool */
    ng_chars_init();

    /* Clear the FIX layer and set a black backdrop */
    clearFix();
    setBACKDROP(BLACK);

    /* HARD CLEAR: zero SCB3 height for all 380 hardware sprite slots */
    ng_sprite_hide_range(0,   255u);
    ng_sprite_hide_range(255, 125u);

    /*
     * Push one blank frame: render the empty sprite list
     * so any hardware state left in VRAM is no longer drawn.
     */
    ng_chars_draw();
    demo_frame();
}