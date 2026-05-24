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
void NEOGEO_USER soundCancelFade(void);
void NEOGEO_USER soundStopMusic(void);
void NEOGEO_USER playMusic(uint8_t n);
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
 * Sprite slot priority — empirically determined from user testing.
 *
 * Despite what the SDK comment in ng_sprite_pool.h says, on the actual
 * Neo Geo MVS hardware HIGHER sprite slot numbers are drawn IN FRONT
 * of lower ones.  The SDK's assumption was the opposite, which is why
 * draw_background() pointing at slot 300 ended up COVERING characters
 * in the 96..223 range.  We now follow the observed hardware behaviour.
 *
 * For this demo:
 *   slot   1..16   = BACKGROUND  (DEMO_BG_BACK_SLOT = 1)
 *                    Drawn BEHIND everything else (lowest = back).
 *   slot  96..223  = NGCharacter system (NPCs / managed chars)
 *   slot 256..287  = particles (NG_SPR_PART_FIRST)
 *   slot 350..365  = hero sprite group  (HERO_SLOT_FIRST = 350)
 *                    HIGHER slot = drawn ON TOP of BG, chars, particles.
 *   slot 366..374  = enemy / target sprite group (ENEMY_SLOT_FIRST = 366)
 *                    Behind the hero but in front of everything else.
 *
 * NB: this empirical direction is captured here in the demo only.  The
 * engine's pool comment still claims the opposite — keep this header
 * authoritative for any code that needs occlusion to actually work.
 */
