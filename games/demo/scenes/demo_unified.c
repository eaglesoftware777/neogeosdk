/*
 * demo_unified.c — One linear showcase of the sdk/2d_engine_plus engine.
 *
 *   Eagle Software — https://eaglesoftware.biz
 *
 * Fourteen chapters run back-to-back. Press A to skip to the next
 * chapter at any time. Hold D to toggle a one-line perf HUD on the
 * top-right of the FIX layer.
 */

#include "demo_unified.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_engine.h"
#include <stddef.h>
#include <stdint.h>

/* External hardware helpers from neogeolib */
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
/*  Perf HUD: held-D shows a 1-line counter overlay                      */
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
    char ch[4], np[4];

    if (!d_held()) {
        if (shown) {
            demo_fix_puts(22u, 0u, "                 ", 0u);
            shown = 0u;
        }
        return;
    }

    digit3(ch, (uint16_t)ng_chars_count());
    digit3(np, (uint16_t)ng_particles_count());

    demo_fix_puts(22u, 0u, "C:",         2u);
    demo_fix_puts(24u, 0u, ch,           1u);
    demo_fix_puts(28u, 0u, "P:",         2u);
    demo_fix_puts(30u, 0u, np,           1u);
    shown = 1u;
}

/*
 * uframe — wraps demo_frame so every chapter gets the perf HUD and
 * the same A-press semantics. Returns 1 when A was pressed this frame.
 */
static uint8_t NEOGEO_USER uframe(void)
{
    perf_hud_draw();
    return demo_frame();
}

