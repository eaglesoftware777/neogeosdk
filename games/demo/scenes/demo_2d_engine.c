#include "demo_2d_engine.h"
#include "demo_screen.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_engine.h"
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
void NEOGEO_USER load_palettes(uint16_t *p_palette, uintptr_t palette_offset);
void NEOGEO_USER setpal(uint16_t *pal_tile,
    uint16_t t0, uint16_t t1, uint16_t t2, uint16_t t3,
    uint16_t t4, uint16_t t5, uint16_t t6, uint16_t t7,
    uint16_t t8, uint16_t t9, uint16_t t10, uint16_t t11,
    uint16_t t12, uint16_t t13, uint16_t t14, uint16_t t15);

/*
 * Main character screen IDs 11-93 (walk, attack, FX frames).
 * NPC frames are looked up through generated metadata.
 */

#define DEMO_MAIN_X            20
#define DEMO_MAIN_Y           (-34)
#define DEMO_MAIN_ATTACK_X     18
#define DEMO_MAIN_ATTACK_Y    (-38)
/* FX overlay aligned with hero canvas — was 94 which caused right-edge wrap */
#define DEMO_FX_X              18
#define DEMO_FX_Y             (-38)
/* NPC vertical position: scan ~140 (lower-center, natural floor level) */
#define DEMO_NPC_Y            (-60)

/*
 * cat_01..cat_12 = screens 110-121 in the generated manifest.
 * The sequence uses one shared palette to avoid frame-to-frame color shifts.
 */
#define DEMO_NPC_FRAME_COUNT    12u
#define DEMO_NPC_FIRST_SCREEN  110u
#define DEMO_NPC_SCREEN(n)     ((uint8_t)(DEMO_NPC_FIRST_SCREEN + ((uint8_t)(n) % DEMO_NPC_FRAME_COUNT)))
#define DEMO_NPC_TILE(n)       DEMO_SCREEN_TILE(DEMO_NPC_SCREEN(n))
#define DEMO_NPC_PALETTE(n)    DEMO_SCREEN_PALETTE(DEMO_NPC_FIRST_SCREEN)
#define DEMO_NPC_STRIPS        12u
#define DEMO_NPC_ROWS          15u
#define DEMO_NPC_STRIDE         16u
#define DEMO_NPC_OFFSET_Y     (-240)

/* Sprite slot outside the NGCharacter range (NG_SPR_CHAR_FIRST=1..NG_SPR_CHAR_LAST=299)
 * used by the scale/flip showcase to avoid overlapping live engine slots. */
#define DEMO_SCALE_SLOT         300u

/*
 * Depth sort/FX Y range.
 * DEPTH_FAR_Y=60  → scan line ~20 (top of screen, small = far).
 * DEPTH_NEAR_Y=-50 → scan line ~130 (lower screen, large = near).
 * Both values account for the NGCharacter NPC draw offset.
 */
#define DEPTH_FAR_Y    60
#define DEPTH_NEAR_Y  (-50)

/* Linear scale: DEPTH_FAR_Y → 0x80 (50%, small/far), DEPTH_NEAR_Y → 0xFF (100%, large/near). */
static uint8_t NEOGEO_USER demo_depth_scale(int16_t y)
{
    int16_t span = (int16_t)(DEPTH_FAR_Y - DEPTH_NEAR_Y);
    int16_t dist = (int16_t)(DEPTH_FAR_Y - y);
    if (dist < 0) dist = 0;
    if (dist > span) dist = span;
    return (uint8_t)(0x80u + (uint16_t)((uint16_t)127u * (uint16_t)dist) / (uint16_t)span);
}