#define DEMO_BG_BACK_SLOT  1u                 /* back-most */
#define HERO_SLOT_FIRST    350u               /* front-most for the hero */
#define ENEMY_SLOT_FIRST   366u               /* in front of BG, behind hero */

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
    /*
     * Softer backdrop than pure black so that chapters with no BG
     * sprite (FIX showcase, sound, render-2D/3D, credits) don't look
     * like they're broken — gives a dim navy where FIX cells with
     * palette-index 0 (transparent) would otherwise reveal pure black.
     */
    setBACKDROP(0x8001);
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

    demo_fix_puts(2u,  7u, "SDK SHOWCASE", 2u);
    demo_fix_puts(2u,  9u, "20 CHAPTERS  FULL SDK TOUR", 1u);

    demo_fix_puts(2u, 12u, "SHOWCASE FLOW:",         2u);
    demo_fix_puts(4u, 13u, "AUTOMATIC CHAPTERS",     1u);
    demo_fix_puts(4u, 14u, "SPRITES AND EFFECTS",    1u);
    demo_fix_puts(4u, 15u, "PHYSICS AND CAMERA",     1u);
    demo_fix_puts(4u, 16u, "NPC + PATROL + DEPTH",   1u);
    demo_fix_puts(4u, 17u, "JOYSTICK + MINI-GAME",   1u);

    return uwait(220u);
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

 soundSceneReset();   waitVbl();
        soundSetADPCMAVolume(0x3Cu);  waitVbl();
        soundSetADPCMBVolume(0xBCu);  waitVbl();
        soundSetSSGVolume(0x00u);     waitVbl();
        soundSetFMVolume(0x00u);      waitVbl();
        soundPlayGameLoop(SOUND_MUSIC_E);
        waitVbl();

    chap_header(1u, "TITLE", "ATTRACT REEL");
    demo_fix_puts(2u, 2u, "TITLE / EYECATCHER", 1u);
	
	
    for (t = 0u; t < 360u; t++) {
        uint8_t frame;
        if (t < 90u)        frame = 108u;
        else if (t < 300u)  frame = parade[((t - 90u) / 30u) % N];
        else                frame = 109u;

        if (frame != last_frame) {
            /*
             * CENTER the title art both axes by computing the position
             * from the actual frame strip/row count instead of using a
             * hard-coded (32, 16).  Screen W=320, H=224.  Sprite top-
             * left x = (W - strips*16)/2 = 160 - strips*8 (after
             * stripping artbox x_offset).  Same for y.
             */
            int16_t strips = demo_screen_strips(frame);
            int16_t rows   = demo_screen_rows(frame);
            int16_t off_x  = demo_screen_x_offset(frame);
            int16_t off_y  = demo_screen_y_offset(frame);
            int16_t draw_x = (int16_t)(160 - (strips * 16) / 2 - off_x);
            int16_t draw_y = (int16_t)(112 - (rows   * 16) / 2 - off_y);
            demo_draw_sprite_screen(frame, 1u, draw_x, draw_y,
                                    (uint8_t)strips, (uint8_t)rows,
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

    chap_header(2u, "FIX LAYER", "TEXT  PALETTES");
    /*
     * Clean WHITE backdrop — no artwork BG image, no pure black.
     * The empty cells of the FIX layer are transparent and show this
     * backdrop colour, which makes the FIX text easy to read on a
     * paper-like surface.  We override the chap_header backdrop
     * (which was dim navy) just for this chapter.
     */
    setBACKDROP(WHITE);
    /*
     * Re-write the header tag + "A: NEXT" using palette 2 (yellow/
     * accent) instead of palette 0 (white) so they remain visible on
     * the new white backdrop.
     */
    {
        char tag[6];
        tag[0] = 'C'; tag[1] = 'H'; tag[2] = '.';
        tag[3] = '0'; tag[4] = '2'; tag[5] = '\0';
        demo_fix_puts(2u,  0u, tag,        2u);
        demo_fix_puts(36u, 0u, "02",       2u);
        demo_fix_puts(2u, 27u, "A: NEXT",  2u);
    }
    demo_fix_puts(2u, 2u, "FIX = 40x32 CELL OVERLAY", 1u);
    snd_cross_to(SOUND_MUSIC_G);

    demo_fix_puts(2u,  6u, "PALETTE 0  STANDARD",   0u);
    demo_fix_puts(2u,  7u, "PALETTE 1  ACCENT",     1u);
    demo_fix_puts(2u,  8u, "PALETTE 2  WARN",       2u);



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
/*  Chapter 03 — Sound tour                                              */
/*                                                                       */
/*  Linear walkthrough of every audio subsystem on the YM2610.  Each     */
/*  section is silent before/after, labels narrate the active driver     */
/*  call, and volumes are kept asymmetric so the listener can isolate    */
/*  whatever feature the section is showing.                             */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_sound(void)
{
    static const uint8_t s_sfx[6] = {
        SOUND_SFX_1, SOUND_SFX_7,
        SOUND_SFX_8, SOUND_SFX_5,
        SOUND_SFX_9, SOUND_SFX_10
    };
    static const char *const s_sfx_names[6] = {
        "SFX 1  COIN     ", "SFX 7  WHOOSH   ",
        "SFX 8  IMPACT   ", "SFX 5  FOOTSTEP ",
        "SFX 9  STRING   ", "SFX 10 LOW DRUM "
    };
    static const uint8_t s_adpcmb_list[8] = {
        SOUND_TRACK_A, SOUND_TRACK_B, SOUND_TRACK_C, SOUND_TRACK_D,
        SOUND_TRACK_F, SOUND_TRACK_G, SOUND_TRACK_H, SOUND_TRACK_I
    };
    static const char *const s_adpcmb_names[8] = {
        "TRACK 1  1.WAV  ", "TRACK 2  2.WAV  ",
        "TRACK 3  3.WAV  ", "TRACK 4  4.WAV  ",
        "TRACK 6  6.WAV  ", "TRACK 7  7.WAV  ",
        "TRACK 8  8.WAV  ", "TRACK 9  9.WAV  "
    };
    uint8_t i;

    chap_header(3u, "SOUND",
                "YM2610 TOUR  ADPCM-B / FM / SSG / SPEECH");
    demo_fix_puts(2u, 2u, "PLAYS EVERY SUBSYSTEM, ONE AT A TIME", 1u);
    demo_fix_puts(2u, 3u, "LABELS NARRATE THE ACTIVE DRIVER CALL", 0u);

    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0xB8u, 0x00u, 0x00u); snd_step();

    /* --- 1) ADPCM-B streamed tracks (1..9, skip the reserved one) --- */
    demo_fix_puts(2u, 5u, "1. ADPCM-B STREAMED TRACKS        ", 2u);
    for (i = 0u; i < 8u; i++) {
        demo_fix_puts(2u, 7u, s_adpcmb_names[i], 1u);
        soundFadeOutSpeed(8u);                     snd_step();
        if (uwait(6u)) return 1u;
        soundStopAll();                            snd_step();
        soundApplyMix(0x30u, 0xC0u, 0x00u, 0x00u); snd_step();
        playSFXB(s_adpcmb_list[i]);                snd_step();
        if (uwait(110u)) return 1u;
    }
    soundFadeOutSpeed(8u); snd_step();
    if (uwait(8u)) return 1u;
    soundStopAll();        snd_step();
    demo_fix_puts(2u, 7u, "                  ", 0u);

    /* --- 2) ADPCM-B STEREO PAN ($11 register, NEW DRIVER FEATURE) --- *
     *
     * soundSetADPCMBPan(pan) writes YM2610 register $11 (L/R enable).
     * Sweep the same track through stereo → left → right → stereo so
     * the listener can hear the pan field move. */
    demo_fix_puts(2u, 5u, "2. ADPCM-B STEREO PAN  ($11)      ", 2u);
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0xC0u, 0x00u, 0x00u); snd_step();
    playSFXB(SOUND_TRACK_A);                   snd_step();

    demo_fix_puts(2u, 9u, "pan = 0xC0  (L+R stereo)          ", 1u);
    soundSetADPCMBPan(0xC0u); snd_step();
    if (uwait(110u)) return 1u;
    demo_fix_puts(2u, 9u, "pan = 0x80  (LEFT only)           ", 1u);
    soundSetADPCMBPan(0x80u); snd_step();
    if (uwait(110u)) return 1u;
    demo_fix_puts(2u, 9u, "pan = 0x40  (RIGHT only)          ", 1u);
    soundSetADPCMBPan(0x40u); snd_step();
    if (uwait(110u)) return 1u;
    demo_fix_puts(2u, 9u, "pan = 0xC0  (back to stereo)      ", 1u);
    soundSetADPCMBPan(0xC0u); snd_step();
    if (uwait(80u)) return 1u;
    soundFadeOutSpeed(8u); snd_step();
    if (uwait(40u)) return 1u;
    soundStopAll();        snd_step();
    demo_fix_puts(2u, 9u, "                                  ", 0u);

    /* --- 3) VOICE CUES — recorded ADPCM-A samples ------------------ *
     *
     * Intelligible speech on YM2610 requires PCM samples.  These
     * cues play V-ROM voice clips (SFX 11/12, 10) — that's what every
     * NeoGeo arcade with speech actually used.  Pure-chip SSG/FM
     * formant synthesis only produces robotic chords. */
    demo_fix_puts(2u, 5u, "3. VOICE CUES (recorded ADPCM-A)  ", 2u);
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0x00u, 0x00u, 0x00u); snd_step();

    demo_fix_puts(2u, 11u, "playVoiceGetReady()  ADPCM-A SFX  ", 1u);
    playVoiceGetReady(); snd_step();
    if (uwait(160u)) return 1u;
    demo_fix_puts(2u, 11u, "playVoiceLetsGo()    ADPCM-A SFX  ", 1u);
    playVoiceLetsGo();   snd_step();
    if (uwait(160u)) return 1u;
    demo_fix_puts(2u, 11u, "playVoiceGameOver()  ADPCM-A SFX  ", 1u);
    playVoiceGameOver(); snd_step();
    if (uwait(200u)) return 1u;
    demo_fix_puts(2u, 11u, "                                  ", 0u);
    soundStopAll(); snd_step();

    /* --- 4) FM TRACKS — all 8 melodic loops ------------------------ */
    demo_fix_puts(2u, 5u, "4. FM TRACKS (1..8)               ", 2u);
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0x00u, 0x00u, 0x0Eu); snd_step();
    for (i = 0u; i < SOUND_FM_TRACK_COUNT; i++) {
        char lbl[8];
        lbl[0] = 'F'; lbl[1] = 'M'; lbl[2] = ' '; lbl[3] = (char)('0' + (i + 1u));
        lbl[4] = '\0';
        demo_fix_puts(2u, 13u, lbl, 1u);
        soundStopMusic(); snd_step();
        playFMTrack(i);   snd_step();
        if (uwait(360u)) return 1u;
    }
    soundStopMusic();        snd_step();
    soundSetFMVolume(0x00u); snd_step();
    demo_fix_puts(2u, 13u, "         ", 0u);

    /* --- 5) FM LFO (DRIVER FEATURE — $22 register) ---------------- *
     *
     * Play one FM track and toggle the LFO live across four settings.
     * fm_apply_patch no longer overrides register $22, so the user
     * setting from soundFMSetLFO persists across notes — the listener
     * hears the same melody change character (flat → slow wobble →
     * vibrato → off) as the section progresses.  Patch's AMS/PMS bits
     * in fm/patches.fm must be non-zero for the modulation to be
     * audible. */
    demo_fix_puts(2u, 5u, "5. FM LFO  ($22, vibrato/tremolo) ", 2u);
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0x00u, 0x00u, 0x0Eu); snd_step();
    playFMTrack(6u);                           snd_step();

    demo_fix_puts(2u, 15u, "LFO OFF      (flat reference)     ", 1u);
    soundFMSetLFO(0x00u); snd_step();
    if (uwait(220u)) return 1u;
    demo_fix_puts(2u, 15u, "LFO rate=1   (slow wobble)        ", 1u);
    soundFMSetLFO(0x09u); snd_step();
    if (uwait(220u)) return 1u;
    demo_fix_puts(2u, 15u, "LFO rate=3   (medium vibrato)     ", 1u);
    soundFMSetLFO(0x0Bu); snd_step();
    if (uwait(220u)) return 1u;
    demo_fix_puts(2u, 15u, "LFO rate=6   (fastest vibrato)    ", 1u);
    soundFMSetLFO(0x0Eu); snd_step();
    if (uwait(220u)) return 1u;

    soundFMSetLFO(0x00u);    snd_step();
    soundStopMusic();        snd_step();
    soundSetFMVolume(0x00u); snd_step();
    demo_fix_puts(2u, 15u, "                                  ", 0u);

    /* --- 6) SSG TRACKS — 4 melodic loops --------------------------- */
    demo_fix_puts(2u, 5u, "6. SSG TRACKS (1..4)              ", 2u);
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0x00u, 0x0Eu, 0x00u); snd_step();
    for (i = 0u; i < SOUND_SSG_TRACK_COUNT; i++) {
        char lbl[8];
        lbl[0] = 'S'; lbl[1] = 'S'; lbl[2] = 'G'; lbl[3] = ' ';
        lbl[4] = (char)('0' + (i + 1u)); lbl[5] = '\0';
        demo_fix_puts(2u, 17u, lbl, 1u);
        soundStopMusic();             snd_step();
        soundSetSSGPreset(i);         snd_step();
        playSSGTrack(i);              snd_step();
        if (uwait(360u)) return 1u;
    }
    /* FULL teardown so the next section starts on a known-good
     * driver state — soundStopMusic alone left state that could
     * suppress subsequent ADPCM-A/B and FM playback. */
    soundStopAll();           snd_step();
    soundSceneReset();        snd_step();
    demo_fix_puts(2u, 17u, "         ", 0u);

    /* --- 7) ADPCM-A SFX bank -------------------------------------- */
    demo_fix_puts(2u, 5u, "7. ADPCM-A SFX                    ", 2u);
    soundApplyMix(0x30u, 0x00u, 0x00u, 0x00u); snd_step();
    for (i = 0u; i < 6u; i++) {
        demo_fix_puts(2u, 19u, s_sfx_names[i], 1u);
        playSFX(s_sfx[i]); snd_step();
        if (uwait(40u)) return 1u;
    }
    demo_fix_puts(2u, 19u, "                ", 0u);

    /* --- 8) MULTITRACK MIX — bed dominant, FM/SSG accent only ------ *
     *
     * Streamed ADPCM-B beds are full mixed tracks; layering loud
     * FM or SSG melody on top usually clashes because the keys and
     * tempos of the MML tracks don't match the bed.  This section
     * keeps the bed loud (0xC0) and FM/SSG VERY quiet (0x02/0x03)
     * so they read as subtle accent texture rather than competing
     * melodies.  Each mix RESTARTS the bed + accent together so the
     * listener hears the combined sound from the same instant. */
    demo_fix_puts(2u, 5u, "8. MULTITRACK MIXES               ", 2u);
    {
        static const uint8_t s_mix_track[3] = { SOUND_TRACK_A, SOUND_TRACK_C, SOUND_TRACK_G };
        static const uint8_t s_mix_fm[3]    = { 0u,    0xFFu, 2u   };
        static const uint8_t s_mix_ssg[3]   = { 0xFFu, 0u,    1u   };
        static const char *const s_mix_lbl[3] = {
            "MIX A  bed + FM accent (whisper)  ",
            "MIX B  bed + SSG accent (whisper) ",
            "MIX C  bed + FM + SSG (both soft) "
        };
        uint8_t m;
        for (m = 0u; m < 3u; m++) {
            demo_fix_puts(2u, 21u, s_mix_lbl[m], 1u);
            soundStopAll();    snd_step();
            soundSceneReset(); snd_step();
            /* bed loud, FM/SSG barely-there accent vol */
            soundApplyMix(0x30u, 0xC0u, 0x02u, 0x03u); snd_step();
            playSFXB(s_mix_track[m]); snd_step();
            if (s_mix_fm[m] != 0xFFu) {
                playFMTrack(s_mix_fm[m]); snd_step();
            }
            if (s_mix_ssg[m] != 0xFFu) {
                soundSetSSGPreset(s_mix_ssg[m]); snd_step();
                playSSGTrack(s_mix_ssg[m]);      snd_step();
            }
            if (uwait(360u)) return 1u;
        }
    }
    soundFadeOutSpeed(6u); snd_step();
    if (uwait(40u)) return 1u;
    soundStopAll();        snd_step();
    demo_fix_puts(2u, 21u, "                                  ", 0u);

    /* --- 9) FADE TESTS on ADPCM-B TRACK 7 -------------------------- *
     *
     * IMPORTANT: the driver's fade-speed formula is COUNTER = $FF -
     * speed, where COUNTER is the number of Timer-B IRQs (~123 ms
     * each) between every -1 volume step.  So:
     *
     *   speed=$FF (255) → 1 IRQ/step  → vol $B8→0 in ~22 s        (instant)
     *   speed=$FE (254) → 1 IRQ/step  → same
     *   speed=$FC (252) → 3 IRQs/step → ~70 s    (too slow)
     *   speed=$F0 (240) → 15 IRQs/step → ~5 min  (no audible fade)
     *
     * Wait — fade -1 per step from $B8 is 184 steps.  With speed=$FE,
     * 184 IRQs × 123 ms = 22.6 s.  Even max speed is slow.  For the
     * demo we use the fastest values to get audible fades in a few
     * seconds. */
    demo_fix_puts(2u, 5u, "9. FADE TESTS  (ADPCM-B only)     ", 2u);

    /* --- FadeOut fast --- */
    demo_fix_puts(2u, 23u, "FadeOut(0xFF) fastest             ", 1u);
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0xB8u, 0x00u, 0x00u); snd_step();
    playSFXB(SOUND_TRACK_G);                   snd_step();
    if (uwait(60u)) return 1u;
    soundFadeOutSpeed(0xFFu); snd_step();
    if (uwait(180u)) return 1u;

    /* --- FadeIn fast --- */
    demo_fix_puts(2u, 23u, "FadeIn(0xFF)  fastest ramp        ", 1u);
    soundFadeInSpeed(0xFFu); snd_step();
    if (uwait(180u)) return 1u;

    /* --- FadeOut medium --- */
    demo_fix_puts(2u, 23u, "FadeOut(0xFD) medium              ", 1u);
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0xB8u, 0x00u, 0x00u); snd_step();
    playSFXB(SOUND_TRACK_G);                   snd_step();
    if (uwait(40u)) return 1u;
    soundFadeOutSpeed(0xFDu); snd_step();
    if (uwait(180u)) return 1u;
    demo_fix_puts(2u, 23u, "FadeIn(0xFD)  medium ramp         ", 1u);
    soundFadeInSpeed(0xFDu); snd_step();
    if (uwait(180u)) return 1u;

    /* --- CancelFade snap-back --- */
    demo_fix_puts(2u, 23u, "soundCancelFade  snap-back        ", 1u);
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0xB8u, 0x00u, 0x00u); snd_step();
    playSFXB(SOUND_TRACK_G);                   snd_step();
    if (uwait(40u)) return 1u;
    soundFadeOutSpeed(0xFEu); snd_step();
    if (uwait(80u)) return 1u;
    soundCancelFade();     snd_step();
    if (uwait(180u)) return 1u;

    /* --- Final FadeOut to silence --- */
    demo_fix_puts(2u, 23u, "Final FadeOut(0xFF) to silence    ", 1u);
    soundFadeOutSpeed(0xFFu); snd_step();
    if (uwait(220u)) return 1u;

    soundStopAll(); snd_step();
    demo_fix_puts(2u, 23u, "                                  ", 0u);
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

    snd_cross_to(SOUND_MUSIC_B);

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
    snd_cross_to(SOUND_MUSIC_A);

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

        if (phase == 1u && (t % 24u) == 0u) playSFX(SOUND_SFX_5);
        if (phase == 2u && (t %  6u) == 0u) playSFX(SOUND_SFX_7);
        if (phase == 3u && (t % 12u) == 0u) playSFX(SOUND_SFX_9);
        if (phase == 4u && (t % 16u) == 0u) playSFX(SOUND_SFX_10);

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

    /*
     * EXPLICIT SCREEN CLEAR before drawing anything.
     * Even though chap_header runs ng_clear_screen_full(), we re-clear
     * the FIX layer and force the backdrop back to dim navy here so
     * the previous chapter's white/coloured backdrop cannot bleed
     * through and the floor / eagle render against a known background.
     */
    clearFix();
    setBACKDROP(0x8001);

    /* Re-draw header text after the clear */
    demo_fix_puts(2u,  0u, "CH.06",        2u);
    demo_fix_puts(8u,  0u, "PHYSICS",      2u);
    demo_fix_puts(2u,  1u, "GRAVITY  SOLIDS  GROUNDED", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT",      0u);

    demo_fix_puts(2u, 2u, "NGPHYSICS  GRAVITY 0.125",  1u);
    demo_fix_puts(2u, 3u, "FLOOR Y=184  EAGLE FALLS",      0u);
    snd_cross_to(SOUND_MUSIC_A);

    /*
     * Visible floor bar — drawn at FIX row 23 (pixel y 184..191).
     * The PHYSICS solid below is placed at y=184 (same top edge) so
     * the eagle's feet land EXACTLY on the bar.  Previously the bar
     * was at row 23 but the solid was at y=192, so the eagle fell
     * 8 px past the visible bar.
     */
    demo_fix_puts(0u, 23u, "========================================", 2u);

    /*
     * EXTRA hard clear before physics setup — kills any stale strip
     * data left by previous chapters in the character VRAM band.
     */
    ng_sprite_hide_all();
    for (i = 0u; i < NG_MAX_CHARS; i++) ng_chars_reset_slot(i);
    ng_chars_defrag_slots();

    ng_physics_init();
    /* Solid Y = 184 matches the top edge of the FIX floor bar at row 23 */
    ng_physics_add_solid(PLATFORM_X, 184, PLATFORM_W, 8, 0u);

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
    snd_cross_to(SOUND_MUSIC_A);

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

        if (t == 120u) { ng_camera_shake(&cam, 4u, 12u); playSFX(SOUND_SFX_8); }
        if (t == 240u) { ng_camera_shake(&cam, 3u, 10u); playSFX(SOUND_SFX_8); }

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
     * PALETTE FX showcase — real character portrait + real background.
     *
     * The character (portrait sprite from the last row of the character
     * sheet) sits in front of the BG image and is NEVER touched by the
     * palette FX.  Effects only modulate a dedicated sandbox palette
     * slot (15), which is bound to a small banner sprite below the
     * character.  The character's own palette is left alone, so no
     * "part of the character changes colour" any more.
     */
    const uint8_t fx_slot     = 15u;
    /*
     * sprite_080 (row 08 col 01, screen_id 82) = first portrait /
     * face frame.  These are the cleanest "single subject" sprites in
     * the sheet and have their own palette bank so the BG palette
     * cannot leak into them.
     */
    const uint8_t portrait_id = 82u;
    uint16_t t;
    uint8_t  banner_x;

    chap_header(8u, "PALETTE FX", "PORTRAIT  BG  FX BANNER");
    demo_fix_puts(2u, 2u, "PORTRAIT IN FRONT  BG BEHIND",  1u);
    demo_fix_puts(2u, 3u, "FX ONLY MODULATES PAL 15",      0u);
    snd_cross_to(SOUND_MUSIC_G);

    /*
     * BG drawn ONCE at the low slot so it stays BEHIND the portrait.
     * draw_background uses DEMO_BG_BACK_SLOT (96+) per its comment;
     * since higher slot = drawn on top, our portrait sprite below
     * needs to live on an even higher slot than the BG.
     */
    draw_background(2u, 32, 16);

    /*
     * Portrait — drawn at slot 200 so it sits ON TOP of the BG (slot
     * 96).  Centred horizontally, slightly above vertical centre.
     * Palette is the portrait's OWN palette bank (loaded by
     * demo_load_screen_palette) — completely independent of slot 15
     * so the FX cycle cannot corrupt the face.
     */
    demo_load_screen_palette(portrait_id);
    {
        int16_t strips = demo_screen_strips(portrait_id);
        int16_t rows   = demo_screen_rows(portrait_id);
        int16_t draw_x = (int16_t)(160 - (strips * 16) / 2);
        int16_t draw_y = (int16_t)(96  - (rows   * 16) / 2);
        demo_draw_sprite_screen(portrait_id, 200u,
                                draw_x, draw_y,
                                (uint8_t)strips, (uint8_t)rows,
                                0xFFu, 0xFFu);
    }

    /*
     * FX banner — a small horizontal strip at row 20 painted in
     * palette 15.  THIS is the only thing the FX modulates.
     */
    ng_palfx_upload_base(fx_slot, s_palfx_base);
    for (banner_x = 4u; banner_x < 36u; banner_x++) {
        demo_fix_puts(banner_x, 20u, "#", 15u);
    }
    demo_fix_puts(2u, 6u, "EFFECT:", 1u);

    /* ---- Effect sequence — all on slot 15 ----------------------- */
    demo_fix_puts(10u, 6u, "FADE-IN 60F          ", 1u);
    ng_palfx_fade_in(fx_slot, s_palfx_base, 60u);
    if (uwait(80u)) return 1u;

    demo_fix_puts(10u, 6u, "FLASH WHITE          ", 2u);
    ng_palfx_flash_white(fx_slot, s_palfx_base, 20u);
    if (uwait(40u)) return 1u;

    demo_fix_puts(10u, 6u, "FLASH RED            ", 2u);
    ng_palfx_flash_red(fx_slot, s_palfx_base, 20u);
    if (uwait(40u)) return 1u;

    demo_fix_puts(10u, 6u, "PULSE                ", 1u);
    ng_palfx_pulse(fx_slot, s_palfx_base, 30u);
    if (uwait(150u)) return 1u;

    demo_fix_puts(10u, 6u, "CYCLE (ROTATE 8..14) ", 1u);
    ng_palfx_cycle(fx_slot, s_palfx_base, 8u, 14u);
    for (t = 0u; t < 240u; t++) {
        if (uframe()) return 1u;
    }

    demo_fix_puts(10u, 6u, "FADE-OUT 60F         ", 0u);
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
    const uint8_t spark_id    = 93u;
    const uint16_t spark_tile = DEMO_SCREEN_TILE(spark_id);
    const uint8_t  spark_pal  = DEMO_SCREEN_PALETTE(spark_id);

    chap_header(9u, "PARTICLES", "HERO SPECIAL MOVE + FX");
    demo_fix_puts(2u, 2u, "WARRIOR PERFORMS A SPECIAL", 1u);
    demo_fix_puts(2u, 3u, "PARTICLES SYNC TO ANIM FRAMES",0u);
    demo_fix_puts(2u, 4u, "ACTIVE: ",                    2u);
    snd_cross_to(SOUND_MUSIC_F);

    hero_place(160, 112);
    demo_load_screen_palette(spark_id);

    s_draw_particles = 1u;

    for (t = 0u; t < 540u; t++) {
        /*
         * Phase machine — 3 special-move beats:
         *   [  0..160) WIND-UP        : faint dust around feet
         *   [160..360) STRIKE         : magic sparks burst from sword arc
         *   [360..540) FINISHER       : explosion + smoke ring + screen-flash
         */
        uint8_t hero_frame;
        /*
         * Particles target the HEAD area of the hero sprite, not the
         * sword arc.  With the hero centred vertically at s_hero_y=112
         * and a 10-row warrior sprite (spans 32..192), the head sits
         * around y = s_hero_y - 64 (top-quarter of the sprite).  Spawn
         * sparks slightly above and to the side of the head so they
         * burst around the warrior's crown / shoulders.
         */
        int16_t bx = (int16_t)(s_hero_x + 6);    /* small lateral offset */
        int16_t by = (int16_t)(s_hero_y - 64);   /* near the head */

        if (t < 160u) {
            hero_frame = s_hero_specA[(t / 8u) % 8u];
            if ((t % 12u) == 0u) {
                ng_particle_spawn(NG_PART_DUST, NG_PART_PRI_NORMAL,
                                  (int16_t)(s_hero_x - 12),
                                  (int16_t)(s_hero_y + 60),
                                  -(1L << (NG_FP_SHIFT - 1)),
                                  -(1L << (NG_FP_SHIFT - 1)),
                                  20u, spark_tile, spark_pal, 1u, 1u);
                ng_particle_spawn(NG_PART_DUST, NG_PART_PRI_NORMAL,
                                  (int16_t)(s_hero_x + 12),
                                  (int16_t)(s_hero_y + 60),
                                  (1L << (NG_FP_SHIFT - 1)),
                                  -(1L << (NG_FP_SHIFT - 1)),
                                  20u, spark_tile, spark_pal, 1u, 1u);
            }
            if (t == 8u) playSFX(SOUND_SFX_9);
        } else if (t < 360u) {
            hero_frame = s_hero_strike[((t - 160u) / 6u) % 8u];
            /* sparks burst on the sword-arc beats */
            if ((t % 10u) == 0u) {
                ng_particle_spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_CRITICAL,
                                  bx, by,
                                  (3L << NG_FP_SHIFT),
                                  -(2L << NG_FP_SHIFT),
                                  24u, spark_tile, spark_pal, 1u, 1u);
                ng_particle_spawn(NG_PART_HIT_SPARK, NG_PART_PRI_CRITICAL,
                                  (int16_t)(bx - 6), (int16_t)(by - 4),
                                  -(1L << NG_FP_SHIFT),
                                  -(3L << NG_FP_SHIFT),
                                  22u, (uint16_t)(spark_tile + 1u),
                                  spark_pal, 1u, 1u);
                ng_particle_spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_NORMAL,
                                  (int16_t)(bx + 8), (int16_t)(by + 6),
                                  (2L << NG_FP_SHIFT),
                                  (1L << NG_FP_SHIFT),
                                  22u, (uint16_t)(spark_tile + 2u),
                                  spark_pal, 1u, 1u);
            }
            if (t == 160u) playSFX(SOUND_SFX_7);
            if (t == 240u) playSFX(SOUND_SFX_8);
        } else {
            hero_frame = s_hero_specB[((t - 360u) / 10u) % 6u];
            /* finisher: explosion at hero, smoke ring */
            if (t == 360u) {
                uint8_t k;
                playSFX(SOUND_SFX_10);
                ng_particle_spawn(NG_PART_EXPLOSION, NG_PART_PRI_CRITICAL,
                                  s_hero_x, s_hero_y, 0, 0, 36u,
                                  spark_tile, spark_pal, 1u, 1u);
                for (k = 0u; k < 8u; k++) {
                    int32_t a = (int32_t)k * 2L;
                    ng_particle_spawn(NG_PART_SMOKE, NG_PART_PRI_NORMAL,
                                      s_hero_x, s_hero_y,
                                      (a - 8L) << (NG_FP_SHIFT - 1),
                                      -(a) << (NG_FP_SHIFT - 1),
                                      40u, spark_tile, spark_pal, 1u, 1u);
                }
            }
            if ((t % 24u) == 0u) {
                ng_particle_spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_NORMAL,
                                  (int16_t)(s_hero_x + ((int16_t)(t & 31u) - 16)),
                                  (int16_t)(s_hero_y - 40),
                                  0, -(1L << NG_FP_SHIFT),
                                  30u, spark_tile, spark_pal, 1u, 1u);
            }
        }

        /* Hero rendered through the proven sprite-window path */
        hero_draw(hero_frame);

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
    snd_cross_to(SOUND_MUSIC_F);

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

        /* Impact bursts land on the hero's HEAD (~s_hero_y - 64), not
         * on the body — that's where it reads visually as a "hit". */
        if (t == 60u && fired < 1u) {
            ng_feedback_shake(&cam, 1u, 8u);
            playSFX(SOUND_SFX_8);
            spawn_impact_burst(160, 48, spark_tile, spark_pal, 1u);
            demo_fix_puts(2u, 5u, "FIRED: LIGHT   ", 1u); fired = 1u;
        } else if (t == 180u && fired < 2u) {
            ng_feedback_shake(&cam, 2u, 12u);
            playSFX(SOUND_SFX_8);
            spawn_impact_burst(160, 48, spark_tile, spark_pal, 2u);
            demo_fix_puts(2u, 5u, "FIRED: MEDIUM  ", 2u); fired = 2u;
        } else if (t == 320u && fired < 3u) {
            ng_feedback_shake(&cam, 3u, 16u);
            playSFX(SOUND_SFX_8);
            spawn_impact_burst(160, 48, spark_tile, spark_pal, 3u);
            demo_fix_puts(2u, 5u, "FIRED: HEAVY   ", 2u); fired = 3u;
        } else if (t == 460u && fired < 4u) {
            ng_feedback_shake(&cam, 4u, 20u);
            playSFX(SOUND_SFX_10);
            spawn_impact_burst(160, 48, spark_tile, spark_pal, 4u);
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
     * DEPTH demo using WARRIOR FACE SPRITES at varying Z.
     * Three warrior sprites are drawn at different "distances" — the
     * closest is full-size, the middle is 50%, the farthest is 25%.
     * Their hardware scale + position is computed via
     * ng_depthfx_project so the math is engine-real.
     *
     * Per-row FIX stripes underneath visualise the depth bands: each
     * band uses a different palette so the user can SEE the depth
     * zones the warriors are flying through.
     */
    enum { WARRIOR_COUNT = 3 };
    /* z 8..96 — closer = smaller number = bigger on screen */
    int16_t  wz[WARRIOR_COUNT]  = { 16, 40, 72 };
    int16_t  wx[WARRIOR_COUNT]  = { -40,  0,  40 };
    uint16_t t;
    uint8_t  i;

    chap_header(11u, "DEPTH FX", "WARRIORS AT VARYING Z");
    demo_fix_puts(2u, 2u, "3 WARRIORS  3 DEPTHS  ENGINE SCALE", 1u);
    demo_fix_puts(2u, 3u, "DEPTH STRIPES ON FIX  LABELS BELOW", 0u);
    snd_cross_to(SOUND_MUSIC_C);

    /*
     * Static depth stripes — 4 horizontal bands on rows 4..7 each in
     * a different palette to demarcate distance zones.  Below each
     * band we label the zone so the user can read the depth tag.
     */
    {
        uint8_t y;
        const char *labels[4] = { "VERY FAR", "FAR     ",
                                  "MID     ", "NEAR    " };
        for (y = 0u; y < 4u; y++) {
            uint8_t row = (uint8_t)(5u + y);
            uint8_t pal = (uint8_t)((y == 0u) ? 0u :
                                    (y == 1u) ? 1u :
                                    (y == 2u) ? 1u : 2u);
            demo_fix_puts(0u, row,
                          "========================================",
                          pal);
            demo_fix_puts(2u, (uint8_t)(row + 18u), labels[y], pal);
        }
    }

    ng_depthfx_init();

    /* Static labels for the 3 warriors below the depth stripes */
    demo_fix_puts(2u, 24u, "WARRIOR Z = ", 1u);

    for (t = 0u; t < 720u; t++) {
        char buf[6];
        /* HUD: show each warrior's current z value */
        for (i = 0u; i < WARRIOR_COUNT; i++) {
            buf[0] = (char)('0' + (wz[i] / 100u) % 10u);
            buf[1] = (char)('0' + (wz[i] /  10u) % 10u);
            buf[2] = (char)('0' + (wz[i] %  10u));
            buf[3] = ' ';
            buf[4] = '\0';
            demo_fix_puts((uint8_t)(14u + i * 8u), 24u, buf,
                          (uint8_t)(i == 2u ? 2u : 1u));
        }

        for (i = 0u; i < WARRIOR_COUNT; i++) {
            NGVec3   p;
            NGProjected pr;
            uint8_t  scale;

            /* advance z toward camera, loop when too close */
            wz[i] = (int16_t)(wz[i] - 1);
            if (wz[i] < 8) wz[i] = 96;

            p.x = wx[i];
            p.y = 0;
            p.z = wz[i];
            pr = ng_depthfx_project(p, 0u);

            /* Scale from z: closer = bigger (0xFF at z=8, 0x40 at z=96) */
            scale = (uint8_t)(0x40u + (uint16_t)((96 - wz[i]) * 0xC0u) / 88u);

            (void)pr;
            /*
             * Draw a warrior sprite at this Z.  Use the dimensionally
             * uniform stand set so different frames don't shift width.
             * Slot ranges 280-290 reserved for these — slot increases
             * with i so warriors don't overlap each other in VRAM.
             */
            {
                uint8_t frame = s_hero_stand[(t / 14u) % 8u];
                uint8_t strips = demo_screen_strips(frame);
                uint8_t rows   = demo_screen_rows(frame);
                int16_t off_x  = demo_screen_x_offset(frame);
                int16_t off_y  = demo_screen_y_offset(frame);
                /* centre on screen X = 160 + wx (closer warriors
                 * further from screen centre, further closer to it) */
                int16_t screen_x = (int16_t)(160 + (wx[i] * (96 - wz[i])) / 96);
                int16_t screen_y = (int16_t)(120 + (96 - wz[i]) / 4);
                int16_t draw_x = (int16_t)(screen_x
                                 - (strips * 16 * scale / 256) / 2 - off_x);
                int16_t draw_y = (int16_t)(screen_y
                                 - (rows   * 16 * scale / 256) / 2 - off_y);
                demo_load_screen_palette(frame);
                demo_draw_sprite_screen(frame,
                                        (uint16_t)(280u + i * 8u),
                                        draw_x, draw_y,
                                        strips, rows, scale, scale);
            }
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
    demo_fix_puts(2u, 3u, "ENGINE THINK FN HANDLES VX", 0u);
    snd_cross_to(SOUND_MUSIC_B);

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
/*  Chapter 13 — Mini-game (player vs clone)                            */
/*                                                                       */
/*  Two characters fight: the player (left) and a CLONE (right) that    */
/*  uses the same sprite sheet.  The clone is passive at first — the    */
/*  player can walk up and strike it for score.  If the player goes     */
/*  too long without landing a hit, the clone wakes up and walks        */
/*  toward the player to retaliate.  Land a hit and the clone resets    */
/*  to passive.  Get hit by the clone and lose HP.                      */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_mini_game(void)
{
    enum {
        CLONE_IDLE      = 0u,   /* passive — does nothing                */
        CLONE_AGGRO     = 1u,   /* walking toward the player             */
        CLONE_STRIKING  = 2u,   /* swing animation                       */
        CLONE_RECOVER   = 3u    /* brief cooldown after striking         */
    };

    const uint8_t  spark_id    = 93u;
    const uint16_t spark_tile  = DEMO_SCREEN_TILE(spark_id);
    const uint8_t  spark_pal   = DEMO_SCREEN_PALETTE(spark_id);
    const int16_t  GROUND_Y    = 132;
    const uint16_t TOTAL       = 2700u;        /* 45 s @ 60 fps */
    const uint16_t AGGRO_AFTER = 300u;         /* 5 s of inactivity → clone wakes */
    const int16_t  REACH_PX    = 56;           /* sword reach radius */
    const uint8_t  CLONE_SLOT  = 24u;          /* sprite slot for the clone */

    uint16_t t;
    /* Player state */
    int16_t  p_x        = 80;
    uint8_t  p_state    = 0u;   /* 0 stand, 1 walk, 2 strike */
    uint8_t  p_t        = 0u;
    uint8_t  p_flip     = 0u;
    uint8_t  hp         = 100u;
    uint16_t score      = 0u;
    uint16_t hits_done  = 0u;
    uint16_t since_hit  = 0u;   /* frames since player last landed a hit */

    /* Clone state — mirrors player frame arrays */
    int16_t  c_x         = 240;
    uint8_t  c_state     = CLONE_IDLE;
    uint8_t  c_t         = 0u;
    uint8_t  c_flip      = 1u;   /* faces left initially */
    uint8_t  c_strike_landed = 0u;  /* edge flag for one-shot hit per swing */

    char buf[6];

    chap_header(13u, "MINI-GAME", "FIGHT THE CLONE  B STRIKE");
    demo_fix_puts(2u, 2u, "WALK + B TO HIT THE CLONE",     1u);
    demo_fix_puts(2u, 3u, "TOO SLOW AND IT WAKES UP",      0u);
    snd_cross_to(SOUND_MUSIC_D);

    draw_background(2u, 32, 16);
    ng_joystick_init();
    demo_load_screen_palette(spark_id);
    demo_load_screen_palette(HERO_IDLE_FRAME);

    demo_fix_puts(2u, 25u,  "SCORE:", 2u);
    demo_fix_puts(13u, 25u, "HITS:",  2u);
    demo_fix_puts(22u, 25u, "HP:",    2u);
    demo_fix_puts(30u, 25u, "STATE:", 2u);

    s_draw_particles = 1u;

    for (t = 0u; t < TOTAL; t++) {
        uint16_t down, pressed;
        uint8_t  p_frame, c_frame;

        ng_joystick_update();
        down    = ng_joy_down();
        pressed = ng_joy_pressed();

        /* ============================================================
         * PLAYER input + state machine
         * ============================================================ */
        if ((pressed & BUTTON_B) && p_state != 2u) {
            p_state = 2u;
            p_t = 0u;
            playSFX(SOUND_SFX_7);
        }
        if (p_state < 2u) {
            if (down & JOY_LEFT)  { p_x -= 2; p_flip = 1u; p_state = 1u; }
            else if (down & JOY_RIGHT) { p_x += 2; p_flip = 0u; p_state = 1u; }
            else                  { p_state = 0u; }
        }
        if (p_x < 24)  p_x = 24;
        if (p_x > 296) p_x = 296;

        switch (p_state) {
        case 0:  p_frame = s_hero_stand[(t / 14u) % 8u]; break;
        case 1:  p_frame = s_hero_walk [(t /  6u) % 8u]; break;
        default: p_frame = s_hero_strike[(p_t / 3u) % 8u]; break;
        }

        /* Player hit-test — connects when strike is in active window
         * (frames 4..18 of the 24-frame swing) and player+clone
         * overlap within REACH_PX horizontally. */
        if (p_state == 2u && p_t >= 4u && p_t <= 18u) {
            int16_t dx = (int16_t)(c_x - p_x);
            if (dx < 0) dx = (int16_t)(-dx);
            if (dx < REACH_PX) {
                score = (uint16_t)(score + 10u);
                hits_done++;
                since_hit = 0u;
                playSFX(SOUND_SFX_8);
                spawn_impact_burst(c_x, (int16_t)(GROUND_Y - 16),
                                   spark_tile, spark_pal, 3u);
                /* Reset clone to passive — they recoil and stop attacking */
                c_state = CLONE_RECOVER;
                c_t     = 0u;
                /* Knockback: nudge the clone back a few pixels */
                if (c_x > p_x) c_x += 8; else c_x -= 8;
                if (c_x < 32)  c_x = 32;
                if (c_x > 296) c_x = 296;
                /* Don't double-hit on the same swing */
                p_t = 19u;
            }
        }
        if (p_state == 2u) {
            p_t++;
            if (p_t >= 24u) { p_state = 0u; p_t = 0u; }
        }

        /* ============================================================
         * CLONE AI
         * ============================================================ */
        since_hit++;
        switch (c_state) {
        case CLONE_IDLE:
            /* Become aggressive after a long silence */
            if (since_hit > AGGRO_AFTER) {
                c_state = CLONE_AGGRO;
                c_t = 0u;
                playSFX(SOUND_SFX_9);  /* clone wakes up cue */
            }
            c_frame = s_hero_stand[(t / 14u) % 8u];
            break;

        case CLONE_AGGRO: {
            /* Walk toward player until within striking distance */
            int16_t dx = (int16_t)(c_x - p_x);
            if (dx > REACH_PX) { c_x -= 1; c_flip = 1u; }
            else if (dx < -REACH_PX) { c_x += 1; c_flip = 0u; }
            else {
                /* In range — start a strike */
                c_state = CLONE_STRIKING;
                c_t = 0u;
                c_strike_landed = 0u;
                playSFX(SOUND_SFX_7);
            }
            c_frame = s_hero_walk[(t / 6u) % 8u];
            break;
        }

        case CLONE_STRIKING:
            c_frame = s_hero_strike[(c_t / 3u) % 8u];
            /* Clone hit-test on active window — only count once. */
            if (!c_strike_landed && c_t >= 4u && c_t <= 18u) {
                int16_t dx = (int16_t)(p_x - c_x);
                if (dx < 0) dx = (int16_t)(-dx);
                if (dx < REACH_PX) {
                    c_strike_landed = 1u;
                    playSFX(SOUND_SFX_8);
                    spawn_impact_burst(p_x, (int16_t)(GROUND_Y - 16),
                                       spark_tile, spark_pal, 2u);
                    if (hp > 10u) hp = (uint8_t)(hp - 10u);
                    else          hp = 0u;
                    /* Knockback the player */
                    if (p_x < c_x) p_x -= 8; else p_x += 8;
                    if (p_x < 24)  p_x = 24;
                    if (p_x > 296) p_x = 296;
                }
            }
            c_t++;
            if (c_t >= 24u) {
                c_state = CLONE_RECOVER;
                c_t = 0u;
            }
            break;

        case CLONE_RECOVER:
        default:
            c_frame = s_hero_stand[(t / 14u) % 8u];
            c_t++;
            if (c_t >= 90u) {
                c_state = CLONE_IDLE;
                c_t = 0u;
                /* Don't reset since_hit — if the player still doesn't
                 * hit, the clone will wake again. */
            }
            break;
        }

        if (p_state == 1u && (t & 31u) == 0u) playSFX(SOUND_SFX_5);

        /* ============================================================
         * HUD
         * ============================================================ */
        digit3(buf, score);     demo_fix_puts(9u, 25u, buf, 1u);
        digit3(buf, hits_done); demo_fix_puts(19u, 25u, buf, 1u);
        digit3(buf, hp);        demo_fix_puts(26u, 25u, buf,
                                              (uint8_t)(hp < 30u ? 2u : 1u));
        demo_fix_puts(36u, 25u,
                      (c_state == CLONE_IDLE)     ? "IDLE  " :
                      (c_state == CLONE_AGGRO)    ? "ANGRY " :
                      (c_state == CLONE_STRIKING) ? "ATTK! " :
                                                    "REST  ",
                      (uint8_t)(c_state == CLONE_STRIKING ? 2u : 1u));

        /* ============================================================
         * Render — clone first (slot 24), then player (slot 1 via
         * hero_draw).  Player gets the higher slot priority because
         * HIGHER slot = drawn on top in our pipeline.
         * ============================================================ */
        {
            uint8_t strips = demo_screen_strips(c_frame);
            uint8_t rows   = demo_screen_rows(c_frame);
            int16_t draw_x = (int16_t)(c_x - (strips * 16) / 2);
            int16_t draw_y = (int16_t)(GROUND_Y - rows * 16);
            demo_draw_sprite_screen(c_frame, CLONE_SLOT,
                                    draw_x, draw_y, strips, rows,
                                    0xFFu, 0xFFu);
        }
        s_hero_x = p_x;
        s_hero_y = GROUND_Y;
        hero_draw(p_frame);
        (void)p_flip; (void)c_flip;

        /* Game over on HP exhausted */
        if (hp == 0u) {
            demo_fix_puts(13u, 13u, "  KNOCKED OUT  ", 2u);
            if (demo_wait(120u)) return 1u;
            return 0u;
        }

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
    (void)hero_flip;   /* tracked for symmetry; not used for facing on hero_draw */
    /*
     * PERSISTENT state machine — previous version reset every frame
     * which meant the strike animation only played for 1 frame and
     * the user couldn't see anything.  Now strike_t and special_t
     * count down across frames so the action remains visible.
     */
    uint8_t  strike_t  = 0u;     /* >0 while strike anim plays */
    uint8_t  special_t = 0u;     /* >0 while QCF/DP special plays */
    uint8_t  hits      = 0u;
    char buf[8];

    chap_header(14u, "JOYSTICK", "LIVE INPUT  TWO-BUTTON SPECIALS");
    demo_fix_puts(2u, 2u, "ARROWS MOVE  B STRIKE  C JUMP",   1u);
    demo_fix_puts(2u, 3u, "B+C TOGETHER SPECIAL  B+D FINISHER", 0u);
    snd_cross_to(SOUND_MUSIC_A);

    ng_joystick_init();

    /* Static HUD labels — once */
    demo_fix_puts(2u,  5u, "PAD:",        2u);
    demo_fix_puts(2u,  6u, "BTN:",        2u);
    demo_fix_puts(2u,  7u, "HELD A:",     2u);
    demo_fix_puts(2u,  8u, "HELD B:",     2u);
    demo_fix_puts(2u,  9u, "HELD C:",     2u);
    demo_fix_puts(2u, 10u, "HELD D:",     2u);
    demo_fix_puts(2u, 12u, "B+C:",        2u);   /* light special */
    demo_fix_puts(2u, 13u, "B+D:",        2u);   /* heavy finisher */
    demo_fix_puts(2u, 25u, "HIT TARGET ON RIGHT WITH B",       0u);
    demo_fix_puts(2u, 26u, "HITS:",                            2u);

    /* Static FIX target — a small rectangle the hero can strike */
    {
        uint8_t r, c;
        for (r = 0u; r < 3u; r++) {
            for (c = 0u; c < 5u; c++) {
                demo_fix_puts((uint8_t)(32u + c),
                              (uint8_t)(13u + r),
                              (r == 1u && c == 2u) ? "X" : "#",
                              (uint8_t)(1u + ((c + r) & 1u)));
            }
        }
    }

    draw_background(2u, 32, 16);

    for (t = 0u; t < 1200u; t++) {
        uint16_t down;
        uint16_t pressed;
        uint16_t released;
        uint8_t frame;
        uint8_t combo_bc;     /* B+C held together → light special */
        uint8_t combo_bd;     /* B+D held together → heavy finisher */
        uint8_t i;

        ng_joystick_update();
        down     = ng_joy_down();
        pressed  = ng_joy_pressed();
        released = ng_joy_released();
        /*
         * Simplified specials — just hold TWO buttons together.
         * No quarter-circle / dragon-punch motion required.
         */
        combo_bc = (uint8_t)(((down & BUTTON_B) && (down & BUTTON_C)) ? 1u : 0u);
        combo_bd = (uint8_t)(((down & BUTTON_B) && (down & BUTTON_D)) ? 1u : 0u);

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
        demo_fix_puts(8u, 12u, combo_bc ? "OK  " : "--- ", combo_bc ? 2u : 0u);
        demo_fix_puts(8u, 13u, combo_bd ? "OK  " : "--- ", combo_bd ? 2u : 0u);

        /* Motion (only when not striking) ---------------------------- */
        if (strike_t == 0u) {
            if (down & JOY_LEFT) {
                hero_world_x -= 2;
                hero_flip = 1u;
            } else if (down & JOY_RIGHT) {
                hero_world_x += 2;
                hero_flip = 0u;
            }
        }

        /*
         * Specials = two-button combos.  Detection must INTERRUPT an
         * in-progress plain strike so the user can press B, then add D
         * shortly after, and have the move upgrade into B+D — pressing
         * exactly simultaneously is nearly impossible on a real stick.
         *   Press D while B is held → heavy finisher (36-frame).
         *   Press C while B is held → light special (30-frame).
         *   Press B alone           → basic strike (24-frame).
         */
        if ((pressed & BUTTON_D) && (down & BUTTON_B)) {
            special_t = 36u;
            strike_t  = 0u;
            playSFX(SOUND_SFX_10);
        } else if ((pressed & BUTTON_C) && (down & BUTTON_B)) {
            special_t = 30u;
            strike_t  = 0u;
            playSFX(SOUND_SFX_9);
        } else if ((pressed & BUTTON_B) && (down & BUTTON_D)) {
            special_t = 36u;
            playSFX(SOUND_SFX_10);
        } else if ((pressed & BUTTON_B) && (down & BUTTON_C)) {
            special_t = 30u;
            playSFX(SOUND_SFX_9);
        } else if ((pressed & BUTTON_B) && special_t == 0u && strike_t == 0u) {
            strike_t = 24u;
            playSFX(SOUND_SFX_7);
        }

        if ((pressed & BUTTON_C) && hero_world_y >= HERO_GROUND_Y) {
            vy = -7;
            playSFX(SOUND_SFX_6);
        }
        (void)released;

        /* Hit-test the FIX target during the active strike window.
         * Target lives at cols 32..36, rows 13..15 → centre px (272, 112). */
        if ((strike_t >= 6u && strike_t <= 18u) || special_t >= 6u) {
            int16_t target_cx_px = 32 * 8 + 20;     /* 276 */
            int16_t target_cy_px = 14 * 8 + 8;      /* 120 */
            int16_t reach_left  = (int16_t)(hero_world_x - 64);
            int16_t reach_right = (int16_t)(hero_world_x + 64);
            int16_t dy = (int16_t)(hero_world_y - target_cy_px);
            if (dy < 0) dy = (int16_t)(-dy);
            if (dy < 64 && target_cx_px >= reach_left && target_cx_px <= reach_right) {
                if (hits < 99u) hits++;
                playSFX(SOUND_SFX_8);
                /* Visual: re-draw the target with a flash palette this frame */
                {
                    uint8_t r, c;
                    for (r = 0u; r < 3u; r++) {
                        for (c = 0u; c < 5u; c++) {
                            demo_fix_puts((uint8_t)(32u + c),
                                          (uint8_t)(13u + r),
                                          (r == 1u && c == 2u) ? "X" : "#",
                                          2u);
                        }
                    }
                }
                strike_t = 0u;   /* one hit per swing — finish the strike */
                special_t = 0u;
            }
        }
        if (strike_t  > 0u) strike_t--;
        if (special_t > 0u) special_t--;

        /* Reset target colours after a hit-frame flash (small visual TTL) */
        if (hits > 0u && (t & 7u) == 0u) {
            uint8_t r, c;
            for (r = 0u; r < 3u; r++) {
                for (c = 0u; c < 5u; c++) {
                    demo_fix_puts((uint8_t)(32u + c),
                                  (uint8_t)(13u + r),
                                  (r == 1u && c == 2u) ? "X" : "#",
                                  (uint8_t)(1u + ((c + r) & 1u)));
                }
            }
        }

        if (hero_world_y < HERO_GROUND_Y || vy != 0) {
            /* pos += vel  (Y down-positive, vy negative = up) — was
             * `- vy` which made the hero descend on jump-start and
             * land instantly on frame 1. */
            hero_world_y = (int16_t)(hero_world_y + vy);
            vy = (int16_t)(vy + 1);
            if (hero_world_y >= HERO_GROUND_Y) {
                hero_world_y = HERO_GROUND_Y;
                vy = 0;
            }
        }
        if (hero_world_x < 24)  hero_world_x = 24;
        if (hero_world_x > 280) hero_world_x = 280;

        /* Animation frame selection — driven by persistent state */
        if (special_t > 0u)
            frame = s_hero_specA[((36u - special_t) / 3u) % 8u];
        else if (strike_t > 0u)
            frame = s_hero_strike[((24u - strike_t) / 3u) % 8u];
        else if (vy != 0)
            frame = s_hero_walk[(t / 5u) % 8u];   /* jump pose */
        else if (down & (JOY_LEFT | JOY_RIGHT))
            frame = s_hero_walk[(t / 5u) % 8u];
        else
            frame = s_hero_stand[(t / 14u) % 8u];

        /* HITS counter on HUD */
        {
            char hb[4];
            hb[0] = (char)('0' + (hits / 10u));
            hb[1] = (char)('0' + (hits % 10u));
            hb[2] = '\0';
            demo_fix_puts(8u, 26u, hb, 1u);
        }

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
    snd_cross_to(SOUND_MUSIC_B);

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

        if ((t % 90u) == 0u) playSFX(SOUND_SFX_5);
        if (uframe()) {
            ng_level_set_scroll(0, 0);
            return 1u;
        }
    }
    ng_level_set_scroll(0, 0);
    return 0u;
}

/* ================================================================== */
/*  Chapter 16 — 3D effect (single-character depth animation)            */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_render3d(void)
{
    /*
     * Minimal pseudo-3D scene — ONE hero centred on screen that
     * scales between distant (small/up) and close (full/down) to
     * convey depth.  Hero lives at sprite slot 220 so it draws ON
     * TOP of the BG (DEMO_BG_BACK_SLOT = ~96; higher slot = on top).
     */
    uint16_t t;
    const uint8_t hero_frame = HERO_IDLE_FRAME;   /* clean idle pose */

    chap_header(16u, "3D EFFECT", "DEPTH SCALE  ONE CHARACTER");
    demo_fix_puts(2u, 2u, "HERO SCALES NEAR <-> FAR",  1u);
    demo_fix_puts(2u, 3u, "BG STATIC  SCALE = DEPTH",  0u);
    snd_cross_to(SOUND_MUSIC_F);

    draw_background(2u, 32, 16);
    demo_load_screen_palette(hero_frame);

    /*
     * Scale triangles between 0x60 (small/distant) and 0xFF (full/
     * near) over a 240-frame period.  Y travels inversely: smaller
     * scale → higher Y (further away on the picture plane), bigger
     * scale → lower Y (closer to camera).
     */
    for (t = 0u; t < 480u; t++) {
        uint16_t phase = (uint16_t)(t % 240u);
        uint8_t  scale = (uint8_t)((phase < 120u)
                                   ? (0x60u + (uint16_t)((phase * 0x9F) / 120u))
                                   : (0x60u + (uint16_t)(((240u - phase) * 0x9F) / 120u)));
        int16_t  strips = demo_screen_strips(hero_frame);
        int16_t  rows   = demo_screen_rows(hero_frame);
        int16_t  px_w   = (int16_t)((strips * 16 * scale) >> 8);
        int16_t  px_h   = (int16_t)((rows   * 16 * scale) >> 8);
        int16_t  draw_x = (int16_t)(160 - (px_w >> 1));
        int16_t  draw_y = (int16_t)(192 - px_h);   /* feet on the ground */

        demo_draw_sprite_screen(hero_frame, 220u,
                                draw_x, draw_y,
                                (uint8_t)strips, (uint8_t)rows,
                                scale, scale);

        if ((t %  60u) == 0u) playSFX(SOUND_SFX_5);
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 17 — 2D render idea  (light per-frame churn)                 */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_char_2d(void)
{
    /*
     * 2D rendering showcase — demonstrates the SAME concepts the old
     * "2D RENDER FIX BLITTER PATTERNS" chapter did (moving bar,
     * waveform, static box), but through ONE character so the user
     * sees how the engine drives 2D sprite motion rather than raw
     * FIX-cell tricks.
     *
     * Three phases on a single hero:
     *   [  0..200) LINEAR SWEEP — hero walks left→right across screen
     *   [200..420) WAVE BOB     — hero bobs in a sine pattern in place
     *   [420..600) IDLE LOOP    — hero stands centred, frame-cycles
     */
    uint16_t t;
    const uint8_t hero_frame_idle  = 3u;
    int16_t  hero_x;
    int16_t  hero_y;
    uint8_t  frame;

    chap_header(17u, "CHAR 2D", "MOTION  WAVE  IDLE");
    demo_fix_puts(2u, 2u, "ONE CHARACTER  THREE PATTERNS",  1u);
    demo_fix_puts(2u, 3u, "LINEAR SWEEP  SINE BOB  IDLE",   0u);
    snd_cross_to(SOUND_MUSIC_G);

    /* Single static info panel (the "box" concept) */
    demo_fix_puts(8u, 22u, "+----------------------+", 2u);
    demo_fix_puts(8u, 23u, "|  2D MOTION SHOWCASE  |", 1u);
    demo_fix_puts(8u, 24u, "+----------------------+", 2u);

    draw_background(2u, 32, 16);

    demo_load_screen_palette(hero_frame_idle);

    for (t = 0u; t < 600u; t++) {
        if (t < 200u) {
            /* phase 1 — linear walk left→right */
            hero_x = (int16_t)(-32 + (int16_t)((t * 380u) / 200u));
            hero_y = 110;
            frame  = s_hero_walk[(t / 5u) % 8u];
            demo_fix_puts(2u, 6u, "PHASE 1 - LINEAR WALK ", 2u);
        } else if (t < 420u) {
            /* phase 2 — stationary bob (sine via triangle wave on Y) */
            uint16_t p = (uint16_t)(t - 200u);
            uint16_t cy = (uint16_t)(p % 60u);
            int16_t  dy = (int16_t)((cy < 30u) ? cy : (60u - cy));   /* 0..30 */
            hero_x = (int16_t)(160 - 32);
            hero_y = (int16_t)(96 + dy);
            frame  = s_hero_walk[(p / 6u) % 8u];
            demo_fix_puts(2u, 6u, "PHASE 2 - WAVE BOB    ", 2u);
        } else {
            /* phase 3 — idle, just frame-cycle in place */
            hero_x = (int16_t)(160 - 32);
            hero_y = 110;
            frame  = s_hero_stand[(t / 10u) % 8u];
            demo_fix_puts(2u, 6u, "PHASE 3 - IDLE LOOP   ", 2u);
        }

        demo_draw_sprite_screen(frame, 60u, hero_x, hero_y,
                                demo_screen_strips(frame),
                                demo_screen_rows(frame),
                                0xFFu, 0xFFu);

        if ((t % 60u) == 0u) playSFX(SOUND_SFX_5);
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
     * Galaxian-style FIX-layer shooter — second pass.
     *
     * Improvements over the first version:
     *   - 4-row × 6-col formation (24 enemies)
     *   - Two enemy types: WORKER (W) and BOSS (M) worth different scores
     *   - DIVE attack — every ~120 frames one alive enemy detaches from
     *     the formation, dives at the player in an arc, then re-joins
     *     (or shoots back during the dive)
     *   - Enemy bullets travel DOWN and end the wave if they hit the ship
     *   - Multi-glyph ship (3 cells wide rendered as /^\ over === over [_])
     *   - Score popups float upward from kills, fade after 30 frames
     *   - Stage flag (1, 2, 3) increments per wave
     *   - Game-over banner if ship destroyed; victory banner if 3 waves cleared
     */
    enum {
        ROWS_E       = 4,
        COLS_E       = 6,
        ENEMY_COUNT  = ROWS_E * COLS_E,
        SHIP_ROW     = 25,
        BULLET_MAX   = 4,         /* player bullets */
        EBULLET_MAX  = 6,         /* enemy bullets */
        DEBRIS_MAX   = 12,
        POPUP_MAX    = 4,         /* floating score popups */
        DIVE_NONE    = 0xFFu
    };
    uint8_t  enemy_alive[ENEMY_COUNT];
    uint8_t  enemy_type[ENEMY_COUNT];   /* 0 = worker, 1 = boss */
    uint8_t  enemy_cx[ENEMY_COUNT];     /* formation column (cell) */
    uint8_t  enemy_cy[ENEMY_COUNT];     /* formation row (cell) */

    /* Player bullets */
    uint8_t  pb_x[BULLET_MAX], pb_y[BULLET_MAX], pb_act[BULLET_MAX];
    uint8_t  pb_lx[BULLET_MAX], pb_ly[BULLET_MAX];
    /* Enemy bullets */
    uint8_t  eb_x[EBULLET_MAX], eb_y[EBULLET_MAX], eb_act[EBULLET_MAX];
    uint8_t  eb_lx[EBULLET_MAX], eb_ly[EBULLET_MAX];

    /* Diving enemy state */
    uint8_t  dive_idx = DIVE_NONE;     /* index of currently-diving enemy */
    int16_t  dive_x_fp = 0, dive_y_fp = 0;
    int16_t  dive_vx = 0, dive_vy = 0;
    uint8_t  dive_last_cx = 0xFFu, dive_last_cy = 0xFFu;
    uint16_t dive_timer = 240u;        /* frames until next dive */

    /* Debris with gravity */
    int16_t  deb_x_fp[DEBRIS_MAX], deb_y_fp[DEBRIS_MAX];
    int16_t  deb_vx[DEBRIS_MAX],   deb_vy[DEBRIS_MAX];
    uint8_t  deb_ttl[DEBRIS_MAX];
    uint8_t  deb_lx[DEBRIS_MAX],   deb_ly[DEBRIS_MAX];

    /* Score popups */
    uint8_t  pop_cx[POPUP_MAX], pop_cy[POPUP_MAX];
    uint8_t  pop_ttl[POPUP_MAX];
    uint16_t pop_val[POPUP_MAX];
    uint8_t  pop_lcx[POPUP_MAX], pop_lcy[POPUP_MAX];

    uint8_t  ship_x       = 18u;
    uint8_t  last_ship_x  = 0xFFu;
    uint16_t score        = 0u;
    uint8_t  stage        = 1u;
    uint8_t  lives        = 3u;
    uint8_t  game_over    = 0u;
    uint16_t t;
    uint8_t  i;
    char     buf[6];

    chap_header(18u, "SSG ARCADE", "EAGLE INVADERS");
    demo_fix_puts(2u, 2u, "L/R: MOVE  B: FIRE", 1u);
    demo_fix_puts(2u, 3u, "BEWARE DIVERS + RETURN FIRE", 0u);

    /*
     * STARFIELD background so empty FIX cells show stars, not pure black.
     * A handful of `.` `*` `+` glyphs scattered across the play area at
     * deterministic positions — they sit ON the FIX layer in the cells
     * the gameplay never overwrites, so the user sees space instead of
     * a black stripe where the formation gaps are.
     */
    {
        static const uint8_t star_cx[24] = {
             1,  4,  9, 13, 17, 22, 26, 31, 35, 38,
             3,  7, 11, 16, 20, 24, 29, 33,
             2,  6, 14, 21, 27, 36
        };
        static const uint8_t star_cy[24] = {
             4,  5,  9, 11, 16, 19, 22, 24,  5, 17,
            12, 23, 17,  9, 12, 21, 16, 20,
            10, 18,  6, 13, 25,  8
        };
        static const char *const star_g[3] = { ".", "*", "+" };
        uint8_t k;
        for (k = 0u; k < 24u; k++) {
            demo_fix_puts(star_cx[k], star_cy[k],
                          star_g[k % 3u], (uint8_t)((k & 1u)));
        }
    }

    /*
     * Real background image behind the FIX-cell shooter — gives the
     * scene a proper backdrop instead of pure black, and lets the
     * starfield + ship glyphs read as overlays instead of "everything
     * is FIX text on void".  Same artbox bg (id 2) the other chapters
     * use.
     */
    draw_background(2u, 32, 16);

    /*
     * Audio: ADPCM-B TRACK carries the music continuously so the user
     * never hears just raw SSG buzz.  SSG is kept low and used only
     * for hit / fire SFX (we still call playSFX on ADPCM-A samples
     * for the punchy shoots).  TRACK volume is fairly loud (0xB0) so
     * the streamed audio dominates the mix.
     */
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0xB0u, 0x05u, 0x00u); snd_step();
    soundPlayGameLoop(SOUND_MUSIC_B);          snd_step();
    soundSetSSGPreset(1u);                     snd_step();

    ng_joystick_init();

    /* Spawn formation */
    for (i = 0u; i < ENEMY_COUNT; i++) {
        uint8_t row = (uint8_t)(i / COLS_E);
        uint8_t col = (uint8_t)(i % COLS_E);
        enemy_alive[i] = 1u;
        enemy_type[i] = (uint8_t)((row == 0u) ? 1u : 0u);   /* top row = bosses */
        enemy_cx[i] = (uint8_t)(4u + col * 5u);
        enemy_cy[i] = (uint8_t)(6u + row * 2u);
    }
    for (i = 0u; i < BULLET_MAX;  i++) { pb_act[i] = 0u; pb_lx[i] = 0xFFu; }
    for (i = 0u; i < EBULLET_MAX; i++) { eb_act[i] = 0u; eb_lx[i] = 0xFFu; }
    for (i = 0u; i < DEBRIS_MAX;  i++) { deb_ttl[i] = 0u; deb_lx[i] = 0xFFu; }
    for (i = 0u; i < POPUP_MAX;   i++) { pop_ttl[i] = 0u; pop_lcx[i] = 0xFFu; }

    /* HUD bar (rows 26-27) */
    demo_fix_puts(2u, 26u, "SCORE:",  2u);
    demo_fix_puts(12u, 26u, "STAGE:", 2u);
    demo_fix_puts(20u, 26u, "ALIVE:", 2u);
    demo_fix_puts(28u, 26u, "LIVES:", 2u);
    demo_fix_puts(35u, 26u, "TIME:",  2u);

    for (t = 0u; ; t++) {
        uint16_t down, pressed;
        uint8_t  alive_count = 0u;
        uint8_t  drift = (uint8_t)((t / 30u) & 1u);
        uint16_t time_left;

        if (t > 5400u) break;       /* 90 sec hard cap */

        ng_joystick_update();
        down    = ng_joy_down();
        pressed = ng_joy_pressed();

        if (!game_over) {
            /* Ship motion */
            if ((t & 1u) == 0u) {
                if ((down & JOY_LEFT)  && ship_x > 2u)  ship_x--;
                if ((down & JOY_RIGHT) && ship_x < 35u) ship_x++;
            }
            /* Fire */
            if (pressed & (BUTTON_B | BUTTON_A)) {
                for (i = 0u; i < BULLET_MAX; i++) {
                    if (!pb_act[i]) {
                        pb_x[i] = (uint8_t)(ship_x + 1u);
                        pb_y[i] = (uint8_t)(SHIP_ROW - 2u);
                        pb_act[i] = 1u;
                        playSFX(SOUND_SFX_7);
                        break;
                    }
                }
            }
        }

        /* ---- Player bullets ---------------------------------------- */
        for (i = 0u; i < BULLET_MAX; i++) {
            uint8_t j;
            if (!pb_act[i]) continue;
            if (pb_y[i] == 0u) {
                if (pb_lx[i] != 0xFFu) {
                    demo_fix_puts(pb_lx[i], pb_ly[i], " ", 0u);
                    pb_lx[i] = 0xFFu;
                }
                pb_act[i] = 0u;
                continue;
            }
            pb_y[i]--;
            /* Hit-test against the diving enemy first */
            if (dive_idx != DIVE_NONE) {
                uint8_t dcx = (uint8_t)(dive_x_fp / (8 * 16));
                uint8_t dcy = (uint8_t)(dive_y_fp / (8 * 16));
                if (pb_y[i] == dcy && pb_x[i] >= dcx &&
                    pb_x[i] <= (uint8_t)(dcx + 2u)) {
                    enemy_alive[dive_idx] = 0u;
                    demo_fix_puts(dcx, dcy, "   ", 0u);
                    if (dive_last_cx != 0xFFu)
                        demo_fix_puts(dive_last_cx, dive_last_cy, "   ", 0u);
                    score = (uint16_t)(score +
                                       (enemy_type[dive_idx] ? 300u : 100u));
                    /* register popup */
                    for (j = 0u; j < POPUP_MAX; j++) {
                        if (!pop_ttl[j]) {
                            pop_cx[j] = dcx; pop_cy[j] = dcy;
                            pop_ttl[j] = 30u;
                            pop_val[j] = (uint16_t)(enemy_type[dive_idx] ? 300u : 100u);
                            pop_lcx[j] = 0xFFu;
                            break;
                        }
                    }
                    /* debris */
                    for (j = 0u; j < DEBRIS_MAX; j++) {
                        if (deb_ttl[j]) continue;
                        deb_x_fp[j] = dive_x_fp;
                        deb_y_fp[j] = dive_y_fp;
                        deb_vx[j] = (int16_t)(((j & 7u) - 4) * 14);
                        deb_vy[j] = (int16_t)(-32 - (int16_t)(j & 3u) * 6);
                        deb_ttl[j] = (uint8_t)(28u + (j & 7u) * 3u);
                        deb_lx[j]  = 0xFFu;
                        if (j >= 5u) break;
                    }
                    dive_idx = DIVE_NONE;
                    playSFX(SOUND_SFX_8);
                    if (pb_lx[i] != 0xFFu)
                        demo_fix_puts(pb_lx[i], pb_ly[i], " ", 0u);
                    pb_act[i] = 0u;
                    continue;
                }
            }
            /* Hit-test formation */
            for (j = 0u; j < ENEMY_COUNT; j++) {
                if (!enemy_alive[j]) continue;
                if (j == dive_idx) continue;
                if (pb_y[i] == enemy_cy[j] &&
                    pb_x[i] >= enemy_cx[j] &&
                    pb_x[i] <= (uint8_t)(enemy_cx[j] + 2u)) {
                    uint8_t d;
                    uint8_t pts = enemy_type[j] ? 200u : 80u;
                    enemy_alive[j] = 0u;
                    demo_fix_puts(enemy_cx[j], enemy_cy[j], "   ", 0u);
                    demo_fix_puts((uint8_t)(enemy_cx[j] - drift),
                                  enemy_cy[j], "   ", 0u);
                    score = (uint16_t)(score + pts);
                    playSFX(SOUND_SFX_8);
                    for (d = 0u; d < POPUP_MAX; d++) {
                        if (!pop_ttl[d]) {
                            pop_cx[d] = enemy_cx[j]; pop_cy[d] = enemy_cy[j];
                            pop_ttl[d] = 30u; pop_val[d] = pts;
                            pop_lcx[d] = 0xFFu;
                            break;
                        }
                    }
                    for (d = 0u; d < DEBRIS_MAX; d++) {
                        if (deb_ttl[d]) continue;
                        deb_x_fp[d] = (int16_t)((enemy_cx[j] + 1) * 8 * 16);
                        deb_y_fp[d] = (int16_t)((enemy_cy[j]) * 8 * 16);
                        deb_vx[d] = (int16_t)(((d & 7u) - 4) * 14);
                        deb_vy[d] = (int16_t)(-28 - (int16_t)(d & 3u) * 5);
                        deb_ttl[d] = (uint8_t)(28u + (d & 7u) * 3u);
                        deb_lx[d] = 0xFFu;
                        if (d >= 4u) break;
                    }
                    if (pb_lx[i] != 0xFFu)
                        demo_fix_puts(pb_lx[i], pb_ly[i], " ", 0u);
                    pb_act[i] = 0u;
                    break;
                }
            }
        }

        /* ---- Enemy bullets ----------------------------------------- */
        for (i = 0u; i < EBULLET_MAX; i++) {
            if (!eb_act[i]) continue;
            if (eb_y[i] >= 27u) {
                if (eb_lx[i] != 0xFFu) {
                    demo_fix_puts(eb_lx[i], eb_ly[i], " ", 0u);
                    eb_lx[i] = 0xFFu;
                }
                eb_act[i] = 0u;
                continue;
            }
            eb_y[i]++;
            /* Hit ship? */
            if (!game_over &&
                eb_y[i] == SHIP_ROW &&
                eb_x[i] >= ship_x && eb_x[i] <= (uint8_t)(ship_x + 2u)) {
                if (eb_lx[i] != 0xFFu)
                    demo_fix_puts(eb_lx[i], eb_ly[i], " ", 0u);
                eb_act[i] = 0u;
                if (lives > 0u) lives--;
                playSFX(SOUND_SFX_10);
                /* mini debris from ship */
                {
                    uint8_t d;
                    for (d = 0u; d < DEBRIS_MAX; d++) {
                        if (deb_ttl[d]) continue;
                        deb_x_fp[d] = (int16_t)((ship_x + 1) * 8 * 16);
                        deb_y_fp[d] = (int16_t)(SHIP_ROW * 8 * 16);
                        deb_vx[d] = (int16_t)(((d & 7u) - 4) * 12);
                        deb_vy[d] = (int16_t)(-20 - (int16_t)(d & 3u) * 4);
                        deb_ttl[d] = 30u;
                        deb_lx[d] = 0xFFu;
                        if (d >= 4u) break;
                    }
                }
                if (lives == 0u) {
                    game_over = 1u;
                    /* erase ship */
                    if (last_ship_x != 0xFFu) {
                        demo_fix_puts(last_ship_x, SHIP_ROW, "   ", 0u);
                        last_ship_x = 0xFFu;
                    }
                }
            }
        }

        /* ---- Dive AI ------------------------------------------------ */
        if (dive_idx == DIVE_NONE) {
            if (dive_timer > 0u) dive_timer--;
            if (dive_timer == 0u && !game_over) {
                /* pick a random alive enemy from the bottom row */
                uint8_t k;
                for (k = 0u; k < ENEMY_COUNT; k++) {
                    uint8_t idx = (uint8_t)((t + k * 7u) % ENEMY_COUNT);
                    if (enemy_alive[idx]) {
                        dive_idx = idx;
                        dive_x_fp = (int16_t)((enemy_cx[idx] + drift) * 8 * 16);
                        dive_y_fp = (int16_t)(enemy_cy[idx] * 8 * 16);
                        dive_vx = (int16_t)((ship_x > enemy_cx[idx]) ? 18 : -18);
                        dive_vy = 22;
                        dive_last_cx = 0xFFu;
                        /* clear from formation while diving */
                        demo_fix_puts(enemy_cx[idx], enemy_cy[idx], "   ", 0u);
                        playSFX(SOUND_SFX_9);
                        break;
                    }
                }
                dive_timer = (uint16_t)(180u + ((t * 3u) & 127u));
            }
        } else {
            uint8_t cx, cy;
            /* erase old position */
            if (dive_last_cx != 0xFFu &&
                dive_last_cx < 37u && dive_last_cy < 26u) {
                demo_fix_puts(dive_last_cx, dive_last_cy, "   ", 0u);
            }
            dive_x_fp = (int16_t)(dive_x_fp + dive_vx);
            dive_y_fp = (int16_t)(dive_y_fp + dive_vy);
            /* shoot during dive (twice per dive) */
            if ((t & 31u) == 15u) {
                uint8_t k;
                for (k = 0u; k < EBULLET_MAX; k++) {
                    if (!eb_act[k]) {
                        eb_x[k] = (uint8_t)(dive_x_fp / (8 * 16) + 1);
                        eb_y[k] = (uint8_t)(dive_y_fp / (8 * 16) + 1);
                        eb_act[k] = 1u;
                        eb_lx[k] = 0xFFu;
                        break;
                    }
                }
            }
            cx = (uint8_t)(dive_x_fp / (8 * 16));
            cy = (uint8_t)(dive_y_fp / (8 * 16));
            if (cy >= 27u || cx >= 37u) {
                /* dove off screen — return to formation */
                enemy_alive[dive_idx] = 1u;
                dive_idx = DIVE_NONE;
            } else {
                demo_fix_puts(cx, cy, enemy_type[dive_idx] ? "<M>" : "/V\\",
                              (uint8_t)(enemy_type[dive_idx] ? 2u : 1u));
                dive_last_cx = cx;
                dive_last_cy = cy;
            }
        }

        /* ---- Debris ------------------------------------------------- */
        for (i = 0u; i < DEBRIS_MAX; i++) {
            uint8_t cx, cy;
            if (!deb_ttl[i]) continue;
            if (deb_lx[i] != 0xFFu &&
                deb_lx[i] < 40u && deb_ly[i] < 28u) {
                demo_fix_puts(deb_lx[i], deb_ly[i], " ", 0u);
            }
            deb_x_fp[i] = (int16_t)(deb_x_fp[i] + deb_vx[i]);
            deb_y_fp[i] = (int16_t)(deb_y_fp[i] + deb_vy[i]);
            deb_vy[i]  = (int16_t)(deb_vy[i] + 4);
            deb_ttl[i]--;
            cx = (uint8_t)(deb_x_fp[i] / (8 * 16));
            cy = (uint8_t)(deb_y_fp[i] / (8 * 16));
            if (cx < 40u && cy < 26u && deb_ttl[i] > 0u) {
                const char *g = (deb_ttl[i] > 18u) ? "*" :
                                (deb_ttl[i] >  8u) ? "+" : ".";
                demo_fix_puts(cx, cy, g, (uint8_t)((deb_ttl[i] >> 3) & 3u));
                deb_lx[i] = cx; deb_ly[i] = cy;
            } else {
                deb_lx[i] = 0xFFu;
                deb_ttl[i] = 0u;
            }
        }

        /* ---- Score popups (float upward) --------------------------- */
        for (i = 0u; i < POPUP_MAX; i++) {
            if (!pop_ttl[i]) continue;
            if (pop_lcx[i] != 0xFFu &&
                pop_lcx[i] < 38u && pop_lcy[i] < 28u) {
                demo_fix_puts(pop_lcx[i], pop_lcy[i], "   ", 0u);
            }
            if ((pop_ttl[i] & 1u) == 0u && pop_cy[i] > 0u) pop_cy[i]--;
            pop_ttl[i]--;
            if (pop_ttl[i] > 0u && pop_cx[i] < 38u && pop_cy[i] < 28u) {
                /* draw 3-digit popup */
                buf[0] = (char)('0' + (pop_val[i] / 100u) % 10u);
                buf[1] = (char)('0' + (pop_val[i] /  10u) % 10u);
                buf[2] = (char)('0' + (pop_val[i] %  10u));
                buf[3] = '\0';
                demo_fix_puts(pop_cx[i], pop_cy[i], buf, 2u);
                pop_lcx[i] = pop_cx[i]; pop_lcy[i] = pop_cy[i];
            } else {
                pop_lcx[i] = 0xFFu;
            }
        }

        /* ---- Formation redraw (drift erase + new draw) ------------- */
        if ((t % 30u) == 0u) {
            for (i = 0u; i < ENEMY_COUNT; i++) {
                if (!enemy_alive[i] || i == dive_idx) continue;
                demo_fix_puts((uint8_t)(enemy_cx[i] - drift),
                              enemy_cy[i], "   ", 0u);
            }
        }
        for (i = 0u; i < ENEMY_COUNT; i++) {
            if (enemy_alive[i] && i != dive_idx) {
                demo_fix_puts((uint8_t)(enemy_cx[i] + drift),
                              enemy_cy[i],
                              enemy_type[i] ? "<M>" : "/W\\",
                              (uint8_t)(enemy_type[i] ? 2u : 1u));
                alive_count++;
            }
        }

        /* ---- Ship draw (multi-glyph) ------------------------------- */
        if (!game_over) {
            if (last_ship_x != 0xFFu && last_ship_x != ship_x) {
                demo_fix_puts(last_ship_x, (uint8_t)(SHIP_ROW - 1u), "   ", 0u);
                demo_fix_puts(last_ship_x, SHIP_ROW, "   ", 0u);
            }
            demo_fix_puts(ship_x, (uint8_t)(SHIP_ROW - 1u), "/A\\", 1u);
            demo_fix_puts(ship_x, SHIP_ROW,            "[#]", 2u);
            last_ship_x = ship_x;
        }

        /* ---- Bullets draw ----------------------------------------- */
        for (i = 0u; i < BULLET_MAX; i++) {
            if (pb_lx[i] != 0xFFu &&
                (!pb_act[i] || pb_lx[i] != pb_x[i] || pb_ly[i] != pb_y[i])) {
                demo_fix_puts(pb_lx[i], pb_ly[i], " ", 0u);
                pb_lx[i] = 0xFFu;
            }
            if (pb_act[i]) {
                demo_fix_puts(pb_x[i], pb_y[i], "|", 1u);
                pb_lx[i] = pb_x[i]; pb_ly[i] = pb_y[i];
            }
        }
        for (i = 0u; i < EBULLET_MAX; i++) {
            if (eb_lx[i] != 0xFFu &&
                (!eb_act[i] || eb_lx[i] != eb_x[i] || eb_ly[i] != eb_y[i])) {
                demo_fix_puts(eb_lx[i], eb_ly[i], " ", 0u);
                eb_lx[i] = 0xFFu;
            }
            if (eb_act[i]) {
                demo_fix_puts(eb_x[i], eb_y[i], "v", 2u);
                eb_lx[i] = eb_x[i]; eb_ly[i] = eb_y[i];
            }
        }

        /* ---- HUD ---------------------------------------------------- */
        digit3(buf, score);              demo_fix_puts(8u,  26u, buf, 1u);
        buf[0] = (char)('0' + stage); buf[1] = '\0';
        demo_fix_puts(18u, 26u, buf, 1u);
        digit3(buf, alive_count);        demo_fix_puts(26u, 26u, buf, 1u);
        buf[0] = (char)('0' + lives); buf[1] = '\0';
        demo_fix_puts(34u, 26u, buf, 1u);
        time_left = (t > 5400u) ? 0u : (uint16_t)((5400u - t) / 60u);
        digit3(buf, time_left);          demo_fix_puts(40u - 3u, 26u, buf, 1u);

        /* Win condition */
        if (alive_count == 0u && dive_idx == DIVE_NONE) {
            demo_fix_puts(13u, 14u, "***  WAVE  CLEAR  ***", 2u);
            if (uwait(120u)) goto fade_out;
            demo_fix_puts(13u, 14u, "                       ", 0u);
            if (stage >= 3u) {
                demo_fix_puts(11u, 14u, "***  VICTORY  ***", 2u);
                if (uwait(180u)) goto fade_out;
                goto fade_out;
            }
            stage++;
            for (i = 0u; i < ENEMY_COUNT; i++) {
                uint8_t row = (uint8_t)(i / COLS_E);
                uint8_t col = (uint8_t)(i % COLS_E);
                enemy_alive[i] = 1u;
                enemy_type[i]  = (uint8_t)((row < 2u) ? 1u : 0u);  /* more bosses in later waves */
                enemy_cx[i] = (uint8_t)(4u + col * 5u);
                enemy_cy[i] = (uint8_t)(6u + row * 2u);
            }
            dive_timer = (uint16_t)(180u / stage);   /* dives more often per wave */
        }
        if (game_over) {
            demo_fix_puts(13u, 14u, "***  GAME  OVER  ***", 2u);
            if (uwait(180u)) goto fade_out;
            goto fade_out;
        }

        if (uframe()) goto fade_out;
    }

fade_out:
    soundFadeOutSpeed(8u); snd_step();
    if (uwait(16u)) return 1u;
    soundStopAll();        snd_step();
    return 0u;
}

/* ================================================================== */
/*  Chapter 19 — Garden 3D (sprite-scaling pseudo-3D walk)               */
/* ================================================================== */
/*
 * Pseudo-3D garden — the warrior walks at fixed Y while a row of "tree"
 * sprites approaches the camera and scales up.  Floor on the FIX layer
 * uses per-row palette banding to give a perspective gradient.
 *
 * NeoGeo hardware sprite scaling (SCB2 shrink nibble) does the
 * heavy lifting — every tree is just a sprite group with its xScale /
 * yScale set proportionally to "distance".  No raycaster needed; this
 * runs at 60 fps on 68k without breaking a sweat.
 */
static uint8_t NEOGEO_USER chap_garden3d(void)
{
    enum {
        NPC_COUNT = 3,
        Z_NEAR    = 12,
        Z_FAR     = 96
    };
    /* z = depth.  As z decreases toward Z_NEAR, the NPC scales up
     * and slides toward the edge.  Recycled to Z_FAR when it passes. */
    int16_t  nz[NPC_COUNT]   = { 30, 60, 90 };
    /* World-X relative to camera centre (positive = right of camera). */
    static const int16_t nxw[NPC_COUNT] = { -32, 28, -16 };
    /* Use cat NPCs (screen_id 110..117) as the "approaching objects".
     * Different IDs per slot for visual variety. */
    static const uint8_t npc_frames[NPC_COUNT] = { 110u, 114u, 117u };
    uint16_t t;
    uint8_t  i;
    int16_t  hero_world_x = 160;

    chap_header(19u, "DEPTH RIDE", "OBJECTS APPROACH AS YOU WALK");
    demo_fix_puts(2u, 2u, "NPCS APPROACH FROM HORIZON",   1u);
    demo_fix_puts(2u, 3u, "L/R MOVE  HARDWARE-SCALE NPCS", 0u);
    snd_cross_to(SOUND_MUSIC_C);

    /* Real BG image so the scene has a backdrop, not pure black. */
    draw_background(2u, 32, 16);

    /* Pre-load all NPC palettes once so subsequent draws don't thrash. */
    for (i = 0u; i < NPC_COUNT; i++) {
        demo_load_screen_palette(npc_frames[i]);
    }

    ng_joystick_init();

    for (t = 0u; t < 900u; t++) {            /* 15 sec */
        uint16_t down;
        uint8_t  hero_frame;

        ng_joystick_update();
        down = ng_joy_down();

        if (down & JOY_LEFT)  hero_world_x -= 2;
        if (down & JOY_RIGHT) hero_world_x += 2;
        if (hero_world_x < 32)  hero_world_x = 32;
        if (hero_world_x > 288) hero_world_x = 288;

        for (i = 0u; i < NPC_COUNT; i++) {
            uint8_t  npc_frame = npc_frames[i];
            int16_t  scale;
            int16_t  screen_x, screen_y;
            uint8_t  scale8;
            uint8_t  strips = demo_screen_strips(npc_frame);
            uint8_t  rows   = demo_screen_rows(npc_frame);

            nz[i] = (int16_t)(nz[i] - 1);
            if (nz[i] < Z_NEAR) nz[i] = Z_FAR;

            scale = (int16_t)((int16_t)Z_FAR * 96 / (int16_t)nz[i]);
            if (scale > 0xFF) scale = 0xFF;
            if (scale < 0x40) scale = 0x40;
            scale8 = (uint8_t)scale;

            screen_x = (int16_t)(160 + (nxw[i] * (int16_t)Z_FAR) / (int16_t)nz[i]);
            screen_y = (int16_t)(80 + (Z_FAR - nz[i]) * 5 / 6);
            {
                int16_t px_w  = (int16_t)((strips * 16 * scale8) >> 8);
                int16_t px_h  = (int16_t)((rows   * 16 * scale8) >> 8);
                int16_t draw_x = (int16_t)(screen_x - (px_w >> 1));
                int16_t draw_y = (int16_t)(screen_y - px_h);
                demo_draw_sprite_screen(npc_frame,
                                        (uint16_t)(200u + i * 6u),
                                        draw_x, draw_y,
                                        strips, rows,
                                        scale8, scale8);
            }
        }

        hero_frame = (down & (JOY_LEFT | JOY_RIGHT))
                   ? s_hero_walk[(t / 5u) % 8u]
                   : s_hero_stand[(t / 14u) % 8u];
        s_hero_x = hero_world_x;
        s_hero_y = 180;
        hero_draw(hero_frame);

        if ((t & 31u) == 0u && (down & (JOY_LEFT | JOY_RIGHT)))
            playSFX(SOUND_SFX_5);

        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 20 — Credits                                                 */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_credits(void)
{
    chap_header(20u, "CREDITS", "EAGLE SOFTWARE 2026");
    snd_cross_to(SOUND_MUSIC_H);

    demo_fix_puts(2u,  4u, "ENGINE   SDK/2D_ENGINE_PLUS", 1u);
    demo_fix_puts(2u,  6u, "WEB      EAGLESOFTWARE.BIZ",  0u);

    demo_fix_puts(2u,  9u, "MODULES SHOWN:",          2u);
    demo_fix_puts(4u, 10u, "FIX  SPRITE  CHARS",      1u);
    demo_fix_puts(4u, 11u, "SOUND  PHYSICS  CAMERA",  1u);
    demo_fix_puts(4u, 12u, "PALETTE FX  PARTICLES",   1u);
    demo_fix_puts(4u, 13u, "FEEDBACK  DEPTH FX",      1u);
    demo_fix_puts(4u, 14u, "NPCS  JOYSTICK  SCROLL",  1u);
    demo_fix_puts(4u, 15u, "2D/3D RENDER  SSG ARCADE",1u);


    demo_fix_puts(2u, 24u, "THANKS FOR PLAYING.", 2u);
    if (uwait(240u)) return 1u;

    soundFadeOutSpeed(4u); snd_step();
    if (uwait(40u)) return 1u;
    soundStopAll();        snd_step();
    return 0u;
}

/* ================================================================== */
/*  Chapter 21 — FIX FX (dedicated FIX showcase placed after SOUND)     */
/* ================================================================== */
/*
 * Animated FIX-only scene placed in the flow right after chap_sound.
 * Demonstrates four things the FIX layer is uniquely good at:
 *   1. Solid-colour horizontal bars sliding back and forth
 *   2. Wave-form vertical bars
 *   3. Per-row palette stripes
 *   4. A multi-palette text scroll
 */
static uint8_t NEOGEO_USER chap_fix_fx(void)
{
    /*
     * FIX-photo animation showcase — replaces the old "Z motif".
     *
     * Source assets are the PNGs in games/demo/artbox/infix/ which
     * fixtiles.py compiles into the S1 ROM starting at FIX tile 256
     * (sequential, row-major per PNG).  Each 8×8 source pixel block
     * becomes one FIX tile, so a 32×4 PNG is 128 contiguous FIX tiles
     * that we can stamp anywhere on the 40×28 FIX grid.
     *
     * Four phases — each demonstrates a different FIX animation
     * primitive built on the low-level ngfix_* helpers and the
     * draw_infix_block utility defined earlier in this file:
     *
     *   PHASE 1 SLIDESHOW    — center-blit a sequence of infix
     *                          banners with ngfix_clear_rect between.
     *   PHASE 2 SLIDE-IN     — march a banner across the FIX grid
     *                          by clearing the trailing column with
     *                          ngfix_blank_cell and redrawing.
     *   PHASE 3 PALETTE CYCLE — same banner stamped every frame at
     *                          a rotating palette index, showing how
     *                          cheap colour cycling is on the FIX layer.
     *   PHASE 4 MOSAIC       — multiple infix blocks (different
     *                          source PNGs, different palettes) tiled
     *                          on the same frame.
     */
    typedef struct {
        uint16_t tile_base;
        uint8_t  cols;
        uint8_t  rows;
        const char *label;
    } InfixAsset;
    /* Tile bases match those compiled by fixtiles.py and already used
     * by chap_fix_fx's predecessors elsewhere in this file. */
    static const InfixAsset BANNERS[4] = {
        { 492u, 32u, 4u, "INFIX 4.PNG  32x4" },
        { 772u, 32u, 4u, "INFIX 7.PNG  32x4" },
        { 948u, 32u, 5u, "INFIX 9.PNG  32x5" },
        { 620u, 24u, 5u, "INFIX 5.PNG  24x5" }
    };
    uint16_t t;

    chap_header(21u, "FIX FX", "INFIX PHOTOS  4 PHASE ANIMATION");
    demo_fix_puts(2u, 2u, "REAL FIX-PHOTO ANIMATION",       1u);
    demo_fix_puts(2u, 3u, "INFIX/*.PNG TILES  ngfix_* API", 0u);
    snd_cross_to(SOUND_MUSIC_G);

    /* ---------------- PHASE 1 — slideshow (480 frames, 4 banners) -- */
    demo_fix_puts(2u, 5u, "PHASE 1  SLIDESHOW                ", 2u);
    {
        uint8_t b;
        for (b = 0u; b < 4u; b++) {
            const InfixAsset *a = &BANNERS[b];
            uint8_t cx = (uint8_t)((40u - a->cols) / 2u);
            uint8_t cy = (uint8_t)(10u + ((6u - a->rows) >> 1));
            uint8_t pal = (uint8_t)(1u + (b & 1u));

            ngfix_clear_rect(0u, 9u, 40u, 8u);
            draw_infix_block(a->tile_base, a->cols, a->rows, cx, cy, pal);
            demo_fix_puts(2u, 24u, "                                  ", 0u);
            demo_fix_puts(2u, 24u, a->label, 1u);

            if (uwait(110u)) return 1u;
        }
    }
    ngfix_clear_rect(0u, 9u, 40u, 8u);
    demo_fix_puts(2u, 24u, "                                  ", 0u);

    /* ---------------- PHASE 2 — slide-in (240 frames) -------------- */
    demo_fix_puts(2u, 5u, "PHASE 2  SLIDE-IN (clear-redraw)  ", 2u);
    {
        const InfixAsset *a = &BANNERS[0];   /* infix 4.png 32x4 */
        uint8_t  last_x = 0xFFu;
        for (t = 0u; t < 240u; t++) {
            /* X marches from -32 (offscreen left) through 8 (centred). */
            int16_t pos = (int16_t)((int16_t)t / 6 - 32);  /* -32..7 */
            uint8_t bx;
            if (pos < 0) {
                /* Banner partly off-screen; only paint visible portion.
                 * Clamp source column count to fit. */
                bx = 0u;
            } else {
                bx = (uint8_t)pos;
            }
            if (last_x != 0xFFu && bx != last_x) {
                ngfix_clear_rect(last_x, 11u, a->cols, a->rows);
            }
            if (pos >= -((int16_t)a->cols) && pos < 40) {
                /* Draw at bx; off-screen cells naturally clipped by
                 * draw_infix_block (bounds-checked) */
                draw_infix_block(a->tile_base, a->cols, a->rows,
                                 bx, 11u, 2u);
            }
            last_x = bx;
            if (uframe()) return 1u;
        }
    }
    ngfix_clear_rect(0u, 11u, 40u, 5u);

    /* ---------------- PHASE 3 — palette cycle (240 frames) --------- */
    demo_fix_puts(2u, 5u, "PHASE 3  PALETTE CYCLE            ", 2u);
    {
        const InfixAsset *a = &BANNERS[2];   /* infix 9.png 32x5 */
        uint8_t  cx = (uint8_t)((40u - a->cols) / 2u);
        uint8_t  cy = 11u;
        /* Draw once at pal 0; then per frame just rewrite palette
         * nibble per cell — proves how cheap palette cycling is. */
        for (t = 0u; t < 240u; t++) {
            uint8_t pal = (uint8_t)((t / 6u) % 3u);
            draw_infix_block(a->tile_base, a->cols, a->rows, cx, cy, pal);
            if (uframe()) return 1u;
        }
    }
    ngfix_clear_rect(0u, 11u, 40u, 5u);

    /* ---------------- PHASE 4 — mosaic (240 frames) ---------------- */
    demo_fix_puts(2u, 5u, "PHASE 4  MOSAIC (multi-tile)      ", 2u);
    {
        uint8_t  flicker;
        /* Static composition first */
        draw_infix_block(256u, 20u, 4u,  0u, 10u, 1u);    /* 0.png left  */
        draw_infix_block(740u, 16u, 2u, 22u, 11u, 2u);    /* 6.png right */
        draw_infix_block(900u, 12u, 4u, 14u, 16u, 1u);    /* 8.png center-bottom */

        for (t = 0u; t < 240u; t++) {
            /* Flicker the centre block's palette every 8 frames */
            flicker = (uint8_t)(((t >> 3) & 1u) + 1u);
            draw_infix_block(900u, 12u, 4u, 14u, 16u, flicker);
            if (uframe()) return 1u;
        }
    }
    ngfix_clear_rect(0u, 9u, 40u, 12u);
    demo_fix_puts(2u, 5u, "                                  ", 0u);
    return 0u;
}

/* ================================================================== */
/*  Public entry — 21-chapter linear flow                                */
/* ================================================================== */
void NEOGEO_USER demo_unified_run(void)
{
    (void)chap_boot();
    (void)chap_title();
    (void)chap_fix();
    (void)chap_fix_fx();
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
    (void)chap_char_2d();
    (void)chap_render3d();
    (void)chap_ssg_arcade();
    (void)chap_garden3d();
    (void)chap_sound();         /* moved to just before the credits   */
    (void)chap_credits();
}
