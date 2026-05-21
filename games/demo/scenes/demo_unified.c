/*
 * demo_unified.c — Linear showcase of the sdk/2d_engine_plus engine.
 *
 *   Eagle Software — https://eaglesoftware.biz
 *
 * Layout convention used by every chapter
 * ---------------------------------------
 *   FIX rows 0-1   chapter number + title + subtitle (top-right also has CH##)
 *   FIX rows 2-4   scene description / live state label
 *   FIX row 27     reserved for scene status text ("A: NEXT")
 *   Screen Y       0..223 (FIX cells 8 px tall, 28 rows visible)
 *   "Floor"        screen Y = 192 — sprites bottom-align here
 *
 * Frame pacing
 * ------------
 *   uframe() is the *only* per-frame pump.  Order:
 *     1. caller computes the next scene state
 *     2. caller posts sprite/char draws (no VRAM writes yet)
 *     3. uframe():
 *          waitVbl                          // arrive at vblank start
 *          ng_chars_draw                    // upload chars in vblank window
 *          ng_particles_draw                // particles
 *          ng_render_queue_flush            // any queued writes
 *          ng_palette_fx_update             // palette FX state machine
 *          ng_particles_update              // particle TTL/motion
 *          ng_feedback_update               // hitstop / slow-mo
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
#include "sdk/ng_fix/ng_fix.h"
#include "sdk/2d_engine/ng_engine.h"
#include "sprite_meta.h"
#include <stddef.h>
#include <stdint.h>

void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER clearSprs(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundPlayGameLoop(uint8_t track);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER soundSetSSGVolume(uint8_t v);
void NEOGEO_USER soundSetFMVolume(uint8_t v);
void NEOGEO_USER soundSetTempo(uint8_t t);
void NEOGEO_USER soundFadeOutSpeed(uint8_t speed);
void NEOGEO_USER playFMTrack(uint8_t n);
void NEOGEO_USER playSSGTrack(uint8_t n);
void NEOGEO_USER soundSetSSGPreset(uint8_t preset);
void NEOGEO_USER playSFXB(uint8_t n);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playVoiceCue(uint8_t n);
void NEOGEO_USER ng_clear_screen_full(void);

#define U_FLOOR_Y       192
#define U_CENTRE_X      160

/*
 * Sprite slot priority on Neo Geo:  LOWER slot = drawn IN FRONT.
 * (See sdk/2d_engine_plus/ng_sprite_pool.hpp for the full layout.)
 *
 * For this demo:
 *   slot   1..15   = hero sprite group         (HERO_SLOT_FIRST = 1)
 *                    Drawn IN FRONT of everything else.
 *   slot  32..47   = enemy / target sprite group (ENEMY_SLOT_FIRST = 32)
 *                    Behind the hero, in front of NPCs/BG.
 *   slot  96..223  = NGCharacter system (NPCs / managed chars)
 *   slot 256..287  = particles (NG_SPR_PART_FIRST)
 *   slot 300..315  = BACKGROUND layer 0  (DEMO_BG_BACK_SLOT = 300)
 *                    Drawn BEHIND everything else.
 */
#define DEMO_BG_BACK_SLOT  NG_SPR_BG0_FIRST   /* 300 — back-most */
#define HERO_SLOT_FIRST    1u                 /* front-most */
#define ENEMY_SLOT_FIRST   32u                /* between hero and BG */

/* Base palette used by palette FX + feedback chapters. */
static const uint16_t s_palfx_base[16] = {
    0x0000,
    0x7000, 0x7800, 0x7C00, 0x7E00, 0x7F00, 0x7F80, 0x7FC0,
    0x7FE0, 0x07E0, 0x03E0, 0x001F, 0x021F, 0x041F, 0x081F, 0x7FFF
};

/* ------------------------------------------------------------------ */
/*  Shared state used by uframe (so it knows what to flush)              */
/* ------------------------------------------------------------------ */
static uint8_t  s_draw_chars      = 0u;
static uint8_t  s_draw_particles  = 0u;

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
    /* Reserved row 0 cols 22..32 if D-held in future — keep no-op for now. */
}

/*
 * Single per-frame pump.  All VRAM writes happen *inside* the vblank
 * window between waitVbl() and ng_render_queue_flush().
 */
static uint8_t NEOGEO_USER uframe(void)
{
    perf_hud_draw();
    waitVbl();                          /* arrive at vblank start */
    if (s_draw_chars)     ng_chars_draw();
    if (s_draw_particles) ng_particles_draw(NG_SPR_PART_FIRST, 0u);
    ng_render_queue_flush();
    ng_palette_fx_update();
    ng_particles_update();
    ng_feedback_update();
    return demo_advance_requested();
}