static void NEOGEO_USER demo_engine_header(const char *phase, const char *line)
{
    clearFix();
    if (phase) demo_fix_puts(2, 0, phase, 2);
    if (line) demo_fix_puts(2, 1, line, 1);
    demo_fix_puts(2, 27, "A:NEXT  START:SKIP", 0);
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

static void NEOGEO_USER demo_preload_npc_palette(uint8_t frame)
{
    (void)frame;
    demo_load_screen_palette(DEMO_NPC_FIRST_SCREEN);
}

/*
 * NPC scene: hero stands center stage while two NGCharacter NPCs walk in
 * opposite directions and pass through the hero area.
 *
 * Hero uses slot 320 — above NG_SPR_CHAR_LAST=299 — so it shares the frame
 * with ng_chars_draw without any slot collision.  demo_draw_sprite_screen
 * also loads the hero palette on every call, so no separate preload is needed.
 *
 * NPC content sits at canvas column 5, so visible body starts at x+80.
 * x=-80 → body at screen 0 (left edge)
 * x=+144 → body at screen 224 (right side, 96px body fits before edge 320)
 * Travel range 224px gives both NPCs a full-screen crossing pass each loop.
 */
static void NEOGEO_USER demo_npc_scene(void)
{
    NGCharacter *ca, *cb;
    uint16_t t;

    demo_prepare_scene("NPC WALK CAST", "HERO + 2 SMALL NPCs  LIVE ENGINE");
    demo_fix_puts(2, 7, "DEPTH SORTED  TILE CYCLE  HFLIP", 1);
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);
    playSFX(SOUND_SFX_STRING_PHRASE);

    ng_chars_init();

    /* NPCs at DEMO_NPC_Y (-60) = scan ~140, lower-center, natural floor level.
     * scale 0x70 (~44%) makes them clearly smaller than the full-canvas hero. */
    ca = chars_add(0, -40, DEMO_NPC_Y);
    cb = chars_add(0, 120, DEMO_NPC_Y);

    if (ca) {
        demo_preload_npc_palette(0);
        ng_char_set_sprite(ca, 0, DEMO_NPC_STRIPS, DEMO_NPC_ROWS,
                           DEMO_NPC_TILE(0), DEMO_NPC_PALETTE(0));
        ng_char_set_tile_stride(ca, DEMO_NPC_STRIDE);
        ca->sprite_offset_y = DEMO_NPC_OFFSET_Y;
        ca->scale_x = 0x70;
        ca->scale_y = 0x70;
    }
    if (cb) {
        demo_preload_npc_palette(6);
        ng_char_set_sprite(cb, 0, DEMO_NPC_STRIPS, DEMO_NPC_ROWS,
                           DEMO_NPC_TILE(6), DEMO_NPC_PALETTE(6));
        ng_char_set_tile_stride(cb, DEMO_NPC_STRIDE);
        cb->sprite_offset_y = DEMO_NPC_OFFSET_Y;
        cb->flip_x = 1;
        cb->sprite_dirty = 1;
        cb->scale_x = 0x70;
        cb->scale_y = 0x70;
    }

    for (t = 0; t < 420u; t++) {
        uint8_t hero_frame = (uint8_t)(11u + ((t / 26u) % 9u));
        uint8_t fa = (uint8_t)((t / 18u) % DEMO_NPC_FRAME_COUNT);
        uint8_t fb = (uint8_t)(((t + 6u) / 18u) % DEMO_NPC_FRAME_COUNT);
        uint16_t ta_tile = DEMO_NPC_TILE(fa);
        uint16_t tb_tile = DEMO_NPC_TILE(fb);
        uint8_t ta_pal = DEMO_NPC_PALETTE(fa);
        uint8_t tb_pal = DEMO_NPC_PALETTE(fb);
        demo_preload_npc_palette(fa);
        demo_preload_npc_palette(fb);

        /* Hero at slot 320 — above NGCharacter range, no slot conflict. */
        demo_draw_sprite_screen(hero_frame, 320u, DEMO_MAIN_X, DEMO_MAIN_Y, 16, 16, 0xFF, 0xFF);

        if (ca) {
            ca->x = (int16_t)(-40 + (int16_t)(t % 180u));
            if (ca->sprite_tile != ta_tile || ca->palette != ta_pal) {
                ca->sprite_tile = ta_tile;
                ca->palette = ta_pal;
                ca->sprite_dirty = 1;
            }
        }
        if (cb) {
            cb->x = (int16_t)(120 - (int16_t)(t % 180u));
            if (cb->sprite_tile != tb_tile || cb->palette != tb_pal) {
                cb->sprite_tile = tb_tile;
                cb->palette = tb_pal;
                cb->sprite_dirty = 1;
            }
        }

        ng_chars_draw();
        if ((t % 108u) == 0u) playSFX(SOUND_SFX_FOOTSTEP);
        if (demo_wait_short(1)) break;
    }

    ng_chars_init();
    demo_clear_scene();
}

