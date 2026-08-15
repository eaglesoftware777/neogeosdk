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
 * ng_clear_screen_full() — that clears all sprite SCB position/scale tables,
 * resets the character pool and physics, clears FIX, and sets the backdrop
 * black. No state leaks across chapters.
 */

#include "demo_unified.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/ng_fix/ng_fix.h"
#include "sdk/2d_engine/ng_engine.h"
#include "sprite_meta.h"
#include "infix_palettes.h"
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
void NEOGEO_USER soundApplyMix(uint8_t adpcma, uint8_t adpcmb,
                               uint8_t ssg, uint8_t fm);
void NEOGEO_USER soundSetADPCMBPan(uint8_t pan);
void NEOGEO_USER soundFMSetLFO(uint8_t value);
void NEOGEO_USER soundFMSetTempo(uint8_t value);
void NEOGEO_USER soundFadeInSpeed(uint8_t speed);
void NEOGEO_USER soundFadeOutSpeed(uint8_t speed);
void NEOGEO_USER soundCancelFade(void);
void NEOGEO_USER soundStopMusic(void);
void NEOGEO_USER playMusic(uint8_t n);
void NEOGEO_USER playFMTrack(uint8_t n);
void NEOGEO_USER playSSGTrack(uint8_t n);
void NEOGEO_USER soundSetSSGPreset(uint8_t preset);
void NEOGEO_USER playSSGVoiceGetReady(void);
void NEOGEO_USER playSSGVoiceLetsGo(void);
void NEOGEO_USER playFMSpeechRobot(void);
void NEOGEO_USER playSFXB(uint8_t n);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playVoiceCue(uint8_t n);
void NEOGEO_USER ng_clear_screen_full(void);

#define U_FLOOR_Y       192
#define U_CENTRE_X      160

/* Lowered-hero baselines used by the palette FX, particles and
 * feedback/hitstop scenes.  FX_HERO_BOTTOM_Y matches the engine
 * floor at U_FLOOR_Y; FX_HERO_LIFT_Y is 4 px above it, which is
 * the actual draw target for those scenes so the eagle/hero
 * doesn't sit on the BG's earth strip and avoids the stale
 * sprite-window strip that was sometimes left attached when the
 * pose frame was uploaded at the old higher Y of 208/210/202. */
#define FX_HERO_BOTTOM_Y 192
#define FX_HERO_LIFT_Y   172   /* was 188 - character/particle effects read too low */

/*
 * Sprite slot plan.
 *
 * Lower sprite slots draw first on Neo Geo, so later slots draw in front.
 * Backgrounds use the low slot window; direct hero/enemy windows stay above
 * them so they are never hidden by a full-screen BG.
 */
#define DEMO_BG_BACK_SLOT  NG_SPR_BG0_FIRST
#define HERO_SLOT_FIRST    64u
#define ENEMY_SLOT_FIRST   80u

#define U_BG_FOREST        2u

#define U_SHOOTER_ENEMY          122u
#define U_SHOOTER_ENEMY_BULLET   123u
#define U_SHOOTER_EXPLOSION      124u
#define U_SHOOTER_PLAYER_BULLET  125u
#define U_SHOOTER_SHIP           126u
#define U_SHOOTER_SHIP_ALT       127u

/* CC0 (public domain), Kenney "Alien UFO pack" via OpenGameArt.org -
 * appended to the npcs asset table after every existing entry so no
 * index shifted (verified: 127 existing entries unchanged, 4 new ones
 * landed at 128-131). One color per formation row instead of reusing
 * the same plain enemy sprite for all 24 enemies. */
#define U_UFO_BLUE               128u
#define U_UFO_GREEN              129u
#define U_UFO_PINK               130u
#define U_UFO_YELLOW             131u

/*
 * Char rendering scale presets.  The NeoGeo sprite chip can only
 * shrink, not stretch, so 0xFFu is the hardware ceiling; every
 * preset below (including U_SCALE_FULL) is scaled down from the
 * chip's native size by U_SCALE_GLOBAL_RATIO so the whole cast reads
 * smaller game-wide while keeping the same relative sizing between
 * presets.  Re-tune by changing the ratio, not the individual bytes.
 * (First pass used 0.82 - too subtle to read as "shrunk" at all;
 * 0.6 is a real, clearly-visible cut instead of a nudge.)
 * U_SCALE_GLOBAL_RATIO = 0.6
 */
#define U_SCALE_30         0x33u   /* ~20% (was ~33%)  */
#define U_SCALE_45         0x4Du   /* ~30% (was ~50%)  */
#define U_SCALE_55         0x5Cu   /* ~36% (was ~60%)  */
#define U_SCALE_57         0x60u   /* ~38% (was ~63%)  */
#define U_SCALE_60         0x65u   /* ~40% (was ~66%)  */
#define U_SCALE_70         0x76u   /* ~46% (was ~77%)  */
#define U_SCALE_FULL       0x99u   /* ~60% (was 100%, hardware ceiling) */

static const uint8_t s_fx_effect_frames[4] = {
    89u, 90u, 91u, 92u
};

static const uint8_t s_fx_char_frames[5] = {
    49u, 50u, 51u, 52u, 53u
};

static const uint8_t s_flight_frames[3] = {
    78u, 79u, 80u
};

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

/* Bottom-row caption bar (row 27) — set once per chapter by chap_header(),
 * redrawn every frame by uframe() with a live elapsed-time counter so
 * every scene carries a concise "what is this" label plus proof it's
 * still ticking, not stuck. */
#define CHAPTER_CAPTION_MAX 24u
static char     s_chapter_caption[CHAPTER_CAPTION_MAX];
static uint16_t s_chapter_elapsed = 0u;

/* Displayed chapter number: counts chap_header() calls in actual viewing
 * order (1, 2, 3...), independent of each call site's internal id
 * argument (whose numbering follows source layout, not playback order,
 * and is left alone). */
static uint8_t  s_chapter_view_index = 0u;

static void NEOGEO_USER hero_scale(uint8_t scale);

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
    /* Bottom caption bar — concise scene label + a live elapsed-frame
     * counter so every scene shows what it is and proves it's still
     * running, not stuck.  Fixed-width so a shorter title never leaves
     * a previous frame's longer text trailing behind it. */
    {
        char line[36];
        char tbuf[4];
        uint8_t p = 0u;
        uint8_t k;

        if (s_chapter_elapsed < 0xFFFFu) s_chapter_elapsed++;

        for (k = 0u; s_chapter_caption[k] != '\0' && p < 22u; k++) {
            line[p++] = s_chapter_caption[k];
        }
        while (p < 22u) line[p++] = ' ';
        line[p++] = 'T'; line[p++] = ':';
        digit3(tbuf, (uint16_t)(s_chapter_elapsed % 1000u));
        line[p++] = tbuf[0]; line[p++] = tbuf[1]; line[p++] = tbuf[2];
        line[p++] = ' '; line[p++] = ' ';
        line[p++] = 'A'; line[p++] = ':'; line[p++] = 'N';
        line[p++] = 'E'; line[p++] = 'X'; line[p++] = 'T';
        line[p] = '\0';
        demo_fix_puts(2u, 27u, line, 0u);
    }

    perf_hud_draw();
    waitVbl();                          /* arrive at vblank start */
    /* Drain queued sprite-group uploads while the screen is blanked.
     * Chapters call demo_draw_sprite_screen() during active video to
     * queue work; this is where it actually reaches SCB.  Without
     * this the sprite chip would read mid-write SCB and tear the
     * char in half. */
    demo_flush_sprite_queue();
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

