/*
 * demo_unified.c — Linear showcase of the sdk/2d_engine_plus engine.
 *
 *   Eagle Software — https://eaglesoftware.biz
 *
 * Layout convention used by every chapter
 * ---------------------------------------
 *   FIX rows 0-1   chapter number + title + subtitle
 *   FIX rows 2-4   scene description / live state label
 *   FIX row 27     A: NEXT  HOLD D: PERF
 *   Screen Y       0..223 (FIX cells 8 px tall, 28 rows visible)
 *   "Floor"        screen Y = 192 — sprites bottom-align here
 *
 * Sprite Y math
 * -------------
 *   Engine setPos(x, y) places sprite TOP at screen y.
 *   For a sprite of rendered height H px:  top = floor - H.
 *
 * Every chapter starts with chap_header() which calls
 * ng_clear_screen_full() — that wipes SCB3 across all 380 hardware
 * sprite slots, resets the character pool and physics, clears FIX,
 * and sets the backdrop black.  No state leaks across chapters.
 */

#include "demo_unified.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_engine.h"
#include <stddef.h>
#include <stdint.h>

void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundPlayGameLoop(uint8_t track);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER soundSetSSGVolume(uint8_t v);
void NEOGEO_USER soundSetFMVolume(uint8_t v);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playVoiceCue(uint8_t n);
void NEOGEO_USER ng_clear_screen_full(void);

/* ------------------------------------------------------------------ */
/*  Shared screen geometry                                               */
/* ------------------------------------------------------------------ */
#define U_FLOOR_Y       192
#define U_CENTRE_X      160

/* ------------------------------------------------------------------ */
/*  Static base palette (used by palette FX + feedback chapters)         */
/* ------------------------------------------------------------------ */
static const uint16_t s_palfx_base[16] = {
    0x0000,
    0x7000, 0x7800, 0x7C00, 0x7E00, 0x7F00, 0x7F80, 0x7FC0,
    0x7FE0, 0x07E0, 0x03E0, 0x001F, 0x021F, 0x041F, 0x081F, 0x7FFF
};

/* ------------------------------------------------------------------ */
/*  Perf HUD  (top-right of FIX layer, only while D held)               */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER d_held(void)
{
    return (NEO_REGISTER8(BIOS_P1CURRENT) & (uint8_t)(1u << CNT_D)) ? 1u : 0u;
}

static void NEOGEO_USER digit3(char *buf, uint16_t n)
{
    if (n > 999u) n = 999u;
    buf[0] = (char)('0' + (n / 100u));
    buf[1] = (char)('0' + ((n / 10u) % 10u));
    buf[2] = (char)('0' + (n % 10u));
    buf[3] = '\0';
}

static void NEOGEO_USER perf_hud_draw(void)
{
    static uint8_t shown = 0u;
    char chrs[4], pts[4];

    if (!d_held()) {
        if (shown) {
            demo_fix_puts(22u, 0u, "                ", 0u);
            shown = 0u;
        }
        return;
    }

    digit3(chrs, (uint16_t)ng_chars_count());
    digit3(pts,  (uint16_t)ng_particles_count());
    demo_fix_puts(22u, 0u, "C:",  2u);
    demo_fix_puts(24u, 0u, chrs, 1u);
    demo_fix_puts(28u, 0u, "P:",  2u);
    demo_fix_puts(30u, 0u, pts,  1u);
    shown = 1u;
}

static uint8_t NEOGEO_USER uframe(void)
{
    perf_hud_draw();
    return demo_frame();
}