/*
 * Scale matrix: 4 NPCs side by side at decreasing scales simultaneously.
 * Shows hardware sprite scaling in one glance — far more readable than
 * sequential phases.  Slots 300-323 (4 × 6 strips = 24 slots).
 */
static void NEOGEO_USER demo_scale_matrix(void)
{
    NGSpriteGroup g[4];
    uint16_t t;
    uint8_t i;
    /* Scales and X positions for 4 NPC-sized sprites side by side */
    static const uint8_t  scales[4]   = { 0xFF, 0xC8, 0x96, 0x64 };
    static const int16_t  xpos[4]     = { 12,   76,  148,  216  };
    static const uint16_t slots[4]    = { 300,  312,  324,  336  };

    demo_prepare_scene("HARDWARE SCALE MATRIX",
                       "4 SIZES LIVE  SCB2 XSCALE/YSCALE");
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);
    playSFX(SOUND_SFX_LOW_DRUM);

    /* Draw scale labels once on FIX (no per-frame rewrite) */
    demo_fix_puts(3,  9, "100%", 1);
    demo_fix_puts(10, 9, " 78%", 1);
    demo_fix_puts(18, 9, " 59%", 2);
    demo_fix_puts(27, 9, " 39%", 2);
    demo_fix_puts(1, 10, "0xFF 0xC8 0x96 0x64", 0);

    for (t = 0; t < 420u; t++) {
        uint8_t frame = (uint8_t)((t / 18u) % DEMO_NPC_FRAME_COUNT);
        uint16_t tile  = DEMO_NPC_TILE(frame);
        uint8_t pal = DEMO_NPC_PALETTE(frame);
        demo_preload_npc_palette(frame);

        for (i = 0; i < 4u; i++) {
            ng_sprite_group_init(&g[i], slots[i], DEMO_NPC_STRIPS, DEMO_NPC_ROWS,
                                 tile, pal);
            ng_sprite_group_set_tile_stride(&g[i], DEMO_NPC_STRIDE);
            ng_sprite_group_set_active_rows(&g[i], DEMO_NPC_ROWS);
            ng_sprite_group_set_scale(&g[i], scales[i], scales[i]);
            ng_sprite_group_set_pos(&g[i], xpos[i], (int16_t)(-40));
            ng_sprite_group_upload(&g[i]);
        }

        if ((t % 120u) == 0u) playSFX(SOUND_SFX_BLADE_WHOOSH);
        if (demo_wait_short(1)) return;
    }
}

/*
 * Palette flash: every 90 frames, overwrite the sprite palette with solid
 * white for 2 frames then let demo_main_frame restore it naturally.
 * The classic NeoGeo "hit flash" — zero extra sprite cost.
 */
static void NEOGEO_USER demo_palette_flash(void)
{
    uint16_t t;
    uint16_t white_pal[16];
    uint8_t i;

    demo_prepare_scene("PALETTE HIT FLASH",
                       "ZERO-COST WHITE-OUT  PALETTE RAM SWAP");
    demo_fix_puts(2, 7, "WHOLE SPRITE INVERTS WITHOUT REWRITING", 1);
    demo_fix_puts(2, 8, "TILE DATA — JUST ONE PALETTE WRITE", 0);
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    for (i = 0; i < 16u; i++) white_pal[i] = 0x7FFFu;
    white_pal[0] = 0x0000u;  /* index 0 = transparent */

    for (t = 0; t < 360u; t++) {
        uint8_t frame    = (uint8_t)(11u + ((t / 24u) % 9u));
        uint8_t pal_bank = DEMO_SCREEN_PALETTE(frame);

        /* demo_main_frame calls showScreen which reloads the original palette */
        demo_main_frame(frame, DEMO_MAIN_X, DEMO_MAIN_Y, 0xFF, 0xFF);

        /* For 2 frames after each hit: overwrite palette → all white */
        if ((t % 90u) < 2u) {
            load_palettes(white_pal, (uintptr_t)(PALETTES + PALOFFSET * (uintptr_t)pal_bank));
            if ((t % 90u) == 0u && t > 0u) playSFX(SOUND_SFX_IMPACT_HIT);
        }

        if (demo_wait_short(1)) return;
    }
}