static uint8_t NEOGEO_USER fm_lfo_hold(uint8_t value, uint16_t frames)
{
    uint16_t t;

    for (t = 0u; t < frames; t++) {
        if ((t & 15u) == 0u) {
            soundFMSetLFO(value);
        }
        if (uframe()) return 1u;
    }

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
     * Pure black keeps transparent padding and freshly-cleared FIX cells
     * from reading as large pale rectangles during chapter transitions.
     * clearSprs() is intentionally omitted here: ng_clear_screen_full()
     * already does a full ng_sprite_hide_all() kill (SCB3=0x8000, full
     * SCB1 wipe with blank-tile 0xFFFF).  A second clearSprs() would
     * overwrite that safe blank-tile fill with tile-0, which may carry art.
     */
    setBACKDROP(BLACK);
    ng_level_set_scroll(0, 0);
    ng_particles_init();
    ng_feedback_init();
    ng_palette_fx_init();

    /* per-chapter flush flags — chapters that need chars/particles set them */
    s_draw_chars     = 0u;
    s_draw_particles = 0u;
    hero_scale(U_SCALE_FULL);

    /* Displayed number is the chapter's position in actual viewing order
     * (1, 2, 3...), not the `n` id argument — call sites number by source
     * layout, which doesn't match playback order (chapters get reordered
     * in demo_unified_run() independent of their id).  `n` itself is left
     * untouched at every call site; only the on-screen digits changed. */
    if (s_chapter_view_index < 99u) s_chapter_view_index++;

    tag[0] = 'C';
    tag[1] = 'H';
    tag[2] = '.';
    tag[3] = (char)('0' + (s_chapter_view_index / 10u));
    tag[4] = (char)('0' + (s_chapter_view_index % 10u));
    tag[5] = '\0';

    demo_fix_puts(2u,  0u, tag,   0u);
    demo_fix_puts(8u,  0u, title, 2u);
    if (subtitle) demo_fix_puts(2u, 1u, subtitle, 1u);

    /* Separator line above the caption bar, drawn once per chapter. */
    {
        char rule[37];
        uint8_t p;
        for (p = 0u; p < 36u; p++) rule[p] = '-';
        rule[36] = '\0';
        demo_fix_puts(2u, 26u, rule, 0u);
    }

    /* Bottom caption bar: "CH## TITLE" — concise, rebuilt once here,
     * redrawn with a live timer every frame by uframe(). */
    s_chapter_elapsed = 0u;
    {
        uint8_t p = 0u;
        uint8_t k;
        s_chapter_caption[p++] = tag[3];
        s_chapter_caption[p++] = tag[4];
        s_chapter_caption[p++] = ' ';
        for (k = 0u; title[k] != '\0' && p < CHAPTER_CAPTION_MAX - 1u; k++) {
            s_chapter_caption[p++] = title[k];
        }
        s_chapter_caption[p] = '\0';
    }

    {
        char idx[3];
        idx[0] = tag[3];
        idx[1] = tag[4];
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

    /* Content-bottom-center anchoring.  DEMO_SCREEN_TILE points at the
     * content's top-left tile, so the sprite group's origin (g->x,
     * g->y) corresponds to that canvas pixel.  The artwork lives at
     * +(x_pad, y_pad) within the rendered strip area and extends for
     * (content_width, content_height) pixels.  Anchoring on the centre
     * of the BOTTOM edge keeps the char's feet planted at (c->x, c->y)
     * across every animation frame, even when strips/rows/y_pad swing
     * by 50% during an attack — only the top of the sprite extends UP
     * for taller poses, never the bottom.
     *
     * For hero animation frames the artbox already aligns
     *   tile_row_start*16 + y_pad + content_height = canvas_bottom_y
     * for every frame, so this anchor is rock-stable. */
    anchor_x = (uint16_t)((uint16_t)demo_screen_x_pad(frame)
                          + (demo_screen_content_width(frame) >> 1));
    anchor_y = (uint16_t)((uint16_t)demo_screen_y_pad(frame)
                          + demo_screen_content_height(frame));

    if (anchor_x == 0u) anchor_x = (uint16_t)(((uint16_t)strips * 16u) >> 1);
    if (anchor_y == 0u) anchor_y = (uint16_t)((uint16_t)rows * 16u);

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

static void NEOGEO_USER clear_fix_rect_force(uint8_t x,
                                             uint8_t y,
                                             uint8_t w,
                                             uint8_t h)
{
    uint8_t row;
    uint8_t col;

    for (row = 0u; row < h; row++) {
        uint8_t py = (uint8_t)(y + row);
        if (py >= 28u) break;

        for (col = 0u; col < w; col++) {
            uint8_t px = (uint8_t)(x + col);
            if (px >= 40u) break;
            ngfix_write_tile(px, py, 0x00FFu, 0u);
        }
    }
}

static int16_t NEOGEO_USER asset_scaled_px(uint8_t cells, uint8_t scale)
{
    uint16_t px = (uint16_t)cells * 16u;
    if (scale >= 0xFFu) return (int16_t)px;
    return (int16_t)(((uint32_t)px * (uint32_t)scale + 127u) >> 8);
}

static void NEOGEO_USER draw_asset_bottom_center(uint8_t frame,
                                                 uint16_t first_sprite,
                                                 int16_t cx,
                                                 int16_t bottom_y,
                                                 uint8_t scale_x,
                                                 uint8_t scale_y)
{
    uint8_t strips = demo_screen_strips(frame);
    uint8_t rows   = demo_screen_rows(frame);
    int16_t draw_x;
    int16_t draw_y;

    /* Anchor on the artwork's painted bottom-centre (x_pad +
     * content_width/2, y_pad + content_height) instead of the raw
     * tile-grid box.  The grid size jumps frame-to-frame as the
     * animation cycles through poses with different used tile
     * footprints, which made the eagle/hero drift up and down
     * between frames and exposed the sprite-window cache to stale
     * strips outside the new frame's footprint. */
    demo_anchor_bottom_center(frame, scale_x, scale_y,
                              cx, bottom_y,
                              &draw_x, &draw_y);

    demo_draw_sprite_screen(frame, first_sprite,
                            draw_x, draw_y,
                            strips, rows, scale_x, scale_y);
}

static void NEOGEO_USER draw_asset_bottom_center_flip(uint8_t frame,
                                                      uint16_t first_sprite,
                                                      int16_t cx,
                                                      int16_t bottom_y,
                                                      uint8_t scale_x,
                                                      uint8_t scale_y,
                                                      uint8_t flip)
{
    uint8_t strips = demo_screen_strips(frame);
    uint8_t rows = demo_screen_rows(frame);
    int16_t w = asset_scaled_px(strips, scale_x);
    int16_t h = asset_scaled_px(rows, scale_y);

    demo_draw_sprite_screen_flip(frame, first_sprite,
                                 (int16_t)(cx - (w >> 1) - demo_screen_x_offset(frame)),
                                 (int16_t)(bottom_y - h - demo_screen_y_offset(frame)),
                                 strips, rows, scale_x, scale_y, flip);
}

static void NEOGEO_USER draw_background(uint8_t frame, int16_t x, int16_t y)
{
    /* Background slot range stays behind characters and effects. */
    demo_draw_sprite_screen(frame, DEMO_BG_BACK_SLOT, x, y,
                            demo_screen_strips(frame),
                            demo_screen_rows(frame),
                            0xFFu, 0xFFu);
}

static void NEOGEO_USER draw_scrolling_background(uint8_t frame, int16_t x, int16_t y)
{
    while (x > 32) x = (int16_t)(x - 256);
    while (x < -192) x = (int16_t)(x + 256);
    demo_draw_sprite_screen(frame, DEMO_BG_BACK_SLOT, x, y,
                            demo_screen_strips(frame),
                            demo_screen_rows(frame),
                            0xFFu, 0xFFu);
    demo_draw_sprite_screen(frame, NG_SPR_BG1_FIRST,
                            (int16_t)(x + 256), y,
                            demo_screen_strips(frame),
                            demo_screen_rows(frame),
                            0xFFu, 0xFFu);
}

static void NEOGEO_USER draw_vertical_background(uint8_t frame, int16_t x, int16_t y)
{
    if (y > 0) y = 0;
    if (y < -32) y = -32;
    demo_draw_sprite_screen(frame, DEMO_BG_BACK_SLOT, x, y,
                            demo_screen_strips(frame),
                            demo_screen_rows(frame),
                            0xFFu, 0xFFu);
    demo_draw_sprite_screen(frame, NG_SPR_BG1_FIRST,
                            (int16_t)(x + 256), y,
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
    demo_fix_puts(2u,  4u, "2026  EAGLESOFTWARE.BIZ", 0u);

    demo_fix_puts(2u,  7u, "SDK SHOWCASE", 2u);
    demo_fix_puts(2u,  9u, "20 CHAPTERS  FULL SDK DEMO", 1u);

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
	
	
    for (t = 0u; t < 450u; t++) {
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
    typedef struct {
        uint16_t tile_base;
        uint8_t cols;
        uint8_t rows;
    } FixImage;
    static const FixImage infix[10] = {
        { 256u, 20u, 4u },
        { 336u, 40u, 2u },
        { 416u, 20u, 3u },
        { 476u,  8u, 2u },
        { 492u, 32u, 4u },
        { 620u, 24u, 5u },
        { 740u, 16u, 2u },
        { 772u, 32u, 4u },
        { 900u, 12u, 4u },
        { 948u, 32u, 5u }
    };
    uint16_t t;
    char buf[8];

    chap_header(2u, "FIX LAYER", "TEXT  PALETTES");
    /*
     * Black backdrop keeps cleared FIX cells fully invisible.  Using a
     * bright backdrop here made transparent areas read as yellow/white
     * blocks when switching between FIX pages.
     */
    setBACKDROP(BLACK);
    /*
     * Re-write the header tag and status row after the backdrop change.
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
    demo_fix_puts(2u,  6u, "PALETTE 0  STANDARD",   0u);
    demo_fix_puts(2u,  7u, "PALETTE 1  ACCENT",     1u);
    demo_fix_puts(2u,  8u, "PALETTE 2  GREEN",      2u);



    for (t = 0u; t < 180u; t++) {
        digit3(buf, t);
        demo_fix_puts(2u, 13u, buf, (uint8_t)((t / 30u) % 3u));
        if (uframe()) return 1u;
    }

    /* Clear only working rows; avoid reprinting blank strings over art. */
    clear_fix_rect_force(0u, 5u, 40u, 22u);

    demo_fix_puts(2u, 2u, "INFIX 0..3  DEFAULT COLOURS", 1u);
    draw_infix_block(infix[0].tile_base, infix[0].cols, infix[0].rows,
                     1u,  5u, 0u);
    draw_infix_block(infix[3].tile_base, infix[3].cols, infix[3].rows,
                     27u, 6u, 0u);
    draw_infix_block(infix[2].tile_base, infix[2].cols, infix[2].rows,
                     1u, 11u, 0u);
    draw_infix_block(infix[1].tile_base, infix[1].cols, infix[1].rows,
                     0u, 20u, 0u);
    if (uwait(120u)) return 1u;

    clear_fix_rect_force(0u, 5u, 40u, 22u);
    demo_fix_puts(2u, 2u, "INFIX 4..7  DEFAULT COLOURS", 1u);
    draw_infix_block(infix[4].tile_base, infix[4].cols, infix[4].rows,
                     4u,  5u, 0u);
    draw_infix_block(infix[5].tile_base, infix[5].cols, infix[5].rows,
                     2u, 11u, 0u);
    draw_infix_block(infix[6].tile_base, infix[6].cols, infix[6].rows,
                     24u, 12u, 0u);
    draw_infix_block(infix[7].tile_base, infix[7].cols, infix[7].rows,
                     4u, 19u, 0u);
    if (uwait(120u)) return 1u;

    clear_fix_rect_force(0u, 5u, 40u, 22u);
    demo_fix_puts(2u, 2u, "INFIX 8..9 + SFIX SHEET", 1u);
    draw_infix_block(infix[8].tile_base, infix[8].cols, infix[8].rows,
                     2u,  6u, 0u);
    draw_infix_block(infix[9].tile_base, infix[9].cols, infix[9].rows,
                     4u, 12u, 0u);
    draw_infix_block(1108u, 16u, 8u, 12u, 19u, 0u);
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

    /* --- 3) VOICE CUES — generated ADPCM-A voice bank --------------- */
    demo_fix_puts(2u, 5u, "3. VOICE CUES (direct word samples)", 2u);
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0x00u, 0x00u, 0x00u); snd_step();

    demo_fix_puts(2u, 11u, "playVoiceGetReady()  READY        ", 1u);
    playVoiceGetReady();
    if (uwait(55u)) return 1u;
    demo_fix_puts(2u, 11u, "playVoiceLetsGo()    GO           ", 1u);
    playVoiceLetsGo();
    if (uwait(55u)) return 1u;

    demo_fix_puts(2u, 11u, "                                  ", 0u);
    demo_fix_puts(2u, 13u, "                                  ", 0u);
    soundStopAll(); snd_step();

    /* --- 4) SSG/FM voice-style chip cues --------------------------- */
    demo_fix_puts(2u, 5u, "4. SSG/FM VOICE-STYLE CUES        ", 2u);
    soundSceneReset();                         snd_step();
    soundApplyMix(0x00u, 0x00u, 0x0Eu, 0x00u); snd_step();
    demo_fix_puts(2u, 13u, "SSG FORMANT: READY                ", 1u);
    playSSGVoiceGetReady();                    snd_step();
    if (uwait(90u)) return 1u;
    demo_fix_puts(2u, 13u, "SSG FORMANT: GO                   ", 1u);
    playSSGVoiceLetsGo();                      snd_step();
    if (uwait(90u)) return 1u;
    demo_fix_puts(2u, 13u, "FM CSM ROBOT SWEEP                ", 1u);
    playFMSpeechRobot();                       snd_step();
    if (uwait(70u)) return 1u;
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    demo_fix_puts(2u, 13u, "                                  ", 0u);

    /* --- 5) FM TRACKS — focused FM4/FM6 demo ----------------------- */
    demo_fix_puts(2u, 5u, "5. FM TRACKS (FM4 + FM6)          ", 2u);
    soundSceneReset();                         snd_step();
    /* FM volume was 0x0A of a 0x0F max (~67%) - noticeably quieter than
     * every other FM/SSG section in this chapter, which use 0x0E. */
    soundApplyMix(0x30u, 0x00u, 0x00u, 0x0Eu); snd_step();
    for (i = 0u; i < 2u; i++) {
        uint8_t fm_track = (i == 0u) ? SOUND_FM_D : SOUND_FM_F;
        demo_fix_puts(2u, 13u, (i == 0u) ? "FM 4  DUEL SUSPENSE" : "FM 6  VIBRATO LINE  ", 1u);
        soundStopAll();                            snd_step();
        soundSceneReset();                         snd_step();
        soundApplyMix(0x30u, 0x00u, 0x00u, 0x0Eu); snd_step();
        playFMTrack(fm_track);                     snd_step();
        /* The driver's patch loader (triggered by playFMTrack) writes
         * register $22 with the patch's own baked-in LFO byte, so a
         * soundFMSetLFO() call BEFORE playFMTrack is silently discarded
         * - the driver's own comment on this says explicitly to call it
         * AFTER the patch loads.  Moved here so "LFO off" actually
         * takes effect instead of being a no-op. */
        soundFMSetLFO(0x00u);                      snd_step();
        if (uwait(220u)) return 1u;
    }
    soundStopAll();          snd_step();
    soundSceneReset();       snd_step();
    soundFMSetLFO(0x00u);    snd_step();
    soundSetFMVolume(0x00u); snd_step();
    demo_fix_puts(2u, 13u, "                                  ", 0u);

    /* --- 6) FM EFFECTS: LFO (vibrato) + live tempo override ------- */
    demo_fix_puts(2u, 5u, "6. FM EFFECTS  (LFO + TEMPO)      ", 2u);
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0x00u, 0x00u, 0x0Eu); snd_step();
    playFMTrack(SOUND_FM_F);                  snd_step();

    demo_fix_puts(2u, 15u, "LFO OFF      (flat reference)     ", 1u);
    soundFMSetLFO(0x00u); snd_step();
    if (fm_lfo_hold(0x00u, 120u)) return 1u;
    demo_fix_puts(2u, 15u, "LFO rate=1   (slow wobble)        ", 1u);
    soundFMSetLFO(0x09u); snd_step();
    if (fm_lfo_hold(0x09u, 120u)) return 1u;
    demo_fix_puts(2u, 15u, "LFO rate=3   (medium vibrato)     ", 1u);
    soundFMSetLFO(0x0Bu); snd_step();
    if (fm_lfo_hold(0x0Bu, 120u)) return 1u;
    demo_fix_puts(2u, 15u, "LFO rate=6   (fast vibrato)       ", 1u);
    soundFMSetLFO(0x0Eu); snd_step();
    if (fm_lfo_hold(0x0Eu, 120u)) return 1u;
    soundFMSetLFO(0x00u); snd_step();

    demo_fix_puts(2u, 15u, "TEMPO period=1  (fast)            ", 1u);
    soundFMSetTempo(1u); snd_step();
    if (uwait(140u)) return 1u;
    demo_fix_puts(2u, 15u, "TEMPO period=6  (slow)            ", 1u);
    soundFMSetTempo(6u); snd_step();
    if (uwait(140u)) return 1u;
    demo_fix_puts(2u, 15u, "TEMPO period=1  (back to fast)    ", 1u);
    soundFMSetTempo(1u); snd_step();
    if (uwait(120u)) return 1u;

    soundStopAll();          snd_step();
    soundSceneReset();       snd_step();
    soundFMSetLFO(0x00u);    snd_step();
    soundSetFMVolume(0x00u); snd_step();
    demo_fix_puts(2u, 15u, "                                  ", 0u);

    /* --- 7) SSG TRACKS — 3 melodic loops --------------------------- */
    demo_fix_puts(2u, 5u, "7. SSG TRACKS (3 SHORT LOOPS)     ", 2u);
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0x00u, 0x0Eu, 0x00u); snd_step();
    for (i = 0u; i < 3u; i++) {
        char lbl[8];
        lbl[0] = 'S'; lbl[1] = 'S'; lbl[2] = 'G'; lbl[3] = ' ';
        lbl[4] = (char)('0' + (i + 1u)); lbl[5] = '\0';
        demo_fix_puts(2u, 17u, lbl, 1u);
        soundStopMusic();             snd_step();
        /* playSSGTrack's driver handler (play_ssg_index) unconditionally
         * resets VAR_SSG_PRESET to 0 as part of loading the track, so a
         * soundSetSSGPreset() call BEFORE playSSGTrack was silently
         * discarded every iteration - all 3 "different" tracks were
         * actually playing under preset 0.  soundSetSSGPreset's own
         * handler (exec_p_ssgpreset) only updates the preset and
         * reapplies it - it doesn't reload the track - so calling it
         * after is what actually makes the requested preset stick. */
        playSSGTrack(i);              snd_step();
        soundSetSSGPreset(i);         snd_step();
        if (uwait(360u)) return 1u;
    }
    /* FULL teardown so the next section starts on a known-good
     * driver state — soundStopMusic alone left state that could
     * suppress subsequent ADPCM-A/B and FM playback. */
    soundStopAll();           snd_step();
    soundSceneReset();        snd_step();
    demo_fix_puts(2u, 17u, "         ", 0u);

    /* --- 8) ADPCM-A SFX bank -------------------------------------- */
    demo_fix_puts(2u, 5u, "8. ADPCM-A SFX                    ", 2u);
    soundApplyMix(0x30u, 0x00u, 0x00u, 0x00u); snd_step();
    for (i = 0u; i < 6u; i++) {
        demo_fix_puts(2u, 19u, s_sfx_names[i], 1u);
        playSFX(s_sfx[i]); snd_step();
        if (uwait(40u)) return 1u;
    }
    demo_fix_puts(2u, 19u, "                ", 0u);

    /* --- 9) MULTITRACK MIX — bed dominant, FM/SSG accent only ------ *
     *
     * Streamed ADPCM-B beds are full mixed tracks; layering loud
     * FM or SSG melody on top usually clashes because the keys and
     * tempos of the MML tracks don't match the bed.  This section
     * keeps the bed dominant and FM/SSG quiet
     * so they read as subtle accent texture rather than competing
     * melodies.  Each mix RESTARTS the bed + accent together so the
     * listener hears the combined sound from the same instant. */
    demo_fix_puts(2u, 5u, "9. MULTITRACK MIXES               ", 2u);
    {
        static const uint8_t s_mix_track[3] = { SOUND_TRACK_A, SOUND_TRACK_C, SOUND_TRACK_G };
        static const uint8_t s_mix_fm[3]    = { SOUND_FM_C, SOUND_FM_D, SOUND_FM_C };
        static const uint8_t s_mix_ssg[3]   = { SOUND_SSG_B, SOUND_SSG_A, SOUND_SSG_B };
        static const char *const s_mix_lbl[3] = {
            "MIX A  bed + FM piano + SSG bass  ",
            "MIX B  bed + FM stab + SSG pulse  ",
            "MIX C  bed + FM/SSG short accent  "
        };
        uint8_t m;
        for (m = 0u; m < 3u; m++) {
            demo_fix_puts(2u, 21u, s_mix_lbl[m], 1u);
            soundStopAll();    snd_step();
            soundSceneReset(); snd_step();
            soundApplyMix(0x30u, 0xB8u, 0x04u, 0x05u); snd_step();
            playSFXB(s_mix_track[m]); snd_step();
            playFMTrack(s_mix_fm[m]);       snd_step();
            soundSetSSGPreset(s_mix_ssg[m]); snd_step();
            playSSGTrack(s_mix_ssg[m]);      snd_step();
            if (uwait(120u)) return 1u;
            soundSetFMVolume(0x00u);         snd_step();
            soundSetSSGVolume(0x02u);        snd_step();
            if (uwait(220u)) return 1u;
        }
    }
    soundFadeOutSpeed(6u); snd_step();
    if (uwait(40u)) return 1u;
    soundStopAll();        snd_step();
    demo_fix_puts(2u, 21u, "                                  ", 0u);

    /* --- 10) FADE TESTS on ADPCM-B TRACK 7 ------------------------- *
     *
     * WHAT YOU SHOULD HEAR:
     *  - FadeOut: the bed starts loud, then volume drops smoothly to
     *    silence over ~2-3 seconds.  Music doesn't stop — only its
     *    AMPLITUDE shrinks.
     *  - FadeIn:  silence first (vol = 0 from the prior fade-out),
     *    then the bed ramps back up to loud over ~2-3 seconds.
     *  - CancelFade: mid-fade, the volume INSTANTLY snaps back to
     *    the base level instead of continuing the fade.
     *
     * The driver's fade engine decrements/increments all master
     * volumes (music, ADPCM-A, ADPCM-B) by 8 per step at every Timer-B
     * IRQ when SPEED >= $FE.  Earlier values like SPEED=8 took ~70 s
     * to fade — essentially inaudible — which is why this section
     * sticks to $FF/$FE/$FD. */
    demo_fix_puts(2u, 5u, "10. FADE TESTS (ADPCM-B only)     ", 2u);

    /* --- FadeOut fast --- */
    demo_fix_puts(2u, 22u, "Listen: bed LOUD then silent      ", 0u);
    demo_fix_puts(2u, 23u, "FadeOut(0xFF) fastest             ", 1u);
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0xB8u, 0x00u, 0x00u); snd_step();
    playSFXB(SOUND_TRACK_G);                   snd_step();
    if (uwait(90u)) return 1u;
    soundFadeOutSpeed(0xFFu); snd_step();
    if (uwait(180u)) return 1u;

    /* --- FadeIn fast (continues from prior fade-out at vol 0) --- */
    demo_fix_puts(2u, 22u, "Listen: silent then LOUD again    ", 0u);
    demo_fix_puts(2u, 23u, "FadeIn(0xFF)  fastest ramp        ", 1u);
    soundFadeInSpeed(0xFFu); snd_step();
    if (uwait(180u)) return 1u;

    /* --- FadeOut medium --- */
    demo_fix_puts(2u, 22u, "Listen: slower fade than first one", 0u);
    demo_fix_puts(2u, 23u, "FadeOut(0xFD) medium              ", 1u);
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0xB8u, 0x00u, 0x00u); snd_step();
    playSFXB(SOUND_TRACK_G);                   snd_step();
    if (uwait(60u)) return 1u;
    soundFadeOutSpeed(0xFDu); snd_step();
    if (uwait(240u)) return 1u;
    demo_fix_puts(2u, 23u, "FadeIn(0xFD)  medium ramp         ", 1u);
    soundFadeInSpeed(0xFDu); snd_step();
    if (uwait(240u)) return 1u;

    /* --- CancelFade snap-back --- */
    demo_fix_puts(2u, 22u, "Listen: fading then SNAPS to loud ", 0u);
    demo_fix_puts(2u, 23u, "soundCancelFade  snap-back        ", 1u);
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0xB8u, 0x00u, 0x00u); snd_step();
    playSFXB(SOUND_TRACK_G);                   snd_step();
    if (uwait(60u)) return 1u;
    soundFadeOutSpeed(0xFEu); snd_step();
    if (uwait(80u)) return 1u;
    soundCancelFade();     snd_step();
    if (uwait(180u)) return 1u;

    /* --- Final FadeOut to silence --- */
    demo_fix_puts(2u, 22u, "                                  ", 0u);
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
static uint8_t s_hero_scale_x = U_SCALE_FULL;
static uint8_t s_hero_scale_y = U_SCALE_FULL;

