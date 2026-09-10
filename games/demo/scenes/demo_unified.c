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
 * to the white page colour. No state leaks across chapters.
 */

#include "demo_unified.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/ng_fix/ng_fix.h"
#include "sdk/2d_engine/ng_engine.h"
#include "sdk/2d_engine/ng_sprite_hw.h"
#include "sprite_meta.h"
#include "infix_palettes.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
/* A USE_2D_PLUS build compiles this file as C++.  Everything here is
 * reached from inline asm, the cart entry vectors or the BIOS by its
 * plain symbol name, so it must keep C linkage and not be mangled. */
extern "C" {
#endif


void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER clearSprs(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundReset(void);
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
void NEOGEO_USER playSFXB(uint8_t n);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playVoiceCue(uint8_t n);
void NEOGEO_USER ng_clear_screen_full(void);
const uint16_t * NEOGEO_USER ng_get_screen_palette(uint16_t screen_id);
const NGArtAsset * NEOGEO_USER ng_screen_art_asset(uint16_t screen_id);

#define U_FLOOR_Y       192
#define U_CENTRE_X      160

/* Lowered-hero baselines used by the palette FX, particles and
 * feedback/hitstop scenes.  FX_HERO_BOTTOM_Y matches the engine
 * floor at U_FLOOR_Y; FX_HERO_LIFT_Y is 4 px above it, which is
 * the actual draw target for those scenes so the eagle/hero
 * doesn't sit on the BG's earth strip and avoids the stale
 * sprite-window strip that was sometimes left attached when the
 * pose frame was uploaded at the old higher Y of 208/210/202. */
#define FX_HERO_BOTTOM_Y 200
#define FX_HERO_LIFT_Y   200

/*
 * Sprite slot plan.
 *
 * Lower sprite slots draw first on Neo Geo, so later slots draw in front.
 * Backgrounds use the low slot window; direct hero/enemy windows stay above
 * them so they are never hidden by a full-screen BG.
 *
 * A sprite drawn at slot S occupies S .. S+strips-1, where `strips` comes
 * from the art's own metadata and varies per FRAME, not per chapter - so
 * every window below has to be sized for the widest frame that can land in
 * it, not the one that happens to be on screen.  The widest asset in the
 * table is 16 strips.  Prop windows used to be bare numeric literals
 * scattered across five chapters with their budget implied only by the gap
 * to the next literal; that is the exact condition that produced the
 * shooter's sprite-slot overlap, so they are named and measured here.
 *
 *   1..48     backgrounds / parallax strips   DEMO_BG_BACK_SLOT, NG_SPR_BG*
 *   64..79    hero                (16 wide)   HERO_SLOT_FIRST
 *   80..95    enemy / clone       (16 wide)   ENEMY_SLOT_FIRST
 *   100..123  joystick hitbox prop            DEMO_PROP_HITBOX_SLOT
 *   124..147  FX prop A                       DEMO_PROP_FX_A_SLOT
 *   148..171  FX prop B                       DEMO_PROP_FX_B_SLOT
 *   172..183  FX prop C                       DEMO_PROP_FX_C_SLOT
 *   220..239  scrolling-level marker          DEMO_PROP_MARKER_SLOT
 *   256..287  particle pool                   NG_SPR_PART_FIRST
 *   340..379  optional HUD letterbox          two 20-strip bands
 *
 * Measured worst cases against those windows (asset -> strips):
 *   hitbox 136        -> 16, fits 100..115
 *   FX frames 89..92  -> up to 10, fit the A/B/C windows
 *   markers 132..135  -> up to 8,  fits 220..227
 *   hero frames       -> up to 16, exactly fills 64..79 up to ENEMY at 80
 *
 * Keep every window's start at least max-strips past the previous
 * window's start.  Two sprites writing the same SCB slots every frame is
 * the failure that corrupted the shooter's formation, and it is easy to
 * reintroduce: the FX frames run to 10 strips, so prop C alone spans
 * 172..181.
 *
 * The shooter chapter deliberately reuses 96..271 wholesale for its
 * formation; it draws none of the props above, so that overlap is by
 * design and confined to that one chapter.
 */
#define DEMO_BG_BACK_SLOT  NG_SPR_BG0_FIRST
#define HERO_SLOT_FIRST    64u
#define ENEMY_SLOT_FIRST   80u

#define DEMO_PROP_HITBOX_SLOT  100u
#define DEMO_PROP_FX_A_SLOT    124u
#define DEMO_PROP_FX_B_SLOT    148u
#define DEMO_PROP_FX_C_SLOT    172u
#define DEMO_PROP_MARKER_SLOT  220u

#define U_BG_FOREST        2u
/* The second background already in the pipeline (screen_id 1,
 * games/demo/artbox/in/backgrounds/0.png - mountain/waterfall/pagoda
 * scene) was sitting unused. */
#define U_BG_MOUNTAIN      1u

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

/* CC0 (public domain), Kenney "Shooting Gallery" pack via OpenGameArt.org -
 * appended after the UFO entries so no index shifted (verified: 131
 * existing entries unchanged, 4 new ones landed at 132-135).  Replaces
 * the target range chapter's old target sprites (reused hero/effect
 * frames 49/50/51/53, which didn't read as "targets" at all). */
#define U_DUCK_BACK              132u
#define U_DUCK_TARGET_BROWN      133u
#define U_DUCK_TARGET_WHITE      134u
#define U_DUCK_TARGET_YELLOW     135u

/* CC0 (public domain), "Wooden box - Pixel art 32x32" by MSavioti via
 * OpenGameArt.org - appended after the duck entries so no index shifted
 * (verified: 135 existing entries unchanged, landed at 136).  Replaces
 * the joystick chapter's ASCII "#"/"." grid stand-in for its punch
 * target with an actual pixel-art crate. */
#define U_HITBOX                 136u

/* CC0 (public domain), Kenney "Particle Pack" via OpenGameArt.org -
 * appended after the hitbox entry so no index shifted (verified: 136
 * existing entries unchanged, 5 new ones landed at 137-141).  The pack's
 * sprites are grayscale (designed for additive blending, which the
 * NeoGeo hardware can't do), so each was pre-tinted to its particle
 * role before import.  Replaces the particles chapter's single reused
 * effect sprite (048.png strip) standing in for every particle kind -
 * dust, magic, hit-spark, explosion and smoke all looked identical. */
#define U_PARTICLE_DUST          137u
#define U_PARTICLE_EXPLOSION     138u
#define U_PARTICLE_HITSPARK      139u
#define U_PARTICLE_MAGIC         140u
#define U_PARTICLE_SMOKE         141u

/* CC0 (public domain), "Seamless Space Backgrounds" (Starfield 3) by
 * Screaming Brain Studios via OpenGameArt.org - appended after the
 * particle entries so no index shifted (verified: 141 existing entries
 * unchanged, landed at 142).  The "npcs" catch-all category gets the
 * same full 256x256 canvas as the dedicated "backgrounds" category, so
 * this can be drawn with the same draw_background() primitive used
 * elsewhere without needing the (unsafe-to-append-to) backgrounds/
 * folder.  Replaces the SSG arcade's plain black backdrop. */
#define U_SSG_STARFIELD          142u

/* User-provided ship art (games/demo/artbox/in/npcs/zzzzzzzz_*) -
 * appended after the starfield entry so no index shifted (verified:
 * 142 existing entries unchanged, landed at 143-146).  Replaces the
 * UFO enemies and the shooter/player ship.  Pink/green are hue-rotated
 * from the source blue so all three read as one consistent enemy
 * design instead of mixed art styles across rows.  The player vessel
 * source was a JPEG with a checkerboard baked into the pixels instead
 * of real alpha (JPEG has no alpha channel at all) - flood-filled the
 * checker tones out from the image edges before import so the sprite
 * doesn't carry a "many dots" artifact from the transparency being
 * misread. */
#define U_ENEMYSHIP_BLUE         143u
#define U_ENEMYSHIP_GREEN        144u
#define U_ENEMYSHIP_PINK         145u
#define U_PLAYER_VESSEL          146u

/* Simple procedurally-generated balloons (no licensing question -
 * avoids the CC-BY-SA/attribution options found while sourcing this)
 * - appended after the player vessel entry so no index shifted
 * (verified: 146 existing entries unchanged, landed at 147-150).
 * Replaces the scrolling-level chapter's cat NPC waypoint markers,
 * which shouldn't be NPCs at all in that chapter. */
#define U_BALLOON_BLUE           147u
#define U_BALLOON_GREEN          148u
#define U_BALLOON_RED            149u
#define U_BALLOON_YELLOW         150u

/*
 * Sky Lance mini — the vertical-shooter slice of games/skylance, drawn
 * from that game's own art imported into this ROM's npcs page.  These
 * seven were deliberately named to sort last inside artbox/in/npcs so
 * every asset id above stays exactly where it was; verified against the
 * regenerated sprite_meta.h, ids 1..150 are byte-identical.
 */
#define U_SKY_BOSS               151u
#define U_SKY_BULLET             152u
#define U_SKY_ENEMY_A            153u
#define U_SKY_ENEMY_B            154u
#define U_SKY_ENEMY_C            155u
#define U_SKY_ORB                156u
#define U_SKY_PLANE              157u

/*
 * Wooden crate props, intact and smashed.  These replace the abstract
 * U_HITBOX square wherever a chapter needs something to shoot or punch:
 * a crate reads as a target at a glance, and having a broken state means
 * a hit can be shown rather than just scored.
 */
#define U_CRATE                  158u
#define U_CRATE_BROKEN           159u

/*
 * Sky page for the Sky Lance mini.
 *
 * It lives in artbox/in/zz_bg/ as 0.png, which is deliberate on both
 * counts: a subdirectory outside CATEGORY_ORDER is appended last, so no
 * existing asset id moves, and the numeric filename is the only thing
 * that matches the legacy_screens rule - the one rule with no category -
 * which routes it through the SCREEN converter.  Dropped into in/npcs/
 * instead it would have been quantised as a sprite against the palette
 * shared with the cats, balloons and particles, and a sky has no business
 * sharing fifteen colours with a cat.
 *
 * The page repeats a 144-px vertical loop, matching the 9-tile-row band
 * the chapter shows - see the backdrop draw in chap_image_shooter().
 */
#define U_SKY_BG                 160u
#define U_BG_FOREGROUND          161u

/* Compact classic NPC walk cycles restored after the main asset table.
 * IDs 162..165 face forward, 166..169 move right, 170..173 move left. */
#define U_NPC_OLD_FIRST          162u
#define U_NPC_OLD_COUNT           12u

/*
 * Object-specific scale presets.  Only chapters that explicitly demonstrate
 * depth or hardware shrinking vary character scale at runtime.
 *
 * These were originally written as a percentage of full size, which sets the
 * height correctly but not the width: the hardware takes X from the top nibble
 * alone, so it quantises to sixteenths while Y keeps the full byte.  A value
 * like 0x73 therefore drew 50% wide and 45% tall - a 9% stretch on every
 * character wearing it.  NG_SCALE() picks the nearest sixteenth and matches
 * both axes to it, so the names below are now the true drawn size.
 *
 * 55% and 57% land on the same sixteenth; the hardware cannot separate them.
 */
#define U_SCALE_30         NG_SCALE(5)    /* 31.25% */
#define U_SCALE_45         NG_SCALE(7)    /* 43.75% */
#define U_SCALE_55         NG_SCALE(9)    /* 56.25% */
#define U_SCALE_57         NG_SCALE(9)    /* 56.25% */
#define U_SCALE_60         NG_SCALE(10)   /* 62.50% */
#define U_SCALE_70         NG_SCALE(11)   /* 68.75% */
#define U_SCALE_FULL       NG_SCALE(16)   /* 100%   */

/*
 * Clean-ratio scales.
 *
 * The hardware shrink byte V renders (V+1)/256 of the source, and the
 * shrink table decides which source lines to drop.  When that fraction
 * is not a simple binary one the dropped lines land unevenly, which on
 * pixel art shows up as some rows of a sprite losing a pixel of detail
 * while their neighbours keep it - thin outlines go dashed and eyes or
 * belts flicker as the character animates.  The presets above are all
 * arbitrary fractions (60.2%, 46.5%, ...) and suffer from exactly that.
 *
 * These are the exact binary fractions, where the table drops every
 * Nth line evenly and the art stays coherent:
 */
/*
 * Draw scales, expressed as a fraction of a 256 px import.
 *
 * They are not fed to the hardware as they stand.  Assets are imported
 * at the size they are drawn, so different categories arrive on
 * different canvases, and the same on-screen size needs a different
 * hardware scale from each.  demo_asset_scale() converts one of these
 * into the scale that asset actually needs, so a call site can go on
 * saying how big the figure should look without knowing what canvas it
 * came in on.
 */
#define U_SCALE_1_4        0x3Fu   /* 64/256  = 1/4   = 25.0% */
#define U_SCALE_5_16       0x4Fu   /* 80/256  = 5/16  = 31.2% */
#define U_SCALE_3_8        0x5Fu   /* 96/256  = 3/8   = 37.5% */
#define U_SCALE_7_16       0x6Fu   /* 112/256 = 7/16  = 43.8% */
#define U_SCALE_1_2        0x7Fu   /* 128/256 = 1/2   = 50.0% */
#define U_SCALE_CHARACTER  U_SCALE_1_2

static const uint8_t s_fx_effect_frames[4] = {
    89u, 90u, 91u, 92u
};

/*
 * The acting character for the palette-FX, particle and feedback
 * chapters: the warrior's sword-spin special (sprite_033..sprite_037).
 *
 * These used to be 49..53, which are the purple spirit-eagle frames.
 * That put a bird on screen in three chapters that are about what the
 * palette and particle systems do to a CHARACTER - and chap_feedback
 * captions itself "WARRIOR TAKES HITS" while showing one.
 */
static const uint8_t s_fx_char_frames[5] = {
    35u, 36u, 37u, 38u, 39u
};

/*
 * Flight cycle: two poses of one bird.
 *
 * Asset 80 is a flying pose too, but of the OTHER eagle in the sheet -
 * the rust-brown one that assets 75..77 perch.  Cycling 78, 79, 80
 * therefore changed the bird's colour every third frame, which reads as
 * a palette fault and is not one: all three assets share a palette and
 * the artwork itself is simply two different birds.
 */
static const uint8_t s_flight_frames[2] = {
    78u, 79u
};

/* ------------------------------------------------------------------ */
/*  Shared state used by uframe (so it knows what to flush)              */
/* ------------------------------------------------------------------ */
static uint8_t  s_draw_chars      = 0u;
static uint8_t  s_draw_particles  = 0u;

/* Whichever face was picked (or left on, in attract mode) when
 * chap_char_select() finished - 0 = girl, 1 = eagle.  Recorded for any
 * later chapter that wants to reflect the choice; the palette FX and
 * particle chapters used to draw a cameo portrait from it and no longer
 * do, so nothing reads it at present. */
static uint8_t  s_selected_char   = 1u;   /* defaults to eagle if select
                                            * chapter never runs first */

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

/*
 * Chapter restart (C).
 *
 * demo_advance_requested() reports A as 1 and C as 2; CHAP_REQ_RESTART
 * names the latter.  uframe() turns a C press into s_restart_requested
 * and still returns non-zero, so every chapter's existing
 * "if (uframe()) return 1u;" unwinds out of its loop with no per-chapter
 * changes at all.  run_chapter() below sees the flag and simply calls
 * the chapter function again, which re-runs its own chap_header() and
 * setup - so a restart resets sprites, palettes, sound and game state
 * by construction instead of each chapter hand-rolling a reset.
 *
 * s_restart_enabled is the opt-out for a chapter that needs C as a
 * gameplay button (the joystick chapter uses it for jump and for the
 * B+C combos it exists to demonstrate).
 */
#define CHAP_REQ_RESTART  2u

static uint8_t  s_restart_requested = 0u;
static uint8_t  s_restart_enabled   = 1u;

/*
 * Skip/restart press that arrived while a sound transition was running.
 *
 * The cross-fade waits below pump uframe() like any other wait, but they
 * cannot return early - they still have Z80 commands to send.  They used
 * to just discard whatever uframe() reported, and a chapter change is the
 * exact moment a player leans on A, so every press that landed inside the
 * ~0.6s fade did nothing at all.  snd_wait() latches it here instead and
 * the chapter's next uframe() reports it as if it had just happened.
 */
static uint8_t  s_pending_req = 0u;

/*
 * ADPCM-B streamed TRACKs have no hardware loop - the chip plays from
 * start address to end address once and stops.  soundPlayGameLoop()'s
 * name is aspirational: nothing on the Z80 side re-triggers it, so any
 * chapter that runs longer than the selected TRACK's raw sample length
 * goes completely silent for the rest of the chapter (confirmed against
 * games/demo/sound/samples/in_wav_b - the wav files there run 11..82s,
 * while several chapters run well past that once a player lingers or
 * plays for a while).  This table (frames at 60Hz, ~0.5s short of the
 * real length as a retrigger margin) lets uframe() restart the current
 * TRACK from the top just before it would run out, so background music
 * actually loops instead of playing once and dying.  Order matches
 * soundPlayGameLoop's own TRACK_pool (games/demo/sound/samples/in_wav_b,
 * skipping TRACK_E which is reserved for the eyecatcher: A/B/C/D/F/G/H/I).
 */
static const uint16_t s_bgm_loop_frames[8] = {
    647u, 4425u, 4884u, 2649u, 3570u, 2010u, 2130u, 3150u
};
static const uint8_t s_bgm_track_id[8] = {
    SOUND_TRACK_A, SOUND_TRACK_B, SOUND_TRACK_C, SOUND_TRACK_D,
    SOUND_TRACK_F, SOUND_TRACK_G, SOUND_TRACK_H, SOUND_TRACK_I
};
static uint8_t  s_bgm_track       = 0xFFu;   /* 0xFF = watchdog disarmed */
static uint16_t s_bgm_frames_left = 0u;

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

    /* Background-music loop watchdog - see s_bgm_loop_frames above.
     * Only armed while a chapter is using snd_cross_to()'s TRACK bed;
     * chap_sound()'s manual TRACK-cycling demo never calls
     * snd_cross_to(), so s_bgm_track stays 0xFF and this is a no-op
     * there. */
    if (s_bgm_track != 0xFFu) {
        if (s_bgm_frames_left > 0u) {
            s_bgm_frames_left--;
        } else {
            playSFXB(s_bgm_track_id[s_bgm_track]);
            s_bgm_frames_left = s_bgm_loop_frames[s_bgm_track];
        }
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

    {
        uint8_t req = demo_advance_requested();

        if (req == 0u) {
            /* Nothing pressed this frame - hand over anything snd_wait()
             * caught during the last sound transition. */
            req = s_pending_req;
            s_pending_req = 0u;
        }

        if (req == CHAP_REQ_RESTART) {
            /* A chapter that uses C as a gameplay button opts out, and
             * for it C must not end the chapter either - swallow the
             * request entirely rather than letting it read as a skip. */
            if (!s_restart_enabled) return 0u;
            s_restart_requested = 1u;
        }
        return req;
    }
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
    soundFMSetLFO(value);
    waitVbl();
    return uwait(frames);
}

/* ------------------------------------------------------------------ */
/*  Sound helpers — every Z80 command separated by one vblank so the    */
/*  driver has time to process without queue collisions.                */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER snd_step(void)
{
    waitVbl();
}

/*
 * The driver's fade "speed" is an INTERVAL, not a rate: its fade engine
 * reloads a counter with (255 - speed) Timer-B ticks between each -16
 * volume step, and Timer B runs at ~8.1 Hz (123ms).  The old speed of 6
 * therefore meant 249 ticks -> ~30 SECONDS per step, so across the old
 * 12-frame (0.2s) wait below not one step ever ran and the "cross-fade"
 * was really an abrupt cut.  0xFE is the fastest the driver can express
 * (1 tick, ~123ms per step), which makes the dip actually audible.
 */
#define SND_FADE_SPEED    0xFEu
#define SND_FADE_FRAMES   36u   /* ~0.6s - about 5 of the -16 steps */
#define SND_SILENCE_FRAMES 48u  /* ~0.8s - a fuller fade before silence */
#define SND_SKIP_TAIL      4u   /* fade frames still owed once A is pending */

/*
 * Vblank wait for the sound transitions.
 *
 * Two things this does that a plain uwait() cannot: it never abandons the
 * caller half-way through a command sequence, and it does not throw away
 * the skip the player just asked for - see s_pending_req.  Once a request
 * is pending the remaining fade is cut to SND_SKIP_TAIL frames, so
 * hammering A walks the reel at the player's pace instead of paying
 * 0.6s of cross-fade per chapter.
 */
static void NEOGEO_USER snd_wait(uint16_t frames)
{
    uint16_t t;
    uint16_t limit = frames;

    for (t = 0u; t < limit; t++) {
        uint8_t req = uframe();

        if (req != 0u) {
            if (s_pending_req == 0u) s_pending_req = req;
            if ((uint16_t)(limit - t) > SND_SKIP_TAIL) {
                limit = (uint16_t)(t + SND_SKIP_TAIL);
            }
        }
    }
}

/*
 * Start a chapter's background bed.
 *
 * This is soundPlayGameLoop()'s exact sequence - stop, driver reset, the
 * four mixer levels, then the ADPCM-B TRACK trigger - but with a vblank
 * between every step.  The library version fires all seven commands back
 * to back, and the whole reason every other Z80 command in this file is
 * separated by snd_step() is that the driver drops commands that arrive
 * while it is still busy with the previous one.  The trigger is the LAST
 * of the seven, so it is the one that gets lost, and the symptom is
 * precisely the outgoing chapter's music carrying on into the next
 * chapter - which is what a fast skip, arriving while the driver is
 * still working through the outgoing chapter's stop, used to produce.
 *
 * The mixer levels and the TRACK table below mirror soundPlayGameLoop()
 * and s_bgm_track_id[] respectively; both are ADPCM-B forward, FM and SSG
 * silent.
 */
static void NEOGEO_USER snd_start_bed(uint8_t track)
{
    uint8_t slot = (uint8_t)(track & 0x07u);

    soundStopAll();               snd_step();
    soundReset();                 snd_step();
    soundSetADPCMAVolume(0x30u);  snd_step();
    soundSetADPCMBVolume(0xB8u);  snd_step();
    soundSetSSGVolume(0x00u);     snd_step();
    soundSetFMVolume(0x00u);      snd_step();
    playSFXB(s_bgm_track_id[slot]);
    snd_step();
}

/*
 * Restore the live volume registers after a fade.
 *
 * The driver's stop-all clears the fade STATE but deliberately leaves
 * the volume registers wherever the fade left them; only its cancel-fade
 * command copies the *_BASE values back into the live volumes.  Those
 * BASE values are exactly what soundApplyMix() last wrote, so this
 * restores the chapter's intended mix rather than any hardcoded default.
 * Without it, now that the fade above actually moves the volume, every
 * chapter transition would start its track quieter than the last and the
 * music would decay toward silence over the course of the reel.
 */
static void NEOGEO_USER snd_restore_mix(void)
{
    soundCancelFade();
    snd_step();
}

static void NEOGEO_USER snd_cross_to(uint8_t track)
{
    soundFadeOutSpeed(SND_FADE_SPEED);
    snd_wait(SND_FADE_FRAMES);
    soundStopAll();
    snd_step();
    snd_restore_mix();
    snd_start_bed(track);
    s_bgm_track = (uint8_t)(track & 0x07u);
    s_bgm_frames_left = s_bgm_loop_frames[s_bgm_track];
}

static void NEOGEO_USER snd_silence(void)
{
    soundFadeOutSpeed(SND_FADE_SPEED);
    snd_wait(SND_SILENCE_FRAMES);
    soundStopAll();
    snd_step();
    /* Leave the mixer at the chapter's configured levels, not at the
     * faded-down ones - the next chapter to start a track inherits
     * these registers. */
    snd_restore_mix();
    s_bgm_track = 0xFFu;
}

/* ------------------------------------------------------------------ */
/*  Common FIX-layer chrome shared by every chapter                     */
/*                                                                      */
/*  Row ownership (FIX rows are 0..27) - respect this or the write is   */
/*  silently lost:                                                      */
/*    0      chapter tag + title + index    chap_header(), once         */
/*    1      chapter subtitle               chap_header(), once         */
/*    2..3   per-chapter title/subtitle     each chapter, once          */
/*    4..25  chapter body / HUD             each chapter, free          */
/*    26     separator rule + control hint  chap_header()/chap_hint()   */
/*    27     caption bar + live timer       uframe(), EVERY FRAME       */
/*                                                                      */
/*  Row 27 belongs to uframe() alone: it is rebuilt from scratch on      */
/*  every single frame, so a chapter writing a one-shot hint there is    */
/*  overwritten before the next vblank and never becomes visible.  Three */
/*  chapters were doing exactly that (the FIX tour, physics, and the     */
/*  shooter's "C: RESTART"), which is why the shooter's restart control  */
/*  was undiscoverable.  Chapters advertise controls with chap_hint()    */
/*  instead, which lands on row 26 and stays put.                        */
/* ------------------------------------------------------------------ */
#define CHAP_RULE_W  36u   /* cols 2..37 - the separator/hint field */

/*
 * Shared FIX-layer flourishes.
 *
 * Both reuse the technique the FIX-FX chapter's palette-cycle phase
 * already proves out: restamp the same cells each frame at a rotating
 * palette index.  It is the cheapest attract-mode trick on this
 * hardware - no tile churn, no extra VRAM, one mess_out per frame - and
 * it is what gives arcade select screens and credit rolls their
 * shimmer instead of flat static text.
 */
static const uint8_t s_fix_cycle_pal[4] = { 2u, 1u, 3u, 1u };

/* Marquee text: same string, palette advancing every `speed` frames. */
static void NEOGEO_USER fix_cycle_puts(uint8_t x, uint8_t y,
                                       const char *text,
                                       uint16_t t, uint8_t speed)
{
    if (speed == 0u) speed = 1u;
    demo_fix_puts(x, y, text, s_fix_cycle_pal[(t / speed) & 3u]);
}

/* Blinking selection brackets around a label, the way a fighting-game
 * select screen marks the active pick. */
static void NEOGEO_USER fix_select_marks(uint8_t x, uint8_t y,
                                         uint8_t width,
                                         uint8_t active,
                                         uint16_t t)
{
    uint8_t pal = active ? s_fix_cycle_pal[(t / 6u) & 3u] : 0u;
    demo_fix_puts(x, y, active ? ">" : " ", pal);
    demo_fix_puts((uint8_t)(x + width + 1u), y, active ? "<" : " ", pal);
}

/* Static sprite letterbox, independent of transparent FIX glyphs. */
static void NEOGEO_USER hud_band(uint16_t first, uint8_t y, uint8_t rows)
{
    uint8_t row;
    uint8_t col;
    uint16_t tiles[32];
    uint16_t attrs[32];
    uint8_t height = (uint8_t)((rows + 1u) / 2u);
    /* FIX glyphs stay transparent. An independently owned sprite band
     * supplies their backing without leaving solid cells between words.
     * FFFD is half solid, FFFE solid, FFFF transparent in Artbox C-ROMs. */
    NEO_REGISTER(0x401fe2u) = DEMO_BG;
    for (row = 0u; row < 32u; row++) {
        tiles[row] = row < rows / 2u ? 0xfffeu : 0xffffu;
        if ((rows & 1u) && row == rows / 2u) tiles[row] = 0xfffdu;
        attrs[row] = 0xff00u;
    }
    for (col = 0u; col < 20u; col++) {
        uint16_t slot = (uint16_t)(first + col);
        NEO_REGISTER(VRAM_ADDR) = (uint16_t)(slot * 64u);
        NEO_REGISTER(VRAM_INC) = 1u;
        for (row = 0u; row < 32u; row++) {
            NEO_REGISTER(VRAM_RW) = tiles[row];
            NEO_REGISTER(VRAM_RW) = attrs[row];
        }
        vram_SCB234((uint16_t)(SCB2_ADDR + slot), 0x0fffu);
        vram_SCB234((uint16_t)(SCB4_ADDR + slot), setSCB4(col * 16u));
        vram_SCB234((uint16_t)(SCB3_ADDR + slot), setSCB3((uint16_t)(496u - y), 0u, height));
    }
}

/*
 * A chapter that runs its backdrop edge to edge asks for a letterbox
 * before it calls chap_header(): the bands have to go down after the
 * screen clear and before any text, or they wipe the header they exist
 * to make readable.  Cleared on use so it cannot leak into the chapter
 * that follows.
 */
static uint8_t s_letterbox_top    = 0u;
static uint8_t s_letterbox_bottom = 0u;


/* "CH.nn" for the chapter now running, as chap_header numbered it. */
static char s_chapter_tag[6] = "CH.00";

static const char * NEOGEO_USER chap_tag(void)
{
    return s_chapter_tag;
}

static void NEOGEO_USER chap_letterbox_next(uint8_t top_rows,
                                            uint8_t bottom_rows)
{
    s_letterbox_top    = top_rows;
    s_letterbox_bottom = bottom_rows;
}

static void NEOGEO_USER chap_hint(const char *hint)
{
    char rule[CHAP_RULE_W + 1u];
    uint8_t n = 0u;
    uint8_t p;
    uint8_t k;

    /* Every chapter can be restarted with C, so that is the default
     * hint; the caption bar on row 27 already carries A:NEXT. */
    if (!hint) hint = "C:RESTART";

    /* Cap well under the field width so the dash padding below can never
     * run past the start of the rule. */
    while (hint[n] != '\0' && n < 28u) n++;

    for (p = 0u; p < CHAP_RULE_W; p++) rule[p] = '-';
    rule[CHAP_RULE_W] = '\0';

    if (n > 0u) {
        uint8_t start = (uint8_t)(CHAP_RULE_W - n);
        rule[start - 1u] = ' ';          /* one blank cell before the hint */
        for (k = 0u; k < n; k++) rule[start + k] = hint[k];
    }

    demo_fix_puts(2u, 26u, rule, 0u);
}

/* ------------------------------------------------------------------ */
/*  Chapter header: hard-clears EVERY subsystem and re-draws labels      */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER chap_header(uint8_t n,
                                    const char *title,
                                    const char *subtitle)
{
    char tag[6];

    /* hard reset hardware + char/physics/particles/feedback/palette FX
     * (it runs the wipe on DEMO_BG_CLEAR and restores DEMO_BG itself) */
    ng_clear_screen_full();
    /* The sprite-window tracking cache (demo.c) is separate from VRAM/
     * SCB state and is NOT touched by ng_clear_screen_full() - it's
     * global across the whole ROM run and only reset via helpers no
     * chapter here calls, so distinct slot numbers used by earlier
     * chapters permanently occupy its 48 entries until evicted.  Reset
     * it every chapter so each one gets the full budget fresh instead
     * of starving a later, slot-heavy chapter (e.g. the 24-enemy
     * shooter) of room and causing sprite thrashing/corruption. */
    demo_sprite_window_cache_reset();
    /*
     * clearSprs() is intentionally omitted here: ng_clear_screen_full()
     * already does a full ng_sprite_hide_all() kill (SCB3=0x8000, full
     * SCB1 wipe with blank-tile 0xFFFF).  A second clearSprs() would
     * overwrite that safe blank-tile fill with tile-0, which may carry art.
     */
    setBACKDROP(DEMO_BG);
    ng_level_set_scroll(0, 0);
    ng_particles_init();
    ng_feedback_init();
    ng_palette_fx_init();

    /* per-chapter flush flags — chapters that need chars/particles set them */
    s_draw_chars     = 0u;
    s_draw_particles = 0u;
    hero_scale(U_SCALE_CHARACTER);

    /* Restart is available by default; a chapter that needs C for its
     * own input clears this after calling us. */
    s_restart_enabled = 1u;

    /* A skip latched by the outgoing chapter's fade-to-silence has done
     * its job already - it ended that chapter.  Drop it here so it cannot
     * also skip this one; snd_cross_to() below runs after this point, so
     * presses made during THIS chapter's cross-fade still survive. */
    s_pending_req = 0u;

    /* Disarm the BGM loop watchdog - re-armed only if this chapter calls
     * snd_cross_to() itself, so chapters managing sound manually (the
     * sound tour, the SSG-only shooter) aren't second-guessed. */
    s_bgm_track = 0xFFu;

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

    /* Kept so a chapter that repaints its own header row restates the
     * running count rather than spelling a number out again. */
    s_chapter_tag[0] = tag[0]; s_chapter_tag[1] = tag[1];
    s_chapter_tag[2] = tag[2]; s_chapter_tag[3] = tag[3];
    s_chapter_tag[4] = tag[4]; s_chapter_tag[5] = tag[5];

    if (s_letterbox_top) hud_band(340u, 0u, s_letterbox_top);
    if (s_letterbox_bottom) hud_band(360u, (uint8_t)((28u - s_letterbox_bottom) * 8u),
                                    s_letterbox_bottom);
    s_letterbox_top    = 0u;
    s_letterbox_bottom = 0u;
    ng_sprite_park_off_range(288u, 40u);

    demo_fix_puts(2u,  0u, tag,   0u);
    demo_fix_puts(8u,  0u, title, 2u);
    if (subtitle) demo_fix_puts(2u, 1u, subtitle, 1u);

    /* Separator line above the caption bar, drawn once per chapter.  A
     * chapter with an extra control to advertise calls chap_hint() after
     * this to fold it into the same row. */
    chap_hint(0);

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
    int16_t anchor_x;
    int16_t anchor_y;

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
    ng_char_set_tile_stride(c, demo_screen_tile_stride(frame));
    {
        const NGArtAsset *art = ng_screen_art_asset(frame);
        ng_char_set_palette_map(c, art ? art->tile_palettes : 0);
    }

    c->scale_x = demo_asset_scale(frame, scale_x);
    c->scale_y = demo_asset_scale(frame, scale_y);

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
    demo_asset_anchor_offset(frame, scale_x, scale_y, 1u, c->flip_x,
                              &anchor_x, &anchor_y);
    c->sprite_offset_x = -anchor_x;
    c->sprite_offset_y = -anchor_y;
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

/*
 * Centre anchor.
 *
 * draw_asset_bottom_center() puts the artwork's painted BOTTOM edge on
 * the given y, which is what a character standing on a floor wants; a
 * flying object wants its middle there instead, so this anchors on
 * (y_pad + content_height/2).
 *
 * Note what it does NOT do: pass cx/cy straight to
 * demo_draw_sprite_screen().  That call takes the origin of the whole
 * 16x16 PAGE, and demo_perform_sprite_draw() adds the artwork's
 * tile-grid offset back on - so a sprite whose art starts six tile rows
 * down its page lands 96 px below where the caller asked, which is off
 * the bottom of the screen for anything already in the lower half.
 * The tile-grid part is unscaled and the in-tile padding is scaled, for
 * the same reason spelled out over demo_anchor_bottom_center().
 */
static void NEOGEO_USER draw_asset_center(uint8_t frame,
                                          uint16_t first_sprite,
                                          int16_t cx,
                                          int16_t cy,
                                          uint8_t scale_x,
                                          uint8_t scale_y)
{
    int16_t pad_x;
    int16_t pad_y;
    demo_asset_anchor_offset(frame, scale_x, scale_y, 0u, 0u, &pad_x, &pad_y);

    demo_draw_sprite_screen(frame, first_sprite,
                            (int16_t)(cx - demo_screen_x_offset(frame)
                                         - pad_x),
                            (int16_t)(cy - demo_screen_y_offset(frame)
                                         - pad_y),
                            demo_screen_strips(frame),
                            demo_screen_rows(frame),
                            scale_x, scale_y);
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
    int16_t anchor_x;
    int16_t anchor_y;
    demo_asset_anchor_offset(frame, scale_x, scale_y, 1u, flip, &anchor_x, &anchor_y);

    demo_draw_sprite_screen_flip(frame, first_sprite,
                                 (int16_t)(cx - anchor_x - demo_screen_x_offset(frame)),
                                 (int16_t)(bottom_y - anchor_y - demo_screen_y_offset(frame)),
                                 strips, rows, scale_x, scale_y, flip);
}

static void NEOGEO_USER draw_background(uint8_t frame, int16_t x, int16_t y)
{
    /* Background slot range stays behind characters and effects. */
    demo_draw_sprite_screen(frame, DEMO_BG_BACK_SLOT, x, y,
                            demo_screen_strips(frame),
                            demo_screen_rows(frame),
                            0xFFu, 0xAFu);
}

static void NEOGEO_USER draw_scrolling_background(uint8_t frame, int16_t x, int16_t y)
{
    uint8_t page;
    while (x > 0) x = (int16_t)(x - 256);
    while (x <= -256) x = (int16_t)(x + 256);
    /* Three pages cover 320 pixels at every sub-page offset. The
     * imported terrain was precompensated for 176/256 vertical scale. */
    for (page = 0u; page < 3u; page++) {
        demo_draw_sprite_screen(frame, (uint16_t)(1u + page * 16u),
                                 (int16_t)(x + page * 256), (int16_t)(y + 24),
                                 16u, 16u, 0xffu, 0xafu);
    }
}

/* Adjacent level pages use distinct art and palettes, not a repeated
 * offset into one image. All six windows remain below character slot 64. */
static void NEOGEO_USER draw_level_background(uint16_t distance, uint8_t vertical)
{
    uint16_t span = vertical ? 176u : 256u;
    uint16_t cell = (uint16_t)(distance / span);
    int16_t offset = -(int16_t)(distance % span);
    uint8_t page;
    for (page = 0u; page < 3u; page++) {
        uint8_t frame = ((cell + page) & 1u) ? U_BG_FOREST : U_BG_MOUNTAIN;
        uint16_t slot = (uint16_t)(1u + page * 20u);
        int16_t x = vertical ? 0 : (int16_t)(offset + page * 256);
        int16_t y = vertical ? (int16_t)(offset + page * 176) : 24;
        demo_draw_sprite_screen(frame, slot, x, y, 16u, 16u, 0xffu, 0xafu);
        if (vertical) {
            demo_draw_sprite_screen(frame, (uint16_t)(slot + 16u),
                                     256, y, 4u, 16u, 0xffu, 0xafu);
        } else {
            demo_hide_sprite_range((uint16_t)(slot + 16u), 4u);
        }
    }
}

static void NEOGEO_USER draw_vertical_background(uint8_t frame, int16_t x, int16_t y)
{
    while (y > 0) y = (int16_t)(y - 256);
    while (y <= -256) y = (int16_t)(y + 256);

    demo_draw_sprite_screen(frame, DEMO_BG_BACK_SLOT, x, y,
                            demo_screen_strips(frame),
                            demo_screen_rows(frame),
                            0xFFu, 0xFFu);
    demo_draw_sprite_screen(frame, NG_SPR_BG1_FIRST,
                            x, (int16_t)(y + 256),
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
                          tile_base,
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

    chap_header(1u, "TITLE", "ATTRACT REEL");
    /* Was a hand-rolled soundSceneReset + mixer + soundPlayGameLoop block
     * running BEFORE chap_header().  Two problems: soundPlayGameLoop()
     * re-applies its own mixer levels, so the ones set just above it never
     * survived, and nothing faded or stopped the outgoing chapter's TRACK
     * first.  snd_cross_to() does both, and arms the loop watchdog so the
     * bed does not die part-way through the 450-frame parade. */
    snd_cross_to(SOUND_MUSIC_E);
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
/* Every label on row 2 shares the field, so each has to overwrite the
 * whole of it - a shorter caption written over a longer one otherwise
 * keeps the tail of its predecessor ("SFIX SHEET" reading as
 * "SFIX SHEETE"). */
static void NEOGEO_USER chap_fix_label(const char *text)
{
    char line[35];
    uint8_t i = 0u;

    while (text[i] != '\0' && i < 34u) { line[i] = text[i]; i++; }
    while (i < 34u) line[i++] = ' ';
    line[34] = '\0';
    demo_fix_puts(2u, 2u, line, 1u);
}

/*
 * Draw one of the generated INFIX images with the palette bank it was
 * generated for.
 *
 * Each image carries its own 16-colour bank (INFIX_PALETTES, banks 4
 * upwards).  Drawing one in a text palette instead does not merely
 * recolour it: the text banks are built for glyphs, so most of the
 * image's colour indices land on entries that are transparent or
 * near-identical, and a full-width banner comes out as a few scattered
 * letters.  That is what this chapter was showing.
 */
static void NEOGEO_USER chap_fix_infix(uint8_t idx, uint8_t x, uint8_t y,
                                       uint8_t rows)
{
    const InfixImage *a;

    if (idx >= INFIX_IMAGE_COUNT) return;
    a = &INFIX_IMAGES[idx];
    draw_infix_block(a->tile_base, a->cols,
                     rows && rows < a->rows ? rows : a->rows,
                     x, y, a->pal_bank);
}

static uint8_t NEOGEO_USER chap_fix(void)
{
    uint16_t t;
    char buf[8];

    chap_header(2u, "FIX LAYER", "TEXT  PALETTES");
    /*
     * Re-assert the page colour: this chapter swaps FIX pages several
     * times and each swap leaves transparent cells showing the backdrop.
     */
    setBACKDROP(DEMO_BG);
    /*
     * Restate the header tag in the accent palette so it holds against
     * the re-asserted backdrop.  The digits come from chap_header's own
     * running count - spelling them out here is how the header came to
     * read CH.02 while the caption bar read 03.
     */
    demo_fix_puts(2u,  0u, chap_tag(),  2u);
    demo_fix_puts(36u, 0u, chap_tag() + 3, 2u);
    chap_fix_label("FIX = 40x32 CELL OVERLAY");
    snd_cross_to(SOUND_MUSIC_G);

    demo_fix_puts(2u,  6u, "PALETTE 0  STANDARD",   0u);
    demo_fix_puts(2u,  7u, "PALETTE 1  ACCENT",     1u);
    demo_fix_puts(2u,  8u, "PALETTE 2  GREEN",      2u);

    for (t = 0u; t < 180u; t++) {
        digit3(buf, t);
        demo_fix_puts(2u, 13u, buf, (uint8_t)((t / 30u) % 3u));
        if (uframe()) return 1u;
    }

    /* Clear only working rows; avoid reprinting blank strings over art. */
    clear_fix_rect_force(0u, 5u, 40u, 21u);

    chap_fix_label("INFIX 0..3  MULTI-PALETTE");
    chap_fix_infix(0u,  1u,  5u, 0u);
    chap_fix_infix(3u, 27u,  6u, 0u);
    chap_fix_infix(2u,  1u, 11u, 0u);
    chap_fix_infix(1u,  0u, 20u, 0u);
    if (uwait(120u)) return 1u;

    clear_fix_rect_force(0u, 5u, 40u, 21u);
    chap_fix_label("INFIX 4..7  MULTI-PALETTE");
    chap_fix_infix(4u,  4u,  5u, 0u);
    chap_fix_infix(5u,  0u, 11u, 0u);
    chap_fix_infix(6u, 24u, 12u, 0u);
    chap_fix_infix(7u,  4u, 19u, 0u);
    if (uwait(120u)) return 1u;

    clear_fix_rect_force(0u, 5u, 40u, 21u);
    chap_fix_label("INFIX 8..9 + SFIX SHEET");
    chap_fix_infix(8u,  2u,  6u, 0u);
    chap_fix_infix(9u,  4u, 12u, 0u);
    /* The sheet is 16 rows tall; six of them is all that fits above the
     * separator rule. */
    chap_fix_infix(10u, 12u, 18u, 6u);
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

    /* --- 4) Short SSG formant cues --------------------------------- */
    demo_fix_puts(2u, 5u, "4. SSG FORMANT CUES               ", 2u);
    soundSceneReset();                         snd_step();
    soundApplyMix(0x00u, 0x00u, 0x0Eu, 0x00u); snd_step();
    demo_fix_puts(2u, 13u, "SSG FORMANT: READY                ", 1u);
    playSSGVoiceGetReady();                    snd_step();
    if (uwait(90u)) return 1u;
    demo_fix_puts(2u, 13u, "SSG FORMANT: GO                   ", 1u);
    playSSGVoiceLetsGo();                      snd_step();
    if (uwait(90u)) return 1u;
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
        demo_fix_puts(2u, 13u,
                      (i == 0u) ? "FM 4  WARM BELL     " :
                                  "FM 6  BATTLE BRASS  ", 1u);
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
    /* Patch 6 carries maximum pitch-modulation sensitivity.  Its long
     * notes make register $22 rate changes audible without confusing
     * note attacks with vibrato. */
    playFMTrack(SOUND_FM_G);                  snd_step();

    demo_fix_puts(2u, 15u, "LFO OFF      (flat reference)     ", 1u);
    if (fm_lfo_hold(0x00u, 120u)) return 1u;
    demo_fix_puts(2u, 15u, "LFO rate=0   (slow LFO)           ", 1u);
    if (fm_lfo_hold(0x08u, 120u)) return 1u;
    demo_fix_puts(2u, 15u, "LFO rate=4   (medium vibrato)     ", 1u);
    if (fm_lfo_hold(0x0Cu, 120u)) return 1u;
    demo_fix_puts(2u, 15u, "LFO rate=7   (fast vibrato)       ", 1u);
    if (fm_lfo_hold(0x0Fu, 120u)) return 1u;
    soundFMSetLFO(0x00u); snd_step();

    demo_fix_puts(2u, 15u, "TEMPO period=2  (quick)           ", 1u);
    soundStopMusic(); snd_step();
    playFMTrack(SOUND_FM_G); snd_step();
    soundFMSetTempo(2u); snd_step();
    if (uwait(180u)) return 1u;
    demo_fix_puts(2u, 15u, "TEMPO period=5  (slow)            ", 1u);
    soundStopMusic(); snd_step();
    playFMTrack(SOUND_FM_G); snd_step();
    soundFMSetTempo(5u); snd_step();
    if (uwait(180u)) return 1u;

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
    {
        static const uint8_t s_ssg_showcase[3] = {
            SOUND_SSG_A, SOUND_SSG_H, SOUND_SSG_I
        };
        static const uint8_t s_ssg_presets[3] = { 0u, 2u, 0u };
        for (i = 0u; i < 3u; i++) {
            uint8_t track = s_ssg_showcase[i];
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
            playSSGTrack(track);          snd_step();
            soundSetSSGPreset(s_ssg_presets[i]); snd_step();
            if (uwait(360u)) return 1u;
        }
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

    /* --- 9) GAME MIX — streamed bed plus sparse ADPCM-A cues ------- */
    demo_fix_puts(2u, 5u, "9. GAME MIX  MUSIC + SFX          ", 2u);
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0xA8u, 0x00u, 0x00u); snd_step();
    playSFXB(SOUND_TRACK_C);                   snd_step();
    demo_fix_puts(2u, 21u, "LEVEL BED + FOOTSTEP              ", 1u);
    playSFX(SOUND_SFX_5);                      snd_step();
    if (uwait(80u)) return 1u;
    demo_fix_puts(2u, 21u, "LEVEL BED + JUMP                  ", 1u);
    playSFX(SOUND_SFX_7);                      snd_step();
    if (uwait(90u)) return 1u;
    demo_fix_puts(2u, 21u, "LEVEL BED + IMPACT                ", 1u);
    playSFX(SOUND_SFX_8);                      snd_step();
    if (uwait(90u)) return 1u;
    demo_fix_puts(2u, 21u, "LEVEL BED + TAIKO HIT             ", 1u);
    playSFX(SOUND_SFX_10);                     snd_step();
    if (uwait(100u)) return 1u;
    soundFadeOutSpeed(0xFEu); snd_step();
    if (uwait(100u)) return 1u;
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

    /* --- 11) MML MUSIC — one coordinated FM + SSG arrangement ------ */
    demo_fix_puts(2u, 5u, "11. MML MUSIC (driver-native loop) ", 2u);
    demo_fix_puts(2u, 22u, "Listen: FM lead + SSG harmony loop", 0u);
    demo_fix_puts(2u, 23u, "MML D  DUEL THEME                 ", 1u);
    soundStopAll();                            snd_step();
    soundSceneReset();                         snd_step();
    soundApplyMix(0x30u, 0x00u, 0x09u, 0x0Bu); snd_step();
    playMusic(SOUND_MUSIC_D);                  snd_step();
    if (uwait(480u)) return 1u;
    soundFadeOutSpeed(0xFEu); snd_step();
    if (uwait(100u)) return 1u;
    soundStopAll(); snd_step();
    demo_fix_puts(2u, 22u, "                                  ", 0u);
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

/*
 * Hero animation cadence: game frames held per animation frame, so a
 * lower number steps the art more often and reads smoother.
 *
 * These were inline literals that had drifted into two unrelated
 * families - showcase chapters ran stand at t/12 and walk at t/6 while
 * gameplay chapters ran stand at t/14 and walk at t/5 - so the same
 * character visibly idled and walked at different speeds depending on
 * which chapter you were watching, for no design reason.  Unified here
 * on the smoother value of each pair.
 */
#define HERO_CAD_STAND    12u   /* 8 frames -> 1.60s idle cycle  */
#define HERO_CAD_WALK      5u   /* 8 frames -> 0.67s walk cycle  */
#define HERO_CAD_STRIKE    6u
#define HERO_CAD_SPECIAL   6u
#define HERO_CAD_SPECIAL_B 8u

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
static uint8_t s_hero_scale_x = U_SCALE_CHARACTER;
static uint8_t s_hero_scale_y = U_SCALE_CHARACTER;

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
    /* Moveset showcase.  Was U_SCALE_FULL (60.2%), which is both larger
     * than it needs to be and an uneven shrink fraction - exactly the
     * case where the shrink table drops source lines unevenly and the
     * hero's outline goes dashed as she animates.  1/2 is smaller and
     * is an exact binary fraction, so every other line is dropped
     * evenly and the poses stay crisp. */
    hero_scale(U_SCALE_1_2);

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
        case 0: frame = s_hero_stand[(t / HERO_CAD_STAND) % 8u]; break;
        case 1: frame = s_hero_walk[(t / HERO_CAD_WALK) % 8u];   break;
        case 2: frame = s_hero_strike[(t / HERO_CAD_STRIKE) % 8u]; break;
        case 3: frame = s_hero_specA[(t / HERO_CAD_SPECIAL) % 8u];  break;
        default:frame = s_hero_specB[(t / HERO_CAD_SPECIAL_B) % 6u];  break;
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
/*  Chapter 24 — Character Select (portrait animation or joystick)     */
/* ================================================================== */
/*
 * Two portraits shown side by side: the girl's 7-frame idle-animation
 * face (screen ids 68-74, already-imported frames sitting right before
 * the eagle's own frames in the same character sheet) and
 * the eagle's existing ground/flying poses (already used by
 * chap_physics/chap_char_2d, reused here rather than sourcing new art).
 * Plays as an attract-mode demo by default, auto-swapping the
 * selection and animating whichever one is highlighted; the moment the
 * joystick moves left/right, the player owns the cursor for the rest of
 * the chapter (same pattern as the shooter and physics chapters), with
 * the same idle-advance once player-controlled.
 */
static uint8_t NEOGEO_USER chap_char_select(void)
{
    enum {
        IDLE_ADVANCE_FRAMES = 300u,   /* ~5s idle once player-controlled */
        ATTRACT_SWITCH_FRAMES = 150u,
        GIRL_FRAME_COUNT    = 7,
        EAGLE_FRAME_COUNT   = 2,
        SLOT_GIRL           = 40u,
        SLOT_EAGLE          = 56u
    };
    static const uint8_t girl_frames[GIRL_FRAME_COUNT]  = { 68u, 69u, 70u, 71u, 72u, 73u, 74u };
    static const uint8_t eagle_ground                   = 75u;
    /* One bird - see s_flight_frames on why 80 is not in the cycle. */
    static const uint8_t eagle_frames[EAGLE_FRAME_COUNT] = { 78u, 79u };
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

    /* Decorative FIX-layer border around the portrait selection area -
     * this chapter had nothing but bare text before, no frame around
     * the two picks. */
    {
        char rule[35];
        uint8_t p;
        uint8_t row;
        rule[0] = '+';
        for (p = 1u; p < 33u; p++) rule[p] = '-';
        rule[33] = '+';
        rule[34] = '\0';
        demo_fix_puts(3u, 5u, rule, 1u);
        /* ':' and not '|' for the uprights: the FIX tile at the ASCII
         * code for '|' is not a bar in this S-ROM's font and renders as
         * a broken glyph, while ':' gives a clean dotted rule that reads
         * as the vertical partner of the '-' dashes above and below. */
        for (row = 6u; row < 24u; row++) {
            demo_fix_puts(3u,  row, ":", 1u);
            demo_fix_puts(35u, row, ":", 1u);
        }
        demo_fix_puts(3u, 24u, rule, 1u);
    }

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
        } else if ((t % ATTRACT_SWITCH_FRAMES) == 0u) {
            sel = (uint8_t)(sel ^ 1u);
            anim_t = 0u;
            playSFX(SOUND_SFX_5);
        }

        /* Selection readout with the attract-mode shimmer: the chosen
         * name cycles palettes and gets blinking arrow marks, the
         * unchosen one stays flat.  Previously both were static text
         * and the only cue was a pair of square brackets. */
        demo_fix_puts(7u,  8u, "  GIRL  ", (uint8_t)(sel == 0u ? 2u : 1u));
        demo_fix_puts(23u, 8u, " EAGLE  ", (uint8_t)(sel == 1u ? 2u : 1u));
        if (sel == 0u) fix_cycle_puts(7u,  8u, "  GIRL  ", t, 6u);
        else           fix_cycle_puts(23u, 8u, " EAGLE  ", t, 6u);
        fix_select_marks(6u,  8u, 8u, (uint8_t)(sel == 0u), t);
        fix_select_marks(22u, 8u, 8u, (uint8_t)(sel == 1u), t);
        fix_cycle_puts(13u, 6u, "SELECT YOUR FIGHTER", t, 8u);

        girl_frame  = (sel == 0u) ? girl_frames[(anim_t / 8u) % GIRL_FRAME_COUNT]   : girl_frames[0];
        eagle_frame = (sel == 1u) ? eagle_frames[(anim_t / 8u) % EAGLE_FRAME_COUNT] : eagle_ground;
        draw_asset_bottom_center(girl_frame,  SLOT_GIRL,  110, 180,
                                 U_SCALE_5_16, U_SCALE_5_16);
        draw_asset_bottom_center(eagle_frame, SLOT_EAGLE, 210, 180,
                                 U_SCALE_5_16, U_SCALE_5_16);
        anim_t++;
        s_selected_char = sel;

        if (uframe()) return 1u;
    }
    s_selected_char = sel;
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
    /* One bird - see s_flight_frames on why 80 is not in the cycle. */
    static const uint8_t s_eagle_fly[2] = { 78u, 79u };
    static const uint8_t s_eagle_ground = 75u;
    enum {
        EAGLE_STRIDE     = 16,
        EAGLE_OFFSET_X   = 3,
        EAGLE_SCALE      = U_SCALE_5_16,
        EAGLE_BODY_W     = 24,
        EAGLE_BODY_H     = 36,
        EAGLE_START_X    = 160,
        EAGLE_START_Y    = 92,
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

    demo_fix_puts(2u, 2u, "WATCHING - PRESS START TO PLAY", 1u);
    demo_fix_puts(2u, 3u, "SOLID Y=180  BAR Y=184  FEET ABOVE BAR", 0u);
    snd_cross_to(SOUND_MUSIC_A);

    /*
     * The FIX bar occupies Y=184..191.  Physics resolves against Y=180,
     * leaving a clear four-pixel boundary above the glyph row.
     */
    demo_fix_puts(0u, 23u, "========================================", 2u);

    ng_physics_init();
    /* The collider is four pixels above the visible bar, so the eagle's
     * feet settle cleanly on top instead of crossing the FIX glyphs. */
    ng_physics_add_solid(PLATFORM_X, 180, PLATFORM_W, 8, 0u);

    reset_palette_memo();
    demo_load_screen_palette(s_eagle_ground);
    for (i = 0u; i < 2u; i++) demo_load_screen_palette(s_eagle_fly[i]);
    frame = s_eagle_fly[0];

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
    /* Gentle gravity plus a one-pixel terminal velocity makes the
     * fall reads clearly on screen instead of being a brief snap. */
    ng_physics_set_gravity(eagle, NG_FP_FROM_FRAC(1, 64), NG_TO_FP(1));

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
        want = grounded ? s_eagle_ground : s_eagle_fly[(t / 6u) % 2u];

        /*
         * Full re-set (sprite + stride + offset_y) on every frame change.
         * This is the working pattern from demo_sprites.c — it's the only
         * one that doesn't leave the previous frame's strips visible.
         */
        if (eagle->sprite_tile        != DEMO_SCREEN_TILE(want)    ||
            eagle->palette            != DEMO_SCREEN_PALETTE(want) ||
            eagle->sprite_strips      != demo_screen_strips(want)  ||
            eagle->sprite_active_rows != demo_screen_rows(want)) {
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
    cam->mode = NG_CAM_FOLLOW;
    switch (mode) {
    case CAMLAB_MODE_HARD_FOLLOW:
        ng_camera_set_follow_speed(cam, 255u);
        ng_camera_set_dead_zone(cam, 0u, 0u);
        ng_camera_set_look_ahead(cam, 0, 0, 0u);
        break;
    case CAMLAB_MODE_SMOOTH:
        ng_camera_set_follow_speed(cam, 96u);
        ng_camera_set_dead_zone(cam, 0u, 0u);
        ng_camera_set_look_ahead(cam, 0, 0, 0u);
        break;
    case CAMLAB_MODE_SHAKE:
        ng_camera_set_follow_speed(cam, 128u);
        ng_camera_set_dead_zone(cam, 0u, 0u);
        ng_camera_set_look_ahead(cam, 0, 0, 0u);
        ng_camera_shake(cam, 6u, 30u);
        break;
    case CAMLAB_MODE_PAN:
        ng_camera_set_follow_speed(cam, 6u);
        ng_camera_pan_to(cam, pan_dest_x, 0, 32u);
        break;
    case CAMLAB_MODE_VERTICAL:
        ng_camera_set_follow_speed(cam, 128u);
        ng_camera_set_dead_zone(cam, 0u, 0u);
        ng_camera_set_look_ahead(cam, 0, 0, 0u);
        break;
    case CAMLAB_MODE_DEADZONE:
        ng_camera_set_follow_speed(cam, 128u);
        ng_camera_set_dead_zone(cam, 48u, 28u);
        ng_camera_set_look_ahead(cam, 0, 0, 0u);
        break;
    default:
        break;
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

    /* Adjust cy_screen by 16px (2 rows) to compensate for FIX_ROW0 */
    cy_screen -= 16;
    if (cy_screen < 0) cy_screen = 0;

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
    const int16_t  WORLD_BOTTOM = 768;
    /* Was 156 - chap_mini_game uses GROUND_Y=204 against this same
     * U_BG_FOREST background, and 156 left the hero floating ~48px
     * above that same ground line instead of standing on it. */
    const int16_t  HERO_Y_REST  = 188;             /* matches U_BG_FOREST's
                                                    * ground line          */
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

    chap_letterbox_next(7u, 4u);
    chap_header(7u, "CAMERA LAB", "FOLLOW  SHAKE  PAN  VERTICAL  DEADZONE");

    /* Sound: ADPCM-B carries the loop the whole chapter.  FM/SSG muted,
     * ADPCM-A reserved for the per-mode SFX chirps - which is exactly the
     * mix snd_cross_to() sets, and unlike the hand-rolled block that used
     * to be here it fades the outgoing chapter out first and arms the
     * loop watchdog for the full 18s tour. */
    snd_cross_to(SOUND_MUSIC_E);

    ng_camera_init(&cam);
    ng_camera_set_bounds(&cam, 0, 0, WORLD_RIGHT, WORLD_BOTTOM);
    /* Was U_SCALE_60 (39.8%, an uneven fraction) - smaller now, and an
     * exact 3/8 so the shrink drops lines evenly. */
    hero_scale(U_SCALE_3_8);

    for (t = 0u; t < TOTAL_FRAMES; t++) {
        uint8_t  next_mode = (uint8_t)((t / MODE_FRAMES) % CAMLAB_MODE_COUNT);
        uint16_t mode_t    = (uint16_t)(t % MODE_FRAMES);
        uint8_t  frame;
        int16_t  screen_x;
        int16_t  screen_y;

        /* --- transition into a new mode --------------------------- */
        if (next_mode != prev_mode) {
            mode = next_mode;
            camlab_apply_mode(&cam, mode, (int16_t)(player_world_x - 112));
            /* Snap player Y back to the resting band whenever the
             * vertical mode releases. */
            if (prev_mode == CAMLAB_MODE_VERTICAL) {
                player_world_y = HERO_Y_REST;
                ng_camera_snap(&cam, cam.x, 0);
            }
            playSFX(SOUND_SFX_5);
            prev_mode = mode;
        }

        /* --- scripted X movement: 2-second swing -------------------
         * Skip in PAN mode so the camera drift to the centre isn't
         * masked by the player chasing it. */
        vx_logical = 0;
        if (mode != CAMLAB_MODE_PAN && mode != CAMLAB_MODE_VERTICAL) {
            uint16_t phase = (uint16_t)(t % 600u);
            if (phase < 300u) {
                vx_logical = +2;
                if (player_world_x < WORLD_RIGHT - 80) player_world_x += 2;
            } else {
                vx_logical = -2;
                if (player_world_x > 80) player_world_x -= 2;
            }
        }

        /* --- scripted Y movement only during VERTICAL mode ---------
         * 130..220 sweep makes the camera follow vertically while the
         * floor BG slides up/down behind the player. */
        vy_logical = 0;
        if (mode == CAMLAB_MODE_VERTICAL) {
            vy_logical = mode_t < 150u ? 2 : 0;
            player_world_y = (int16_t)(player_world_y + vy_logical);
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
                ng_camera_pan_to(&cam, (int16_t)(player_world_x - 112), 0, 32u);
                playSFX(SOUND_SFX_7);
            }
            if (mode_t == 110u) {
                ng_camera_pan_to(&cam, (int16_t)(player_world_x - 160), 0, 32u);
                playSFX(SOUND_SFX_7);
            }
        }

        /* --- camera update ---------------------------------------- */
        /* Follow a point inside the actor, not its feet. Horizontal modes
         * keep the stage floor fixed; vertical mode follows between pages. */
        ng_camera_update(&cam, player_world_x,
            mode == CAMLAB_MODE_VERTICAL ? (int16_t)(player_world_y - 76) : 112,
            vx_logical);

        /* --- background ------------------------------------------- */
        if (mode == CAMLAB_MODE_VERTICAL) {
            draw_level_background((uint16_t)(cam.y < 0 ? 0 : cam.y), 1u);
        } else {
            draw_level_background((uint16_t)(cam.x < 0 ? 0 : cam.x), 0u);
        }

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
        digit3(hud, (uint16_t)(1u + (mode == CAMLAB_MODE_VERTICAL
                                    ? (uint16_t)cam.y / 176u : (uint16_t)(cam.x < 0 ? 0 : cam.x) / 256u)));
        demo_fix_puts(2u, 24u, "LEVEL", 1u);
        demo_fix_puts(8u, 24u, hud, 2u);
        demo_fix_puts(18u, 24u, "LIFE [##########]", 1u);

        /* Dead-zone box only on the dead-zone mode itself. */
        if (mode == CAMLAB_MODE_DEADZONE) {
            camlab_deadzone_box(160,
                                140,
                                cam.dead_zone_x,
                                cam.dead_zone_y);
        }

        /* --- player draw ----------------------------------------- */
        frame = (vx_logical || vy_logical)
              ? s_hero_walk [(t / 6u)  % 8u]
              : s_hero_stand[(t / HERO_CAD_STAND) % 8u];
        {
            int16_t saved_x = s_hero_x;
            int16_t saved_y = s_hero_y;
            s_hero_x = screen_x;
            s_hero_y = screen_y;
            draw_asset_bottom_center(frame, HERO_SLOT_FIRST, screen_x,
                                     screen_y, U_SCALE_3_8, U_SCALE_3_8);
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
    const uint16_t *base_pal = ng_get_screen_palette(s_fx_effect_frames[0]);

    chap_header(8u, "PALETTE FX", "SPRITE PALETTE STAGES");
    /* One-shot hard clear of the effect and lower-hero sprite windows before
     * the first pose draw — kills any stale strips left from prior chapters. */
    ng_sprite_park_off_range(48u, 16u);
    ng_sprite_park_off_range(HERO_SLOT_FIRST, 16u);
    demo_fix_puts(2u, 2u, "EFFECTS: 040 / 041 / 048 / 050", 1u);
    demo_fix_puts(2u, 3u, "CHAR: 033R04C03 -> 037R04C07", 0u);
    snd_cross_to(SOUND_MUSIC_G);

    demo_load_screen_palette(s_fx_effect_frames[0]);
    demo_load_screen_palette(s_fx_effect_frames[1]);
    demo_load_screen_palette(s_fx_effect_frames[2]);
    demo_load_screen_palette(s_fx_effect_frames[3]);
    demo_fix_puts(2u, 6u, "ACTIVE:", 1u);

    /* Was 660 frames (11s) at 132 per stage - trimmed to 8s. */
    for (t = 0u; t < 480u; t++) {
        uint8_t next_stage = (uint8_t)(t / 96u);
        uint8_t pose = s_fx_char_frames[(t / 12u) % 5u];

        if (next_stage != stage) {
            ng_palfx_stop(active_pal);
            demo_load_screen_palette(s_fx_effect_frames[0]);
            demo_load_screen_palette(s_fx_effect_frames[1]);
            demo_load_screen_palette(s_fx_effect_frames[2]);
            demo_load_screen_palette(s_fx_effect_frames[3]);
            stage = next_stage;
            active_fx = s_fx_effect_frames[stage & 3u];
            active_pal = DEMO_SCREEN_PALETTE(active_fx);
            base_pal = ng_get_screen_palette(active_fx);
            if (!base_pal) return 0u;

            switch (stage) {
            case 0u:
                demo_fix_puts(10u, 6u, "FADE IN          ", 2u);
                ng_palfx_fade_in(active_pal, base_pal, 72u);
                break;
            case 1u:
                demo_fix_puts(10u, 6u, "WHITE FLASH      ", 2u);
                ng_palfx_flash_white(active_pal, base_pal, 28u);
                break;
            case 2u:
                demo_fix_puts(10u, 6u, "RED HIT FLASH    ", 2u);
                ng_palfx_flash_red(active_pal, base_pal, 28u);
                break;
            case 3u:
                demo_fix_puts(10u, 6u, "PULSE            ", 1u);
                ng_palfx_pulse(active_pal, base_pal, 32u);
                break;
            default:
                demo_fix_puts(10u, 6u, "COLOR CYCLE      ", 1u);
                ng_palfx_cycle(active_pal, base_pal, 8u, 14u);
                break;
            }
        }

        demo_load_screen_palette(pose);
        /* Draw aura/effect at slot 48 behind character at slot 64 so it does not occlude the hero */
        draw_asset_center(active_fx, 48u,
                           95, 106, U_SCALE_3_8, U_SCALE_3_8);
        draw_asset_bottom_center(pose, HERO_SLOT_FIRST, 224,
                                 FX_HERO_LIFT_Y,
                                 U_SCALE_3_8, U_SCALE_3_8);

        if (uframe()) {
            ng_sprite_park_off_range(48u, 16u);
            ng_sprite_park_off_range(HERO_SLOT_FIRST, 16u);
            return 1u;
        }
    }

    ng_palfx_stop(active_pal);
    ng_sprite_park_off_range(48u, 16u);
    ng_sprite_park_off_range(HERO_SLOT_FIRST, 16u);
    return 0u;
}

/* ================================================================== */
/*  Chapter 09 — Particles                                              */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_particles(void)
{
    uint16_t t;
    const uint8_t impact_id  = s_fx_char_frames[4];
    /* Dedicated art per particle kind (U_PARTICLE_*) instead of one
     * reused effect strip standing in for dust/magic/spark/explosion/
     * smoke alike - see the U_PARTICLE_* block near the top of the
     * file. */
    const uint16_t dust_tile = DEMO_SCREEN_TILE(U_PARTICLE_DUST);
    const uint8_t  dust_pal  = DEMO_SCREEN_PALETTE(U_PARTICLE_DUST);
    const uint16_t magic_tile = DEMO_SCREEN_TILE(U_PARTICLE_MAGIC);
    const uint8_t  magic_pal  = DEMO_SCREEN_PALETTE(U_PARTICLE_MAGIC);
    const uint16_t hit_tile  = DEMO_SCREEN_TILE(U_PARTICLE_HITSPARK);
    const uint8_t  hit_pal   = DEMO_SCREEN_PALETTE(U_PARTICLE_HITSPARK);
    const uint16_t boom_tile = DEMO_SCREEN_TILE(U_PARTICLE_EXPLOSION);
    const uint8_t  boom_pal  = DEMO_SCREEN_PALETTE(U_PARTICLE_EXPLOSION);
    const uint16_t smoke_tile = DEMO_SCREEN_TILE(U_PARTICLE_SMOKE);
    const uint8_t  smoke_pal  = DEMO_SCREEN_PALETTE(U_PARTICLE_SMOKE);

    chap_header(9u, "PARTICLES", "HERO SPECIAL MOVE + FX");
    /* Same hard wipe as the palette FX chapter so the lower hero
     * starts on a clean window — no stale strips left over when the
     * next pose is narrower than the previous one. */
    ng_sprite_park_off_range(HERO_SLOT_FIRST, 16u);
    demo_fix_puts(2u, 2u, "EFFECTS STAY ABOVE  CHARACTER BELOW", 1u);
    demo_fix_puts(2u, 3u, "DUST / MAGIC / SPARK / EXPLOSION / SMOKE",0u);
    demo_fix_puts(2u, 4u, "ACTIVE: ",                    2u);
    snd_cross_to(SOUND_MUSIC_F);

    hero_place(160, FX_HERO_LIFT_Y);
    demo_load_screen_palette(impact_id);
    demo_load_screen_palette(U_PARTICLE_DUST);
    demo_load_screen_palette(U_PARTICLE_MAGIC);
    demo_load_screen_palette(U_PARTICLE_HITSPARK);
    demo_load_screen_palette(U_PARTICLE_EXPLOSION);
    demo_load_screen_palette(U_PARTICLE_SMOKE);

    s_draw_particles = 1u;

    for (t = 0u; t < 540u; t++) {
        /*
         * Phase machine — 3 special-move beats:
         *   [  0..160) WIND-UP        : faint dust around feet
         *   [160..360) STRIKE         : magic sparks burst from sword arc
         *   [360..540) FINISHER       : explosion + smoke ring + screen-flash
         *
         * bx/by track the moving strike point.  Was centred around
         * y=78..98, ~80px above where the hero actually stands
         * (FX_HERO_LIFT_Y) - the sparks read as floating disconnected
         * over the character's head instead of coming from their
         * weapon.  Rebased near the character's own height instead.
         */
        uint8_t hero_frame;
        int16_t bx = (int16_t)(92 + (int16_t)((t * 2u) % 136u));
        int16_t by = (int16_t)(FX_HERO_LIFT_Y - 34 + (int16_t)((t & 31u) >> 3));

        if (t < 160u) {
            hero_frame = s_fx_char_frames[(t / 12u) % 5u];
            if ((t % 12u) == 0u) {
                ng_particle_spawn(NG_PART_DUST, NG_PART_PRI_NORMAL,
                                  (int16_t)(126 + (int16_t)(t & 15u)),
                                  FX_HERO_LIFT_Y,
                                  -(1L << (NG_FP_SHIFT - 1)),
                                  -(1L << (NG_FP_SHIFT - 1)),
                                  20u, dust_tile, dust_pal, 1u, 1u);
                ng_particle_spawn(NG_PART_DUST, NG_PART_PRI_NORMAL,
                                  (int16_t)(178 - (int16_t)(t & 15u)),
                                  FX_HERO_LIFT_Y,
                                  (1L << (NG_FP_SHIFT - 1)),
                                  -(1L << (NG_FP_SHIFT - 1)),
                                  20u, dust_tile, dust_pal, 1u, 1u);
            }
            if (t == 8u) playSFX(SOUND_SFX_9);
        } else if (t < 360u) {
            hero_frame = s_fx_char_frames[((t / 10u) + 1u) % 5u];
            if ((t % 10u) == 0u) {
                ng_particle_spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_CRITICAL,
                                  bx, by,
                                  (3L << NG_FP_SHIFT),
                                  -(2L << NG_FP_SHIFT),
                                  24u, magic_tile, magic_pal, 1u, 1u);
                ng_particle_spawn(NG_PART_HIT_SPARK, NG_PART_PRI_CRITICAL,
                                  (int16_t)(bx - 6), (int16_t)(by - 4),
                                  -(1L << NG_FP_SHIFT),
                                  -(3L << NG_FP_SHIFT),
                                  22u, hit_tile, hit_pal, 1u, 1u);
                ng_particle_spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_NORMAL,
                                  (int16_t)(bx + 8), (int16_t)(by + 6),
                                  (2L << NG_FP_SHIFT),
                                  (1L << NG_FP_SHIFT),
                                  22u, magic_tile, magic_pal, 1u, 1u);
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
                                  160, (int16_t)(FX_HERO_LIFT_Y - 30), 0, 0, 36u,
                                  boom_tile, boom_pal, 1u, 1u);
                for (k = 0u; k < 8u; k++) {
                    int32_t a = (int32_t)k * 2L;
                    ng_particle_spawn(NG_PART_SMOKE, NG_PART_PRI_NORMAL,
                                      160, (int16_t)(FX_HERO_LIFT_Y - 30),
                                      (a - 8L) << (NG_FP_SHIFT - 1),
                                      -(a) << (NG_FP_SHIFT - 1),
                                      40u, smoke_tile, smoke_pal, 1u, 1u);
                }
            }
            if ((t % 24u) == 0u) {
                ng_particle_spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_NORMAL,
                                  (int16_t)(160 + ((int16_t)(t & 31u) - 16)),
                                  (int16_t)(FX_HERO_LIFT_Y - 42),
                                  0, -(1L << NG_FP_SHIFT),
                                  30u, magic_tile, magic_pal, 1u, 1u);
            }
        }

        draw_asset_bottom_center(hero_frame, HERO_SLOT_FIRST,
                                 160, FX_HERO_LIFT_Y,
                                 U_SCALE_3_8, U_SCALE_3_8);

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
    const uint16_t dust_tile  = DEMO_SCREEN_TILE(U_PARTICLE_DUST);
    const uint8_t  dust_pal   = DEMO_SCREEN_PALETTE(U_PARTICLE_DUST);
    const uint16_t boom_tile  = DEMO_SCREEN_TILE(U_PARTICLE_EXPLOSION);
    const uint8_t  boom_pal   = DEMO_SCREEN_PALETTE(U_PARTICLE_EXPLOSION);
    const uint16_t spark_tile = DEMO_SCREEN_TILE(U_PARTICLE_HITSPARK);
    const uint8_t  spark_pal  = DEMO_SCREEN_PALETTE(U_PARTICLE_HITSPARK);
    const uint16_t magic_tile = DEMO_SCREEN_TILE(U_PARTICLE_MAGIC);
    const uint8_t  magic_pal  = DEMO_SCREEN_PALETTE(U_PARTICLE_MAGIC);
    const uint16_t smoke_tile = DEMO_SCREEN_TILE(U_PARTICLE_SMOKE);
    const uint8_t  smoke_pal  = DEMO_SCREEN_PALETTE(U_PARTICLE_SMOKE);
    uint16_t t;
    char cnt[10];

    chap_header(23u, "PARTICLE LOAD", "POOL STRESS + PRIORITY EVICTION");
    demo_fix_puts(2u, 2u, "BURST  AMBIENT  EXPLOSION  POOL FLOOD", 1u);
    demo_fix_puts(2u, 3u, "OPTIONAL PARTICLES DROP GRACEFULLY", 0u);
    snd_cross_to(SOUND_MUSIC_F);
    demo_load_screen_palette(U_PARTICLE_DUST);
    demo_load_screen_palette(U_PARTICLE_EXPLOSION);
    demo_load_screen_palette(U_PARTICLE_HITSPARK);
    demo_load_screen_palette(U_PARTICLE_MAGIC);
    demo_load_screen_palette(U_PARTICLE_SMOKE);

    s_draw_particles = 1u;

    for (t = 0u; t < 560u; t++) {
        if (t < 140u) {
            /* Phase 1: 8-way hit-spark bursts. */
            demo_fix_puts(2u, 5u, "PHASE: IMPACT BURSTS ", 2u);
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
            /* Phase 2: particles fall from the screen centre. */
            demo_fix_puts(2u, 5u, "PHASE: CENTRE RAIN    ", 1u);
            if ((t % 8u) == 0u) {
                int16_t dx = (int16_t)(160 + (int16_t)((int16_t)(t & 31u) - 16));
                ng_particle_spawn(NG_PART_DUST, NG_PART_PRI_NORMAL,
                                  dx, 88, 0, 1L << (NG_FP_SHIFT - 1),
                                  48u, dust_tile, dust_pal, 1u, 1u);
                if ((t % 24u) == 0u) {
                    ng_particle_spawn(NG_PART_SMOKE, NG_PART_PRI_NORMAL,
                                      (int16_t)(dx + 12), 80,
                                      0, 1L << (NG_FP_SHIFT - 2),
                                      56u, smoke_tile, smoke_pal, 1u, 1u);
                }
            }
        } else if (t < 420u) {
            /* Phase 3: explosion + fanned slash sparks. */
            demo_fix_puts(2u, 5u, "PHASE: EXPLOSION FAN  ", 2u);
            if ((t % 60u) == 0u) {
                uint8_t k;
                int16_t ex = (int16_t)(120 + (int16_t)((t * 5u) & 0x3Fu));
                ng_particle_spawn(NG_PART_EXPLOSION, NG_PART_PRI_CRITICAL,
                                  ex, 96, 0, 0, 36u,
                                  boom_tile, boom_pal, 1u, 1u);
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
                                  magic_tile, magic_pal, 1u, 1u);
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
    const uint8_t  spark_id   = U_PARTICLE_HITSPARK;
    const uint16_t spark_tile = DEMO_SCREEN_TILE(spark_id);
    const uint8_t  spark_pal  = DEMO_SCREEN_PALETTE(spark_id);
    const uint16_t magic_tile = DEMO_SCREEN_TILE(U_PARTICLE_MAGIC);
    const uint8_t  magic_pal  = DEMO_SCREEN_PALETTE(U_PARTICLE_MAGIC);
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

    hero_scale(U_SCALE_CHARACTER);
    hero_place(160, FX_HERO_LIFT_Y);
    demo_load_screen_palette(spark_id);
    demo_load_screen_palette(U_PARTICLE_MAGIC);

    s_draw_particles = 1u;

    for (t = 0u; t < 540u; t++) {
        uint8_t hero_frame = s_hero_stand[(t / HERO_CAD_STAND) % 8u];
        int16_t bob = (int16_t)(((t & 31u) < 16u) ? 1 : -1);

        if (t == 28u || t == 148u || t == 288u || t == 428u) {
            ng_particle_spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_CRITICAL,
                              32, 116, 4L << NG_FP_SHIFT, 0,
                              38u, magic_tile, magic_pal, 1u, 1u);
        }

        /* Impact bursts land on the hero's HEAD (~s_hero_y - 64), not
         * on the body — that's where it reads visually as a "hit". */
        if (t == 60u && fired < 1u) {
            ng_feedback_shake(&cam, 1u, 8u);
            playSFX(SOUND_SFX_8);
            spawn_impact_burst(160, 116, spark_tile, spark_pal, 1u);
            demo_fix_puts(2u, 5u, "FIRED: LIGHT   ", 1u); fired = 1u;
        } else if (t == 180u && fired < 2u) {
            ng_feedback_shake(&cam, 2u, 12u);
            playSFX(SOUND_SFX_8);
            spawn_impact_burst(160, 116, spark_tile, spark_pal, 2u);
            demo_fix_puts(2u, 5u, "FIRED: MEDIUM  ", 2u); fired = 2u;
        } else if (t == 320u && fired < 3u) {
            ng_feedback_shake(&cam, 3u, 16u);
            playSFX(SOUND_SFX_8);
            spawn_impact_burst(160, 116, spark_tile, spark_pal, 3u);
            demo_fix_puts(2u, 5u, "FIRED: HEAVY   ", 2u); fired = 3u;
        } else if (t == 460u && fired < 4u) {
            ng_feedback_shake(&cam, 4u, 20u);
            playSFX(SOUND_SFX_10);
            spawn_impact_burst(160, 116, spark_tile, spark_pal, 4u);
            demo_fix_puts(2u, 5u, "FIRED: BOSS    ", 2u); fired = 4u;
        }

        ng_camera_update(&cam, 160, 112, 0);
        draw_asset_bottom_center(hero_frame, HERO_SLOT_FIRST,
                                 (int16_t)(160 + bob + cam.shake_offset_x),
                                 (int16_t)(FX_HERO_LIFT_Y + cam.shake_offset_y),
                                 U_SCALE_3_8, U_SCALE_3_8);
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

    chap_letterbox_next(5u, 3u);
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
        uint8_t frame = s_hero_stand[(t / HERO_CAD_STAND) % 8u];
        uint8_t strips = demo_screen_strips(frame);
        uint8_t rows = demo_screen_rows(frame);
        /*
         * Z-to-scale ramp.  Was 0x58 + (96-z)*0xA7/84, which reaches
         * 0xFF at the near end - the hardware maximum, i.e. the art at
         * its full 256px, towering over the screen when she approaches.
         * 0x40..0xA0 keeps the same sense of depth (a 2.5x swing between
         * far and near) with the near end at a sane ~63%.
         */
        uint8_t scale = (uint8_t)(0x30u + (uint16_t)((96 - z) * 0x4fu) / 84u);
        int16_t draw_x = (int16_t)(160 - (strips * 16 * scale / 256) / 2
                                  - demo_screen_x_offset(frame));
        int16_t draw_y = (int16_t)(124 - (rows * 16 * scale / 256) / 2
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
        SLOT_FAR_A  = NG_SPR_BG0_FIRST,
        SLOT_FAR_B  = NG_SPR_BG1_FIRST,
        SLOT_NEAR_A = 40,
        SLOT_NEAR_B = 56
    };
    uint8_t far_strips  = demo_screen_strips(U_BG_MOUNTAIN);
    uint8_t far_rows    = demo_screen_rows(U_BG_MOUNTAIN);
    uint8_t near_strips = demo_screen_strips(U_BG_FOREGROUND);
    uint8_t near_rows   = demo_screen_rows(U_BG_FOREGROUND);
    uint16_t t;

    chap_letterbox_next(4u, 4u);
    chap_header(22u, "DEPTH PARALLAX", "TWO SCROLLING BG LAYERS");
    demo_fix_puts(2u, 2u, "MOUNTAIN FAR  FOREST FOREGROUND", 1u);
    demo_fix_puts(2u, 3u, "NEAR LAYER MOVES 2X + VERTICAL REVEAL", 0u);
    snd_cross_to(SOUND_MUSIC_A);
    demo_load_screen_palette(U_BG_MOUNTAIN);
    demo_load_screen_palette(U_BG_FOREGROUND);
    for (t = 0u; t < 360u; t++) {
        uint16_t phase = (uint16_t)(t % 180u);
        int16_t far_x = -(int16_t)(t / 4u);
        int16_t near_x = -(int16_t)(t / 2u);
        int16_t far_y = -(int16_t)((phase < 90u) ? phase / 6u
                                                        : (180u - phase) / 6u);
        int16_t near_y = (int16_t)(-16 - ((phase < 90u) ? phase / 3u
                                                              : (180u - phase) / 3u));
        demo_draw_sprite_screen(U_BG_MOUNTAIN, SLOT_FAR_A, far_x, far_y,
                                far_strips, far_rows, 0xFFu, 0xFFu);
        demo_draw_sprite_screen(U_BG_MOUNTAIN, SLOT_FAR_B,
                                (int16_t)(far_x + 256), far_y,
                                far_strips, far_rows, 0xFFu, 0xFFu);

        demo_draw_sprite_screen(U_BG_FOREGROUND, SLOT_NEAR_A, near_x, near_y,
                                near_strips, near_rows, 0xFFu, 0xFFu);
        demo_draw_sprite_screen(U_BG_FOREGROUND, SLOT_NEAR_B,
                                (int16_t)(near_x + 256), near_y,
                                near_strips, near_rows, 0xFFu, 0xFFu);

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
    enum { NPC_FLOOR_Y = 190 };
    /*
     * 80 px apart, patrolling 22 either side.  At 64 apart and 44 either
     * side the lanes overlapped by 24 px, so neighbouring NPCs walked
     * through each other and the middle of the line read as one smeared
     * sprite.  Two NPCs stay 36 px apart at their closest here, which
     * clears the widest of these at this scale.
     */
    static const int16_t home_x[N] = { 40, 120, 200, 280 };
    NGNpc *npcs[N];
    uint8_t last_asset[N];
    uint16_t t;
    uint8_t i;

    chap_header(12u, "NPCS", "PATROL + THINK CALLBACK");
    demo_fix_puts(2u, 2u, "RESTORED 4-FRAME ARCADE WALK CYCLES", 1u);
    demo_fix_puts(2u, 3u, "ENGINE THINK FN HANDLES PATROL + FACING", 0u);
    snd_cross_to(SOUND_MUSIC_B);
    ng_npcs_init();
    reset_palette_memo();

    for (i = 0u; i < (uint8_t)N; i++) {
        NGCharacter *c;
        uint8_t asset = (uint8_t)(U_NPC_OLD_FIRST + 4u + (i & 3u));
        last_asset[i] = asset;

        /* npc_kind = char_kind = i (unique) so chars_find / chars_at work */
        npcs[i] = npc_spawn((uint8_t)i, (uint8_t)i, home_x[i], NPC_FLOOR_Y);
        if (!npcs[i]) continue;
        c = npc_char(npcs[i]);
        if (!c) continue;

        bind_character_asset(c, asset, U_SCALE_1_2, U_SCALE_1_2);
        c->vx_fp = (i & 1u) ? NG_TO_FP(-1) : NG_TO_FP(1);
        ng_npc_set_home(npcs[i], home_x[i], NPC_FLOOR_Y);
        ng_npc_set_patrol_bounds(npcs[i],
                                 (int16_t)(home_x[i] - 22),
                                 (int16_t)(home_x[i] + 22),
                                 NPC_FLOOR_Y, NPC_FLOOR_Y);
        npcs[i]->flags = NG_NPC_FLAG_PATROL_X | NG_NPC_FLAG_FACE_MOTION;
        ng_npc_set_think(npcs[i], ng_npc_think_patrol, 2u);
    }

    s_draw_chars = 1u;

    for (t = 0u; t < 480u; t++) {
        /* Bind the left/right cycle selected by the live facing state. */
        for (i = 0u; i < (uint8_t)N; i++) {
            NGCharacter *c;
            uint8_t phase;
            uint8_t asset;
            if (!npcs[i]) continue;
            c = npc_char(npcs[i]);
            if (!c) continue;
            phase = (uint8_t)((t / 8u + i) & 3u);
            asset = (uint8_t)((c->facing ? U_NPC_OLD_FIRST + 8u
                                         : U_NPC_OLD_FIRST + 4u) + phase);
            if (!npcs[i] || asset == last_asset[i]) continue;
            bind_character_asset(c, asset, U_SCALE_1_2, U_SCALE_1_2);
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

    /* Was a reused generic effect frame (93) - now the same dedicated
     * hit-spark particle art used in chap_particles, instead of a
     * different-but-still-generic sprite standing in for an impact. */
    const uint16_t spark_tile  = DEMO_SCREEN_TILE(U_PARTICLE_HITSPARK);
    const uint8_t  spark_pal   = DEMO_SCREEN_PALETTE(U_PARTICLE_HITSPARK);
    const int16_t  GROUND_Y    = 188;
    const uint16_t TOTAL       = 1800u;
    const uint16_t AGGRO_AFTER = 45u;
    const int16_t  REACH_PX    = 70;
    const uint8_t  CLONE_SLOT  = 64u;
    const uint8_t  PLAYER_SLOT = 80u;

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

    chap_letterbox_next(5u, 3u);
    chap_header(13u, "MINI-GAME", "FIGHT THE CLONE  B STRIKE");
    demo_fix_puts(2u, 2u, "CP CLOSES IN, STRIKES, RECOVERS", 1u);
    demo_fix_puts(2u, 3u, "PLAYER CAN STILL WALK + B",       0u);
    snd_cross_to(SOUND_MUSIC_D);

    draw_background(U_BG_FOREST, 32, 16);
    hero_scale(U_SCALE_CHARACTER);
    ng_joystick_init();
    demo_load_screen_palette(U_PARTICLE_HITSPARK);
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
        case 0:  p_frame = s_hero_stand[(t / HERO_CAD_STAND) % 8u]; break;
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
            c_frame = s_hero_stand[(t / HERO_CAD_STAND) % 8u];
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
            c_frame = s_hero_walk[(t / HERO_CAD_WALK) % 8u];
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
            c_frame = s_hero_stand[(t / HERO_CAD_STAND) % 8u];
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
         * Render clone at slot 64, player at slot 80.  On Neo Geo hardware,
         * higher slot numbers render in front, so the player stays cleanly
         * in front of the clone and backdrop.
         * ============================================================ */
        draw_asset_bottom_center_flip(c_frame, CLONE_SLOT,
                                      c_x, GROUND_Y,
                                      U_SCALE_CHARACTER, U_SCALE_CHARACTER, c_flip);
        draw_asset_bottom_center_flip(p_frame, PLAYER_SLOT,
                                      p_x, GROUND_Y,
                                      U_SCALE_CHARACTER, U_SCALE_CHARACTER, p_flip);

        /* Game over on HP exhausted */
        if (hp == 0u) {
            demo_fix_puts(13u, 13u, "  KNOCKED OUT  ", 2u);
            if (demo_wait(120u)) {
                ng_sprite_park_off_range(64u, 32u);
                return 1u;
            }
            ng_sprite_park_off_range(64u, 32u);
            return 0u;
        }

        if (uframe()) {
            ng_sprite_park_off_range(64u, 32u);
            return 1u;
        }
    }
    ng_sprite_park_off_range(64u, 32u);
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
    int16_t  hero_world_y = 194;
    const int16_t HERO_GROUND_Y = 188;
    uint8_t  hero_flip = 0u;
    int16_t  vy = 0;
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
    uint8_t  box_flash = 0u;     /* >0 briefly after a hit - scale pulse */
    char buf[8];

    chap_letterbox_next(5u, 3u);
    chap_header(14u, "JOYSTICK", "LIVE INPUT  TWO-BUTTON SPECIALS");
    /* The one chapter that cannot take the global C restart: C is jump
     * here, and B+C is one of the two-button specials this chapter
     * exists to demonstrate.  Restarting on C would make both
     * impossible to show. */
    s_restart_enabled = 0u;
    chap_hint("A:NEXT ONLY");
    demo_fix_puts(2u, 2u, "ARROWS MOVE  B STRIKE  C JUMP",   1u);
    demo_fix_puts(2u, 3u, "B+C TOGETHER SPECIAL  B+D FINISHER", 0u);
    snd_cross_to(SOUND_MUSIC_A);

    ng_joystick_init();
    hero_scale(U_SCALE_CHARACTER);

    /* Static HUD labels — once */
    /* Frame the live-input readout as its own panel so it reads as an
     * instrument rather than loose text floating over the forest. */
    {
        char rule[18];
        uint8_t p;
        uint8_t row;
        rule[0] = '+';
        for (p = 1u; p < 16u; p++) rule[p] = '-';
        rule[16] = '+';
        rule[17] = '\0';
        demo_fix_puts(1u, 4u, rule, 1u);
        /* Interior spans rows 5..13 - the B+C / B+D combo readouts sit
         * on 12 and 13, so the closing rule goes on 14. */
        for (row = 5u; row <= 13u; row++) {
            demo_fix_puts(1u,  row, ":", 1u);
            demo_fix_puts(17u, row, ":", 1u);
        }
        demo_fix_puts(1u, 14u, rule, 1u);
    }

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

    demo_load_screen_palette(U_CRATE);
    demo_load_screen_palette(U_CRATE_BROKEN);
    demo_load_screen_palette(U_PARTICLE_HITSPARK);
    draw_background(U_BG_FOREST, 32, 16);
    s_draw_particles = 1u;

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

        /* Hit-test window for the box target.  Was x=276 (nearly at the
         * 320px screen edge) - pulled in to 250 so there's clearly
         * visible room around it.  Must stay in sync with the box's
         * own draw position below. */
        if ((strike_t >= 6u && strike_t <= 18u) || special_t >= 6u) {
            int16_t target_cx_px = 250;
            int16_t target_cy_px = HERO_GROUND_Y;
            int16_t reach_left  = (int16_t)(hero_world_x - 64);
            int16_t reach_right = (int16_t)(hero_world_x + 64);
            int16_t dy = (int16_t)(hero_world_y - target_cy_px);
            if (dy < 0) dy = (int16_t)(-dy);
            if (dy < 64 && target_cx_px >= reach_left && target_cx_px <= reach_right) {
                if (hits < 99u) hits++;
                inactive = 0u;
                playSFX(SOUND_SFX_8);
                spawn_impact_burst(target_cx_px,
                                   (int16_t)(HERO_GROUND_Y - 28),
                                   DEMO_SCREEN_TILE(U_PARTICLE_HITSPARK),
                                   DEMO_SCREEN_PALETTE(U_PARTICLE_HITSPARK),
                                   3u);
                box_flash = 14u;
                strike_t = 0u;   /* one hit per swing — finish the strike */
                special_t = 0u;
            }
        }
        if (strike_t  > 0u) strike_t--;
        if (special_t > 0u) special_t--;
        if (box_flash  > 0u) box_flash--;

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
            frame = s_hero_walk[(t / HERO_CAD_WALK) % 8u];   /* jump pose */
        else if (down & (JOY_LEFT | JOY_RIGHT))
            frame = s_hero_walk[(t / HERO_CAD_WALK) % 8u];
        else
            frame = s_hero_stand[(t / HERO_CAD_STAND) % 8u];

        /* HITS counter on HUD */
        {
            char hb[4];
            hb[0] = (char)('0' + (hits / 10u));
            hb[1] = (char)('0' + (hits % 10u));
            hb[2] = '\0';
            demo_fix_puts(8u, 26u, hb, 1u);
        }

        draw_asset_bottom_center_flip(frame, HERO_SLOT_FIRST,
                                      hero_world_x, hero_world_y,
                                      U_SCALE_CHARACTER, U_SCALE_CHARACTER,
                                      hero_flip);

        draw_asset_bottom_center(box_flash ? U_CRATE_BROKEN : U_CRATE,
                                 DEMO_PROP_HITBOX_SLOT, 250, HERO_GROUND_Y,
                                 U_SCALE_3_8, U_SCALE_3_8);

        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 15 — Scrolling level  (camera follows hero across world)     */
/* ================================================================== */
static uint8_t NEOGEO_USER chap_scrolling_level(void)
{
    uint16_t t;
    uint8_t previous_phase = 0xffu;

    chap_letterbox_next(5u, 4u);
    chap_header(15u, "SCROLL LEVEL", "WORLD MAP  H/V STAGES");
    demo_fix_puts(2u, 2u, "MOUNTAIN -> FOREST -> VERTICAL CLIMB", 1u);
    demo_fix_puts(2u, 3u, "CONNECTED PAGES  HORIZONTAL / VERTICAL", 0u);
    snd_cross_to(SOUND_MUSIC_B);
    demo_load_screen_palette(U_BG_MOUNTAIN);
    demo_load_screen_palette(U_BG_FOREST);

    hero_scale(U_SCALE_CHARACTER);
    demo_fix_puts(2u, 24u, "LEVEL 1  HP [####################]", 1u);
    demo_fix_puts(2u, 25u, "ROAD: MOUNTAIN HORIZONTAL", 2u);

    for (t = 0u; t < 720u; t++) {
        uint8_t phase = (uint8_t)(t / 240u);
        uint16_t phase_t = (uint16_t)(t % 240u);
        int16_t jump = 0;
        int16_t hero_bottom = 188;
        uint8_t frame;
        char bar[21];
        uint8_t fill;
        uint8_t k;

        if (phase != previous_phase) {
            previous_phase = phase;
            if (phase == 0u) {
                demo_fix_puts(2u, 24u, "LEVEL 1  MOUNTAIN PASS", 1u);
                demo_fix_puts(2u, 25u, "ROAD: HORIZONTAL       ", 2u);
            } else if (phase == 1u) {
                demo_fix_puts(2u, 24u, "LEVEL 2  FOREST ROAD  ", 1u);
                demo_fix_puts(2u, 25u, "ROAD: HORIZONTAL       ", 2u);
            } else {
                demo_fix_puts(2u, 24u, "LEVEL 3  SUMMIT CLIMB ", 1u);
                demo_fix_puts(2u, 25u, "ROAD: VERTICAL         ", 2u);
            }
            playSFX(SOUND_SFX_7);
        }

        if ((phase_t % 160u) > 44u && (phase_t % 160u) < 120u) {
            uint16_t jt = (uint16_t)((phase_t % 160u) - 44u);
            jump = (int16_t)((jt < 38u) ? jt : (76u - jt));
            frame = s_hero_specA[(t / HERO_CAD_SPECIAL) % 8u];
        } else {
            frame = s_hero_walk[(t / HERO_CAD_WALK) % 8u];
        }

        draw_level_background(phase < 2u ? (uint16_t)(t * 2u)
                                         : (uint16_t)(phase_t * 2u), phase == 2u);

        fill = (uint8_t)(phase_t / 12u);
        if (fill > 20u) fill = 20u;
        for (k = 0u; k < 20u; k++) bar[k] = (char)(k < fill ? '#' : '.');
        bar[20] = '\0';
        demo_fix_puts(18u, 25u, bar, 1u);
        fix_cycle_puts(12u, 4u, "SCROLLING LEVEL", t, 7u);

        draw_asset_bottom_center(frame, HERO_SLOT_FIRST,
                                 160, (int16_t)(hero_bottom - jump),
                                 U_SCALE_3_8, U_SCALE_3_8);

        if ((t % 90u) == 0u) playSFX(SOUND_SFX_5);
        if (uframe()) return 1u;
    }
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
        RETICLE_MIN = 40,
        RETICLE_MAX = 280,
        /* Vertical aim range, in pixels.  Kept inside the range frame so
         * the reticle cannot wander into the HUD rows. */
        RETICLE_Y_MIN = 72,
        RETICLE_Y_MAX = 184,
        RETICLE_STEP  = 4
    };
    static const int16_t lane_x[TARGET_COUNT] = { -108, -36, 38, 108 };
    /*
     * All four lanes run the square hitbox target.  Character art was
     * tried here first and read as random scenery that happened to be
     * in the way; a hitbox reads as something you are meant to put the
     * reticle on, which is what this chapter demonstrates.
     */
    static const uint8_t target_frame[TARGET_COUNT] = {
        U_CRATE, U_CRATE, U_CRATE, U_CRATE
    };
    int16_t target_z[TARGET_COUNT] = { 36, 62, 88, 108 };
    int16_t target_x[TARGET_COUNT];
    int16_t target_y[TARGET_COUNT];
    uint8_t target_scale[TARGET_COUNT];
    uint8_t target_flash[TARGET_COUNT] = { 0u, 0u, 0u, 0u };
    int16_t reticle_x = 160;
    int16_t reticle_y = 128;
    uint8_t prev_col  = 0xFFu;
    uint8_t prev_row  = 0xFFu;
    uint8_t fire_timer = 0u;
    uint8_t ammo = 24u;
    uint16_t score = 0u;
    uint16_t t;
    uint8_t i;
    char buf[4];
    const uint16_t shot_tile = DEMO_SCREEN_TILE(U_PARTICLE_MAGIC);
    const uint8_t shot_pal = DEMO_SCREEN_PALETTE(U_PARTICLE_MAGIC);
    const uint16_t spark_tile = DEMO_SCREEN_TILE(U_PARTICLE_HITSPARK);
    const uint8_t spark_pal = DEMO_SCREEN_PALETTE(U_PARTICLE_HITSPARK);

    chap_header(16u, "TARGET RANGE", "MOVING TARGETS  SPRITE DEPTH");
    demo_fix_puts(2u, 2u, "D-PAD AIMS X AND Y   B FIRE", 1u);
    demo_fix_puts(2u, 3u, "MOVING CRATE TARGETS + DEPTH", 0u);
    snd_cross_to(SOUND_MUSIC_F);

    for (i = 0u; i < TARGET_COUNT; i++) {
        demo_load_screen_palette(target_frame[i]);
    }
    demo_load_screen_palette(U_CRATE_BROKEN);
    demo_load_screen_palette(U_PARTICLE_EXPLOSION);
    demo_load_screen_palette(U_PARTICLE_MAGIC);
    demo_load_screen_palette(U_PARTICLE_HITSPARK);
    s_draw_particles = 1u;

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

        /* Two-axis aim.  Up/down did nothing before, so the reticle
         * could only slide along one line and the chapter did not
         * really respond to the stick. */
        if (down & JOY_LEFT) {
            reticle_x = (int16_t)((reticle_x > RETICLE_MIN + RETICLE_STEP)
                                  ? reticle_x - RETICLE_STEP : RETICLE_MIN);
        }
        if (down & JOY_RIGHT) {
            reticle_x = (int16_t)((reticle_x < RETICLE_MAX - RETICLE_STEP)
                                  ? reticle_x + RETICLE_STEP : RETICLE_MAX);
        }
        if (down & JOY_UP) {
            reticle_y = (int16_t)((reticle_y > RETICLE_Y_MIN + RETICLE_STEP)
                                  ? reticle_y - RETICLE_STEP : RETICLE_Y_MIN);
        }
        if (down & JOY_DOWN) {
            reticle_y = (int16_t)((reticle_y < RETICLE_Y_MAX - RETICLE_STEP)
                                  ? reticle_y + RETICLE_STEP : RETICLE_Y_MAX);
        }
        reticle_col = (uint8_t)(reticle_x >> 3);

        /* Project once for both hit testing and drawing. The source crate
         * is 256 pixels, so 1/8..1/4 gives readable 32..64 pixel targets. */
        for (i = 0u; i < TARGET_COUNT; i++) {
            uint16_t approach;
            if (!target_flash[i]) {
                target_z[i]--;
                if (target_z[i] < Z_NEAR) target_z[i] = Z_FAR;
            }
            approach = (uint16_t)(Z_FAR - target_z[i]);
            target_scale[i] = (uint8_t)((0x1fu + approach / 3u) | 0x0fu);
            target_x[i] = (int16_t)(160 + lane_x[i] * (64 + approach / 2) / 112);
            target_y[i] = (int16_t)(104 + approach / 3);
        }

        for (i = 0u; i < TARGET_COUNT; i++) {
            int16_t sx;
            int16_t sy;
            int16_t dx;
            int16_t dy;
            if (target_flash[i]) continue;
            sx = target_x[i];
            /* Same depth mapping the draw pass below uses, so the lock
             * follows where the target actually appears. */
            sy = target_y[i];
            dx = (int16_t)(sx - reticle_x);
            dy = (int16_t)(sy - reticle_y);
            if (dx < 0) dx = (int16_t)-dx;
            if (dy < 0) dy = (int16_t)-dy;
            /* Aim is two-axis now, so a target only counts as the
             * closest one when the reticle is near it vertically too. */
            if (dy < (int16_t)(ng_sprite_scaled_y(demo_screen_content_height(U_CRATE), target_scale[i]) / 2u)
                && dx < (int16_t)(ng_sprite_scaled_x(demo_screen_content_width(U_CRATE), target_scale[i]) / 2u)
                && dx < best_dx) {
                best_dx = dx;
                locked_idx = (int8_t)i;
            }
        }
        is_locked = (uint8_t)(locked_idx >= 0);

        if ((pressed & BUTTON_B) && ammo > 0u) {
            int16_t shot_vx = (int16_t)((reticle_x - 160) / 16);
            int16_t shot_vy = (int16_t)((reticle_y - 196) / 16);
            fire_timer = 6u;
            ammo--;
            playSFX(SOUND_SFX_7);
            ng_particle_spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_CRITICAL,
                              160, 196,
                              (int32_t)shot_vx << NG_FP_SHIFT,
                              (int32_t)shot_vy << NG_FP_SHIFT,
                              18u, shot_tile, shot_pal, 1u, 1u);
            ng_particle_spawn(NG_PART_HIT_SPARK, NG_PART_PRI_CRITICAL,
                              160, 196, 0, 0,
                              8u, spark_tile, spark_pal, 1u, 1u);
            if (is_locked) {
                target_flash[locked_idx] = 18u;
                score = (uint16_t)((score < 975u) ? score + 25u : 999u);
                spawn_impact_burst(reticle_x, reticle_y,
                                   spark_tile, spark_pal, 3u);
                playSFX(SOUND_SFX_8);
            }
        }

        {
            uint8_t reticle_row = (uint8_t)(reticle_y >> 3);

            /* Blank the cells the reticle occupied last frame before
             * drawing it in its new place - it can move on both axes
             * now, so without this it smears a trail across the range. */
            if (prev_col != 0xFFu &&
                (prev_col != reticle_col || prev_row != reticle_row)) {
                raytrace_put((uint8_t)(prev_col - 1u), prev_row,
                             NGFIX_DEFAULT_BLANK_TILE, 0u);
                raytrace_put(prev_col, prev_row,
                             NGFIX_DEFAULT_BLANK_TILE, 0u);
                raytrace_put((uint8_t)(prev_col + 1u), prev_row,
                             NGFIX_DEFAULT_BLANK_TILE, 0u);
            }

            raytrace_put((uint8_t)(reticle_col - 1u), reticle_row, '[', is_locked ? 2u : 1u);
            raytrace_put(reticle_col, reticle_row, '+', is_locked ? 2u : 1u);
            raytrace_put((uint8_t)(reticle_col + 1u), reticle_row, ']', is_locked ? 2u : 1u);

            prev_col = reticle_col;
            prev_row = reticle_row;
        }

        for (i = 0u; i < TARGET_COUNT; i++) {
            draw_asset_center(target_flash[i] ? U_CRATE_BROKEN : target_frame[i],
                              (uint16_t)(128u + i * 20u), target_x[i], target_y[i],
                              target_scale[i], target_scale[i]);
            if (target_flash[i]) {
                target_flash[i]--;
                if (!target_flash[i]) target_z[i] = Z_FAR;
            }
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

    enum { EAGLE_BASE_Y = 146 };

    /*
     * The forest page starts at the top of the screen, so every line of
     * the header used to be printed straight onto the canopy.  The FIX
     * layer draws in front of every sprite, so a band under the header
     * is all it takes - and it costs the artwork nothing that was not
     * already covered in text.
     */
    chap_letterbox_next(5u, 3u);
    chap_header(17u, "CHAR 2D", "EAGLE FLIGHT ARC");
    demo_fix_puts(2u, 2u, "EAGLE FLYING FRAME BIND", 1u);
    demo_fix_puts(2u, 3u, "076 -> 077 FLAP CYCLE AT 70%", 0u);
    snd_cross_to(SOUND_MUSIC_G);

    draw_background(U_BG_FOREST, 32, 16);
    demo_load_screen_palette(s_flight_frames[0]);
    demo_load_screen_palette(s_flight_frames[1]);

    /*
     * Was 600 frames with a 300-frame flight period; 400/200 keeps the
     * same two full passes across the screen in two thirds of the time.
     */
    for (t = 0u; t < 400u; t++) {
        uint16_t p = (uint16_t)(t % 200u);
        uint8_t frame = s_flight_frames[(t / 8u) % 2u];
        int16_t x;
        int16_t y;
        uint16_t altitude;
        char buf[8];

        if (t < 133u) {
            demo_fix_puts(2u, 4u, "PHASE: ARC WALK        ", 2u);
        } else if (t < 280u) {
            demo_fix_puts(2u, 4u, "PHASE: HOP AND RECOVER ", 2u);
        } else {
            demo_fix_puts(2u, 4u, "PHASE: READY LOOP      ", 2u);
        }

        if (p < 100u) {
            x = (int16_t)(64 + ((uint16_t)p * 192u) / 100u);
        } else {
            x = (int16_t)(256 - (((uint16_t)(p - 100u) * 192u) / 100u));
        }
        /*
         * Flight arc.  The eagle used to sit on a y=186 baseline and
         * climb at most 20px out of it, so it skimmed along just above
         * the ground for the whole "flight".  The baseline is now 150
         * and the arc is 56px deep, which puts it up in the canopy and
         * makes the climb and descent actually read as flight.
         */
        y = (int16_t)(EAGLE_BASE_Y - (int16_t)((p < 100u)
                          ? ((uint16_t)p * 56u) / 100u
                          : ((uint16_t)(200u - p) * 56u) / 100u));

        /* Replaces a raw "[###----]" progress bar that didn't tell the
         * viewer anything about what it was tracking - an altitude
         * readout actually reflects what's happening on screen (the
         * eagle rising through the arc/hop phases). */
        altitude = (uint16_t)(EAGLE_BASE_Y - y);
        demo_fix_puts(24u, 4u, "ALT:", 1u);
        digit3(buf, altitude);
        demo_fix_puts(29u, 4u, buf, 2u);

        draw_asset_bottom_center(frame, HERO_SLOT_FIRST,
                                 x, y, U_SCALE_3_8, U_SCALE_3_8);

        if ((t % 60u) == 0u) playSFX(SOUND_SFX_5);
        if (uframe()) return 1u;
    }
    return 0u;
}

/* ================================================================== */
/*  Chapter 18 — Sky Lance mini (vertical shooter slice)                 */
/* ================================================================== */
static int16_t NEOGEO_USER u_abs16(int16_t v)
{
    return (int16_t)((v < 0) ? -v : v);
}

/* Claim the first free blast slot, if any.  Blasts are cosmetic, so a
 * full pool just means this kill goes unmarked rather than stealing a
 * slot that is still animating. */
static void NEOGEO_USER sky_boom_spawn(int16_t *bx, int16_t *by, uint8_t *bt,
                                       uint8_t count, int16_t x, int16_t y)
{
    uint8_t i;
    for (i = 0u; i < count; i++) {
        if (!bt[i]) { bx[i] = x; by[i] = y; bt[i] = 10u; return; }
    }
}

/*
 * A playable slice of games/skylance, not the whole game: one stage of
 * squadron waves and one boss, using that game's own art (U_SKY_*)
 * imported into this ROM.  What is deliberately left out is everything
 * that needs its own front end - the three-pilot select, the seven-stage
 * run and the energy/lives economy - because a demo chapter has to be
 * enterable and finishable inside a reel that also has to keep moving.
 *
 * It replaces the Galaxian formation mini that used to sit here.  The
 * two are different games: this one scrolls, its enemies arrive in
 * squadrons on flight paths instead of sitting in a grid, and it ends
 * on a boss rather than on a cleared formation.
 */
static uint8_t NEOGEO_USER chap_image_shooter(void)
{
    enum {
        /*
         * Slot map.  The hard rule inherited from the chapter that used
         * to live here: with this chapter's sprite load, nothing at or
         * above roughly slot 192 reaches the screen, so everything below
         * stays well under it.
         *
         * The strides below were sized for a larger import of the sky art and
         * are now more generous than it needs - plane 2 strips, jets and drone
         * 2, boss 8, bolt and orb 2.  Over-reserving is harmless (the pools
         * still cannot overlap each other's VRAM) and leaves room if the art
         * is ever re-imported larger, so they are left as they are.
         */
        SKY_SLOT_BG0     = NG_SPR_BG0_FIRST,  /* 16 strips  -> 1..16     */
        SKY_SLOT_BG1     = NG_SPR_BG1_FIRST,  /* 16 strips  -> 17..32    */
        SKY_SLOT_BG2     = 33,   /* 16 strips       -> 33..48   */
        SKY_SLOT_PLAYER  = 50,   /* 8 strips        -> 50..57   */
        SKY_SLOT_PBULLET = 60,   /* 4 * 2 strips    -> 60..67   */
        SKY_SLOT_EBULLET = 70,   /* 6 * 2 strips    -> 70..81   */
        SKY_SLOT_BOOM    = 84,   /* 3 * 2 strips    -> 84..89   */
        SKY_SLOT_ENEMY   = 96,   /* 6 * 8 strips    -> 96..143  */
        SKY_SLOT_BOSS    = 148,  /* 12 strips       -> 148..159 */

        /* The sky page: 256 px of art across, of which the top 144 px
         * loop seamlessly, which is why the page is nine characters
         * tall and not the sixteen the asset holds. */
        SKY_FIELD_X   = 32,
        SKY_PAGE_H    = 144,
        SKY_PAGE_ROWS = 9,

        SKY_ENEMY_MAX   = 6,
        SKY_PBULLET_MAX = 4,
        SKY_EBULLET_MAX = 6,
        SKY_BOOM_MAX    = 3,

        /* Play area, in pixels, between the two HUD bands. */
        SKY_LEFT   = 32,
        SKY_RIGHT  = 288,
        SKY_TOP    = 56,
        SKY_BOTTOM = 190,

        SKY_TIME = 1800,             /* ~30s, then the chapter moves on   */
        SKY_IDLE_ADVANCE = 600,      /* ~10s untouched -> skip ahead      */
        SKY_WAVES_TO_BOSS = 4
    };

    /* Squadron roster.  Two jets and a drone: different speeds and
     * scores, but all readable as "a formation of the same thing" for
     * the few seconds each squadron is on screen. */
    static const uint8_t  sq_asset[3] = { U_SKY_ENEMY_A, U_SKY_ENEMY_B, U_SKY_ENEMY_C };
    static const uint8_t  sq_hp[3]    = { 2u, 3u, 1u };
    static const uint16_t sq_score[3] = { 100u, 150u, 50u };
    /*
     * Full, accurate arcade sprite scales without shrink distortion.
     * Enemies are 32x32 sprites drawn 1:1 with NG_SCALE(16) so every pixel
     * and line is preserved without ugly scanline dropping.
     * Player is 16x16 1:1, and boss is an imposing 96x96 flagship.
     */
    static const uint8_t  sq_scale[3] = { NG_SCALE(16), NG_SCALE(16), NG_SCALE(16) };
    enum { SKY_SCALE_SHIP = NG_SCALE(16),   /* 16x16 - drawn 1:1 */
           SKY_SCALE_BOSS = NG_SCALE(12) }; /* 128x128 -> 96x96 crisp arcade boss */

    int16_t  en_x[SKY_ENEMY_MAX], en_y[SKY_ENEMY_MAX];
    uint8_t  en_alive[SKY_ENEMY_MAX], en_shown[SKY_ENEMY_MAX];
    uint8_t  en_type[SKY_ENEMY_MAX], en_hp[SKY_ENEMY_MAX];
    uint16_t en_phase[SKY_ENEMY_MAX];

    int16_t  pb_x[SKY_PBULLET_MAX], pb_y[SKY_PBULLET_MAX];
    uint8_t  pb_active[SKY_PBULLET_MAX], pb_shown[SKY_PBULLET_MAX];
    int16_t  eb_x[SKY_EBULLET_MAX], eb_y[SKY_EBULLET_MAX];
    int16_t  eb_vx[SKY_EBULLET_MAX], eb_vy[SKY_EBULLET_MAX];
    uint8_t  eb_active[SKY_EBULLET_MAX], eb_shown[SKY_EBULLET_MAX];
    int16_t  bm_x[SKY_BOOM_MAX], bm_y[SKY_BOOM_MAX];
    uint8_t  bm_timer[SKY_BOOM_MAX], bm_shown[SKY_BOOM_MAX];

    int16_t  ship_x = 160;
    int16_t  ship_y = SKY_BOTTOM;
    uint16_t score = 0u;
    uint8_t  lives = 3u;
    uint8_t  wave = 1u;
    uint8_t  fire_cd = 0u;
    uint8_t  hit_cd = 0u;

    uint8_t  squad_left = 0u;
    uint8_t  squad_type = 0u;
    uint8_t  squad_gap = 0u;
    uint16_t wave_timer = 60u;

    uint8_t  boss_on = 0u, boss_shown = 0u;
    uint8_t  boss_hp = 0u, boss_hp_max = 1u;
    int16_t  boss_x = 160, boss_y = -40;
    int16_t  boss_vx = 1;
    uint16_t boss_t = 0u;

    uint16_t bg_y = 0u;
    uint16_t idle_frames = 0u;
    uint16_t rng = 0x2F1Du;
    uint16_t t;
    uint8_t  i;
    char     buf[8];

    chap_letterbox_next(5u, 3u);
    chap_header(18u, "SKY LANCE", "VERTICAL SHOOTER MINI");
    demo_fix_puts(2u, 2u, "ARROWS MOVE  B FIRE  C:RESTART", 1u);

    demo_load_screen_palette(U_SKY_BG);
    demo_load_screen_palette(U_SKY_PLANE);
    demo_load_screen_palette(U_SKY_BULLET);
    demo_load_screen_palette(U_SKY_ORB);
    demo_load_screen_palette(U_SKY_ENEMY_A);
    demo_load_screen_palette(U_SKY_ENEMY_B);
    demo_load_screen_palette(U_SKY_ENEMY_C);
    demo_load_screen_palette(U_SKY_BOSS);
    demo_load_screen_palette(U_PARTICLE_EXPLOSION);

    snd_cross_to(SOUND_MUSIC_D);

    /*
     * The sky runs edge to edge, so the chapter is letterboxed rather
     * than boxed: an opaque band under the header and another under the
     * caption bar.  A dotted outline drawn straight onto the sky was
     * what the old cropped backdrop needed, and over a full-screen one
     * it reads as leftovers.
     */

    for (i = 0u; i < SKY_ENEMY_MAX; i++)   { en_alive[i] = 0u; en_shown[i] = 0u; }
    for (i = 0u; i < SKY_PBULLET_MAX; i++) { pb_active[i] = 0u; pb_shown[i] = 0u; }
    for (i = 0u; i < SKY_EBULLET_MAX; i++) { eb_active[i] = 0u; eb_shown[i] = 0u; }
    for (i = 0u; i < SKY_BOOM_MAX; i++)    { bm_timer[i] = 0u; bm_shown[i] = 0u; }

    for (t = 0u; t < SKY_TIME; t++) {
        uint16_t joy = poll_joystick();

        rng = (uint16_t)(rng * 2053u + 13849u);

        /* ---- backdrop -------------------------------------------- */
        /*
         * Three copies of a page that loops every 144 px, stacked one
         * page apart, so whatever the scroll offset is their union
         * always covers the whole screen and both seams sit outside it.
         *
         * Cropping the sky to a window instead is the thing the hardware
         * will not do.  A sprite is a whole number of characters tall,
         * so a window whose edge is not on a character boundary always
         * overhangs it by up to 15 px - the overhang is not optional,
         * only its destination is, and off-screen is the one place it
         * costs nothing.  Covering the screen is also cheaper than
         * cropping: the pages are uploaded once and a frame moves each
         * of them for two words.
         */
        bg_y = (uint16_t)((bg_y + 1u) % SKY_PAGE_H);
        demo_draw_sprite_screen(U_SKY_BG, SKY_SLOT_BG0, SKY_FIELD_X,
                                (int16_t)((int16_t)bg_y - SKY_PAGE_H),
                                16u, SKY_PAGE_ROWS, 0xFFu, 0xFFu);
        demo_draw_sprite_screen(U_SKY_BG, SKY_SLOT_BG1, SKY_FIELD_X,
                                (int16_t)bg_y,
                                16u, SKY_PAGE_ROWS, 0xFFu, 0xFFu);
        demo_draw_sprite_screen(U_SKY_BG, SKY_SLOT_BG2, SKY_FIELD_X,
                                (int16_t)((int16_t)bg_y + SKY_PAGE_H),
                                16u, SKY_PAGE_ROWS, 0xFFu, 0xFFu);

        /* ---- player ---------------------------------------------- */
        if (joy & (JOY_LEFT | JOY_RIGHT | JOY_UP | JOY_DOWN | BUTTON_B)) idle_frames = 0u;
        else if (idle_frames < 0xFFF0u) idle_frames++;

        if ((joy & JOY_LEFT)  && ship_x > SKY_LEFT)   ship_x = (int16_t)(ship_x - 8);
        if ((joy & JOY_RIGHT) && ship_x < SKY_RIGHT)  ship_x = (int16_t)(ship_x + 8);
        if ((joy & JOY_UP)    && ship_y > SKY_TOP)    ship_y = (int16_t)(ship_y - 7);
        if ((joy & JOY_DOWN)  && ship_y < SKY_BOTTOM) ship_y = (int16_t)(ship_y + 7);

        if (fire_cd) fire_cd--;
        if (hit_cd)  hit_cd--;
        /* Auto-fire on hold; B alone is the trigger so the joystick
         * hand never has to leave the stick. */
        if ((joy & BUTTON_B) && !fire_cd) {
            for (i = 0u; i < SKY_PBULLET_MAX; i++) {
                if (!pb_active[i]) {
                    pb_active[i] = 1u;
                    pb_x[i] = ship_x;
                    pb_y[i] = (int16_t)(ship_y - 14);
                    fire_cd = 7u;
                    playSFX(SOUND_SFX_4);
                    break;
                }
            }
        }

        /* ---- squadron director ----------------------------------- */
        if (!boss_on) {
            if (squad_left) {
                if (squad_gap) {
                    squad_gap--;
                } else {
                    for (i = 0u; i < SKY_ENEMY_MAX; i++) {
                        if (en_alive[i]) continue;
                        en_alive[i] = 1u;
                        en_type[i]  = squad_type;
                        en_hp[i]    = sq_hp[squad_type];
                        en_phase[i] = (uint16_t)(rng & 63u);
                        /* Fixed lanes: a squadron has to read as a
                         * formation, not as scatter. */
                        en_x[i] = (int16_t)(SKY_LEFT + 24
                                            + (int16_t)((squad_left & 3u)
                                                        * ((SKY_RIGHT - SKY_LEFT - 48) / 3)));
                        en_y[i] = (int16_t)(SKY_TOP - 20);
                        squad_left--;
                        squad_gap = 12u;
                        break;
                    }
                }
            } else if (wave_timer) {
                wave_timer--;
            } else if (wave <= SKY_WAVES_TO_BOSS) {
                squad_type = (uint8_t)(rng % 3u);
                squad_left = 4u;
                squad_gap  = 0u;
                wave_timer = 100u;
                wave++;
            } else {
                uint8_t any = 0u;
                for (i = 0u; i < SKY_ENEMY_MAX; i++) if (en_alive[i]) any = 1u;
                if (!any) {
                    boss_on = 1u;
                    boss_hp = boss_hp_max = 40u;
                    boss_x = 160; boss_y = -40; boss_vx = 1; boss_t = 0u;
                    playSFX(SOUND_SFX_2);
                }
            }
        }

        /* ---- enemies --------------------------------------------- */
        for (i = 0u; i < SKY_ENEMY_MAX; i++) {
            if (!en_alive[i]) {
                if (en_shown[i]) {
                    demo_hide_sprite_range(SKY_SLOT_ENEMY + i * 8u, 8u);
                    en_shown[i] = 0u;
                }
                continue;
            }
            en_phase[i]++;
            /* Triangle-wave weave - a sine table would cost more than
             * the effect is worth at this amplitude - plus a drift toward the
             * player's column, so a pass threatens where the player actually
             * is rather than the lane the squadron happened to enter in.  This
             * is the behaviour the full game gives its diving enemies. */
            {
                uint8_t ph = (uint8_t)(en_phase[i] & 63u);
                int16_t sway = (int16_t)((ph < 32u) ? ((int16_t)ph - 16) : (47 - (int16_t)ph));
                int16_t chase = (ship_x > en_x[i]) ? 1 : ((ship_x < en_x[i]) ? -1 : 0);
                en_x[i] = (int16_t)(en_x[i] + (sway >> 3) + chase);
            }
            en_y[i] = (int16_t)(en_y[i] + ((en_type[i] == 2u) ? 3 : 2));
            if (en_x[i] < SKY_LEFT)  en_x[i] = SKY_LEFT;
            if (en_x[i] > SKY_RIGHT) en_x[i] = SKY_RIGHT;

            if (en_type[i] != 2u && (rng % 90u) == 0u) {
                uint8_t k;
                for (k = 0u; k < SKY_EBULLET_MAX; k++) {
                    if (!eb_active[k]) {
                        /*
                         * Aim at the player rather than firing straight down.
                         * A shot that ignores where the player is only ever
                         * hits by accident, which reads as scenery instead of
                         * as opposition.  Manhattan distance is close enough
                         * for a normaliser at this range and costs no divide
                         * beyond the two here.
                         */
                        int16_t dx  = (int16_t)(ship_x - en_x[i]);
                        int16_t dy  = (int16_t)(ship_y - en_y[i]);
                        int16_t mag = (int16_t)(u_abs16(dx) + u_abs16(dy));

                        if (mag < 1) mag = 1;
                        eb_active[k] = 1u;
                        eb_x[k] = en_x[i];
                        eb_y[k] = (int16_t)(en_y[i] + 10);
                        eb_vx[k] = (int16_t)(((int32_t)dx * 3) / mag);
                        eb_vy[k] = (int16_t)(((int32_t)dy * 3) / mag);
                        /* Always carry some downward travel so a shot fired
                         * from level with the player still clears its owner. */
                        if (eb_vy[k] < 1) eb_vy[k] = 1;
                        break;
                    }
                }
            }

            if (en_y[i] > SKY_BOTTOM + 24) en_alive[i] = 0u;
        }

        /* ---- boss ------------------------------------------------- */
        if (boss_on) {
            boss_t++;
            if (boss_y < SKY_TOP + 4) {
                boss_y = (int16_t)(boss_y + 2);
            } else {
                boss_x = (int16_t)(boss_x + boss_vx);
                if (boss_x < SKY_LEFT + 40)  boss_vx =  1;
                if (boss_x > SKY_RIGHT - 40) boss_vx = -1;
                /* Spread of three, tightening once it is below half
                 * health so the last stretch actually threatens. */
                if ((boss_t % (uint16_t)((boss_hp * 2u < boss_hp_max) ? 40u : 70u)) == 0u) {
                    int16_t d;
                    for (d = -1; d <= 1; d++) {
                        uint8_t k;
                        for (k = 0u; k < SKY_EBULLET_MAX; k++) {
                            if (!eb_active[k]) {
                                eb_active[k] = 1u;
                                eb_x[k] = boss_x;
                                eb_y[k] = (int16_t)(boss_y + 20);
                                eb_vx[k] = (int16_t)(d * 2);
                                eb_vy[k] = 3;
                                break;
                            }
                        }
                    }
                }
            }
        }

        /* ---- bullets --------------------------------------------- */
        for (i = 0u; i < SKY_PBULLET_MAX; i++) {
            if (!pb_active[i]) continue;
            pb_y[i] = (int16_t)(pb_y[i] - 9);
            if (pb_y[i] < SKY_TOP - 16) pb_active[i] = 0u;
        }
        for (i = 0u; i < SKY_EBULLET_MAX; i++) {
            if (!eb_active[i]) continue;
            eb_x[i] = (int16_t)(eb_x[i] + eb_vx[i]);
            eb_y[i] = (int16_t)(eb_y[i] + eb_vy[i]);
            if (eb_y[i] > SKY_BOTTOM + 16 || eb_x[i] < SKY_LEFT - 16 ||
                eb_x[i] > SKY_RIGHT + 16) eb_active[i] = 0u;
        }

        /* ---- collisions ------------------------------------------ */
        for (i = 0u; i < SKY_PBULLET_MAX; i++) {
            uint8_t k;
            if (!pb_active[i]) continue;

            for (k = 0u; k < SKY_ENEMY_MAX; k++) {
                if (!en_alive[k]) continue;
                if (u_abs16((int16_t)(pb_x[i] - en_x[k])) > 16) continue;
                if (u_abs16((int16_t)(pb_y[i] - en_y[k])) > 16) continue;
                pb_active[i] = 0u;
                if (en_hp[k] > 1u) {
                    en_hp[k]--;
                } else {
                    en_alive[k] = 0u;
                    score = (uint16_t)(score + sq_score[en_type[k]]);
                    sky_boom_spawn(bm_x, bm_y, bm_timer, SKY_BOOM_MAX, en_x[k], en_y[k]);
                    playSFX(SOUND_SFX_5);
                }
                break;
            }
            if (!pb_active[i]) continue;

            if (boss_on && boss_y > SKY_TOP - 20 &&
                u_abs16((int16_t)(pb_x[i] - boss_x)) <= 40 &&
                u_abs16((int16_t)(pb_y[i] - boss_y)) <= 28) {
                pb_active[i] = 0u;
                if (boss_hp) boss_hp--;
                sky_boom_spawn(bm_x, bm_y, bm_timer, SKY_BOOM_MAX, pb_x[i], pb_y[i]);
                if (!boss_hp) {
                    boss_on = 0u;
                    score = (uint16_t)(score + 2000u);
                    sky_boom_spawn(bm_x, bm_y, bm_timer, SKY_BOOM_MAX, boss_x, boss_y);
                    playSFX(SOUND_SFX_5);
                }
            }
        }

        if (!hit_cd) {
            uint8_t hit = 0u;
            for (i = 0u; i < SKY_EBULLET_MAX; i++) {
                if (!eb_active[i]) continue;
                if (u_abs16((int16_t)(eb_x[i] - ship_x)) > 10) continue;
                if (u_abs16((int16_t)(eb_y[i] - ship_y)) > 10) continue;
                eb_active[i] = 0u;
                hit = 1u;
                break;
            }
            if (!hit) {
                for (i = 0u; i < SKY_ENEMY_MAX; i++) {
                    if (!en_alive[i]) continue;
                    if (u_abs16((int16_t)(en_x[i] - ship_x)) > 16) continue;
                    if (u_abs16((int16_t)(en_y[i] - ship_y)) > 16) continue;
                    en_alive[i] = 0u;
                    hit = 1u;
                    break;
                }
            }
            if (hit) {
                sky_boom_spawn(bm_x, bm_y, bm_timer, SKY_BOOM_MAX, ship_x, ship_y);
                playSFX(SOUND_SFX_2);
                hit_cd = 90u;
                if (lives) lives--;
                ship_x = 160;
                ship_y = SKY_BOTTOM;
                if (!lives) {
                    demo_fix_puts(15u, 14u, "SQUAD DOWN", 2u);
                    if (uwait(150u)) { snd_silence(); return 1u; }
                    lives = 3u;
                    score = 0u;
                    wave = 1u;
                    boss_on = 0u;
                    demo_fix_puts(15u, 14u, "          ", 2u);
                }
            }
        }

        for (i = 0u; i < SKY_BOOM_MAX; i++) if (bm_timer[i]) bm_timer[i]--;

        /* ---- draw ------------------------------------------------- */
        for (i = 0u; i < SKY_ENEMY_MAX; i++) {
            if (!en_alive[i]) continue;
            draw_asset_center(sq_asset[en_type[i]],
                              (uint16_t)(SKY_SLOT_ENEMY + i * 8u),
                              en_x[i], en_y[i],
                              sq_scale[en_type[i]], sq_scale[en_type[i]]);
            en_shown[i] = 1u;
        }

        if (boss_on) {
            draw_asset_center(U_SKY_BOSS, SKY_SLOT_BOSS, boss_x, boss_y,
                              SKY_SCALE_BOSS, SKY_SCALE_BOSS);
            boss_shown = 1u;
        } else if (boss_shown) {
            demo_hide_sprite_range(SKY_SLOT_BOSS, 12u);
            boss_shown = 0u;
        }

        /* The plane blinks through its invulnerability window so a hit
         * reads as a hit and not as a dropped sprite. */
        if (!hit_cd || (hit_cd & 4u)) {
            draw_asset_center(U_SKY_PLANE, SKY_SLOT_PLAYER, ship_x, ship_y,
                              SKY_SCALE_SHIP, SKY_SCALE_SHIP);
        } else {
            demo_hide_sprite_range(SKY_SLOT_PLAYER, 8u);
        }

        for (i = 0u; i < SKY_PBULLET_MAX; i++) {
            uint16_t slot = (uint16_t)(SKY_SLOT_PBULLET + i * 2u);
            if (pb_active[i]) {
                /* Full width, half height: the art was doubled so the
                 * bolt would be wide enough to see, and at full scale it
                 * came out 80 px long - a bar reaching most of the way up
                 * the playfield. */
                draw_asset_center(U_SKY_BULLET, slot, pb_x[i], pb_y[i],
                                  0xFFu, 0x80u);
                pb_shown[i] = 1u;
            } else if (pb_shown[i]) {
                demo_hide_sprite_range(slot, 2u);
                pb_shown[i] = 0u;
            }
        }
        for (i = 0u; i < SKY_EBULLET_MAX; i++) {
            uint16_t slot = (uint16_t)(SKY_SLOT_EBULLET + i * 2u);
            if (eb_active[i]) {
                draw_asset_center(U_SKY_ORB, slot, eb_x[i], eb_y[i],
                                  0xFFu, 0xFFu);
                eb_shown[i] = 1u;
            } else if (eb_shown[i]) {
                demo_hide_sprite_range(slot, 2u);
                eb_shown[i] = 0u;
            }
        }
        for (i = 0u; i < SKY_BOOM_MAX; i++) {
            uint16_t slot = (uint16_t)(SKY_SLOT_BOOM + i * 2u);
            if (bm_timer[i]) {
                uint8_t sc = (uint8_t)(0x70u +
                                       (uint8_t)((10u - bm_timer[i]) * 12u));
                draw_asset_center(U_PARTICLE_EXPLOSION, slot,
                                  bm_x[i], bm_y[i], sc, sc);
                bm_shown[i] = 1u;
            } else if (bm_shown[i]) {
                demo_hide_sprite_range(slot, 2u);
                bm_shown[i] = 0u;
            }
        }

        /* ---- readouts --------------------------------------------- */
        demo_fix_puts(2u, 3u, "SCORE", 1u);
        digit3(buf, score);
        demo_fix_puts(8u, 3u, buf, 2u);
        demo_fix_puts(14u, 3u, boss_on ? "BOSS" : "WAVE", 1u);
        digit3(buf, boss_on ? (uint16_t)boss_hp : (uint16_t)wave);
        demo_fix_puts(19u, 3u, buf, 2u);
        demo_fix_puts(25u, 3u, "LIFE", 1u);
        digit3(buf, lives);
        demo_fix_puts(30u, 3u, buf, 2u);

        {
            /* Row 5 doubles as a boss health bar once the boss is up and
             * as a stage progress bar before that. */
            char bar[26];
            uint8_t n, k;
            if (boss_on) {
                n = (uint8_t)(((uint16_t)boss_hp * 24u) / boss_hp_max);
            } else {
                n = (uint8_t)(((uint16_t)(wave - 1u) * 24u) / SKY_WAVES_TO_BOSS);
            }
            if (n > 24u) n = 24u;
            for (k = 0u; k < 24u; k++) bar[k] = (char)(k < n ? '#' : '.');
            bar[24] = '\0';
            demo_fix_puts(8u, 4u, bar, boss_on ? 3u : 0u);
        }

        if (idle_frames >= SKY_IDLE_ADVANCE) { snd_silence(); return 1u; }
        if (uframe()) { snd_silence(); return 1u; }
    }

    snd_silence();
    return 0u;
}

static uint8_t NEOGEO_USER chap_ssg_arcade(void)
{
    return chap_image_shooter();
}

/* ================================================================== */
/*  Chapter 25 — Star Raid Lance (Combined Galaxy + Sky Lance Arcade)   */
/* ================================================================== */
/*
 * Combined arcade combat minigame fusing Star Raid (neogeogame)
 * formation swoop dynamics with Sky Lance tactical craft and boss combat.
 *
 * Uses crisp, un-distorted arcade scales (1:1 NG_SCALE(16) on 32x32 craft,
 * NG_SCALE(8) on 112x112 flagship cruisers, and NG_SCALE(12) on boss)
 * to ensure pixel-perfect rendering without line-dropping distortion.
 *
 * Sprites are layered strictly:
 * Background starfield -> Bullets -> Enemies -> Player -> Explosions
 * completely eliminating occlusion artifacts.
 */
static uint8_t NEOGEO_USER chap_galaxy_skylance(void)
{
    enum {
        GALAXY_SLOT_BG0      = 1,   /* 16 strips -> 1..16 */
        GALAXY_SLOT_BG1      = 17,  /* 16 strips -> 17..32 */
        GALAXY_SLOT_PBULLET  = 34,  /* 4 * 2 strips -> 34..41 */
        GALAXY_SLOT_EBULLET  = 42,  /* 6 * 2 strips -> 42..53 */
        GALAXY_SLOT_ENEMY    = 54,  /* 6 * 8 strips -> 54..101 */
        GALAXY_SLOT_BOSS     = 102, /* 8 strips -> 102..109 */
        GALAXY_SLOT_PLAYER   = 110, /* 6 strips -> 110..115 (front of enemies) */
        GALAXY_SLOT_BOOM     = 116, /* 4 * 2 strips -> 116..123 */

        GALAXY_PBULLET_MAX   = 4,
        GALAXY_EBULLET_MAX   = 6,
        GALAXY_ENEMY_MAX     = 6,
        GALAXY_BOOM_MAX      = 4,

        GALAXY_LEFT          = 36,
        GALAXY_RIGHT         = 284,
        GALAXY_TOP           = 52,
        GALAXY_BOTTOM        = 186,

        GALAXY_TIME          = 1800,
        GALAXY_IDLE_ADVANCE  = 600,
        GALAXY_WAVES_TO_BOSS = 3
    };

    /* Squadron roster: Galaxy Cruisers + Sky Lance Interceptors */
    static const uint8_t  gx_asset[6] = {
        U_ENEMYSHIP_BLUE, U_ENEMYSHIP_GREEN, U_ENEMYSHIP_PINK,
        U_SKY_ENEMY_A,    U_SKY_ENEMY_B,     U_SKY_ENEMY_C
    };
    static const uint8_t  gx_hp[6]    = { 2u, 3u, 2u, 1u, 3u, 1u };
    static const uint16_t gx_score[6] = { 200u, 250u, 200u, 100u, 150u, 100u };
    static const uint8_t  gx_scale[6] = {
        NG_SCALE(8), NG_SCALE(8), NG_SCALE(8),
        NG_SCALE(16), NG_SCALE(16), NG_SCALE(16)
    };

    enum {
        GALAXY_SCALE_PLAYER = NG_SCALE(8),   /* 75x112 -> 37x56 crisp 50% */
        GALAXY_SCALE_BOSS   = NG_SCALE(12)   /* 128x128 -> 96x96 flagship */
    };

    int16_t  en_x[GALAXY_ENEMY_MAX], en_y[GALAXY_ENEMY_MAX];
    uint8_t  en_alive[GALAXY_ENEMY_MAX], en_shown[GALAXY_ENEMY_MAX];
    uint8_t  en_type[GALAXY_ENEMY_MAX], en_hp[GALAXY_ENEMY_MAX];
    uint16_t en_phase[GALAXY_ENEMY_MAX];

    int16_t  pb_x[GALAXY_PBULLET_MAX], pb_y[GALAXY_PBULLET_MAX];
    uint8_t  pb_active[GALAXY_PBULLET_MAX], pb_shown[GALAXY_PBULLET_MAX];
    int16_t  eb_x[GALAXY_EBULLET_MAX], eb_y[GALAXY_EBULLET_MAX];
    int16_t  eb_vx[GALAXY_EBULLET_MAX], eb_vy[GALAXY_EBULLET_MAX];
    uint8_t  eb_active[GALAXY_EBULLET_MAX], eb_shown[GALAXY_EBULLET_MAX];
    int16_t  bm_x[GALAXY_BOOM_MAX], bm_y[GALAXY_BOOM_MAX];
    uint8_t  bm_timer[GALAXY_BOOM_MAX], bm_shown[GALAXY_BOOM_MAX];

    int16_t  ship_x = 160;
    int16_t  ship_y = GALAXY_BOTTOM - 8;
    uint16_t score = 0u;
    uint8_t  lives = 3u;
    uint8_t  wave = 1u;
    uint8_t  fire_cd = 0u;
    uint8_t  hit_cd = 0u;

    uint8_t  squad_left = 0u;
    uint8_t  squad_type = 0u;
    uint8_t  squad_gap = 0u;
    uint16_t wave_timer = 60u;

    uint8_t  boss_on = 0u, boss_shown = 0u;
    uint8_t  boss_hp = 0u, boss_hp_max = 1u;
    int16_t  boss_x = 160, boss_y = -40;
    int16_t  boss_vx = 1;
    uint16_t boss_t = 0u;

    int16_t  bg_y = 0;
    uint16_t t;
    uint16_t idle_frames = 0u;
    uint8_t  i;
    char     buf[12];

    chap_letterbox_next(5u, 4u);
    chap_header(25u, "STAR RAID LANCE", "FORMATION ASSAULT");
    demo_fix_puts(2u, 2u, "D-PAD MOVE   B FIRE   C RESTART", 1u);

    demo_load_screen_palette(U_SSG_STARFIELD);
    demo_load_screen_palette(U_PLAYER_VESSEL);
    for (i = 0u; i < 6u; i++) demo_load_screen_palette(gx_asset[i]);
    demo_load_screen_palette(U_SKY_BOSS);
    demo_load_screen_palette(U_SKY_BULLET);
    demo_load_screen_palette(U_SKY_ORB);
    demo_load_screen_palette(U_PARTICLE_EXPLOSION);

    for (i = 0u; i < GALAXY_ENEMY_MAX; i++)   en_alive[i] = en_shown[i] = 0u;
    for (i = 0u; i < GALAXY_PBULLET_MAX; i++) pb_active[i] = pb_shown[i] = 0u;
    for (i = 0u; i < GALAXY_EBULLET_MAX; i++) eb_active[i] = eb_shown[i] = 0u;
    for (i = 0u; i < GALAXY_BOOM_MAX; i++)    bm_timer[i] = bm_shown[i] = 0u;

    snd_cross_to(SOUND_MUSIC_A);

    for (t = 0u; t < GALAXY_TIME; t++) {
        uint16_t joy = poll_joystick();
        uint16_t rng = (uint16_t)(t * 11035u + 12345u);

        /* ---- scroll starfield ------------------------------------- */
        bg_y = (int16_t)((bg_y + 2) % 256);
        draw_vertical_background(U_SSG_STARFIELD, 32, -bg_y);

        /* ---- player controls -------------------------------------- */
        if (joy & (JOY_LEFT | JOY_RIGHT | JOY_UP | JOY_DOWN | BUTTON_A | BUTTON_B)) {
            idle_frames = 0u;
        } else if (idle_frames < 0xFFF0u) {
            idle_frames++;
        }

        if ((joy & JOY_LEFT)  && ship_x > GALAXY_LEFT)   ship_x = (int16_t)(ship_x - 3);
        if ((joy & JOY_RIGHT) && ship_x < GALAXY_RIGHT)  ship_x = (int16_t)(ship_x + 3);
        if ((joy & JOY_UP)    && ship_y > GALAXY_TOP)    ship_y = (int16_t)(ship_y - 3);
        if ((joy & JOY_DOWN)  && ship_y < GALAXY_BOTTOM) ship_y = (int16_t)(ship_y + 3);

        if (fire_cd) fire_cd--;
        if (hit_cd)  hit_cd--;

        if ((joy & (BUTTON_A | BUTTON_B)) && !fire_cd) {
            for (i = 0u; i < GALAXY_PBULLET_MAX; i++) {
                if (!pb_active[i]) {
                    pb_active[i] = 1u;
                    pb_x[i] = ship_x;
                    pb_y[i] = (int16_t)(ship_y - 18);
                    fire_cd = 8u;
                    playSFX(SOUND_SFX_4);
                    break;
                }
            }
        }

        /* ---- wave / squadron manager ------------------------------ */
        if (!boss_on) {
            if (squad_left) {
                if (squad_gap) {
                    squad_gap--;
                } else {
                    for (i = 0u; i < GALAXY_ENEMY_MAX; i++) {
                        if (en_alive[i]) continue;
                        en_alive[i] = 1u;
                        en_type[i]  = squad_type;
                        en_hp[i]    = gx_hp[squad_type];
                        en_phase[i] = (uint16_t)(rng & 63u);
                        en_x[i] = (int16_t)(GALAXY_LEFT + 24
                                            + (int16_t)((squad_left & 3u)
                                                        * ((GALAXY_RIGHT - GALAXY_LEFT - 48) / 3)));
                        en_y[i] = (int16_t)(GALAXY_TOP - 20);
                        squad_left--;
                        squad_gap = 14u;
                        break;
                    }
                }
            } else if (wave_timer) {
                wave_timer--;
            } else if (wave <= GALAXY_WAVES_TO_BOSS) {
                /* Cycle between cruisers (0..2) and interceptors (3..5) */
                squad_type = (uint8_t)((wave == 1u) ? (rng % 3u)
                                     : ((wave == 2u) ? (3u + (rng % 3u))
                                                     : (rng % 6u)));
                squad_left = 4u;
                squad_gap  = 0u;
                wave_timer = 120u;
                wave++;
            } else {
                uint8_t any = 0u;
                for (i = 0u; i < GALAXY_ENEMY_MAX; i++) if (en_alive[i]) any = 1u;
                if (!any) {
                    boss_on = 1u;
                    boss_hp = boss_hp_max = 36u;
                    boss_x = 160;
                    boss_y = (int16_t)(GALAXY_TOP - 30);
                    boss_vx = 1;
                    boss_t = 0u;
                    demo_fix_puts(14u, 4u, "WARNING: BOSS", 3u);
                    playSFX(SOUND_SFX_10);
                }
            }
        }

        /* ---- enemy behavior & movement ---------------------------- */
        for (i = 0u; i < GALAXY_ENEMY_MAX; i++) {
            if (!en_alive[i]) {
                if (en_shown[i]) {
                    demo_hide_sprite_range((uint16_t)(GALAXY_SLOT_ENEMY + i * 8u), 8u);
                    en_shown[i] = 0u;
                }
                continue;
            }
            en_phase[i]++;
            {
                uint8_t ph = (uint8_t)(en_phase[i] & 63u);
                int16_t sway = (int16_t)((ph < 32u) ? ((int16_t)ph - 16) : (47 - (int16_t)ph));
                int16_t chase = (ship_x > en_x[i]) ? 1 : ((ship_x < en_x[i]) ? -1 : 0);
                en_x[i] = (int16_t)(en_x[i] + (sway >> 2) + chase);
            }
            en_y[i] = (int16_t)(en_y[i] + ((en_type[i] >= 3u) ? 3 : 2));
            if (en_x[i] < GALAXY_LEFT)  en_x[i] = GALAXY_LEFT;
            if (en_x[i] > GALAXY_RIGHT) en_x[i] = GALAXY_RIGHT;

            if ((rng % 80u) == 0u) {
                uint8_t k;
                for (k = 0u; k < GALAXY_EBULLET_MAX; k++) {
                    if (!eb_active[k]) {
                        int16_t dx = (int16_t)(ship_x - en_x[i]);
                        int16_t dy = (int16_t)(ship_y - en_y[i]);
                        int16_t mag = (int16_t)(u_abs16(dx) + u_abs16(dy));
                        if (mag < 1) mag = 1;
                        eb_active[k] = 1u;
                        eb_x[k] = en_x[i];
                        eb_y[k] = (int16_t)(en_y[i] + 12);
                        eb_vx[k] = (int16_t)(((int32_t)dx * 3) / mag);
                        eb_vy[k] = (int16_t)(((int32_t)dy * 3) / mag);
                        if (eb_vy[k] < 1) eb_vy[k] = 1;
                        break;
                    }
                }
            }

            if (en_y[i] > GALAXY_BOTTOM + 24) en_alive[i] = 0u;
        }

        /* ---- boss behavior ---------------------------------------- */
        if (boss_on) {
            boss_t++;
            if (boss_y < GALAXY_TOP + 12) {
                boss_y = (int16_t)(boss_y + 1);
            } else {
                boss_x = (int16_t)(boss_x + boss_vx);
                if (boss_x < GALAXY_LEFT + 36)  boss_vx = 1;
                if (boss_x > GALAXY_RIGHT - 36) boss_vx = -1;

                if ((boss_t % ((boss_hp * 2u < boss_hp_max) ? 35u : 60u)) == 0u) {
                    int16_t d;
                    for (d = -1; d <= 1; d++) {
                        uint8_t k;
                        for (k = 0u; k < GALAXY_EBULLET_MAX; k++) {
                            if (!eb_active[k]) {
                                eb_active[k] = 1u;
                                eb_x[k] = boss_x;
                                eb_y[k] = (int16_t)(boss_y + 24);
                                eb_vx[k] = (int16_t)(d * 2);
                                eb_vy[k] = 3;
                                break;
                            }
                        }
                    }
                }
            }
        }

        /* ---- bullet movement -------------------------------------- */
        for (i = 0u; i < GALAXY_PBULLET_MAX; i++) {
            if (!pb_active[i]) continue;
            pb_y[i] = (int16_t)(pb_y[i] - 8);
            if (pb_y[i] < GALAXY_TOP - 16) pb_active[i] = 0u;
        }
        for (i = 0u; i < GALAXY_EBULLET_MAX; i++) {
            if (!eb_active[i]) continue;
            eb_x[i] = (int16_t)(eb_x[i] + eb_vx[i]);
            eb_y[i] = (int16_t)(eb_y[i] + eb_vy[i]);
            if (eb_y[i] > GALAXY_BOTTOM + 16 || eb_x[i] < GALAXY_LEFT - 16 ||
                eb_x[i] > GALAXY_RIGHT + 16) eb_active[i] = 0u;
        }

        /* ---- collisions ------------------------------------------- */
        for (i = 0u; i < GALAXY_PBULLET_MAX; i++) {
            uint8_t k;
            if (!pb_active[i]) continue;

            for (k = 0u; k < GALAXY_ENEMY_MAX; k++) {
                if (!en_alive[k]) continue;
                if (u_abs16((int16_t)(pb_x[i] - en_x[k])) > 18) continue;
                if (u_abs16((int16_t)(pb_y[i] - en_y[k])) > 18) continue;
                pb_active[i] = 0u;
                if (en_hp[k] > 1u) {
                    en_hp[k]--;
                    playSFX(SOUND_SFX_2);
                } else {
                    en_alive[k] = 0u;
                    score = (uint16_t)(score + gx_score[en_type[k]]);
                    sky_boom_spawn(bm_x, bm_y, bm_timer, GALAXY_BOOM_MAX, en_x[k], en_y[k]);
                    playSFX(SOUND_SFX_10);
                }
                break;
            }
            if (!pb_active[i]) continue;

            if (boss_on && boss_y > GALAXY_TOP - 20 &&
                u_abs16((int16_t)(pb_x[i] - boss_x)) <= 40 &&
                u_abs16((int16_t)(pb_y[i] - boss_y)) <= 28) {
                pb_active[i] = 0u;
                if (boss_hp) boss_hp--;
                sky_boom_spawn(bm_x, bm_y, bm_timer, GALAXY_BOOM_MAX, pb_x[i], pb_y[i]);
                playSFX(SOUND_SFX_2);
                if (!boss_hp) {
                    boss_on = 0u;
                    score = (uint16_t)(score + 3000u);
                    sky_boom_spawn(bm_x, bm_y, bm_timer, GALAXY_BOOM_MAX, boss_x, boss_y);
                    playSFX(SOUND_SFX_10);
                }
            }
        }

        /* Player collision check */
        if (!hit_cd) {
            uint8_t hit = 0u;
            for (i = 0u; i < GALAXY_EBULLET_MAX; i++) {
                if (!eb_active[i]) continue;
                if (u_abs16((int16_t)(eb_x[i] - ship_x)) > 12) continue;
                if (u_abs16((int16_t)(eb_y[i] - ship_y)) > 16) continue;
                eb_active[i] = 0u;
                hit = 1u;
                break;
            }
            if (!hit) {
                for (i = 0u; i < GALAXY_ENEMY_MAX; i++) {
                    if (!en_alive[i]) continue;
                    if (u_abs16((int16_t)(en_x[i] - ship_x)) > 18) continue;
                    if (u_abs16((int16_t)(en_y[i] - ship_y)) > 18) continue;
                    en_alive[i] = 0u;
                    hit = 1u;
                    break;
                }
            }
            if (hit) {
                sky_boom_spawn(bm_x, bm_y, bm_timer, GALAXY_BOOM_MAX, ship_x, ship_y);
                playSFX(SOUND_SFX_2);
                hit_cd = 90u;
                if (lives) lives--;
                ship_x = 160;
                ship_y = GALAXY_BOTTOM - 8;
                if (!lives) {
                    demo_fix_puts(15u, 14u, "FLEET LOST", 2u);
                    if (uwait(150u)) { snd_silence(); return 1u; }
                    lives = 3u;
                    score = 0u;
                    wave = 1u;
                    boss_on = 0u;
                    demo_fix_puts(15u, 14u, "          ", 2u);
                }
            }
        }

        for (i = 0u; i < GALAXY_BOOM_MAX; i++) if (bm_timer[i]) bm_timer[i]--;

        /* ---- draw entities (strictly ordered for correct priority) -- */
        /* 1. Bullets (behind craft) */
        for (i = 0u; i < GALAXY_PBULLET_MAX; i++) {
            uint16_t slot = (uint16_t)(GALAXY_SLOT_PBULLET + i * 2u);
            if (pb_active[i]) {
                draw_asset_center(U_SKY_BULLET, slot, pb_x[i], pb_y[i], 0xFFu, 0x80u);
                pb_shown[i] = 1u;
            } else if (pb_shown[i]) {
                demo_hide_sprite_range(slot, 2u);
                pb_shown[i] = 0u;
            }
        }
        for (i = 0u; i < GALAXY_EBULLET_MAX; i++) {
            uint16_t slot = (uint16_t)(GALAXY_SLOT_EBULLET + i * 2u);
            if (eb_active[i]) {
                draw_asset_center(U_SKY_ORB, slot, eb_x[i], eb_y[i], 0xFFu, 0xFFu);
                eb_shown[i] = 1u;
            } else if (eb_shown[i]) {
                demo_hide_sprite_range(slot, 2u);
                pb_shown[i] = 0u;
            }
        }

        /* 2. Enemies */
        for (i = 0u; i < GALAXY_ENEMY_MAX; i++) {
            if (!en_alive[i]) continue;
            draw_asset_center(gx_asset[en_type[i]],
                              (uint16_t)(GALAXY_SLOT_ENEMY + i * 8u),
                              en_x[i], en_y[i],
                              gx_scale[en_type[i]], gx_scale[en_type[i]]);
            en_shown[i] = 1u;
        }

        /* 3. Boss */
        if (boss_on) {
            draw_asset_center(U_SKY_BOSS, GALAXY_SLOT_BOSS, boss_x, boss_y,
                              GALAXY_SCALE_BOSS, GALAXY_SCALE_BOSS);
            boss_shown = 1u;
        } else if (boss_shown) {
            demo_hide_sprite_range(GALAXY_SLOT_BOSS, 8u);
            boss_shown = 0u;
        }

        /* 4. Player Ship (drawn at slot 110 in front of enemies) */
        if (!hit_cd || (hit_cd & 4u)) {
            draw_asset_center(U_PLAYER_VESSEL, GALAXY_SLOT_PLAYER, ship_x, ship_y,
                              GALAXY_SCALE_PLAYER, GALAXY_SCALE_PLAYER);
        } else {
            demo_hide_sprite_range(GALAXY_SLOT_PLAYER, 6u);
        }

        /* 5. Explosions (drawn at slot 116 topmost) */
        for (i = 0u; i < GALAXY_BOOM_MAX; i++) {
            uint16_t slot = (uint16_t)(GALAXY_SLOT_BOOM + i * 2u);
            if (bm_timer[i]) {
                uint8_t sc = (uint8_t)(0x70u + (uint8_t)((10u - bm_timer[i]) * 12u));
                draw_asset_center(U_PARTICLE_EXPLOSION, slot,
                                  bm_x[i], bm_y[i], sc, sc);
                bm_shown[i] = 1u;
            } else if (bm_shown[i]) {
                demo_hide_sprite_range(slot, 2u);
                bm_shown[i] = 0u;
            }
        }

        /* ---- readouts --------------------------------------------- */
        demo_fix_puts(2u, 3u, "SCORE", 1u);
        digit3(buf, score);
        demo_fix_puts(8u, 3u, buf, 2u);
        demo_fix_puts(14u, 3u, boss_on ? "BOSS" : "WAVE", 1u);
        digit3(buf, boss_on ? (uint16_t)boss_hp : (uint16_t)wave);
        demo_fix_puts(19u, 3u, buf, 2u);
        demo_fix_puts(25u, 3u, "LIFE", 1u);
        digit3(buf, lives);
        demo_fix_puts(30u, 3u, buf, 2u);

        {
            char bar[26];
            uint8_t n, k;
            if (boss_on) {
                n = (uint8_t)(((uint16_t)boss_hp * 24u) / boss_hp_max);
            } else {
                n = (uint8_t)(((uint16_t)(wave - 1u) * 24u) / GALAXY_WAVES_TO_BOSS);
            }
            if (n > 24u) n = 24u;
            for (k = 0u; k < 24u; k++) bar[k] = (char)(k < n ? '#' : '.');
            bar[24] = '\0';
            demo_fix_puts(8u, 4u, bar, boss_on ? 3u : 0u);
        }

        /* Boss defeated check */
        if (!boss_on && wave > GALAXY_WAVES_TO_BOSS && score >= 3000u) {
            demo_fix_puts(13u, 14u, "MISSION COMPLETE", 2u);
            demo_fix_puts(14u, 15u, "ALL CLEAR!!", 1u);
            if (uwait(180u)) {
                ng_sprite_park_off_range(1u, 130u);
                snd_silence();
                return 1u;
            }
            break;
        }

        if (idle_frames >= GALAXY_IDLE_ADVANCE) {
            ng_sprite_park_off_range(1u, 130u);
            snd_silence();
            return 1u;
        }
        if (uframe()) {
            ng_sprite_park_off_range(1u, 130u);
            snd_silence();
            return 1u;
        }
    }

    ng_sprite_park_off_range(1u, 130u);
    snd_silence();
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

        chap_letterbox_next(5u, 4u);
        chap_header(19u, "DEPTH RIDE", "ROAD DEPTH  SCALE OBJECTS");
        demo_fix_puts(2u, 2u, "THREE EFFECT LANES APPROACH CAMERA", 1u);
        demo_fix_puts(2u, 3u, "EAGLE AT 30%  HARDWARE DEPTH SCALE", 0u);
        demo_fix_puts(13u, 5u, "DEPTH RIDE", 2u);
        snd_cross_to(SOUND_MUSIC_C);

        /* Was the forest background plus a crude ASCII "\"/"/" overlay
         * simulating road-perspective lines - an incoherent mashup
         * that didn't actually read as a road.  Tried a downloaded
         * "dark forest path" image next, but it read as too dark/ugly
         * against the rest of the demo - swapped for the second
         * background already sitting in the pipeline instead. */
        demo_load_screen_palette(U_BG_MOUNTAIN);
        for (i = 0u; i < OBJ_COUNT; i++) demo_load_screen_palette(obj_frame[i]);
        hero_scale(U_SCALE_30);
        hero_place(160, 96);

        for (t = 0u; t < 560u; t++) {
            uint8_t frame = s_flight_frames[(t / 8u) % 2u];
            int16_t bg_x = -(int16_t)(t % 256u);

            draw_scrolling_background(U_BG_MOUNTAIN, bg_x, 0);

            for (i = 0u; i < OBJ_COUNT; i++) {
                uint8_t scale;
                int16_t sx;
                int16_t sy;

                z[i] = (int16_t)(z[i] - 1);
                if (z[i] < Z_NEAR) z[i] = Z_FAR;

                scale = (uint8_t)(0x30u + (((uint16_t)(Z_FAR - z[i]) * 0x80u) / (Z_FAR - Z_NEAR)));
                sx = (int16_t)(160 + ((lane_x[i] * (Z_FAR - z[i])) / (Z_FAR - Z_NEAR)));
                sy = (int16_t)(74 + ((Z_FAR - z[i]) * 70) / (Z_FAR - Z_NEAR));

                draw_asset_bottom_center(obj_frame[i],
                                         (uint16_t)(110u + i * 24u),
                                         sx, sy, scale, scale);
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

    chap_letterbox_next(5u, 3u);
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

    for (t = 0u; t < 600u; t++) {            /* 15 sec */
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
                   ? s_hero_walk[(t / HERO_CAD_WALK) % 8u]
                   : s_hero_stand[(t / HERO_CAD_STAND) % 8u];
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


    /*
     * Animated sign-off instead of a static line: the studio name and
     * the thanks both ride the palette-cycle shimmer, and a rule wipes
     * in underneath them a cell at a time.  Same restamp-with-rotating-
     * palette technique the FIX-FX chapter demonstrates, which is how
     * an arcade credit roll earns its shine without extra tiles.
     */
    {
        uint16_t t;
        for (t = 0u; t < 240u; t++) {
            fix_cycle_puts(10u, 20u, "EAGLE SOFTWARE 2026", t, 7u);
            fix_cycle_puts(10u, 24u, "THANKS FOR PLAYING.", t, 5u);

            /* Rule wipes outward from the centre as the credits hold. */
            {
                uint8_t half = (uint8_t)((t < 120u) ? (t / 8u) : 15u);
                uint8_t k;
                for (k = 0u; k <= half; k++) {
                    demo_fix_puts((uint8_t)(19u - k), 22u, "-", 1u);
                    demo_fix_puts((uint8_t)(20u + k), 22u, "-", 1u);
                }
            }

            if (uframe()) return 1u;
        }
    }

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
    /* Was {4,7,9,5} - infix 5.png ("DEBUG HUD") is flat green-on-black
     * text with no real colour variety, reading as a black-and-white
     * eyesore next to the other three banners' bordered, multi-colour
     * bitmap art.  infix 2.png ("PRESS START", rainbow-coloured per
     * letter) was already sitting unused in the same asset folder. */
    static const uint8_t  BANNER_IDX[4] = { 4u, 7u, 9u, 2u };
    static const char *const BANNER_LBL[4] = {
        "INFIX 4.PNG", "INFIX 7.PNG",
        "INFIX 9.PNG", "INFIX 2.PNG"
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

/*
 * Run one chapter, honouring a C restart request.
 *
 * The chapter function is simply called again from the top, so its own
 * chap_header() and setup re-run and every bit of state it owns is
 * rebuilt - no chapter needs its own reset path.  s_chapter_view_index
 * is rewound first because chap_header() increments it, and a restart
 * should redraw the same chapter number rather than counting upward.
 */
static void NEOGEO_USER run_chapter(uint8_t (*fn)(void))
{
    uint8_t index_before = s_chapter_view_index;

    do {
        s_chapter_view_index = index_before;
        s_restart_requested  = 0u;
        (void)fn();
    } while (s_restart_requested);

    s_restart_requested = 0u;
}

/* ================================================================== */
/*  Public entry — 21-chapter linear flow                                */
/* ================================================================== */
void NEOGEO_USER demo_unified_run(void)
{
    s_chapter_view_index = 0u;
    run_chapter(chap_boot);
    run_chapter(chap_title);
    run_chapter(chap_fix);
    run_chapter(chap_fix_fx);
    run_chapter(chap_sprite);
    run_chapter(chap_chars);
    run_chapter(chap_char_select);
    run_chapter(chap_physics);
    run_chapter(chap_camera);
    run_chapter(chap_palette_fx);
    run_chapter(chap_particles);
    run_chapter(chap_particle_showcase);
    run_chapter(chap_feedback);
    run_chapter(chap_depthfx);
    run_chapter(chap_depth_parallax);
    run_chapter(chap_npcs);
    run_chapter(chap_mini_game);
    run_chapter(chap_joystick);
    run_chapter(chap_scrolling_level);
    run_chapter(chap_char_2d);
    run_chapter(chap_raytrace3d);
    run_chapter(chap_garden3d);
    run_chapter(chap_sound);
    run_chapter(chap_ssg_arcade);
    run_chapter(chap_galaxy_skylance); /* combined arcade shooter */
    run_chapter(chap_credits);
}

#ifdef __cplusplus
}  /* extern "C" */
#endif