/*
 * Live 2D engine scene: Y-depth sort.
 *
 * Two NGCharacter NPCs sweep opposite Y directions.  When their Y values
 * cross, ng_chars_depth_sort() reassigns hardware slot order so the
 * higher-Y (nearer) character always renders in front.  The visible
 * overlap swap each crossing proves the sorter is live every frame.
 */
static void NEOGEO_USER demo_live_depth_sort(void)
{
    NGCharacter *ca, *cb;
    int16_t ya, yb;
    uint16_t t;

    demo_prepare_scene("LIVE ENGINE: Y-DEPTH SORT",
                       "LOWER ON SCREEN = NEARER = FRONT");
    demo_fix_puts(2, 7, "2 CHARS CROSS: SLOT SWAPS LIVE", 1);
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);
    demo_preload_npc_palette(0);

    ng_chars_init();

    ya = DEPTH_FAR_Y;
    yb = DEPTH_NEAR_Y;

    ca = chars_add(0, 55, ya);
    cb = chars_add(0, 82, yb);

    if (ca) {
        demo_preload_npc_palette(0);
        ng_char_set_sprite(ca, 0, DEMO_NPC_STRIPS, DEMO_NPC_ROWS,
                           DEMO_NPC_TILE(0), DEMO_NPC_PALETTE(0));
        ng_char_set_tile_stride(ca, DEMO_NPC_STRIDE);
        ca->sprite_offset_y = DEMO_NPC_OFFSET_Y;
    }
    if (cb) {
        demo_preload_npc_palette(6);
        ng_char_set_sprite(cb, 0, DEMO_NPC_STRIPS, DEMO_NPC_ROWS,
                           DEMO_NPC_TILE(6), DEMO_NPC_PALETTE(6));
        ng_char_set_tile_stride(cb, DEMO_NPC_STRIDE);
        cb->sprite_offset_y = DEMO_NPC_OFFSET_Y;
        cb->flip_x = 1;
        cb->sprite_dirty = 1;
    }

    for (t = 0; t < 480u; t++) {
        uint8_t fa = (uint8_t)((t / 18u) % DEMO_NPC_FRAME_COUNT);
        uint8_t fb = (uint8_t)(((t + 6u) / 18u) % DEMO_NPC_FRAME_COUNT);
        uint16_t ta_tile = DEMO_NPC_TILE(fa);
        uint16_t tb_tile = DEMO_NPC_TILE(fb);
        uint8_t ta_pal = DEMO_NPC_PALETTE(fa);
        uint8_t tb_pal = DEMO_NPC_PALETTE(fb);
        demo_preload_npc_palette(fa);
        demo_preload_npc_palette(fb);

        if (ca) {
            uint8_t sc = demo_depth_scale(ya);
            if (ca->sprite_tile != ta_tile || ca->palette != ta_pal) {
                ca->sprite_tile = ta_tile;
                ca->palette = ta_pal;
                ca->sprite_dirty = 1;
            }
            ca->y = ya;
            ca->scale_x = sc;
            ca->scale_y = sc;
        }
        if (cb) {
            uint8_t sc = demo_depth_scale(yb);
            if (cb->sprite_tile != tb_tile || cb->palette != tb_pal) {
                cb->sprite_tile = tb_tile;
                cb->palette = tb_pal;
                cb->sprite_dirty = 1;
            }
            cb->y = yb;
            cb->scale_x = sc;
            cb->scale_y = sc;
        }

        /* ca: FAR (top/small) → NEAR (bottom/large) → repeat. cb: opposite. */
        ya--;
        if (ya < DEPTH_NEAR_Y) ya = DEPTH_FAR_Y;
        yb++;
        if (yb > DEPTH_FAR_Y) yb = DEPTH_NEAR_Y;

        ng_chars_draw();

        if ((t % 144u) == 0u) playSFX(SOUND_SFX_FOOTSTEP);
        if (demo_wait_short(1)) break;
    }

    ng_chars_init();
    demo_clear_scene();
}