static uint8_t NEOGEO_USER uwait(uint16_t frames)
{
    uint16_t t;
    for (t = 0u; t < frames; t++)
        if (uframe()) return 1u;
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Sound helpers — every Z80 command separated by one vblank so the    */
/*  driver has time to process without queue collisions.                */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER snd_step(void)
{
    waitVbl();
}

static void NEOGEO_USER snd_cross_to(uint8_t track)
{
    soundFadeOutSpeed(6u);
    (void)uwait(12u);
    soundStopAll();
    snd_step();
    soundPlayGameLoop(track);
    snd_step();
}

static void NEOGEO_USER snd_silence(void)
{
    soundFadeOutSpeed(8u);
    (void)uwait(10u);
    soundStopAll();
    snd_step();
}

/* ------------------------------------------------------------------ */
/*  Chapter header: hard-clears EVERY subsystem and re-draws labels      */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER chap_header(uint8_t n,
                                    const char *title,
                                    const char *subtitle)
{
    char tag[6];

    /* hard reset hardware + char/physics/particles/feedback/palette FX */
    ng_clear_screen_full();
    /*
     * clearSprs wipes ALL FOUR SCB tables (not only SCB3 heights), so no
     * leftover tile pointers or X coords from previous chapter can
     * "stick" onto a freshly-bound sprite in the new chapter.
     */
    clearSprs();
    ng_level_set_scroll(0, 0);
    ng_particles_init();
    ng_feedback_init();
    ng_palette_fx_init();

    /* per-chapter flush flags — chapters that need chars/particles set them */
    s_draw_chars     = 0u;
    s_draw_particles = 0u;

    tag[0] = 'C';
    tag[1] = 'H';
    tag[2] = '.';
    tag[3] = (char)('0' + (n / 10u));
    tag[4] = (char)('0' + (n % 10u));
    tag[5] = '\0';

    demo_fix_puts(2u,  0u, tag,   0u);
    demo_fix_puts(8u,  0u, title, 2u);
    if (subtitle) demo_fix_puts(2u, 1u, subtitle, 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    {
        char idx[3];
        idx[0] = (char)('0' + (n / 10u));
        idx[1] = (char)('0' + (n % 10u));
        idx[2] = '\0';
        demo_fix_puts(36u, 0u, idx, 2u);
    }
}

/*
 * Tracks the currently-loaded palette slot per character so we only
 * reload when the asset palette actually changes.  Per-char palette
 * memo: index is the demo char slot used by chars_at().
 */
static uint8_t s_last_palette_for_char[NG_MAX_CHARS];

static void NEOGEO_USER reset_palette_memo(void)
{
    uint8_t i;
    for (i = 0u; i < NG_MAX_CHARS; i++) s_last_palette_for_char[i] = 0xFFu;
}

static void NEOGEO_USER bind_character_asset(NGCharacter *c,
                                             uint8_t frame,
                                             uint8_t scale_x,
                                             uint8_t scale_y)
{
    uint16_t first;
    uint8_t strips;
    uint8_t rows;
    uint8_t pal;
    uint8_t idx;
    uint16_t anchor_x;
    uint16_t anchor_y;

    if (!c) return;

    first = c->sprite_first;
    if (first == 0xffffu) first = 0u;

    strips = (uint8_t)demo_screen_strips(frame);
    rows   = (uint8_t)demo_screen_rows(frame);
    pal    = DEMO_SCREEN_PALETTE(frame);

    /* only reload palette when the asset palette slot actually changes */
    idx = ng_chars_index(c);
    if (idx < NG_MAX_CHARS && s_last_palette_for_char[idx] != pal) {
        demo_load_screen_palette(frame);
        s_last_palette_for_char[idx] = pal;
    }

    ng_char_set_sprite(c, first, strips, rows,
                       DEMO_SCREEN_TILE(frame), pal);
    ng_char_set_tile_stride(c, 16u);

    c->scale_x = scale_x;
    c->scale_y = scale_y;

    /*
     * GRID anchor (not meta x_pad/y_pad).
     * Animation frames have different content_height/y_pad in the meta
     * table (e.g. idle frame 14 has content_height=96, attack frame 30
     * has 140) — using those values shifts the sprite by ~22 px between
     * frames and produces the "feet detach from body" glitch during an
     * attack swing.  Using rows*16 (full grid) anchors every frame to
     * the same bottom-centre point: foot stays planted, swing only
     * extends UP.
     */
    anchor_x = (uint16_t)(((uint16_t)strips * 16u) >> 1);
    anchor_y = (uint16_t)((uint16_t)rows * 16u);

    c->sprite_offset_x = -(int16_t)((anchor_x * scale_x) >> 8);
    c->sprite_offset_y = -(int16_t)((anchor_y * scale_y) >> 8);
}

static void NEOGEO_USER draw_infix_block(uint16_t tile_base,
                                         uint8_t cols,
                                         uint8_t rows,
                                         uint8_t x,
                                         uint8_t y,
                                         uint8_t pal)
{
    uint8_t row;
    uint8_t col;

    for (row = 0u; row < rows; row++) {
        for (col = 0u; col < cols; col++) {
            if ((uint8_t)(x + col) < 40u && (uint8_t)(y + row) < 28u) {
                ngfix_write_tile((uint8_t)(x + col),
                                 (uint8_t)(y + row),
                                 (uint16_t)(tile_base + (uint16_t)row * cols + col),
                                 pal);
            }
        }
    }
}

static void NEOGEO_USER draw_background(uint8_t frame, int16_t x, int16_t y)
{
    /* Low slot so it sits BEHIND chars (96+) on real hardware. */
    demo_draw_sprite_screen(frame, DEMO_BG_BACK_SLOT, x, y,
                            demo_screen_strips(frame),
                            demo_screen_rows(frame),
                            0xFFu, 0xFFu);
}

static void NEOGEO_USER spawn_impact_burst(int16_t x,
                                           int16_t y,
                                           uint16_t tile_base,
                                           uint8_t pal,
                                           uint8_t power)
{
    static const int8_t vx[8] = { -3, -2, -1, 0, 1, 2, 3, 0 };
    static const int8_t vy[8] = { -1, -3, -4, -5, -4, -3, -1, 2 };
    uint8_t i;
    uint8_t count = (uint8_t)(4u + power);
    if (count > 8u) count = 8u;

    for (i = 0u; i < count; i++) {
        ng_particle_spawn((i & 1u) ? NG_PART_MAGIC_SPARK : NG_PART_HIT_SPARK,
                          (i < 4u) ? NG_PART_PRI_CRITICAL : NG_PART_PRI_NORMAL,
                          x, y,
                          ((int32_t)vx[i] << NG_FP_SHIFT),
                          ((int32_t)vy[i] << NG_FP_SHIFT),
                          (uint8_t)(18u + power * 4u),
                          (uint16_t)(tile_base + (uint16_t)(i % 3u)),
                          pal,
                          1u,
                          1u);
    }
}

/* ================================================================== */
/*  Chapter 00 — Boot                                                    */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_boot(void)
{
    chap_header(0u, "BOOT", "ENGINE READY");
    snd_silence();

    demo_fix_puts(2u,  3u, "EAGLE SOFTWARE NEOGEO SDK", 1u);
    demo_fix_puts(2u,  4u, "(C) 2026  EAGLESOFTWARE.BIZ", 0u);

    demo_fix_puts(2u,  7u, "UNIFIED ENGINE SHOWCASE", 2u);
    demo_fix_puts(2u,  9u, "20 CHAPTERS  FULL ENGINE TOUR", 1u);

    demo_fix_puts(2u, 12u, "SHOWCASE FLOW:",         2u);
    demo_fix_puts(4u, 13u, "AUTOMATIC CHAPTERS",     1u);
    demo_fix_puts(4u, 14u, "SPRITES AND EFFECTS",    1u);
    demo_fix_puts(4u, 15u, "PHYSICS AND CAMERA",     1u);
    demo_fix_puts(4u, 16u, "NPC + PATROL + DEPTH",   1u);
    demo_fix_puts(4u, 17u, "JOYSTICK + MINI-GAME",   1u);

    return uwait(180u);
}

/* ================================================================== */
/*  Chapter 01 — Title (animated parade reel, stable centring)          */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_title(void)
{
    static const uint8_t parade[] = { 94u, 95u, 96u, 97u, 98u, 99u, 100u };
    const uint8_t N = (uint8_t)(sizeof(parade) / sizeof(parade[0]));
    uint16_t t;
    uint8_t last_frame = 0xFFu;

    chap_header(1u, "TITLE", "ATTRACT REEL");
    demo_fix_puts(2u, 2u, "TITLE ART + EYECATCHER", 1u);
    demo_fix_puts(2u, 3u, "STABLE CENTERED REEL",   0u);
    snd_cross_to(SOUND_MUSIC_EAGLE_FANFARE);

    for (t = 0u; t < 360u; t++) {
        uint8_t frame;
        if (t < 90u)        frame = 108u;
        else if (t < 300u)  frame = parade[((t - 90u) / 30u) % N];
        else                frame = 109u;

        if (frame != last_frame) {
            demo_draw_sprite_screen(frame, 1u, 32, 16,
                                    demo_screen_strips(frame),
                                    demo_screen_rows(frame),
                                    0xFFu, 0xFFu);
            last_frame = frame;
        }
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 02 — FIX layer  (no mid-chapter screen flashes)              */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_fix(void)
{
    uint16_t t;
    char buf[8];

    chap_header(2u, "FIX LAYER", "TEXT  PALETTES  DIRTY CACHE");
    demo_fix_puts(2u, 2u, "FIX = 40x32 CELL OVERLAY", 1u);
    demo_fix_puts(2u, 3u, "DIRTY-CELL CACHE  NO TEAR", 0u);
    snd_cross_to(SOUND_MUSIC_SHOP_JINGLE);

    demo_fix_puts(2u,  6u, "PALETTE 0  STANDARD",   0u);
    demo_fix_puts(2u,  7u, "PALETTE 1  ACCENT",     1u);
    demo_fix_puts(2u,  8u, "PALETTE 2  WARN",       2u);

    demo_fix_puts(2u, 11u, "OVERWRITING ROW 13 EACH FRAME:", 1u);
    demo_fix_puts(2u, 12u, "(WATCH: NO FLICKER)",            0u);

    for (t = 0u; t < 180u; t++) {
        digit3(buf, t);
        demo_fix_puts(2u, 13u, buf, (uint8_t)((t / 30u) % 3u));
        if (uframe()) return 1u;
    }

    /* Clear only the working rows — keep header / scene number stable */
    {
        uint8_t r;
        for (r = 5u; r < 27u; r++) demo_fix_puts(0u, r, "                                        ", 0u);
    }

    demo_fix_puts(2u, 2u, "INFIX TILE BANK PAGE 1     ", 1u);
    draw_infix_block(256u, 20u, 4u,  2u,  5u, 1u);
    draw_infix_block(476u,  8u, 2u, 26u,  6u, 2u);
    draw_infix_block(416u, 20u, 3u,  2u, 10u, 1u);
    draw_infix_block(492u, 32u, 4u,  4u, 14u, 2u);
    draw_infix_block(336u, 40u, 2u,  0u, 20u, 1u);
    if (uwait(120u)) return 1u;

    {
        uint8_t r;
        for (r = 5u; r < 27u; r++) demo_fix_puts(0u, r, "                                        ", 0u);
    }
    demo_fix_puts(2u, 2u, "INFIX TILE BANK PAGE 2     ", 1u);
    draw_infix_block(620u, 24u, 5u,  2u,  5u, 1u);
    draw_infix_block(740u, 16u, 2u, 24u,  6u, 2u);
    draw_infix_block(772u, 32u, 4u,  4u, 11u, 2u);
    draw_infix_block(900u, 12u, 4u,  2u, 17u, 1u);
    draw_infix_block(948u, 32u, 5u,  4u, 22u, 1u);
    if (uwait(120u)) return 1u;

    {
        uint8_t r;
        for (r = 5u; r < 27u; r++) demo_fix_puts(0u, r, "                                        ", 0u);
    }
    demo_fix_puts(2u, 2u, "SFIX TILE SHEET            ", 1u);
    draw_infix_block(1108u, 16u, 16u, 12u, 6u, 1u);
    if (uwait(120u)) return 1u;

    return 0u;
}

/* ================================================================== */
/*  Chapter 03 — Sound (vblank-spaced Z80 commands; no FM noise)         */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_sound(void)
{
    static const uint8_t  s_tracks[4] = {
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
        SOUND_SFX_COIN_CHIME, SOUND_SFX_BLADE_WHOOSH,
        SOUND_SFX_IMPACT_HIT, SOUND_SFX_FOOTSTEP,
        SOUND_SFX_STRING_PHRASE, SOUND_SFX_LOW_DRUM
    };
    static const char * const s_sfx_names[6] = {
        "COIN CHIME      ", "BLADE WHOOSH    ",
        "IMPACT HIT      ", "FOOTSTEP        ",
        "STRING PHRASE   ", "LOW DRUM        "
    };
    uint8_t i;

    chap_header(3u, "SOUND", "MUSIC + SFX SAMPLES");
    demo_fix_puts(2u, 2u, "Z80 SOUND CPU  4 CHANNELS:",  1u);
    demo_fix_puts(2u, 3u, "FM  SSG  ADPCM-A  ADPCM-B",   0u);

    /* --- FM patch demo ------------------------------------------------ */
    demo_fix_puts(2u, 5u, "FM PATCH                       ", 2u);
    soundStopAll();                          snd_step();
    soundSceneReset();                       snd_step();
    soundApplyMix(0x00u, 0x00u, 0x00u, 0x0Du); snd_step();
    soundSetTempo(5u);                       snd_step();
    soundSetFMVolume(0x0Du);                 snd_step();
    playFMTrack(SOUND_FM_PATCH_SHOWCASE);    snd_step();
    demo_fix_puts(2u, 6u, "PATCH BANK SHOWCASE            ", 1u);
    if (uwait(140u)) return 1u;

    /* --- 4 music tracks, each with a real fade between -------------- */
    demo_fix_puts(2u, 5u, "MUSIC PLAYBACK                 ", 2u);
    for (i = 0u; i < 4u; i++) {
        char lbl[4];
        lbl[0] = 'T'; lbl[1] = (char)('0' + (i + 1u)); lbl[2] = '\0';
        demo_fix_puts(2u, 6u, lbl,               1u);
        demo_fix_puts(5u, 6u, s_track_names[i],  2u);
        soundFadeOutSpeed(6u);                snd_step();
        if (uwait(8u)) return 1u;
        soundStopAll();                       snd_step();
        soundPlayGameLoop(s_tracks[i]);       snd_step();
        if (uwait(120u)) return 1u;
    }

    soundFadeOutSpeed(6u);    snd_step();
    if (uwait(10u)) return 1u;
    soundStopAll();           snd_step();

    /* --- SFX section -------------------------------------------------- */
    demo_fix_puts(2u,  5u, "SFX TRIGGERS                   ", 2u);
    demo_fix_puts(2u,  6u, "                               ", 0u);
    for (i = 0u; i < 6u; i++) {
        demo_fix_puts(2u, 8u, s_sfx_names[i], 1u);
        playSFX(s_sfx[i]);    snd_step();
        if (uwait(36u)) return 1u;
    }
    demo_fix_puts(2u, 8u, "                ", 0u);

    /* --- Full mix (each command separated by vblank) ----------------- */
    soundStopAll();           snd_step();
    soundSceneReset();        snd_step();
    demo_fix_puts(2u, 12u, "FULL MIX                       ", 2u);
    soundApplyMix(0x30u, 0xB8u, 0x08u, 0x09u); snd_step();
    playSFXB(SOUND_BED_STAGE_ONE);             snd_step();
    playFMTrack(SOUND_FM_BASS_MOTIF);          snd_step();
    playSSGTrack(SOUND_SSG_MENU_LOOP);         snd_step();
    soundSetSSGPreset(1u);                     snd_step();
    demo_fix_puts(2u, 13u, "ADPCM-B BED + FM BASS",   1u);
    demo_fix_puts(2u, 14u, "SSG LINE + ADPCM-A HIT",  0u);
    if (uwait(90u)) return 1u;
    playSFX(SOUND_SFX_IMPACT_HIT); snd_step();
    if (uwait(80u)) return 1u;

    soundFadeOutSpeed(6u); snd_step();
    if (uwait(20u)) return 1u;
    soundStopAll();        snd_step();
    return 0u;
}

/* ================================================================== */
/*  Chapter 04 — Sprite group  (hardware scale matrix)                   */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_sprite(void)
{
    static const uint8_t screens[] = { 101u, 102u, 103u, 104u, 105u, 106u, 107u };
    const uint8_t count = (uint8_t)(sizeof(screens) / sizeof(screens[0]));
    uint16_t t;
    uint8_t last = 0xFFu;

    chap_header(4u, "SPRITE SCREENS", "GENERATED FRAMES");
    demo_fix_puts(2u, 2u, "SCREEN ASSETS REEL", 1u);
    demo_fix_puts(2u, 3u, "IDS 101..107",       0u);

    snd_cross_to(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    for (t = 0u; t < 252u; t++) {
        uint8_t frame = screens[(t / 36u) % count];
        if (frame != last) {
            demo_draw_sprite_screen(frame, 1u, 32, 16,
                                    demo_screen_strips(frame),
                                    demo_screen_rows(frame),
                                    0xFFu, 0xFFu);
            last = frame;
        }
        if (uframe()) return 1u;
    }
    return 0u;
}

/*
 * Hero = the main warrior character (sprite_001_r01_c01.png onward).
 * The artbox lays out the full move set in rows:
 *   r01 (screens 3..14)  STAND/IDLE
 *   r02 (screens 15..22) WALK CYCLE
 *   r03 (screens 23..33) STRIKE combo
 *   r04 (screens 34..43) SPECIAL A
 *   r05 (screens 44..53) SPECIAL B / RECOVERY
 *
 * The Character system (chars_add / bind_character_asset) cannot
 * cleanly handle the wildly-varying strip counts between these frames
 * — that is what produced the "feet split from body" glitch.  Instead
 * the hero is drawn as a SPRITE GROUP via demo_draw_sprite_screen,
 * which uses the sprite_window cache to clear tail strips when a new
 * frame is narrower than the previous one.  Same pattern as
 * demo_sprites_walk in demo_sprites.c (the proven working path).
 */
static const uint8_t s_hero_stand[8]  = { 3u, 4u, 5u, 7u, 8u, 9u, 11u, 12u };
static const uint8_t s_hero_walk[8]   = { 15u, 16u, 17u, 18u, 19u, 20u, 21u, 22u };
static const uint8_t s_hero_strike[8] = { 23u, 24u, 25u, 26u, 27u, 28u, 29u, 30u };
static const uint8_t s_hero_specA[8]  = { 34u, 35u, 36u, 37u, 38u, 39u, 41u, 43u };
static const uint8_t s_hero_specB[6]  = { 44u, 45u, 47u, 49u, 50u, 52u };

#define HERO_IDLE_FRAME   3u

/*
 * Hero position semantics:
 *   s_hero_x = desired HORIZONTAL CENTRE of the hero on screen
 *   s_hero_y = desired VERTICAL CENTRE of the hero on screen
 *
 * hero_draw() strips the artbox per-frame x_offset/y_offset (which
 * exist for full-screen background composites and would push the
 * sprite off-screen if applied to a centred character) and computes
 * the top-left feed-in to the sprite group from the actual frame
 * dimensions, so the SPRITE GRID CENTRE always lands at (s_hero_x,
 * s_hero_y) regardless of which frame is bound.
 */
static int16_t s_hero_x = 160;
static int16_t s_hero_y = 112;

static void NEOGEO_USER hero_draw(uint8_t frame)
{
    int16_t strips = demo_screen_strips(frame);
    int16_t rows   = demo_screen_rows(frame);
    int16_t grid_w = (int16_t)(strips * 16);
    int16_t grid_h = (int16_t)(rows * 16);
    int16_t off_x  = demo_screen_x_offset(frame);
    int16_t off_y  = demo_screen_y_offset(frame);
    int16_t draw_x = (int16_t)(s_hero_x - (grid_w / 2) - off_x);
    int16_t draw_y = (int16_t)(s_hero_y - (grid_h / 2) - off_y);
    demo_draw_sprite_screen(frame, HERO_SLOT_FIRST,
                            draw_x, draw_y,
                            (uint8_t)strips,
                            (uint8_t)rows,
                            0xFFu, 0xFFu);
}

static void NEOGEO_USER hero_place(int16_t cx, int16_t cy)
{
    s_hero_x = cx;
    s_hero_y = cy;
}

/* ================================================================== */
/*  Chapter 05 — Characters  (full warrior moveset showcase)             */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_chars(void)
{
    uint16_t t;
    uint8_t  prev_phase = 0xFFu;

    chap_header(5u, "CHARACTERS", "FULL WARRIOR MOVESET");
    demo_fix_puts(2u, 2u, "STAND -> WALK -> STRIKE -> SPECIAL", 1u);
    demo_fix_puts(2u, 3u, "DEMO_DRAW_SPRITE_SCREEN PIPELINE",    0u);
    snd_cross_to(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    hero_place(160, 112);   /* centre of screen */

    /*
     * Five-phase showcase (96 frames each, total 480 frames = 8 sec):
     *   0  STAND  (idle pose cycle)
     *   1  WALK   (locomotion cycle)
     *   2  STRIKE (attack combo)
     *   3  SPEC A (special move set 1)
     *   4  SPEC B (special move set 2)
     */
    for (t = 0u; t < 480u; t++) {
        uint8_t phase = (uint8_t)((t / 96u) % 5u);
        uint8_t frame;

        if (phase != prev_phase) {
            demo_fix_puts(2u, 5u, "                       ", 0u);
            switch (phase) {
            case 0: demo_fix_puts(2u, 5u, "STAND   (R01)", 1u); break;
            case 1: demo_fix_puts(2u, 5u, "WALK    (R02)", 2u); break;
            case 2: demo_fix_puts(2u, 5u, "STRIKE  (R03)", 2u); break;
            case 3: demo_fix_puts(2u, 5u, "SPECIAL (R04)", 2u); break;
            default:demo_fix_puts(2u, 5u, "SPECIAL (R05)", 1u); break;
            }
            prev_phase = phase;
        }

        switch (phase) {
        case 0: frame = s_hero_stand[(t / 12u) % 8u]; break;
        case 1: frame = s_hero_walk[(t / 6u) % 8u];   break;
        case 2: frame = s_hero_strike[(t / 6u) % 8u]; break;
        case 3: frame = s_hero_specA[(t / 6u) % 8u];  break;
        default:frame = s_hero_specB[(t / 8u) % 6u];  break;
        }

        hero_draw(frame);

        if (phase == 1u && (t % 24u) == 0u) playSFX(SOUND_SFX_FOOTSTEP);
        if (phase == 2u && (t %  6u) == 0u) playSFX(SOUND_SFX_BLADE_WHOOSH);
        if (phase == 3u && (t % 12u) == 0u) playSFX(SOUND_SFX_STRING_PHRASE);
        if (phase == 4u && (t % 16u) == 0u) playSFX(SOUND_SFX_LOW_DRUM);

        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 06 — Physics  (gravity + solid floor, body re-fitted each   */
/*  frame so the sprite always sits on the floor regardless of asset)   */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_physics(void)
{
    /*
     * Matches the proven pattern from demo_sprites.c spr_physics:
     *   - pre-compute asset_bounds across BOTH flying and ground frames
     *     so set_sprite() validates the larger tile window once
     *   - re-set sprite_offset_y = -(rows * 16) when the frame changes
     *   - keep tile_stride = 16
     * That is the path the previous "Fix sprite sticking" commit (fa7587b)
     * cleared up — replicate it verbatim here.
     */
    static const uint8_t s_eagle_fly[3] = { 78u, 79u, 80u };
    static const uint8_t s_eagle_ground = 75u;
    enum {
        EAGLE_STRIDE     = 16,
        EAGLE_OFFSET_X   = 3,
        EAGLE_SCALE      = 0x80u,
        EAGLE_BODY_W     = 32,
        EAGLE_BODY_H     = 48,
        EAGLE_START_X    = 160,
        EAGLE_START_Y    = 110,    /* closer to the floor so the fall is short */
        PLATFORM_X       = 0,
        PLATFORM_W       = 320
    };

    NGCharacter *eagle;
    uint16_t t;
    uint8_t  frame;
    uint8_t  i;

    chap_header(6u, "PHYSICS", "GRAVITY  SOLIDS  GROUNDED");
    demo_fix_puts(2u, 2u, "NGPHYSICSBODY  GRAVITY 0.125",  1u);
    demo_fix_puts(2u, 3u, "FLOOR Y=192  EAGLE FALLS",      0u);
    snd_cross_to(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    /* Floor strip drawn as FIX so the user sees the contact plane */
    demo_fix_puts(0u, 24u, "========================================", 2u);

    /*
     * EXTRA hard clear before physics setup — kills any stale strip
     * data left by previous chapters in the character VRAM band.
     */
    ng_sprite_hide_all();
    for (i = 0u; i < NG_MAX_CHARS; i++) ng_chars_reset_slot(i);
    ng_chars_defrag_slots();

    ng_physics_init();
    ng_physics_add_solid(PLATFORM_X, U_FLOOR_Y, PLATFORM_W, 8, 0u);

    reset_palette_memo();
    frame = s_eagle_fly[0];
    demo_load_screen_palette(frame);

    eagle = chars_add(0u, EAGLE_START_X, EAGLE_START_Y);
    if (!eagle) return uwait(60u);

    /*
     * Asset bounds cover both ground (75) and all flying (78/79/80) tile
     * ranges so set_sprite()'s window validation passes for every frame swap.
     */
    {
        uint16_t base = DEMO_SCREEN_TILE(s_eagle_ground);
        uint8_t  strips = demo_screen_strips(s_eagle_ground);
        uint8_t  rows   = demo_screen_rows(s_eagle_ground);
        uint16_t tile_start = base;
        uint16_t tile_end = (uint16_t)(base + ((uint16_t)(rows - 1u) * 16u) + (uint16_t)(strips - 1u));
        for (i = 0u; i < 3u; i++) {
            uint16_t fb = DEMO_SCREEN_TILE(s_eagle_fly[i]);
            uint8_t  fs = demo_screen_strips(s_eagle_fly[i]);
            uint8_t  fr = demo_screen_rows(s_eagle_fly[i]);
            uint16_t fe = (uint16_t)(fb + ((uint16_t)(fr - 1u) * 16u) + (uint16_t)(fs - 1u));
            if (fb < tile_start) tile_start = fb;
            if (fe > tile_end)   tile_end   = fe;
        }
        ng_char_set_asset_bounds(eagle, tile_start, tile_end);
    }

    ng_char_set_sprite(eagle, 0u,
                       demo_screen_strips(frame),
                       demo_screen_rows(frame),
                       DEMO_SCREEN_TILE(frame),
                       DEMO_SCREEN_PALETTE(frame));
    ng_char_set_tile_stride(eagle, EAGLE_STRIDE);
    eagle->sprite_offset_x = EAGLE_OFFSET_X;
    eagle->sprite_offset_y = -(int16_t)((int16_t)demo_screen_rows(frame) * 16);
    eagle->scale_x = EAGLE_SCALE;
    eagle->scale_y = EAGLE_SCALE;

    ng_char_set_body(eagle, -(int16_t)(EAGLE_BODY_W / 2),
                            -(int16_t)EAGLE_BODY_H,
                            EAGLE_BODY_W,
                            EAGLE_BODY_H);
    ng_physics_attach(eagle, (uint16_t)(NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS));
    /* Gentle gravity (1/32 px/frame^2) + low terminal velocity so the
     * fall reads clearly on screen instead of being a brief snap. */
    ng_physics_set_gravity(eagle, NG_FP_FROM_FRAC(1, 32), NG_FP_FROM_FRAC(3, 2));

    s_draw_chars = 1u;

    for (t = 0u; t < 300u; t++) {
        uint8_t grounded;
        uint8_t want;

        ng_physics_update_pre();
        ng_chars_update();
        ng_physics_resolve();

        grounded = ng_physics_is_grounded(eagle);
        want = grounded ? s_eagle_ground : s_eagle_fly[(t / 6u) % 3u];

        /*
         * Full re-set (sprite + stride + offset_y) on every frame change.
         * This is the working pattern from demo_sprites.c — it's the only
         * one that doesn't leave the previous frame's strips visible.
         */
        if (eagle->sprite_tile        != DEMO_SCREEN_TILE(want)    ||
            eagle->palette            != DEMO_SCREEN_PALETTE(want) ||
            eagle->sprite_strips      != demo_screen_strips(want)  ||
            eagle->sprite_active_rows != demo_screen_rows(want)) {
            demo_load_screen_palette(want);
            ng_char_set_sprite(eagle, eagle->sprite_first,
                               demo_screen_strips(want),
                               demo_screen_rows(want),
                               DEMO_SCREEN_TILE(want),
                               DEMO_SCREEN_PALETTE(want));
            ng_char_set_tile_stride(eagle, EAGLE_STRIDE);
            eagle->sprite_offset_y = -(int16_t)((int16_t)demo_screen_rows(want) * 16);
            frame = want;
        }

        demo_fix_puts(2u, 4u, grounded ? "STATE: GROUNDED" : "STATE: FALLING ",
                              grounded ? 2u : 1u);

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
    uint16_t t;

    chap_header(7u, "CAMERA", "FOLLOW  DEAD ZONE  SHAKE");
    demo_fix_puts(2u, 2u, "WARRIOR WALKS  CAMERA SCROLLS", 1u);
    demo_fix_puts(2u, 3u, "T=120, T=240 -> SHAKE",         0u);
    snd_cross_to(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 640, 224);
    ng_camera_set_follow_speed(&cam, 8u);
    ng_camera_set_dead_zone(&cam, 32u, 16u);
    ng_camera_set_look_ahead(&cam, 24, 0, 4u);

    hero_place(80, 112);

    for (t = 0u; t < 300u; t++) {
        int16_t vx = (int16_t)((t < 240u) ? 2 : 0);
        int16_t bg_x;
        uint8_t frame;

        if (vx != 0 && s_hero_x < 600) s_hero_x = (int16_t)(s_hero_x + vx);
        frame = (vx != 0) ? s_hero_walk[(t / 6u) % 8u]
                          : s_hero_stand[(t / 12u) % 8u];

        if (t == 120u) { ng_camera_shake(&cam, 4u, 12u); playSFX(SOUND_SFX_IMPACT_HIT); }
        if (t == 240u) { ng_camera_shake(&cam, 3u, 10u); playSFX(SOUND_SFX_IMPACT_HIT); }

        ng_camera_update(&cam, s_hero_x, U_FLOOR_Y, vx);
        bg_x = (int16_t)(32 - (int16_t)((uint16_t)cam.x & 0x00FFu));
        demo_draw_sprite_screen(2u, DEMO_BG_BACK_SLOT, bg_x, 16,
                                demo_screen_strips(2u),
                                demo_screen_rows(2u),
                                0xFFu, 0xFFu);

        /* hero stays at a fixed screen X — camera shows the BG scrolling */
        {
            int16_t saved = s_hero_x;
            s_hero_x = 160;
            hero_draw(frame);
            s_hero_x = saved;
        }

        if (uframe()) return 1u;
    }
    ng_level_set_scroll(0, 0);
    return 0u;
}

/* ================================================================== */
/*  Chapter 08 — Palette FX                                              */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_palette_fx(void)
{
    /*
     * Demoing palette FX without touching the BG: we upload our own
     * synthetic palette (s_palfx_base) into a DEDICATED palette slot
     * (15 — usually unused by demo assets), then animate FX on THAT
     * slot.  The BG continues to use its own authored palette and is
     * never modified, so colours don't get distorted.
     *
     * The visual effect is shown via FIX-layer text rendered on the
     * dedicated palette: the FX colour changes are visible there.
     */
    const uint8_t fx_slot = 15u;
    uint16_t t;

    chap_header(8u, "PALETTE FX", "ISOLATED SLOT  BG INTACT");
    demo_fix_puts(2u, 2u, "PALETTE 15 = FX SANDBOX",        1u);
    demo_fix_puts(2u, 3u, "BG PALETTE LEFT UNTOUCHED",      0u);
    snd_cross_to(SOUND_MUSIC_SHOP_JINGLE);

    draw_background(2u, 32, 16);

    /*
     * The BG keeps its own palette.  We upload s_palfx_base into a
     * SEPARATE slot so that fade / flash / cycle effects animate
     * THERE without modifying the BG palette VRAM at all.
     */
    ng_palfx_upload_base(fx_slot, s_palfx_base);

    demo_fix_puts(2u, 5u, "EFFECT: FADE-IN 60F  ", 1u);
    ng_palfx_fade_in(fx_slot, s_palfx_base, 60u);
    if (uwait(80u)) return 1u;

    demo_fix_puts(2u, 5u, "EFFECT: FLASH WHITE  ", 2u);
    ng_palfx_flash_white(fx_slot, s_palfx_base, 20u);
    if (uwait(40u)) return 1u;

    demo_fix_puts(2u, 5u, "EFFECT: FLASH RED    ", 2u);
    ng_palfx_flash_red(fx_slot, s_palfx_base, 20u);
    if (uwait(40u)) return 1u;

    demo_fix_puts(2u, 5u, "EFFECT: PULSE        ", 1u);
    ng_palfx_pulse(fx_slot, s_palfx_base, 30u);
    if (uwait(150u)) return 1u;

    demo_fix_puts(2u, 5u, "EFFECT: CYCLE 8-14   ", 1u);
    ng_palfx_cycle(fx_slot, s_palfx_base, 8u, 14u);
    for (t = 0u; t < 180u; t++) {
        if (uframe()) return 1u;
    }

    demo_fix_puts(2u, 5u, "EFFECT: FADE-OUT 60F ", 0u);
    ng_palfx_fade_out(fx_slot, s_palfx_base, 60u);
    if (uwait(80u)) return 1u;

    ng_palfx_stop(fx_slot);
    return 0u;
}

/* ================================================================== */
/*  Chapter 09 — Particles                                              */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_particles(void)
{
    uint16_t t;
    char cnt[4];
    const uint8_t spark_id    = 93u;
    const uint16_t spark_tile = DEMO_SCREEN_TILE(spark_id);
    const uint8_t  spark_pal  = DEMO_SCREEN_PALETTE(spark_id);

    chap_header(9u, "PARTICLES", "POOL + PRIORITIES");
    demo_fix_puts(2u, 2u, "TYPES: SPARK DUST MAGIC SMOKE", 1u);
    demo_fix_puts(2u, 3u, "PRIORITIES: CRITICAL/NORMAL/OPT",0u);
    demo_fix_puts(2u, 4u, "ACTIVE: ",                       2u);
    snd_cross_to(SOUND_MUSIC_BOSS_TENSION);

    /* Hero stands at fixed position; sparks fly around him */
    hero_place(160, 112);
    demo_load_screen_palette(spark_id);

    s_draw_particles = 1u;

    for (t = 0u; t < 360u; t++) {
        int16_t bx = (int16_t)(s_hero_x + 50);
        int16_t by = (int16_t)(s_hero_y + 40);

        if ((t % 16u) == 0u) {
            ng_particle_spawn(NG_PART_HIT_SPARK, NG_PART_PRI_CRITICAL,
                              bx, by, 0, -(2L << NG_FP_SHIFT),
                              24u, spark_tile, spark_pal, 1u, 1u);
            ng_particle_spawn(NG_PART_DUST, NG_PART_PRI_NORMAL,
                              (int16_t)(bx + 16), (int16_t)(by + 12),
                              -(1L << NG_FP_SHIFT), -(1L << (NG_FP_SHIFT - 1)),
                              28u, (uint16_t)(spark_tile + 1u), spark_pal, 1u, 1u);
            ng_particle_spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_NORMAL,
                              (int16_t)(bx - 16), (int16_t)(by + 8),
                              (1L << NG_FP_SHIFT), -(1L << NG_FP_SHIFT),
                              22u, (uint16_t)(spark_tile + 2u), spark_pal, 1u, 1u);
        }
        if (t == 80u) {
            ng_particle_spawn(NG_PART_EXPLOSION, NG_PART_PRI_CRITICAL,
                              160, 120, 0, 0,
                              36u, spark_tile, spark_pal, 1u, 1u);
            playSFX(SOUND_SFX_LOW_DRUM);
        }
        if (t == 200u) {
            ng_particle_spawn(NG_PART_SMOKE, NG_PART_PRI_OPTIONAL,
                              80, 100, 0, -(1L << (NG_FP_SHIFT - 1)),
                              45u, spark_tile, spark_pal, 1u, 1u);
            ng_particle_spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_NORMAL,
                              220, 100, 0, -(1L << NG_FP_SHIFT),
                              32u, spark_tile, spark_pal, 1u, 1u);
            playSFX(SOUND_SFX_STRING_PHRASE);
        }

        digit3(cnt, (uint16_t)ng_particles_count());
        demo_fix_puts(10u, 4u, cnt, 1u);

        /* Hero rendered through the proven sprite-window path */
        hero_draw(s_hero_stand[(t / 14u) % 8u]);

        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 10 — Feedback                                                */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_feedback(void)
{
    NGCamera cam;
    const uint8_t  spark_id   = 93u;
    const uint16_t spark_tile = DEMO_SCREEN_TILE(spark_id);
    const uint8_t  spark_pal  = DEMO_SCREEN_PALETTE(spark_id);
    uint16_t t;
    uint8_t  fired = 0u;

    chap_header(10u, "FEEDBACK", "SHAKE  PARTICLE IMPACTS");
    demo_fix_puts(2u, 2u, "WARRIOR TAKES HITS", 1u);
    demo_fix_puts(2u, 3u, "4 INTENSITIES OVER 9 SECONDS",     0u);
    snd_cross_to(SOUND_MUSIC_BOSS_TENSION);

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 320, 224);

    hero_place(160, 112);
    demo_load_screen_palette(spark_id);

    s_draw_particles = 1u;

    for (t = 0u; t < 540u; t++) {
        /* Hero bobs slightly using stand poses (no strip jumps) */
        int16_t bob = (int16_t)(((t & 31u) < 16u) ? 1 : -1);
        int16_t saved = s_hero_x;
        s_hero_x = (int16_t)(160 + bob);

        if (t == 60u && fired < 1u) {
            ng_feedback_shake(&cam, 1u, 8u);
            playSFX(SOUND_SFX_IMPACT_HIT);
            spawn_impact_burst(160, 132, spark_tile, spark_pal, 1u);
            demo_fix_puts(2u, 5u, "FIRED: LIGHT   ", 1u); fired = 1u;
        } else if (t == 180u && fired < 2u) {
            ng_feedback_shake(&cam, 2u, 12u);
            playSFX(SOUND_SFX_IMPACT_HIT);
            spawn_impact_burst(160, 132, spark_tile, spark_pal, 2u);
            demo_fix_puts(2u, 5u, "FIRED: MEDIUM  ", 2u); fired = 2u;
        } else if (t == 320u && fired < 3u) {
            ng_feedback_shake(&cam, 3u, 16u);
            playSFX(SOUND_SFX_IMPACT_HIT);
            spawn_impact_burst(160, 132, spark_tile, spark_pal, 3u);
            demo_fix_puts(2u, 5u, "FIRED: HEAVY   ", 2u); fired = 3u;
        } else if (t == 460u && fired < 4u) {
            ng_feedback_shake(&cam, 4u, 20u);
            playSFX(SOUND_SFX_LOW_DRUM);
            spawn_impact_burst(160, 132, spark_tile, spark_pal, 4u);
            demo_fix_puts(2u, 5u, "FIRED: BOSS    ", 2u); fired = 4u;
        }

        ng_camera_apply(&cam, 160, 112, 0);
        hero_draw(s_hero_stand[(t / 18u) % 8u]);
        s_hero_x = saved;
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 11 — Depth FX (now actually uses ng_depthfx_project)         */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_depthfx(void)
{
    /*
     * STARFIELD demo — the correct use of ng_depthfx_advance_star.
     * 24 stars at random (x,y,z) world positions; each frame z is
     * decremented; when a star reaches the camera plane it loops
     * back to z_max with a fresh random x/y spread.
     *
     * Rendering uses the FIX layer (no warrior sprite distorted):
     * each projected star is drawn as a `.` (far), `+` (mid), or
     * `*` (near) cell at the projected screen coordinates.
     */
    enum { STAR_COUNT = 24 };
    NGVec3 stars[STAR_COUNT];
    /* Track last cell positions so we can clear them next frame */
    uint8_t last_cx[STAR_COUNT];
    uint8_t last_cy[STAR_COUNT];
    uint8_t last_valid[STAR_COUNT];
    uint16_t t;
    uint8_t i;
    /* xorshift seed — deterministic */
    uint16_t rng = 0xACE1u;

    chap_header(11u, "DEPTH FX", "STARFIELD  Z-PROJECTION");
    demo_fix_puts(2u, 2u, "NG_DEPTHFX_ADVANCE_STAR + PROJECT", 1u);
    demo_fix_puts(2u, 3u, "FAR=.  MID=+  NEAR=*",              0u);
    snd_cross_to(SOUND_MUSIC_SAMURAI_ENDING_SCENE);

    ng_depthfx_init();

    /* Seed stars at random (x, y) in [-80..80], z in [16..127] */
    for (i = 0u; i < STAR_COUNT; i++) {
        rng ^= (uint16_t)(rng << 7);
        rng ^= (uint16_t)(rng >> 9);
        rng ^= (uint16_t)(rng << 8);
        stars[i].x = (int16_t)((int16_t)(rng & 0xFFu) - 128);
        stars[i].y = (int16_t)((int16_t)((rng >> 8) & 0x7Fu) - 64);
        stars[i].z = (int16_t)(16 + (rng & 0x6Fu));
        last_valid[i] = 0u;
    }

    for (t = 0u; t < 540u; t++) {
        for (i = 0u; i < STAR_COUNT; i++) {
            NGProjected pr;

            /* Clear last position */
            if (last_valid[i] &&
                last_cx[i] < 40u && last_cy[i] < 28u) {
                demo_fix_puts(last_cx[i], last_cy[i], " ", 0u);
            }

            /* Advance z, wrap to z_max with new spread on each lap */
            ng_depthfx_advance_star(&stars[i], 1, 127, 80, 60);

            pr = ng_depthfx_project(stars[i], 0u);
            if (pr.visible) {
                uint8_t cx = (uint8_t)((pr.screen_x + 160) >> 3);
                uint8_t cy = (uint8_t)((pr.screen_y + 112) >> 3);
                if (cx < 40u && cy < 28u) {
                    const char *glyph;
                    uint8_t pal;
                    if (stars[i].z > 80)      { glyph = ".";  pal = 0u; }
                    else if (stars[i].z > 32) { glyph = "+";  pal = 1u; }
                    else                       { glyph = "*";  pal = 2u; }
                    demo_fix_puts(cx, cy, glyph, pal);
                    last_cx[i]    = cx;
                    last_cy[i]    = cy;
                    last_valid[i] = 1u;
                    continue;
                }
            }
            last_valid[i] = 0u;
        }
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 12 — NPCs (each NPC has its own kind so chars_find works)    */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_npcs(void)
{
    enum { N = 4 };
    static const int16_t home_x[N] = { 56, 126, 196, 266 };
    NGNpc *npcs[N];
    uint8_t last_asset[N];
    uint16_t t;
    uint8_t i;

    chap_header(12u, "NPCS", "PATROL + THINK CALLBACK");
    demo_fix_puts(2u, 2u, "4 CATS  EACH OWN NPC KIND",  1u);
    demo_fix_puts(2u, 3u, "ENGINE THINK FN HANDLES VX", 0u);
    snd_cross_to(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    ng_npcs_init();
    reset_palette_memo();

    for (i = 0u; i < (uint8_t)N; i++) {
        NGCharacter *c;
        uint8_t asset = (uint8_t)(110u + i);
        last_asset[i] = asset;

        /* npc_kind = char_kind = i (unique) so chars_find / chars_at work */
        npcs[i] = npc_spawn((uint8_t)i, (uint8_t)i, home_x[i], U_FLOOR_Y);
        if (!npcs[i]) continue;
        c = npc_char(npcs[i]);
        if (!c) continue;

        bind_character_asset(c, asset, 0x40u, 0x40u);
        ng_npc_set_home(npcs[i], home_x[i], U_FLOOR_Y);
        ng_npc_set_patrol_bounds(npcs[i],
                                 (int16_t)(home_x[i] - 44),
                                 (int16_t)(home_x[i] + 44),
                                 U_FLOOR_Y, U_FLOOR_Y);
        npcs[i]->flags = NG_NPC_FLAG_PATROL_X | NG_NPC_FLAG_FACE_MOTION;
        ng_npc_set_think(npcs[i], ng_npc_think_patrol, 2u);
    }

    s_draw_chars = 1u;

    for (t = 0u; t < 480u; t++) {
        /* Animate each NPC sprite — only rebind when asset truly changes */
        for (i = 0u; i < (uint8_t)N; i++) {
            NGCharacter *c;
            uint8_t asset = (uint8_t)(110u + ((t / 8u + i * 3u) % 12u));
            if (!npcs[i] || asset == last_asset[i]) continue;
            c = npc_char(npcs[i]);
            if (!c) continue;
            bind_character_asset(c, asset, 0x40u, 0x40u);
            last_asset[i] = asset;
        }
        ng_npcs_update();
        ng_chars_update();
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 13 — Mini-game (30 sec deterministic loop)                   */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_mini_game(void)
{
    const uint8_t spark_id    = 93u;
    const uint16_t spark_tile = DEMO_SCREEN_TILE(spark_id);
    const uint8_t  spark_pal  = DEMO_SCREEN_PALETTE(spark_id);

    /*
     * Hit target = a colourful 6x2-cell rectangle drawn on the FIX layer.
     * No character sprite involved — no split/strip issues.  The target
     * has a position, an "alive" flag, and a respawn timer.  When the
     * hero's strike reaches the target it clears, sound plays, particles
     * burst, then it respawns at a new X after a short delay.
     */
    uint8_t  target_cx     = 28u;     /* FIX cell column of target left edge */
    uint8_t  target_cy     = 14u;     /* FIX cell row */
    uint8_t  target_alive  = 1u;
    uint8_t  target_respawn = 0u;
    static const uint8_t TARGET_W = 6u;    /* 6 cells = 48 px wide */
    static const uint8_t TARGET_H = 2u;    /* 2 cells = 16 px tall */

    uint16_t t;
    uint8_t  hero_state = 0u;   /* 0=stand 1=walk 2=jump 3=strike */
    uint8_t  state_t    = 0u;
    int16_t  vy         = 0;
    int16_t  hero_world_x = 80;
    int16_t  hero_world_y = 112;     /* vertical centre of screen */
    const int16_t HERO_GROUND_Y = 112;
    uint8_t  hero_flip  = 0u;
    uint16_t score      = 0u;
    uint16_t enemy_hits = 0u;
    const uint16_t TOTAL = 2700u;        /* 45 sec @ 60 fps */
    char buf[6];

    chap_header(13u, "MINI-GAME", "B STRIKE  C JUMP  D-PAD MOVE");
    demo_fix_puts(2u, 2u, "HOLD LEFT/RIGHT TO RUN", 1u);
    demo_fix_puts(2u, 3u, "STRIKE THE COLOURED TARGET", 0u);
    snd_cross_to(SOUND_MUSIC_WARRIOR_BATTLE);

    /* BG drawn ONCE at slot 300 (back).  Hero (slot 1) sits in front. */
    draw_background(2u, 32, 16);

    ng_joystick_init();
    demo_load_screen_palette(spark_id);

    demo_fix_puts(2u, 25u, "SCORE:", 2u);
    demo_fix_puts(15u, 25u, "TIME:", 2u);
    demo_fix_puts(26u, 25u, "HITS:", 2u);

    s_draw_particles = 1u;

    /* helper: draw the target rectangle on FIX in alternating palettes */
    {
        uint8_t r;
        for (r = 0u; r < TARGET_H; r++) {
            uint8_t c;
            for (c = 0u; c < TARGET_W; c++) {
                demo_fix_puts((uint8_t)(target_cx + c),
                              (uint8_t)(target_cy + r),
                              (r & 1u) ? "#" : "*",
                              (uint8_t)((c + r) & 3u ? 2u : 1u));
            }
        }
    }

    for (t = 0u; t < TOTAL; t++) {
        uint16_t down;
        uint16_t pressed;
        uint8_t hero_frame;
        int16_t hx_old = hero_world_x;

        ng_joystick_update();
        down    = ng_joy_down();
        pressed = ng_joy_pressed();

        /* Strike start (B), only on the ground */
        if ((pressed & BUTTON_B) && hero_state != 3u && hero_state != 2u) {
            hero_state = 3u;
            state_t = 0u;
            playSFX(SOUND_SFX_BLADE_WHOOSH);
        }
        /* Jump start (C) */
        if ((pressed & BUTTON_C) && hero_state != 2u && hero_state != 3u) {
            hero_state = 2u;
            state_t = 0u;
            vy = -8;
            playSFX(SOUND_SFX_SHORT_SHOUT);
        }

        /* Movement is allowed in stand/walk states only */
        if (hero_state < 2u) {
            if (down & JOY_LEFT) {
                hero_world_x -= 2;
                hero_flip = 1u;
                hero_state = 1u;
            } else if (down & JOY_RIGHT) {
                hero_world_x += 2;
                hero_flip = 0u;
                hero_state = 1u;
            } else {
                hero_state = 0u;
            }
        }
        if (hero_world_x < 24)  hero_world_x = 24;
        if (hero_world_x > 280) hero_world_x = 280;

        /* State-machine animation frame selection ---------------------- */
        switch (hero_state) {
        case 0:  hero_frame = s_hero_stand[(t / 14u) % 8u]; break;
        case 1:  hero_frame = s_hero_walk[(t /  6u) % 8u]; break;
        case 2:                               /* jump */
            hero_world_y = (int16_t)(hero_world_y - vy);
            vy++;
            if (hero_world_y >= HERO_GROUND_Y) {
                hero_world_y = HERO_GROUND_Y;
                vy = 0;
                hero_state = 0u;
            }
            hero_frame = s_hero_specA[(t / 5u) % 8u];
            break;
        default:                              /* strike */
            hero_frame = s_hero_strike[(state_t / 3u) % 8u];
            if (state_t == 9u && target_alive) {
                /* hit-test against the FIX-layer target rectangle */
                int16_t reach_px = (int16_t)(hero_flip ? (hero_world_x - 40)
                                                       : (hero_world_x + 40));
                int16_t tx_px = (int16_t)(target_cx * 8 + (TARGET_W * 4));
                int16_t ty_px = (int16_t)(target_cy * 8 + (TARGET_H * 4));
                int16_t dx = (int16_t)(reach_px - tx_px);
                int16_t dy = (int16_t)(hero_world_y - ty_px);
                if (dx < 0) dx = (int16_t)(-dx);
                if (dy < 0) dy = (int16_t)(-dy);
                if (dx < 32 && dy < 40) {
                    uint8_t r;
                    score = (uint16_t)(score + 10u);
                    enemy_hits++;
                    playSFX(SOUND_SFX_IMPACT_HIT);
                    spawn_impact_burst(tx_px, ty_px, spark_tile, spark_pal, 3u);
                    /* CLEAN the FIX cells of the destroyed target */
                    for (r = 0u; r < TARGET_H; r++) {
                        uint8_t c;
                        for (c = 0u; c < TARGET_W; c++) {
                            demo_fix_puts((uint8_t)(target_cx + c),
                                          (uint8_t)(target_cy + r), " ", 0u);
                        }
                    }
                    target_alive   = 0u;
                    target_respawn = 60u;          /* 1 sec until respawn */
                }
            }
            state_t++;
            if (state_t >= 24u) { hero_state = 0u; state_t = 0u; }
            break;
        }

        /* Target respawn timer + relocation */
        if (!target_alive) {
            if (target_respawn > 0u) {
                target_respawn--;
            } else {
                uint8_t r;
                /* pick a new X (cell) — pseudo-random walk from prior position */
                target_cx = (uint8_t)(8u + ((t * 7u) % 22u));
                target_alive = 1u;
                for (r = 0u; r < TARGET_H; r++) {
                    uint8_t c;
                    for (c = 0u; c < TARGET_W; c++) {
                        demo_fix_puts((uint8_t)(target_cx + c),
                                      (uint8_t)(target_cy + r),
                                      (r & 1u) ? "#" : "*",
                                      (uint8_t)((c + r) & 3u ? 2u : 1u));
                    }
                }
            }
        }

        if (hero_state == 1u && hx_old != hero_world_x && (t & 31u) == 0u)
            playSFX(SOUND_SFX_FOOTSTEP);

        /* HUD */
        digit3(buf, score);
        demo_fix_puts(8u, 25u, buf, 1u);
        digit3(buf, (uint16_t)(45u - (t / 60u)));
        demo_fix_puts(20u, 25u, buf, 1u);
        digit3(buf, enemy_hits);
        demo_fix_puts(31u, 25u, buf, 1u);

        /* Hero rendered via the proven sprite-window pipeline.
         * hero_world_x/y are already in CENTRE-of-character coords. */
        s_hero_x = hero_world_x;
        s_hero_y = hero_world_y;
        hero_draw(hero_frame);

        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 14 — Joystick (real engine showcase: dpad, A/B/C/D,         */
/*  pressed vs down vs released, QCF and DP detection, held-frames)     */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_joystick(void)
{
    uint16_t t;
    int16_t  hero_world_x = U_CENTRE_X;
    int16_t  hero_world_y = 112;     /* vertical centre of screen */
    const int16_t HERO_GROUND_Y = 112;
    uint8_t  hero_flip = 0u;
    int16_t  vy = 0;
    char buf[8];

    chap_header(14u, "JOYSTICK", "LIVE INPUT  QCF + DP");
    demo_fix_puts(2u, 2u, "A IDLE FOR 60F SKIPS THIS",   0u);
    demo_fix_puts(2u, 3u, "B STRIKE  C JUMP  QCF+B FX",  1u);
    snd_cross_to(SOUND_MUSIC_SAMURAI_GAME_LOOP);

    ng_joystick_init();

    /* Static HUD labels — once */
    demo_fix_puts(2u,  5u, "PAD:",        2u);
    demo_fix_puts(2u,  6u, "BTN:",        2u);
    demo_fix_puts(2u,  7u, "HELD A:",     2u);
    demo_fix_puts(2u,  8u, "HELD B:",     2u);
    demo_fix_puts(2u,  9u, "HELD C:",     2u);
    demo_fix_puts(2u, 10u, "HELD D:",     2u);
    demo_fix_puts(2u, 12u, "QCF+B:",      2u);
    demo_fix_puts(2u, 13u, "DP +B:",      2u);
    demo_fix_puts(2u, 25u, "MOVE: ARROWS   B: STRIKE   C: JUMP", 0u);

    draw_background(2u, 32, 16);

    for (t = 0u; t < 720u; t++) {
        uint16_t down;
        uint16_t pressed;
        uint16_t released;
        uint8_t frame;
        uint8_t qcf;
        uint8_t dpc;
        uint8_t i;
        uint8_t striking = 0u;

        ng_joystick_update();
        down     = ng_joy_down();
        pressed  = ng_joy_pressed();
        released = ng_joy_released();
        qcf      = ng_joy_special_qcf(hero_flip, BUTTON_B);
        dpc      = ng_joy_special_dp(hero_flip, BUTTON_B);

        /* Pad letters: U D L R */
        {
            char pad[5];
            pad[0] = (down & JOY_UP)    ? 'U' : '.';
            pad[1] = (down & JOY_DOWN)  ? 'D' : '.';
            pad[2] = (down & JOY_LEFT)  ? 'L' : '.';
            pad[3] = (down & JOY_RIGHT) ? 'R' : '.';
            pad[4] = '\0';
            demo_fix_puts(8u, 5u, pad, 1u);
        }
        /* Buttons: A B C D */
        {
            char btn[5];
            btn[0] = (down & BUTTON_A) ? 'A' : '.';
            btn[1] = (down & BUTTON_B) ? 'B' : '.';
            btn[2] = (down & BUTTON_C) ? 'C' : '.';
            btn[3] = (down & BUTTON_D) ? 'D' : '.';
            btn[4] = '\0';
            demo_fix_puts(8u, 6u, btn, (uint8_t)(pressed ? 2u : 1u));
        }
        for (i = 0u; i < 4u; i++) {
            uint16_t mask = (uint16_t)(BUTTON_A << i);
            uint8_t  h = ng_joy_held_frames(mask);
            digit3(buf, h);
            demo_fix_puts(10u, (uint8_t)(7u + i), buf, 1u);
        }
        demo_fix_puts(9u, 12u, qcf ? "OK  " : "--- ", qcf ? 2u : 0u);
        demo_fix_puts(9u, 13u, dpc ? "OK  " : "--- ", dpc ? 2u : 0u);

        /* Motion ---------------------------------------------------- */
        if (down & JOY_LEFT) {
            hero_world_x -= 2;
            hero_flip = 1u;
        } else if (down & JOY_RIGHT) {
            hero_world_x += 2;
            hero_flip = 0u;
        }
        if ((pressed & BUTTON_C) && hero_world_y >= HERO_GROUND_Y) {
            vy = -7;
            playSFX(SOUND_SFX_SHORT_SHOUT);
        }
        if (pressed & BUTTON_B) {
            striking = 1u;
            playSFX(SOUND_SFX_BLADE_WHOOSH);
        }
        if (qcf)  playSFX(SOUND_SFX_STRING_PHRASE);
        if (dpc)  playSFX(SOUND_SFX_LOW_DRUM);
        (void)released;

        if (hero_world_y < HERO_GROUND_Y || vy != 0) {
            hero_world_y = (int16_t)(hero_world_y - vy);
            vy = (int16_t)(vy + 1);
            if (hero_world_y >= HERO_GROUND_Y) {
                hero_world_y = HERO_GROUND_Y;
                vy = 0;
            }
        }
        if (hero_world_x < 24)  hero_world_x = 24;
        if (hero_world_x > 280) hero_world_x = 280;

        /* Select animation set based on input */
        if (striking)             frame = s_hero_strike[(t / 3u) % 8u];
        else if (vy != 0)         frame = s_hero_specA[(t / 5u) % 8u];
        else if (down & (JOY_LEFT | JOY_RIGHT))
                                  frame = s_hero_walk[(t / 5u) % 8u];
        else                      frame = s_hero_stand[(t / 14u) % 8u];

        s_hero_x = hero_world_x;
        s_hero_y = hero_world_y;
        hero_draw(frame);

        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 15 — Scrolling level  (camera follows hero across world)     */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_scrolling_level(void)
{
    NGCamera cam;
    uint16_t t;

    chap_header(15u, "SCROLL LEVEL", "WORLD 768  CAM FOLLOWS");
    demo_fix_puts(2u, 2u, "WARRIOR RUNS LEFT-TO-RIGHT", 1u);
    demo_fix_puts(2u, 3u, "JUMP ARC EVERY 3 SEC",       0u);
    snd_cross_to(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 768, 224);
    ng_camera_set_follow_speed(&cam, 12u);
    ng_camera_set_dead_zone(&cam, 24u, 16u);

    hero_place(160, 112);

    for (t = 0u; t < 540u; t++) {
        int16_t bg_x;
        int16_t jump = 0;
        uint8_t frame;
        int16_t world_x = (int16_t)(40 + (int16_t)((t * 2u) % 560u));

        if ((t % 180u) > 50u && (t % 180u) < 126u) {
            uint16_t jt = (uint16_t)((t % 180u) - 50u);
            jump = (int16_t)((jt < 38u) ? jt : (76u - jt));
            frame = s_hero_specA[(t / 4u) % 8u];
        } else {
            frame = s_hero_walk[(t / 5u) % 8u];
        }

        ng_camera_update(&cam, world_x, U_FLOOR_Y, 2);
        ng_camera_apply(&cam, world_x, U_FLOOR_Y, 2);
        bg_x = (int16_t)(32 - (int16_t)((uint16_t)cam.x & 0x00FFu));
        demo_draw_sprite_screen(2u, DEMO_BG_BACK_SLOT, bg_x, 16,
                                demo_screen_strips(2u), demo_screen_rows(2u),
                                0xFFu, 0xFFu);

        /* Hero stays at a fixed screen X — camera shows the world scroll */
        s_hero_x = 160;
        s_hero_y = (int16_t)(112 - jump);
        hero_draw(frame);

        if ((t % 90u) == 0u) playSFX(SOUND_SFX_FOOTSTEP);
        if (uframe()) {
            ng_level_set_scroll(0, 0);
            return 1u;
        }
    }
    ng_level_set_scroll(0, 0);
    return 0u;
}

/* ================================================================== */
/*  Chapter 16 — 3D effect (FIX perspective road, low churn)             */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_render3d(void)
{
    uint16_t t;

    chap_header(16u, "3D EFFECT", "FIX PERSPECTIVE ROAD");
    demo_fix_puts(2u, 2u, "ROAD WIDTH GROWS BY DEPTH",  1u);
    demo_fix_puts(2u, 3u, "STATIC TEXTURE  ANIMATED Y", 0u);
    snd_cross_to(SOUND_MUSIC_BOSS_TENSION);

    /* Lay out the road once — no per-frame full clears */
    {
        uint8_t row;
        for (row = 7u; row < 26u; row++) {
            uint8_t w = (uint8_t)(4u + (row - 7u) * 2u);
            uint8_t x = (uint8_t)(20u - (w >> 1));
            uint8_t col;
            demo_fix_puts(0u, row, "                                        ", 0u);
            for (col = 0u; col < w && (uint8_t)(x + col) < 39u; col++) {
                demo_fix_puts((uint8_t)(x + col), row,
                              (row & 1u) ? "=" : "-",
                              (uint8_t)((row & 1u) ? 2u : 1u));
            }
        }
    }

    /* Per-frame: just sweep a single highlight row to fake motion */
    for (t = 0u; t < 360u; t++) {
        uint8_t row = (uint8_t)(7u + (t / 4u) % 19u);
        uint8_t w   = (uint8_t)(4u + (row - 7u) * 2u);
        uint8_t x   = (uint8_t)(20u - (w >> 1));
        uint8_t col;
        for (col = 0u; col < w && (uint8_t)(x + col) < 39u; col++) {
            demo_fix_puts((uint8_t)(x + col), row, "*", 0u);
        }
        if ((t & 1u) == 0u) {
            /* restore previous row */
            uint8_t prow = (uint8_t)(7u + ((t / 4u + 18u) % 19u));
            uint8_t pw   = (uint8_t)(4u + (prow - 7u) * 2u);
            uint8_t px   = (uint8_t)(20u - (pw >> 1));
            for (col = 0u; col < pw && (uint8_t)(px + col) < 39u; col++) {
                demo_fix_puts((uint8_t)(px + col), prow,
                              (prow & 1u) ? "=" : "-",
                              (uint8_t)((prow & 1u) ? 2u : 1u));
            }
        }
        if ((t % 120u) == 0u) playSFX(SOUND_SFX_LOW_DRUM);
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 17 — 2D render idea  (light per-frame churn)                 */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_render2d(void)
{
    uint16_t t;
    uint8_t last_bar_x = 0xFFu;

    chap_header(17u, "2D RENDER", "FIX BLITTER PATTERNS");
    demo_fix_puts(2u, 2u, "MOVING BAR  WAVEFORM  BOX",   1u);
    demo_fix_puts(2u, 3u, "CACHED CLEARS  LOW CPU",      0u);
    snd_cross_to(SOUND_MUSIC_SHOP_JINGLE);

    demo_fix_puts(8u, 12u, "+----------------------+", 2u);
    demo_fix_puts(8u, 13u, "|  SOFTWARE 2D LAYER   |", 1u);
    demo_fix_puts(8u, 14u, "+----------------------+", 2u);

    for (t = 0u; t < 360u; t++) {
        uint8_t bar_x = (uint8_t)(2u + ((t >> 1) % 30u));

        /* Move horizontal bar: only erase old position then draw new */
        if (last_bar_x != 0xFFu) {
            uint8_t i;
            for (i = 0u; i < 8u; i++) {
                demo_fix_puts((uint8_t)(last_bar_x + i), 6u, " ", 0u);
            }
        }
        {
            uint8_t i;
            for (i = 0u; i < 8u; i++) {
                demo_fix_puts((uint8_t)(bar_x + i), 6u, "#", 1u);
            }
        }
        last_bar_x = bar_x;

        /* Animated waveform on row 18 — clamp Y range so it really moves */
        {
            uint8_t i;
            demo_fix_puts(1u, 18u, "                                      ", 0u);
            for (i = 0u; i < 28u; i++) {
                /* spread across rows 17..21 */
                uint8_t off = (uint8_t)(((i + (t >> 2)) & 7u));
                uint8_t y = (uint8_t)(17u + (off >> 1));
                if (i == 0u || y != 18u) {
                    demo_fix_puts((uint8_t)(5u + i), y, "*", (uint8_t)(1u + (i & 1u)));
                }
            }
        }
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 18 — SSG arcade (vblank-spaced Z80 setup)                    */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_ssg_arcade(void)
{
    /*
     * Playable Galaxian-style mini-game on the FIX layer.
     *   LEFT / RIGHT       move ship horizontally
     *   B (or A)           fire a single bullet
     *   12 enemies in a 4×3 formation
     *   ship row = 25, enemies on rows 6..11, bullet rises through rows 7..24
     *   30-second timer, score per kill
     */
    enum {
        ROWS_E = 3,
        COLS_E = 4,
        ENEMY_COUNT = ROWS_E * COLS_E,
        SHIP_ROW = 25,
        BULLET_INACTIVE = 0xFFu
    };
    uint8_t enemy_alive[ENEMY_COUNT];
    uint8_t enemy_cx[ENEMY_COUNT];
    uint8_t enemy_cy[ENEMY_COUNT];
    uint8_t ship_x = 18u;
    uint8_t last_ship_x = 0xFFu;
    uint8_t bullet_x = BULLET_INACTIVE;
    uint8_t bullet_y = 0u;
    uint8_t last_bullet_x = BULLET_INACTIVE;
    uint8_t last_bullet_y = 0u;
    uint16_t score = 0u;
    uint16_t t;
    uint8_t i;
    char buf[6];

    chap_header(18u, "SSG ARCADE", "GALAXIAN MINI-SHOOTER");
    demo_fix_puts(2u, 2u, "L/R: MOVE   B: FIRE", 1u);
    demo_fix_puts(2u, 3u, "DESTROY THE FORMATION",0u);

    /* Vblank-spaced Z80 setup so SSG track plays cleanly */
    soundStopAll();                           snd_step();
    soundSceneReset();                        snd_step();
    soundApplyMix(0x20u, 0x00u, 0x0Fu, 0x00u); snd_step();
    soundSetSSGPreset(2u);                    snd_step();
    playSSGTrack(SOUND_SSG_ARCADE_ALERT);     snd_step();

    ng_joystick_init();

    /* Initial enemy formation */
    for (i = 0u; i < ENEMY_COUNT; i++) {
        enemy_alive[i] = 1u;
        enemy_cx[i] = (uint8_t)(8u + (i % COLS_E) * 6u);
        enemy_cy[i] = (uint8_t)(7u + (i / COLS_E) * 2u);
    }

    demo_fix_puts(2u, 26u, "SCORE:",        2u);
    demo_fix_puts(20u, 26u, "ALIVE:",       2u);
    demo_fix_puts(30u, 26u, "TIME:",        2u);

    for (t = 0u; t < 1800u; t++) {       /* 30 sec @ 60 fps */
        uint16_t down;
        uint16_t pressed;
        uint8_t alive_count = 0u;
        uint8_t enemy_drift = (uint8_t)((t / 60u) & 1u);   /* slow side-to-side */

        ng_joystick_update();
        down    = ng_joy_down();
        pressed = ng_joy_pressed();

        /* Ship motion (every 2 frames so it feels smooth, not jittery) */
        if ((t & 1u) == 0u) {
            if ((down & JOY_LEFT)  && ship_x > 2u)  ship_x--;
            if ((down & JOY_RIGHT) && ship_x < 35u) ship_x++;
        }

        /* Fire (B or A) */
        if ((pressed & (BUTTON_B | BUTTON_A)) && bullet_x == BULLET_INACTIVE) {
            bullet_x = (uint8_t)(ship_x + 1u);
            bullet_y = (uint8_t)(SHIP_ROW - 1u);
            playSFX(SOUND_SFX_BLADE_WHOOSH);
        }

        /* Bullet rise + collision */
        if (bullet_x != BULLET_INACTIVE) {
            if (bullet_y == 0u) {
                bullet_x = BULLET_INACTIVE;       /* off top */
            } else {
                bullet_y--;
                for (i = 0u; i < ENEMY_COUNT; i++) {
                    if (!enemy_alive[i]) continue;
                    if (bullet_y == enemy_cy[i] &&
                        bullet_x >= enemy_cx[i] &&
                        bullet_x <= (uint8_t)(enemy_cx[i] + 2u)) {
                        enemy_alive[i] = 0u;
                        bullet_x = BULLET_INACTIVE;
                        score = (uint16_t)(score + 50u);
                        playSFX(SOUND_SFX_IMPACT_HIT);
                        /* erase dead enemy */
                        demo_fix_puts(enemy_cx[i], enemy_cy[i], "   ", 0u);
                        break;
                    }
                }
            }
        }

        /* Redraw the enemy formation only when it drifts */
        if ((t % 60u) == 0u) {
            for (i = 0u; i < ENEMY_COUNT; i++) {
                if (!enemy_alive[i]) continue;
                /* erase old shifted cell */
                demo_fix_puts((uint8_t)(enemy_cx[i] - enemy_drift), enemy_cy[i],
                              "   ", 0u);
            }
            enemy_drift ^= 1u;
        }
        for (i = 0u; i < ENEMY_COUNT; i++) {
            if (enemy_alive[i]) {
                demo_fix_puts((uint8_t)(enemy_cx[i] + enemy_drift),
                              enemy_cy[i], "/W\\",
                              (uint8_t)(1u + (i & 1u)));
                alive_count++;
            }
        }

        /* Ship draw: erase old position, draw new */
        if (last_ship_x != 0xFFu && last_ship_x != ship_x) {
            demo_fix_puts(last_ship_x, SHIP_ROW, "   ", 0u);
        }
        demo_fix_puts(ship_x, SHIP_ROW, "/A\\", 2u);
        last_ship_x = ship_x;

        /* Bullet draw: erase old, draw current */
        if (last_bullet_x != BULLET_INACTIVE &&
            (last_bullet_x != bullet_x || last_bullet_y != bullet_y)) {
            demo_fix_puts(last_bullet_x, last_bullet_y, " ", 0u);
        }
        if (bullet_x != BULLET_INACTIVE) {
            demo_fix_puts(bullet_x, bullet_y, "|", 1u);
        }
        last_bullet_x = bullet_x;
        last_bullet_y = bullet_y;

        /* HUD */
        digit3(buf, score);
        demo_fix_puts(8u, 26u, buf, 1u);
        digit3(buf, alive_count);
        demo_fix_puts(26u, 26u, buf, 1u);
        digit3(buf, (uint16_t)(30u - (t / 60u)));
        demo_fix_puts(35u, 26u, buf, 1u);

        /* Win condition: clear formation */
        if (alive_count == 0u) {
            demo_fix_puts(13u, 14u, "*** STAGE CLEAR ***", 2u);
            if (uwait(120u)) goto fade_out;
            /* respawn formation for next wave */
            for (i = 0u; i < ENEMY_COUNT; i++) {
                enemy_alive[i] = 1u;
                enemy_cx[i] = (uint8_t)(8u + (i % COLS_E) * 6u);
                enemy_cy[i] = (uint8_t)(7u + (i / COLS_E) * 2u);
            }
            demo_fix_puts(13u, 14u, "                  ", 0u);
        }

        if (uframe()) goto fade_out;
    }

fade_out:
    soundFadeOutSpeed(8u); snd_step();
    if (uwait(12u)) return 1u;
    soundStopAll();        snd_step();
    return 0u;
}

/* ================================================================== */
/*  Chapter 19 — Credits                                                 */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_credits(void)
{
    chap_header(19u, "CREDITS", "EAGLE SOFTWARE 2026");
    snd_cross_to(SOUND_MUSIC_ENDING_CREDITS);

    demo_fix_puts(2u,  4u, "ENGINE   SDK/2D_ENGINE_PLUS", 1u);
    demo_fix_puts(2u,  5u, "BUILD    USE_2D_PLUS=1",      0u);
    demo_fix_puts(2u,  6u, "WEB      EAGLESOFTWARE.BIZ",  0u);

    demo_fix_puts(2u,  9u, "MODULES SHOWN:",          2u);
    demo_fix_puts(4u, 10u, "FIX  SPRITE  CHARS",      1u);
    demo_fix_puts(4u, 11u, "SOUND  PHYSICS  CAMERA",  1u);
    demo_fix_puts(4u, 12u, "PALETTE FX  PARTICLES",   1u);
    demo_fix_puts(4u, 13u, "FEEDBACK  DEPTH FX",      1u);
    demo_fix_puts(4u, 14u, "NPCS  JOYSTICK  SCROLL",  1u);
    demo_fix_puts(4u, 15u, "2D/3D RENDER  SSG ARCADE",1u);

    demo_fix_puts(2u, 17u, "BEHIND THE SCENES:",          2u);
    demo_fix_puts(4u, 18u, "TIMERS  PROGRESS  STATUS",    0u);
    demo_fix_puts(4u, 19u, "PROPERTIES  EVENTS  BORDERS", 0u);
    demo_fix_puts(4u, 20u, "RENDER QUEUE  VBLANK FLUSH",  0u);

    demo_fix_puts(2u, 24u, "THANKS FOR PLAYING.", 2u);
    if (uwait(240u)) return 1u;

    soundFadeOutSpeed(4u); snd_step();
    if (uwait(40u)) return 1u;
    soundStopAll();        snd_step();
    return 0u;
}

/* ================================================================== */
/*  Public entry — 20-chapter linear flow                                */
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
    (void)chap_joystick();
    (void)chap_scrolling_level();
    (void)chap_render3d();
    (void)chap_render2d();
    (void)chap_ssg_arcade();
    (void)chap_credits();
}