static void NEOGEO_USER hero_draw(uint8_t frame)
{
    /* Grid-center anchoring is what the chapter code was tuned for —
     * s_hero_x / s_hero_y are treated as the centre of the tile grid,
     * so reverting to this stops the "everyone too high on the screen"
     * shift the bottom-center anchor caused.  Frame jitter is solved
     * separately by routing the actual VRAM writes through the demo's
     * vblank-safe queue (see demo_draw_sprite_screen). */
    int16_t strips = demo_screen_strips(frame);
    int16_t rows   = demo_screen_rows(frame);
    int16_t grid_w = asset_scaled_px((uint8_t)strips, s_hero_scale_x);
    int16_t grid_h = asset_scaled_px((uint8_t)rows, s_hero_scale_y);
    int16_t off_x  = demo_screen_x_offset(frame);
    int16_t off_y  = demo_screen_y_offset(frame);
    int16_t draw_x = (int16_t)(s_hero_x - (grid_w / 2) - off_x);
    int16_t draw_y = (int16_t)(s_hero_y - (grid_h / 2) - off_y);
    demo_draw_sprite_screen(frame, HERO_SLOT_FIRST,
                            draw_x, draw_y,
                            (uint8_t)strips,
                            (uint8_t)rows,
                            s_hero_scale_x, s_hero_scale_y);
}

static void NEOGEO_USER hero_place(int16_t cx, int16_t cy)
{
    s_hero_x = cx;
    s_hero_y = cy;
}