static uint8_t NEOGEO_USER uwait(uint16_t frames)
{
    uint16_t t;
    for (t = 0u; t < frames; t++)
        if (uframe()) return 1u;
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Chapter header: hard-clears EVERY subsystem and re-draws labels      */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER chap_header(uint8_t n,
                                    const char *title,
                                    const char *subtitle)
{
    char tag[6];

    /*
     * ng_clear_screen_full() does:
     *   - ng_physics_clear_solids()
     *   - ng_chars_init()             (resets the character pool)
     *   - clearFix() + setBACKDROP(BLACK)
     *   - SCB3 = 0 for all 380 sprite slots
     *   - ng_chars_draw() + demo_frame()  (one blank frame to flush)
     */
    ng_clear_screen_full();

    tag[0] = 'C';
    tag[1] = 'H';
    tag[2] = '.';
    tag[3] = (char)('0' + (n / 10u));
    tag[4] = (char)('0' + (n % 10u));
    tag[5] = '\0';

    demo_fix_puts(2u,  0u, tag,   0u);
    demo_fix_puts(8u,  0u, title, 2u);
    if (subtitle) demo_fix_puts(2u, 1u, subtitle, 1u);
    demo_fix_puts(2u, 27u, "A: NEXT  HOLD D: PERF", 0u);

    /*
     * Top-right scene number — same digits as the CH.NN tag but
     * mirrored on the right edge so it's always visible regardless
     * of what overlays the left side.
     */
    {
        char idx[3];
        idx[0] = (char)('0' + (n / 10u));
        idx[1] = (char)('0' + (n % 10u));
        idx[2] = '\0';
        demo_fix_puts(36u, 0u, idx, 2u);
    }
}

static void NEOGEO_USER chap_music(uint8_t track)
{
    soundStopAll();
    soundPlayGameLoop(track);
}

/* ================================================================== */
/*  Chapter 00 — Boot                                                    */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_boot(void)
{
    chap_header(0u, "BOOT", "ENGINE READY");
    soundStopAll();

    demo_fix_puts(2u,  3u, "EAGLE SOFTWARE NEOGEO SDK", 1u);
    demo_fix_puts(2u,  4u, "(C) 2026  EAGLESOFTWARE.BIZ", 0u);

    demo_fix_puts(2u,  7u, "UNIFIED ENGINE SHOWCASE", 2u);
    demo_fix_puts(2u,  9u, "15 CHAPTERS  ~4 MINUTES", 1u);

    demo_fix_puts(2u, 12u, "CONTROLS:",      2u);
    demo_fix_puts(4u, 13u, "A     SKIP CHAPTER",   1u);
    demo_fix_puts(4u, 14u, "B     ACTION (CH 13)", 1u);
    demo_fix_puts(4u, 15u, "PAD   MOVE  (CH 13)",  1u);
    demo_fix_puts(4u, 16u, "HOLD D  SHOW PERF HUD",1u);
    demo_fix_puts(4u, 17u, "START QUIT MINIGAME",  1u);

    return uwait(240u);
}

/* ================================================================== */
/*  Chapter 01 — Title  (animated parade reel)                           */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_title(void)
{
    static const uint8_t parade[] = {
        94u, 95u, 96u, 97u, 98u, 99u, 100u
    };
    const uint8_t N = (uint8_t)(sizeof(parade) / sizeof(parade[0]));
    uint16_t t;

    chap_header(1u, "TITLE", "ATTRACT REEL");
    demo_fix_puts(2u, 2u, "EYECATCHER FRAMES", 1u);
    demo_fix_puts(2u, 3u, "FULL SIZE CLEAN REEL", 0u);
    chap_music(SOUND_MUSIC_EAGLE_FANFARE);

    for (t = 0u; t < 280u; t++) {
        uint8_t frame = parade[(t / 12u) % N];
        demo_load_screen_palette(frame);
        demo_draw_sprite_screen(frame, 1u,
                                /*x*/ 32, /*y*/ 16,
                                /*cols*/ 16u, /*rows*/ 11u,
                                /*sx*/ 0xFFu, /*sy*/ 0xFFu);
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 02 — FIX layer                                               */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_fix(void)
{
    uint16_t t;
    char buf[8];

    chap_header(2u, "FIX LAYER", "TEXT  PALETTES  DIRTY CACHE");
    demo_fix_puts(2u, 2u, "FIX = 40x32 CELL OVERLAY", 1u);
    demo_fix_puts(2u, 3u, "DIRTY-CELL CACHE  NO TEAR", 0u);
    chap_music(SOUND_MUSIC_SHOP_JINGLE);

    demo_fix_puts(2u,  6u, "PALETTE 0  STANDARD",   0u);
    demo_fix_puts(2u,  7u, "PALETTE 1  ACCENT",     1u);
    demo_fix_puts(2u,  8u, "PALETTE 2  WARN",       2u);

    demo_fix_puts(2u, 11u, "OVERWRITING ROW 13 EACH FRAME:", 1u);
    demo_fix_puts(2u, 12u, "(WATCH: NO FLICKER)",            0u);

    for (t = 0u; t < 240u; t++) {
        digit3(buf, t);
        demo_fix_puts(2u, 13u, buf, (uint8_t)((t / 30u) % 3u));
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 03 — Sound  (music + SFX showcase)                           */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_sound(void)
{
    static const uint8_t  s_tracks[4]      = {
        SOUND_MUSIC_EAGLE_FANFARE,
        SOUND_MUSIC_SAMURAI_GAME_LOOP,
        SOUND_MUSIC_SAMURAI_BATTLE_LOOP,
        SOUND_MUSIC_BOSS_TENSION
    };
    static const char * const s_track_names[4] = {
        "EAGLE FANFARE   ",
        "SAMURAI GAME    ",
        "SAMURAI BATTLE  ",
        "BOSS TENSION    "
    };
    static const uint8_t s_sfx[6] = {
        SOUND_SFX_COIN_CHIME,
        SOUND_SFX_BLADE_WHOOSH,
        SOUND_SFX_IMPACT_HIT,
        SOUND_SFX_FOOTSTEP,
        SOUND_SFX_STRING_PHRASE,
        SOUND_SFX_LOW_DRUM
    };
    static const char * const s_sfx_names[6] = {
        "COIN CHIME      ",
        "BLADE WHOOSH    ",
        "IMPACT HIT      ",
        "FOOTSTEP        ",
        "STRING PHRASE   ",
        "LOW DRUM        "
    };
    uint8_t i;

    chap_header(3u, "SOUND", "MUSIC + SFX SAMPLES");
    demo_fix_puts(2u, 2u, "Z80 SOUND CPU  4 CHANNELS:",  1u);
    demo_fix_puts(2u, 3u, "FM  SSG  ADPCM-A  ADPCM-B",   0u);

    /* Music section */
    demo_fix_puts(2u,  7u, "MUSIC PLAYBACK:", 2u);
    for (i = 0u; i < 4u; i++) {
        char tag[4];
        digit3(tag, (uint16_t)(i + 1u));
        demo_fix_puts(2u, 8u, "TRACK ",        1u);
        demo_fix_puts(9u, 8u, tag + 2,         1u); /* show 1..4 */
        demo_fix_puts(2u, 9u, s_track_names[i], 2u);
        soundStopAll();
        soundPlayGameLoop(s_tracks[i]);
        if (uwait(150u)) return 1u;
    }

    soundStopAll();
    demo_fix_puts(2u, 8u, "                          ", 0u);
    demo_fix_puts(2u, 9u, "                          ", 0u);

    /* SFX section */
    demo_fix_puts(2u, 12u, "SFX TRIGGERS:", 2u);
    for (i = 0u; i < 6u; i++) {
        demo_fix_puts(2u, 13u, s_sfx_names[i], 1u);
        playSFX(s_sfx[i]);
        if (uwait(45u)) return 1u;
    }

    demo_fix_puts(2u, 13u, "                ", 0u);
    return 0u;
}

/* ================================================================== */
/*  Chapter 04 — Sprite group  (hardware scale matrix)                   */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_sprite(void)
{
    static const uint8_t screens[] = {
        101u, 102u, 103u, 104u, 105u, 106u, 107u
    };
    const uint8_t count = (uint8_t)(sizeof(screens) / sizeof(screens[0]));
    uint16_t t;

    chap_header(4u, "SPRITE SCREENS", "SCREENS 1 TO 7 PNG");
    demo_fix_puts(2u, 2u, "GENERATED SCREEN ASSETS", 1u);
    demo_fix_puts(2u, 3u, "SCREEN IDS 101..107",     0u);
    demo_fix_puts(2u, 4u, "CENTERED FULL FRAME REEL",1u);

    chap_music(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    for (t = 0u; t < 252u; t++) {
        uint8_t frame = screens[(t / 36u) % count];
        demo_draw_sprite_screen(frame, 1u, 32, -16,
                                demo_screen_strips(frame),
                                demo_screen_rows(frame),
                                0xFFu, 0xFFu);
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 05 — Characters  (warrior animation states)                  */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_chars(void)
{
    /*
     * Warrior at 100 % is 14 strips x 10 rows = 224 x 160 px.  At
     * 50 % shrink it is 112 x 80, which we anchor bottom-centre on
     * the floor line so the whole thing is visible and centred.
     */
    static const uint8_t idle_f[1]    = { 13u };
    static const uint8_t walk_f[4]    = { 15u, 16u, 17u, 18u };
    static const uint8_t atk_f[5]     = { 26u, 27u, 28u, 29u, 30u };

    NGCharacter *hero;
    uint16_t t;
    uint8_t prev_phase = 0xFFu;

    chap_header(5u, "CHARACTERS", "IDLE / WALK / ATTACK CYCLE");
    demo_fix_puts(2u, 2u, "NGCHARACTER + CHARMANAGER",     1u);
    demo_fix_puts(2u, 3u, "FRAMES 13 / 15-18 / 26-30",     0u);
    chap_music(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    hero = chars_add(0u, U_CENTRE_X, U_FLOOR_Y);
    if (!hero) return uwait(60u);

    demo_load_screen_palette(idle_f[0]);
    ng_char_set_sprite(hero, 0u, 14u, 10u,
                       DEMO_SCREEN_TILE(idle_f[0]),
                       DEMO_SCREEN_PALETTE(idle_f[0]));
    ng_char_set_tile_stride(hero, 16u);
    hero->scale_x = 0x80u;   /* 50 % */
    hero->scale_y = 0x80u;
    hero->sprite_offset_x = -56;  /* (14*16/2) at 50 % = 56 px */
    hero->sprite_offset_y = -80;  /* (10*16) at 50 % = 80 px   */

    for (t = 0u; t < 360u; t++) {
        uint8_t phase = (uint8_t)((t / 90u) & 3u);
        uint8_t frame;

        if (phase != prev_phase) {
            demo_fix_puts(2u, 4u, "STATE:           ", 0u);
            switch (phase) {
            case 0:  demo_fix_puts(2u, 4u, "STATE: IDLE    ", 1u); break;
            case 1:  demo_fix_puts(2u, 4u, "STATE: WALK    ", 2u); break;
            case 2:  demo_fix_puts(2u, 4u, "STATE: ATTACK  ", 2u); break;
            default: demo_fix_puts(2u, 4u, "STATE: IDLE    ", 1u); break;
            }
            prev_phase = phase;
        }

        switch (phase) {
        case 0:  frame = idle_f[0]; break;
        case 1:  frame = walk_f[(t / 8u) % 4u]; break;
        case 2:  frame = atk_f[(t / 7u) % 5u]; break;
        default: frame = idle_f[0]; break;
        }

        demo_load_screen_palette(frame);
        if (hero->sprite_tile != DEMO_SCREEN_TILE(frame) ||
            hero->palette     != DEMO_SCREEN_PALETTE(frame)) {
            hero->sprite_tile  = DEMO_SCREEN_TILE(frame);
            hero->palette      = DEMO_SCREEN_PALETTE(frame);
            hero->sprite_dirty = 1u;
        }

        if (phase == 1u && (t & 31u) == 0u) playSFX(SOUND_SFX_FOOTSTEP);
        if (phase == 2u && (t & 31u) == 0u) playSFX(SOUND_SFX_BLADE_WHOOSH);

        ng_chars_draw();
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 06 — Physics  (gravity + solid floor)                        */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_physics(void)
{
    NGCharacter *eagle;
    NGSpriteGroup g;
    uint16_t t;
    uint8_t frame;
    uint8_t old_strips = 0u;
    int16_t draw_x;
    int16_t draw_y;

    enum {
        EAGLE_FLY_FRAME    = 78u,
        EAGLE_GROUND_FRAME = 75u,
        EAGLE_STRIDE       = 16u,
        EAGLE_SCALE        = 0x80u,
        EAGLE_SLOT         = 256u,
        EAGLE_BODY_W       = 32,
        EAGLE_BODY_H       = 48
    };

    chap_header(6u, "PHYSICS", "GRAVITY  SOLIDS  GROUNDED");
    demo_fix_puts(2u, 2u, "NGPHYSICSBODY  GRAVITY 0.25",   1u);
    demo_fix_puts(2u, 3u, "FLOOR Y=192  EAGLE STARTS Y=40",0u);
    chap_music(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    /* Draw the floor as a FIX strip so the user sees what eagle lands on. */
    demo_fix_puts(0u, 24u, "================================================", 2u);

    ng_physics_init();
    ng_physics_add_solid(0, U_FLOOR_Y, 320, 8, 0u);

    frame = EAGLE_FLY_FRAME;
    eagle = chars_add(0u, U_CENTRE_X, 40);
    if (!eagle) return uwait(60u);

    ng_char_set_body(eagle,
                     -(int16_t)(EAGLE_BODY_W / 2),
                     -(int16_t)EAGLE_BODY_H,
                     EAGLE_BODY_W,
                     EAGLE_BODY_H);

    ng_physics_attach(eagle,
        (uint16_t)(NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS));
    ng_physics_set_gravity(eagle,
        NG_FP_FROM_FRAC(1, 4), NG_TO_FP(4));

    demo_load_screen_palette(frame);
    old_strips = (uint8_t)demo_screen_strips(frame);

    for (t = 0u; t < 300u; t++) {
        uint8_t grounded;
        uint8_t want;
        uint8_t strips;
        uint8_t rows;

        ng_physics_update_pre();
        ng_chars_update();
        ng_physics_resolve();

        grounded = ng_physics_is_grounded(eagle);
        want = grounded ? EAGLE_GROUND_FRAME : EAGLE_FLY_FRAME;
        if (want != frame) {
            frame = want;
            demo_load_screen_palette(frame);
        }

        demo_fix_puts(2u, 4u, grounded ? "STATE: GROUNDED" : "STATE: FALLING ",
                              grounded ? 2u : 1u);

        strips = (uint8_t)demo_screen_strips(frame);
        rows = (uint8_t)demo_screen_rows(frame);
        if (old_strips > strips)
            ng_sprite_hide_range((uint16_t)(EAGLE_SLOT + strips),
                                 (uint8_t)(old_strips - strips));
        old_strips = strips;

        draw_x = (int16_t)(eagle->x - (int16_t)((uint16_t)strips * 4u));
        draw_y = (int16_t)(eagle->y - (int16_t)((uint16_t)rows * 8u));

        ng_sprite_group_init(&g, EAGLE_SLOT, strips, rows,
                             DEMO_SCREEN_TILE(frame),
                             DEMO_SCREEN_PALETTE(frame));
        ng_sprite_group_set_tile_stride(&g, EAGLE_STRIDE);
        ng_sprite_group_set_active_rows(&g, rows);
        ng_sprite_group_set_scale(&g, EAGLE_SCALE, EAGLE_SCALE);
        ng_sprite_group_set_pos(&g, draw_x, draw_y);
        ng_sprite_group_upload(&g);

        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 07 — Camera  (follow + dead zone + shake)                    */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_camera(void)
{
    NGCamera cam;
    NGCharacter *hero;
    uint16_t t;

    chap_header(7u, "CAMERA", "FOLLOW  DEAD ZONE  SHAKE");
    demo_fix_puts(2u, 2u, "NGCAMERA SCROLLS WITH HERO", 1u);
    demo_fix_puts(2u, 3u, "T=120 + T=240 -> SHAKE",     0u);
    chap_music(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 640, 224);
    ng_camera_set_follow_speed(&cam, 8u);
    ng_camera_set_dead_zone(&cam, 32u, 16u);
    ng_camera_set_look_ahead(&cam, 24, 0, 4u);

    hero = chars_add(0u, 40, U_FLOOR_Y);
    if (!hero) return uwait(60u);

    demo_load_screen_palette(13u);
    ng_char_set_sprite(hero, 0u, 14u, 10u,
                       DEMO_SCREEN_TILE(13u),
                       DEMO_SCREEN_PALETTE(13u));
    ng_char_set_tile_stride(hero, 16u);
    hero->scale_x = 0x80u;
    hero->scale_y = 0x80u;
    hero->sprite_offset_x = -56;
    hero->sprite_offset_y = -80;

    for (t = 0u; t < 360u; t++) {
        int16_t vx = (int16_t)((t < 280u) ? 2 : 0);

        hero->x = (int16_t)(hero->x + vx);
        if (hero->x > 600) hero->x = 600;

        /* swap to walk frames while moving */
        if (vx != 0) {
            uint8_t wf[4] = { 15u, 16u, 17u, 18u };
            uint8_t want  = wf[(t / 6u) & 3u];
            if (hero->sprite_tile != DEMO_SCREEN_TILE(want)) {
                demo_load_screen_palette(want);
                hero->sprite_tile  = DEMO_SCREEN_TILE(want);
                hero->palette      = DEMO_SCREEN_PALETTE(want);
                hero->sprite_dirty = 1u;
            }
        }

        if (t == 120u) { ng_camera_shake(&cam, 4u, 12u); playSFX(SOUND_SFX_IMPACT_HIT); }
        if (t == 240u) { ng_camera_shake(&cam, 3u, 10u); playSFX(SOUND_SFX_IMPACT_HIT); }

        ng_camera_update(&cam, hero->x, hero->y, vx);
        ng_camera_apply(&cam, hero->x, hero->y, vx);

        ng_chars_draw();
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 08 — Palette FX  (target sprite recoloured live)             */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_palette_fx(void)
{
    /*
     * Draw the eagle once with palette slot = 16, which we own.
     * Apply each effect to slot 16, the sprite recolours live.
     */
    NGSpriteGroup g;
    const uint8_t  slot   = 16u;
    const uint8_t  frame  = 73u;
    const uint16_t tile   = DEMO_SCREEN_TILE(frame);
    const uint16_t group_slot = 280u;

    chap_header(8u, "PALETTE FX", "FADE  FLASH  PULSE  CYCLE");
    demo_fix_puts(2u, 2u, "EAGLE USES PALETTE SLOT 16",  1u);
    demo_fix_puts(2u, 3u, "FX MUTATES THAT SLOT LIVE",   0u);
    chap_music(SOUND_MUSIC_SHOP_JINGLE);

    ng_palette_fx_init();
    ng_palfx_upload_base(slot, s_palfx_base);

    /* Big centred eagle at 100 %.  96x96 px, top at 192-96=96. */
    ng_sprite_group_init(&g, group_slot,
                         (uint8_t)demo_screen_strips(frame),
                         6u, tile, slot);
    ng_sprite_group_set_tile_stride(&g, 16u);
    ng_sprite_group_set_scale(&g, 0xFFu, 0xFFu);
    ng_sprite_group_set_pos(&g, 112, 96);
    ng_sprite_group_upload(&g);

    demo_fix_puts(2u, 5u, "EFFECT: FADE-IN 60F  ", 1u);
    ng_palfx_fade_in (slot, s_palfx_base, 60u);
    if (uwait(80u)) return 1u;

    demo_fix_puts(2u, 5u, "EFFECT: FLASH WHITE  ", 2u);
    ng_palfx_flash_white(slot, s_palfx_base, 20u);
    if (uwait(40u)) return 1u;

    demo_fix_puts(2u, 5u, "EFFECT: FLASH RED    ", 2u);
    ng_palfx_flash_red  (slot, s_palfx_base, 20u);
    if (uwait(40u)) return 1u;

    demo_fix_puts(2u, 5u, "EFFECT: PULSE  P=30  ", 1u);
    ng_palfx_pulse(slot, s_palfx_base, 30u);
    if (uwait(150u)) return 1u;

    demo_fix_puts(2u, 5u, "EFFECT: CYCLE 1..7   ", 1u);
    ng_palfx_cycle(slot, s_palfx_base, 1u, 7u);
    if (uwait(180u)) return 1u;

    demo_fix_puts(2u, 5u, "EFFECT: FADE-OUT 60F ", 0u);
    ng_palfx_fade_out(slot, s_palfx_base, 60u);
    if (uwait(80u)) return 1u;

    ng_palfx_stop(slot);
    return 0u;
}

/* ================================================================== */
/*  Chapter 09 — Particles                                              */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_particles(void)
{
    const uint8_t effect_id = 93u;
    const uint16_t tile_base = DEMO_SCREEN_TILE(effect_id);
    const uint8_t pal = DEMO_SCREEN_PALETTE(effect_id);
    uint16_t t;
    char cnt[4];

    chap_header(9u, "PARTICLES", "POOL + PRIORITY BUDGET");
    demo_fix_puts(2u, 2u, "32 SLOT POOL  PRIORITY DROP", 1u);
    demo_fix_puts(2u, 3u, "SPAWN: SPARK DUST EXPL MAGIC",0u);
    demo_fix_puts(2u, 4u, "ACTIVE: ",                    2u);
    chap_music(SOUND_MUSIC_BOSS_TENSION);

    ng_particles_init();
    demo_load_screen_palette(effect_id);

    for (t = 0u; t < 360u; t++) {
        int16_t bx = (int16_t)(64 + (int16_t)((t * 3u) & 0x7Fu));
        int16_t by = (int16_t)(120 + (int16_t)(((t >> 2) & 7u) - 3u));

        if ((t % 8u) == 0u) {
            ng_particle_spawn(NG_PART_HIT_SPARK, NG_PART_PRI_CRITICAL,
                              bx, by,
                              0, -(2L << NG_FP_SHIFT),
                              24u, tile_base, pal, 1u, 1u);
            ng_particle_spawn(NG_PART_DUST, NG_PART_PRI_NORMAL,
                              (int16_t)(bx + 16), (int16_t)(by + 12),
                              -(1L << NG_FP_SHIFT), -(1L << (NG_FP_SHIFT - 1)),
                              28u, (uint16_t)(tile_base + 1u), pal, 1u, 1u);
            ng_particle_spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_NORMAL,
                              (int16_t)(bx - 16), (int16_t)(by + 8),
                              (1L << NG_FP_SHIFT), -(1L << NG_FP_SHIFT),
                              22u, (uint16_t)(tile_base + 2u), pal, 1u, 1u);
        }
        if (t == 80u) {
            ng_particle_spawn(NG_PART_EXPLOSION, NG_PART_PRI_CRITICAL,
                              160, 120,
                              0, 0,
                              36u, tile_base, pal, 1u, 1u);
            playSFX(SOUND_SFX_LOW_DRUM);
        }
        if (t == 200u) {
            ng_particle_spawn(NG_PART_SMOKE, NG_PART_PRI_OPTIONAL,
                              80, 100,
                              0, -(1L << (NG_FP_SHIFT - 1)),
                              45u, (uint16_t)(tile_base + 1u), pal, 1u, 1u);
            ng_particle_spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_NORMAL,
                              220, 100,
                              0, -(1L << NG_FP_SHIFT),
                              32u, (uint16_t)(tile_base + 2u), pal, 1u, 1u);
            playSFX(SOUND_SFX_STRING_PHRASE);
        }

        digit3(cnt, (uint16_t)ng_particles_count());
        demo_fix_puts(10u, 4u, cnt, 1u);

        ng_particles_draw(256u, 0u);
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 10 — Feedback  (impact composer on a target sprite)          */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_feedback(void)
{
    NGSpriteGroup target;
    NGCamera cam;
    const uint8_t  flash_slot = 16u;
    const uint8_t  frame      = 93u;
    const uint16_t tile       = DEMO_SCREEN_TILE(frame);
    const uint8_t  strips     = (uint8_t)demo_screen_strips(frame);
    const uint8_t  rows       = (uint8_t)demo_screen_rows(frame);

    uint16_t t;
    uint8_t  fired = 0u;

    chap_header(10u, "FEEDBACK", "HITSTOP  SHAKE  FLASH");
    demo_fix_puts(2u, 2u, "NG_IMPACT_EVENT COMPOSER",      1u);
    demo_fix_puts(2u, 3u, "LIGHT MED HEAVY BOSS",          0u);
    chap_music(SOUND_MUSIC_BOSS_TENSION);

    ng_feedback_init();
    ng_palette_fx_init();
    ng_palfx_upload_base(flash_slot, s_palfx_base);
    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 320, 224);

    demo_load_screen_palette(frame);
    ng_sprite_group_init(&target, 240u, strips, rows, tile, flash_slot);
    ng_sprite_group_set_tile_stride(&target, 16u);
    ng_sprite_group_set_active_rows(&target, rows);
    ng_sprite_group_set_scale(&target, 0xFFu, 0xFFu);
    ng_sprite_group_set_pos(&target, 136, 82);
    ng_sprite_group_upload(&target);

    for (t = 0u; t < 540u; t++) {
        if (t == 60u && fired < 1u) {
            ng_impact_event(NG_IMPACT_LIGHT,  flash_slot, s_palfx_base,
                            &cam, SOUND_SFX_IMPACT_HIT, 160, 120,
                            tile, flash_slot);
            demo_fix_puts(2u, 5u, "FIRED: LIGHT   ", 1u); fired = 1u;
        } else if (t == 180u && fired < 2u) {
            ng_impact_event(NG_IMPACT_MEDIUM, flash_slot, s_palfx_base,
                            &cam, SOUND_SFX_IMPACT_HIT, 160, 120,
                            tile, flash_slot);
            demo_fix_puts(2u, 5u, "FIRED: MEDIUM  ", 2u); fired = 2u;
        } else if (t == 320u && fired < 3u) {
            ng_impact_event(NG_IMPACT_HEAVY,  flash_slot, s_palfx_base,
                            &cam, SOUND_SFX_IMPACT_HIT, 160, 120,
                            tile, flash_slot);
            demo_fix_puts(2u, 5u, "FIRED: HEAVY   ", 2u); fired = 3u;
        } else if (t == 460u && fired < 4u) {
            ng_impact_event(NG_IMPACT_BOSS,   flash_slot, s_palfx_base,
                            &cam, SOUND_SFX_LOW_DRUM,    160, 120,
                            tile, flash_slot);
            demo_fix_puts(2u, 5u, "FIRED: BOSS    ", 2u); fired = 4u;
        }

        ng_camera_apply(&cam, 160, 112, 0);
        ng_particles_draw(256u, 0u);

        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 11 — Depth FX                                                */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_depthfx(void)
{
    enum { N = 5 };
    NGVec3 pts[N];
    NGSpriteGroup g;
    const uint8_t  frame = 73u;
    const uint16_t tile  = DEMO_SCREEN_TILE(frame);
    const uint8_t  pal   = DEMO_SCREEN_PALETTE(frame);
    uint16_t t;
    uint8_t i;

    chap_header(11u, "DEPTH FX", "2.5D Z PROJECTION");
    demo_fix_puts(2u, 2u, "NG_DEPTHFX SHRINKS WITH Z",   1u);
    demo_fix_puts(2u, 3u, "5 EAGLES AT STAGGERED DEPTH", 0u);
    chap_music(SOUND_MUSIC_SAMURAI_ENDING_SCENE);

    ng_depthfx_init();
    demo_load_screen_palette(frame);

    for (i = 0u; i < (uint8_t)N; i++) {
        pts[i].x = (int16_t)(40 + i * 50);
        pts[i].y = 140;
        pts[i].z = (int16_t)(i * 24);
    }

    for (t = 0u; t < 360u; t++) {
        for (i = 0u; i < (uint8_t)N; i++) {
            ng_depthfx_advance_star(&pts[i], 1, 128, 160, 100);
            ng_sprite_group_init(&g, (uint16_t)(280u + i * 8u),
                                 (uint8_t)demo_screen_strips(frame),
                                 6u, tile, pal);
            ng_sprite_group_set_tile_stride(&g, 16u);
            ng_depthfx_draw_group(&g, pts[i], pal);
        }
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 12 — NPCs  (patrol + depth sort)                             */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_npcs(void)
{
    enum { N = 5 };
    static const int16_t home_x[N] = { 50, 110, 170, 230, 290 };
    NGNpc *npcs[N];
    uint16_t t;
    uint8_t i;

    chap_header(12u, "NPCS", "PATROL  THINK  DEPTH SORT");
    demo_fix_puts(2u, 2u, "NPCMANAGER + THINK CALLBACKS", 1u);
    demo_fix_puts(2u, 3u, "5 CATS  AT 50% SCALE",          0u);
    chap_music(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    ng_npcs_init();

    for (i = 0u; i < (uint8_t)N; i++) {
        NGCharacter *c;

        npcs[i] = npc_spawn(0u, 0u, home_x[i], U_FLOOR_Y);
        if (!npcs[i]) continue;

        c = npc_char(npcs[i]);
        if (!c) continue;

        demo_load_screen_palette(110u);
        ng_char_set_sprite(c, 0u, 12u, 15u,
                           DEMO_SCREEN_TILE(110u),
                           DEMO_SCREEN_PALETTE(110u));
        ng_char_set_tile_stride(c, 16u);
        /* 12 strips * 15 rows = 192 x 240 -> 50 % -> 96 x 120 */
        c->scale_x = 0x80u;
        c->scale_y = 0x80u;
        c->sprite_offset_x = -48;
        c->sprite_offset_y = -120;

        ng_npc_set_home(npcs[i], home_x[i], U_FLOOR_Y);
        ng_npc_set_patrol_bounds(npcs[i],
                                 (int16_t)(home_x[i] - 28),
                                 (int16_t)(home_x[i] + 28),
                                 U_FLOOR_Y, U_FLOOR_Y);
        npcs[i]->flags = NG_NPC_FLAG_PATROL_X | NG_NPC_FLAG_FACE_MOTION;
        ng_npc_set_think(npcs[i], ng_npc_think_patrol, 4u);
    }

    for (t = 0u; t < 360u; t++) {
        ng_npcs_update();
        ng_chars_update();
        ng_chars_draw();
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 13 — Mini-game  (45 s integration)                           */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_mini_game(void)
{
    /*
     * Eagle on a scrolling floor.  Pad left/right moves, B jumps,
     * impact event every second.  Camera follows.  Timer in row 0.
     */
    NGCamera cam;
    NGCharacter *eagle;
    const uint16_t spark_tile = DEMO_SCREEN_TILE(73u);
    const uint8_t  spark_pal  = DEMO_SCREEN_PALETTE(73u);
    uint16_t t;
    uint8_t  frame = 76u;
    char timer[4];

    chap_header(13u, "MINI-GAME", "PLAY 45 SECONDS");
    demo_fix_puts(2u, 2u, "PAD MOVE  B JUMP  START QUIT", 1u);
    demo_fix_puts(2u, 3u, "ALL SYSTEMS INTEGRATED",        0u);
    chap_music(SOUND_MUSIC_WARRIOR_BATTLE);

    ng_physics_init();
    ng_particles_init();
    ng_feedback_init();

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 640, 224);
    ng_camera_set_follow_speed(&cam, 8u);
    ng_camera_set_dead_zone(&cam, 32u, 16u);

    /* World */
    ng_physics_add_solid(0,   U_FLOOR_Y, 640, 8, 0u);
    ng_physics_add_solid(160, 144, 80,  6, 0u);
    ng_physics_add_solid(360, 144, 80,  6, 0u);

    eagle = chars_add(0u, 60, 40);
    if (!eagle) return uwait(60u);

    demo_load_screen_palette(frame);
    ng_char_set_sprite(eagle, 0u,
                       (uint8_t)demo_screen_strips(frame), 6u,
                       DEMO_SCREEN_TILE(frame),
                       DEMO_SCREEN_PALETTE(frame));
    ng_char_set_tile_stride(eagle, 16u);
    eagle->scale_x = 0x80u;
    eagle->scale_y = 0x80u;
    eagle->sprite_offset_x = -24;
    eagle->sprite_offset_y = -48;
    ng_char_set_body(eagle, -16, -48, 32, 48);

    ng_physics_attach(eagle,
        (uint16_t)(NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS));
    ng_physics_set_gravity(eagle,
        NG_FP_FROM_FRAC(1, 4), NG_TO_FP(4));

    for (t = 0u; t < (uint16_t)(60u * 45u); t++) {
        uint8_t cur  = NEO_REGISTER8(BIOS_P1CURRENT);
        uint8_t prev = NEO_REGISTER8(BIOS_P1PREVIOUS);
        uint8_t edge = (uint8_t)(cur & ~prev);
        int16_t vx = 0;
        uint8_t grounded;
        uint8_t want;

        if (cur & (1u << CNT_LEFT))  vx = -2;
        if (cur & (1u << CNT_RIGHT)) vx =  2;
        eagle->x = (int16_t)(eagle->x + vx);
        if (eagle->x <   8) eagle->x =   8;
        if (eagle->x > 624) eagle->x = 624;

        if ((edge & (1u << CNT_B)) && ng_physics_is_grounded(eagle)) {
            ng_char_set_speed_fp(eagle, NG_TO_FP(vx), NG_FP_FROM_FRAC(-5, 1));
            playSFX(SOUND_SFX_SHORT_SHOUT);
        }

        ng_physics_update_pre();
        ng_chars_update();
        ng_physics_resolve();

        grounded = ng_physics_is_grounded(eagle);
        want = grounded ? (uint8_t)(73u + ((t / 6u) % 3u))
                        : (uint8_t)(76u + ((t / 8u) % 3u));
        if (want != frame) {
            frame = want;
            demo_load_screen_palette(frame);
            eagle->sprite_tile   = DEMO_SCREEN_TILE(frame);
            eagle->palette       = DEMO_SCREEN_PALETTE(frame);
            eagle->sprite_strips = (uint8_t)demo_screen_strips(frame);
            eagle->sprite_dirty  = 1u;
        }

        if ((t % 60u) == 0u && t > 0u) {
            ng_impact_event(NG_IMPACT_LIGHT, 0u, NULL,
                            &cam, SOUND_SFX_FOOTSTEP,
                            eagle->x, eagle->y - 24,
                            spark_tile, spark_pal);
        }

        ng_camera_update(&cam, eagle->x, eagle->y, vx);
        ng_camera_apply(&cam, eagle->x, eagle->y, vx);

        ng_feedback_update();
        ng_particles_update();
        ng_particles_draw(256u, 0u);
        ng_chars_draw();

        digit3(timer, (uint16_t)(45u - (t / 60u)));
        demo_fix_puts(33u, 1u, "T:",  2u);
        demo_fix_puts(35u, 1u, timer, 1u);

        if (uframe()) return 1u;
        if (cur & (1u << CNT_START1)) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 14 — Credits                                                 */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_credits(void)
{
    chap_header(14u, "CREDITS", "EAGLE SOFTWARE 2026");
    chap_music(SOUND_MUSIC_ENDING_CREDITS);

    demo_fix_puts(2u,  4u, "ENGINE   SDK/2D_ENGINE_PLUS", 1u);
    demo_fix_puts(2u,  5u, "BUILD    USE_2D_PLUS=1",      0u);
    demo_fix_puts(2u,  6u, "WEB      EAGLESOFTWARE.BIZ",  0u);

    demo_fix_puts(2u,  9u, "MODULES SHOWN:",        2u);
    demo_fix_puts(4u, 10u, "FIX  SPRITE  CHARS",    1u);
    demo_fix_puts(4u, 11u, "SOUND  PHYSICS  CAMERA",1u);
    demo_fix_puts(4u, 12u, "PALETTE FX  PARTICLES", 1u);
    demo_fix_puts(4u, 13u, "FEEDBACK  DEPTH FX",    1u);
    demo_fix_puts(4u, 14u, "NPCS  MINI-GAME",       1u);

    demo_fix_puts(2u, 17u, "BEHIND THE SCENES:",                 2u);
    demo_fix_puts(4u, 18u, "TIMERS  PROGRESS  STATUS",           0u);
    demo_fix_puts(4u, 19u, "PROPERTIES  EVENTS  BORDERS",        0u);
    demo_fix_puts(4u, 20u, "RENDER QUEUE  VBLANK FLUSH",         0u);

    demo_fix_puts(2u, 24u, "THANKS FOR PLAYING.", 2u);
    return uwait(360u);
}

/* ================================================================== */
/*  Public entry — 15-chapter linear flow                                */
/* ================================================================== */
void NEOGEO_USER demo_unified_run(void)
{
    (void)chap_boot();
    (void)chap_title();
    (void)chap_fix();
    (void)chap_sound();
    (void)chap_sprite();
    (void)chap_chars();
    (void)chap_physics();
    (void)chap_camera();
    (void)chap_palette_fx();
    (void)chap_particles();
    (void)chap_feedback();
    (void)chap_depthfx();
    (void)chap_npcs();
    (void)chap_mini_game();
    (void)chap_credits();
}
