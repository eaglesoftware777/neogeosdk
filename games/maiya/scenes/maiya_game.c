/*
 * Maiya: Super Nature Girl - a Neo Geo arcade nature adventure.
 *
 * Six valleys of multi-tier road: climbing vines to the canopy, villagers
 * who stop Maiya for a word, allies caged by the blight, hidden roses and
 * sunlight seeds, an Ancient Nature Gate that only the Golden Sun Key will
 * open, and the guardian waiting in the arena behind it.
 *
 * Built on the NeoGeoSDK 2D engine with YM2610 sound.
 */
#include "maiya_game.h"
#include "sdk/2d_engine/ng_engine.h"
#include "sdk/sound_ids.h"
#include "artbox/generated/maiya_assets.h"
#include "maiya_levels.h"   /* the mission tables name decoration tiles */
#include <stddef.h>

#pragma GCC optimize ("O2")

void NEOGEO_USER waitVbl(void);

void *NEOGEO_USER memcpy(void *destination, const void *source, size_t count)
{
    volatile uint8_t *out = (volatile uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    while (count--) *out++ = *in++;
    return destination;
}

void *NEOGEO_USER memset(void *destination, int value, size_t count)
{
    volatile uint8_t *out = (volatile uint8_t *)destination;
    while (count--) *out++ = (uint8_t)value;
    return destination;
}

/* ------------------------------------------------------------------ */
/*  Budgets and slots                                                 */
/* ------------------------------------------------------------------ */
enum {
    MG_ENEMIES = 5, MG_SHOTS = 8, MG_SPARKS = 6, MG_ITEMS = 4,
    MG_LEDGE_BLOCKS = 12, MG_HAZARD_BLOCKS = 5, MG_DECOR_SLOTS = 8,
    MG_NPC_SLOTS = 2,

    /*
     * Sprite slots.  Higher slots draw in front, and a scanline can only
     * carry 96 strips, so every pool is sized for what one screen of road
     * can hold: scenery, ledges, dressing, then the cast on top.
     */
    SLOT_FAR = 1,        /* 32 strips, scenery far layer            */
    SLOT_ROAD = 33,      /* 32 strips, scenery road layer           */
    SLOT_LEDGE = 65,     /* 12 blocks (24 strips), one-way ledges   */
    SLOT_SHOT = 224,     /* 8 projectile sprites                    */
    SLOT_SPARK = 232,    /* 6 particle sparks                       */
    SLOT_ITEM = 238,     /* 4 pickups (2 strips each)               */
    SLOT_CAGE = 246,     /* 1 captive cage (2 strips)               */
    SLOT_GATE = 248,     /* the Ancient Nature Gate (2 strips)      */
    SLOT_HAZARD = 250,   /* 5 blocks (10 strips) fire/spikes/sludge */
    SLOT_DECOR = 260,    /* 8 pieces (16 strips) of scenery         */
    SLOT_VINE = 276,     /* 3 climbing vines (2 strips each)        */
    SLOT_HUD = 284,      /* face avatar, hearts, roses, key         */
    SLOT_TITLE = 304,    /* attract mode key visual                 */

    /* Palette banks. */
    PAL_TEXT = 0, PAL_GOLD = 1, PAL_WARN = 2, PAL_SKY = 3,
    PAL_HERO = 4, PAL_ENEMY0 = 5, PAL_ENEMY1 = 6, PAL_ENEMY2 = 7,
    PAL_BOSS = 8, PAL_ALLY = 9, PAL_BLOCK = 10, PAL_EAGLE = 11,
    PAL_TOOL = 12, PAL_PORTRAIT = 13, PAL_DECOR = 14, PAL_PROP = 15,
    PAL_ITEM = 32, PAL_NPC = 33, PAL_GATE = 34, PAL_TRINKET = 36, PAL_BG = 16,

    /* Character kinds. */
    K_PLAYER = 0, K_ENEMY = 1, K_BOSS = 2, K_ALLY = 3, K_EAGLE = 4,

    /* Game states. */
    MG_INTRO = 0, MG_PLAY, MG_CLEAR, MG_BONUS, MG_DEAD, MG_OVER, MG_ENDING, MG_DONE,
    MG_INTERLUDE,

    HERO_STRIPS = 5, HERO_ROWS = 4, HERO_STRIDE = 5,
    EAGLE_STRIPS = 3, EAGLE_ROWS = 2,
    BOSS_STRIPS = 6, BOSS_ROWS = 6, BOSS_STRIDE = 6,

    WALK_SPEED = 512, DASH_SPEED = 1088, JUMP_SPEED = 5 * NG_FP_ONE + 160,
    CLIMB_SPEED = 320,
    MAX_HP = 5, MAX_LIVES = 7, MAX_ART = 3,

    /* FIX rows: 2..29 are visible (8 px each). */
    ROW_SCORE = 2, ROW_LIVES = 4, ROW_POWER = 5, ROW_HINT = 7, ROW_CARD = 8,

    /* HUD glyphs written into the low FIX codes by build_fix_assets.py. */
    GLYPH_HEART = 1, GLYPH_ROSE = 2, GLYPH_KEY = 3, GLYPH_COIN = 4,
    GLYPH_LEAF = 5, GLYPH_SPARK = 6,
    HURT_LOCK = 60,      /* frames of mg.hurt above this lock the controls */
    SIT_DELAY = 70,      /* frames of crouching before Maiya sits down    */
    TALK_RANGE = 34,     /* how close a villager will speak up            */
    POWER_TIME = 480,    /* swiftness and might last eight seconds        */
    VEIL_TIME = 300,     /* the mist veil hides her for five              */
    ANGEL_TIME = 150,    /* how long she rises before the valley resets   */
    MAX_CONTINUES = 3,   /* the cabinet allows three, then the run is over */
    CONTINUE_TIME = 600  /* ten seconds on the clock to decide            */
};

typedef struct {
    NGCharacter *body;
    uint16_t timer;
    uint8_t type, hurt, posted;
} MGEnemy;

typedef struct {
    int16_t x, y, vx, vy;
    uint8_t life, hostile, kind;
    NGSpriteGroup sprite;
} MGShot;

typedef struct {
    int16_t x, y, vx, vy;
    uint8_t life;
    NGSpriteGroup sprite;
} MGSpark;

typedef struct {
    int16_t x, y;
    uint8_t life, kind, key, trinket;
    NGSpriteGroup sprite;
} MGItem;

typedef struct {
    NGCamera camera;
    NGCharacter *player, *boss, *rescue, *eagle;
    NGSpriteGroup far, road;
    NGSpriteGroup ledges[MG_LEDGE_BLOCKS];
    NGSpriteGroup hazards[MG_HAZARD_BLOCKS];
    NGSpriteGroup decor[MG_DECOR_SLOTS];
    NGSpriteGroup vines[MG_VINE_COUNT];
    NGSpriteGroup gate;
    const uint16_t *block_tiles;
    NGCharacter *npcs[MG_NPC_SLOTS];
    NGSpriteGroup cage;
    NGSpriteGroup hud[16];
    MGEnemy enemies[MG_ENEMIES];
    MGShot  shots[MG_SHOTS];
    MGSpark sparks[MG_SPARKS];
    MGItem  items[MG_ITEMS];

    uint32_t score;
    uint16_t tick, encounter_mask, archer_mask, checkpoint, pick_mask;
    uint16_t boss_timer, state_timer, clear_bonus;
    int16_t  boss_home;
    uint8_t  stage, state, lives, art, kills, rescue_mask;
    uint8_t  hurt, coyote, jump_buffer, drop, boss_hurt, boss_active;
    uint8_t  attack, combo, dash, dash_wait, cast, super_surge, sitting;
    uint8_t  facing, notice, hud_dirty, pause, session_over;
    uint8_t  has_key, gate_unlocked, gate_shown, key_taken;
    uint8_t  climbing, crouch_timer, npc_mask, npc_live, npc_here;
    uint16_t swift, might, veil;      /* power-ups, in frames              */
    uint8_t  flash;                   /* frames of Secret Art palette      */
    uint8_t  angel;                   /* rising-to-the-sky death           */
    uint8_t  flowers, critters;       /* bonus tally for the mission end   */
    uint8_t  power_tag;
    uint8_t  demo;                    /* attract mode plays it herself     */
    uint8_t  entrance;                /* her drop-in at the start of a run */
    uint8_t  next_stage;              /* what the interlude leads into     */
    uint8_t  continues;               /* three, and no more                */
    uint8_t  eagle_timer, ledges_used, on_ledge;
    int16_t  player_prev_y;
    uint8_t  combo_buffer[8], combo_timer;
    uint8_t  bonus_shots, bonus_hits, bonus_timer;
    int16_t  bonus_cursor_x, bonus_cursor_y;
    char     hint_text[36];
    uint8_t  hint_timer;
    uint16_t previous_joy;
} MGState;

static MGState mg;

/* ------------------------------------------------------------------ */
/*  RNG and math helpers                                              */
/* ------------------------------------------------------------------ */
static uint16_t NEOGEO_USER mg_rand(void)
{
    static uint16_t seed = 0x3C71u;
    seed = (uint16_t)(seed * 2053u + 13849u);
    return seed;
}

static int16_t NEOGEO_USER mg_abs(int16_t value)
{
    return value < 0 ? (int16_t)-value : value;
}

/* ------------------------------------------------------------------ */
/*  Palettes and text                                                 */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER mg_palette(uint8_t bank, const uint16_t *colors)
{
    load_palettes((uint16_t *)colors, PALETTES + (uint32_t)bank * 32u);
}

static void NEOGEO_USER mg_ink(uint8_t bank, uint16_t ink)
{
    uint16_t colors[16];
    uint8_t i;
    colors[0] = 0;
    for (i = 1; i < 16; i++) colors[i] = ink;
    mg_palette(bank, colors);
}

static void NEOGEO_USER mg_ui_palettes(void)
{
    mg_ink(PAL_TEXT, 0x7FFFu);   /* white  */
    mg_ink(PAL_GOLD, 0x6FE0u);   /* gold   */
    mg_ink(PAL_WARN, 0x4F44u);   /* red    */
    mg_ink(PAL_SKY,  0x39FFu);   /* cyan   */
}

static void NEOGEO_USER mg_centre(uint8_t y, const char *text, uint8_t pal)
{
    uint8_t n = 0;
    while (text[n]) n++;
    ng_fix_puts((uint8_t)(20 - n / 2), y, text, pal);
}

static void NEOGEO_USER mg_hint(const char *text, uint8_t pal, uint8_t frames)
{
    ng_fix_clear_rect(1, ROW_HINT, 38, 1, PAL_TEXT);
    mg_centre(ROW_HINT, text, pal);
    mg.hint_timer = frames;
}

static void NEOGEO_USER mg_number(uint8_t x, uint8_t y, uint32_t value, uint8_t digits, uint8_t pal)
{
    char text[10];
    uint8_t i;
    text[digits] = 0;
    for (i = digits; i--;) {
        text[i] = (char)('0' + value % 10u);
        value /= 10u;
    }
    ng_fix_puts(x, y, text, pal);
}

/* ------------------------------------------------------------------ */
/*  Scenery & Multi-Layer Backgrounds                                 */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER mg_background(uint8_t id, uint8_t restored)
{
    const uint16_t *pal;
    const uint8_t *far_map, *road_map;
    uint16_t far_tile, road_tile;
    uint8_t count, i;

    switch (id) {
    case 1:  /* Valley of Sacred Falls */
        far_tile = MG_BG1_TILE; road_tile = MG_GROUND1_TILE;
        pal = restored ? mg_bg1_pal : mg_bg1_blight_pal;
        far_map = mg_bg1_map; road_map = mg_ground1_map; count = MG_BG1_BANKS;
        break;
    case 2:  /* Azure Coral Coast */
        far_tile = MG_BG2_TILE; road_tile = MG_GROUND2_TILE;
        pal = restored ? mg_bg2_pal : mg_bg2_blight_pal;
        far_map = mg_bg2_map; road_map = mg_ground2_map; count = MG_BG2_BANKS;
        break;
    case 3:  /* Golden Autumn Grove */
        far_tile = MG_BG3_TILE; road_tile = MG_GROUND3_TILE;
        pal = restored ? mg_bg3_pal : mg_bg3_blight_pal;
        far_map = mg_bg3_map; road_map = mg_ground3_map; count = MG_BG3_BANKS;
        break;
    case 4:  /* Crystal Grotto */
        far_tile = MG_BG4_TILE; road_tile = MG_GROUND4_TILE;
        pal = restored ? mg_bg4_pal : mg_bg4_blight_pal;
        far_map = mg_bg4_map; road_map = mg_ground4_map; count = MG_BG4_BANKS;
        break;
    case 5:  /* Sacred World Tree */
        far_tile = MG_BG5_TILE; road_tile = MG_GROUND5_TILE;
        pal = restored ? mg_bg5_pal : mg_bg5_blight_pal;
        far_map = mg_bg5_map; road_map = mg_ground5_map; count = MG_BG5_BANKS;
        break;
    default: /* Sunlit Emerald Forest */
        far_tile = MG_BG0_TILE; road_tile = MG_GROUND0_TILE;
        pal = restored ? mg_bg0_pal : mg_bg0_blight_pal;
        far_map = mg_bg0_map; road_map = mg_ground0_map; count = MG_BG0_BANKS;
        break;
    }

    for (i = 0; i < count; i++) mg_palette((uint8_t)(PAL_BG + i), pal + i * 16u);

    ng_sprite_group_init(&mg.far, SLOT_FAR, 32, 12, far_tile, PAL_BG);
    ng_sprite_group_set_palette_map(&mg.far, far_map);
    ng_sprite_group_set_pos(&mg.far, 0, 0);
    ng_sprite_group_upload(&mg.far);

    ng_sprite_group_init(&mg.road, SLOT_ROAD, 32, 2, road_tile, PAL_BG);
    ng_sprite_group_set_palette_map(&mg.road, road_map);
    ng_sprite_group_set_pos(&mg.road, 0, MG_GROUND_Y);
    ng_sprite_group_upload(&mg.road);
}

static void NEOGEO_USER mg_scroll_scenery(int16_t camera_x)
{
    ng_sprite_group_set_pos(&mg.far, (int16_t)(-(camera_x / 2) & 511), 0);
    ng_sprite_group_flush(&mg.far);
    ng_sprite_group_set_pos(&mg.road, (int16_t)(-camera_x & 511), MG_GROUND_Y);
    ng_sprite_group_flush(&mg.road);
}

/* Ledge set per valley: forest turf, mossy falls, coast sand, autumn earth,
 * grotto snow, world-tree bark. */
static const uint16_t *NEOGEO_USER mg_block_set(uint8_t stage, const uint16_t **pal)
{
    switch (stage) {
    case 1: *pal = mg_block_moss_pal;   return mg_block_moss_tiles;
    case 2: *pal = mg_block_sand_pal;   return mg_block_sand_tiles;
    case 3: *pal = mg_block_autumn_pal; return mg_block_autumn_tiles;
    case 4: *pal = mg_block_snow_pal;   return mg_block_snow_tiles;
    case 5: *pal = mg_block_bark_pal;   return mg_block_bark_tiles;
    default: *pal = mg_block_grass_pal; return mg_block_grass_tiles;
    }
}

static uint16_t NEOGEO_USER mg_hazard_tile(uint8_t type)
{
    if (type == MG_H_FIRE) return mg_prop_tiles[MG_P_LAVA];
    if (type == MG_H_SLUDGE) return mg_prop_tiles[MG_P_SLUDGE];
    return mg_prop_tiles[MG_P_SPIKES];
}

/*
 * One-way ledges are drawn as rows of 32x32 blocks (left / mid / right
 * piece) from a small pool; blocks outside the screen are released.
 */
static void NEOGEO_USER mg_draw_ledges(int16_t camera_x)
{
    const MGLevel *level = &mg_levels[mg.stage];
    uint8_t i, k, used = 0;

    for (i = 0; i < MG_PLATFORM_COUNT; i++) {
        const MGPlatform *pl = &level->platforms[i];
        uint8_t blocks;
        int16_t scr = (int16_t)(pl->x - camera_x);

        if (!pl->width) continue;
        if (scr > 336 || (int16_t)(scr + pl->width) < -16) continue;

        blocks = (uint8_t)((pl->width + 16) / 32);
        if (blocks < 2) blocks = 2;
        for (k = 0; k < blocks && used < MG_LEDGE_BLOCKS; k++) {
            int16_t bx = (int16_t)(scr + k * 32);
            NGSpriteGroup *g = &mg.ledges[used];
            uint8_t piece = k == 0 ? 0 : (k + 1 == blocks ? 2 : 1);

            if (bx > 336 || bx < -32) continue;
            ng_sprite_group_set_tile_base(g, mg.block_tiles[piece]);
            ng_sprite_group_set_pos(g, bx, pl->y);
            ng_sprite_group_set_visible(g, 1);
            ng_sprite_group_flush(g);
            used++;
        }
    }
    for (; used < MG_LEDGE_BLOCKS; used++) {
        ng_sprite_group_set_visible(&mg.ledges[used], 0);
        ng_sprite_group_flush(&mg.ledges[used]);
    }
}

static void NEOGEO_USER mg_draw_hazards(int16_t camera_x)
{
    const MGLevel *level = &mg_levels[mg.stage];
    uint8_t i, k, used = 0;

    for (i = 0; i < MG_HAZARD_COUNT; i++) {
        const MGHazard *hz = &level->hazards[i];
        uint8_t blocks;
        int16_t scr = (int16_t)(hz->x - camera_x);

        if (!hz->type) continue;
        if (scr > 336 || (int16_t)(scr + hz->width) < -16) continue;

        blocks = (uint8_t)((hz->width + 31) / 32);
        for (k = 0; k < blocks && used < MG_HAZARD_BLOCKS; k++) {
            int16_t bx = (int16_t)(scr + k * 32);
            NGSpriteGroup *g = &mg.hazards[used];

            if (bx > 336 || bx < -32) continue;
            ng_sprite_group_set_tile_base(g, mg_hazard_tile(hz->type));
            ng_sprite_group_set_pos(g, bx, MG_GROUND_Y - 32);
            ng_sprite_group_set_visible(g, 1);
            ng_sprite_group_flush(g);
            used++;
        }
    }
    for (; used < MG_HAZARD_BLOCKS; used++) {
        ng_sprite_group_set_visible(&mg.hazards[used], 0);
        ng_sprite_group_flush(&mg.hazards[used]);
    }
}

/*
 * Scenery: grass, blossoms, saplings, lanterns and signposts taken from a
 * small pool as the road scrolls past, so a valley feels lived in without
 * spending a sprite on ground that is off-screen.
 */
static void NEOGEO_USER mg_draw_decor(int16_t camera_x)
{
    uint8_t i, used = 0;

    for (i = 0; i < MG_DECOR_COUNT && used < MG_DECOR_SLOTS; i++) {
        const MGDecor *d = &mg_decor[mg.stage][i];
        int16_t scr = (int16_t)(d->x - camera_x);
        NGSpriteGroup *g;

        if (!d->x || scr < -32 || scr > 336) continue;
        g = &mg.decor[used];
        ng_sprite_group_set_tile_base(g, mg_decor_tiles[d->kind]);
        ng_sprite_group_set_pos(g, scr, (int16_t)d->y);
        ng_sprite_group_set_visible(g, 1);
        ng_sprite_group_flush(g);
        used++;
    }
    for (; used < MG_DECOR_SLOTS; used++) {
        ng_sprite_group_set_visible(&mg.decor[used], 0);
        ng_sprite_group_flush(&mg.decor[used]);
    }
}

/* Climbing vines run from the road to a canopy shelf; one sprite group each. */
static void NEOGEO_USER mg_draw_vines(int16_t camera_x)
{
    uint8_t i;

    for (i = 0; i < MG_VINE_COUNT; i++) {
        const MGVine *v = &mg_vines[mg.stage][i];
        int16_t scr = (int16_t)(v->x - camera_x);

        if (!v->x || scr < -32 || scr > 336) {
            ng_sprite_group_set_visible(&mg.vines[i], 0);
        } else {
            ng_sprite_group_set_pos(&mg.vines[i], scr, (int16_t)v->top);
            ng_sprite_group_set_visible(&mg.vines[i], 1);
        }
        ng_sprite_group_flush(&mg.vines[i]);
    }
}

/* The Ancient Nature Gate seals the guardian's arena until the key turns. */
static void NEOGEO_USER mg_draw_gate(int16_t camera_x)
{
    const MGLevel *level = &mg_levels[mg.stage];
    int16_t scr = (int16_t)((int16_t)level->gate_x - camera_x);

    if (!level->gate_x || scr < -32 || scr > 336) {
        ng_sprite_group_set_visible(&mg.gate, 0);
    } else {
        ng_sprite_group_set_tile_base(&mg.gate, mg_gate_tiles[mg.gate_unlocked ? 1 : 0]);
        ng_sprite_group_set_pos(&mg.gate, scr, (int16_t)(MG_GROUND_Y - 48));
        ng_sprite_group_set_visible(&mg.gate, 1);
    }
    ng_sprite_group_flush(&mg.gate);
}

/* The vine column Maiya is standing in, if any. */
static const MGVine *NEOGEO_USER mg_vine_at(int16_t x, int16_t y)
{
    uint8_t i;
    for (i = 0; i < MG_VINE_COUNT; i++) {
        const MGVine *v = &mg_vines[mg.stage][i];
        if (!v->x) continue;
        if (x < v->x + 2 || x > v->x + 30) continue;
        if (y < (int16_t)v->top - 4 || y > (int16_t)v->bottom + 4) continue;
        return v;
    }
    return 0;
}

static void NEOGEO_USER mg_climb_end(void)
{
    if (!mg.climbing) return;
    mg.climbing = 0;
    ng_physics_set_gravity(mg.player, 64, 6 * NG_FP_ONE);
}

static void NEOGEO_USER mg_draw_cage(int16_t camera_x)
{
    if (mg.rescue) {
        ng_sprite_group_set_pos(&mg.cage, (int16_t)(mg.rescue->x - 16 - camera_x),
                                (int16_t)(mg.rescue->y - 32));
        ng_sprite_group_set_visible(&mg.cage, 1);
    } else {
        ng_sprite_group_set_visible(&mg.cage, 0);
    }
    ng_sprite_group_flush(&mg.cage);
}

static void NEOGEO_USER mg_update_sparks(int16_t camera_x)
{
    uint8_t i;
    for (i = 0; i < MG_SPARKS; i++) {
        MGSpark *p = &mg.sparks[i];
        int16_t scr_x;

        if (p->life) {
            p->life--;
            p->x = (int16_t)(p->x + p->vx);
            p->y = (int16_t)(p->y + p->vy);
            if ((p->life & 3) == 0) p->vy++;
        }
        scr_x = (int16_t)(p->x - camera_x);
        if (!p->life || scr_x < -16 || scr_x > 336) {
            p->life = 0;
            ng_sprite_group_set_visible(&p->sprite, 0);
        } else {
            ng_sprite_group_set_pos(&p->sprite, scr_x, p->y);
            ng_sprite_group_set_visible(&p->sprite, 1);
        }
        ng_sprite_group_flush(&p->sprite);
    }
}

/*
 * Engine hook, runs right after the solid resolve: land the heroine on
 * one-way ledges (unless she is dropping through) and keep her inside
 * the mission's road.
 */
static void NEOGEO_USER mg_collision_hook(void)
{
    const MGLevel *level = &mg_levels[mg.stage];
    NGCharacter *p = mg.player;
    uint8_t i;

    if (!p) return;

    if (p->x < 16) { ng_char_set_pos(p, 16, p->y); p->vx_fp = 0; }
    if (p->x > (int16_t)(level->width - 16)) {
        ng_char_set_pos(p, (int16_t)(level->width - 16), p->y);
        p->vx_fp = 0;
    }

    /* A sealed gate is a wall: the guardian waits behind it. */
    if (level->gate_x && !mg.gate_unlocked && p->x > (int16_t)(level->gate_x - 14)) {
        ng_char_set_pos(p, (int16_t)(level->gate_x - 14), p->y);
        if (p->vx_fp > 0) p->vx_fp = 0;
        if (!mg.gate_shown) {
            mg.gate_shown = 1;
            playSFX(SOUND_SFX_6);
            mg_hint(mg.has_key ? "PRESS UP: THE SUN KEY TURNS"
                               : "SEALED. FIND THE GOLDEN SUN KEY", PAL_WARN, 150);
        }
    }

    if (mg.climbing) {
        mg.on_ledge = 0;
        return;
    }

    mg.on_ledge = 0;
    if (mg.drop || p->vy_fp < 0) return;

    for (i = 0; i < MG_PLATFORM_COUNT; i++) {
        const MGPlatform *pl = &level->platforms[i];
        if (!pl->width) continue;
        if (p->x < pl->x || p->x > (int16_t)(pl->x + pl->width)) continue;
        /* Feet crossed the ledge top this frame (or rest on it). */
        if (mg.player_prev_y <= pl->y && p->y >= pl->y) {
            ng_char_set_pos(p, p->x, pl->y);
            p->vy_fp = 0;
            physics_body(p)->grounded = 1;
            mg.on_ledge = 1;
            return;
        }
    }
}

/*
 * Engine hook, runs right before the characters are drawn: follow the
 * heroine with the camera and move every world-space sprite with it.
 */
static void NEOGEO_USER mg_before_draw_hook(void)
{
    NGCharacter *p = mg.player;
    if (p) ng_camera_update(&mg.camera, p->x, 112, (int16_t)(p->vx_fp >> 8));
    ng_level_set_scroll(mg.camera.x, 0);
    mg_scroll_scenery(mg.camera.x);
    mg_draw_ledges(mg.camera.x);
    mg_draw_hazards(mg.camera.x);
    mg_draw_vines(mg.camera.x);
    mg_draw_decor(mg.camera.x);
    mg_draw_gate(mg.camera.x);
    mg_draw_cage(mg.camera.x);
    mg_update_sparks(mg.camera.x);
}

/* ------------------------------------------------------------------ */
/*  Characters and Animation                                          */
/* ------------------------------------------------------------------ */
static const uint16_t *NEOGEO_USER mg_boss_tiles(uint8_t style)
{
    switch (style) {
    case MG_B_TOAD:      return mg_boss_toad_tiles;
    case MG_B_JACKAL:    return mg_boss_jackal_tiles;
    case MG_B_OWL:       return mg_boss_owl_tiles;
    case MG_B_LEVIATHAN: return mg_boss_leviathan_tiles;
    case MG_B_SMOGGAR:   return mg_boss_smoggar_tiles;
    default:             return mg_boss_beetle_tiles;
    }
}

static const uint16_t *NEOGEO_USER mg_boss_pal(uint8_t style)
{
    switch (style) {
    case MG_B_TOAD:      return mg_boss_toad_pal;
    case MG_B_JACKAL:    return mg_boss_jackal_pal;
    case MG_B_OWL:       return mg_boss_owl_pal;
    case MG_B_LEVIATHAN: return mg_boss_leviathan_pal;
    case MG_B_SMOGGAR:   return mg_boss_smoggar_pal;
    default:             return mg_boss_beetle_pal;
    }
}

static const uint16_t *NEOGEO_USER mg_enemy_tiles(uint8_t type)
{
    switch (type) {
    case MG_E_BEETLE: return mg_beetle_tiles;
    case MG_E_CROW:   return mg_crow_tiles;
    case MG_E_GOBLIN: return mg_goblin_tiles;
    case MG_E_WORM:   return mg_worm_tiles;
    case MG_E_DRONE:  return mg_robot_tiles;
    default:          return mg_slime_tiles;
    }
}

static const uint16_t *NEOGEO_USER mg_ally_tiles(uint8_t type)
{
    switch (type) {
    case 1:  return mg_girl_tiles;
    case 2:  return mg_spirit_tiles;
    case 3:  return mg_sunboy_tiles;
    default: return mg_elder_tiles;
    }
}

static const uint16_t *NEOGEO_USER mg_ally_pal(uint8_t type)
{
    switch (type) {
    case 1:  return mg_girl_pal;
    case 2:  return mg_spirit_pal;
    case 3:  return mg_sunboy_pal;
    default: return mg_elder_pal;
    }
}

static void NEOGEO_USER mg_frame(NGCharacter *c, uint8_t frame, uint8_t flip)
{
    uint16_t tile;
    if (c->kind == K_EAGLE) {
        tile = mg_eagle_tiles[frame % 6u];
    } else if (c->kind == K_BOSS) {
        const uint16_t *bt = mg_boss_tiles(c->data0);
        tile = bt[frame % 2u];
    } else if (c->kind == K_ENEMY) {
        const uint16_t *et = mg_enemy_tiles(c->data0);
        tile = et[frame % 2u];
    } else if (c->kind == K_ALLY) {
        const uint16_t *at = mg_ally_tiles(c->data0);
        tile = at[frame % 2u];
    } else {
        tile = mg_hero_tiles[frame % MG_HERO_FRAMES];
    }

    if (c->sprite_tile != tile || c->flip_x != flip) {
        c->sprite_tile = tile;
        c->flip_x = flip;
        c->sprite_dirty = 1;
    }
}

static NGCharacter *NEOGEO_USER mg_character(uint8_t kind, int16_t x, int16_t y,
                                             uint8_t palette, uint8_t band, uint8_t subtype)
{
    NGCharacter *c = chars_add(kind, x, y);
    if (!c) return 0;
    c->data0 = subtype;
    ng_physics_detach(c);

    if (kind == K_EAGLE) {
        ng_char_set_sprite(c, NG_SPR_CHAR_FIRST, EAGLE_STRIPS, EAGLE_ROWS, mg_eagle_tiles[3], palette);
        ng_char_set_tile_stride(c, EAGLE_STRIPS);
        c->sprite_offset_x = -24;
        c->sprite_offset_y = -28;
        ng_char_set_body(c, -14, -22, 28, 22);
    } else if (kind == K_BOSS) {
        const uint16_t *bt = mg_boss_tiles(subtype);
        ng_char_set_sprite(c, NG_SPR_CHAR_FIRST, BOSS_STRIPS, BOSS_ROWS, bt[0], palette);
        ng_char_set_tile_stride(c, BOSS_STRIDE);
        c->sprite_offset_x = -48;
        c->sprite_offset_y = -94;
        ng_char_set_body(c, -28, -76, 56, 76);
    } else if (kind == K_ENEMY) {
        const uint16_t *et = mg_enemy_tiles(subtype);
        uint8_t strips = 2, rows = 2;
        int16_t ox = -16, oy = -30;
        int16_t bx = -12, by = -24, bw = 24, bh = 24;

        if (subtype == MG_E_BEETLE || subtype == MG_E_CROW) {
            strips = 3; rows = 2; ox = -24; oy = -30; bx = -16; by = -24; bw = 32; bh = 24;
        } else if (subtype == MG_E_GOBLIN || subtype == MG_E_WORM) {
            strips = 2; rows = 3; ox = -16; oy = -46; bx = -11; by = -38; bw = 22; bh = 38;
        }

        ng_char_set_sprite(c, NG_SPR_CHAR_FIRST, strips, rows, et[0], palette);
        ng_char_set_tile_stride(c, strips);
        c->sprite_offset_x = ox;
        c->sprite_offset_y = oy;
        ng_char_set_body(c, bx, by, bw, bh);
    } else if (kind == K_ALLY) {
        const uint16_t *at = mg_ally_tiles(subtype);
        ng_char_set_sprite(c, NG_SPR_CHAR_FIRST, 2, 3, at[0], palette);
        ng_char_set_tile_stride(c, 2);
        c->sprite_offset_x = -16;
        c->sprite_offset_y = -46;
        ng_char_set_body(c, -11, -40, 22, 40);
    } else {
        /* Player */
        ng_char_set_sprite(c, NG_SPR_CHAR_FIRST, HERO_STRIPS, HERO_ROWS, mg_hero_tiles[0], palette);
        ng_char_set_tile_stride(c, HERO_STRIDE);
        c->sprite_offset_x = -40;
        c->sprite_offset_y = -62;
        ng_char_set_body(c, -10, -48, 20, 48);
    }

    ng_char_set_priority(c, band, 0);
    return c;
}

/* ------------------------------------------------------------------ */
/*  Shots, Sparks, Items & Hazards                                    */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER mg_sparks(int16_t x, int16_t y)
{
    uint8_t i;
    for (i = 0; i < MG_SPARKS; i++) {
        MGSpark *p = &mg.sparks[i];
        p->x = x; p->y = y;
        p->vx = (int16_t)((i % 3) - 1);
        p->vy = -(int16_t)(1 + i / 2);
        p->life = (uint8_t)(12 + i * 2);
    }
}

static void NEOGEO_USER mg_fire(int16_t x, int16_t y, int16_t vx, int16_t vy, uint8_t hostile, uint8_t kind)
{
    uint8_t i;
    for (i = 0; i < MG_SHOTS; i++) {
        MGShot *p = &mg.shots[i];
        if (p->life) continue;
        p->x = x; p->y = y; p->vx = vx; p->vy = vy;
        p->life = 90; p->hostile = hostile; p->kind = kind;
        return;
    }
}

static uint8_t NEOGEO_USER mg_shots_in_flight(void)
{
    uint8_t i, n = 0;
    for (i = 0; i < MG_SHOTS; i++) if (mg.shots[i].life && !mg.shots[i].hostile) n++;
    return n;
}

static void NEOGEO_USER mg_drop(int16_t x, int16_t y, uint8_t kind)
{
    uint8_t i;
    for (i = 0; i < MG_ITEMS; i++) {
        MGItem *p = &mg.items[i];
        if (p->life) continue;
        p->x = x; p->y = y; p->kind = kind; p->life = 255; p->key = 0; p->trinket = 0;
        ng_sprite_group_set_tile_base(&p->sprite, mg_item_tiles[kind]);
        ng_sprite_group_set_palette(&p->sprite, PAL_ITEM);
        return;
    }
}

/* Coins, flowers, forest friends, extra lives and the three power-ups. */
static void NEOGEO_USER mg_drop_trinket(int16_t x, int16_t y, uint8_t kind)
{
    uint8_t i;
    for (i = 0; i < MG_ITEMS; i++) {
        MGItem *p = &mg.items[i];
        if (p->life) continue;
        p->x = x; p->y = y; p->kind = kind; p->life = 255; p->key = 0; p->trinket = 1;
        ng_sprite_group_set_tile_base(&p->sprite, mg_trinket_tiles[kind]);
        ng_sprite_group_set_palette(&p->sprite, PAL_TRINKET);
        return;
    }
}

/* The Golden Sun Key: the one pick-up a mission cannot be finished without. */
static void NEOGEO_USER mg_drop_key(int16_t x, int16_t y)
{
    uint8_t i;
    for (i = 0; i < MG_ITEMS; i++) {
        MGItem *p = &mg.items[i];
        if (p->life) continue;
        p->x = x; p->y = y; p->kind = MG_I_GEM; p->life = 255; p->key = 1; p->trinket = 0;
        ng_sprite_group_set_tile_base(&p->sprite, mg_item_tiles[MG_I_GEM]);
        ng_sprite_group_set_palette(&p->sprite, PAL_ITEM);
        return;
    }
}

/* ------------------------------------------------------------------ */
/*  Combat, Damage & Secret Arts                                      */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER mg_enemy_damage(MGEnemy *e, uint8_t damage)
{
    if (!e->body || e->hurt) return;
    e->hurt = 14;
    mg_sparks(e->body->x, (int16_t)(e->body->y - 20));
    playSFX(SOUND_SFX_3); /* squish / hit */

    if (damage >= e->body->hp) {
        int16_t x = e->body->x, y = e->body->y;
        ng_chars_remove(e->body);
        e->body = 0;
        mg.score += 250u;
        mg.kills++;

        /*
         * What the blight was holding: coins mostly, a heart or a rose now
         * and then, and once in a while a power-up or a forest friend.
         */
        switch (mg.kills % 12u) {
        case 3:  mg_drop_trinket(x, (int16_t)(y - 20), MG_K_SILVER); break;
        case 6:  mg_drop_trinket(x, (int16_t)(y - 20), MG_K_GOLD); break;
        case 4:  mg_drop(x, (int16_t)(y - 20), MG_I_HEART); break;
        case 8:  mg_drop(x, (int16_t)(y - 20), MG_I_ROSE_RED); break;
        case 9:  mg_drop_trinket(x, (int16_t)(y - 20), MG_K_SWIFT); break;
        case 10: mg_drop_trinket(x, (int16_t)(y - 20), MG_K_MIGHT); break;
        case 0:  mg_drop_trinket(x, (int16_t)(y - 20), MG_K_VEIL); break;
        default: break;
        }
        playSFX(SOUND_SFX_10); /* explosion */
    } else {
        e->body->hp -= damage;
    }
}

static void NEOGEO_USER mg_boss_damage(uint8_t damage)
{
    NGCharacter *b = mg.boss ? mg.boss : mg.eagle;
    if (!b || mg.boss_hurt || mg.state != MG_PLAY) return;
    mg.boss_hurt = 12;
    mg_sparks(b->x, (int16_t)(b->y - 40));
    playSFX(SOUND_SFX_4); /* metal clank / damage */

    if (damage >= b->hp) {
        b->hp = 0;
        mg.state = MG_CLEAR;
        mg.state_timer = 220;
        mg.clear_bonus = (uint16_t)(2000u + mg.stage * 1000u + (mg.player->hp * 200u));
        mg.score += mg.clear_bonus;
        playSFX(SOUND_SFX_10);
        playSFX(SOUND_SFX_13);

        /* Cleanse the world: transform to restored nature palette! */
        mg_background(mg_levels[mg.stage].background, 1);
        mg_centre(ROW_CARD + 2, "EARTH RESTORED!", PAL_GOLD);
        mg_centre(ROW_CARD + 4, "THE BLIGHT IS CLEANSED", PAL_SKY);
    } else {
        b->hp -= damage;
    }
}

static uint8_t NEOGEO_USER mg_strike(void)
{
    return (uint8_t)(mg.might ? 4 : 2);
}

static void NEOGEO_USER mg_player_damage(void)
{
    NGCharacter *p = mg.player;
    if (mg.hurt || mg.veil || mg.dash || mg.super_surge || mg.state != MG_PLAY) return;
    mg.hurt = 90;
    mg.attack = mg.combo = mg.cast = 0;
    mg.hud_dirty = 1;
    playSFX(SOUND_SFX_16); /* player hurt */
    mg_sparks(p->x, (int16_t)(p->y - 30));
    ng_camera_shake(&mg.camera, 3, 10);
    p->vx_fp = mg.facing ? 640 : -640;

    if (--p->hp == 0) {
        /*
         * Maiya does not fall over: the valley lifts her, and she rises
         * out of frame in the sunlight before the mission starts again.
         */
        mg.state = MG_DEAD;
        mg.state_timer = ANGEL_TIME;
        mg.angel = 1;
        mg.swift = mg.might = mg.veil = 0;
        p->vx_fp = p->vy_fp = 0;
        ng_physics_detach(p);
        mg_palette(PAL_HERO, mg_hero_sun_pal);
        playSFX(SOUND_SFX_10);
        playSFX(SOUND_SFX_13);
    }
}

/*
 * Secret Art.  Each valley teaches Maiya a different one, and the roses she
 * gathers are its charges.  The sunlight palette is a flash, not a costume:
 * mg.flash counts it down and hands her own colours back.
 */
static void NEOGEO_USER mg_secret_art(void)
{
    uint8_t i;
    if (mg.art == 0 || mg.state != MG_PLAY) return;
    mg.art--;
    mg.hud_dirty = 1;
    playSFX(SOUND_SFX_13); /* art power surge */

    mg_palette(PAL_HERO, mg_hero_sun_pal);
    mg.flash = 30;
    mg.attack = 16;
    ng_camera_shake(&mg.camera, 4, 16);

    for (i = 0; i < MG_ENEMIES; i++) {
        if (mg.enemies[i].body) {
            mg_sparks(mg.enemies[i].body->x, (int16_t)(mg.enemies[i].body->y - 20));
            mg_enemy_damage(&mg.enemies[i], 10);
        }
    }
    if (mg.boss) mg_boss_damage(6);

    mg_hint(mg_art_name[mg.stage], PAL_GOLD, 90);
}

/* ------------------------------------------------------------------ */
/*  Scene & Level Initialization                                      */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER mg_hud_static(void);
static void NEOGEO_USER mg_draw_lives(void);
static void NEOGEO_USER mg_spawn(void);

static void NEOGEO_USER mg_scene(uint8_t stage, uint16_t checkpoint)
{
    uint8_t i;
    const MGLevel *level = &mg_levels[stage];

    soundStopAll();
    setBACKDROP(0x8000);
    ng_fix_clear();
    ng_sprite_hide_all();
    waitVbl();
    ng_game_engine_init();
    ng_game_engine_set_hooks(0, mg_collision_hook, 0, mg_before_draw_hook, 0);

    mg_ui_palettes();
    mg.stage = stage; mg.state = MG_INTRO; mg.pause = 0;
    mg.tick = mg.boss_timer = mg.encounter_mask = mg.archer_mask = 0;
    mg.attack = mg.combo = mg.dash = mg.dash_wait = mg.boss_hurt = mg.boss_active = 0;
    mg.hurt = 0; mg.coyote = mg.jump_buffer = mg.drop = mg.cast = mg.super_surge = 0;
    mg.boss = mg.rescue = mg.eagle = 0; mg.ledges_used = 0; mg.eagle_timer = 0;
    mg.on_ledge = 0; mg.combo_timer = 0; mg.combo_buffer[0] = mg.combo_buffer[1] = 0;
    mg.climbing = 0; mg.crouch_timer = 0; mg.sitting = 0;
    mg.has_key = 0; mg.gate_unlocked = 0; mg.gate_shown = 0; mg.key_taken = 0;
    mg.npc_mask = 0; mg.npc_live = 0; mg.npc_here = 0; mg.pick_mask = 0;
    mg.swift = mg.might = mg.veil = 0; mg.flash = 0; mg.angel = 0;
    mg.power_tag = 0;
    for (i = 0; i < MG_NPC_SLOTS; i++) mg.npcs[i] = 0;
    mg.notice = 0; mg.facing = 0; mg.checkpoint = checkpoint; mg.kills = 0;
    mg.state_timer = 0;
    mg.hint_timer = 0;
    mg.previous_joy = poll_joystick();

    ng_level_set_world_bounds(0, 0, (int16_t)level->width, 224);
    ng_physics_add_solid(0, MG_GROUND_Y, (int16_t)level->width, 32, 0);

    /* Load entity palettes */
    mg_palette(PAL_HERO, mg_hero_pal);
    mg_palette(PAL_ENEMY0, mg_slime_pal);
    mg_palette(PAL_ENEMY1, mg_beetle_pal);
    mg_palette(PAL_ENEMY2, mg_crow_pal);
    mg_palette(PAL_BOSS, mg_boss_pal(level->boss_style));
    mg_palette(PAL_ALLY, mg_ally_pal(level->rescue_type[0]));
    mg_palette(PAL_EAGLE, mg_eagle_pal);
    mg_palette(PAL_TOOL, mg_tool_pal);
    mg_palette(PAL_PORTRAIT, mg_portrait_pal);
    mg_palette(PAL_PORTRAIT + 1, mg_portrait_pal + 16);
    mg_palette(PAL_PROP, mg_prop_pal);
    mg_palette(PAL_DECOR, mg_decor_pal);
    mg_palette(PAL_ITEM, mg_item_pal);
    mg_palette(PAL_TRINKET, mg_trinket_pal);
    mg_palette(PAL_GATE, mg_gate_pal);
    {
        const uint16_t *block_pal;
        mg.block_tiles = mg_block_set(stage, &block_pal);
        mg_palette(PAL_BLOCK, block_pal);
    }

    /* Load corrupted stage background */
    mg_background(level->background, 0);

    /*
     * Maiya arrives the way she leaves: out of the sky.  A fresh run drops
     * her in above the road so the first thing the player sees is her
     * landing; a checkpoint restart just puts her back on her feet.
     */
    mg.entrance = (uint8_t)(checkpoint ? 0 : 1);
    mg.player = mg_character(K_PLAYER, checkpoint ? (int16_t)checkpoint : 64,
                             (int16_t)(mg.entrance ? -24 : MG_GROUND_Y),
                             PAL_HERO, NG_RENDER_BAND_PLAYER, 0);
    if (!mg.player) return;
    mg.player->hp = mg.player->max_hp = MAX_HP;
    ng_physics_attach(mg.player, NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS);
    ng_physics_set_gravity(mg.player, 64, 6 * NG_FP_ONE);
    mg_frame(mg.player, MG_F_IDLE0, 0);
    mg.player_prev_y = mg.player->y;

    /* Camera setup */
    ng_camera_init(&mg.camera);
    ng_camera_set_bounds(&mg.camera, 0, 0, (int16_t)level->width, 224);
    ng_camera_set_dead_zone(&mg.camera, 24, 24);
    ng_camera_set_follow_speed(&mg.camera, 96);
    ng_camera_snap(&mg.camera, checkpoint > 128 ? (int16_t)(checkpoint - 128) : 0, 0);
    ng_level_set_scroll(mg.camera.x, 0);

    /* Initialize enemies, shots, sparks, items, ledges, hazards */
    for (i = 0; i < MG_ENEMIES; i++) mg.enemies[i].body = 0;
    for (i = 0; i < MG_SHOTS; i++) {
        mg.shots[i].life = 0;
        ng_sprite_group_init(&mg.shots[i].sprite, (uint16_t)(SLOT_SHOT + i), 1, 1,
                             MG_TOOL_TILE + MG_T_THORN0, PAL_TOOL);
    }
    for (i = 0; i < MG_SPARKS; i++) {
        mg.sparks[i].life = 0;
        ng_sprite_group_init(&mg.sparks[i].sprite, (uint16_t)(SLOT_SPARK + i), 1, 1,
                             MG_TOOL_TILE + MG_T_SPARK, PAL_TOOL);
    }
    for (i = 0; i < MG_ITEMS; i++) {
        mg.items[i].life = 0;
        mg.items[i].key = 0;
        ng_sprite_group_init(&mg.items[i].sprite, (uint16_t)(SLOT_ITEM + i * 2), 2, 2,
                             mg_item_tiles[MG_I_HEART], PAL_ITEM);
        ng_sprite_group_set_visible(&mg.items[i].sprite, 0);
    }
    for (i = 0; i < MG_LEDGE_BLOCKS; i++) {
        ng_sprite_group_init(&mg.ledges[i], (uint16_t)(SLOT_LEDGE + i * 2), 2, 2,
                             mg.block_tiles[1], PAL_BLOCK);
        ng_sprite_group_set_visible(&mg.ledges[i], 0);
    }
    for (i = 0; i < MG_HAZARD_BLOCKS; i++) {
        ng_sprite_group_init(&mg.hazards[i], (uint16_t)(SLOT_HAZARD + i * 2), 2, 2,
                             mg_prop_tiles[MG_P_SPIKES], PAL_PROP);
        ng_sprite_group_set_visible(&mg.hazards[i], 0);
    }
    for (i = 0; i < MG_DECOR_SLOTS; i++) {
        ng_sprite_group_init(&mg.decor[i], (uint16_t)(SLOT_DECOR + i * 2), 2, 2,
                             mg_decor_tiles[MG_D_GRASS], PAL_DECOR);
        ng_sprite_group_set_visible(&mg.decor[i], 0);
    }
    for (i = 0; i < MG_VINE_COUNT; i++) {
        const MGVine *v = &mg_vines[stage][i];
        uint8_t rows = v->x ? (uint8_t)((v->bottom - v->top + 15) / 16) : 1;
        if (rows > 8) rows = 8;
        ng_sprite_group_init(&mg.vines[i], (uint16_t)(SLOT_VINE + i * 2), 2, 8,
                             mg_decor_tiles[MG_D_VINE], PAL_DECOR);
        ng_sprite_group_set_active_rows(&mg.vines[i], rows);
        ng_sprite_group_set_visible(&mg.vines[i], 0);
    }
    ng_sprite_group_init(&mg.gate, SLOT_GATE, 2, 3, mg_gate_tiles[0], PAL_GATE);
    ng_sprite_group_set_visible(&mg.gate, 0);

    ng_sprite_group_init(&mg.cage, SLOT_CAGE, 2, 2, mg_prop_tiles[MG_P_CHEST], PAL_PROP);
    ng_sprite_group_set_visible(&mg.cage, 0);

    /* Hidden treasures of the mission: golden roses and sunlight seeds. */
    for (i = 0; i < MG_SECRET_COUNT && i < 2; i++) {
        const MGSecret *s = &level->secrets[i];
        if (s->x) mg_drop(s->x, s->y, s->type == 1 ? MG_I_SEED : MG_I_ROSE_GOLD);
    }

    /* The Sun Key rests on a canopy shelf: climb a vine to reach it. */
    if (!mg.has_key) mg_drop_key((int16_t)mg_key_pos[stage][0], (int16_t)mg_key_pos[stage][1]);

    /* Spawn initial wave immediately so enemies are on-screen from frame 1 */
    mg_spawn();

    waitVbl();
    mg_hud_static();
    mg_draw_lives();
    {
        char stage_msg[32];
        uint8_t k = 0;
        stage_msg[0] = 'M'; stage_msg[1] = 'I'; stage_msg[2] = 'S'; stage_msg[3] = 'S'; stage_msg[4] = 'I';
        stage_msg[5] = 'O'; stage_msg[6] = 'N'; stage_msg[7] = ' ';
        stage_msg[8] = (char)('1' + stage); stage_msg[9] = ':'; stage_msg[10] = ' ';
        while (level->name[k] && k < 18) {
            stage_msg[11 + k] = level->name[k];
            k++;
        }
        stage_msg[11 + k] = '\0';
        mg_hint(stage_msg, PAL_GOLD, 120);
    }

    if (mg.entrance) {
        playSFX(SOUND_SFX_13);       /* the sun answers her */
        playSFX(SOUND_SFX_15);
        mg_centre(ROW_CARD + 6, "MAIYA: THE VALLEY CALLED ME!", PAL_SKY);
    }

    soundSetADPCMAVolume(64);
    soundSetADPCMBVolume(160);
    soundSetADPCMBLoop(1);
    soundPlayGameLoop(level->music);
}

void NEOGEO_USER maiya_boot(void)
{
    mg.lives = 3; mg.art = MAX_ART; mg.score = 0; mg.rescue_mask = 0;
    mg.continues = MAX_CONTINUES;
    mg.session_over = 0;
    mg_scene(0, 0);
}

uint8_t NEOGEO_USER maiya_session_over(void)
{
    return mg.session_over;
}

/*
 * Attract mode.  The cabinet shows the game playing itself -- a different
 * valley each time round -- and the title card in between.  The BIOS side of
 * it (coins, start) stays in user.c; this only sets the demo up and tears it
 * down again so a credited game starts from a clean slate.
 */
void NEOGEO_USER maiya_demo_begin(void)
{
    static uint8_t demo_stage = 0;

    mg.demo = 1;
    mg.lives = 3; mg.art = MAX_ART; mg.score = 0; mg.rescue_mask = 0;
    mg.continues = 0;
    mg.session_over = 0;
    mg_scene(demo_stage, 0);
    ng_fix_clear_rect(1, ROW_CARD, 38, 7, PAL_TEXT);
    mg_centre(ROW_CARD, "ATTRACT MODE", PAL_GOLD);
    mg_centre(ROW_CARD + 2, mg_levels[demo_stage].name, PAL_SKY);
    mg.state_timer = 60;

    demo_stage = (uint8_t)(demo_stage + 1);
    if (demo_stage >= MG_LEVEL_COUNT) demo_stage = 0;
}

void NEOGEO_USER maiya_demo_end(void)
{
    mg.demo = 0;
    mg.session_over = 0;
    soundStopAll();
    ng_sprite_hide_all();
    ng_fix_clear();
}

/* The demo never ends on a game over: it just picks the valley up again. */
uint8_t NEOGEO_USER maiya_demo_spent(void)
{
    return (uint8_t)(mg.state == MG_OVER || mg.state == MG_DONE ||
                     mg.state == MG_ENDING);
}

/* ------------------------------------------------------------------ */
/*  Attract Mode & Title Screen                                       */
/* ------------------------------------------------------------------ */
void NEOGEO_USER maiya_title(void)
{
    NGSpriteGroup title_vis;
    ng_sprite_hide_all();
    ng_fix_init();
    mg_ui_palettes();

    /* Load title visual */
    mg_palette(PAL_PORTRAIT, mg_title_pal);
    mg_palette(PAL_PORTRAIT + 1, mg_title_pal + 16);

    ng_sprite_group_init(&title_vis, SLOT_TITLE, 19, 14, MG_TITLE_TILE, PAL_PORTRAIT);
    ng_sprite_group_set_palette_map(&title_vis, mg_title_map);
    ng_sprite_group_set_pos(&title_vis, 8, 8);
    ng_sprite_group_upload(&title_vis);
}

/* ------------------------------------------------------------------ */
/*  Spawning: Enemies, Allies & Guardians                             */
/* ------------------------------------------------------------------ */
static MGEnemy *NEOGEO_USER mg_spawn_enemy(uint8_t type, int16_t x, int16_t y, uint8_t posted)
{
    uint8_t slot;
    MGEnemy *e;
    uint8_t pal = type == MG_E_BEETLE ? PAL_ENEMY1 : (type == MG_E_CROW ? PAL_ENEMY2 : PAL_ENEMY0);

    for (slot = 0; slot < MG_ENEMIES; slot++) if (!mg.enemies[slot].body) break;
    if (slot == MG_ENEMIES) return 0;

    e = &mg.enemies[slot];
    e->type = type;
    e->posted = posted;
    e->body = mg_character(K_ENEMY, x, y, pal, NG_RENDER_BAND_ENEMY, type);
    if (!e->body) return 0;

    e->body->hp = (uint8_t)(type == MG_E_BEETLE ? 4 : (type == MG_E_DRONE ? 3 : 2));
    e->body->max_hp = e->body->hp;

    if (!posted) {
        ng_physics_attach(e->body, NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS);
        ng_physics_set_gravity(e->body, 64, 5 * NG_FP_ONE);
    }
    e->timer = (uint16_t)(slot * 29 + (mg_rand() & 31));
    e->hurt = 0;
    return e;
}

static void NEOGEO_USER mg_spawn(void)
{
    const MGLevel *level = &mg_levels[mg.stage];
    uint8_t i;
    int16_t px = mg.player->x;

    /* Encounter waves */
    for (i = 0; i < MG_ENCOUNTER_COUNT; i++) {
        const MGEncounter *en = &level->encounters[i];
        uint16_t bit = (uint16_t)(1u << i);
        if (!en->x || (mg.encounter_mask & bit) || en->x > px + 240) continue;
        if (en->x + 200 < px) { mg.encounter_mask |= bit; continue; }

        if (en->type == MG_E_PAIR) {
            mg_spawn_enemy(MG_E_SLIME, en->x, MG_GROUND_Y, 0);
            mg_spawn_enemy(MG_E_BEETLE, (int16_t)(en->x + 40), MG_GROUND_Y, 0);
        } else {
            mg_spawn_enemy(en->type, en->x, MG_GROUND_Y, 0);
        }
        mg.encounter_mask |= bit;
    }

    /* Posted throwers / drones on ledges */
    for (i = 0; i < MG_ARCHER_COUNT; i++) {
        const MGArcher *a = &level->archers[i];
        uint16_t bit = (uint16_t)(1u << i);
        if (!a->x || (mg.archer_mask & bit) || a->x > px + 300 || a->x + 300 < px) continue;
        if (mg_spawn_enemy(MG_E_DRONE, a->x, a->y, 1)) mg.archer_mask |= bit;
    }

    /* Coins, flowers, charms and the hidden life, handed out as she nears them. */
    for (i = 0; i < MG_PICK_COUNT; i++) {
        const MGPickup *pk = &mg_picks[mg.stage][i];
        uint16_t bit = (uint16_t)(1u << i);
        if (!pk->x || (mg.pick_mask & bit)) continue;
        if (mg_abs((int16_t)(pk->x - px)) > 220) continue;
        mg_drop_trinket(pk->x, (int16_t)pk->y, pk->kind);
        mg.pick_mask |= bit;
    }

    /* Villagers who live on this road: they greet Maiya and pass on a hint. */
    for (i = 0; i < MG_NPC_SLOTS; i++) {
        if (!mg.npcs[i]) continue;
        if (mg_abs((int16_t)(mg.npcs[i]->x - px)) > 260) {
            mg.npc_here &= (uint8_t)~(1u << (uint8_t)mg.npcs[i]->data1);
            ng_chars_remove(mg.npcs[i]);
            mg.npcs[i] = 0;
            mg.npc_live--;
        }
    }
    for (i = 0; i < MG_NPC_COUNT && mg.npc_live < MG_NPC_SLOTS; i++) {
        const MGNpc *n = &mg_npcs[mg.stage][i];
        uint8_t slot;
        /* One villager stands in one place: never two copies of the elder. */
        if (!n->x || (mg.npc_here & (uint8_t)(1u << i))) continue;
        if (mg_abs((int16_t)(n->x - px)) > 200) continue;

        for (slot = 0; slot < MG_NPC_SLOTS; slot++) if (!mg.npcs[slot]) break;
        if (slot == MG_NPC_SLOTS) break;

        mg_palette((uint8_t)(PAL_NPC + slot), mg_ally_pal(n->type));
        mg.npcs[slot] = mg_character(K_ALLY, n->x, MG_GROUND_Y,
                                     (uint8_t)(PAL_NPC + slot), NG_RENDER_BAND_NPC, n->type);
        if (!mg.npcs[slot]) break;
        mg.npcs[slot]->data1 = i;
        mg.npc_here |= (uint8_t)(1u << i);
        mg_frame(mg.npcs[slot], 0, (uint8_t)(n->x > px ? 1 : 0));
        mg.npc_live++;
    }

    /* Allies to rescue */
    if (!mg.rescue) {
        for (i = 0; i < 4; i++) {
            if (!(mg.rescue_mask & (1u << i)) && level->rescue_x[i] &&
                mg_abs((int16_t)(level->rescue_x[i] - px)) < 240) {
                uint8_t ally_type = level->rescue_type[i];
                mg.rescue = mg_character(K_ALLY, (int16_t)level->rescue_x[i], MG_GROUND_Y, PAL_ALLY, NG_RENDER_BAND_NPC, ally_type);
                if (mg.rescue) {
                    mg.rescue->data0 = ally_type;
                    mg_frame(mg.rescue, 0, 0);
                    ng_sprite_group_set_pos(&mg.cage, (int16_t)(mg.rescue->x - 16), (int16_t)(mg.rescue->y - 32));
                    ng_sprite_group_set_visible(&mg.cage, 1);
                }
                break;
            }
        }
    }

    /* The guardian only shows itself once the gate is open and the arena
     * is entered -- two to four minutes of road, climbing and rescues in. */
    if (!mg.boss_active && mg.gate_unlocked && px > (int16_t)(level->gate_x + 150)) {
        mg.boss_active = 1;
        mg.boss_home = (int16_t)(level->width - 160);
        mg.boss = mg_character(K_BOSS, (int16_t)(level->width - 70), MG_GROUND_Y, PAL_BOSS, NG_RENDER_BAND_ENEMY, level->boss_style);
        if (mg.boss) {
            mg.boss->hp = mg.boss->max_hp = level->boss_hp;
            ng_physics_attach(mg.boss, NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS);
            ng_physics_set_gravity(mg.boss, 56, 6 * NG_FP_ONE);
            playSFX(SOUND_SFX_14); /* boss roar */
            mg_hint(level->guardian, PAL_WARN, 150);
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Controls, Specials & Movement                                     */
/* ------------------------------------------------------------------ */
/*
 * Attract mode plays the game itself.  The pattern below is deliberately
 * plain -- walk the road, cut what steps in front of her, hop the spikes --
 * because a demo only has to show what the valley looks like in motion.
 */
static uint16_t NEOGEO_USER mg_demo_joystick(void)
{
    uint16_t joy = JOY_RIGHT;
    uint8_t i;
    NGCharacter *p = mg.player;

    for (i = 0; i < MG_ENEMIES; i++) {
        NGCharacter *e = mg.enemies[i].body;
        if (!e) continue;
        if (mg_abs((int16_t)(e->x - p->x)) < 70 && mg_abs((int16_t)(e->y - p->y)) < 48) {
            if ((mg.tick & 15) < 4) joy |= BUTTON_B;
            break;
        }
    }
    if ((mg.tick % 170u) < 5u) joy |= BUTTON_A;
    if ((mg.tick % 620u) < 90u) joy = (uint16_t)((joy & ~JOY_RIGHT) | JOY_UP);
    if ((mg.tick % 1500u) < 6u && mg.art) joy |= BUTTON_D;
    return joy;
}

static uint16_t NEOGEO_USER mg_input(void)
{
    return mg.demo ? mg_demo_joystick() : poll_joystick();
}

static void NEOGEO_USER mg_controls(void)
{
    uint16_t joy = mg_input();
    uint16_t pressed = (uint16_t)(joy & (uint16_t)(~mg.previous_joy));
    NGCharacter *p = mg.player;
    int16_t vx = 0;

    if (mg.hurt > HURT_LOCK || mg.state != MG_PLAY) {
        mg.previous_joy = joy;
        return;
    }

    /* ---- Climbing a vine ------------------------------------------- */
    if (mg.climbing) {
        const MGVine *v = mg_vine_at(p->x, p->y);
        p->vx_fp = 0;
        p->vy_fp = 0;
        if (!v) {
            mg_climb_end();
        } else {
            if (joy & JOY_UP) {
                ng_char_set_pos(p, p->x, (int16_t)(p->y - 2));
                if (p->y <= (int16_t)v->top) {
                    /* Over the lip and onto the shelf. */
                    ng_char_set_pos(p, p->x, (int16_t)v->top);
                    mg_climb_end();
                }
            } else if (joy & JOY_DOWN) {
                ng_char_set_pos(p, p->x, (int16_t)(p->y + 2));
                if (p->y >= (int16_t)v->bottom) {
                    ng_char_set_pos(p, p->x, (int16_t)v->bottom);
                    mg_climb_end();
                }
            }
            if (pressed & BUTTON_A) {
                mg_climb_end();
                p->vy_fp = -JUMP_SPEED;
                p->vx_fp = mg.facing ? -WALK_SPEED : WALK_SPEED;
                playSFX(SOUND_SFX_15);
            }
            if (pressed & BUTTON_B) {
                mg.cast = 10;
                mg_fire(p->x, (int16_t)(p->y - 26), (int16_t)(mg.facing ? -6 : 6), 0, 0, MG_T_THORN0);
                playSFX(SOUND_SFX_2);
            }
        }
        mg.previous_joy = joy;
        return;
    }

    if ((joy & (JOY_UP | JOY_DOWN)) && mg_vine_at(p->x, p->y)) {
        mg.climbing = 1;
        mg.crouch_timer = 0;
        mg.drop = 0;
        p->vy_fp = 0;
        ng_physics_set_gravity(p, 0, 0);
        playSFX(SOUND_SFX_11);
        mg.previous_joy = joy;
        return;
    }

    /* ---- Turning the Sun Key in the Ancient Nature Gate ------------- */
    if ((pressed & JOY_UP) && mg.has_key && !mg.gate_unlocked) {
        const MGLevel *lvl = &mg_levels[mg.stage];
        if (lvl->gate_x && mg_abs((int16_t)(p->x - (int16_t)lvl->gate_x)) < 48) {
            mg.gate_unlocked = 1;
            mg.score += 3000u;
            mg.hud_dirty = 1;
            playSFX(SOUND_SFX_13);
            playSFX(SOUND_SFX_10);
            ng_camera_shake(&mg.camera, 3, 20);
            mg_hint("THE GATE OPENS. THE GUARDIAN WAITS", PAL_GOLD, 150);
            mg.previous_joy = joy;
            return;
        }
    }

    /* Track joystick motions for combo super move (Down, Forward + B) */
    if (mg.combo_timer && --mg.combo_timer == 0) mg.combo_buffer[0] = mg.combo_buffer[1] = 0;
    if (pressed & JOY_DOWN) { mg.combo_buffer[0] = 1; mg.combo_timer = 24; }
    if ((pressed & (JOY_LEFT | JOY_RIGHT)) && mg.combo_buffer[0]) mg.combo_buffer[1] = 1;

    /* Walk / Run -- the swiftness leaf lends her half a step again. */
    {
        int16_t speed = (int16_t)(mg.swift ? (WALK_SPEED * 3) / 2 : WALK_SPEED);
        if (joy & JOY_LEFT) {
            vx = (int16_t)-speed;
            mg.facing = 1;
        } else if (joy & JOY_RIGHT) {
            vx = speed;
            mg.facing = 0;
        }
    }

    /* ---- Kneel, then sit ------------------------------------------- */
    if ((joy & JOY_DOWN) && !vx && (ng_physics_is_grounded(p) || mg.on_ledge ||
                                    p->y >= MG_GROUND_Y - 4)) {
        if (mg.crouch_timer < 255) mg.crouch_timer++;
        vx = 0;
    } else if (mg.crouch_timer) {
        mg.crouch_timer = 0;
    }
    mg.sitting = mg.crouch_timer > SIT_DELAY;

    /* Jump & drop through ledges */
    if (pressed & BUTTON_A) {
        if ((joy & JOY_DOWN) && mg.on_ledge) {
            mg.drop = 12;
        } else if (ng_physics_is_grounded(p) || p->y >= MG_GROUND_Y - 4) {
            p->vy_fp = -JUMP_SPEED;
            playSFX(SOUND_SFX_15);
        }
    }

    /* Attack (B button) */
    if (pressed & BUTTON_B) {
        /* Check special combo: Down -> Forward + B = Rose Blossom Surge */
        if (mg.combo_buffer[0] && mg.combo_buffer[1]) {
            mg.super_surge = 20;
            mg.combo_buffer[0] = mg.combo_buffer[1] = 0;
            playSFX(SOUND_SFX_15);
            playSFX(SOUND_SFX_1);
        } else {
            /* Check melee distance to nearest enemy or boss */
            uint8_t melee = 0;
            if (mg.boss && mg_abs((int16_t)(mg.boss->x - p->x)) < 48) {
                mg_boss_damage(mg_strike());
                melee = 1;
            }
            if (!melee) {
                uint8_t i;
                for (i = 0; i < MG_ENEMIES; i++) {
                    if (mg.enemies[i].body && mg_abs((int16_t)(mg.enemies[i].body->x - p->x)) < 44) {
                        mg_enemy_damage(&mg.enemies[i], mg_strike());
                        melee = 1;
                        break;
                    }
                }
            }
            if (melee) {
                mg.attack = 12;
                playSFX(SOUND_SFX_1); /* whip crack */
            } else if (mg_shots_in_flight() < 3) {
                /* Throw rose thorn projectile */
                mg.cast = 10;
                mg_fire(p->x, (int16_t)(p->y - 32), (int16_t)(mg.facing ? -6 : 6), 0, 0, MG_T_THORN0);
                playSFX(SOUND_SFX_2); /* thorn toss */
            }
        }
    }

    /* Secret Art (D button) */
    if (pressed & BUTTON_D) {
        mg_secret_art();
    }

    /* Handle Super Surge dash */
    if (mg.super_surge) {
        mg.super_surge--;
        vx = (int16_t)(mg.facing ? -DASH_SPEED * 3 / 2 : DASH_SPEED * 3 / 2);
        if (mg.boss && mg_abs((int16_t)(mg.boss->x - p->x)) < 52) mg_boss_damage(3);
    }

    p->vx_fp = vx;
    mg.previous_joy = joy;
}

static void NEOGEO_USER mg_update_entities(void)
{
    uint8_t i, j;
    int16_t cam_x = mg.camera.x;
    NGCharacter *p = mg.player;

    /* Update shots */
    for (i = 0; i < MG_SHOTS; i++) {
        MGShot *s = &mg.shots[i];
        if (!s->life) {
            ng_sprite_group_set_visible(&s->sprite, 0);
            ng_sprite_group_flush(&s->sprite);
            continue;
        }
        s->x = (int16_t)(s->x + s->vx);
        s->y = (int16_t)(s->y + s->vy);
        s->life--;

        int16_t scr_x = (int16_t)(s->x - cam_x);
        if (scr_x < -16 || scr_x > 336 || s->life == 0) {
            s->life = 0;
            ng_sprite_group_set_visible(&s->sprite, 0);
            ng_sprite_group_flush(&s->sprite);
            continue;
        }

        /* Collision */
        if (!s->hostile) {
            for (j = 0; j < MG_ENEMIES; j++) {
                MGEnemy *e = &mg.enemies[j];
                if (e->body && mg_abs((int16_t)(e->body->x - s->x)) < 24 &&
                    mg_abs((int16_t)(e->body->y - s->y)) < 32) {
                    mg_enemy_damage(e, (uint8_t)(mg.might ? 2 : 1));
                    s->life = 0;
                    break;
                }
            }
            if (s->life && mg.boss && mg_abs((int16_t)(mg.boss->x - s->x)) < 40 &&
                mg_abs((int16_t)(mg.boss->y - s->y)) < 48) {
                mg_boss_damage(1);
                s->life = 0;
            }
        } else {
            if (mg_abs((int16_t)(p->x - s->x)) < 20 &&
                mg_abs((int16_t)((p->y - 24) - s->y)) < 24) {
                mg_player_damage();
                s->life = 0;
            }
        }

        if (s->life) {
            ng_sprite_group_set_pos(&s->sprite, scr_x, s->y);
            ng_sprite_group_set_visible(&s->sprite, 1);
            ng_sprite_group_upload(&s->sprite);
        } else {
            ng_sprite_group_set_visible(&s->sprite, 0);
            ng_sprite_group_flush(&s->sprite);
        }
    }

    /* Update items */
    for (i = 0; i < MG_ITEMS; i++) {
        MGItem *it = &mg.items[i];
        if (!it->life) {
            ng_sprite_group_set_visible(&it->sprite, 0);
            ng_sprite_group_flush(&it->sprite);
            continue;
        }
        int16_t scr_x = (int16_t)(it->x - cam_x);
        if (scr_x < -36 || scr_x > 340) {
            ng_sprite_group_set_visible(&it->sprite, 0);
            ng_sprite_group_flush(&it->sprite);
            continue;
        }

        /* The pick-up art is 32 px: reach for its middle, not its corner. */
        if (mg_abs((int16_t)(p->x - (int16_t)(it->x + 16))) < 28 &&
            mg_abs((int16_t)((int16_t)(p->y - 20) - (int16_t)(it->y + 16))) < 34) {
            it->life = 0;
            ng_sprite_group_set_visible(&it->sprite, 0);
            ng_sprite_group_flush(&it->sprite);
            playSFX(SOUND_SFX_11);
            if (it->key) {
                mg.has_key = 1;
                mg.score += 2000u;
                playSFX(SOUND_SFX_13);
                mg_hint("THE GOLDEN SUN KEY IS YOURS", PAL_GOLD, 150);
            } else if (it->trinket) {
                switch (it->kind) {
                case MG_K_GOLD:
                    mg.score += 500u;
                    break;
                case MG_K_SILVER:
                    mg.score += 200u;
                    break;
                case MG_K_FLOWER:
                    mg.score += 250u;
                    if (mg.flowers < 99) mg.flowers++;
                    break;
                case MG_K_CRITTER:
                    mg.score += 750u;
                    if (mg.critters < 99) mg.critters++;
                    mg_hint("A FOREST FRIEND IS FREE", PAL_SKY, 90);
                    break;
                case MG_K_LIFE:
                    if (mg.lives < MAX_LIVES) mg.lives++;
                    playSFX(SOUND_SFX_13);
                    mg_hint("EXTRA LIFE!", PAL_GOLD, 120);
                    break;
                case MG_K_SWIFT:
                    mg.swift = POWER_TIME;
                    mg.power_tag = 1;
                    mg_hint("SWIFT WIND: SHE RUNS LIGHT", PAL_SKY, 90);
                    break;
                case MG_K_MIGHT:
                    mg.might = POWER_TIME;
                    mg.power_tag = 1;
                    mg_hint("THORN MIGHT: HER STRIKE BITES", PAL_GOLD, 90);
                    break;
                case MG_K_VEIL:
                    mg.veil = VEIL_TIME;
                    mg.power_tag = 1;
                    mg_hint("MIST VEIL: NOTHING CAN TOUCH HER", PAL_SKY, 90);
                    break;
                default:  /* the elder's charm */
                    mg.score += 300u;
                    mg_hint(mg.boss_active ? mg_boss_hint[mg.stage]
                                           : mg_secret_hint[mg.stage], PAL_GOLD, 180);
                    break;
                }
            } else if (it->kind == MG_I_HEART) {
                if (p->hp < MAX_HP) p->hp++;
                mg.score += 200u;
            } else if (it->kind == MG_I_ROSE_RED) {
                if (mg.art < MAX_ART) mg.art++;
                mg.score += 500u;
            } else {
                mg.score += 1000u;
            }
            mg.hud_dirty = 1;
            continue;
        }

        ng_sprite_group_set_pos(&it->sprite, scr_x, it->y);
        ng_sprite_group_set_visible(&it->sprite, 1);
        ng_sprite_group_upload(&it->sprite);
    }

    /* Update enemies AI */
    for (i = 0; i < MG_ENEMIES; i++) {
        MGEnemy *e = &mg.enemies[i];
        if (!e->body) continue;
        e->timer++;
        if (e->hurt) e->hurt--;

        int16_t dx = (int16_t)(p->x - e->body->x);
        if (!e->posted) {
            if (e->type == MG_E_BEETLE) {
                e->body->vx_fp = dx < 0 ? -320 : 320;
                mg_frame(e->body, (uint8_t)(MG_F_WALK0 + ((e->timer / 8) % 2)), (uint8_t)(dx < 0));
            } else if (e->type == MG_E_CROW) {
                e->body->vx_fp = dx < 0 ? -480 : 480;
                e->body->vy_fp = (int32_t)((e->timer & 32) ? -120 : 120);
                mg_frame(e->body, (uint8_t)((e->timer / 6) % 2), (uint8_t)(dx < 0));
            } else if (e->type == MG_E_DRONE) {
                e->body->vx_fp = dx < 0 ? -180 : 180;
                mg_frame(e->body, (uint8_t)((e->timer / 10) % 2), (uint8_t)(dx < 0));
                if ((e->timer % 120) == 60 && mg_abs(dx) < 180) {
                    mg_fire(e->body->x, e->body->y, (int16_t)(dx < 0 ? -4 : 4), 0, 1, MG_T_BOLT);
                    playSFX(SOUND_SFX_6);
                }
            } else {
                if ((e->timer % 60) == 0 && (ng_physics_is_grounded(e->body) || e->body->y >= MG_GROUND_Y - 4)) {
                    e->body->vy_fp = -4 * NG_FP_ONE;
                    e->body->vx_fp = dx < 0 ? -240 : 240;
                }
                mg_frame(e->body, (uint8_t)((e->timer / 12) % 2), (uint8_t)(dx < 0));
                if (e->type == MG_E_SLIME && (e->timer % 150) == 75 && mg_abs(dx) < 160) {
                    mg_fire(e->body->x, (int16_t)(e->body->y - 12), (int16_t)(dx < 0 ? -3 : 3), -1, 1, MG_T_SPIT);
                    playSFX(SOUND_SFX_5);
                }
            }
        }

        if (!e->hurt && mg_abs((int16_t)(e->body->x - p->x)) < 18 &&
            mg_abs((int16_t)(e->body->y - p->y)) < 24) {
            mg_player_damage();
            /* Shove the creature off: being cornered by one slime was death. */
            e->body->vx_fp = dx < 0 ? 900 : -900;
            e->hurt = 20;
        }
    }

    /* Update boss AI */
    if (mg.boss && mg.boss_active) {
        mg.boss_timer++;
        if (mg.boss_hurt) mg.boss_hurt--;
        int16_t b_dx = (int16_t)(p->x - mg.boss->x);

        if ((mg.boss_timer % 180) < 120) {
            mg.boss->vx_fp = b_dx < 0 ? -220 : 220;
        } else if ((mg.boss_timer % 180) == 120) {
            mg.boss->vy_fp = -6 * NG_FP_ONE;
            mg.boss->vx_fp = b_dx < 0 ? -400 : 400;
            playSFX(SOUND_SFX_14);
        } else if ((mg.boss_timer % 180) == 160) {
            mg_fire(mg.boss->x, (int16_t)(mg.boss->y - 48), (int16_t)(b_dx < 0 ? -5 : 5), 0, 1, MG_T_FIRE);
            playSFX(SOUND_SFX_8);
        }
        mg_frame(mg.boss, (uint8_t)((mg.boss_timer / 16) % 2), (uint8_t)(b_dx < 0));

        if (!mg.boss_hurt && mg_abs((int16_t)(mg.boss->x - p->x)) < 36 &&
            mg_abs((int16_t)(mg.boss->y - p->y)) < 36) {
            mg_player_damage();
        }
    }

    /*
     * Invulnerability read-out.  A two-frame blink made her strobe, so the
     * flicker is slow, and the last half-second is solid again.
     */
    if (mg.hurt) {
        mg.hurt--;
        p->visible = (mg.hurt > 30 && (mg.hurt & 8)) ? 0 : 1;
    } else if (mg.veil) {
        p->visible = (mg.veil & 8) ? 0 : 1;
    } else {
        p->visible = 1;
    }

    /* Power-ups run down whether or not she is fighting. */
    if (mg.swift) mg.swift--;
    if (mg.might) mg.might--;
    if (mg.veil) mg.veil--;
    if (mg.flash && --mg.flash == 0) mg_palette(PAL_HERO, mg_hero_pal);
}

/* ------------------------------------------------------------------ */
/*  HUD Drawing                                                       */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER mg_hud_static(void)
{
    uint8_t i;
    /* Face portrait avatar: 2x2 sprite strips (slots SLOT_HUD .. SLOT_HUD + 1) */
    ng_sprite_group_init(&mg.hud[0], SLOT_HUD, 2, 2, MG_FACE_TILE, PAL_HERO);
    ng_sprite_group_set_pos(&mg.hud[0], 16, 8);
    ng_sprite_group_upload(&mg.hud[0]);

    /* Hearts: 5 strips (slots SLOT_HUD + 2 .. SLOT_HUD + 6) */
    for (i = 0; i < MAX_HP; i++) {
        ng_sprite_group_init(&mg.hud[1 + i], (uint16_t)(SLOT_HUD + 2 + i), 1, 1, MG_TOOL_TILE + MG_T_HEART, PAL_TOOL);
        ng_sprite_group_set_pos(&mg.hud[1 + i], (int16_t)(40 + i * 12), 8);
        ng_sprite_group_upload(&mg.hud[1 + i]);
    }

    /* Secret Art Rose icons: 3 strips (slots SLOT_HUD + 7 .. SLOT_HUD + 9) */
    for (i = 0; i < MAX_ART; i++) {
        ng_sprite_group_init(&mg.hud[6 + i], (uint16_t)(SLOT_HUD + 7 + i), 1, 1, MG_TOOL_TILE + MG_T_ROSE, PAL_TOOL);
        ng_sprite_group_set_pos(&mg.hud[6 + i], (int16_t)(40 + i * 14), 20);
        ng_sprite_group_upload(&mg.hud[6 + i]);
    }

    /* Sun Key: lights up in the corner once Maiya is carrying it. */
    ng_sprite_group_init(&mg.hud[9], (uint16_t)(SLOT_HUD + 10), 2, 2, mg_item_tiles[MG_I_GEM], PAL_ITEM);
    ng_sprite_group_set_pos(&mg.hud[9], 288, 8);
    ng_sprite_group_set_visible(&mg.hud[9], mg.has_key);
    ng_sprite_group_upload(&mg.hud[9]);

    ng_fix_puts(22, ROW_SCORE, "SCORE", PAL_GOLD);
    mg_number(28, ROW_SCORE, mg.score, 6, PAL_TEXT);
    ng_fix_puts(22, ROW_LIVES, "LIVES", PAL_GOLD);
}

/* Lives, drawn on the FIX layer as a little row of hearts. */
static void NEOGEO_USER mg_draw_lives(void)
{
    uint8_t i;
    for (i = 0; i < 6; i++) {
        ng_fix_putc((uint8_t)(28 + i), ROW_LIVES,
                    (char)(i < mg.lives ? GLYPH_HEART : ' '), PAL_WARN);
    }
}

/* What Maiya is carrying right now: swiftness, might, the mist veil. */
static void NEOGEO_USER mg_draw_powers(void)
{
    ng_fix_clear_rect(20, ROW_POWER, 19, 1, PAL_TEXT);
    if (mg.swift) ng_fix_puts(22, ROW_POWER, "SWIFT", PAL_SKY);
    if (mg.might) ng_fix_puts(28, ROW_POWER, "MIGHT", PAL_GOLD);
    if (mg.veil)  ng_fix_puts(34, ROW_POWER, "VEIL", PAL_SKY);
    mg.power_tag = (uint8_t)((mg.swift || mg.might || mg.veil) ? 1 : 0);
}

static void NEOGEO_USER mg_update_hud(void)
{
    uint8_t i;
    for (i = 0; i < MAX_HP; i++) {
        ng_sprite_group_set_visible(&mg.hud[1 + i], i < mg.player->hp);
    }
    for (i = 0; i < MAX_ART; i++) {
        ng_sprite_group_set_visible(&mg.hud[6 + i], i < mg.art);
    }
    ng_sprite_group_set_visible(&mg.hud[9], mg.has_key && !mg.gate_unlocked);
    ng_sprite_group_flush(&mg.hud[9]);
    mg_number(28, ROW_SCORE, mg.score, 6, PAL_TEXT);
    mg_draw_lives();
}

/* ------------------------------------------------------------------ */
/*  Main Per-Frame Update                                             */
/* ------------------------------------------------------------------ */
/* Ground hazards burn, spike or poison the heroine while she stands in them. */
static void NEOGEO_USER mg_hazard_check(void)
{
    const MGLevel *level = &mg_levels[mg.stage];
    NGCharacter *p = mg.player;
    uint8_t i;

    if (p->y < MG_GROUND_Y - 4 || mg.on_ledge || mg.climbing) return;
    for (i = 0; i < MG_HAZARD_COUNT; i++) {
        const MGHazard *hz = &level->hazards[i];
        if (hz->type && p->x > hz->x && p->x < (int16_t)(hz->x + hz->width)) {
            mg_player_damage();
            if (mg.state == MG_PLAY) p->vy_fp = -3 * NG_FP_ONE;
            return;
        }
    }
}

/* Physics, character draw and world sprites for one frame. */
static void NEOGEO_USER mg_world_step(void)
{
    mg.player_prev_y = mg.player->y;
    if (mg.drop) mg.drop--;
    ng_game_engine_frame();
}

static void NEOGEO_USER mg_animate_player(void)
{
    NGCharacter *p = mg.player;
    uint8_t grounded = ng_physics_is_grounded(p) || mg.on_ledge || (p->y >= MG_GROUND_Y - 4);

    if (mg.climbing) {
        mg_frame(p, (uint8_t)(MG_F_JUMP0 + ((mg.tick / 10) % 2)), mg.facing);
        return;
    }
    if (mg.sitting) {
        mg_frame(p, MG_F_SIT, mg.facing);
        return;
    }
    if (mg.crouch_timer) {
        mg_frame(p, MG_F_CROUCH, mg.facing);
        return;
    }
    if (mg.hurt > HURT_LOCK) {
        mg_frame(p, MG_F_HURT0, mg.facing);
    } else if (mg.super_surge) {
        mg_frame(p, (uint8_t)(MG_F_RUN0 + ((mg.tick / 4) % 3)), mg.facing);
    } else if (mg.attack) {
        mg.attack--;
        mg_frame(p, (uint8_t)(mg.attack > 6 ? MG_F_ATK1 : MG_F_ATK2), mg.facing);
    } else if (mg.cast) {
        mg.cast--;
        mg_frame(p, (uint8_t)(mg.cast > 5 ? MG_F_CAST1 : MG_F_CAST2), mg.facing);
    } else if (!grounded) {
        mg_frame(p, (uint8_t)(p->vy_fp < 0 ? MG_F_JUMP1 : MG_F_JUMP3), mg.facing);
    } else if (p->vx_fp != 0) {
        mg_frame(p, (uint8_t)(MG_F_WALK0 + ((mg.tick / 5) % 8)), mg.facing);
    } else {
        mg_frame(p, (uint8_t)(MG_F_IDLE0 + ((mg.tick / 20) % 3)), mg.facing);
    }
}

/* A villager greets Maiya once, then goes back to minding the valley. */
static void NEOGEO_USER mg_npc_check(void)
{
    uint8_t i;
    for (i = 0; i < MG_NPC_SLOTS; i++) {
        NGCharacter *n = mg.npcs[i];
        uint8_t which;
        if (!n) continue;

        which = (uint8_t)n->data1;
        mg_frame(n, (uint8_t)((mg.tick / 32) % 2), (uint8_t)(n->x > mg.player->x ? 1 : 0));
        if (mg.npc_mask & (uint8_t)(1u << which)) continue;
        if (mg_abs((int16_t)(n->x - mg.player->x)) > TALK_RANGE) continue;

        mg.npc_mask |= (uint8_t)(1u << which);
        mg_hint(mg_npcs[mg.stage][which].line, PAL_SKY, 150);
        playSFX(SOUND_SFX_11);
        mg.score += 100u;
        mg.hud_dirty = 1;
    }
}

static void NEOGEO_USER mg_rescue_check(void)
{
    uint8_t type;

    if (!mg.rescue || mg_abs((int16_t)(mg.player->x - mg.rescue->x)) >= 32) return;

    type = mg.rescue->data0;
    mg.rescue_mask |= (uint8_t)(1u << type);
    ng_chars_remove(mg.rescue);
    mg.rescue = 0;
    playSFX(SOUND_SFX_11); /* pickup chime */

    if (type == 0) {
        mg_hint("ELDER: STRIKE THE GUARDIAN'S CORE!", PAL_GOLD, 120);
    } else if (type == 1) {
        if (mg.player->hp < MAX_HP) mg.player->hp++;
        mg_hint("MAIDEN: HEALTH RESTORED!", PAL_SKY, 120);
    } else if (type == 2) {
        if (mg.art < MAX_ART) mg.art++;
        mg_hint("SPIRIT: SECRET ART RESTORED!", PAL_GOLD, 120);
    } else {
        mg_hint("SUNBOY: MAIYA! WE WON!", PAL_GOLD, 120);
    }
    mg.score += 1000u;
    mg.hud_dirty = 1;
}

/*
 * Between missions.  Sunboy is still held in the crystal, and each time a
 * valley is cleansed he can reach a little further: he reports in, counts
 * what Maiya gathered, and points her at the next road.
 */
static void NEOGEO_USER mg_interlude(uint8_t next_stage)
{
    NGSpriteGroup boy;
    uint8_t done = (uint8_t)(next_stage >= MG_LEVEL_COUNT);

    soundStopAll();
    ng_sprite_hide_all();
    ng_fix_clear();
    setBACKDROP(0x8000);
    waitVbl();
    mg_ui_palettes();
    mg_palette(PAL_ALLY, mg_sunboy_pal);

    ng_sprite_group_init(&boy, SLOT_TITLE, 2, 3, mg_sunboy_tiles[0], PAL_ALLY);
    ng_sprite_group_set_pos(&boy, 144, 60);
    ng_sprite_group_upload(&boy);

    mg_centre(4, "SUNBOY CALLS OUT", PAL_GOLD);
    mg_centre(14, mg_sunboy_line[mg.stage][0], PAL_SKY);
    mg_centre(16, mg_sunboy_line[mg.stage][1], PAL_TEXT);

    ng_fix_puts(8, 19, "FLOWERS", PAL_GOLD);
    mg_number(17, 19, mg.flowers, 2, PAL_TEXT);
    ng_fix_puts(21, 19, "FRIENDS FREED", PAL_GOLD);
    mg_number(35, 19, mg.critters, 2, PAL_TEXT);
    ng_fix_puts(8, 21, "SCORE", PAL_GOLD);
    mg_number(17, 21, mg.score, 6, PAL_TEXT);

    if (!done) mg_centre(24, mg_boss_hint[next_stage], PAL_WARN);

    mg.state = MG_INTERLUDE;
    mg.next_stage = next_stage;
    mg.state_timer = 420;
    mg.previous_joy = mg_input();
    playSFX(SOUND_SFX_13);
}

/*
 * Bonus round, in the spirit of the between-stage tests of the old arcade
 * ninja games: the blight sends a line of creatures down the road and Maiya
 * answers with thorns.  Clear enough of them and the valley gives a life.
 */
static void NEOGEO_USER mg_bonus_enter(uint8_t next_stage)
{
    uint8_t i;

    mg.state = MG_BONUS;
    mg.next_stage = next_stage;
    mg.state_timer = 1500;
    mg.bonus_hits = 0;
    mg.bonus_shots = 0;
    mg.bonus_timer = 0;
    mg.boss = 0;
    mg.boss_active = 0;
    mg.rescue = 0;

    for (i = 0; i < MG_ENEMIES; i++) {
        if (mg.enemies[i].body) {
            ng_chars_remove(mg.enemies[i].body);
            mg.enemies[i].body = 0;
        }
    }
    for (i = 0; i < MG_ITEMS; i++) mg.items[i].life = 0;

    ng_fix_clear_rect(1, ROW_HINT, 38, 1, PAL_TEXT);
    mg_centre(ROW_CARD, "BONUS ROUND", PAL_GOLD);
    mg_centre(ROW_CARD + 2, "THORN THEM BEFORE THEY REACH HER", PAL_TEXT);
    playSFX(SOUND_SFX_13);
}

static void NEOGEO_USER mg_bonus_frame(void)
{
    NGCharacter *p = mg.player;
    uint8_t i;

    /* A creature every second and a half, always from the right. */
    if ((mg.state_timer % 90u) == 0u) {
        uint8_t kind = (uint8_t)((mg.state_timer / 90u) & 1u ? MG_E_SLIME : MG_E_BEETLE);
        mg_spawn_enemy(kind, (int16_t)(p->x + 190), MG_GROUND_Y, 0);
    }

    mg_controls();
    mg_animate_player();
    mg_world_step();
    mg_update_entities();

    /* Every creature cleared here is worth a coin's weight in points. */
    if (mg.kills != mg.bonus_hits) {
        mg.bonus_hits = mg.kills;
        mg.score += 300u;
        mg.hud_dirty = 1;
    }
    if (mg.hud_dirty) {
        mg_update_hud();
        mg.hud_dirty = 0;
    }

    if (--mg.state_timer == 0) {
        for (i = 0; i < MG_ENEMIES; i++) {
            if (mg.enemies[i].body) {
                ng_chars_remove(mg.enemies[i].body);
                mg.enemies[i].body = 0;
            }
        }
        if (mg.bonus_hits >= 8 && mg.lives < MAX_LIVES) {
            mg.lives++;
            mg_hint("PERFECT! ONE LIFE RETURNED", PAL_GOLD, 120);
            playSFX(SOUND_SFX_13);
        }
        mg_interlude(mg.next_stage);
    }
}

void NEOGEO_USER maiya_frame(void)
{
    mg.tick++;
    if (!mg.player) return;


    if (mg.state == MG_INTRO) {
        /* Mission card: the world is live behind it, any button skips. */
        uint16_t joy = mg_input();
        uint16_t pressed = (uint16_t)(joy & (uint16_t)(~mg.previous_joy));
        mg.previous_joy = joy;

        mg.player->vx_fp = 0;
        mg_spawn();
        if (mg.entrance) {
            /* She is still falling in: let her land before anything else. */
            if (mg.player->y >= MG_GROUND_Y - 2) {
                mg.entrance = 0;
                playSFX(SOUND_SFX_9);
                ng_camera_shake(&mg.camera, 2, 8);
                mg_frame(mg.player, MG_F_LAND, mg.facing);
            } else {
                mg_frame(mg.player, MG_F_JUMP3, mg.facing);
            }
        } else {
            mg_animate_player();
        }
        mg_world_step();
        mg_update_entities();
        if (mg.hurt) mg.hurt--;

        /* The card only counts down once her feet are on the road. */
        if (!mg.entrance && mg.state_timer) mg.state_timer--;
        if (!mg.entrance &&
            (mg.state_timer == 0 || (pressed & (BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_D)))) {
            ng_fix_clear_rect(1, ROW_CARD, 38, 8, PAL_TEXT);
            mg.state = MG_PLAY;
            mg.state_timer = 0;
        }
        return;
    }

    if (mg.state == MG_PLAY) {
        mg_controls();
        mg_spawn();
        mg_animate_player();
        mg_world_step();
        mg_update_entities();
        mg_hazard_check();
        mg_npc_check();
        mg_rescue_check();

        if (mg.hint_timer && --mg.hint_timer == 0) {
            ng_fix_clear_rect(1, ROW_HINT, 38, 1, PAL_TEXT);
        }
        if (mg.hud_dirty) {
            mg_update_hud();
            mg.hud_dirty = 0;
        }
        if (mg.power_tag || mg.swift || mg.might || mg.veil) mg_draw_powers();
        return;
    }

    if (mg.state == MG_CLEAR) {
        /* Guardian defeated: victory pose while the land heals. */
        mg.player->vx_fp = 0;
        mg_frame(mg.player, (uint8_t)((mg.tick / 16) & 1 ? MG_F_WIN : MG_F_IDLE0), mg.facing);
        if (mg.boss) {
            mg.boss->vx_fp = 0;
            mg.boss->visible = (mg.tick & 4) ? 0 : 1;
        }
        mg_world_step();
        if (mg.hud_dirty) {
            mg_update_hud();
            mg.hud_dirty = 0;
        }
        if (--mg.state_timer == 0) {
            mg.checkpoint = 0;
            if (mg.stage + 1 < MG_LEVEL_COUNT) {
                /* Every other valley ends with a bonus round first. */
                if ((mg.stage & 1u) == 1u) {
                    mg_bonus_enter((uint8_t)(mg.stage + 1));
                } else {
                    mg_interlude((uint8_t)(mg.stage + 1));
                }
            } else {
                /* Victory Ending */
                mg.state = MG_ENDING;
                mg.state_timer = 300;
                ng_fix_clear_rect(1, ROW_CARD, 38, 12, PAL_TEXT);
                mg_centre(ROW_CARD + 2, "CONGRATULATIONS!", PAL_GOLD);
                mg_centre(ROW_CARD + 4, "EARTH IS RESTORED", PAL_SKY);
                mg_centre(ROW_CARD + 6, "MAIYA AND SUNBOY SAVED THE VALLEY", PAL_TEXT);
                mg_centre(ROW_CARD + 10, "EAGLE SOFTWARE 1996", PAL_GOLD);
            }
        }
        return;
    }

    if (mg.state == MG_DEAD) {
        /* The valley carries her up: a slow rise, arms open, into the light. */
        mg.player->visible = 1;
        if (mg.state_timer > ANGEL_TIME - 24) {
            mg_frame(mg.player, MG_F_HURT1, mg.facing);
        } else {
            mg_frame(mg.player, (uint8_t)((mg.tick / 12) & 1 ? MG_F_WIN : MG_F_JUMP3), mg.facing);
            ng_char_set_pos(mg.player, mg.player->x, (int16_t)(mg.player->y - 1));
            if ((mg.tick & 15) == 0) {
                mg_sparks(mg.player->x, (int16_t)(mg.player->y - 10));
            }
        }
        mg_world_step();
        if (--mg.state_timer == 0) {
            mg.angel = 0;
            if (mg.lives > 1) {
                mg.lives--;
                mg_scene(mg.stage, mg.checkpoint);
            } else if (mg.continues) {
                /* The valley waits: ten seconds, and one of three continues. */
                mg.state = MG_OVER;
                mg.state_timer = CONTINUE_TIME;
                ng_fix_clear_rect(1, ROW_CARD, 38, 10, PAL_TEXT);
                mg_centre(ROW_CARD + 2, "GAME OVER", PAL_WARN);
                mg_centre(ROW_CARD + 4, "CONTINUE?", PAL_GOLD);
                ng_fix_puts(9, ROW_CARD + 8, "PRESS A OR B", PAL_TEXT);
                ng_fix_puts(23, ROW_CARD + 8, "CONTINUES", PAL_SKY);
                mg_number(33, ROW_CARD + 8, mg.continues, 1, PAL_GOLD);
                playSFX(SOUND_SFX_6);
            } else {
                mg.state = MG_DONE;
                mg.state_timer = 180;
                mg.session_over = 1;
                ng_fix_clear_rect(1, ROW_CARD, 38, 10, PAL_TEXT);
                mg_centre(ROW_CARD + 4, "GAME OVER", PAL_WARN);
            }
        }
        return;
    }

    if (mg.state == MG_BONUS) {
        mg_bonus_frame();
        return;
    }

    if (mg.state == MG_INTERLUDE) {
        uint16_t joy = mg_input();
        uint16_t pressed = (uint16_t)(joy & (uint16_t)(~mg.previous_joy));
        mg.previous_joy = joy;
        if (--mg.state_timer == 0 ||
            (pressed & (BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_D))) {
            if (mg.next_stage < MG_LEVEL_COUNT) {
                mg_scene(mg.next_stage, 0);
            } else {
                mg.state = MG_ENDING;
                mg.state_timer = 360;
            }
        }
        return;
    }

    if (mg.state == MG_OVER) {
        uint16_t joy = mg_input();
        uint16_t pressed = (uint16_t)(joy & (uint16_t)(~mg.previous_joy));
        mg.previous_joy = joy;

        mg.player->vx_fp = 0;
        mg.player->visible = 0;      /* she has already gone up; the road waits */
        mg_world_step();

        /* One digit, counting down, the way the cabinet has always done it. */
        if ((mg.state_timer % 60u) == 0u) {
            mg_number(20, ROW_CARD + 6, (uint32_t)(mg.state_timer / 60u), 1, PAL_WARN);
            if (mg.state_timer) playSFX(SOUND_SFX_6);
        }

        if (!mg.demo && (pressed & (BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_D))) {
            mg.continues--;
            mg.lives = 3;
            mg.art = MAX_ART;
            mg.rescue_mask = 0;
            ng_fix_clear_rect(1, ROW_CARD, 38, 10, PAL_TEXT);
            playSFX(SOUND_SFX_13);
            mg_scene(mg.stage, mg.checkpoint);
            return;
        }

        if (mg.state_timer && --mg.state_timer == 0) {
            mg.session_over = 1;
            mg.state = MG_DONE;
            ng_fix_clear_rect(1, ROW_CARD, 38, 10, PAL_TEXT);
            mg_centre(ROW_CARD + 4, "GAME OVER", PAL_WARN);
        }
        return;
    }

    if (mg.state == MG_ENDING || mg.state == MG_DONE) {
        mg.player->vx_fp = 0;
        if (mg.state_timer && --mg.state_timer == 0) {
            mg.session_over = 1;
            mg.state = MG_DONE;
        }
    }
}