static uint8_t NEOGEO_USER uwait(uint16_t frames)
{
    uint16_t t;
    for (t = 0u; t < frames; t++) {
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Chapter scaffolding                                                  */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER chap_header(uint8_t n,
                                    const char *title,
                                    const char *subtitle)
{
    char tag[6];

    clearFix();
    setBACKDROP(BLACK);

    tag[0] = 'C';
    tag[1] = 'H';
    tag[2] = '.';
    tag[3] = (char)('0' + (n / 10u));
    tag[4] = (char)('0' + (n % 10u));
    tag[5] = '\0';

    demo_fix_puts(2u,  0u, tag,      0u);
    demo_fix_puts(8u,  0u, title,    2u);
    if (subtitle) demo_fix_puts(2u, 1u, subtitle, 1u);
    demo_fix_puts(2u, 27u, "A: NEXT  HOLD D: PERF", 0u);
}

static void NEOGEO_USER chap_music(uint8_t track)
{
    soundStopAll();
    soundPlayGameLoop(track);
}

/* ------------------------------------------------------------------ */
/*  Static base palette used by chap_palette_fx                          */
/* ------------------------------------------------------------------ */
static const uint16_t s_palfx_base[16] = {
    0x0000, /* 0 transparent */
    0x7000, 0x7800, 0x7C00, 0x7E00, 0x7F00, 0x7F80, 0x7FC0,
    0x7FE0, 0x07E0, 0x03E0, 0x001F, 0x021F, 0x041F, 0x081F, 0x7FFF
};

/* ------------------------------------------------------------------ */
/*  Chapter 0 — boot: FIX bring-up                                       */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER chap_boot(void)
{
    chap_header(0u, "BOOT", "FIX LAYER ONLINE");
    soundStopAll();

    demo_fix_puts(2u,  5u, "EAGLE SOFTWARE NEOGEO SDK", 1u);
    demo_fix_puts(2u,  6u, "(C) 2026  EAGLESOFTWARE.BIZ", 0u);
    demo_fix_puts(2u,  9u, "UNIFIED ENGINE SHOWCASE", 2u);
    demo_fix_puts(2u, 11u, "14 CHAPTERS  ~3 MINUTES", 1u);
    demo_fix_puts(2u, 13u, "PRESS A TO ADVANCE", 0u);

    return uwait(180u);
}

/* ------------------------------------------------------------------ */
/*  Chapter 1 — title: animated parade reel + palette fade-in            */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER chap_title(void)
{
    static const uint8_t parade[] = {
        79u, 80u, 81u, 82u, 83u, 84u, 85u, 86u,
        87u, 88u, 89u, 90u, 91u, 92u
    };
    uint16_t t;

    chap_header(1u, "TITLE", "ANIMATED REEL + FADE-IN");
    chap_music(SOUND_MUSIC_EAGLE_FANFARE);

    for (t = 0u; t < 240u; t++) {
        uint8_t frame = (uint8_t)(t / 12u);
        if (frame >= (uint8_t)(sizeof(parade) / sizeof(parade[0])))
            frame = (uint8_t)((sizeof(parade) / sizeof(parade[0])) - 1u);

        demo_load_screen_palette(parade[frame]);
        demo_draw_sprite_screen(parade[frame], 1u, 60, -16,
                                16u, 16u, 0xFFu, 0xFFu);
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Chapter 2 — FIX layer: text, palettes, dirty cache                   */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER chap_fix(void)
{
    uint16_t t;
    char buf[8];

    chap_header(2u, "FIX LAYER", "TEXT  PALETTES  DIRTY CACHE");
    chap_music(SOUND_MUSIC_SHOP_JINGLE);

    demo_fix_puts(2u,  5u, "PAL 0  STANDARD",   0u);
    demo_fix_puts(2u,  6u, "PAL 1  ACCENT",     1u);
    demo_fix_puts(2u,  7u, "PAL 2  WARN",       2u);

    demo_fix_puts(2u, 10u, "DIRTY CACHE PROOF:",        1u);
    demo_fix_puts(2u, 11u, "OVERWRITING SAME CELL...",  0u);

    for (t = 0u; t < 240u; t++) {
        digit3(buf, t);
        demo_fix_puts(2u, 13u, buf, (uint8_t)((t / 30u) % 3u));
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Chapter 3 — sprite groups: hardware scale matrix + flip              */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER chap_sprite(void)
{
    /*
     * Four copies of the eagle (96x96 px at 100 %) rendered with
     * hardware shrink — SCB2 high nibble for X, low byte for Y.
     * Bottoms aligned to a common floor line so the size difference
     * is obvious.  One copy has horizontal flip.
     */
    static const uint8_t  s_scales[4]  = { 0xFFu, 0xC8u, 0x96u, 0x64u };
    static const int16_t  s_xpos[4]    = {  -8,    82,   171,   261 };
    static const int16_t  s_ytop[4]    = {  96,   117,   135,   155 };
    static const uint16_t s_slots[4]   = { 300u, 312u, 324u, 336u };
    static const char    *s_pct[4]     = { "100%", " 78%", " 59%", " 39%" };

    uint16_t  t;
    NGSpriteGroup g;
    uint8_t   i;
    const uint8_t frame = 73u; /* grounded eagle: 6 strips x 6 rows */
    uint16_t  tile;
    uint8_t   pal;

    chap_header(3u, "SPRITE GROUP", "HARDWARE SCALE MATRIX");

    /* Identification banner on the top FIX rows. */
    demo_fix_puts(2u,  2u, "4 EAGLES, ONE TILE SET",                  1u);
    demo_fix_puts(2u,  3u, "X+Y SHRINK VIA SCB2  CHAIN VIA SCB3",     0u);
    demo_fix_puts(2u,  4u, "SCALES BELOW EACH SPRITE",                1u);
    /* Per-slot scale labels lined up over each shrunk sprite. */
    demo_fix_puts(3u,   6u, s_pct[0], 2u);
    demo_fix_puts(13u,  6u, s_pct[1], 2u);
    demo_fix_puts(23u,  6u, s_pct[2], 2u);
    demo_fix_puts(33u,  6u, s_pct[3], 2u);

    chap_music(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    demo_load_screen_palette(frame);
    tile = DEMO_SCREEN_TILE(frame);
    pal  = DEMO_SCREEN_PALETTE(frame);

    for (t = 0u; t < 240u; t++) {
        for (i = 0u; i < 4u; i++) {
            ng_sprite_group_init(&g, s_slots[i],
                                 (uint8_t)demo_screen_strips(frame),
                                 6u, tile, pal);
            ng_sprite_group_set_tile_stride(&g, 16u);
            ng_sprite_group_set_active_rows(&g, 6u);
            ng_sprite_group_set_scale(&g, s_scales[i], s_scales[i]);
            ng_sprite_group_set_flip(&g, (uint8_t)((i == 1u) ? 1u : 0u), 0u);
            ng_sprite_group_set_pos(&g, s_xpos[i], s_ytop[i]);
            ng_sprite_group_upload(&g);
        }

        if (uframe()) return 1u;
    }
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Chapter 4 — characters via action script DSL                         */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER chap_chars(void)
{
    /*
     * Single warrior cycling IDLE -> WALK -> ATTACK -> IDLE.
     * Sprite is 14 strips x 10 rows = 224 x 160 px at 100 %, anchored
     * bottom-centre so it sits centred on screen.
     */
    static const uint8_t idle_frames[1]    = { 13u };
    static const uint8_t walk_frames[4]    = { 15u, 16u, 17u, 18u };
    static const uint8_t attack_frames[5]  = { 26u, 27u, 28u, 29u, 30u };

    NGCharacter *hero;
    uint16_t t;
    uint8_t  prev_phase = 0xFFu;

    chap_header(4u, "CHARACTERS", "IDLE / WALK / ATTACK CYCLE");
    demo_fix_puts(2u, 2u, "WARRIOR ANIMATION STATES",      1u);
    demo_fix_puts(2u, 3u, "FRAME 13 / 15-18 / 26-30",      0u);
    /* live state label updated below */

    chap_music(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    ng_clear_screen_full();
    /* re-render the chapter header — ng_clear_screen_full clears FIX */
    chap_header(4u, "CHARACTERS", "IDLE / WALK / ATTACK CYCLE");
    demo_fix_puts(2u, 2u, "WARRIOR ANIMATION STATES",      1u);
    demo_fix_puts(2u, 3u, "FRAME 13 / 15-18 / 26-30",      0u);

    ng_chars_init();

    /* Bottom-centre anchor: x = screen centre 160, y = floor 192. */
    hero = chars_add(0u, 160, 192);
    if (!hero) return uwait(60u);

    demo_load_screen_palette(idle_frames[0]);
    ng_char_set_sprite(hero, 0u, 14u, 10u,
                       DEMO_SCREEN_TILE(idle_frames[0]),
                       DEMO_SCREEN_PALETTE(idle_frames[0]));
    ng_char_set_tile_stride(hero, 16u);
    hero->sprite_offset_x = (int16_t)(-(int16_t)(14 * 16 / 2)); /* -112 */
    hero->sprite_offset_y = (int16_t)(-(int16_t)(10 * 16));     /* -160 */

    for (t = 0u; t < 360u; t++) {
        uint8_t phase = (uint8_t)((t / 90u) & 3u);
        uint8_t frame;

        switch (phase) {
        case 0: frame = idle_frames[0]; break;
        case 1: frame = walk_frames[(t / 8u) % 4u]; break;
        case 2: frame = attack_frames[(t / 7u) % 5u]; break;
        default: frame = idle_frames[0]; break;
        }

        /* Update the upper-screen state caption whenever the phase changes. */
        if (phase != prev_phase) {
            demo_fix_puts(2u, 4u, "STATE:           ", 0u);
            switch (phase) {
            case 0: demo_fix_puts(2u, 4u, "STATE: IDLE    ", 1u); break;
            case 1: demo_fix_puts(2u, 4u, "STATE: WALK    ", 2u); break;
            case 2: demo_fix_puts(2u, 4u, "STATE: ATTACK  ", 2u); break;
            default: demo_fix_puts(2u, 4u, "STATE: IDLE    ", 1u); break;
            }
            prev_phase = phase;
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
        if (uframe()) { ng_clear_screen_full(); return 1u; }
    }

    ng_clear_screen_full();
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Chapter 5 — physics: gravity, drag, solids, jump-through             */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER chap_physics(void)
{
    NGCharacter *eagle;
    uint16_t t;
    uint8_t frame;

    chap_header(5u, "PHYSICS", "GRAVITY  SOLIDS  PLATFORMS");
    chap_music(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    ng_clear_screen_full();
    ng_chars_init();
    ng_physics_init();

    /* Solid floor + two stacked platforms */
    ng_physics_add_solid(0,   200, 320, 8, 0u);
    ng_physics_add_solid(60,  150, 120, 6, 0u);
    ng_physics_add_solid(160, 110, 100, 6, 0u);

    demo_fix_puts(2u, 23u, "EAGLE FALLS  BOUNCES  SETTLES", 1u);

    eagle = chars_add(0u, 80, 0);
    if (!eagle) return uwait(60u);

    frame = 76u;
    demo_load_screen_palette(frame);
    ng_char_set_sprite(eagle, 0u,
                       demo_screen_strips(frame), 6u,
                       DEMO_SCREEN_TILE(frame),
                       DEMO_SCREEN_PALETTE(frame));
    ng_char_set_tile_stride(eagle, 16u);
    eagle->sprite_offset_y = -48;
    eagle->scale_x = 0x80u;
    eagle->scale_y = 0x80u;

    ng_physics_attach(eagle,
        (uint16_t)(NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS));
    ng_physics_set_gravity(eagle,
        NG_FP_FROM_FRAC(1, 4), NG_TO_FP(4));

    for (t = 0u; t < 300u; t++) {
        uint8_t grounded;
        uint8_t want;

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
            eagle->sprite_strips = demo_screen_strips(frame);
            eagle->sprite_dirty  = 1u;
        }

        ng_chars_draw();
        if (uframe()) { ng_clear_screen_full(); return 1u; }
    }
    ng_clear_screen_full();
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Chapter 6 — camera: follow + look-ahead + shake                      */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER chap_camera(void)
{
    NGCamera cam;
    NGCharacter *hero;
    uint16_t t;
    int16_t target_vx;

    chap_header(6u, "CAMERA", "FOLLOW  LOOK-AHEAD  SHAKE");
    chap_music(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    ng_clear_screen_full();
    ng_chars_init();

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 640, 224);
    ng_camera_set_follow_speed(&cam, 8u);
    ng_camera_set_dead_zone(&cam, 32u, 16u);
    ng_camera_set_look_ahead(&cam, 24, 0, 4u);

    hero = chars_add(0u, 60, 160);
    if (!hero) return uwait(60u);
    demo_load_screen_palette(13u);
    ng_char_set_sprite(hero, 0u, 14u, 10u,
                       DEMO_SCREEN_TILE(13u),
                       DEMO_SCREEN_PALETTE(13u));
    ng_char_set_tile_stride(hero, 16u);
    hero->sprite_offset_x = -112;
    hero->sprite_offset_y = -160;

    for (t = 0u; t < 360u; t++) {
        target_vx = (int16_t)((t < 240u) ? 2 : 0);
        hero->x = (int16_t)(hero->x + target_vx);
        if (hero->x > 600) hero->x = 600;

        if (t == 120u) ng_camera_shake(&cam, 4u, 12u);
        if (t == 240u) ng_camera_shake(&cam, 3u, 10u);

        ng_camera_update(&cam, hero->x, hero->y, target_vx);
        ng_camera_apply(&cam, hero->x, hero->y, target_vx);

        ng_chars_draw();
        if (uframe()) { ng_clear_screen_full(); return 1u; }
    }
    ng_clear_screen_full();
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Chapter 7 — palette FX                                              */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER chap_palette_fx(void)
{
    uint8_t slot = 16u; /* arbitrary palette slot for the demo */
    uint16_t t;

    chap_header(7u, "PALETTE FX", "FADE  FLASH  PULSE  CYCLE");
    chap_music(SOUND_MUSIC_SHOP_JINGLE);

    ng_palette_fx_init();
    ng_palfx_upload_base(slot, s_palfx_base);

    /* Caption changes track which FX is active */
    demo_fix_puts(2u,  9u, "FADE-IN  60F",     1u);
    ng_palfx_fade_in(slot, s_palfx_base, 60u);
    if (uwait(80u)) return 1u;

    demo_fix_puts(2u,  9u, "FLASH WHITE  20F", 2u);
    ng_palfx_flash_white(slot, s_palfx_base, 20u);
    if (uwait(40u)) return 1u;

    demo_fix_puts(2u,  9u, "FLASH RED  20F  ", 2u);
    ng_palfx_flash_red(slot, s_palfx_base, 20u);
    if (uwait(40u)) return 1u;

    demo_fix_puts(2u,  9u, "PULSE  PERIOD 30 ", 1u);
    ng_palfx_pulse(slot, s_palfx_base, 30u);
    if (uwait(120u)) return 1u;

    demo_fix_puts(2u,  9u, "CYCLE  IDX 1..7  ", 1u);
    ng_palfx_cycle(slot, s_palfx_base, 1u, 7u);
    if (uwait(180u)) return 1u;

    demo_fix_puts(2u,  9u, "FADE-OUT  60F    ", 0u);
    ng_palfx_fade_out(slot, s_palfx_base, 60u);
    if (uwait(80u)) return 1u;

    ng_palfx_stop(slot);
    for (t = 0u; t < 30u; t++) if (uframe()) return 1u;
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Chapter 8 — particles: priority + budget                             */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER chap_particles(void)
{
    uint16_t tile_base = DEMO_SCREEN_TILE(13u);
    uint8_t  pal       = DEMO_SCREEN_PALETTE(13u);
    uint16_t t;

    chap_header(8u, "PARTICLES", "SPARK  DUST  EXPLOSION");
    chap_music(SOUND_MUSIC_BOSS_TENSION);

    ng_particles_init();
    demo_load_screen_palette(13u);

    demo_fix_puts(2u,  9u, "SPAWN: HIT SPARKS",  1u);

    for (t = 0u; t < 360u; t++) {
        int16_t x = (int16_t)(40 + ((t * 5u) & 0xFF));
        int16_t y = (int16_t)(60 + (((t * 7u) >> 1) & 0x3F));

        if ((t & 7u) == 0u)
            ng_spawn_hit_spark(x, y, tile_base, pal);
        if ((t & 15u) == 0u)
            ng_spawn_dust(x + 16, y + 16, tile_base, pal);
        if (t == 120u) {
            ng_spawn_explosion(160, 100, tile_base, pal);
            demo_fix_puts(2u,  9u, "EXPLOSION FIRED  ",  2u);
        }
        if (t == 240u) {
            ng_spawn_smoke(80, 80, tile_base, pal);
            ng_spawn_magic_spark(200, 90, tile_base, pal);
            demo_fix_puts(2u,  9u, "SMOKE + MAGIC    ",  1u);
        }

        ng_particles_update();
        ng_particles_draw(256u, 0u);
        if (uframe()) { ng_particles_init(); return 1u; }
    }
    ng_particles_init();
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Chapter 9 — feedback / impact composer                               */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER chap_feedback(void)
{
    NGCamera cam;
    uint16_t t;
    uint8_t  fired = 0u;

    chap_header(9u, "FEEDBACK", "HITSTOP  SHAKE  IMPACT");
    chap_music(SOUND_MUSIC_BOSS_TENSION);

    ng_feedback_init();
    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 320, 224);

    demo_fix_puts(2u,  9u, "WATCH THE FIX  IT JITTERS", 1u);

    for (t = 0u; t < 480u; t++) {
        if (t == 60u && fired < 1u) {
            ng_impact_event(NG_IMPACT_LIGHT, 0u, NULL,
                            &cam, 0u, 160, 100, 0u, 0u);
            demo_fix_puts(2u, 11u, "IMPACT LIGHT   ", 1u);
            fired = 1u;
        } else if (t == 180u && fired < 2u) {
            ng_impact_event(NG_IMPACT_MEDIUM, 0u, NULL,
                            &cam, 0u, 160, 100, 0u, 0u);
            demo_fix_puts(2u, 11u, "IMPACT MEDIUM  ", 2u);
            fired = 2u;
        } else if (t == 300u && fired < 3u) {
            ng_impact_event(NG_IMPACT_HEAVY, 0u, NULL,
                            &cam, 0u, 160, 100, 0u, 0u);
            demo_fix_puts(2u, 11u, "IMPACT HEAVY   ", 2u);
            fired = 3u;
        } else if (t == 420u && fired < 4u) {
            ng_impact_event(NG_IMPACT_BOSS, 0u, NULL,
                            &cam, 0u, 160, 100, 0u, 0u);
            demo_fix_puts(2u, 11u, "IMPACT BOSS    ", 2u);
            fired = 4u;
        }

        ng_feedback_update();
        ng_camera_apply(&cam, 160, 112, 0);

        if (uframe()) return 1u;
    }
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Chapter 10 — depthfx (2.5D)                                          */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER chap_depthfx(void)
{
    enum { N = 6 };
    NGVec3 p[N];
    NGSpriteGroup g;
    uint16_t t;
    uint8_t i;

    chap_header(10u, "DEPTH FX", "2.5D PROJECT + SHRINK");
    chap_music(SOUND_MUSIC_SAMURAI_ENDING_SCENE);

    ng_depthfx_init();

    for (i = 0u; i < (uint8_t)N; i++) {
        p[i].x = (int16_t)(40 + i * 40);
        p[i].y = 120;
        p[i].z = (int16_t)(i * 16);
    }

    demo_load_screen_palette(13u);

    for (t = 0u; t < 360u; t++) {
        uint16_t tile = DEMO_SCREEN_TILE(13u);
        uint8_t  pal  = DEMO_SCREEN_PALETTE(13u);

        for (i = 0u; i < (uint8_t)N; i++) {
            ng_depthfx_advance_star(&p[i], 1, 128, 160, 112);
            ng_sprite_group_init(&g, (uint16_t)(280u + i),
                                 4u, 4u, tile, pal);
            ng_sprite_group_set_tile_stride(&g, 16u);
            ng_depthfx_draw_group(&g, p[i], pal);
        }

        if (uframe()) return 1u;
    }
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Chapter 11 — NPC crowd: patrol + depth-sort                          */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER chap_npcs(void)
{
    enum { N = 5 };
    static const int16_t home_x[N] = { 30, 90, 150, 210, 270 };
    static const int16_t home_y[N] = { -40, -10, 20, 50, 80 };
    NGNpc *npcs[N];
    uint16_t t;
    uint8_t i;

    chap_header(11u, "NPCS", "PATROL  THINK  DEPTH SORT");
    chap_music(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    ng_clear_screen_full();
    ng_chars_init();
    ng_npcs_init();

    for (i = 0u; i < (uint8_t)N; i++) {
        NGCharacter *c;
        npcs[i] = npc_spawn(0u, 0u, home_x[i], home_y[i]);
        if (!npcs[i]) continue;

        c = npc_char(npcs[i]);
        if (!c) continue;

        demo_load_screen_palette(110u);
        ng_char_set_sprite(c, 0u, 12u, 15u,
                           DEMO_SCREEN_TILE(110u),
                           DEMO_SCREEN_PALETTE(110u));
        ng_char_set_tile_stride(c, 16u);
        c->sprite_offset_y = (int16_t)(-240);

        ng_npc_set_home(npcs[i], home_x[i], home_y[i]);
        ng_npc_set_patrol_bounds(npcs[i],
                                 (int16_t)(home_x[i] - 24),
                                 (int16_t)(home_x[i] + 24),
                                 home_y[i], home_y[i]);
        npcs[i]->flags = NG_NPC_FLAG_PATROL_X | NG_NPC_FLAG_FACE_MOTION;
        ng_npc_set_think(npcs[i], ng_npc_think_patrol, 4u);
    }

    for (t = 0u; t < 360u; t++) {
        ng_npcs_update();
        ng_chars_update();
        ng_chars_draw();
        if (uframe()) { ng_clear_screen_full(); return 1u; }
    }
    ng_clear_screen_full();
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Chapter 12 — mini-game: 45 s playable integration                    */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER chap_mini_game(void)
{
    NGCamera cam;
    NGCharacter *hero;
    uint16_t t;
    uint16_t tile_spark = DEMO_SCREEN_TILE(13u);
    uint8_t  pal_spark  = DEMO_SCREEN_PALETTE(13u);
    uint8_t  hero_frame = 13u;
    char timer_buf[4];

    chap_header(12u, "MINI-GAME", "MOVE: PAD  JUMP: A IS HERE");
    demo_fix_puts(2u, 27u, "HOLD START TO QUIT EARLY", 0u);
    chap_music(SOUND_MUSIC_WARRIOR_BATTLE);

    ng_clear_screen_full();
    ng_chars_init();
    ng_physics_init();
    ng_particles_init();
    ng_feedback_init();

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 640, 224);
    ng_camera_set_follow_speed(&cam, 8u);
    ng_camera_set_dead_zone(&cam, 32u, 16u);

    ng_physics_add_solid(0,   200, 640, 8, 0u);
    ng_physics_add_solid(120, 150, 60,  6, 0u);
    ng_physics_add_solid(260, 120, 60,  6, 0u);
    ng_physics_add_solid(420, 150, 60,  6, 0u);

    hero = chars_add(0u, 40, 100);
    if (!hero) return uwait(60u);

    demo_load_screen_palette(hero_frame);
    ng_char_set_sprite(hero, 0u, 14u, 10u,
                       DEMO_SCREEN_TILE(hero_frame),
                       DEMO_SCREEN_PALETTE(hero_frame));
    ng_char_set_tile_stride(hero, 16u);
    hero->sprite_offset_x = -112;
    hero->sprite_offset_y = -160;

    ng_physics_attach(hero,
        (uint16_t)(NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS));
    ng_physics_set_gravity(hero,
        NG_FP_FROM_FRAC(1, 4), NG_TO_FP(4));

    for (t = 0u; t < (uint16_t)(60u * 45u); t++) {
        uint8_t pad = NEO_REGISTER8(BIOS_P1CURRENT);
        uint8_t edge = NEO_REGISTER8(BIOS_P1CHANGE) & NEO_REGISTER8(BIOS_P1CURRENT);
        int16_t vx = 0;

        if (pad & (1u << CNT_LEFT))  vx = -2;
        if (pad & (1u << CNT_RIGHT)) vx =  2;
        if ((edge & (1u << CNT_B)) && ng_physics_is_grounded(hero)) {
            ng_char_set_speed(hero, vx, -5);
            playSFX(SOUND_SFX_SHORT_SHOUT);
        }
        hero->x = (int16_t)(hero->x + vx);
        if (hero->x < 0)   hero->x = 0;
        if (hero->x > 600) hero->x = 600;

        /* Light "hit" feedback every second */
        if ((t % 60u) == 30u) {
            ng_impact_event(NG_IMPACT_LIGHT, 0u, NULL,
                            &cam, 0u,
                            (int16_t)(hero->x + 16),
                            (int16_t)(hero->y - 32),
                            tile_spark, pal_spark);
        }

        if (vx != 0) {
            uint8_t walk_frames[4] = { 15u, 16u, 17u, 18u };
            uint8_t want = walk_frames[(t / 6u) & 3u];
            if (want != hero_frame) {
                hero_frame = want;
                demo_load_screen_palette(hero_frame);
                hero->sprite_tile  = DEMO_SCREEN_TILE(hero_frame);
                hero->palette      = DEMO_SCREEN_PALETTE(hero_frame);
                hero->sprite_dirty = 1u;
            }
        } else if (hero_frame != 13u) {
            hero_frame = 13u;
            demo_load_screen_palette(hero_frame);
            hero->sprite_tile  = DEMO_SCREEN_TILE(hero_frame);
            hero->palette      = DEMO_SCREEN_PALETTE(hero_frame);
            hero->sprite_dirty = 1u;
        }

        ng_physics_update_pre();
        ng_chars_update();
        ng_physics_resolve();
        ng_camera_update(&cam, hero->x, hero->y, vx);
        ng_camera_apply(&cam, hero->x, hero->y, vx);

        ng_feedback_update();
        ng_particles_update();
        ng_particles_draw(256u, 0u);
        ng_chars_draw();

        digit3(timer_buf, (uint16_t)(45u - (t / 60u)));
        demo_fix_puts(34u, 0u, timer_buf, 1u);

        if (uframe()) { ng_clear_screen_full(); return 1u; }
        if (NEO_REGISTER8(BIOS_P1CURRENT) & (1u << CNT_START1)) {
            ng_clear_screen_full();
            return 1u;
        }
    }

    ng_clear_screen_full();
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Chapter 13 — credits                                                 */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER chap_credits(void)
{
    chap_header(13u, "CREDITS", NULL);
    chap_music(SOUND_MUSIC_ENDING_CREDITS);

    demo_fix_puts(2u,  4u, "ENGINE   EAGLE SOFTWARE 2026", 2u);
    demo_fix_puts(2u,  5u, "BUILD    SDK/2D_ENGINE_PLUS",  1u);
    demo_fix_puts(2u,  6u, "WEB      EAGLESOFTWARE.BIZ",   0u);

    demo_fix_puts(2u,  9u, "MODULES SHOWN:",       2u);
    demo_fix_puts(4u, 10u, "FIX  SPRITE  CHARS",   1u);
    demo_fix_puts(4u, 11u, "PHYSICS  CAMERA",      1u);
    demo_fix_puts(4u, 12u, "PALETTE FX  PARTICLES",1u);
    demo_fix_puts(4u, 13u, "FEEDBACK  DEPTH FX",   1u);
    demo_fix_puts(4u, 14u, "NPCS  MINI-GAME",      1u);

    demo_fix_puts(2u, 17u, "BEHIND THE SCENES:",                2u);
    demo_fix_puts(4u, 18u, "TIMERS  PROGRESS  STATUS",          0u);
    demo_fix_puts(4u, 19u, "PROPERTIES  EVENTS  BORDERS",       0u);
    demo_fix_puts(4u, 20u, "RENDER QUEUE  VBLANK FLUSH",        0u);

    demo_fix_puts(2u, 24u, "THANKS FOR PLAYING.",  2u);

    return uwait(360u);
}

/* ------------------------------------------------------------------ */
/*  Public entry                                                         */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_unified_run(void)
{
    /*
     * Each chapter's return value is "did the user press A?". We
     * don't honour it specifically (A already skipped the current
     * chapter): we just fall through to the next chapter in order.
     */
    (void)chap_boot();
    (void)chap_title();
    (void)chap_fix();
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