static void NEOGEO_USER hero_scale(uint8_t scale)
{
    s_hero_scale_x = scale;
    s_hero_scale_y = scale;
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
/*  Chapter 24 — Character Select (portrait animation, AI or joystick)  */
/* ================================================================== */
/*
 * Two portraits shown side by side: the girl's 7-frame idle-animation
 * face (screen ids 68-74, already-imported frames sitting right before
 * the eagle's own frames in the same character sheet) and the eagle's
 * existing ground/flying poses (already used by chap_physics/chap_char_2d,
 * reused here rather than sourcing new art).  Plays as an attract-mode
 * AI demo by default, auto-swapping the selection and animating whichever
 * one is highlighted; the moment the joystick moves left/right, the
 * player owns the cursor for the rest of the chapter (same pattern as
 * the shooter and physics chapters), with the same idle-advance once
 * player-controlled.
 */
static uint8_t NEOGEO_USER chap_char_select(void)
{
    enum {
        IDLE_ADVANCE_FRAMES = 300u,   /* ~5s idle once player-controlled */
        AI_SWITCH_FRAMES    = 150u,   /* ~2.5s per AI auto-switch */
        GIRL_FRAME_COUNT    = 7,
        EAGLE_FRAME_COUNT   = 3,
        SLOT_GIRL           = 40u,
        SLOT_EAGLE          = 56u
    };
    static const uint8_t girl_frames[GIRL_FRAME_COUNT]  = { 68u, 69u, 70u, 71u, 72u, 73u, 74u };
    static const uint8_t eagle_ground                   = 75u;
    static const uint8_t eagle_frames[EAGLE_FRAME_COUNT] = { 78u, 79u, 80u };
    uint16_t prev_joy = 0u;
    uint16_t idle_frames = 0u;
    uint8_t  player_controlled = 0u;
    uint8_t  sel = 0u;    /* 0 = girl, 1 = eagle */
    uint16_t anim_t = 0u;
    uint16_t t;
    uint8_t  i;

    chap_header(24u, "CHAR SELECT", "PORTRAIT DEMO  JOYSTICK OVERRIDE");
    demo_fix_puts(2u, 2u, "WATCHING - ARROWS TO CHOOSE", 1u);
    demo_fix_puts(2u, 3u, "LEFT: GIRL   RIGHT: EAGLE", 0u);
    snd_cross_to(SOUND_MUSIC_C);

    for (i = 0u; i < GIRL_FRAME_COUNT; i++)  demo_load_screen_palette(girl_frames[i]);
    demo_load_screen_palette(eagle_ground);
    for (i = 0u; i < EAGLE_FRAME_COUNT; i++) demo_load_screen_palette(eagle_frames[i]);

    for (t = 0u; t < 900u; t++) {
        uint16_t joy = poll_joystick();
        uint16_t edge = (uint16_t)(joy & (uint16_t)(~prev_joy));
        uint8_t girl_frame;
        uint8_t eagle_frame;
        prev_joy = joy;

        if (!player_controlled && (edge & (JOY_LEFT | JOY_RIGHT))) {
            player_controlled = 1u;
            idle_frames = 0u;
            demo_fix_puts(2u, 2u, "ARROWS TO CHOOSE               ", 1u);
        }

        if (player_controlled) {
            if (joy) idle_frames = 0u;
            else if (idle_frames < 0xFFFFu) idle_frames++;
            if (idle_frames >= IDLE_ADVANCE_FRAMES) return 1u;

            if ((edge & JOY_LEFT)  && sel != 0u) { sel = 0u; anim_t = 0u; playSFX(SOUND_SFX_5); }
            if ((edge & JOY_RIGHT) && sel != 1u) { sel = 1u; anim_t = 0u; playSFX(SOUND_SFX_5); }
        } else if ((t % AI_SWITCH_FRAMES) == 0u) {
            sel = (uint8_t)(sel ^ 1u);
            anim_t = 0u;
            playSFX(SOUND_SFX_5);
        }

        demo_fix_puts(6u,  8u, sel == 0u ? "[ GIRL  ]" : "  GIRL   ", sel == 0u ? 2u : 1u);
        demo_fix_puts(22u, 8u, sel == 1u ? "[ EAGLE ]" : "  EAGLE  ", sel == 1u ? 2u : 1u);

        girl_frame  = (sel == 0u) ? girl_frames[(anim_t / 8u) % GIRL_FRAME_COUNT]   : girl_frames[0];
        eagle_frame = (sel == 1u) ? eagle_frames[(anim_t / 8u) % EAGLE_FRAME_COUNT] : eagle_ground;
        draw_asset_bottom_center(girl_frame,  SLOT_GIRL,  110, 180, U_SCALE_60, U_SCALE_60);
        draw_asset_bottom_center(eagle_frame, SLOT_EAGLE, 210, 180, U_SCALE_45, U_SCALE_45);
        anim_t++;

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
        EAGLE_SCALE      = 0x33u,   /* ~20% (was 0x4D/30%, before that 0x80/50%) */
        EAGLE_BODY_W     = 13,      /* scaled down with EAGLE_SCALE so the
                                     * physics collision box keeps matching
                                     * the smaller sprite (was 20, before 32) */
        EAGLE_BODY_H     = 19,      /* was 28, before 48 */
        EAGLE_START_X    = 160,
        EAGLE_START_Y    = 110,    /* closer to the floor so the fall is short */
        PLATFORM_X       = 0,
        PLATFORM_W       = 320
    };

    enum { IDLE_ADVANCE_FRAMES = 300u };  /* ~5s idle once player-controlled */
    NGCharacter *eagle;
    uint16_t t;
    uint8_t  frame;
    uint8_t  i;
    uint16_t prev_joy = 0u;
    uint16_t idle_frames = 0u;
    uint8_t  player_controlled = 0u;
    int16_t  drift_x = EAGLE_START_X;

    chap_header(6u, "PHYSICS", "GRAVITY  SOLIDS  GROUNDED");

    /*
     * EXPLICIT SCREEN CLEAR before drawing anything.
     * Even though chap_header runs ng_clear_screen_full(), we re-clear
     * the FIX layer and force the backdrop back to black here so
     * previous chapter colours cannot bleed through.
     */
    clearFix();
    setBACKDROP(BLACK);

    /* Re-draw header text after the clear */
    demo_fix_puts(2u,  0u, "CH.06",        2u);
    demo_fix_puts(8u,  0u, "PHYSICS",      2u);
    demo_fix_puts(2u,  1u, "GRAVITY  SOLIDS  GROUNDED", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT",      0u);

    demo_fix_puts(2u, 2u, "WATCHING - PRESS START TO PLAY", 1u);
    demo_fix_puts(2u, 3u, "FLOOR Y=184  BAR Y=184",      0u);
    snd_cross_to(SOUND_MUSIC_A);

    /*
     * Bar drawn at row 23 (Y=184..191) to align with the physics solid's
     * top at Y=184 - it was previously one row lower (row 24, Y=192) to
     * avoid the FIX layer drawing over the eagle's feet, but that left
     * a visible 8px gap between the grounded eagle and the line it's
     * meant to be standing on.
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
    /* Solid Y = 184; visible FIX bar starts at y=192. */
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
    eagle->sprite_offset_y = -(int16_t)(((uint16_t)demo_screen_rows(frame) * 16u * EAGLE_SCALE) >> 8);
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

    for (t = 0u; t < 540u; t++) {
        uint8_t grounded;
        uint8_t want;
        uint16_t joy = poll_joystick();
        uint16_t edge = (uint16_t)(joy & (uint16_t)(~prev_joy));
        prev_joy = joy;

        if (!player_controlled && (edge & (START1 | START2))) {
            player_controlled = 1u;
            idle_frames = 0u;
            demo_fix_puts(2u, 2u, "ARROWS MOVE                    ", 1u);
        }

        if (player_controlled) {
            if (joy) idle_frames = 0u;
            else if (idle_frames < 0xFFFFu) idle_frames++;
            if (idle_frames >= IDLE_ADVANCE_FRAMES) return 1u;

            if ((joy & JOY_LEFT)  && eagle->x > (int16_t)(PLATFORM_X + 16)) {
                ng_char_set_pos(eagle, (int16_t)(eagle->x - 2), eagle->y);
            }
            if ((joy & JOY_RIGHT) && eagle->x < (int16_t)(PLATFORM_X + PLATFORM_W - 16)) {
                ng_char_set_pos(eagle, (int16_t)(eagle->x + 2), eagle->y);
            }
        } else {
            /* Attract-mode default: gentle back-and-forth drift so the
             * eagle isn't just sitting static while unattended. */
            drift_x = (int16_t)(EAGLE_START_X + (((int16_t)((t >> 2) % 120u)) - 60));
            ng_char_set_pos(eagle, drift_x, eagle->y);
        }

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
            eagle->sprite_offset_y = -(int16_t)(((uint16_t)demo_screen_rows(want) * 16u * EAGLE_SCALE) >> 8);
            frame = want;
        }

        demo_fix_puts(2u, 4u, grounded ? "STATE: GROUNDED" : "STATE: FALLING ",
                              grounded ? 2u : 1u);

        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 07 — Camera Lab (auto-tour)                                  */
/*                                                                       */
/*  Auto-cycles through six camera modes; the player walks in a         */
/*  scripted pattern so the user just watches each behaviour.           */
/*                                                                       */
/*  Music: ADPCM-B loops the whole chapter as the bed; FM and SSG are    */
/*  muted; ADPCM-A is reserved for the per-mode SFX chirps.             */
/* ================================================================== */
enum {
    CAMLAB_MODE_HARD_FOLLOW = 0u,   /* tight  */
    CAMLAB_MODE_SMOOTH      = 1u,   /* eased  */
    CAMLAB_MODE_SHAKE       = 2u,   /* shake bursts                       */
    CAMLAB_MODE_PAN         = 3u,   /* cinematic pan to centre and back   */
    CAMLAB_MODE_VERTICAL    = 4u,   /* vertical scroll, camera follows Y  */
    CAMLAB_MODE_DEADZONE    = 5u,   /* dead-zone box, shown last          */
    CAMLAB_MODE_COUNT       = 6u
};

static const char *const CAMLAB_LABEL[CAMLAB_MODE_COUNT] = {
    "HARD FOLLOW   ",
    "SMOOTH FOLLOW ",
    "SHAKE TEST    ",
    "CINEMATIC PAN ",
    "VERTICAL SCROL",
    "DEAD-ZONE BOX ",
};

static void NEOGEO_USER camlab_apply_mode(NGCamera *cam, uint8_t mode,
                                          int16_t pan_dest_x)
{
    switch (mode) {
    case CAMLAB_MODE_HARD_FOLLOW:
        ng_camera_set_follow_speed(cam, 16u);
        ng_camera_set_dead_zone(cam, 0u, 0u);
        ng_camera_set_look_ahead(cam, 0, 0, 0u);
        break;
    case CAMLAB_MODE_SMOOTH:
        ng_camera_set_follow_speed(cam, 4u);
        ng_camera_set_dead_zone(cam, 0u, 0u);
        ng_camera_set_look_ahead(cam, 0, 0, 0u);
        break;
    case CAMLAB_MODE_SHAKE:
        ng_camera_set_follow_speed(cam, 8u);
        ng_camera_set_dead_zone(cam, 0u, 0u);
        ng_camera_set_look_ahead(cam, 0, 0, 0u);
        ng_camera_shake(cam, 6u, 30u);
        break;
    case CAMLAB_MODE_PAN:
        ng_camera_set_follow_speed(cam, 6u);
        ng_camera_pan_to(cam, pan_dest_x, 0, 6u);
        break;
    case CAMLAB_MODE_VERTICAL:
        ng_camera_set_follow_speed(cam, 8u);
        ng_camera_set_dead_zone(cam, 0u, 0u);
        ng_camera_set_look_ahead(cam, 0, 0, 0u);
        break;
    case CAMLAB_MODE_DEADZONE:
        ng_camera_set_follow_speed(cam, 8u);
        ng_camera_set_dead_zone(cam, 56u, 0u);
        ng_camera_set_look_ahead(cam, 0, 0, 0u);
        break;
    default:
        break;
    }
}

/*
 * Two-line world ruler painted into the FIX layer along rows 8..9.
 * Tracks the camera so the labelled markers slide past as the
 * camera scrolls — visible proof that the world coordinates the
 * char is using ARE actually scrolling under the camera, not just
 * the background image.
 */
static void NEOGEO_USER camlab_world_markers(int16_t cam_x)
{
    static const int16_t markers[5] = { 0, 192, 384, 576, 768 };
    static const char  *labels[5]   = { "S", "1", "2", "3", "E" };
    uint8_t i;

    /* Solid track row. */
    demo_fix_puts(0u, 8u,
                  "----------------------------------------",
                  0u);

    /* Marker labels on the row below. */
    demo_fix_puts(0u, 9u,
                  "                                        ",
                  0u);
    for (i = 0u; i < 5u; i++) {
        int16_t sx = (int16_t)(markers[i] - cam_x);
        if (sx >= 0 && sx < 320) {
            uint8_t col = (uint8_t)(sx >> 3);
            demo_fix_puts(col, 9u, labels[i], 2u);
        }
    }
}

/* Optional dead-zone box painted around the player's screen position
 * so the user can see when the camera is "asleep" because the char
 * hasn't yet reached the edge of the dead zone. */
static void NEOGEO_USER camlab_deadzone_box(int16_t cx_screen,
                                            int16_t cy_screen,
                                            uint8_t half_w,
                                            uint8_t half_h)
{
    int16_t x0;
    int16_t x1;
    int16_t y0;
    int16_t y1;
    uint8_t col;

    if (half_w == 0u && half_h == 0u) return;

    x0 = (int16_t)(cx_screen - (int16_t)half_w);
    x1 = (int16_t)(cx_screen + (int16_t)half_w);
    y0 = (int16_t)(cy_screen - (int16_t)half_h);
    y1 = (int16_t)(cy_screen + (int16_t)half_h);

    /* Snap to FIX columns/rows (8-px), clamp to visible 40x28. */
    if (x0 < 0) x0 = 0;
    if (x1 > 319) x1 = 319;
    if (y0 < 0) y0 = 0;
    if (y1 > 223) y1 = 223;

    {
        uint8_t cx0 = (uint8_t)(x0 >> 3);
        uint8_t cx1 = (uint8_t)(x1 >> 3);
        uint8_t cy0 = (uint8_t)(y0 >> 3);
        uint8_t cy1 = (uint8_t)(y1 >> 3);

        if (cx1 <= cx0 || cy1 <= cy0) return;

        for (col = cx0; col <= cx1; col++) {
            demo_fix_puts(col, cy0, "-", 1u);
            demo_fix_puts(col, cy1, "-", 1u);
        }
        demo_fix_puts(cx0, cy0, "+", 1u);
        demo_fix_puts(cx1, cy0, "+", 1u);
        demo_fix_puts(cx0, cy1, "+", 1u);
        demo_fix_puts(cx1, cy1, "+", 1u);
    }
}

static uint8_t NEOGEO_USER chap_camera(void)
{
    NGCamera cam;
    const int16_t  WORLD_RIGHT  = 768;
    const int16_t  WORLD_BOTTOM = 320;             /* > 224 -> Y can scroll */
    const int16_t  PAN_DEST_X   = 384;             /* world centre          */
    const int16_t  HERO_Y_REST  = 156;             /* mid-low resting band  */
    const uint16_t MODE_FRAMES  = 180u;            /* 3 sec per mode -> 18 s
                                                    * total for 6 modes    */
    const uint16_t TOTAL_FRAMES = MODE_FRAMES * CAMLAB_MODE_COUNT;

    int16_t  player_world_x = 80;
    int16_t  player_world_y = HERO_Y_REST;
    uint8_t  mode           = CAMLAB_MODE_HARD_FOLLOW;
    uint8_t  prev_mode      = 0xFFu;
    uint16_t t;
    int16_t  vx_logical     = 0;
    int16_t  vy_logical     = 0;
    char     hud[6];

    chap_header(7u, "CAMERA LAB",
                "AUTO TOUR: HARD SMOOTH SHAKE PAN VERT DEADZONE");

    /* Sound: ADPCM-B carries the loop the whole chapter.  FM/SSG muted,
     * ADPCM-A reserved for the per-mode SFX chirps. */
    soundSceneReset();              waitVbl();
    soundSetADPCMAVolume(0x3Cu);    waitVbl();
    soundSetADPCMBVolume(0xBCu);    waitVbl();
    soundSetSSGVolume(0x00u);       waitVbl();
    soundSetFMVolume(0x00u);        waitVbl();
    soundPlayGameLoop(SOUND_MUSIC_E);
    waitVbl();

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, WORLD_RIGHT, WORLD_BOTTOM);
    hero_scale(U_SCALE_60);

    for (t = 0u; t < TOTAL_FRAMES; t++) {
        uint8_t  next_mode = (uint8_t)((t / MODE_FRAMES) % CAMLAB_MODE_COUNT);
        uint16_t mode_t    = (uint16_t)(t % MODE_FRAMES);
        uint8_t  frame;
        int16_t  screen_x;
        int16_t  screen_y;
        int16_t  bg_x;
        int16_t  bg_y;

        /* --- transition into a new mode --------------------------- */
        if (next_mode != prev_mode) {
            mode = next_mode;
            camlab_apply_mode(&cam, mode, PAN_DEST_X);
            /* Snap player Y back to the resting band whenever the
             * vertical mode releases. */
            if (prev_mode == CAMLAB_MODE_VERTICAL) {
                player_world_y = HERO_Y_REST;
            }
            playSFX(SOUND_SFX_5);
            prev_mode = mode;
        }

        /* --- scripted X movement: 2-second swing -------------------
         * Skip in PAN mode so the camera drift to the centre isn't
         * masked by the player chasing it. */
        vx_logical = 0;
        if (mode != CAMLAB_MODE_PAN) {
            uint16_t phase = (uint16_t)(mode_t % 120u);   /* 2 s period */
            if (phase < 60u) {
                vx_logical = +2;
                if (player_world_x < WORLD_RIGHT - 16) player_world_x += 2;
            } else {
                vx_logical = -2;
                if (player_world_x > 16) player_world_x -= 2;
            }
        }

        /* --- scripted Y movement only during VERTICAL mode ---------
         * 130..220 sweep makes the camera follow vertically while the
         * floor BG slides up/down behind the player. */
        vy_logical = 0;
        if (mode == CAMLAB_MODE_VERTICAL) {
            if (mode_t < (MODE_FRAMES / 2u)) {
                vy_logical = +1;
                if (player_world_y < 220) player_world_y += 1;
            } else {
                vy_logical = -1;
                if (player_world_y > 100) player_world_y -= 1;
            }
        }

        /* Mode-specific scripted events */
        if (mode == CAMLAB_MODE_SHAKE) {
            /* Re-trigger every ~60 frames inside the 180-frame slot. */
            if (mode_t == 20u || mode_t == 90u) {
                ng_camera_shake(&cam, 6u, 20u);
                playSFX(SOUND_SFX_8);
            }
        }
        if (mode == CAMLAB_MODE_PAN) {
            if (mode_t == 20u) {
                ng_camera_pan_to(&cam, PAN_DEST_X, 0, 6u);
                playSFX(SOUND_SFX_7);
            }
            if (mode_t == 110u) {
                ng_camera_pan_to(&cam, player_world_x, 0, 6u);
                playSFX(SOUND_SFX_7);
            }
        }

        /* --- camera update ---------------------------------------- */
        ng_camera_update(&cam, player_world_x, player_world_y, vx_logical);

        /* --- background ------------------------------------------- */
        bg_x = -(int16_t)((uint16_t)cam.x & 0x00FFu);
        /* In VERTICAL mode also tie BG Y to cam.y so the world looks
         * like it's scrolling, not just the player.  Modulo 32 keeps
         * the offset inside one tile so the strips don't tear. */
        if (mode == CAMLAB_MODE_VERTICAL) {
            bg_y = -(int16_t)((uint16_t)cam.y & 0x001Fu);
        } else {
            bg_y = 0;
        }
        draw_scrolling_background(U_BG_FOREST, bg_x, bg_y);

        /* --- screen position of the player ------------------------ */
        screen_x = (int16_t)(player_world_x - cam.x);
        screen_y = (int16_t)(player_world_y - cam.y);
        if (screen_x < -32) screen_x = -32;
        if (screen_x > 320) screen_x =  320;

        /* --- FIX HUD (top of screen) ----------------------------- */
        demo_fix_puts(2u, 4u, "MODE:", 1u);
        demo_fix_puts(8u, 4u, CAMLAB_LABEL[mode], 2u);

        digit3(hud, (uint16_t)player_world_x);
        demo_fix_puts(2u, 5u, "PLAYER X:", 1u);
        demo_fix_puts(12u, 5u, hud, 2u);
        digit3(hud, (uint16_t)((cam.x < 0) ? 0 : cam.x));
        demo_fix_puts(18u, 5u, "CAM X:", 1u);
        demo_fix_puts(25u, 5u, hud, 2u);

        demo_fix_puts(2u, 6u, "DEADZONE:", 1u);
        if (cam.dead_zone_x || cam.dead_zone_y) {
            digit3(hud, (uint16_t)cam.dead_zone_x);
            demo_fix_puts(12u, 6u, hud, 2u);
        } else {
            demo_fix_puts(12u, 6u, "OFF", 0u);
        }
        demo_fix_puts(18u, 6u, "SHAKE:", 1u);
        if (cam.shake_frames) {
            digit3(hud, (uint16_t)cam.shake_frames);
            demo_fix_puts(25u, 6u, hud, 2u);
        } else {
            demo_fix_puts(25u, 6u, "--  ", 0u);
        }

        /* World ruler with S/1/2/3/E markers sliding under the camera. */
        camlab_world_markers(cam.x);

        /* Dead-zone box only on the dead-zone mode itself. */
        if (mode == CAMLAB_MODE_DEADZONE) {
            camlab_deadzone_box(screen_x,
                                (int16_t)(screen_y - 24),
                                cam.dead_zone_x,
                                (uint8_t)(cam.dead_zone_y ? cam.dead_zone_y : 20u));
        }

        /* --- player draw ----------------------------------------- */
        frame = (vx_logical || vy_logical)
              ? s_hero_walk [(t / 6u)  % 8u]
              : s_hero_stand[(t / 12u) % 8u];
        {
            int16_t saved_x = s_hero_x;
            int16_t saved_y = s_hero_y;
            s_hero_x = screen_x;
            s_hero_y = screen_y;
            hero_draw(frame);
            s_hero_x = saved_x;
            s_hero_y = saved_y;
        }

        if (uframe()) {
            ng_level_set_scroll(0, 0);
            return 1u;
        }
    }
    ng_level_set_scroll(0, 0);
    return 0u;
}

/* ================================================================== */
/*  Chapter 08 — Palette FX                                              */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_palette_fx(void)
{
    uint16_t t;
    uint8_t stage = 0xffu;
    uint8_t active_fx = 0u;
    uint8_t active_pal = DEMO_SCREEN_PALETTE(s_fx_effect_frames[0]);

    chap_header(8u, "PALETTE FX", "SPRITE PALETTE STAGES");
    /* One-shot hard clear of the lower-hero sprite window before the
     * first pose draw — kills any stale strips left from the prior
     * chapter that would otherwise stay attached when the new frame's
     * footprint happens to be narrower than the old one. */
    ng_sprite_park_off_range(HERO_SLOT_FIRST, 16u);
    demo_fix_puts(2u, 2u, "EFFECTS: 040 / 041 / 048 / 050", 1u);
    demo_fix_puts(2u, 3u, "CHAR: 047R05C06 -> 051R05C10", 0u);
    snd_cross_to(SOUND_MUSIC_G);

    demo_load_screen_palette(s_fx_effect_frames[0]);
    demo_load_screen_palette(s_fx_effect_frames[1]);
    demo_load_screen_palette(s_fx_effect_frames[2]);
    demo_load_screen_palette(s_fx_effect_frames[3]);
    demo_fix_puts(2u, 6u, "ACTIVE:", 1u);

    for (t = 0u; t < 660u; t++) {
        uint8_t next_stage = (uint8_t)(t / 132u);
        uint8_t pose = s_fx_char_frames[(t / 12u) % 5u];
        uint8_t fx_left = s_fx_effect_frames[(t / 16u) & 3u];
        uint8_t fx_mid = s_fx_effect_frames[((t / 16u) + 1u) & 3u];
        uint8_t fx_right = s_fx_effect_frames[((t / 16u) + 2u) & 3u];

        if (next_stage != stage) {
            ng_palfx_stop(active_pal);
            demo_load_screen_palette(s_fx_effect_frames[0]);
            demo_load_screen_palette(s_fx_effect_frames[1]);
            demo_load_screen_palette(s_fx_effect_frames[2]);
            demo_load_screen_palette(s_fx_effect_frames[3]);
            stage = next_stage;
            active_fx = s_fx_effect_frames[stage & 3u];
            active_pal = DEMO_SCREEN_PALETTE(active_fx);

            switch (stage) {
            case 0u:
                demo_fix_puts(10u, 6u, "FADE IN          ", 2u);
                ng_palfx_fade_in(active_pal, s_palfx_base, 72u);
                break;
            case 1u:
                demo_fix_puts(10u, 6u, "WHITE FLASH      ", 2u);
                ng_palfx_flash_white(active_pal, s_palfx_base, 28u);
                break;
            case 2u:
                demo_fix_puts(10u, 6u, "RED HIT FLASH    ", 2u);
                ng_palfx_flash_red(active_pal, s_palfx_base, 28u);
                break;
            case 3u:
                demo_fix_puts(10u, 6u, "PULSE            ", 1u);
                ng_palfx_pulse(active_pal, s_palfx_base, 32u);
                break;
            default:
                demo_fix_puts(10u, 6u, "COLOR CYCLE      ", 1u);
                ng_palfx_cycle(active_pal, s_palfx_base, 8u, 14u);
                break;
            }
        }

        demo_load_screen_palette(pose);
        draw_asset_bottom_center(fx_left, 124u, 70, 98, U_SCALE_55, U_SCALE_55);
        draw_asset_bottom_center(fx_mid, 148u, 160, 86, U_SCALE_55, U_SCALE_55);
        draw_asset_bottom_center(fx_right, 172u, 250, 98, U_SCALE_55, U_SCALE_55);
        draw_asset_bottom_center(pose, HERO_SLOT_FIRST, 160,
                                 FX_HERO_LIFT_Y, U_SCALE_55, U_SCALE_55);

        if (uframe()) return 1u;
    }

    ng_palfx_stop(active_pal);
    return 0u;
}

/* ================================================================== */
/*  Chapter 09 — Particles                                              */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_particles(void)
{
    uint16_t t;
    const uint8_t spark_id    = s_fx_effect_frames[0];
    const uint8_t impact_id   = s_fx_char_frames[4];
    const uint16_t spark_tile = DEMO_SCREEN_TILE(spark_id);
    const uint8_t  spark_pal  = DEMO_SCREEN_PALETTE(spark_id);

    chap_header(9u, "PARTICLES", "HERO SPECIAL MOVE + FX");
    /* Same hard wipe as the palette FX chapter so the lower hero
     * starts on a clean window — no stale strips left over when the
     * next pose is narrower than the previous one. */
    ng_sprite_park_off_range(HERO_SLOT_FIRST, 16u);
    demo_fix_puts(2u, 2u, "EFFECTS STAY ABOVE  CHARACTER BELOW", 1u);
    demo_fix_puts(2u, 3u, "040/041/048/050 PARTICLE BURSTS",0u);
    demo_fix_puts(2u, 4u, "ACTIVE: ",                    2u);
    snd_cross_to(SOUND_MUSIC_F);

    hero_place(160, 152);
    demo_load_screen_palette(spark_id);
    demo_load_screen_palette(impact_id);

    s_draw_particles = 1u;

    for (t = 0u; t < 540u; t++) {
        /*
         * Phase machine — 3 special-move beats:
         *   [  0..160) WIND-UP        : faint dust around feet
         *   [160..360) STRIKE         : magic sparks burst from sword arc
         *   [360..540) FINISHER       : explosion + smoke ring + screen-flash
         */
        uint8_t hero_frame;
        int16_t bx = (int16_t)(92 + (int16_t)((t * 2u) % 136u));
        int16_t by = (int16_t)(78 + (int16_t)((t & 31u) >> 3));

        if (t < 160u) {
            hero_frame = s_fx_char_frames[(t / 12u) % 5u];
            if ((t % 12u) == 0u) {
                ng_particle_spawn(NG_PART_DUST, NG_PART_PRI_NORMAL,
                                  (int16_t)(126 + (int16_t)(t & 15u)),
                                  178,
                                  -(1L << (NG_FP_SHIFT - 1)),
                                  -(1L << (NG_FP_SHIFT - 1)),
                                  20u, spark_tile, spark_pal, 1u, 1u);
                ng_particle_spawn(NG_PART_DUST, NG_PART_PRI_NORMAL,
                                  (int16_t)(178 - (int16_t)(t & 15u)),
                                  178,
                                  (1L << (NG_FP_SHIFT - 1)),
                                  -(1L << (NG_FP_SHIFT - 1)),
                                  20u, spark_tile, spark_pal, 1u, 1u);
            }
            if (t == 8u) playSFX(SOUND_SFX_9);
        } else if (t < 360u) {
            hero_frame = s_fx_char_frames[((t / 10u) + 1u) % 5u];
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
            hero_frame = ((t / 20u) & 1u) ? impact_id
                                          : s_fx_char_frames[((t - 360u) / 10u) % 5u];
            if (t == 360u) {
                uint8_t k;
                playSFX(SOUND_SFX_10);
                ng_particle_spawn(NG_PART_EXPLOSION, NG_PART_PRI_CRITICAL,
                                  160, 92, 0, 0, 36u,
                                  spark_tile, spark_pal, 1u, 1u);
                for (k = 0u; k < 8u; k++) {
                    int32_t a = (int32_t)k * 2L;
                    ng_particle_spawn(NG_PART_SMOKE, NG_PART_PRI_NORMAL,
                                      160, 92,
                                      (a - 8L) << (NG_FP_SHIFT - 1),
                                      -(a) << (NG_FP_SHIFT - 1),
                                      40u, spark_tile, spark_pal, 1u, 1u);
                }
            }
            if ((t % 24u) == 0u) {
                ng_particle_spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_NORMAL,
                                  (int16_t)(160 + ((int16_t)(t & 31u) - 16)),
                                  86,
                                  0, -(1L << NG_FP_SHIFT),
                                  30u, spark_tile, spark_pal, 1u, 1u);
            }
        }

        draw_asset_bottom_center(s_fx_effect_frames[(t / 8u) & 3u],
                                 164u, bx, by, U_SCALE_55, U_SCALE_55);
        draw_asset_bottom_center(hero_frame, HERO_SLOT_FIRST,
                                 160, FX_HERO_LIFT_Y, U_SCALE_55, U_SCALE_55);

        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 23 — Particle Load (pool stress test + priority eviction)    */
/* ================================================================== */
/*
 * Demonstrates the particle system under load rather than in a single
 * narrative use (that's chap_particles above): burst sparks, ambient
 * dust/smoke, an explosion fan, then a priority-eviction stress test
 * that floods the 32-slot pool and shows the live count on FIX.
 *
 * Spawns via ng_particle_spawn() and lets uframe() draw them through
 * s_draw_particles - the same contract chap_particles/chap_feedback
 * already use.  An earlier version of this chapter called into
 * demo_particles.c's demo_particles_run(), which calls ng_particles_draw()
 * directly mid-loop (before its own demo_frame()'s waitVbl) instead of
 * going through that queue - untested dead code that had never actually
 * been exercised, and the direct draw caused visible flicker once wired
 * into the live reel.  Native ng_particle_spawn() calls avoid that path
 * entirely.
 */
static uint8_t NEOGEO_USER chap_particle_showcase(void)
{
    const uint8_t  spark_id    = s_fx_effect_frames[0];
    const uint16_t spark_tile  = DEMO_SCREEN_TILE(spark_id);
    const uint8_t  spark_pal   = DEMO_SCREEN_PALETTE(spark_id);
    uint16_t t;
    char cnt[10];

    chap_header(23u, "PARTICLE LOAD", "POOL STRESS + PRIORITY EVICTION");
    demo_fix_puts(2u, 2u, "BURST  AMBIENT  EXPLOSION  POOL FLOOD", 1u);
    demo_fix_puts(2u, 3u, "OPTIONAL PARTICLES DROP GRACEFULLY", 0u);
    snd_cross_to(SOUND_MUSIC_F);
    demo_load_screen_palette(spark_id);

    s_draw_particles = 1u;

    for (t = 0u; t < 600u; t++) {
        if (t < 140u) {
            /* Phase 1: 8-way hit-spark bursts. */
            if ((t % 40u) == 0u) {
                uint8_t k;
                int16_t cx = (int16_t)(80 + (int16_t)((t >> 2) & 0x3Fu));
                for (k = 0u; k < 8u; k++) {
                    int32_t a = (int32_t)k;
                    ng_particle_spawn(NG_PART_HIT_SPARK, NG_PART_PRI_CRITICAL,
                                      cx, 112,
                                      ((a - 4L) << (NG_FP_SHIFT - 1)),
                                      ((a - 4L) << (NG_FP_SHIFT - 1)),
                                      18u, spark_tile, spark_pal, 1u, 1u);
                }
                playSFX(SOUND_SFX_8);
            }
        } else if (t < 280u) {
            /* Phase 2: ambient dust + smoke drift. */
            if ((t % 8u) == 0u) {
                int16_t dx = (int16_t)(40 + (int16_t)((t * 3u) & 0xFFu));
                ng_particle_spawn(NG_PART_DUST, NG_PART_PRI_NORMAL,
                                  dx, 160, 0, -(1L << (NG_FP_SHIFT - 1)),
                                  24u, spark_tile, spark_pal, 1u, 1u);
                if ((t % 24u) == 0u) {
                    ng_particle_spawn(NG_PART_SMOKE, NG_PART_PRI_NORMAL,
                                      (int16_t)(dx + 16), 150,
                                      0, -(1L << (NG_FP_SHIFT - 1)),
                                      40u, spark_tile, spark_pal, 1u, 1u);
                }
            }
        } else if (t < 420u) {
            /* Phase 3: explosion + fanned slash sparks. */
            if ((t % 60u) == 0u) {
                uint8_t k;
                int16_t ex = (int16_t)(120 + (int16_t)((t * 5u) & 0x3Fu));
                ng_particle_spawn(NG_PART_EXPLOSION, NG_PART_PRI_CRITICAL,
                                  ex, 96, 0, 0, 36u,
                                  spark_tile, spark_pal, 1u, 1u);
                for (k = 0u; k < 6u; k++) {
                    int32_t a = (int32_t)k * 2L - 5L;
                    ng_particle_spawn(NG_PART_SLASH_TRAIL, NG_PART_PRI_NORMAL,
                                      ex, 96,
                                      (a << (NG_FP_SHIFT - 1)),
                                      -(2L << NG_FP_SHIFT),
                                      26u, spark_tile, spark_pal, 1u, 1u);
                }
                playSFX(SOUND_SFX_7);
            }
        } else {
            /* Phase 4: flood the pool with OPTIONAL particles while
             * CRITICAL hit-sparks keep landing, to show priority
             * eviction under load. */
            if ((t % 4u) == 0u) {
                int16_t mx = (int16_t)(160 + (int16_t)((int16_t)(t & 0x3Fu) - 32));
                int16_t my = (int16_t)(112 + (int16_t)((int16_t)(t & 0x1Fu) - 16));
                ng_particle_spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_OPTIONAL,
                                  mx, my, 0, 0, 20u,
                                  spark_tile, spark_pal, 1u, 1u);
            }
            if ((t % 30u) == 0u) {
                ng_particle_spawn(NG_PART_HIT_SPARK, NG_PART_PRI_CRITICAL,
                                  160, 112, 0, 0, 18u,
                                  spark_tile, spark_pal, 1u, 1u);
                playSFX(SOUND_SFX_9);
            }

            digit3(cnt, ng_particles_count());
            demo_fix_puts(2u, 5u, "PART:", 1u);
            demo_fix_puts(8u, 5u, cnt, 2u);
            demo_fix_puts(11u, 5u, "/32", 1u);
        }

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
    const uint8_t  spark_id   = s_fx_effect_frames[0];
    const uint16_t spark_tile = DEMO_SCREEN_TILE(spark_id);
    const uint8_t  spark_pal  = DEMO_SCREEN_PALETTE(spark_id);
    uint16_t t;
    uint8_t  fired = 0u;

    chap_header(10u, "FEEDBACK", "SHAKE  PARTICLE IMPACTS");
    /* Hard wipe of the lower hero's strip window before the first
     * feedback shake/impact so no stale strips ride the bob offset. */
    ng_sprite_park_off_range(HERO_SLOT_FIRST, 16u);
    demo_fix_puts(2u, 2u, "WARRIOR TAKES HITS", 1u);
    demo_fix_puts(2u, 3u, "4 INTENSITIES OVER 9 SECONDS",     0u);
    snd_cross_to(SOUND_MUSIC_F);

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 320, 224);

    hero_scale(U_SCALE_60);
    hero_place(160, 134);
    demo_load_screen_palette(spark_id);

    s_draw_particles = 1u;

    for (t = 0u; t < 540u; t++) {
        uint8_t fx_frame = s_fx_effect_frames[(t / 8u) & 3u];
        uint8_t hero_frame = s_fx_char_frames[(t / 14u) % 5u];
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
        draw_asset_bottom_center(fx_frame, 124u,
                                 (int16_t)(60 + (int16_t)((t * 3u) % 220u)),
                                 92, U_SCALE_60, U_SCALE_60);
        draw_asset_bottom_center(hero_frame, HERO_SLOT_FIRST,
                                 s_hero_x, FX_HERO_LIFT_Y, U_SCALE_60, U_SCALE_60);
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
    int16_t z = 96;
    int16_t dz = -1;
    uint16_t t;

    chap_header(11u, "DEPTH FX", "ONE WARRIOR  CLEAN Z SCALE");
    demo_fix_puts(2u, 2u, "CENTERED SPRITE APPROACHES AND RECEDES", 1u);
    demo_fix_puts(2u, 3u, "NO PALETTE ROTATION  NO EXTRA STRIPES", 0u);
    snd_cross_to(SOUND_MUSIC_C);
    draw_background(U_BG_FOREST, 32, 16);

    ng_depthfx_init();
    demo_fix_puts(2u, 24u, "Z:", 1u);
    demo_fix_puts(10u, 24u, "SCALE:", 1u);

    for (t = 0u; t < 720u; t++) {
        char buf[4];
        uint8_t frame = s_hero_stand[(t / 12u) % 8u];
        uint8_t strips = demo_screen_strips(frame);
        uint8_t rows = demo_screen_rows(frame);
        uint8_t scale = (uint8_t)(0x58u + (uint16_t)((96 - z) * 0xA7u) / 84u);
        int16_t draw_x = (int16_t)(160 - (strips * 16 * scale / 256) / 2
                                  - demo_screen_x_offset(frame));
        int16_t draw_y = (int16_t)(132 - (rows * 16 * scale / 256) / 2
                                  - demo_screen_y_offset(frame));

        z = (int16_t)(z + dz);
        if (z <= 12) dz = 1;
        if (z >= 96) dz = -1;

        digit3(buf, (uint16_t)z);
        demo_fix_puts(5u, 24u, buf, 2u);
        digit3(buf, (uint16_t)scale);
        demo_fix_puts(17u, 24u, buf, 2u);

        demo_draw_sprite_screen(frame, HERO_SLOT_FIRST,
                                draw_x, draw_y,
                                strips, rows, scale, scale);

        if ((t % 90u) == 0u) {
            playSFX(SOUND_SFX_5);
        }

        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 22 — Depth Parallax (two-speed scrolling background layers)  */
/* ================================================================== */
/*
 * Two background layers scrolled at different rates for a genuine
 * depth illusion, distinct from chap_depthfx/chap_garden3d's per-strip
 * Z-scale approach.  Uses demo_draw_sprite_screen() — the same queued,
 * vblank-deferred primitive draw_scrolling_background() (used by the
 * already-live chap_camera/chap_scrolling_level) is built on — with the
 * same two-copy wraparound trick, once per layer at a different rate.
 *
 * An earlier version of this chapter drove NGSpriteGroup directly
 * (ng_sprite_group_set_pos + flush) every frame, ported verbatim from
 * demo_camera.c's cam_parallax_demo() - untested dead code that had
 * never actually run.  That call path writes SCB1-4 to VRAM immediately,
 * during active video, which is the exact "sprite tearing" class of bug
 * demo.c's demo_draw_sprite_screen queue was written to eliminate (see
 * the comment above demo_flush_sprite_queue() in demo.c) - and it
 * produced visible flicker once this chapter was wired into the live
 * reel.  Routing through the queue instead avoids that entirely.
 */
static uint8_t NEOGEO_USER chap_depth_parallax(void)
{
    enum {
        SLOT_FAR_A  = NG_SPR_BG0_FIRST,   /* far layer, primary copy   */
        SLOT_FAR_B  = NG_SPR_BG1_FIRST,   /* far layer, wrap copy      */
        SLOT_NEAR_A = 40,                 /* near layer, primary copy */
        SLOT_NEAR_B = 56                  /* near layer, wrap copy    */
    };
    uint8_t far_strips  = demo_screen_strips(1u);
    uint8_t far_rows    = demo_screen_rows(1u);
    uint8_t near_strips = demo_screen_strips(2u);
    uint8_t near_rows   = demo_screen_rows(2u);
    int16_t far_x  = 0;
    int16_t near_x = 0;
    uint16_t t;

    chap_header(22u, "DEPTH PARALLAX", "TWO SCROLLING BG LAYERS");
    demo_fix_puts(2u, 2u, "FAR BG LAYER MOVES AT 0.5X", 1u);
    demo_fix_puts(2u, 3u, "NEAR BG LAYER MOVES AT 1.0X", 0u);
    snd_cross_to(SOUND_MUSIC_A);

    for (t = 0u; t < 480u; t++) {
        far_x  = (int16_t)(far_x  - 1);
        near_x = (int16_t)(near_x - 2);
        while (far_x  >  32) far_x  = (int16_t)(far_x  - 256);
        while (far_x  < -192) far_x = (int16_t)(far_x  + 256);
        while (near_x >  32) near_x = (int16_t)(near_x - 256);
        while (near_x < -192) near_x = (int16_t)(near_x + 256);

        demo_draw_sprite_screen(1u, SLOT_FAR_A, far_x, 0,
                                far_strips, far_rows, 0xFFu, 0xFFu);
        demo_draw_sprite_screen(1u, SLOT_FAR_B, (int16_t)(far_x + 256), 0,
                                far_strips, far_rows, 0xFFu, 0xFFu);

        demo_draw_sprite_screen(2u, SLOT_NEAR_A, near_x, 0,
                                near_strips, near_rows, 0xFFu, 0xFFu);
        demo_draw_sprite_screen(2u, SLOT_NEAR_B, (int16_t)(near_x + 256), 0,
                                near_strips, near_rows, 0xFFu, 0xFFu);

        if ((t % 90u) == 0u) playSFX(SOUND_SFX_5);
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
    static const int16_t home_x[N] = { 78, 132, 188, 242 };
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

        bind_character_asset(c, asset, U_SCALE_45, U_SCALE_45);
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
            bind_character_asset(c, asset, U_SCALE_45, U_SCALE_45);
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
    const int16_t  GROUND_Y    = 204;
    const uint16_t TOTAL       = 1800u;
    const uint16_t AGGRO_AFTER = 45u;
    const int16_t  REACH_PX    = 70;
    const uint8_t  CLONE_SLOT  = ENEMY_SLOT_FIRST;

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
    uint16_t inactive   = 0u;

    /* Clone state — mirrors player frame arrays */
    int16_t  c_x         = 210;
    uint8_t  c_state     = CLONE_AGGRO;
    uint8_t  c_t         = 0u;
    uint8_t  c_flip      = 1u;   /* faces left initially */
    uint8_t  c_strike_landed = 0u;  /* edge flag for one-shot hit per swing */

    char buf[6];

    chap_header(13u, "MINI-GAME", "FIGHT THE CLONE  B STRIKE");
    demo_fix_puts(2u, 2u, "CP CLOSES IN, STRIKES, RECOVERS", 1u);
    demo_fix_puts(2u, 3u, "PLAYER CAN STILL WALK + B",       0u);
    snd_cross_to(SOUND_MUSIC_D);

    draw_background(U_BG_FOREST, 32, 16);
    hero_scale(U_SCALE_57);
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

        if (down & (JOY_UP | JOY_DOWN | JOY_LEFT | JOY_RIGHT |
                    BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_D)) {
            inactive = 0u;
        } else if (inactive < 900u) {
            inactive++;
        }
        if (inactive >= 900u) return 0u;

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
                inactive = 0u;
                playSFX(SOUND_SFX_8);
                spawn_impact_burst(c_x, (int16_t)(GROUND_Y - 62),
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
         * CLONE LOGIC
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
            if (dx > REACH_PX) { c_x -= 2; c_flip = 1u; }
            else if (dx < -REACH_PX) { c_x += 2; c_flip = 0u; }
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
                    inactive = 0u;
                    spawn_impact_burst(p_x, (int16_t)(GROUND_Y - 62),
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

        demo_fix_puts(0u, 9u, "                                        ", 0u);
        demo_fix_puts((uint8_t)(p_x / 8), 9u, "P1", 2u);
        demo_fix_puts((uint8_t)(c_x / 8), 9u, "CP", 1u);

        /* ============================================================
         * Render clone first, then the player.  The player window uses
         * the lower front slot so it stays over the clone and backdrop.
         * ============================================================ */
        draw_asset_bottom_center_flip(c_frame, CLONE_SLOT,
                                      c_x, GROUND_Y,
                                      U_SCALE_57, U_SCALE_57, c_flip);
        draw_asset_bottom_center_flip(p_frame, HERO_SLOT_FIRST,
                                      p_x, GROUND_Y,
                                      U_SCALE_57, U_SCALE_57, p_flip);

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
    uint16_t inactive  = 0u;
    char buf[8];

    chap_header(14u, "JOYSTICK", "LIVE INPUT  TWO-BUTTON SPECIALS");
    demo_fix_puts(2u, 2u, "ARROWS MOVE  B STRIKE  C JUMP",   1u);
    demo_fix_puts(2u, 3u, "B+C TOGETHER SPECIAL  B+D FINISHER", 0u);
    snd_cross_to(SOUND_MUSIC_A);

    ng_joystick_init();
    hero_scale(U_SCALE_70);

    /* Static HUD labels — once */
    demo_fix_puts(2u,  5u, "PAD:",        2u);
    demo_fix_puts(2u,  6u, "BTN:",        2u);
    demo_fix_puts(2u,  7u, "HELD A:",     2u);
    demo_fix_puts(2u,  8u, "HELD B:",     2u);
    demo_fix_puts(2u,  9u, "HELD C:",     2u);
    demo_fix_puts(2u, 10u, "HELD D:",     2u);
    demo_fix_puts(2u, 12u, "B+C:",        2u);   /* light special */
    demo_fix_puts(2u, 13u, "B+D:",        2u);   /* heavy finisher */
    demo_fix_puts(2u, 25u, "HIT BOX ON RIGHT  B STRIKE",        0u);
    demo_fix_puts(2u, 26u, "HITS:",                            2u);

    /* Static FIX target hitbox. */
    {
        uint8_t r, c;
        for (r = 0u; r < 4u; r++) {
            for (c = 0u; c < 6u; c++) {
                demo_fix_puts((uint8_t)(32u + c),
                              (uint8_t)(12u + r),
                              (r == 0u || r == 3u || c == 0u || c == 5u) ? "#" : ".",
                              (uint8_t)(1u + ((c + r) & 1u)));
            }
        }
    }

    draw_background(U_BG_FOREST, 32, 16);

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
        if (down & (JOY_UP | JOY_DOWN | JOY_LEFT | JOY_RIGHT |
                    BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_D)) {
            inactive = 0u;
        } else if (inactive < 900u) {
            inactive++;
        }
        if (inactive >= 900u) return 0u;
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
                inactive = 0u;
                playSFX(SOUND_SFX_8);
                /* Visual: re-draw the target with a flash palette this frame */
                {
                    uint8_t r, c;
                    for (r = 0u; r < 4u; r++) {
                        for (c = 0u; c < 6u; c++) {
                            demo_fix_puts((uint8_t)(32u + c),
                                          (uint8_t)(12u + r),
                                          (r == 0u || r == 3u || c == 0u || c == 5u) ? "#" : ".",
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
            for (r = 0u; r < 4u; r++) {
                for (c = 0u; c < 6u; c++) {
                    demo_fix_puts((uint8_t)(32u + c),
                                  (uint8_t)(12u + r),
                                  (r == 0u || r == 3u || c == 0u || c == 5u) ? "#" : ".",
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
    static const uint8_t s_level_bg[4] = {
        U_BG_FOREST, U_BG_FOREST, U_BG_FOREST, U_BG_FOREST
    };

    chap_header(15u, "SCROLL LEVEL", "WORLD MAP  H/V STAGES");
    demo_fix_puts(2u, 2u, "RUN ACROSS LEVELS, THEN CLIMB", 1u);
    demo_fix_puts(2u, 3u, "BG SWAPS AT EACH LEVEL GATE",   0u);
    snd_cross_to(SOUND_MUSIC_B);

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, 768, 288);
    ng_camera_set_follow_speed(&cam, 12u);
    ng_camera_set_dead_zone(&cam, 24u, 16u);

    hero_scale(U_SCALE_70);
    hero_place(160, 112);
    demo_fix_puts(2u, 24u, "LEVEL 1  HP [####################]", 1u);
    demo_fix_puts(2u, 25u, "GATE: HORIZONTAL ROAD", 2u);

    for (t = 0u; t < 780u; t++) {
        uint8_t level = (uint8_t)(1u + (t / 195u));
        uint8_t bg = s_level_bg[(level - 1u) & 3u];
        int16_t bg_x;
        int16_t bg_y;
        int16_t jump = 0;
        uint8_t frame;
        int16_t world_x = (int16_t)(40 + (int16_t)((t * 2u) % 640u));
        char level_text[2];

        if ((t % 180u) > 50u && (t % 180u) < 126u) {
            uint16_t jt = (uint16_t)((t % 180u) - 50u);
            jump = (int16_t)((jt < 38u) ? jt : (76u - jt));
            frame = s_hero_specA[(t / 4u) % 8u];
        } else {
            frame = s_hero_walk[(t / 5u) % 8u];
        }

        ng_camera_update(&cam, world_x, U_FLOOR_Y, 2);
        if (level < 3u) {
            bg_x = -(int16_t)((uint16_t)cam.x & 0x00FFu);
            draw_scrolling_background(bg, bg_x, 0);
            demo_fix_puts(2u, 25u, "GATE: HORIZONTAL ROAD", 2u);
        } else {
            bg_y = -(int16_t)(((t - 390u) / 4u) & 31u);
            draw_vertical_background(bg, 32, bg_y);
            demo_fix_puts(2u, 25u, "GATE: VERTICAL PASS   ", 2u);
        }

        level_text[0] = (char)('0' + level);
        level_text[1] = '\0';
        demo_fix_puts(8u, 24u, level_text, 2u);

        /* Hero stays at a fixed screen X — camera shows the world scroll */
        s_hero_x = 160;
        s_hero_y = (int16_t)((level < 3u ? 112 : 132) - jump);
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

static void NEOGEO_USER raytrace_put(uint8_t x,
                                     uint8_t y,
                                     uint16_t tile,
                                     uint8_t pal)
{
    if (x < 40u && y < 28u) {
        ngfix_write_tile(x, y, tile, pal);
    }
}

/*
 * Static one-point-perspective "target range" frame: an outer and
 * inner rectangle joined by converging corner lines plus a couple of
 * floor rungs, drawn ONCE.
 *
 * This replaces an earlier per-frame raycaster that walked a 16x16
 * tile map and redrew all 40 FIX columns every frame.  No matter how
 * the shading ramp was tuned, a live-recalculated ASCII maze read as
 * flat and blocky rather than an actual 3D corridor.  A still
 * wireframe backdrop reads as an intentional "shooting range" set
 * instead - the depth cue now comes entirely from the sprite-scaled
 * targets approaching through it, the same proven SCB2-shrink
 * technique chap_garden3d already uses successfully.
 */
static void NEOGEO_USER draw_range_frame(void)
{
    enum {
        OUT_L = 2, OUT_R = 37, OUT_T = 4, OUT_B = 23,
        IN_L = 15, IN_R = 24, IN_T = 10, IN_B = 17,
        STEPS = 6
    };
    uint8_t x, y, k;

    for (x = OUT_L; x <= OUT_R; x++) {
        raytrace_put(x, OUT_T, '-', 1u);
        raytrace_put(x, OUT_B, '-', 1u);
    }
    for (y = OUT_T; y <= OUT_B; y++) {
        raytrace_put(OUT_L, y, '|', 1u);
        raytrace_put(OUT_R, y, '|', 1u);
    }

    for (x = IN_L; x <= IN_R; x++) {
        raytrace_put(x, IN_T, '-', 2u);
        raytrace_put(x, IN_B, '-', 2u);
    }
    for (y = IN_T; y <= IN_B; y++) {
        raytrace_put(IN_L, y, '|', 2u);
        raytrace_put(IN_R, y, '|', 2u);
    }

    for (k = 1u; k < STEPS; k++) {
        uint8_t xl = (uint8_t)(OUT_L + ((IN_L - OUT_L) * k) / STEPS);
        uint8_t xr = (uint8_t)(OUT_R - ((OUT_R - IN_R) * k) / STEPS);
        uint8_t yt = (uint8_t)(OUT_T + ((IN_T - OUT_T) * k) / STEPS);
        uint8_t yb = (uint8_t)(OUT_B - ((OUT_B - IN_B) * k) / STEPS);
        raytrace_put(xl, yt, '\\', 1u);
        raytrace_put(xr, yt, '/',  1u);
        raytrace_put(xl, yb, '/',  1u);
        raytrace_put(xr, yb, '\\', 1u);
    }

    for (k = 1u; k < 3u; k++) {
        uint8_t y2 = (uint8_t)(IN_B + ((OUT_B - IN_B) * k) / 3u);
        uint8_t xl = (uint8_t)(IN_L - ((IN_L - OUT_L) * k) / 3u);
        uint8_t xr = (uint8_t)(IN_R + ((OUT_R - IN_R) * k) / 3u);
        for (x = xl; x <= xr; x++) {
            if (((uint8_t)(x + y2) & 3u) == 0u) raytrace_put(x, y2, '.', 1u);
        }
    }
}

/* ================================================================== */
/*  Chapter 16 — Raytrace 3D                                            */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_raytrace3d(void)
{
    enum {
        TARGET_COUNT = 4,
        Z_NEAR = 16,
        Z_FAR = 112,
        RETICLE_MIN = 60,
        RETICLE_MAX = 260
    };
    static const int16_t lane_x[TARGET_COUNT] = { -72, -26, 30, 78 };
    static const uint8_t target_frame[TARGET_COUNT] = { 49u, 50u, 51u, 53u };
    int16_t target_z[TARGET_COUNT] = { 36, 62, 88, 108 };
    uint8_t target_flash[TARGET_COUNT] = { 0u, 0u, 0u, 0u };
    int16_t reticle_x = 160;
    uint8_t fire_timer = 0u;
    uint8_t ammo = 24u;
    uint16_t score = 0u;
    uint16_t t;
    uint8_t i;
    char buf[4];

    chap_header(16u, "TARGET RANGE", "PERSPECTIVE FRAME  SPRITE DEPTH");
    demo_fix_puts(2u, 2u, "D-PAD AIM  B FIRE", 1u);
    demo_fix_puts(2u, 3u, "STATIC FRAME + SPRITE-SCALED TARGETS", 0u);
    snd_cross_to(SOUND_MUSIC_F);

    for (i = 0u; i < TARGET_COUNT; i++) {
        demo_load_screen_palette(target_frame[i]);
    }

    /* AMMO/SCORE/LOCK are the only values this chapter actually tracks -
     * a previous static "HP [########]" bar never moved (no damage
     * mechanic exists here), which read as an unfinished placeholder.
     * Row 26 is the shared separator line chap_header() just drew;
     * leave it alone instead of overwriting it with a redundant title
     * (the caption bar on row 27 already names this chapter). */
    demo_fix_puts(2u, 24u, "AMMO:", 1u);
    demo_fix_puts(2u, 25u, "SCORE:", 0u);
    draw_range_frame();
    ng_joystick_init();

    for (t = 0u; t < 900u; t++) {
        uint16_t down;
        uint16_t pressed;
        uint8_t reticle_col;
        int8_t locked_idx = -1;
        int16_t best_dx = 32000;
        uint8_t is_locked;

        ng_joystick_update();
        down = ng_joy_down();
        pressed = ng_joy_pressed();

        if (down & JOY_LEFT) {
            reticle_x = (int16_t)((reticle_x > RETICLE_MIN + 3) ? reticle_x - 3 : RETICLE_MIN);
        }
        if (down & JOY_RIGHT) {
            reticle_x = (int16_t)((reticle_x < RETICLE_MAX - 3) ? reticle_x + 3 : RETICLE_MAX);
        }
        reticle_col = (uint8_t)(reticle_x >> 3);

        for (i = 0u; i < TARGET_COUNT; i++) {
            int16_t sx;
            int16_t dx;
            if (target_flash[i]) continue;
            sx = (int16_t)(160 + ((lane_x[i] * (Z_FAR - target_z[i])) /
                                  (Z_FAR - Z_NEAR)));
            dx = (int16_t)(sx - reticle_x);
            if (dx < 0) dx = (int16_t)-dx;
            if (dx < best_dx) {
                best_dx = dx;
                locked_idx = (int8_t)i;
            }
        }
        is_locked = (uint8_t)(locked_idx >= 0 && best_dx < 28);

        if ((pressed & BUTTON_B) && ammo > 0u) {
            fire_timer = 6u;
            ammo--;
            playSFX(SOUND_SFX_7);
            if (is_locked) {
                target_flash[locked_idx] = 12u;
                target_z[locked_idx] = Z_FAR;
                score = (uint16_t)((score < 975u) ? score + 25u : 999u);
                playSFX(SOUND_SFX_8);
            }
        }

        raytrace_put((uint8_t)(reticle_col - 1u), 13u, '[', is_locked ? 2u : 1u);
        raytrace_put(reticle_col, 13u, '+', is_locked ? 2u : 1u);
        raytrace_put((uint8_t)(reticle_col + 1u), 13u, ']', is_locked ? 2u : 1u);

        if (fire_timer) {
            raytrace_put(18u, 22u, '/', 2u);
            raytrace_put(19u, 21u, '/', 2u);
            raytrace_put(20u, 20u, '*', 2u);
            raytrace_put(21u, 21u, '\\', 2u);
            raytrace_put(22u, 22u, '\\', 2u);
        } else {
            raytrace_put(18u, 22u, NGFIX_DEFAULT_BLANK_TILE, 0u);
            raytrace_put(19u, 21u, NGFIX_DEFAULT_BLANK_TILE, 0u);
            raytrace_put(20u, 20u, NGFIX_DEFAULT_BLANK_TILE, 0u);
            raytrace_put(21u, 21u, NGFIX_DEFAULT_BLANK_TILE, 0u);
            raytrace_put(22u, 22u, NGFIX_DEFAULT_BLANK_TILE, 0u);
        }

        for (i = 0u; i < TARGET_COUNT; i++) {
            uint8_t scale;
            int16_t sx;
            int16_t bottom_y;

            if (target_flash[i]) {
                draw_asset_bottom_center(s_fx_effect_frames[(target_flash[i] >> 1) & 3u],
                                         (uint16_t)(224u + i * 12u),
                                         (int16_t)(160 + lane_x[i] / 3),
                                         130,
                                         U_SCALE_45,
                                         U_SCALE_45);
                target_flash[i]--;
                continue;
            }

            target_z[i] = (int16_t)(target_z[i] - 1);
            if (target_z[i] < Z_NEAR) target_z[i] = Z_FAR;

            sx = (int16_t)(160 + ((lane_x[i] * (Z_FAR - target_z[i])) /
                                  (Z_FAR - Z_NEAR)));
            bottom_y = (int16_t)(116 + (((Z_FAR - target_z[i]) * 64) /
                                        (Z_FAR - Z_NEAR)));
            /* Capped well below native size (was up to ~83% at closest
             * approach) so a close target no longer blots out most of
             * the FIX-layer raycast scene behind it. */
            scale = (uint8_t)(0x20u + (((uint16_t)(Z_FAR - target_z[i]) * 0x50u) /
                                        (Z_FAR - Z_NEAR)));
            draw_asset_bottom_center(target_frame[i],
                                     (uint16_t)(128u + i * 20u),
                                     sx,
                                     bottom_y,
                                     scale,
                                     scale);
        }

        digit3(buf, score);
        demo_fix_puts(8u, 25u, buf, 1u);
        digit3(buf, ammo);
        demo_fix_puts(8u, 24u, buf, 2u);
        demo_fix_puts(17u, 25u, is_locked ? "LOCK" : "--- ", is_locked ? 2u : 0u);

        if (fire_timer) fire_timer--;

        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 17 — 2D render idea  (light per-frame churn)                 */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_char_2d(void)
{
    uint16_t t;

    chap_header(17u, "CHAR 2D", "PATHS  EASE  FRAME BIND");
    demo_fix_puts(2u, 2u, "EAGLE FLYING FRAME BIND", 1u);
    demo_fix_puts(2u, 3u, "076 -> 077 -> 078 AT 70%", 0u);
    snd_cross_to(SOUND_MUSIC_G);

    draw_background(U_BG_FOREST, 32, 16);
    demo_fix_puts(5u, 23u, "[------------------------------]", 1u);

    for (t = 0u; t < 600u; t++) {
        uint16_t p = (uint16_t)(t % 300u);
        uint8_t frame = s_flight_frames[(t / 8u) % 3u];
        int16_t x;
        int16_t y;
        uint8_t k;
        char bar[31];

        if (t < 200u) {
            demo_fix_puts(2u, 6u, "PHASE: ARC WALK        ", 2u);
        } else if (t < 420u) {
            demo_fix_puts(2u, 6u, "PHASE: HOP AND RECOVER ", 2u);
        } else {
            demo_fix_puts(2u, 6u, "PHASE: READY LOOP      ", 2u);
        }

        if (p < 150u) {
            x = (int16_t)(64 + ((uint16_t)p * 192u) / 150u);
        } else {
            x = (int16_t)(256 - (((uint16_t)(p - 150u) * 192u) / 150u));
        }
        y = (int16_t)(186 - (int16_t)((p < 80u) ? (p >> 2)
                                                : ((p < 160u) ? ((160u - p) >> 2) : 0u)));

        for (k = 0u; k < 30u; k++) {
            bar[k] = (k < (uint8_t)((p * 30u) / 300u)) ? '#' : '-';
        }
        bar[30] = '\0';
        demo_fix_puts(6u, 23u, bar, 2u);

        demo_load_screen_palette(frame);
        draw_asset_bottom_center(frame, HERO_SLOT_FIRST,
                                 x, y, U_SCALE_45, U_SCALE_45);

        if ((t % 60u) == 0u) playSFX(SOUND_SFX_5);
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 18 — SSG arcade (vblank-spaced Z80 setup)                    */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_image_shooter(void)
{
    enum {
        FORM_ROWS = 4,
        FORM_COLS = 6,
        SHOOTER_ENEMIES = FORM_ROWS * FORM_COLS,   /* 24 - Galaxian-style grid */
        DIVER_MAX    = 2,      /* concurrent divers detached from formation */
        PBULLET_MAX  = 3,
        EBULLET_MAX  = 4,
        BOOM_MAX     = 3,
        ENTRY_FRAMES = 60,     /* formation fly-in duration at wave start */
        STAGE_MAX    = 3,
        SHOOTER_SLOT_ENEMY   = 96,   /* 24 * 4 strips = 96 slots -> 96..191 */
        SHOOTER_SLOT_PLAYER  = 192,  /* 4 strips      -> 192..195 */
        SHOOTER_SLOT_BOOM    = 196,  /* 3 * 4 strips  -> 196..207 */
        SHOOTER_SLOT_PBULLET = 208,  /* 3 * 2 strips  -> 208..213 */
        SHOOTER_SLOT_EBULLET = 214,  /* 4 * 2 strips  -> 214..221 */
        SHOOTER_TIME = 1200
    };
    /* Back row (row 0) is worth the most, matching classic Galaxian
     * scoring where the deeper formation rows are harder to clear a
     * path to. */
    static const uint16_t row_score[FORM_ROWS] = { 150u, 120u, 90u, 60u };

    int16_t home_x[SHOOTER_ENEMIES];
    int16_t home_y[SHOOTER_ENEMIES];
    uint8_t enemy_alive[SHOOTER_ENEMIES];
    int16_t enemy_x[SHOOTER_ENEMIES];
    int16_t enemy_y[SHOOTER_ENEMIES];

    int16_t pb_x[PBULLET_MAX], pb_y[PBULLET_MAX];
    uint8_t pb_active[PBULLET_MAX];
    int16_t eb_x[EBULLET_MAX], eb_y[EBULLET_MAX];
    uint8_t eb_active[EBULLET_MAX];
    int16_t boom_x[BOOM_MAX], boom_y[BOOM_MAX];
    uint8_t boom_timer[BOOM_MAX];

    enum { IDLE_ADVANCE_FRAMES = 300u };  /* ~5s idle once player-controlled */
    int16_t ship_x = 160;
    uint16_t prev_joy = 0u;
    uint8_t wave = 1u;
    uint8_t stage = 1u;
    uint8_t lives = 3u;
    uint16_t score = 0u;
    uint16_t t;
    uint16_t wave_t = 0u;
    uint16_t dive_interval;
    uint16_t idle_frames = 0u;
    uint8_t player_controlled = 0u;
    uint8_t i, j;
    char buf[8];

    chap_header(18u, "SSG ARCADE", "GALAXIAN FORMATION MINI");
    /* Plays itself by default (attract-mode style) - press START to
     * take manual control for the rest of the chapter.  Row 2 is
     * updated live below once control state can change. */
    demo_fix_puts(2u, 2u, "WATCHING - PRESS START TO PLAY", 1u);
    demo_fix_puts(2u, 3u, "IMAGE SPRITES + SSG + ADPCM", 0u);

    /* Blank space backdrop - the forest background didn't fit a space
     * shooter; chap_header() already leaves the backdrop black. */
    demo_load_screen_palette(U_SHOOTER_ENEMY);
    demo_load_screen_palette(U_SHOOTER_ENEMY_BULLET);
    demo_load_screen_palette(U_SHOOTER_EXPLOSION);
    demo_load_screen_palette(U_SHOOTER_PLAYER_BULLET);
    demo_load_screen_palette(U_SHOOTER_SHIP);
    demo_load_screen_palette(U_SHOOTER_SHIP_ALT);
    demo_load_screen_palette(U_UFO_BLUE);
    demo_load_screen_palette(U_UFO_GREEN);
    demo_load_screen_palette(U_UFO_PINK);
    demo_load_screen_palette(U_UFO_YELLOW);

    /* SSG-only music - the ADPCM-B game-loop track was playing prominent
     * (0xB8) alongside SSG at near-silent (0x05), so SSG never actually
     * read as the music.  Dropped the ADPCM-B loop and gave SSG the
     * volume instead. */
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x34u, 0x00u, 0x0Eu, 0x00u); snd_step();
    playSSGTrack(SOUND_SSG_B);                 snd_step();

    for (i = 0u; i < SHOOTER_ENEMIES; i++) {
        uint8_t row = (uint8_t)(i / FORM_COLS);
        uint8_t col = (uint8_t)(i % FORM_COLS);
        home_x[i] = (int16_t)(60 + col * 40);
        home_y[i] = (int16_t)(40 + row * 22);
        enemy_alive[i] = 1u;
        enemy_x[i] = home_x[i];
        enemy_y[i] = home_y[i];
    }
    for (j = 0u; j < PBULLET_MAX; j++) pb_active[j] = 0u;
    for (j = 0u; j < EBULLET_MAX; j++) eb_active[j] = 0u;
    for (j = 0u; j < BOOM_MAX; j++) boom_timer[j] = 0u;

    for (t = 0u; t < SHOOTER_TIME; t++) {
        uint16_t joy = poll_joystick();
        /* Was uint8_t, which silently truncated away the high byte where
         * START1/START2 live - no press on those bits could ever be
         * detected here regardless of what checked for it. */
        uint16_t edge = (uint16_t)(joy & (uint16_t)(~prev_joy));
        uint8_t alive_count = 0u;
        uint8_t entering = (wave_t < ENTRY_FRAMES) ? 1u : 0u;
        uint16_t settled_t = entering ? 0u : (uint16_t)(wave_t - ENTRY_FRAMES);
        int16_t sway = (int16_t)((int16_t)((settled_t >> 2) & 31u) - 15);
        uint8_t diver[DIVER_MAX];

        dive_interval = (uint16_t)(160u - (uint16_t)(stage - 1u) * 20u);
        for (j = 0u; j < DIVER_MAX; j++) {
            uint16_t phase = (uint16_t)(settled_t + (uint16_t)j * (dive_interval / DIVER_MAX));
            diver[j] = (uint8_t)((phase / dive_interval) % SHOOTER_ENEMIES);
        }

        prev_joy = joy;

        if (!player_controlled && (edge & (START1 | START2))) {
            player_controlled = 1u;
            idle_frames = 0u;
            demo_fix_puts(2u, 2u, "B FIRE  ARROWS MOVE           ", 1u);
        }

        if (player_controlled) {
            if (joy) idle_frames = 0u;
            else if (idle_frames < 0xFFFFu) idle_frames++;
            if (idle_frames >= IDLE_ADVANCE_FRAMES) return 1u;

            if ((joy & JOY_LEFT) && ship_x > 54) ship_x = (int16_t)(ship_x - 3);
            if ((joy & JOY_RIGHT) && ship_x < 266) ship_x = (int16_t)(ship_x + 3);
        } else {
            /* Simple attract-mode AI: steer toward whichever enemy is
             * currently diving (the immediate threat), or the first
             * surviving enemy if none are - firing is handled by the
             * existing periodic auto-fire below, shared with manual
             * play, so no separate AI fire logic is needed. */
            int16_t target_x = ship_x;
            uint8_t found = 0u;
            for (j = 0u; j < DIVER_MAX; j++) {
                if (enemy_alive[diver[j]]) {
                    target_x = enemy_x[diver[j]];
                    found = 1u;
                    break;
                }
            }
            if (!found) {
                for (i = 0u; i < SHOOTER_ENEMIES; i++) {
                    if (enemy_alive[i]) { target_x = enemy_x[i]; break; }
                }
            }
            if (target_x < (int16_t)(ship_x - 3) && ship_x > 54) ship_x = (int16_t)(ship_x - 3);
            else if (target_x > (int16_t)(ship_x + 3) && ship_x < 266) ship_x = (int16_t)(ship_x + 3);
        }

        if ((edge & BUTTON_B) || ((t & 31u) == 20u)) {
            for (j = 0u; j < PBULLET_MAX; j++) {
                if (!pb_active[j]) {
                    pb_active[j] = 1u;
                    pb_x[j] = ship_x;
                    pb_y[j] = 158;
                    playSFX(SOUND_SFX_7);
                    break;
                }
            }
        }
        for (j = 0u; j < PBULLET_MAX; j++) {
            if (!pb_active[j]) continue;
            pb_y[j] = (int16_t)(pb_y[j] - 5);
            if (pb_y[j] < 28) pb_active[j] = 0u;
        }

        if (!entering && (wave_t % dive_interval) == 30u) {
            for (j = 0u; j < DIVER_MAX; j++) {
                uint8_t idx = diver[j];
                uint8_t k;
                uint8_t slot_found = 0xFFu;
                if (!enemy_alive[idx]) continue;
                for (k = 0u; k < EBULLET_MAX; k++) {
                    if (!eb_active[k]) { slot_found = k; break; }
                }
                if (slot_found != 0xFFu) {
                    eb_active[slot_found] = 1u;
                    eb_x[slot_found] = enemy_x[idx];
                    eb_y[slot_found] = (int16_t)(enemy_y[idx] + 22);
                }
            }
        }
        for (j = 0u; j < EBULLET_MAX; j++) {
            if (!eb_active[j]) continue;
            eb_y[j] = (int16_t)(eb_y[j] + 3);
            if (eb_y[j] > 190) { eb_active[j] = 0u; continue; }
            if (eb_y[j] > 158 && eb_y[j] < 184 &&
                eb_x[j] > (int16_t)(ship_x - 18) &&
                eb_x[j] < (int16_t)(ship_x + 18)) {
                eb_active[j] = 0u;
                if (lives > 0u) lives--;
                for (i = 0u; i < BOOM_MAX; i++) {
                    if (boom_timer[i] == 0u) {
                        boom_x[i] = ship_x;
                        boom_y[i] = 166;
                        boom_timer[i] = 32u;
                        break;
                    }
                }
                playSFX(SOUND_SFX_10);
            }
        }

        for (i = 0u; i < SHOOTER_ENEMIES; i++) {
            uint8_t is_diver = 0u;

            if (entering) {
                enemy_x[i] = home_x[i];
                enemy_y[i] = (int16_t)(-30 + ((int32_t)(home_y[i] + 30) * wave_t) / ENTRY_FRAMES);
            } else {
                int16_t y_wave = (int16_t)((i & 1u) ? ((settled_t >> 3) & 7u) : -((settled_t >> 3) & 7u));
                enemy_x[i] = (int16_t)(home_x[i] + sway);
                enemy_y[i] = (int16_t)(home_y[i] + y_wave);

                for (j = 0u; j < DIVER_MAX; j++) {
                    if (diver[j] == i) is_diver = 1u;
                }
                if (is_diver && enemy_alive[i]) {
                    uint8_t dive_step = (uint8_t)((settled_t % dive_interval) / 3u);
                    if (dive_step < 32u) {
                        enemy_y[i] = (int16_t)(enemy_y[i] + dive_step);
                        enemy_x[i] = (int16_t)(enemy_x[i] + ((dive_step & 1u) ? dive_step : -dive_step));
                    }
                }
            }

            if (enemy_alive[i]) {
                for (j = 0u; j < PBULLET_MAX; j++) {
                    if (!pb_active[j]) continue;
                    if (pb_y[j] > (int16_t)(enemy_y[i] - 20) &&
                        pb_y[j] < (int16_t)(enemy_y[i] + 18) &&
                        pb_x[j] > (int16_t)(enemy_x[i] - 20) &&
                        pb_x[j] < (int16_t)(enemy_x[i] + 20)) {
                        uint8_t row = (uint8_t)(i / FORM_COLS);
                        uint8_t k;
                        enemy_alive[i] = 0u;
                        pb_active[j] = 0u;
                        for (k = 0u; k < BOOM_MAX; k++) {
                            if (boom_timer[k] == 0u) {
                                boom_x[k] = enemy_x[i];
                                boom_y[k] = enemy_y[i];
                                boom_timer[k] = 24u;
                                break;
                            }
                        }
                        score = (uint16_t)(score + row_score[row]);
                        playSFX(SOUND_SFX_8);
                        break;
                    }
                }
            }

            if (enemy_alive[i]) alive_count++;
        }

        if (alive_count == 0u) {
            wave++;
            if (stage < STAGE_MAX) stage++;
            score = (uint16_t)(score + 500u);
            wave_t = 0u;
            for (i = 0u; i < SHOOTER_ENEMIES; i++) enemy_alive[i] = 1u;
            playSFX(SOUND_SFX_9);
        } else {
            wave_t++;
        }

        demo_fix_puts(2u, 5u, "SCORE", 1u);
        digit3(buf, (uint16_t)(score % 1000u));
        demo_fix_puts(9u, 5u, buf, 2u);
        demo_fix_puts(15u, 5u, "WAVE", 1u);
        digit3(buf, wave);
        demo_fix_puts(21u, 5u, buf, 2u);
        demo_fix_puts(27u, 5u, "LIFE", 1u);
        digit3(buf, lives);
        demo_fix_puts(33u, 5u, buf, 2u);

        /* Wave-clear progress bar. */
        {
            uint8_t done = (uint8_t)(SHOOTER_ENEMIES - alive_count);
            uint8_t p;
            char cell[2];
            cell[1] = '\0';
            for (p = 0u; p < 18u; p++) {
                cell[0] = (p < (uint8_t)((done * 18u) / SHOOTER_ENEMIES)) ? '#' : '-';
                demo_fix_puts((uint8_t)(2u + p), 6u, cell, (uint8_t)(cell[0] == '#' ? 2u : 1u));
            }
        }

        for (i = 0u; i < SHOOTER_ENEMIES; i++) {
            uint16_t slot = (uint16_t)(SHOOTER_SLOT_ENEMY + (uint16_t)i * 4u);
            /* One UFO colour per row - back row (highest row_score) is
             * pink, then blue, green, yellow - instead of the same
             * plain enemy sprite repeated across all 24 formation
             * slots, so the score tiers actually look distinct. */
            static const uint8_t row_ufo[FORM_ROWS] = {
                U_UFO_PINK, U_UFO_BLUE, U_UFO_GREEN, U_UFO_YELLOW
            };
            uint8_t enemy_frame = row_ufo[i / FORM_COLS];
            if (enemy_alive[i]) {
                draw_asset_bottom_center(enemy_frame, slot,
                                         enemy_x[i], (int16_t)(enemy_y[i] + 22),
                                         U_SCALE_55, U_SCALE_55);
            } else {
                draw_asset_bottom_center(enemy_frame, slot,
                                         -220, -220,
                                         U_SCALE_55, U_SCALE_55);
            }
        }

        draw_asset_bottom_center((t & 16u) ? U_SHOOTER_SHIP_ALT : U_SHOOTER_SHIP,
                                 SHOOTER_SLOT_PLAYER,
                                 ship_x, 184,
                                 U_SCALE_60, U_SCALE_60);

        for (j = 0u; j < PBULLET_MAX; j++) {
            uint16_t slot = (uint16_t)(SHOOTER_SLOT_PBULLET + (uint16_t)j * 2u);
            draw_asset_bottom_center(U_SHOOTER_PLAYER_BULLET, slot,
                                     pb_active[j] ? pb_x[j] : -220,
                                     pb_active[j] ? pb_y[j] : -220,
                                     U_SCALE_FULL, U_SCALE_FULL);
        }
        for (j = 0u; j < EBULLET_MAX; j++) {
            uint16_t slot = (uint16_t)(SHOOTER_SLOT_EBULLET + (uint16_t)j * 2u);
            draw_asset_bottom_center(U_SHOOTER_ENEMY_BULLET, slot,
                                     eb_active[j] ? eb_x[j] : -220,
                                     eb_active[j] ? eb_y[j] : -220,
                                     U_SCALE_FULL, U_SCALE_FULL);
        }
        for (j = 0u; j < BOOM_MAX; j++) {
            uint16_t slot = (uint16_t)(SHOOTER_SLOT_BOOM + (uint16_t)j * 4u);
            if (boom_timer[j] > 0u) {
                draw_asset_bottom_center(U_SHOOTER_EXPLOSION, slot,
                                         boom_x[j], (int16_t)(boom_y[j] + 18),
                                         U_SCALE_55, U_SCALE_55);
                boom_timer[j]--;
            } else {
                draw_asset_bottom_center(U_SHOOTER_EXPLOSION, slot,
                                         -220, -220,
                                         U_SCALE_55, U_SCALE_55);
            }
        }

        if ((t & 127u) == 0u) playSFX(SOUND_SFX_5);
        if (uframe()) return 1u;
    }

    soundFadeOutSpeed(8u);
    (void)uwait(12u);
    soundStopAll();
    return 0u;
}

static uint8_t NEOGEO_USER chap_ssg_arcade(void)
{
    return chap_image_shooter();
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
    {
        enum {
            OBJ_COUNT = 3,
            Z_NEAR = 18,
            Z_FAR = 104
        };
        static const uint8_t obj_frame[OBJ_COUNT] = {
            89u, 90u, 91u
        };
        static const int16_t lane_x[OBJ_COUNT] = { -68, 0, 68 };
        int16_t z[OBJ_COUNT] = { 36, 72, 104 };
        uint16_t t;
        uint8_t i;

        chap_header(19u, "DEPTH RIDE", "ROAD DEPTH  SCALE OBJECTS");
        demo_fix_puts(2u, 2u, "BACKGROUND 1 + SMALL DEPTH OBJECTS", 1u);
        demo_fix_puts(2u, 3u, "EAGLE CENTERED UPPER SCREEN 30%", 0u);
        snd_cross_to(SOUND_MUSIC_C);

        draw_background(U_BG_FOREST, 32, 16);
        for (i = 0u; i < OBJ_COUNT; i++) demo_load_screen_palette(obj_frame[i]);
        hero_scale(U_SCALE_30);
        hero_place(160, 96);

        for (t = 0u; t < 840u; t++) {
            uint8_t road;
            uint8_t frame = s_flight_frames[(t / 8u) % 3u];

            for (road = 0u; road < 6u; road++) {
                uint8_t y = (uint8_t)(12u + road * 2u);
                demo_fix_puts((uint8_t)(12u - road), y, "\\", 1u);
                demo_fix_puts((uint8_t)(27u + road), y, "/", 1u);
            }

            for (i = 0u; i < OBJ_COUNT; i++) {
                uint8_t scale;
                uint8_t strips = demo_screen_strips(obj_frame[i]);
                uint8_t rows = demo_screen_rows(obj_frame[i]);
                int16_t sx;
                int16_t sy;
                int16_t w;
                int16_t h;

                z[i] = (int16_t)(z[i] - 1);
                if (z[i] < Z_NEAR) z[i] = Z_FAR;

                scale = (uint8_t)(0x30u + (((uint16_t)(Z_FAR - z[i]) * 0x80u) / (Z_FAR - Z_NEAR)));
                sx = (int16_t)(160 + ((lane_x[i] * (Z_FAR - z[i])) / (Z_FAR - Z_NEAR)));
                sy = (int16_t)(80 + ((Z_FAR - z[i]) * 104) / (Z_FAR - Z_NEAR));
                w = (int16_t)((strips * 16 * scale) >> 8);
                h = (int16_t)((rows * 16 * scale) >> 8);

                demo_draw_sprite_screen(obj_frame[i],
                                        (uint16_t)(110u + i * 24u),
                                        (int16_t)(sx - (w >> 1)),
                                        (int16_t)(sy - h),
                                        strips, rows,
                                        scale, scale);
            }

            hero_draw(frame);
            if ((t & 31u) == 0u) playSFX(SOUND_SFX_5);
            if (uframe()) return 1u;
        }
        return 0u;
    }

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
    draw_background(U_BG_FOREST, 32, 16);

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
    demo_fix_puts(4u, 15u, "RAYTRACE 3D  SSG ARCADE",  1u);


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
     *                          banners with direct blank-tile clears.
     *   PHASE 2 SLIDE-IN     — march a banner across the FIX grid
     *                          by clearing the trailing column with
     *                          ngfix_blank_cell and redrawing.
     *   PHASE 3 PALETTE CYCLE — same banner stamped every frame at
     *                          a rotating palette index, showing how
     *                          cheap colour cycling is on the FIX layer.
     *   PHASE 4 TILE GRID    — multiple infix blocks (different
     *                          source PNGs, different palettes) tiled
     *                          on the same frame.
     */
    /* All metadata comes from INFIX_IMAGES[] (auto-generated by
     * fixtiles.py) so the demo is always in sync with whatever the
     * artist dropped into games/demo/artbox/infix/.  Each photo has
     * its own FIX palette bank uploaded by setup_fix_palettes(). */
    static const uint8_t  BANNER_IDX[4] = { 4u, 7u, 9u, 5u };
    static const char *const BANNER_LBL[4] = {
        "INFIX 4.PNG", "INFIX 7.PNG",
        "INFIX 9.PNG", "INFIX 5.PNG"
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
            const InfixImage *a = &INFIX_IMAGES[BANNER_IDX[b]];
            uint8_t cx = (uint8_t)((40u - a->cols) / 2u);
            uint8_t cy = (uint8_t)(10u + ((6u - a->rows) >> 1));

            clear_fix_rect_force(0u, 9u, 40u, 8u);
            draw_infix_block(a->tile_base, a->cols, a->rows,
                             cx, cy, a->pal_bank);
            demo_fix_puts(2u, 24u, "                                  ", 0u);
            demo_fix_puts(2u, 24u, BANNER_LBL[b], 1u);

            if (uwait(110u)) return 1u;
        }
    }
    clear_fix_rect_force(0u, 9u, 40u, 8u);
    demo_fix_puts(2u, 24u, "                                  ", 0u);

    /* ---------------- PHASE 2 — slide-in (240 frames) -------------- */
    demo_fix_puts(2u, 5u, "PHASE 2  SLIDE-IN (clear-redraw)  ", 2u);
    {
        const InfixImage *a = &INFIX_IMAGES[BANNER_IDX[0]];  /* 4.png */
        uint8_t  last_x = 0xFFu;
        for (t = 0u; t < 240u; t++) {
            /* X marches from -32 (offscreen left) through 8 (centred). */
            int16_t pos = (int16_t)((int16_t)t / 6 - 32);  /* -32..7 */
            uint8_t bx;
            if (pos < 0) {
                bx = 0u;
            } else {
                bx = (uint8_t)pos;
            }
            if (last_x != 0xFFu && bx != last_x) {
                clear_fix_rect_force(last_x, 11u, a->cols, a->rows);
            }
            if (pos >= -((int16_t)a->cols) && pos < 40) {
                draw_infix_block(a->tile_base, a->cols, a->rows,
                                 bx, 11u, a->pal_bank);
            }
            last_x = bx;
            if (uframe()) return 1u;
        }
    }
    clear_fix_rect_force(0u, 11u, 40u, 5u);

    /* ---------------- PHASE 3 — show the photo in real colour ------ */
    demo_fix_puts(2u, 5u, "PHASE 3  REAL COLOUR              ", 2u);
    {
        const InfixImage *a = &INFIX_IMAGES[BANNER_IDX[2]];  /* 9.png */
        uint8_t  cx = (uint8_t)((40u - a->cols) / 2u);
        uint8_t  cy = 11u;
        for (t = 0u; t < 240u; t++) {
            draw_infix_block(a->tile_base, a->cols, a->rows,
                             cx, cy, a->pal_bank);
            if (uframe()) return 1u;
        }
    }
    clear_fix_rect_force(0u, 11u, 40u, 5u);

    /* ---------------- PHASE 4 — tile grid (240 frames) ------------- */
    demo_fix_puts(2u, 5u, "PHASE 4  TILE GRID                ", 2u);
    {
        const InfixImage *a0 = &INFIX_IMAGES[0];   /* 0.png left   */
        const InfixImage *a6 = &INFIX_IMAGES[6];   /* 6.png right  */
        const InfixImage *a8 = &INFIX_IMAGES[8];   /* 8.png centre */
        draw_infix_block(a0->tile_base, a0->cols, a0->rows,
                         0u, 10u, a0->pal_bank);
        draw_infix_block(a6->tile_base, a6->cols, a6->rows,
                         22u, 11u, a6->pal_bank);
        draw_infix_block(a8->tile_base, a8->cols, a8->rows,
                         14u, 16u, a8->pal_bank);
        for (t = 0u; t < 240u; t++) {
            if (uframe()) return 1u;
        }
    }
    clear_fix_rect_force(0u, 9u, 40u, 12u);
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
    (void)chap_char_select();
    (void)chap_physics();
    (void)chap_camera();
    (void)chap_palette_fx();
    (void)chap_particles();
    (void)chap_particle_showcase();
    (void)chap_feedback();
    (void)chap_depthfx();
    (void)chap_depth_parallax();
    (void)chap_npcs();
    (void)chap_mini_game();
    (void)chap_joystick();
    (void)chap_scrolling_level();
    (void)chap_char_2d();
    (void)chap_raytrace3d();
    (void)chap_ssg_arcade();
    (void)chap_garden3d();
    (void)chap_sound();         /* moved to just before the credits   */
    (void)chap_credits();
}