/*
 * Perspective crowd: 5 NPCs spread across X, each marching toward the camera.
 * Scale grows as Y moves from FAR (top/small) to NEAR (bottom/large).
 * Staggered Y-start phases mean all depth levels are visible simultaneously.
 * This is the core brawler engine selling point — enemies at different depths.
 */
static void NEOGEO_USER demo_perspective_crowd(void)
{
#define CROWD_COUNT 5u
    NGCharacter *chars[CROWD_COUNT];
    int16_t cy[CROWD_COUNT];
    uint8_t i;
    uint16_t t;

    /* NPCs spread across screen width, each at a different starting Y depth */
    static const int16_t start_x[CROWD_COUNT] = { 20, 68, 116, 164, 212 };
    static const int16_t start_y[CROWD_COUNT] = {
        DEPTH_NEAR_Y,
        (int16_t)(DEPTH_NEAR_Y + (DEPTH_FAR_Y - DEPTH_NEAR_Y) / 4),
        (int16_t)(DEPTH_NEAR_Y + (DEPTH_FAR_Y - DEPTH_NEAR_Y) / 2),
        (int16_t)(DEPTH_NEAR_Y + (DEPTH_FAR_Y - DEPTH_NEAR_Y) * 3 / 4),
        DEPTH_FAR_Y
    };

    demo_prepare_scene("BRAWLER PERSPECTIVE CROWD",
                       "5 NPCS  DEPTH SCALE  Y-SORT  LIVE");
    demo_fix_puts(2, 7,  "BOTTOM = NEAR = LARGE", 1);
    demo_fix_puts(2, 8,  "TOP    = FAR  = SMALL", 2);

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);
    playSFX(SOUND_SFX_STRING_PHRASE);
    demo_preload_npc_palette(0);
    ng_chars_init();

    for (i = 0; i < CROWD_COUNT; i++) {
        uint8_t fn = (uint8_t)((i * 2u) % DEMO_NPC_FRAME_COUNT);
        cy[i] = start_y[i];
        chars[i] = chars_add(0, start_x[i], cy[i]);
        if (chars[i]) {
            demo_preload_npc_palette(fn);
            ng_char_set_sprite(chars[i], 0, DEMO_NPC_STRIPS, DEMO_NPC_ROWS,
                               DEMO_NPC_TILE(fn), DEMO_NPC_PALETTE(fn));
            ng_char_set_tile_stride(chars[i], DEMO_NPC_STRIDE);
            chars[i]->sprite_offset_y = DEMO_NPC_OFFSET_Y;
        }
    }

    for (t = 0; t < 480u; t++) {
        uint8_t anim_frame = (uint8_t)((t / 18u) % DEMO_NPC_FRAME_COUNT);
        uint8_t anim_pal = DEMO_NPC_PALETTE(anim_frame);
        demo_preload_npc_palette(anim_frame);

        for (i = 0; i < CROWD_COUNT; i++) {
            NGCharacter *c = chars[i];
            uint16_t tile;
            uint8_t sc;
            if (!c) continue;

            tile = DEMO_NPC_TILE(anim_frame);
            if (c->sprite_tile != tile || c->palette != anim_pal) {
                c->sprite_tile = tile;
                c->palette = anim_pal;
                c->sprite_dirty = 1;
            }

            /* March toward camera: Y decreases = moves to NEAR (bottom/large) */
            cy[i] = (int16_t)(cy[i] - 1);
            if (cy[i] < DEPTH_NEAR_Y) cy[i] = DEPTH_FAR_Y;
            c->y = cy[i];

            sc = demo_depth_scale(cy[i]);
            c->scale_x = sc;
            c->scale_y = sc;
        }

        ng_chars_draw();

        if ((t % 90u) == 0u) playSFX(SOUND_SFX_FOOTSTEP);
        if (demo_wait_short(1)) break;
    }

    ng_chars_init();
    demo_clear_scene();
#undef CROWD_COUNT
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
    demo_scale_matrix();
    demo_live_depth_sort();
    demo_perspective_crowd();
    demo_palette_flash();

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
