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

#ifdef NG_AES
volatile uint8_t maiya_console_start;
#endif

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
    MG_ENEMIES = 4, MG_SHOTS = 6, MG_SPARKS = 12, MG_ITEMS = 4,
    MG_LEDGE_BLOCKS = 9, MG_HAZARD_BLOCKS = 4, MG_DECOR_SLOTS = 6,
    MG_NPC_SLOTS = 2, MG_FRONT_SLOTS = 3,

    /*
     * Sprite slots.  Higher slots draw in front, and a scanline can only
     * carry 96 strips, so every pool is sized for what one screen of road
     * can hold: scenery, ledges, dressing, then the cast on top.
     */
    SLOT_FAR = 1,        /* 32 strips, scenery far layer            */
    SLOT_ROAD = 33,      /* 32 strips, scenery road layer           */
    SLOT_LEDGE = 65,     /* 12 blocks (24 strips), one-way ledges   */
    SLOT_SHOT = 224,     /* 8 projectile sprites                    */
    SLOT_SPARK = 230,    /* 12 particles: sparks, petals, dust      */
    SLOT_ITEM = 242,     /* 4 pickups (2 strips each)               */
    SLOT_CAGE = 250,     /* 1 captive cage (2 strips)               */
    SLOT_GATE = 252,     /* the Ancient Nature Gate (2 strips)      */
    SLOT_HAZARD = 254,   /* 3 blocks (6 strips) fire/spikes/sludge  */
    SLOT_DECOR = 83,     /* 6 props behind the character pool      */
    SLOT_VINE = 264,     /* 3 climbing vines (2 strips each)        */
    SLOT_FRONT = 282,    /* 3 foreground props, in front of the cast */
    SLOT_HUD = 290,      /* face avatar, hearts, halo, key          */
    SLOT_TITLE = 310,    /* attract mode key visual                 */
    SLOT_TRAY = 330,     /* 12 shrunk pick-up icons (2 strips each) */
    MG_TRAY_SLOTS = 12,

    /*
     * Palette banks. Two hardware limits shape this list:
     *  - FIX-layer text carries only four palette bits, so every ink used
     *    on the FIX layer must live in banks 0..15. A FIX ink in bank 43
     *    silently draws with bank 11.
     *  - The stage background loads 16 banks from PAL_BG, i.e. 16..31, so
     *    nothing else may live there.
     * Sprite-only palettes therefore sit at 32 and above, keeping 0..15 free
     * for the inks the FIX layer needs.
     */
    PAL_TEXT = 0, PAL_GOLD = 1, PAL_WARN = 2, PAL_SKY = 3,
    PAL_HERO = 4, PAL_ENEMY0 = 5, PAL_ENEMY1 = 6, PAL_ENEMY2 = 7,
    PAL_ENEMY3 = 38, PAL_ENEMY4 = 39, PAL_ENEMY5 = 40,
    PAL_BOSS = 8, PAL_ALLY = 60, PAL_BLOCK = 44, PAL_EAGLE = 45,
    PAL_TOOL = 12, PAL_PORTRAIT = 41, PAL_DECOR = 46, PAL_PROP = 47,
    PAL_ITEM = 32, PAL_NPC = 33, PAL_GATE = 35, PAL_TRINKET = 36,
    PAL_FRONT = 37, PAL_BG = 16,
    /* FIX palettes for her framed HP bar, one per tier. */
    PAL_HP_HI = 13, PAL_HP_MID = 10, PAL_HP_LO = 9,
    /* Its own art, its own bank, not a recolour of one of the shared six. */
    PAL_JELLYFISH = 48, PAL_TOXICCRAB = 49, PAL_ACIDMOTH = 50, PAL_SEWERRAT = 51,
    PAL_SMOGBAT = 52, PAL_POACHDRONE = 53, PAL_CHEMFLY = 54, PAL_PLASTICBAT = 55,
    PAL_SLAGGOLEM = 56, PAL_VINESTING = 57, PAL_SPOREGOB = 58, PAL_WRAITH = 59,
    PAL_HAZARD = 61, PAL_FACE = 43, PAL_PIT = 66,
    /* FIX inks for the guardian's bar: dirty and toxic rather than the
     * clean traffic-light colours of her own -- it's the blight's health. */
    PAL_BOSS_HP_HI = 11, PAL_BOSS_HP_MID = 14, PAL_BOSS_HP_LO = 15,

    /* Character kinds. */
    K_PLAYER = 0, K_ENEMY = 1, K_BOSS = 2, K_ALLY = 3, K_EAGLE = 4,

    /* Game states. */
    MG_INTRO = 0, MG_PLAY, MG_CLEAR, MG_BONUS, MG_DEAD, MG_OVER, MG_ENDING, MG_DONE,
    MG_INTERLUDE, MG_BOSS_INTRO, MG_WARP,

    HERO_STRIPS = 5, HERO_ROWS = 4, HERO_STRIDE = 5,
    EAGLE_STRIPS = 4, EAGLE_ROWS = 3,
    BOSS_STRIPS = 8, BOSS_ROWS = 6, BOSS_STRIDE = 8,

    WALK_SPEED = 512, DASH_SPEED = 1088, JUMP_SPEED = 5 * NG_FP_ONE + 160,
    CLIMB_SPEED = 320,
    WALK_ACCEL = 112,    /* she leans into a run instead of snapping to it */
    WALK_BRAKE = 96,
    MAX_HP = 5, MAX_LIVES = 7, MAX_ART = 3,
    /* The hidden extra life is one trinket among ten, but unlike the rest
     * it hands out a life -- worth capping across the whole run, not just
     * the one mission it sits in, or a player who farms deaths-and-retries
     * on an early valley could stack lives without ever earning them. */
    MG_LIFE_PICKUP_LIMIT = 3,
    MG_DASH_TIME = 14,
    /* Every mission runs against the clock: at MG_HURRY_AT seconds left the
     * valley warns her, from MG_WRAITH_AT the smog wraiths come for her,
     * and at nought the smog takes the life. */
    MG_LEVEL_SECONDS = 240, MG_HURRY_AT = 40, MG_WRAITH_AT = 25,
    /* Where a flying guardian's feet ride: its 96-pixel body then fills the
     * upper screen instead of hanging above the top edge. */
    MG_BOSS_SKY_Y = 110,
    ROW_CLOCK = 3,
    /* What each valley throws at her besides its creatures. */
    MG_M_NONE = 0, MG_M_CRUMBLE = 3, MG_M_ICE = 4, MG_M_WATER = 5,
    MG_CRUMBLE_AFTER = 45, MG_CRUMBLE_BACK = 180,
    MG_SURGE_TIME = 30, MG_SURGE_WINDUP = 6,
    MG_BONUS_LIFE_SCORE_STEP = 50000,
    MG_BONUS_LIFE_SCORE_FIRST = 20000,
    MG_BOSS_BAR_COL = 12, MG_BOSS_BAR_LABEL_COL = 7,
    /* Framed bar tiles (see mg_draw_bar) and the two bars' sizes. */
    MG_BAR_TILE = 0x180, MG_BAR_CELL_TILE = MG_BAR_TILE + 6, MG_BAR_RCAP_TILE = MG_BAR_TILE + 15,
    MG_FRAME_TILE = MG_BAR_TILE + 21,   /* the select screen's card frame: TL T TR L R BL B BR */
    MG_HP_BAR_COL = 10, MG_HP_BAR_CELLS = 8, MG_HP_BAR_PX = 5 + MG_HP_BAR_CELLS * 8 + 5,
    MG_BOSS_BAR_CELLS = 18, MG_BOSS_BAR_PX = 5 + MG_BOSS_BAR_CELLS * 8 + 5,

    /* FIX rows: 2..29 are visible (8 px each). ROW_POWER was reserved but
     * never used until the boss HP bar took it. */
    ROW_SCORE = 2, ROW_LIVES = 4, ROW_POWER = 5, ROW_HINT = 7, ROW_CARD = 8,

    /* HUD glyphs written into the low FIX codes by build_fix_assets.py. */
    GLYPH_HEART = 1, GLYPH_ROSE = 2, GLYPH_KEY = 3, GLYPH_COIN = 4,
    GLYPH_LEAF = 5, GLYPH_SPARK = 6, GLYPH_BLOCK = 7, GLYPH_DOT = 8,
    GLYPH_FLOWER = 9, GLYPH_FRIEND = 10, GLYPH_BERRY = 11, GLYPH_ORB = 12,
    GLYPH_BUD = 13, GLYPH_CROWN = 14,
    ROW_TRAY = 26,                   /* the collection tray, bottom left  */
    MG_TRAY_ICON_SCALE = 0x60,       /* small badge icons, not a HUD bar  */
    MG_TRAY_ICON_PX = 13,            /* ~32px source shrunk by the above  */
    HURT_LOCK = 60,      /* frames of mg.hurt above this lock the controls */
    MG_KNOCK = 768,      /* her knockback: a short stagger, eased to a stop */
    MG_CAM_LEAD = 40,    /* the camera keeps this much more road ahead    */
    MG_BOSS_TOUCH = 36,  /* closer than this on the ground, a guardian hurts */
    MG_BOSS_TOUCH_AIR = 24, /* in the air only its body does: she can jump it */
    MG_BOSS_BACKOFF = 40,   /* frames a guardian gives ground after a touch */
    MG_MOOD_DYING = 0xEE, /* a beaten creature on its way off the screen   */
    SIT_DELAY = 70,      /* frames of crouching before Maiya sits down    */
    TALK_RANGE = 34,     /* how close a villager will speak up            */
    POWER_TIME = 480,    /* swiftness and might last eight seconds        */
    STOMP_KICK = 4 * NG_FP_ONE,  /* the hop she takes off a squashed slime */
    VEIL_TIME = 300,     /* the mist veil hides her for five              */
    ANGEL_TIME = 150,    /* how long she rises before the valley resets   */
    MAX_CONTINUES = 3,   /* the cabinet allows three, then the run is over */
    CONTINUE_TIME = 600  /* ten seconds on the clock to decide            */
};

typedef struct {
    NGCharacter *body;
    uint16_t timer;
    uint8_t type, hurt, posted;
    uint8_t mood;          /* 0 unaware, 1 hunting, 2+ a creature-specific move in progress */
    uint8_t move_timer;    /* frames left in the current move                              */
    int8_t  heading;       /* direction a committed move (charge, dive) is locked to        */
    int8_t  face;          /* which way it faces: only turns once she's clearly past it     */
    int16_t home;          /* where it was placed: the centre of its patrol                */
} MGEnemy;

typedef struct {
    int16_t x, y, vx, vy;
    uint8_t life, hostile, kind;
    uint8_t mode;                /* MG_SHOT_*: how it flies and what it passes through */
    NGSpriteGroup sprite;
} MGShot;

/* Her throws. Plain thorns are counted; a special weapon, while it lasts,
 * replaces them and spends its own ammunition instead. */
enum {
    MG_SHOT_PLAIN = 0, MG_SHOT_PIERCE = 1, MG_SHOT_GALE = 2, MG_SHOT_ARC = 3,
    MG_W_NONE = 0, MG_W_SPREAD = 1, MG_W_PIERCE = 2, MG_W_GALE = 3,
    MG_THORNS_MAX = 99, MG_THORNS_REFILL = 15, MG_WEAPON_AMMO = 24,
};

typedef struct {
    int16_t x, y, vx, vy;
    uint8_t life;
    uint8_t shrink;      /* dust: the life it started with -- it drifts, no
                          * pull, and shrinks away as that runs out; 0 for a
                          * full-size particle that falls */
    NGSpriteGroup sprite;
} MGSpark;

typedef struct {
    int16_t x, y;
    uint8_t life, kind, key, trinket;
    uint8_t source;                   /* 0: dropped; then pickups and secrets */
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
    NGSpriteGroup front[MG_FRONT_SLOTS];
    NGSpriteGroup gate;
    const uint16_t *block_tiles;
    NGCharacter *npcs[MG_NPC_SLOTS];
    NGSpriteGroup cage;
    NGSpriteGroup hud[16];
    NGSpriteGroup tray[MG_TRAY_SLOTS];
    MGEnemy enemies[MG_ENEMIES];
    MGShot  shots[MG_SHOTS];
    MGSpark sparks[MG_SPARKS];
    MGItem  items[MG_ITEMS];

    uint32_t score;
    uint32_t encounter_mask;
    uint16_t tick, archer_mask, pick_mask;
    uint8_t secret_mask;
    uint8_t hazard_warn_mask;          /* one bit per hazard: already warned */
    uint8_t hazard_disabled_mask;      /* one bit per hazard: shut off for good */
    uint16_t boss_timer, state_timer, clear_bonus;
    int16_t  boss_home;
    int16_t arena_left, boss_direction;
    MGPlatform arena[2];
    uint16_t walk_distance;
    uint8_t climb_cooldown;
    uint8_t  stage, state, lives, art, kills, rescue_mask;
    uint8_t  hurt, coyote, jump_buffer, drop, boss_hurt, boss_active;
    uint8_t  attack, combo, dash, dash_wait, cast, super_surge, sitting;
    uint8_t  facing, notice, hud_dirty, pause, session_over;
    uint8_t  has_key, gate_unlocked, gate_shown, key_taken;
    uint8_t  climbing, crouch_timer, npc_mask, npc_live, npc_here;
    uint16_t swift, might, veil;      /* power-ups, in frames              */
    uint16_t spring, crown;           /* higher jump, bigger thorns        */
    uint8_t  flash;                   /* frames of Secret Art palette      */
    uint8_t  angel;                   /* rising-to-the-sky death           */
    uint8_t  flowers, critters;       /* bonus tally for the mission end   */
    uint8_t  hurt_lit, shake;
    int16_t  shake_x;
    uint8_t  music_track, music_on;
    uint8_t  art_wave;                /* frames until the storm's second wave */
    uint8_t  over_pick;               /* console continue screen: 0 go on, 1 exit */
    uint8_t  airborne, land_pose;
    uint8_t  hero_choice;             /* 0 Maiya (blonde/green), 1 Luna (dark/blue) */
    uint8_t  coins;
    uint32_t score_shown;
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
    uint8_t  life_pickups_used;      /* the hidden extra life, capped for the whole run */
    uint32_t next_life_score;        /* next score milestone that hands out a bonus life */
    uint8_t  thorns;                 /* plain thorns left to throw                    */
    uint8_t  weapon, weapon_ammo;    /* MG_W_* in hand, and throws it has left        */
    uint8_t  level_falls;            /* lives lost in this mission, across retries    */
    uint8_t  attempt_hits;           /* hits taken since the mission (re)started      */
    uint8_t  surge_struck_boss;      /* the Surge strikes a guardian once per use     */
    uint8_t  boss_rage;              /* below half health: faster, one extra beat     */
    uint8_t  hp_px, boss_px;         /* fill shown in her bar and the guardian bar    */
    uint8_t  cage_open;              /* frames an opened chest stays after a rescue   */
    uint8_t  boss_down;              /* a beaten guardian has hit the ground          */
    uint8_t  arena_bg;               /* the guardian arena is the backdrop            */
    uint8_t  ledge_index;            /* which ledge she last stood on                 */
    uint8_t  ledge_stand[MG_PLATFORM_COUNT];   /* frames stood on a rotten ledge      */
    uint8_t  ledge_gone[MG_PLATFORM_COUNT];    /* frames until a crumbled one returns */
    int16_t  cage_x, cage_y;         /* where that chest stood                        */
    uint16_t clock;                  /* seconds left in this attempt                  */
    uint8_t  clock_sub;              /* frames into the current second                */
    uint8_t  wraith_timer;           /* frames until the next wraith, once they come  */
    uint8_t  wraith_side;            /* which screen edge the next one comes from     */
    uint8_t  win_step, win_wait;     /* her victory: landing, the hop, the held pose  */
    int16_t  cam_lead;               /* how far the camera looks ahead of her         */
    uint8_t  boss_backoff;           /* a guardian that just struck her steps back    */
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
/*
 * A whole-screen fade to white and back, for her walk through a guardian's
 * gate. The engine's palette effects run on a handful of banks at a time;
 * this lifts every bank the game uses together. mg_fade_begin() copies the
 * colours on screen now; while a fade is up, mg_palette() keeps whatever it
 * is asked to load in that copy and shows it at the fade's level, so a
 * scene loaded behind the white comes up with it instead of flashing
 * through. 16 is all white; 0 puts the true colours back.
 */
#define MG_FADE_BANKS 67u
static uint16_t mg_fade_src[MG_FADE_BANKS * 16u];
static uint16_t mg_fade_out[MG_FADE_BANKS * 16u];   /* what goes on screen next */
static uint8_t mg_fade_lut[32];
static uint8_t mg_fade_k;
static uint8_t mg_fade_pending;

static void NEOGEO_USER mg_lut_for(uint8_t *lut, uint8_t k)
{
    uint8_t v;
    for (v = 0; v < 32; v++) lut[v] = (uint8_t)(v + (((31u - v) * k) >> 4));
}

/* Sixteen colours through a lookup, five bits a channel (the low bit of
 * each is carried in bits 12..14 of the palette word). */
static void NEOGEO_USER mg_blend(uint16_t *out, const uint16_t *src, const uint8_t *lut)
{
    uint8_t i;
    for (i = 0; i < 16; i++) {
        uint16_t c = src[i];
        uint8_t r = lut[((c >> 7) & 0x1Eu) | ((c >> 14) & 1u)];
        uint8_t g = lut[((c >> 3) & 0x1Eu) | ((c >> 13) & 1u)];
        uint8_t b = lut[((c << 1) & 0x1Eu) | ((c >> 12) & 1u)];
        out[i] = (uint16_t)(((uint16_t)(r & 1u) << 14) | ((uint16_t)(g & 1u) << 13) |
                            ((uint16_t)(b & 1u) << 12) | ((uint16_t)(r >> 1) << 8) |
                            ((uint16_t)(g >> 1) << 4) | (uint16_t)(b >> 1));
    }
}

/* One bank's faded colours, from the copy; at level 0 the copy exactly. */
static void NEOGEO_USER mg_fade_bank(uint8_t bank)
{
    uint16_t *src = &mg_fade_src[(uint16_t)bank * 16u];
    uint16_t *out = &mg_fade_out[(uint16_t)bank * 16u];
    if (!mg_fade_k) memcpy(out, src, 32);
    else mg_blend(out, src, mg_fade_lut);
}

/*
 * Word copy for palette RAM, kept in assembly on purpose: as a C loop GCC
 * folds it into "move.w (a0)+,(0,a0,d0.l)", and a 68000 works that
 * destination out with the already incremented a0 -- every colour lands
 * one entry along and the whole screen turns to garbage. Two address
 * registers, both post-incremented, and nothing to fold. `n` is at least 1.
 */
__attribute__((noinline))
static void NEOGEO_USER mg_copy_words(volatile uint16_t *dst, const volatile uint16_t *src, uint16_t n)
{
    n = (uint16_t)(n - 1u);
    __asm__ volatile (
        "1:\n\t"
        "move.w (%0)+,(%1)+\n\t"
        "dbf %2,1b"
        : "+a" (src), "+a" (dst), "+d" (n)
        :
        : "memory");
}

static void NEOGEO_USER mg_fade_begin(void)
{
    mg_copy_words(mg_fade_src, (const volatile uint16_t *)PALETTES, MG_FADE_BANKS * 16u);
}

/*
 * Works the whole screen's colours out for level k now; they go on screen
 * at the next mg_fade_commit(), which is a straight copy fast enough to
 * finish inside the vertical blank. Written bank by bank as they were
 * worked out, the banks changed at different points down the picture and
 * the screen showed a patchwork of two brightnesses for a frame.
 */
static void NEOGEO_USER mg_fade_set(uint8_t k)
{
    uint8_t bank;
    mg_fade_k = k;
    mg_lut_for(mg_fade_lut, k);
    for (bank = 0; bank < MG_FADE_BANKS; bank++) mg_fade_bank(bank);
    mg_fade_pending = 1;
}

static void NEOGEO_USER mg_fade_commit(void)
{
    if (!mg_fade_pending) return;
    mg_fade_pending = 0;
    mg_copy_words((volatile uint16_t *)PALETTES, mg_fade_out, MG_FADE_BANKS * 16u);
}

static void NEOGEO_USER mg_palette(uint8_t bank, const uint16_t *colors)
{
    if (mg_fade_k && bank < MG_FADE_BANKS) {
        memcpy(&mg_fade_src[(uint16_t)bank * 16u], colors, 32);
        mg_fade_bank(bank);
        load_palettes(&mg_fade_out[(uint16_t)bank * 16u], PALETTES + (uint32_t)bank * 32u);
        return;
    }
    load_palettes((uint16_t *)colors, PALETTES + (uint32_t)bank * 32u);
}

/* One bank lifted toward white, for a glow on a single sprite. */
static void NEOGEO_USER mg_whiten_bank(uint8_t bank, const uint16_t *src, uint8_t k)
{
    uint8_t lut[32];
    uint16_t out[16];
    mg_lut_for(lut, k);
    mg_blend(out, src, lut);
    load_palettes(out, PALETTES + (uint32_t)bank * 32u);
}

/* An 8-bit-per-channel colour as a Neo Geo palette word (5 bits a channel,
 * the lowest bit of each carried in bits 12..14). */
#define MG_RGB(r, g, b) ((uint16_t)(((((r) >> 3) & 1) << 14) | ((((g) >> 3) & 1) << 13) | \
                                    ((((b) >> 3) & 1) << 12) | (((r) >> 4) << 8) | \
                                    (((g) >> 4) << 4) | ((b) >> 4)))

/*
 * The framed bars' palettes, pen by pen: 1 outline, 2 frame, 3 fill
 * highlight, 4 fill, 5 fill shadow, 6 empty track; the rest repeat the fill
 * so any plain glyph printed in the bank still reads in its colour. Her
 * bar sits in a gold frame; the guardian's in rusted metal, its fill the
 * dirty colours of the blight it is.
 */
#define MG_BAR_PAL(fr, fg, fb, lr, lg, lb, mr, mg_, mb, dr, dg, db) { \
    0, MG_RGB(24, 16, 8), MG_RGB(fr, fg, fb), MG_RGB(lr, lg, lb), MG_RGB(mr, mg_, mb), \
    MG_RGB(dr, dg, db), MG_RGB(40, 28, 28), MG_RGB(mr, mg_, mb), MG_RGB(mr, mg_, mb), \
    MG_RGB(mr, mg_, mb), MG_RGB(mr, mg_, mb), MG_RGB(mr, mg_, mb), MG_RGB(mr, mg_, mb), \
    MG_RGB(mr, mg_, mb), MG_RGB(mr, mg_, mb), MG_RGB(mr, mg_, mb) }
static const uint16_t mg_bar_hp_hi[16]  = MG_BAR_PAL(214, 170, 64, 176, 255, 144,  64, 200,  64,  24, 112,  40);
static const uint16_t mg_bar_hp_mid[16] = MG_BAR_PAL(214, 170, 64, 255, 232, 128, 240, 168,  32, 160,  88,  16);
static const uint16_t mg_bar_hp_lo[16]  = MG_BAR_PAL(214, 170, 64, 255, 144, 128, 224,  40,  40, 120,  16,  16);
static const uint16_t mg_bar_boss_hi[16]  = MG_BAR_PAL(128, 112, 96, 168, 184,  80, 112, 128,  40,  56,  64,  16);
static const uint16_t mg_bar_boss_mid[16] = MG_BAR_PAL(128, 112, 96, 176, 120,  64, 120,  80,  32,  64,  40,  16);
static const uint16_t mg_bar_boss_lo[16]  = MG_BAR_PAL(128, 112, 96, 144,  48,  40,  96,  24,  24,  48,   8,   8);

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
    mg_ink(PAL_TEXT,   0x7FFFu);   /* white          */
    mg_ink(PAL_GOLD,   0x6FE0u);   /* gold           */
    mg_ink(PAL_WARN,   0x4F44u);   /* red            */
    mg_ink(PAL_SKY,    0x39FFu);   /* cyan           */
    mg_palette(PAL_HP_HI, mg_bar_hp_hi);
    mg_palette(PAL_HP_MID, mg_bar_hp_mid);
    mg_palette(PAL_HP_LO, mg_bar_hp_lo);
    /* The guardian's bar reads dirty and toxic rather than her own
     * clean traffic-light colours -- it's the blight's health draining,
     * not a status she'd want for herself. */
    mg_palette(PAL_BOSS_HP_HI, mg_bar_boss_hi);
    mg_palette(PAL_BOSS_HP_MID, mg_bar_boss_mid);
    mg_palette(PAL_BOSS_HP_LO, mg_bar_boss_lo);
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
/*  Music                                                             */
/* ------------------------------------------------------------------ */

/*
 * Start an ADPCM-B track and keep it going.
 *
 * soundPlayGameLoop() resets the driver before it starts a track, and the
 * reset clears the repeat flag, so a loop requested ahead of it was thrown
 * away and every valley fell silent after one pass.  The flag is latched
 * when a track starts, so it has to be set between the reset and the start
 * -- which is exactly what this does.  Tracks are named directly: 1.wav is
 * SOUND_TRACK_A.
 */
static const uint16_t *NEOGEO_USER mg_hero_normal_pal(void)
{
    return mg.hero_choice ? mg_hero_alt_pal : mg_hero_pal;
}


static void NEOGEO_USER mg_music(uint8_t track)
{
    if (mg.music_on && mg.music_track == track) return;
    mg.music_track = track;
    mg.music_on = 1;
    isZ80Ready(); soundSceneReset();
    /* ADPCM-A level is six bits: 64 masks to 0 and mutes every effect. */
    isZ80Ready(); soundApplyMix(0x3C, 0xB8, 0x00, 0x00);
    isZ80Ready(); soundSetADPCMBLoop(1);
    isZ80Ready(); playSFXB(track);
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
    case 6:  /* Rio Negro Works */
        far_tile = MG_BG6_TILE; road_tile = MG_GROUND6_TILE;
        pal = restored ? mg_bg6_pal : mg_bg6_blight_pal;
        far_map = mg_bg6_map; road_map = mg_ground6_map; count = MG_BG6_BANKS;
        break;
    default: /* Sunlit Emerald Forest */
        far_tile = MG_BG0_TILE; road_tile = MG_GROUND0_TILE;
        pal = restored ? mg_bg0_pal : mg_bg0_blight_pal;
        far_map = mg_bg0_map; road_map = mg_ground0_map; count = MG_BG0_BANKS;
        break;
    }

    for (i = 0; i < count; i++) mg_palette((uint8_t)(PAL_BG + i), pal + i * 16u);

    /* A cleansed valley is shown in its own painted colours: the restored
     * palette is the reward. (A brightness pulse used to run on it, which
     * washed the whole sky out.) */
    (void)restored;

    ng_sprite_group_init(&mg.far, SLOT_FAR, 32, 12, far_tile, PAL_BG);
    ng_sprite_group_set_palette_map(&mg.far, far_map);
    ng_sprite_group_set_pos(&mg.far, 0, 0);
    ng_sprite_group_upload(&mg.far);

    ng_sprite_group_init(&mg.road, SLOT_ROAD, 32, 2, road_tile, PAL_BG);
    ng_sprite_group_set_palette_map(&mg.road, road_map);
    ng_sprite_group_set_pos(&mg.road, 0, MG_GROUND_Y);
    ng_sprite_group_upload(&mg.road);
}

/*
 * A guardian's own arena. When it shows itself the valley behind gives way
 * to its lair -- the logging camp, the poisoned falls, the burning grove --
 * and, the camera being locked in the arena, the scene holds still.
 */
static void NEOGEO_USER mg_arena_background(uint8_t style)
{
    uint16_t far = 0, road = 0;
    const uint16_t *pal = 0;
    const uint8_t *far_map = 0, *road_map = 0;
    uint8_t count = 0, i;
    switch (style) {
#if MG_ARENA0_READY
    case 0: far = MG_ARENA0_TILE; road = MG_ARENAGROUND0_TILE; pal = mg_arena0_pal;
             far_map = mg_arena0_map; road_map = mg_arenaground0_map; count = MG_ARENA0_BANKS; break;
#endif
#if MG_ARENA1_READY
    case 1: far = MG_ARENA1_TILE; road = MG_ARENAGROUND1_TILE; pal = mg_arena1_pal;
             far_map = mg_arena1_map; road_map = mg_arenaground1_map; count = MG_ARENA1_BANKS; break;
#endif
#if MG_ARENA2_READY
    case 2: far = MG_ARENA2_TILE; road = MG_ARENAGROUND2_TILE; pal = mg_arena2_pal;
             far_map = mg_arena2_map; road_map = mg_arenaground2_map; count = MG_ARENA2_BANKS; break;
#endif
#if MG_ARENA3_READY
    case 3: far = MG_ARENA3_TILE; road = MG_ARENAGROUND3_TILE; pal = mg_arena3_pal;
             far_map = mg_arena3_map; road_map = mg_arenaground3_map; count = MG_ARENA3_BANKS; break;
#endif
#if MG_ARENA4_READY
    case 4: far = MG_ARENA4_TILE; road = MG_ARENAGROUND4_TILE; pal = mg_arena4_pal;
             far_map = mg_arena4_map; road_map = mg_arenaground4_map; count = MG_ARENA4_BANKS; break;
#endif
#if MG_ARENA5_READY
    case 5: far = MG_ARENA5_TILE; road = MG_ARENAGROUND5_TILE; pal = mg_arena5_pal;
             far_map = mg_arena5_map; road_map = mg_arenaground5_map; count = MG_ARENA5_BANKS; break;
#endif
#if MG_ARENA6_READY
    case 6: far = MG_ARENA6_TILE; road = MG_ARENAGROUND6_TILE; pal = mg_arena6_pal;
             far_map = mg_arena6_map; road_map = mg_arenaground6_map; count = MG_ARENA6_BANKS; break;
#endif
#if MG_ARENA7_READY
    case 7: far = MG_ARENA7_TILE; road = MG_ARENAGROUND7_TILE; pal = mg_arena7_pal;
             far_map = mg_arena7_map; road_map = mg_arenaground7_map; count = MG_ARENA7_BANKS; break;
#endif
#if MG_ARENA8_READY
    case 8: far = MG_ARENA8_TILE; road = MG_ARENAGROUND8_TILE; pal = mg_arena8_pal;
             far_map = mg_arena8_map; road_map = mg_arenaground8_map; count = MG_ARENA8_BANKS; break;
#endif
#if MG_ARENA9_READY
    case 9: far = MG_ARENA9_TILE; road = MG_ARENAGROUND9_TILE; pal = mg_arena9_pal;
             far_map = mg_arena9_map; road_map = mg_arenaground9_map; count = MG_ARENA9_BANKS; break;
#endif
    default: return;
    }
    for (i = 0; i < count; i++) mg_palette((uint8_t)(PAL_BG + i), pal + i * 16u);
    ng_sprite_group_init(&mg.far, SLOT_FAR, 32, 12, far, PAL_BG);
    ng_sprite_group_set_palette_map(&mg.far, far_map);
    ng_sprite_group_set_pos(&mg.far, 0, 0);
    ng_sprite_group_upload(&mg.far);
    ng_sprite_group_init(&mg.road, SLOT_ROAD, 32, 2, road, PAL_BG);
    ng_sprite_group_set_palette_map(&mg.road, road_map);
    ng_sprite_group_set_pos(&mg.road, 0, MG_GROUND_Y);
    ng_sprite_group_upload(&mg.road);
    mg.arena_bg = 1;
}

/* Waterfall shimmer.  The "fall" decor tile paints its curtain in exactly
 * two dedicated palette slots (water, water-light -- NATURE indices 14/15),
 * which nothing else in the shared decor palette touches, so swapping just
 * those two every few frames reads as flowing water without recolouring
 * any other scenery sharing PAL_DECOR. Cheap enough to leave running every
 * scene; it is invisible wherever no falls decor is actually placed. */
static void NEOGEO_USER mg_animate_water(void)
{
    static uint8_t phase = 0;
    if ((mg.tick & 7u) == 0u) {
        uint16_t buf[16];
        uint8_t i;
        phase ^= 1;
        for (i = 0; i < 16; i++) buf[i] = mg_decor_pal[i];
        if (phase) {
            buf[14] = mg_decor_pal[15];
            buf[15] = mg_decor_pal[14];
        }
        mg_palette(PAL_DECOR, buf);
    }
}

static void NEOGEO_USER mg_scroll_scenery(int16_t camera_x)
{
    if (mg.arena_bg) {       /* the arena is a fixed stage */
        ng_sprite_group_set_pos(&mg.far, 0, 0);
        ng_sprite_group_flush(&mg.far);
        ng_sprite_group_set_pos(&mg.road, 0, MG_GROUND_Y);
        ng_sprite_group_flush(&mg.road);
        return;
    }
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
    case 6: *pal = mg_block_rust_pal;   return mg_block_rust_tiles;
    default: *pal = mg_block_grass_pal; return mg_block_grass_tiles;
    }
}

/* Each hazard has its own painting and two frames: the fire flickers, the
 * sludge bubbles, the leaking drum breathes gas, the spikes glint. The
 * toxic drum is what can be shut off for good, so it looks the part. */
static uint16_t NEOGEO_USER mg_hazard_tile(uint8_t type)
{
    uint8_t frame = (uint8_t)((mg.tick >> 3) & 1);
    if (type == MG_H_FIRE) return mg_hazard_tiles[MG_HZ_FIRE0 + frame];
    if (type == MG_H_SLUDGE) return mg_hazard_tiles[MG_HZ_SLUDGE0 + ((mg.tick >> 4) & 1)];
    if (type == MG_H_TOXIC) return mg_hazard_tiles[MG_HZ_TOXIC0 + ((mg.tick >> 4) & 1)];
    return mg_hazard_tiles[MG_HZ_SPIKES0 + (((mg.tick >> 5) & 3) == 0)];
}

/* One obstacle per valley, in mission order: autumn's rotten ledges, the
 * grotto's ice, the world tree's crumbling bark, the reef underwater and
 * the cave's ice again. The falls, the coast and the savanna have none of
 * their own -- the wind and tide that used to shove her there are gone. */
static const uint8_t mg_stage_mech[MG_LEVEL_COUNT] = {
    MG_M_NONE, MG_M_NONE, MG_M_NONE, MG_M_CRUMBLE, MG_M_ICE,
    MG_M_CRUMBLE, MG_M_NONE, MG_M_WATER, MG_M_ICE, MG_M_NONE,
};

/* What lies at the bottom of each valley's pits. */
static const uint16_t *NEOGEO_USER mg_pit_art(const uint16_t **pal)
{
    switch (mg.stage) {
    case 3: case 9: *pal = mg_pit_fire_pal;  return mg_pit_fire_tiles;
    case 5: case 6: *pal = mg_pit_toxic_pal; return mg_pit_toxic_tiles;
    case 4: case 7: case 8: *pal = mg_pit_void_pal; return mg_pit_void_tiles;
    default:        *pal = mg_pit_water_pal; return mg_pit_water_tiles;
    }
}

/* Is this x over a pit? `margin` widens the test for creatures that should
 * stop short of the edge. */
static uint8_t NEOGEO_USER mg_over_pit(int16_t x, int16_t margin)
{
    const MGLevel *level = &mg_levels[mg.stage];
    uint8_t i;
    for (i = 0; i < MG_HAZARD_COUNT; i++) {
        const MGHazard *hz = &level->hazards[i];
        if (hz->type == MG_H_PIT && x > hz->x - margin && x < (int16_t)(hz->x + hz->width + margin))
            return 1;
    }
    return 0;
}

static uint8_t NEOGEO_USER mg_mech(void)
{
    return mg.stage < MG_LEVEL_COUNT ? mg_stage_mech[mg.stage] : MG_M_NONE;
}

/* Underwater she falls slowly and floats through her jumps. */
static void NEOGEO_USER mg_player_gravity(void)
{
    if (mg_mech() == MG_M_WATER) ng_physics_set_gravity(mg.player, 34, 3 * NG_FP_ONE);
    else ng_physics_set_gravity(mg.player, 64, 6 * NG_FP_ONE);
}

static const MGPlatform *NEOGEO_USER mg_platform(uint8_t index)
{
    if (mg.state == MG_BONUS) return 0;
    if (mg.boss_active) return index < 2 ? &mg.arena[index] : 0;
    if (index < MG_PLATFORM_COUNT && mg.ledge_gone[index]) return 0;   /* crumbled away */
    return &mg_levels[mg.stage].platforms[index];
}

/*
 * One-way ledges are drawn as rows of 32x32 blocks (left / mid / right
 * piece) from a small pool; blocks outside the screen are released.
 */
static void NEOGEO_USER mg_draw_ledges(int16_t camera_x)
{
    uint8_t i, k, used = 0;

    for (i = 0; i < MG_PLATFORM_COUNT; i++) {
        const MGPlatform *pl = mg_platform(i);
        uint8_t blocks;
        int16_t scr;

        if (!pl || !pl->width) continue;
        scr = (int16_t)(pl->x - camera_x);
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

        if (!hz->type || (mg.hazard_disabled_mask & (uint8_t)(1u << i))) continue;
        if (scr > 336 || (int16_t)(scr + hz->width) < -16) continue;

        blocks = (uint8_t)((hz->width + 31) / 32);
        for (k = 0; k < blocks && used < MG_HAZARD_BLOCKS; k++) {
            int16_t bx = (int16_t)(scr + k * 32);
            NGSpriteGroup *g = &mg.hazards[used];

            if (bx > 336 || bx < -32) continue;
            if (hz->type == MG_H_PIT) {
                /* Blocks per width: 32 -> S; 48 -> L48 R48; 64 -> L64 R64. */
                const uint16_t *pal;
                const uint16_t *art = mg_pit_art(&pal);
                uint8_t piece = (uint8_t)(hz->width <= 32 ? 0 : (hz->width <= 48 ? 1 + k : 3 + k));
                ng_sprite_group_set_tile_base(g, art[((mg.tick >> 4) & 1) * 5 + piece]);
                ng_sprite_group_set_palette(g, PAL_PIT);
            } else {
                ng_sprite_group_set_tile_base(g, mg_hazard_tile(hz->type));
                ng_sprite_group_set_palette(g, PAL_HAZARD);
            }
            /* A pit is cut into the road itself; everything else stands on it. */
            ng_sprite_group_set_pos(g, bx, (int16_t)(hz->type == MG_H_PIT ? MG_GROUND_Y : MG_GROUND_Y - 32));
            ng_sprite_group_set_visible(g, 1);
            ng_sprite_group_flush(g);
            used++;
        }
        /* A pit reads as a texture change more than a hole at a glance, so
         * a caution sign stands planted at its near edge -- a board and a
         * post, not another line of HUD text. */
        if (hz->type == MG_H_PIT && used < MG_HAZARD_BLOCKS) {
            int16_t sx = (int16_t)(scr - 28);
            if (sx >= -32 && sx <= 336) {
                NGSpriteGroup *g = &mg.hazards[used];
                ng_sprite_group_set_tile_base(g, mg_hazard_tiles[MG_HZ_SIGN0 + ((mg.tick >> 4) & 1)]);
                ng_sprite_group_set_palette(g, PAL_HAZARD);
                ng_sprite_group_set_pos(g, sx, (int16_t)(MG_GROUND_Y - 32));
                ng_sprite_group_set_visible(g, 1);
                ng_sprite_group_flush(g);
                used++;
            }
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

/*
 * The front plane.  Boulders and fern fronds pass between the player and
 * the road at a quarter again the camera's speed: the valley gains a near
 * edge, and the ledges stop looking pasted onto the painting.
 */
static void NEOGEO_USER mg_draw_front(int16_t camera_x)
{
    int16_t plane = (int16_t)(camera_x + camera_x / 4);
    uint8_t i;

    for (i = 0; i < MG_FRONT_SLOTS; i++) {
        /* Wrap only in the off-screen gap, never through the playfield. */
        int16_t scr = (int16_t)(((i * 140 + 420 - (plane % 420) + 40) % 420) - 40);

        if (scr < -40 || scr > 340) {
            ng_sprite_group_set_visible(&mg.front[i], 0);
        } else {
            ng_sprite_group_set_pos(&mg.front[i], scr, (int16_t)(MG_GROUND_Y - 16));
            ng_sprite_group_set_visible(&mg.front[i], 1);
        }
        ng_sprite_group_flush(&mg.front[i]);
    }
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
    mg.climb_cooldown = 12;
    mg_player_gravity();
}

/* The chest a captive is held in: shut while she waits, then thrown open
 * where it stood for a moment after she's freed, instead of vanishing. */
static void NEOGEO_USER mg_draw_cage(int16_t camera_x)
{
    if (mg.rescue) {
        ng_sprite_group_set_tile_base(&mg.cage, mg_prop_tiles[MG_P_CHEST]);
        ng_sprite_group_set_pos(&mg.cage, (int16_t)(mg.rescue->x - 16 - camera_x),
                                (int16_t)(mg.rescue->y - 32));
        ng_sprite_group_set_visible(&mg.cage, 1);
    } else if (mg.cage_open) {
        /* Stays open and fully visible for its whole run, then simply
         * clears -- it used to blink for its last 12 frames (toggling on
         * bit 2 of the countdown), which read as broken, not as fading. */
        mg.cage_open--;
        ng_sprite_group_set_tile_base(&mg.cage, mg_prop_tiles[MG_P_CHEST_OPEN]);
        ng_sprite_group_set_pos(&mg.cage, (int16_t)(mg.cage_x - camera_x), mg.cage_y);
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
            if (p->shrink) {
                /* dust slows as it spreads, and never falls */
                if ((p->life & 7) == 0) {
                    p->vx = (int16_t)(p->vx / 2);
                    p->vy = (int16_t)(p->vy / 2);
                }
            } else if ((p->life & 3) == 0) {
                p->vy++;
            }
        }
        scr_x = (int16_t)(p->x - camera_x);
        if (!p->life || scr_x < -16 || scr_x > 336) {
            p->life = 0;
            p->shrink = 0;
            ng_sprite_group_set_scale(&p->sprite, NG_SPRITE_FULL_XSCALE, NG_SPRITE_FULL_YSCALE);
            ng_sprite_group_set_visible(&p->sprite, 0);
        } else if (p->shrink) {
            /* Shrunk toward its own middle, not its corner. */
            uint8_t sc = (uint8_t)(48u + ((uint16_t)p->life * 207u) / p->shrink);
            int16_t in = (int16_t)(8 - (sc >> 5));
            ng_sprite_group_set_scale(&p->sprite, sc, sc);
            ng_sprite_group_set_pos(&p->sprite, (int16_t)(scr_x + in), (int16_t)(p->y + in));
            ng_sprite_group_set_visible(&p->sprite, 1);
        } else {
            ng_sprite_group_set_scale(&p->sprite, NG_SPRITE_FULL_XSCALE, NG_SPRITE_FULL_YSCALE);
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

    if (mg.boss_active || mg.state == MG_BONUS) {
        int16_t left = mg.boss_active ? mg.arena_left : 0;
        if (p->x < left + 20) { ng_char_set_pos(p, left + 20, p->y); p->vx_fp = 0; }
        if (p->x > left + 300) { ng_char_set_pos(p, left + 300, p->y); p->vx_fp = 0; }
        if (mg.boss) {
            NGCharacter *b = mg.boss;
            if (b->x < left + 52) { ng_char_set_pos(b, left + 52, b->y); b->vx_fp = 0; }
            if (b->x > left + 268) { ng_char_set_pos(b, left + 268, b->y); b->vx_fp = 0; }
        }
    }

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
        const MGPlatform *pl = mg_platform(i);
        if (!pl || !pl->width) continue;
        if (p->x < pl->x || p->x > (int16_t)(pl->x + pl->width)) continue;
        /* Feet crossed the ledge top this frame (or rest on it). */
        if (mg.player_prev_y <= pl->y && p->y >= pl->y) {
            ng_char_set_pos(p, p->x, pl->y);
            p->vy_fp = 0;
            physics_body(p)->grounded = 1;
            mg.on_ledge = 1;
            mg.ledge_index = i;
            return;
        }
    }
}

/*
 * Engine hook, runs right before the characters are drawn: follow the
 * heroine with the camera and move every world-space sprite with it.
 */
/*
 * The camera.
 *
 * ng_camera_update() multiplies the error by follow_speed in 16.16, which
 * overflows a 32-bit int once the camera is more than about 128 px from its
 * target: at a tight follow speed one knock-back sent it the wrong way and
 * left Maiya parked off the left edge of the screen.  A side-scroller only
 * needs her kept a little left of centre, so do it here, in whole pixels,
 * with a step small enough to stay smooth and a shake of our own.
 */
static void NEOGEO_USER mg_camera_follow(void)
{
    const MGLevel *level = &mg_levels[mg.stage];
    int16_t want;
    int16_t max = (int16_t)(level->width - NG_SCREEN_W);
    int16_t have = (int16_t)(mg.camera.x - mg.shake_x);
    int16_t step;

    /*
     * Look-ahead: more of the road ahead of her than behind, on whichever
     * side she is walking toward. When she turns, the lead swings across
     * one pixel a frame, so the view pans over instead of jumping. Whole
     * pixels only -- a fractional follow let the valley judder by one
     * against her sprite. A stagger doesn't swing it.
     */
    if (mg.hurt <= HURT_LOCK) {
        int16_t aim = mg.cam_lead;
        if (mg.player->vx_fp > 128) aim = MG_CAM_LEAD;
        else if (mg.player->vx_fp < -128) aim = -MG_CAM_LEAD;
        if (mg.cam_lead < aim) mg.cam_lead++;
        else if (mg.cam_lead > aim) mg.cam_lead--;
    }
    want = (int16_t)(mg.player->x - NG_SCREEN_W / 2 + mg.cam_lead);

    if (mg.boss_active) want = mg.arena_left;
    if (mg.state == MG_BONUS) want = 0;

    if (want < 0) want = 0;
    if (want > max) want = max;

    step = (int16_t)(want - have);
    if (step > 8) step = 8;
    else if (step < -8) step = -8;
    have = (int16_t)(have + step);

    if (mg.shake) {
        mg.shake--;
        mg.shake_x = (int16_t)((mg.shake & 2u) ? 2 : -2);
    } else {
        mg.shake_x = 0;
    }

    ng_camera_snap(&mg.camera, (int16_t)(have + mg.shake_x), 0);
}

static void NEOGEO_USER mg_before_draw_hook(void)
{
    if (mg.player) mg_camera_follow();
    ng_level_set_scroll(mg.camera.x, 0);
    mg_scroll_scenery(mg.camera.x);
    if (mg.state == MG_BONUS) {
        mg_update_sparks(mg.camera.x);
        return;
    }
    mg_draw_ledges(mg.camera.x);
    mg_draw_hazards(mg.camera.x);
    mg_draw_vines(mg.camera.x);
    mg_draw_decor(mg.camera.x);
    mg_draw_front(mg.camera.x);
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
    case MG_B_VULTURE:   return mg_boss_vulture_tiles;
    case MG_B_EEL:       return mg_boss_eel_tiles;
    case MG_B_WYRM:      return mg_boss_wyrm_tiles;
    case MG_B_HYENA:     return mg_boss_hyena_tiles;
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
    case MG_B_VULTURE:   return mg_boss_vulture_pal;
    case MG_B_EEL:       return mg_boss_eel_pal;
    case MG_B_WYRM:      return mg_boss_wyrm_pal;
    case MG_B_HYENA:     return mg_boss_hyena_pal;
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
    case MG_E_JELLYFISH: return mg_jellyfish_tiles;
    case MG_E_TOXICCRAB: return mg_toxiccrab_tiles;
    case MG_E_ACIDMOTH: return mg_acidmoth_tiles;
    case MG_E_SEWERRAT: return mg_sewerrat_tiles;
    case MG_E_SMOGBAT: return mg_smogbat_tiles;
    case MG_E_POACHDRONE: return mg_poachdrone_tiles;
    case MG_E_CHEMFLY: return mg_chemfly_tiles;
    case MG_E_PLASTICBAT: return mg_plasticbat_tiles;
    case MG_E_SLAGGOLEM: return mg_slaggolem_tiles;
    case MG_E_VINESTING: return mg_vinesting_tiles;
    case MG_E_SPOREGOB: return mg_sporegob_tiles;
    case MG_E_WRAITH: return mg_acidmoth_tiles;
    default:          return mg_slime_tiles;
    }
}

/* How many poses each creature's sheet actually has. Animating past the
 * end drew whatever tiles happened to follow in the ROM. */
static uint8_t NEOGEO_USER mg_enemy_frames(uint8_t type)
{
    switch (type) {
    case MG_E_SLIME:  return MG_SLIME_FRAMES;
    case MG_E_BEETLE: return MG_BEETLE_FRAMES;
    case MG_E_CROW:   return MG_CROW_FRAMES;
    case MG_E_GOBLIN: return MG_GOBLIN_FRAMES;
    case MG_E_WORM:   return MG_WORM_FRAMES;
    case MG_E_DRONE:  return MG_ROBOT_FRAMES;
    default:          return 2;   /* every newer creature, and the wraith, has two */
    }
}

/*
 * Every creature has its own palette bank.  Sharing one left the goblins,
 * worms and drones wearing the slime's colours, which is why half the
 * roster looked wrong.
 */
static uint8_t NEOGEO_USER mg_enemy_palette(uint8_t type)
{
    switch (type) {
    case MG_E_BEETLE: return PAL_ENEMY1;
    case MG_E_CROW:   return PAL_ENEMY2;
    case MG_E_GOBLIN: return PAL_ENEMY3;
    case MG_E_WORM:   return PAL_ENEMY4;
    case MG_E_DRONE:  return PAL_ENEMY5;
    case MG_E_JELLYFISH: return PAL_JELLYFISH;
    case MG_E_TOXICCRAB: return PAL_TOXICCRAB;
    case MG_E_ACIDMOTH: return PAL_ACIDMOTH;
    case MG_E_SEWERRAT: return PAL_SEWERRAT;
    case MG_E_SMOGBAT: return PAL_SMOGBAT;
    case MG_E_POACHDRONE: return PAL_POACHDRONE;
    case MG_E_CHEMFLY: return PAL_CHEMFLY;
    case MG_E_PLASTICBAT: return PAL_PLASTICBAT;
    case MG_E_SLAGGOLEM: return PAL_SLAGGOLEM;
    case MG_E_VINESTING: return PAL_VINESTING;
    case MG_E_SPOREGOB: return PAL_SPOREGOB;
    case MG_E_WRAITH: return PAL_WRAITH;
    default:          return PAL_ENEMY0;
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
        tile = bt[frame % MG_BOSS_FRAMES];
    } else if (c->kind == K_ENEMY) {
        const uint16_t *et = mg_enemy_tiles(c->data0);
        tile = et[frame % mg_enemy_frames(c->data0)];
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
        c->sprite_offset_x = -32;
        c->sprite_offset_y = -44;
        ng_char_set_body(c, -18, -32, 36, 32);
    } else if (kind == K_BOSS) {
        const uint16_t *bt = mg_boss_tiles(subtype);
        ng_char_set_sprite(c, NG_SPR_CHAR_FIRST, BOSS_STRIPS, BOSS_ROWS, bt[0], palette);
        ng_char_set_tile_stride(c, BOSS_STRIDE);
        c->sprite_offset_x = -64;
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
static void NEOGEO_USER mg_sparks(int16_t x, int16_t y);

/* A curtain of rose petals across the playfield: the Secret Art you see. */
/*
 * The purifying storm.  Twelve pieces of the valley -- petals, leaves, seeds
 * and drops of clean water -- burst out of her and cross the whole screen
 * in both directions, so everything standing on it is touched by the art.
 * Two waves: the first radiates from her, the second rains from the sky.
 */
static void NEOGEO_USER mg_petal_sweep(void)
{
    static const uint8_t tiles[4] = { MG_T_PETAL, MG_T_LEAF, MG_T_DRIP, MG_T_SPARK };
    uint8_t i;

    for (i = 0; i < MG_SPARKS; i++) {
        MGSpark *p = &mg.sparks[i];
        /* out of her hands, fanning across the road */
        p->x = mg.player->x;
        p->y = (int16_t)(mg.player->y - 30);
        p->vx = (int16_t)((i & 1) ? (2 + (i >> 1)) : -(2 + (i >> 1)));
        p->vy = (int16_t)(-1 - (i % 3));
        p->life = (uint8_t)(50 + (i & 3) * 4);
        ng_sprite_group_set_tile_base(&p->sprite, (uint16_t)(MG_TOOL_TILE + tiles[i & 3]));
        ng_sprite_group_set_palette(&p->sprite, PAL_TOOL);
        p->shrink = 0;
    }
}

static void NEOGEO_USER mg_sparks(int16_t x, int16_t y)
{
    uint8_t i;
    for (i = 0; i < MG_SPARKS; i++) {
        MGSpark *p = &mg.sparks[i];
        ng_sprite_group_set_tile_base(&p->sprite, MG_TOOL_TILE + MG_T_SPARK);
        p->shrink = 0;
        p->x = x; p->y = y;
        p->vx = (int16_t)((i % 3) - 1);
        p->vy = -(int16_t)(1 + i / 2);
        p->life = (uint8_t)(12 + i * 2);
    }
}

/*
 * A small burst -- road dust under her feet, stars around her when a
 * villager speaks -- that takes only the particles nobody is using.
 */
static void NEOGEO_USER mg_burst(int16_t x, int16_t y, uint8_t tile, uint8_t count, int8_t rise)
{
    uint8_t i, k = 0;
    for (i = 0; i < MG_SPARKS && k < count; i++) {
        MGSpark *p = &mg.sparks[i];
        if (p->life) continue;
        ng_sprite_group_set_tile_base(&p->sprite, (uint16_t)(MG_TOOL_TILE + tile));
        /* Every ordinary burst is her tools' own colours -- explicit, so a
         * particle slot the Secret Art borrowed for its gold ring doesn't
         * carry that tint into the next dash puff or hit spark. */
        ng_sprite_group_set_palette(&p->sprite, PAL_TOOL);
        p->shrink = 0;
        p->x = (int16_t)(x + (int16_t)(k * 9) - (int16_t)(count * 4));
        p->y = y;
        p->vx = (int16_t)((k & 1) ? 1 : -1);
        p->vy = (int16_t)(rise - (k & 1));
        p->life = (uint8_t)(14 + k * 3);
        k++;
    }
}

/*
 * The Secret Art's own signature: a ring of golden halos expanding out
 * from her, distinct from the petal/leaf/dust particles every other
 * effect in the game reuses. Same particle pool, borrowed palette.
 */
static void NEOGEO_USER mg_secret_art_ring(int16_t x, int16_t y)
{
    static const int8_t vx8[8] = { 0, 5, 7, 5, 0, -5, -7, -5 };
    static const int8_t vy8[8] = { -7, -5, 0, 5, 7, 5, 0, -5 };
    uint8_t i, k = 0;
    for (i = 0; i < MG_SPARKS && k < 8; i++) {
        MGSpark *p = &mg.sparks[i];
        if (p->life) continue;
        ng_sprite_group_set_tile_base(&p->sprite, (uint16_t)(MG_TOOL_TILE + MG_T_HALO));
        ng_sprite_group_set_palette(&p->sprite, PAL_GOLD);
        p->shrink = 0;
        p->x = x; p->y = y;
        p->vx = vx8[k]; p->vy = vy8[k];
        p->life = 26;
        k++;
    }
}

/*
 * A strike landing: a few petals and leaves knocked loose, radiating from
 * the point of contact and gone in a quarter second. It only takes free
 * particles -- the full twelve-star shower used to fire on every single
 * whip crack, speckling the screen white and cutting off whatever dust
 * or petals were already in the air.
 */
static void NEOGEO_USER mg_hit_burst(int16_t x, int16_t y)
{
    static const int8_t vx[5] = { -3, 3, -2, 2, 0 };
    static const int8_t vy[5] = { -2, -2, -4, -4, -5 };
    uint8_t i, k = 0;
    for (i = 0; i < MG_SPARKS && k < 5; i++) {
        MGSpark *p = &mg.sparks[i];
        if (p->life) continue;
        ng_sprite_group_set_tile_base(&p->sprite,
                                      (uint16_t)(MG_TOOL_TILE + ((k & 1) ? MG_T_LEAF : MG_T_PETAL)));
        p->shrink = 0;
        p->x = x; p->y = y;
        p->vx = vx[k]; p->vy = vy[k];
        p->life = (uint8_t)(10 + k * 2);
        k++;
    }
}

/*
 * A creature of the blight coming apart: a puff of dust that blows out in
 * a ring, each puff slowing, drifting and shrinking away to nothing, with
 * a flash at the heart of it. The ring's turn, the puffs' speeds and
 * lives are drawn fresh each time, and now and then a petal floats up out
 * of it -- seen a hundred times a run, no two alike.
 */
static void NEOGEO_USER mg_dust_burst(int16_t x, int16_t y)
{
    static const int8_t rx[8] = { 3, 2, 0, -2, -3, -2, 0, 2 };
    static const int8_t ry[8] = { 0, -2, -3, -2, 0, 1, 2, 1 };
    uint8_t i, k = 0, turn = (uint8_t)(mg_rand() & 7u);
    uint8_t want = (uint8_t)(6u + (mg_rand() & 1u));
    uint8_t petal = (uint8_t)((mg_rand() & 3u) == 0u);
    for (i = 0; i < MG_SPARKS && k < want + 1u + petal; i++) {
        MGSpark *p = &mg.sparks[i];
        if (p->life) continue;
        ng_sprite_group_set_palette(&p->sprite, PAL_TOOL);
        if (k == 0) {
            /* the flash at the heart, gone almost at once */
            ng_sprite_group_set_tile_base(&p->sprite, (uint16_t)(MG_TOOL_TILE + MG_T_SPARK));
            p->x = (int16_t)(x - 8); p->y = (int16_t)(y - 8);
            p->vx = 0; p->vy = 0;
            p->life = 6;
            p->shrink = 6;
        } else if (k > want) {
            /* the petal the valley gets back */
            ng_sprite_group_set_tile_base(&p->sprite, (uint16_t)(MG_TOOL_TILE + ((mg_rand() & 1u) ? MG_T_PETAL : MG_T_LEAF)));
            p->x = (int16_t)(x - 8); p->y = (int16_t)(y - 12);
            p->vx = (int16_t)((mg_rand() & 1u) ? 1 : -1); p->vy = -2;
            p->life = 34;
            p->shrink = 0;
        } else {
            uint8_t d = (uint8_t)((k - 1u + turn) & 7u);
            uint8_t fast = (uint8_t)(mg_rand() & 1u);
            ng_sprite_group_set_tile_base(&p->sprite, (uint16_t)(MG_TOOL_TILE + MG_T_DUST));
            p->x = (int16_t)(x - 8 + rx[d] * 2); p->y = (int16_t)(y - 8 + ry[d] * 2);
            p->vx = (int16_t)(rx[d] + (fast ? rx[d] / 2 : 0));
            p->vy = (int16_t)(ry[d] - 1);
            p->life = (uint8_t)(16u + (mg_rand() % 12u));
            p->shrink = p->life;
        }
        k++;
    }
}

/* Beetles, crabs, moths and flies burst into dust when they're beaten:
 * turned over on its back and falling, a bug read as a dead insect. */
static uint8_t NEOGEO_USER mg_enemy_is_bug(uint8_t type)
{
    return (uint8_t)(type == MG_E_BEETLE || type == MG_E_TOXICCRAB ||
                     type == MG_E_ACIDMOTH || type == MG_E_CHEMFLY);
}

/* The shot takes its own tile: spit looks like spit and fire like fire,
 * not every projectile borrowing the thorn it was first set up with. */
static MGShot *NEOGEO_USER mg_fire(int16_t x, int16_t y, int16_t vx, int16_t vy, uint8_t hostile, uint8_t kind)
{
    uint8_t i;
    for (i = 0; i < MG_SHOTS; i++) {
        MGShot *p = &mg.shots[i];
        if (p->life) continue;
        p->x = x; p->y = y; p->vx = vx; p->vy = vy;
        p->life = 90; p->hostile = hostile; p->kind = kind; p->mode = MG_SHOT_PLAIN;
        ng_sprite_group_set_tile_base(&p->sprite, (uint16_t)(MG_TOOL_TILE + kind));
        return p;
    }
    return 0;
}

static uint8_t NEOGEO_USER mg_shots_in_flight(void)
{
    uint8_t i, n = 0;
    for (i = 0; i < MG_SHOTS; i++) if (mg.shots[i].life && !mg.shots[i].hostile) n++;
    return n;
}

static MGItem *NEOGEO_USER mg_drop(int16_t x, int16_t y, uint8_t kind)
{
    uint8_t i;
    for (i = 0; i < MG_ITEMS; i++) {
        MGItem *p = &mg.items[i];
        if (p->life) continue;
        p->x = x; p->y = y; p->kind = kind; p->life = 255; p->key = 0; p->trinket = 0;
        p->source = 0;
        ng_sprite_group_set_tile_base(&p->sprite, mg_item_tiles[kind]);
        ng_sprite_group_set_palette(&p->sprite, PAL_ITEM);
        return p;
    }
    return 0;
}

/* Coins, flowers, forest friends, extra lives and the three power-ups. */
static MGItem *NEOGEO_USER mg_drop_trinket(int16_t x, int16_t y, uint8_t kind)
{
    uint8_t i;
    for (i = 0; i < MG_ITEMS; i++) {
        MGItem *p = &mg.items[i];
        if (p->life) continue;
        p->x = x; p->y = y; p->kind = kind; p->life = 255; p->key = 0; p->trinket = 1;
        p->source = 0;
        ng_sprite_group_set_tile_base(&p->sprite, mg_trinket_tiles[kind]);
        ng_sprite_group_set_palette(&p->sprite, PAL_TRINKET);
        return p;
    }
    return 0;
}

/* The Golden Sun Key: the one pick-up a mission cannot be finished without. */
static void NEOGEO_USER mg_drop_key(int16_t x, int16_t y)
{
    uint8_t i;
    for (i = 0; i < MG_ITEMS; i++) {
        MGItem *p = &mg.items[i];
        if (p->life) continue;
        p->x = x; p->y = y; p->kind = MG_I_GEM; p->life = 255; p->key = 1; p->trinket = 0;
        p->source = 0;
        ng_sprite_group_set_tile_base(&p->sprite, mg_item_tiles[MG_I_GEM]);
        ng_sprite_group_set_palette(&p->sprite, PAL_ITEM);
        return;
    }
}

static uint8_t NEOGEO_USER mg_pickup_active(uint8_t source)
{
    uint8_t i;
    for (i = 0; i < MG_ITEMS; i++) {
        if (mg.items[i].life && mg.items[i].source == source) return 1;
    }
    return 0;
}

static void NEOGEO_USER mg_hud_static(void);
static void NEOGEO_USER mg_draw_lives(void);
static void NEOGEO_USER mg_draw_tray(void);
static void NEOGEO_USER mg_update_hud(void);
static void NEOGEO_USER mg_draw_hp_bar(void);
static void NEOGEO_USER mg_draw_clock(void);
static void NEOGEO_USER mg_arena_setup(uint8_t style);

/* ------------------------------------------------------------------ */
/*  Combat, Damage & Secret Arts                                      */
/* ------------------------------------------------------------------ */
/*
 * Playing well: no life lost in this mission yet, and only a scratch or
 * two taken since it (re)started. The special weapons and the bloom buds
 * are earned this way -- a run of deaths and retries turns the valley
 * stingy, handing out plain thorns and not much else.
 */
static uint8_t NEOGEO_USER mg_playing_well(void)
{
    return (uint8_t)(mg.level_falls == 0 && mg.attempt_hits <= 3);
}

static void NEOGEO_USER mg_enemy_damage(MGEnemy *e, uint8_t damage)
{
    if (!e->body || e->hurt || e->mood == MG_MOOD_DYING) return;
    e->hurt = 14;
    playSFX(SOUND_SFX_3); /* squish / hit */

    if (damage >= e->body->hp) {
        NGCharacter *b = e->body;
        int16_t x = b->x, y = b->y;
        if (mg_enemy_is_bug(e->type)) {
            /* A bug bursts into dust where it stood. */
            mg_dust_burst(x, (int16_t)(y - 14));
            ng_chars_remove(b);
            e->body = 0;
        } else {
            /*
             * Anything else is beaten the classic way: one spark where the
             * blow landed, then the creature pops up, turns over and drops
             * off the bottom of the screen.
             */
            ng_physics_detach(b);
            b->vx_fp = b->vy_fp = 0;
            b->flip_y = 1;
            b->sprite_dirty = 1;
            e->mood = MG_MOOD_DYING;
            e->move_timer = 0;
            e->heading = (int8_t)(mg.player->x < x ? 1 : -1);
            mg_burst(x, (int16_t)(y - 20), MG_T_SPARK, 1, -1);
        }
        ng_feedback_hitstop(3);
        mg.score += 250u;
        mg.kills++;

        /*
         * What the blight was holding: coins mostly, a heart or a rose now
         * and then, and once in a while a power-up or a forest friend.
         */
        switch (mg.kills % 12u) {
        /* Ammunition is earned: a sheaf or a special weapon only while
         * she's playing well; otherwise the creature gives up a coin. */
        case 1:
            mg_drop_trinket(x, (int16_t)(y - 20), mg_playing_well() ? MG_K_THORNS : MG_K_SILVER);
            break;
        case 11:
            if (mg_playing_well()) {
                static const uint8_t arms[3] = { MG_K_SPREAD, MG_K_PIERCE, MG_K_GALE };
                mg_drop_trinket(x, (int16_t)(y - 20), arms[(mg.kills / 12u) % 3u]);
            } else {
                mg_drop_trinket(x, (int16_t)(y - 20), MG_K_SILVER);
            }
            break;
        case 3:  mg_drop_trinket(x, (int16_t)(y - 20), MG_K_SILVER); break;
        case 6:  mg_drop_trinket(x, (int16_t)(y - 20), MG_K_GOLD); break;
        case 2:
        case 4:  mg_drop(x, (int16_t)(y - 20), MG_I_HEART); break;
        case 8:
            /* A Secret Art charge is special ammunition too: earned, not handed out. */
            if (!mg_playing_well()) mg_drop_trinket(x, (int16_t)(y - 20), MG_K_SILVER);
            else if ((mg.kills / 12u) % 2u) mg_drop_trinket(x, (int16_t)(y - 20), MG_K_BLOOM);
            else mg_drop(x, (int16_t)(y - 20), MG_I_ROSE_RED);
            break;
        case 9:  mg_drop_trinket(x, (int16_t)(y - 20), MG_K_SWIFT); break;
        case 10: mg_drop_trinket(x, (int16_t)(y - 20), MG_K_MIGHT); break;
        case 5:  mg_drop_trinket(x, (int16_t)(y - 20), MG_K_SPRING); break;
        case 7:  mg_drop_trinket(x, (int16_t)(y - 20), MG_K_CROWN); break;
        case 0:  mg_drop_trinket(x, (int16_t)(y - 20), MG_K_VEIL); break;
        default: break;
        }
        playSFX(SOUND_SFX_10); /* explosion */
    } else {
        mg_hit_burst(e->body->x, (int16_t)(e->body->y - 20));
        e->body->hp -= damage;
    }
}

/* A palette bank recoloured from `src`: each channel scaled by num/4,
 * optionally drained to grey first. Used for a guardian failing and
 * falling. */
static void NEOGEO_USER mg_shade_bank(uint8_t bank, const uint16_t *src, uint8_t num, uint8_t grey)
{
    uint16_t pal[16];
    uint8_t i;
    pal[0] = src[0];
    for (i = 1; i < 16; i++) {
        uint16_t c = src[i];
        uint8_t r = (uint8_t)((c >> 8) & 15u), g = (uint8_t)((c >> 4) & 15u), b = (uint8_t)(c & 15u);
        if (grey) r = g = b = (uint8_t)((r + g + b) / 3u);
        r = (uint8_t)((r * num) / 4u); g = (uint8_t)((g * num) / 4u); b = (uint8_t)((b * num) / 4u);
        pal[i] = (uint16_t)(((uint16_t)r << 8) | ((uint16_t)g << 4) | b);
    }
    mg_palette(bank, pal);
}

static void NEOGEO_USER mg_boss_damage(uint8_t damage)
{
    NGCharacter *b = mg.boss ? mg.boss : mg.eagle;
    if (!b || mg.boss_hurt || mg.state != MG_PLAY) return;
    mg.boss_hurt = 12;
    mg_hit_burst(b->x, (int16_t)(b->y - 40));
    playSFX(SOUND_SFX_4); /* metal clank / damage */
    ng_feedback_hitstop(damage >= b->hp ? 16 : 4);

    if (damage >= b->hp) {
        /*
         * The works keep two guardians.  When the Iron Vulture falls, Lord
         * Smoggar himself climbs out of the plant, at full strength, and the
         * fight goes on where it stood.  Rio Negro Works is always stage 6
         * regardless of how many valleys follow it, since it is the only one
         * the Vulture ever guards.
         */
        if (mg.stage == 6 && b->data0 == MG_B_VULTURE && mg.boss) {
            int16_t bx = b->x;
            ng_chars_remove(mg.boss);
            mg.boss = 0;
            mg_sparks(bx, (int16_t)(MG_GROUND_Y - 40));
            playSFX(SOUND_SFX_10);
            mg_palette(PAL_BOSS, mg_boss_pal(MG_B_SMOGGAR));
            mg.boss = mg_character(K_BOSS, bx, MG_GROUND_Y, PAL_BOSS, NG_RENDER_BAND_ENEMY, MG_B_SMOGGAR);
            if (mg.boss) {
                mg.boss->hp = mg.boss->max_hp = 36;
                mg.boss_timer = 0; mg.boss_rage = 0; mg.boss_direction = 0; mg.boss_px = 0;
                mg.boss_backoff = 0;
                ng_physics_attach(mg.boss, NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS);
                ng_physics_set_gravity(mg.boss, 56, 6 * NG_FP_ONE);
                mg.boss_hurt = 40;
                mg.boss->vx_fp = 0;
                mg.score += 5000u;
                mg.hud_dirty = 1;
                playSFX(SOUND_SFX_14);
                mg.state = MG_BOSS_INTRO;
                mg.state_timer = 240;
                ng_fix_clear_rect(1, ROW_CARD, 38, 9, PAL_TEXT);
                mg_centre(ROW_CARD, "LORD SMOGGAR", PAL_WARN);
                mg_centre(ROW_CARD + 2, MG_SMOGGAR_TAUNT, PAL_WARN);
                mg_centre(ROW_CARD + 5, "MAIYA", PAL_GOLD);
                mg_centre(ROW_CARD + 7, MG_SMOGGAR_REPLY, PAL_SKY);
                return;
            }
        }
        b->hp = 0;
        mg.hud_dirty = 1;
        mg.state = MG_CLEAR;
        mg.state_timer = 220;
        /* Her moment starts clean: whatever she was in the middle of -- a
         * stagger, the veil, a dash, the Secret Art's glow -- ends here, in
         * her own colours and fully on screen. */
        mg.hurt = 0; mg.hurt_lit = 0; mg.veil = 0; mg.dash = 0; mg.super_surge = 0;
        mg.attack = 0; mg.flash = 0; mg.win_step = 0; mg.win_wait = 0;
        mg_climb_end();
        mg.player->visible = 1;
        mg_palette(PAL_HERO, mg_hero_normal_pal());
        mg.hint_timer = 0;
        ng_fix_clear_rect(1, ROW_HINT, 38, 1, PAL_TEXT);
        /* It doesn't just blink away: drained of colour, knocked up and
         * back, it falls -- even a flyer -- and lies still where it lands. */
        mg_shade_bank(PAL_BOSS, mg_boss_pal((uint8_t)b->data0), 3, 1);
        ng_physics_set_gravity(b, 56, 6 * NG_FP_ONE);
        b->vy_fp = -3 * NG_FP_ONE;
        b->vx_fp = (mg.player->x < b->x) ? 320 : -320;
        mg_frame(b, MG_BF_HURT, (uint8_t)(mg.player->x < b->x));
        mg.boss_down = 0;
        mg.shake = 16;
        mg.clear_bonus = (uint16_t)(2000u + mg.stage * 1000u + (mg.player->hp * 200u) + mg.clock * 10u);
        mg.score += mg.clear_bonus;
        playSFX(SOUND_SFX_10);
        playSFX(SOUND_SFX_13);

        /* Cleanse the world: the valley's restored palette -- unless the
         * fight took place in the guardian's own arena, which stays up
         * behind the victory. */
        if (!mg.arena_bg) mg_background(mg_levels[mg.stage].background, 1);
        mg_centre(ROW_CARD + 2, "EARTH RESTORED!", PAL_GOLD);
        mg_centre(ROW_CARD + 4, "THE BLIGHT IS CLEANSED", PAL_SKY);
    } else {
        b->hp -= damage;
        mg.hud_dirty = 1;
    }
}

/* Slimes, beetles and goblins can be landed on; the flier, the worm and
 * the drone cannot -- they are all edge, spark or altitude. */
static uint8_t NEOGEO_USER mg_soft_enemy(uint8_t type)
{
    return (uint8_t)(type == MG_E_SLIME || type == MG_E_BEETLE || type == MG_E_GOBLIN
                      || type == MG_E_JELLYFISH);
}

static uint8_t NEOGEO_USER mg_strike(void)
{
    return (uint8_t)(mg.might ? 4 : 2);
}

static void NEOGEO_USER mg_sad_face_palette(void);
static void NEOGEO_USER mg_bonus_caught(void);

/* Returns 1 when the hit actually landed. */
static uint8_t NEOGEO_USER mg_player_damage(void)
{
    NGCharacter *p = mg.player;
    /* In the bonus round one touch ends it: no health lost, no reward. */
    if (mg.state == MG_BONUS) { mg_bonus_caught(); return 0; }
    if (mg.hurt || mg.veil || mg.dash || mg.super_surge || mg.state != MG_PLAY) return 0;
    mg.hurt = 90;
    if (mg.attempt_hits < 255) mg.attempt_hits++;
    mg_climb_end();
    mg.attack = mg.combo = mg.cast = 0;
    mg.hud_dirty = 1;
    playSFX(SOUND_SFX_16); /* player hurt */
    ng_feedback_hitstop(5);
    /* Two small sparks where the blow lands -- it happens on every hit,
     * so it stays small and quick; her bar and her glint say the rest. */
    mg_burst(p->x, (int16_t)(p->y - 30), MG_T_SPARK, 2, -1);
    mg.shake = 10;
    /* Staggered back, away from the way she faces unless mg_player_hit()
     * knows where the blow came from; the controls ease it to a stop. */
    p->vx_fp = mg.facing ? MG_KNOCK : -MG_KNOCK;

    if (--p->hp == 0) {
        /* The bar shows the loss in full: it empties now rather than
         * freezing at the last point when play stops for her fall. */
        mg.hp_px = 0;
        mg_draw_hp_bar();
        mg_sad_face_palette();
        /*
         * Maiya does not fall over: the valley lifts her, and she rises
         * out of frame in the sunlight before the mission starts again.
         */
        mg.state = MG_DEAD;
        mg.state_timer = ANGEL_TIME + 40;
        mg.angel = 1;
        if (mg.lives) mg.lives--;
        mg.hud_dirty = 1;
        mg_update_hud();
        mg.swift = mg.might = mg.veil = mg.spring = mg.crown = 0;
        p->vx_fp = p->vy_fp = 0;
        ng_physics_detach(p);
        playSFX(SOUND_SFX_16);   /* the blow */
        playSFX(SOUND_SFX_10);
        mg_hint("MAIYA: SUNBOY... WAIT FOR ME", PAL_SKY, 200);
    }
    return 1;
}

/*
 * A hit with a source: she is knocked away from whatever struck her, not
 * simply backwards -- facing away from a guardian, "backwards" threw her
 * into its body, where the next touch found her again.
 */
static void NEOGEO_USER mg_player_hit(int16_t from_x)
{
    NGCharacter *p = mg.player;
    if (!mg_player_damage() || mg.state != MG_PLAY) return;
    p->vx_fp = (p->x < from_x) ? -MG_KNOCK : MG_KNOCK;
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

    /*
     * Petals sweep the whole screen and the valley shakes.  Her own colours
     * only lift for a moment -- a long recolour read as a costume change and
     * made it hard to tell what had actually happened.
     */
    mg.flash = 12;                 /* her colours lift, and come straight back */
    mg_palette(PAL_HERO, mg_hero_sun_pal);
    mg.attack = 22;
    mg.shake = 24;
    mg.art_wave = 24;              /* the second wave follows the first */
    playSFX(SOUND_SFX_9);          /* the clear ring of the purification */
    mg_petal_sweep();

    /* Everyone present takes the art -- no hit sparks, they would overwrite
     * the storm in the same particle pool. */
    for (i = 0; i < MG_ENEMIES; i++) {
        if (mg.enemies[i].body) mg_enemy_damage(&mg.enemies[i], 10);
    }
    if (mg.boss) mg_boss_damage(6);

    mg_hint(mg_art_words[mg.stage], PAL_GOLD, 100);
}

/* ------------------------------------------------------------------ */
/*  Scene & Level Initialization                                      */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER mg_spawn(void);
static void NEOGEO_USER mg_hazard_disable_check(uint16_t pressed);

/*
 * retry: this is her trying the mission again after a fall, not a fresh
 * arrival -- same starting spot (the road always begins at its own head;
 * there are no mid-level checkpoints), but the keys, the gate and anything
 * she had picked up are gone, the same as if the valley had reset itself.
 */
/* The smog wraith wears the moth's shape in a washed-out, blue-grey
 * version of its own colours: the same creature, drained to a ghost. */
static void NEOGEO_USER mg_ghost_palette(void)
{
    uint16_t pal[16];
    uint8_t i;
    for (i = 0; i < 16; i++) {
        uint16_t c = mg_acidmoth_pal[i];
        uint8_t v = (uint8_t)((((c >> 8) & 15u) + ((c >> 4) & 15u) + (c & 15u)) / 3u);
        uint8_t b = (uint8_t)(v + 4u > 15u ? 15u : v + 4u);
        pal[i] = (uint16_t)((v << 8) | (v << 4) | b);
    }
    pal[0] = mg_acidmoth_pal[0];
    mg_palette(PAL_WRAITH, pal);
}

/*
 * Her HUD avatar, drained the same way as the wraith above, the moment she
 * loses a life -- the same face, but pale and blue instead of her own warm
 * colours, so the loss reads on her portrait and not just the HP bar.
 * mg_hud_static() puts her own colours straight back the next time the
 * scene starts, so nothing has to restore this explicitly.
 */
static void NEOGEO_USER mg_sad_face_palette(void)
{
    const uint16_t *base = mg.hero_choice ? mg_face_alt_pal : mg_face_pal;
    uint16_t pal[16];
    uint8_t i;
    for (i = 0; i < 16; i++) {
        uint16_t c = base[i];
        uint8_t v = (uint8_t)((((c >> 8) & 15u) + ((c >> 4) & 15u) + (c & 15u)) / 3u);
        uint8_t b = (uint8_t)(v + 4u > 15u ? 15u : v + 4u);
        pal[i] = (uint16_t)((v << 8) | (v << 4) | b);
    }
    pal[0] = base[0];
    mg_palette(PAL_FACE, pal);
}

static void NEOGEO_USER mg_scene(uint8_t stage, uint8_t retry)
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
    /* A heavy blow holds the whole valley still for a few frames. */
    ng_game_engine_set_hitstop_freeze(1);

    mg_ui_palettes();
    if (mg.stage != stage) mg.rescue_mask = 0;
    mg.stage = stage; mg.state = MG_INTRO; mg.pause = 0;
    mg.tick = mg.boss_timer = mg.encounter_mask = mg.archer_mask = 0;
    mg.walk_distance = 0;
    mg.climb_cooldown = 0; mg.airborne = 0; mg.land_pose = 0;
    mg.arena_left = (int16_t)(level->width - 320);
    mg.arena[0] = (MGPlatform){ (int16_t)(mg.arena_left + 24), 144, 64 };
    mg.arena[1] = (MGPlatform){ (int16_t)(mg.arena_left + 232), 144, 64 };
    mg.attack = mg.combo = mg.dash = mg.dash_wait = mg.boss_hurt = mg.boss_active = 0;
    mg.hurt = 0; mg.coyote = mg.jump_buffer = mg.drop = mg.cast = mg.super_surge = 0;
    mg.boss = mg.rescue = mg.eagle = 0; mg.ledges_used = 0; mg.eagle_timer = 0;
    mg.on_ledge = 0; mg.combo_timer = 0; mg.combo_buffer[0] = mg.combo_buffer[1] = 0;
    mg.climbing = 0; mg.crouch_timer = 0; mg.sitting = 0;
    /*
     * A retry (death, or a continue) drops her back at the start of the
     * same mission, but whatever she'd already collected -- coins, the
     * charm, the hidden life -- stays collected. Without this, dying next
     * to the life pickup and walking back to the same spot every time was
     * a free, repeatable source of extra lives.
     */
    if (!retry) {
        mg.has_key = 0; mg.gate_unlocked = 0; mg.key_taken = 0;
        mg.pick_mask = 0; mg.secret_mask = 0; mg.hazard_warn_mask = 0; mg.hazard_disabled_mask = 0;
        mg.level_falls = 0;
    } else {
        /* A fall costs the special weapon and the thorn sheaf; her own
         * whip and thorn throw are always hers. */
        if (mg.level_falls < 255) mg.level_falls++;
        mg.weapon = MG_W_NONE; mg.weapon_ammo = 0;
        mg.thorns = 0;
    }
    mg.attempt_hits = 0;
    mg.clock = MG_LEVEL_SECONDS; mg.clock_sub = 0; mg.wraith_timer = 0;
    mg.hp_px = MG_HP_BAR_PX; mg.boss_px = 0; mg.cage_open = 0; mg.arena_bg = 0;
    mg_fade_k = 0; mg.win_step = 0; mg.win_wait = 0; mg.cam_lead = MG_CAM_LEAD;
    for (i = 0; i < MG_PLATFORM_COUNT; i++) { mg.ledge_stand[i] = 0; mg.ledge_gone[i] = 0; }
    mg.gate_shown = 0;
    mg.npc_mask = 0; mg.npc_live = 0; mg.npc_here = 0;
    mg.swift = mg.might = mg.veil = 0; mg.spring = mg.crown = 0;
    mg.flash = 0; mg.angel = 0; mg.hurt_lit = 0; mg.art_wave = 0;
    for (i = 0; i < MG_NPC_SLOTS; i++) mg.npcs[i] = 0;
    mg.notice = 0; mg.facing = 0; mg.kills = 0;
    mg.state_timer = 0;
    mg.hint_timer = 0;
    mg.previous_joy = poll_joystick();

    ng_level_set_world_bounds(0, 0, (int16_t)level->width, 224);
    /* The road, in stretches: every pit is a gap in the ground itself. */
    {
        int16_t from = 0;
        uint8_t k, done[MG_HAZARD_COUNT];
        for (k = 0; k < MG_HAZARD_COUNT; k++) done[k] = 0;
        for (;;) {
            int8_t next = -1;
            for (k = 0; k < MG_HAZARD_COUNT; k++) {
                const MGHazard *hz = &level->hazards[k];
                if (done[k] || hz->type != MG_H_PIT || !hz->width) continue;
                if (next < 0 || hz->x < level->hazards[next].x) next = (int8_t)k;
            }
            if (next < 0) break;
            done[next] = 1;
            if (level->hazards[next].x > from)
                ng_physics_add_solid(from, MG_GROUND_Y, (int16_t)(level->hazards[next].x - from), 32, 0);
            from = (int16_t)(level->hazards[next].x + level->hazards[next].width);
        }
        ng_physics_add_solid(from, MG_GROUND_Y, (int16_t)(level->width - from), 32, 0);
    }

    /* Load entity palettes.  The creatures wear this valley's colours. */
    mg_palette(PAL_HERO, mg_hero_normal_pal());
    {
        uint16_t tint = (uint16_t)(stage * 16u);
        mg_palette(PAL_ENEMY0, mg_slime_valley_pal + tint);
        mg_palette(PAL_ENEMY1, mg_beetle_valley_pal + tint);
        mg_palette(PAL_ENEMY2, mg_crow_valley_pal + tint);
        mg_palette(PAL_ENEMY3, mg_goblin_valley_pal + tint);
        mg_palette(PAL_ENEMY4, mg_worm_valley_pal + tint);
        mg_palette(PAL_ENEMY5, mg_robot_valley_pal + tint);
    }
    mg_palette(PAL_BOSS, mg_boss_pal(level->boss_style));
    mg_palette(PAL_ALLY, mg_ally_pal(level->rescue_type[0]));
    mg_palette(PAL_EAGLE, mg_eagle_pal);
    mg_palette(PAL_JELLYFISH, mg_jellyfish_pal);
    mg_palette(PAL_TOXICCRAB, mg_toxiccrab_pal);
    mg_palette(PAL_ACIDMOTH, mg_acidmoth_pal);
    mg_palette(PAL_SEWERRAT, mg_sewerrat_pal);
    mg_palette(PAL_SMOGBAT, mg_smogbat_pal);
    mg_palette(PAL_POACHDRONE, mg_poachdrone_pal);
    mg_palette(PAL_CHEMFLY, mg_chemfly_pal);
    mg_palette(PAL_PLASTICBAT, mg_plasticbat_pal);
    mg_palette(PAL_SLAGGOLEM, mg_slaggolem_pal);
    mg_palette(PAL_VINESTING, mg_vinesting_pal);
    mg_palette(PAL_SPOREGOB, mg_sporegob_pal);
    mg_ghost_palette();
    mg_palette(PAL_TOOL, mg_tool_pal);
    mg_palette(PAL_PORTRAIT, mg_portrait_pal);
    mg_palette(PAL_PORTRAIT + 1, mg_portrait_pal + 16);
    mg_palette(PAL_PROP, mg_prop_pal);
    mg_palette(PAL_HAZARD, mg_hazard_pal);
    {
        const uint16_t *pit_pal;
        mg_pit_art(&pit_pal);
        mg_palette(PAL_PIT, pit_pal);
    }
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
     * her in above the road; a respawn uses the same entrance at the
     * position where she fell, keeping the key and gate progress.
     */
    mg.entrance = 1;
    mg.player = mg_character(K_PLAYER, 64, -24,
                             PAL_HERO, NG_RENDER_BAND_PLAYER, 0);
    if (!mg.player) return;
    mg.player->hp = mg.player->max_hp = MAX_HP;
    ng_physics_attach(mg.player, NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS);
    mg_player_gravity();
    mg_frame(mg.player, MG_F_IDLE0, 0);
    mg.player_prev_y = mg.player->y;

    /* Camera setup */
    ng_camera_init(&mg.camera);
    ng_camera_set_bounds(&mg.camera, 0, 0, (int16_t)level->width, 224);
    /* A tight, quick camera: a lazy one lurched a few pixels every third
     * frame, which read as the whole valley juddering. */
    ng_camera_set_dead_zone(&mg.camera, 0, 24);
    ng_camera_set_follow_speed(&mg.camera, 64);
    mg.shake = 0;
    mg.shake_x = 0;
    ng_camera_snap(&mg.camera, 0, 0);
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
        mg.sparks[i].shrink = 0;
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
                             mg_hazard_tiles[MG_HZ_SPIKES0], PAL_HAZARD);
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
    mg_palette(PAL_FRONT, mg_front_pal);
    for (i = 0; i < MG_FRONT_SLOTS; i++) {
        /* A frond, then the valley's own stone, then a frond again. */
        uint16_t tile = (i == 1) ? mg_front_tiles[MG_FR_STONE_GRASS + stage]
                                 : mg_front_tiles[MG_FR_FERN];
        ng_sprite_group_init(&mg.front[i], (uint16_t)(SLOT_FRONT + i * 2), 2, 3,
                             tile, PAL_FRONT);
        ng_sprite_group_set_visible(&mg.front[i], 0);
    }

    ng_sprite_group_init(&mg.gate, SLOT_GATE, 2, 3, mg_gate_tiles[0], PAL_GATE);
    ng_sprite_group_set_visible(&mg.gate, 0);

    ng_sprite_group_init(&mg.cage, SLOT_CAGE, 2, 2, mg_prop_tiles[MG_P_CHEST], PAL_PROP);
    ng_sprite_group_set_visible(&mg.cage, 0);

    /* Reserve the mandatory key before streaming optional pickups. */
    if (!mg.has_key && !mg.gate_unlocked) mg_drop_key((int16_t)mg_key_pos[stage][0], (int16_t)mg_key_pos[stage][1]);

    /* Spawn initial wave immediately so enemies are on-screen from frame 1 */
    mg_spawn();

    waitVbl();
    mg_hud_static();
    /* The bar, tray and key icon otherwise stay blank until something later
     * flips hud_dirty (damage, a pickup, a power-up expiring) -- drawing
     * them here means the HUD is fully correct from frame 1 of every scene,
     * not just eventually. */
    mg_update_hud();
    mg.hud_dirty = 0;
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
        mg_centre(ROW_CARD + 6, retry ? "MAIYA: I AM NOT DONE YET!"
                                      : "MAIYA: THE VALLEY CALLED ME!", PAL_SKY);
        {
            static const char *const warn[] = {
                0, 0, 0,
                "ROTTEN LEDGES CRUMBLE - KEEP MOVING", "THE ROAD IS ICE - SHE WILL SLIDE",
                "UNDERWATER - EVERY JUMP FLOATS",
            };
            uint8_t m = mg_mech();
            if (m && m < sizeof(warn) / sizeof(warn[0]) && warn[m]) mg_centre(ROW_CARD + 8, warn[m], PAL_GOLD);
        }
    }

    mg.music_on = 0;
    mg_music(level->music);
}

uint8_t NEOGEO_USER maiya_hero_choice(void);

void NEOGEO_USER maiya_boot(void)
{
    mg.lives = 3; mg.art = MAX_ART; mg.score = 0; mg.rescue_mask = 0;
    mg.continues = MAX_CONTINUES;
    mg.coins = mg.flowers = mg.critters = 0;
    mg.hero_choice = maiya_hero_choice();
    mg.session_over = 0;
    mg.life_pickups_used = 0;
    mg.next_life_score = MG_BONUS_LIFE_SCORE_FIRST;
    mg.thorns = 0; mg.weapon = MG_W_NONE; mg.weapon_ammo = 0;
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
    mg.coins = mg.flowers = mg.critters = 0;
    mg.continues = 0;
    mg.session_over = 0;
    mg.life_pickups_used = 0;
    mg.next_life_score = MG_BONUS_LIFE_SCORE_FIRST;
    mg.thorns = 0; mg.weapon = MG_W_NONE; mg.weapon_ammo = 0;
    mg_scene(demo_stage, 0);
    ng_fix_clear_rect(1, ROW_HINT, 38, 9, PAL_TEXT);
    mg_centre(ROW_CARD + 2, "ATTRACT MODE", PAL_GOLD);
    mg.state_timer = 80;

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
/*  Company Eyecatcher                                                */
/* ------------------------------------------------------------------ */

/*
 * EAGLE SOFTWARE, built out of the solid FIX block at code 7.  Three cells
 * wide and five tall per letter, drawn a column at a time so the name
 * assembles itself while the theme plays.
 */
static const uint8_t mg_logo_glyph[13][5] = {
    { 7, 4, 6, 4, 7 },   /*  0 E */
    { 2, 5, 7, 5, 5 },   /*  1 A */
    { 3, 4, 5, 5, 3 },   /*  2 G */
    { 4, 4, 4, 4, 7 },   /*  3 L */
    { 3, 4, 2, 1, 6 },   /*  4 S */
    { 2, 5, 5, 5, 2 },   /*  5 O */
    { 7, 4, 6, 4, 4 },   /*  6 F */
    { 7, 2, 2, 2, 2 },   /*  7 T */
    { 5, 5, 5, 7, 5 },   /*  8 W */
    { 6, 5, 6, 5, 5 },   /*  9 R */
    { 0, 0, 0, 0, 0 },   /* 10 space */
    { 5, 5, 5, 5, 2 },   /* 11 U */
    { 7, 1, 2, 4, 7 },   /* 12 Z (spare) */
};

/* EAGLE / SOFTWARE as indices into the table above. */
static const uint8_t mg_logo_eagle[5] = { 0, 1, 2, 3, 0 };
static const uint8_t mg_logo_software[8] = { 4, 5, 6, 7, 8, 1, 9, 0 };

static void NEOGEO_USER mg_logo_word(const uint8_t *word, uint8_t len,
                                     uint8_t left, uint8_t top, uint8_t pal)
{
    uint8_t i, row, col;

    for (i = 0; i < len; i++) {
        const uint8_t *glyph = mg_logo_glyph[word[i]];
        for (row = 0; row < 5; row++) {
            for (col = 0; col < 3; col++) {
                if (glyph[row] & (uint8_t)(4u >> col)) {
                    ng_fix_putc((uint8_t)(left + i * 4 + col),
                                (uint8_t)(top + row), (char)GLYPH_BLOCK, pal);
                }
            }
        }
        waitVbl();
        waitVbl();
        waitVbl();
    }
}

void NEOGEO_USER maiya_eyecatcher(void)
{
    uint16_t i;

    /*
     * The eyecatcher runs before the game has a screen: the palette bank is
     * wherever the BIOS left it and VRAM still holds whatever was there, so
     * take the screen over properly -- bank 0, our own inks, a black
     * backdrop, and an empty FIX layer -- before drawing a single block.
     */
    NEO_REGISTER8(REG_PALBANK0) = 0;
    clearSprs();
    clearFix();
    setBACKDROP(BLACK);
    ng_fix_init();
    ng_fix_clear();
    mg_ui_palettes();
    waitVbl();

    /* FM alone under the logo: three rising notes and a held fifth. */
    soundStopAll();
    mg.music_on = 0;
    soundSetADPCMBVolume(0x00);
    soundSetFMVolume(0x0C);
    playFMTrack(SOUND_FM_TRACK_1);

    mg_logo_word(mg_logo_eagle, 5, 10, 10, PAL_GOLD);
    mg_logo_word(mg_logo_software, 8, 4, 17, PAL_TEXT);

    for (i = 0; i < 8; i++) {
        ng_fix_putc((uint8_t)(9 + i * 3), 24, (char)GLYPH_SPARK, PAL_GOLD);
        waitVbl();
        waitVbl();
    }
    ng_fix_puts(16, 27, "PRESENTS", PAL_SKY);

    for (i = 0; i < 120; i++) waitVbl();

    soundStopAll();
    ng_fix_clear();
    waitVbl();
}

/* ------------------------------------------------------------------ */
/*  Attract Mode & Title Screen                                       */
/* ------------------------------------------------------------------ */
/*
 * Who the player is offered at the title: her own face, in her own
 * colours, so the choice is seen before it is made.  The picture never
 * changes, only which of the two palette banks it is drawn with.
 */
static NGSpriteGroup mg_chooser_maiya, mg_chooser_luna;
static uint8_t mg_chooser_pick;

/*
 * The select screen: the two heroines' faces side by side in gold frames,
 * each standing full length under her own card, over the Emerald Forest
 * dimmed and drifting behind. The one being chosen is in her full colours
 * with her frame shimmering and moves; the other is shown in grey, still,
 * rather than hidden, so it's always clear who the choice is between.
 * The frames are FIX tiles hugging each 96 x 96 portrait; the name sits on
 * the frame's lower edge like a plate.
 */
#define MG_CHOOSER_MAIYA_X    40
#define MG_CHOOSER_LUNA_X    184
#define MG_CHOOSER_FACE_Y     40
#define MG_CHOOSER_HINT_ROW   26
#define MG_CHOOSER_FEET_Y    206
#define PAL_PORTRAIT_ALT      62
/* FIX and sprite banks borrowed while the select screen is up; the game's
 * own palettes are loaded over them before play starts. */
#define PAL_SEL_FRAME         9    /* +0 Maiya's frame, +1 Luna's      */
#define PAL_SEL_GREY         11    /* a grey ink for the name not picked */
#define PAL_SEL_HERO          4    /* +0 Maiya standing, +1 Luna        */

static NGSpriteGroup mg_chooser_body[2];

/* A portrait's palette in grey, a little dimmed: still visibly her, just
 * not the one being picked. */
static void NEOGEO_USER mg_grey_banks(uint8_t bank, const uint16_t *src, uint8_t banks)
{
    uint16_t pal[16];
    uint8_t k, i;
    for (k = 0; k < banks; k++) {
        for (i = 0; i < 16; i++) {
            uint16_t c = src[k * 16 + i];
            uint8_t v = (uint8_t)((((c >> 8) & 15u) + ((c >> 4) & 15u) + (c & 15u)) / 3u);
            v = (uint8_t)((v * 3u) / 4u + 2u);   /* dimmed, but dark hair stays readable */
            if (v > 15u) v = 15u;
            pal[i] = (uint16_t)((v << 8) | (v << 4) | v);
        }
        pal[0] = src[k * 16];
        mg_palette((uint8_t)(bank + k), pal);
    }
}

/* The frame's pens: 1 outline, 2 gold, 3 gold light, 4 gold shadow,
 * 5-6 the corner jewel in her colour. Lit, the light pen shimmers. */
static void NEOGEO_USER mg_chooser_frame_pal(uint8_t who, uint8_t lit, uint8_t phase)
{
    static const uint8_t shine[4] = { 0, 1, 2, 1 };
    uint16_t pal[16];
    uint8_t i, s = shine[phase & 3u];
    pal[0] = 0;
    if (lit) {
        pal[1] = MG_RGB(40, 24, 8);
        pal[2] = MG_RGB(214 + s * 16, 170 + s * 16, 64 + s * 24);
        pal[3] = MG_RGB(255, 232 + s * 8, 150 + s * 40);
        pal[4] = MG_RGB(150, 100, 30);
        pal[5] = who ? MG_RGB(80, 140, 255) : MG_RGB(40, 200, 90);
        pal[6] = who ? MG_RGB(200, 230, 255) : MG_RGB(200, 255, 200);
    } else {
        pal[1] = MG_RGB(16, 16, 16);
        pal[2] = MG_RGB(104, 104, 104);
        pal[3] = MG_RGB(144, 144, 144);
        pal[4] = MG_RGB(64, 64, 64);
        pal[5] = MG_RGB(88, 88, 88);
        pal[6] = MG_RGB(136, 136, 136);
    }
    for (i = 7; i < 16; i++) pal[i] = pal[2];
    mg_palette((uint8_t)(PAL_SEL_FRAME + who), pal);
}

/* Fourteen cells square around a portrait whose top-left cell is (c, r);
 * the lower edge carries her name. */
static void NEOGEO_USER mg_chooser_frame(uint8_t c, uint8_t r, uint8_t who, const char *name)
{
    uint8_t pal = (uint8_t)(PAL_SEL_FRAME + who), k;
    uint8_t len = 0, left;
    while (name[len]) len++;
    left = (uint8_t)(c + (12u - len) / 2u);
    ng_fix_put_tile((uint8_t)(c - 1), (uint8_t)(r - 1), MG_FRAME_TILE + 0, pal);
    ng_fix_put_tile((uint8_t)(c + 12), (uint8_t)(r - 1), MG_FRAME_TILE + 2, pal);
    ng_fix_put_tile((uint8_t)(c - 1), (uint8_t)(r + 12), MG_FRAME_TILE + 5, pal);
    ng_fix_put_tile((uint8_t)(c + 12), (uint8_t)(r + 12), MG_FRAME_TILE + 7, pal);
    for (k = 0; k < 12; k++) {
        ng_fix_put_tile((uint8_t)(c + k), (uint8_t)(r - 1), MG_FRAME_TILE + 1, pal);
        ng_fix_put_tile((uint8_t)(c - 1), (uint8_t)(r + k), MG_FRAME_TILE + 3, pal);
        ng_fix_put_tile((uint8_t)(c + 12), (uint8_t)(r + k), MG_FRAME_TILE + 4, pal);
        if (c + k < left - 1 || c + k > left + len)
            ng_fix_put_tile((uint8_t)(c + k), (uint8_t)(r + 12), MG_FRAME_TILE + 6, pal);
        else
            ng_fix_blank_cell((uint8_t)(c + k), (uint8_t)(r + 12));
    }
}

static void NEOGEO_USER mg_draw_chooser(void)
{
    uint8_t who;
    for (who = 0; who < 2; who++) {
        uint8_t lit = (uint8_t)(mg_chooser_pick == who);
        uint8_t col = (uint8_t)((who ? MG_CHOOSER_LUNA_X : MG_CHOOSER_MAIYA_X) / 8);
        mg_chooser_frame_pal(who, lit, 0);
        mg_chooser_frame(col, MG_CHOOSER_FACE_Y / 8, who, who ? "LUNA" : "MAIYA");
        ng_fix_puts((uint8_t)(col + (who ? 4 : 3)), (uint8_t)(MG_CHOOSER_FACE_Y / 8 + 12),
                    who ? "LUNA" : "MAIYA", lit ? PAL_GOLD : PAL_SEL_GREY);
        if (lit) {
            mg_palette((uint8_t)(PAL_SEL_HERO + who), who ? mg_hero_alt_pal : mg_hero_pal);
        } else {
            mg_grey_banks((uint8_t)(PAL_SEL_HERO + who), who ? mg_hero_alt_pal : mg_hero_pal, 1);
        }
    }
    if (mg_chooser_pick == 0) {
        mg_palette(PAL_PORTRAIT, mg_portrait_pal);
        mg_palette(PAL_PORTRAIT + 1, mg_portrait_pal + 16);
        mg_grey_banks(PAL_PORTRAIT_ALT, mg_portrait_alt_pal, 2);
    } else {
        mg_grey_banks(PAL_PORTRAIT, mg_portrait_pal, 2);
        mg_palette(PAL_PORTRAIT_ALT, mg_portrait_alt_pal);
        mg_palette(PAL_PORTRAIT_ALT + 1, mg_portrait_alt_pal + 16);
    }
    ng_sprite_group_set_pos(&mg_chooser_maiya, MG_CHOOSER_MAIYA_X, MG_CHOOSER_FACE_Y);
    ng_sprite_group_set_pos(&mg_chooser_luna, MG_CHOOSER_LUNA_X, MG_CHOOSER_FACE_Y);
    ng_sprite_group_upload(&mg_chooser_maiya);
    ng_sprite_group_upload(&mg_chooser_luna);
}

/* One frame of the select screen's life: the forest drifts, the chosen
 * frame shimmers, the chosen girl breathes; after a confirm, she holds her
 * victory pose. */
static void NEOGEO_USER mg_chooser_tick(uint16_t t, uint8_t chosen)
{
    uint8_t who;
    ng_sprite_group_set_pos(&mg.far, (int16_t)(-(int16_t)(t / 3u) & 511), 0);
    ng_sprite_group_flush(&mg.far);
    ng_sprite_group_set_pos(&mg.road, (int16_t)(-(int16_t)(t / 2u) & 511), MG_GROUND_Y);
    ng_sprite_group_flush(&mg.road);
    if ((t & 7u) == 0u) mg_chooser_frame_pal(mg_chooser_pick, 1, (uint8_t)(t >> 3));
    for (who = 0; who < 2; who++) {
        uint8_t f = MG_F_IDLE0;
        if (who == mg_chooser_pick) f = chosen ? MG_F_WIN : (uint8_t)(MG_F_IDLE0 + (t / 12u) % 3u);
        ng_sprite_group_set_tile_base(&mg_chooser_body[who], mg_hero_tiles[f]);
        ng_sprite_group_upload(&mg_chooser_body[who]);
    }
}

void NEOGEO_USER maiya_title(void)
{
    NGSpriteGroup title_vis;
    ng_sprite_hide_all();
    ng_fix_init();
    mg_ui_palettes();

    /* Load title visual: sixteen palettes, one picked per tile. */
    {
        uint8_t k;
        for (k = 0; k < MG_TITLE_BANKS; k++)
            mg_palette((uint8_t)(MG_TITLE_PAL_BANK + k), mg_title_pal + k * 16u);
    }

    ng_sprite_group_init(&title_vis, SLOT_TITLE, 19, 14, MG_TITLE_TILE, MG_TITLE_PAL_BANK);
    ng_sprite_group_set_palette_map(&title_vis, mg_title_map);
    ng_sprite_group_set_pos(&title_vis, 8, 8);
    ng_sprite_group_upload(&title_vis);


    /* The title theme, looping until a credit or the demo takes over. */
    mg.music_on = 0;
    mg_music(SOUND_TRACK_G);
}

/*
 * Left over from an earlier layout that showed the chooser during the
 * coin-wait, overlapping "PUSH START". No longer called there -- the
 * chooser is now its own screen, shown after Start, in
 * maiya_hero_select() below -- but kept as a no-op entry point since
 * user.c still declares it.
 */
void NEOGEO_USER maiya_title_frame(void)
{
}

uint8_t NEOGEO_USER maiya_hero_choice(void)
{
    return mg_chooser_pick;
}

/*
 * A quick reminder of the controls, shown once after she's chosen and
 * skippable the moment a hand is on the stick -- nobody who already knows
 * the game should have to sit through it twice in a session.
 */
static void NEOGEO_USER mg_show_how_to_play(void)
{
    uint8_t i;
    uint16_t joy;

    ng_fix_clear();
    mg_centre(6,  "HOW TO PLAY", PAL_GOLD);
    mg_centre(10, "LEFT / RIGHT: WALK", PAL_TEXT);
    mg_centre(11, "UP: CLIMB A VINE, OR TURN THE KEY", PAL_TEXT);
    mg_centre(13, "A: JUMP (HOLD FOR HEIGHT)", PAL_TEXT);
    mg_centre(14, "B: THROW A THORN, OR STRIKE UP CLOSE", PAL_TEXT);
    mg_centre(15, "C: DASH", PAL_TEXT);
    mg_centre(16, "D: SECRET ART", PAL_TEXT);
    mg_centre(18, "DOWN, FORWARD + B: ROSE BLOSSOM SURGE", PAL_SKY);
    mg_centre(22, "PRESS ANY BUTTON TO BEGIN", PAL_GOLD);

    poll_joystick_edge();
    /* The confirm that brought her here shouldn't also skip this: give it
     * a moment to let go before a press counts. */
    for (i = 0; i < 20; i++) { waitVbl(); poll_joystick_edge(); }

    for (;;) {
        waitVbl();
        joy = poll_joystick_edge();
        if (joy & (BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_D | START1 | START2)) break;
    }
    ng_fix_clear();
}

/*
 * The story, told once, between the control reminder and the first
 * mission -- so a run opens on why she's walking into the forest, not
 * straight onto a health bar with no context.
 */
static void NEOGEO_USER mg_show_intro_story(void)
{
    uint8_t i;
    uint16_t joy;

    ng_fix_clear();
    mg_centre(7,  "A BLIGHT HAS FALLEN ON THE VALLEYS", PAL_WARN);
    mg_centre(9,  "THE RIVERS RUN GREY.", PAL_TEXT);
    mg_centre(10, "THE GROVES FALL SILENT.", PAL_TEXT);
    mg_centre(13, "ONLY THE GOLDEN SUN KEY CAN SEAL", PAL_TEXT);
    mg_centre(14, "EACH GUARDIAN'S POISONED GATE.", PAL_TEXT);
    mg_centre(17, "MAIYA, DAUGHTER OF THE FOREST,", PAL_SKY);
    mg_centre(18, "RISES TO CLEANSE THE LAND.", PAL_SKY);
    mg_centre(22, "PRESS ANY BUTTON TO BEGIN", PAL_GOLD);

    poll_joystick_edge();
    for (i = 0; i < 20; i++) { waitVbl(); poll_joystick_edge(); }

    for (;;) {
        waitVbl();
        joy = poll_joystick_edge();
        if (joy & (BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_D | START1 | START2)) break;
    }
    ng_fix_clear();
}

/*
 * The dedicated "CHOOSE YOUR GUARDIAN" screen: shown once, after Start is
 * pressed, not layered over the coin-insert prompt, and not over the
 * title's own logo art either -- her own screen, with its own music.
 * Left/Right moves the pick, A confirms: the chosen girl's own fanfare
 * plays and she holds her victory pose a moment before the game goes on.
 * If the credit that started the game came in on the cabinet's second
 * player side, Luna answers the call by default instead of Maiya -- still
 * just a starting point, still changeable before confirming.
 */
void NEOGEO_USER maiya_hero_select(void)
{
    uint8_t i, who;
    uint16_t joy, t = 0;
    mg_chooser_pick = (NEO_REGISTER8(BIOS_PLAYER2_MODE) != 0) ? 1 : 0;

    ng_sprite_hide_all();
    ng_fix_clear();
    mg_ui_palettes();
    mg_ink(PAL_SEL_GREY, MG_RGB(120, 120, 120));
    /* Everything is set up behind a white screen and comes up out of it in
     * one piece, instead of the forest, the cards and the text popping in
     * one after another while they load. */
    mg_fade_begin();
    mg_fade_set(16);
    mg_fade_commit();

    /* The forest behind, at half its brightness so the cards stand out. */
    mg.arena_bg = 0;
    mg_background(0, 1);
    for (i = 0; i < MG_BG0_BANKS; i++)
        mg_shade_bank((uint8_t)(PAL_BG + i), mg_bg0_pal + i * 16u, 2, 0);

    ng_sprite_group_init(&mg_chooser_maiya, SLOT_TITLE, 6, 6, MG_PORTRAIT_TILE, PAL_PORTRAIT);
    ng_sprite_group_set_palette_map(&mg_chooser_maiya, mg_portrait_map);
    ng_sprite_group_init(&mg_chooser_luna, (uint16_t)(SLOT_TITLE + 6), 6, 6, MG_PORTRAIT_ALT_TILE, PAL_PORTRAIT_ALT);
    ng_sprite_group_set_palette_map(&mg_chooser_luna, mg_portrait_alt_map);
    ng_sprite_group_set_visible(&mg_chooser_maiya, 1);
    ng_sprite_group_set_visible(&mg_chooser_luna, 1);
    /* Each girl stands under her own card, the two turned toward each other. */
    for (who = 0; who < 2; who++) {
        NGSpriteGroup *g = &mg_chooser_body[who];
        ng_sprite_group_init(g, (uint16_t)(NG_SPR_CHAR_FIRST + who * HERO_STRIPS), HERO_STRIPS, HERO_ROWS,
                             mg_hero_tiles[MG_F_IDLE0], (uint8_t)(PAL_SEL_HERO + who));
        ng_sprite_group_set_tile_stride(g, HERO_STRIDE);
        ng_sprite_group_set_flip(g, who, 0);
        ng_sprite_group_set_pos(g, (int16_t)((who ? MG_CHOOSER_LUNA_X : MG_CHOOSER_MAIYA_X) + 48 - 40),
                                (int16_t)(MG_CHOOSER_FEET_Y - 62));
        ng_sprite_group_set_visible(g, 1);
    }
    mg_centre(2, "CHOOSE YOUR GUARDIAN", PAL_GOLD);
    mg_centre(MG_CHOOSER_HINT_ROW, "LEFT / RIGHT TO CHOOSE - A TO CONFIRM", PAL_SKY);
    mg_draw_chooser();
    mg_chooser_tick(t, 0);

    mg.music_on = 0;
    mg_music(SOUND_TRACK_A);
    for (i = 16; i > 0; i = (uint8_t)(i - 2u)) {
        mg_fade_set((uint8_t)(i - 2u));
        waitVbl();
        mg_fade_commit();
        t++;
        mg_chooser_tick(t, 0);
    }

    poll_joystick_edge();
    /* The same Start press that opened this screen is still fresh on a
     * real cabinet's own change-detection; without this pause it could
     * read as an immediate confirm and blow straight through to
     * gameplay, which looked exactly like Start not doing anything. */
    for (;;) {
        waitVbl();
        t++;
        joy = poll_joystick_edge();
        if (joy & (JOY_LEFT | JOY_RIGHT)) {
            mg_chooser_pick = (uint8_t)(mg_chooser_pick ^ 1u);
            mg_draw_chooser();
            playSFX(SOUND_SFX_11);
        }
        mg_chooser_tick(t, 0);
        if (t > 20 && (joy & BUTTON_A)) break;
    }

    /* Her fanfare, her card flashing white and settling, her victory pose. */
    soundStopAll();
    mg.music_on = 0;
    soundSetFMVolume(0x0C);
    playFMTrack(mg_chooser_pick ? SOUND_FM_TRACK_3 : SOUND_FM_TRACK_2);
    playSFX(SOUND_SFX_12);
    mg_grey_banks(mg_chooser_pick ? PAL_PORTRAIT : PAL_PORTRAIT_ALT,
                  mg_chooser_pick ? mg_portrait_pal : mg_portrait_alt_pal, 2);
    for (i = 0; i < 110; i++) {
        waitVbl();
        t++;
        if (i < 17) {
            const uint16_t *src = mg_chooser_pick ? mg_portrait_alt_pal : mg_portrait_pal;
            uint8_t bank = mg_chooser_pick ? PAL_PORTRAIT_ALT : PAL_PORTRAIT;
            mg_whiten_bank(bank, src, (uint8_t)(16u - i));
            mg_whiten_bank((uint8_t)(bank + 1), src + 16, (uint8_t)(16u - i));
        }
        mg_chooser_tick(t, 1);
        poll_joystick_edge();
    }

    ng_sprite_group_set_visible(&mg_chooser_maiya, 0);
    ng_sprite_group_set_visible(&mg_chooser_luna, 0);
    ng_sprite_group_flush(&mg_chooser_maiya);
    ng_sprite_group_flush(&mg_chooser_luna);
    ng_sprite_hide_all();
    ng_fix_clear();
    mg_ui_palettes();
    mg.music_on = 0;
    mg_music(SOUND_TRACK_A);
    mg_show_how_to_play();
    mg_show_intro_story();
}

/* ------------------------------------------------------------------ */
/*  Spawning: Enemies, Allies & Guardians                             */
/* ------------------------------------------------------------------ */
/* How hard the creatures push, by valley: 1 is a stroll, 4 is the citadel. */
static int16_t NEOGEO_USER mg_pace(int16_t base)
{
    int16_t scale = (int16_t)(10 + mg.stage * 2);   /* 10/12/14/16/18/20 */
    return (int16_t)((base * scale) / 16);
}

/* Creatures that live in the air: they hover and swoop instead of falling
 * to the road, so they're spawned without gravity. */
static uint8_t NEOGEO_USER mg_enemy_flies(uint8_t type)
{
    return (uint8_t)(type == MG_E_CROW || type == MG_E_DRONE || type == MG_E_JELLYFISH ||
                     type == MG_E_ACIDMOTH || type == MG_E_SMOGBAT || type == MG_E_POACHDRONE ||
                     type == MG_E_CHEMFLY || type == MG_E_PLASTICBAT || type == MG_E_WRAITH);
}

/* How many hits each kind takes before the valley's own difficulty is added. */
static uint8_t NEOGEO_USER mg_enemy_base_hp(uint8_t type)
{
    switch (type) {
    case MG_E_SLAGGOLEM: return 4;
    case MG_E_BEETLE:    return 3;
    case MG_E_DRONE: case MG_E_TOXICCRAB: case MG_E_SEWERRAT:
    case MG_E_POACHDRONE: case MG_E_VINESTING: return 2;
    default:             return 1;
    }
}

static MGEnemy *NEOGEO_USER mg_spawn_enemy(uint8_t type, int16_t x, int16_t y, uint8_t posted)
{
    uint8_t slot;
    MGEnemy *e;
    uint8_t pal = mg_enemy_palette(type);

    for (slot = 0; slot < MG_ENEMIES; slot++) if (!mg.enemies[slot].body) break;
    if (slot == MG_ENEMIES) return 0;

    /* A flyer placed on the road starts up in the air where it belongs. */
    if (!posted && mg_enemy_flies(type) && y >= MG_GROUND_Y - 4) y = (int16_t)(MG_GROUND_Y - 72);

    e = &mg.enemies[slot];
    e->type = type;
    e->posted = posted;
    e->body = mg_character(K_ENEMY, x, y, pal, NG_RENDER_BAND_ENEMY, type);
    if (!e->body) return 0;

    /*
     * Difficulty is the valley's job, not the creature's.  In the first two
     * missions everything is softer and slower; by the world tree they take
     * a beating and come at her properly.
     */
    e->body->hp = mg_enemy_base_hp(type);
    if (mg.stage >= 2) e->body->hp++;
    if (mg.stage >= 4) e->body->hp++;
    e->body->max_hp = e->body->hp;

    /*
     * Everything walking attaches to gravity. A flyer used to get gravity
     * too, and since its AI only ever set a small up/down bob, every frame
     * the pull won: moths, bats and jellyfish sank to the road and hopped
     * along it. They now integrate their own velocity with no pull.
     */
    if (!posted) {
        ng_physics_attach(e->body, NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS);
        /* No pull, but a real speed limit: a max fall of zero would clamp
         * every downward move to nothing and pin flyers to the sky. */
        if (mg_enemy_flies(type)) ng_physics_set_gravity(e->body, 0, 8 * NG_FP_ONE);
        else ng_physics_set_gravity(e->body, 64, 5 * NG_FP_ONE);
    }
    e->timer = (uint16_t)(slot * 29 + (mg_rand() & 31));
    e->hurt = 0;
    e->home = x;
    e->mood = 0;
    e->move_timer = 0;
    e->heading = 1;
    e->face = (int8_t)((mg.player && mg.player->x < x) ? -1 : 1);
    return e;
}

/* Where an encounter at this X actually stands: on the elevated ledge
 * that runs under it, if the level has one there, or the road below if
 * not. Every encounter used to spawn flat on the road regardless of the
 * platform tiers around it, so the high ground the level draws was
 * never populated -- every fight looked the same no matter how varied
 * the terrain was. Gravity and solid collision already make a spawned
 * enemy land and stand wherever it's dropped, so this only changes
 * where it starts, not how it moves. */
static int16_t NEOGEO_USER mg_ledge_y_at(const MGLevel *level, int16_t x)
{
    uint8_t i;
    for (i = 0; i < MG_PLATFORM_COUNT; i++) {
        const MGPlatform *p = &level->platforms[i];
        if (p->width && x >= p->x && x < (int16_t)(p->x + p->width)) return p->y;
    }
    return MG_GROUND_Y;
}

/*
 * The guardian and its lair: the road's creatures and shots are cleared,
 * the guardian stands at the far end, the arena's cover and backdrop go
 * up. Returns 0 if there was no character slot for it.
 */
static uint8_t NEOGEO_USER mg_boss_arrive(void)
{
    const MGLevel *level = &mg_levels[mg.stage];
    uint8_t i;
    for (i = 0; i < MG_ENEMIES; i++) {
        if (mg.enemies[i].body) ng_chars_remove(mg.enemies[i].body);
        mg.enemies[i].body = 0;
    }
    for (i = 0; i < MG_SHOTS; i++) mg.shots[i].life = 0;
    mg.boss_home = (int16_t)(level->width - 160);
    mg.boss = mg_character(K_BOSS, (int16_t)(level->width - 70), MG_GROUND_Y, PAL_BOSS, NG_RENDER_BAND_ENEMY, level->boss_style);
    if (!mg.boss) return 0;
    mg.boss_active = 1;
    mg.boss_timer = 0;
    mg.boss_rage = 0;
    mg.boss_direction = 0;
    mg.boss_backoff = 0;
    mg.boss_px = 0;          /* the guardian's bar fills in as it appears */
    mg_arena_setup(level->boss_style);
    mg_arena_background(level->boss_style);
    if (level->boss_style == MG_B_OWL || level->boss_style == MG_B_VULTURE) {
        ng_char_set_pos(mg.boss, mg.boss->x, MG_BOSS_SKY_Y);
    }
    mg.boss->hp = mg.boss->max_hp = level->boss_hp;
    ng_physics_attach(mg.boss, NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS);
    if (level->boss_style == MG_B_OWL || level->boss_style == MG_B_VULTURE)
        ng_physics_set_gravity(mg.boss, 0, 8 * NG_FP_ONE);   /* they fight on the wing */
    else
        ng_physics_set_gravity(mg.boss, 56, 6 * NG_FP_ONE);
    mg.boss->vx_fp = 0;
    return 1;
}

/* It speaks, she answers, and the fight's music starts. */
static void NEOGEO_USER mg_boss_announce(void)
{
    const MGLevel *level = &mg_levels[mg.stage];
    playSFX(SOUND_SFX_14); /* boss roar */
    mg.state = MG_BOSS_INTRO;
    mg.state_timer = 210;
    ng_fix_clear_rect(1, ROW_CARD, 38, 9, PAL_TEXT);
    mg_centre(ROW_CARD, level->guardian, PAL_WARN);
    mg_centre(ROW_CARD + 2, mg_boss_taunt[mg.stage], PAL_WARN);
    mg_centre(ROW_CARD + 5, "MAIYA", PAL_GOLD);
    mg_centre(ROW_CARD + 7, mg_boss_reply[mg.stage], PAL_SKY);
    mg_music(SOUND_TRACK_H);
}

static void NEOGEO_USER mg_warp_begin(void);

static void NEOGEO_USER mg_spawn(void)
{
    const MGLevel *level = &mg_levels[mg.stage];
    uint8_t i;
    int16_t px = mg.player->x;

    if (mg.boss_active) return;

    /* Encounter waves */
    for (i = 0; i < MG_ENCOUNTER_COUNT; i++) {
        const MGEncounter *en = &level->encounters[i];
        uint32_t bit = (uint32_t)1u << i;
        if (!en->x || (mg.encounter_mask & bit) || en->x > px + 240) continue;
        if (en->x + 200 < px) { mg.encounter_mask |= bit; continue; }

        if (en->type == MG_E_PAIR) {
            int16_t x2 = (int16_t)(en->x + 40);
            mg_spawn_enemy(MG_E_SLIME, en->x, mg_ledge_y_at(level, en->x), 0);
            mg_spawn_enemy(MG_E_BEETLE, x2, mg_ledge_y_at(level, x2), 0);
        } else {
            mg_spawn_enemy(en->type, en->x, mg_ledge_y_at(level, en->x), 0);
        }
        mg.encounter_mask |= bit;
    }

    /* Posted throwers / drones on ledges */
    for (i = 0; i < MG_ARCHER_COUNT; i++) {
        const MGArcher *a = &level->archers[i];
        uint16_t bit = (uint16_t)(1u << i);
        if (!a->x || (mg.archer_mask & bit) || a->x > px + 300 || a->x + 300 < px) continue;
        /* No drones until the coast: the early shelves hold goblins, and
         * the first valley posts nothing at all. */
        if (mg.stage == 0) { mg.archer_mask |= bit; continue; }
        if (mg_spawn_enemy((uint8_t)(mg.stage < 3 ? MG_E_GOBLIN : MG_E_DRONE),
                           a->x, a->y, 1)) mg.archer_mask |= bit;
    }

    /* Coins, flowers, charms and the hidden life, handed out as she nears them. */
    for (i = 0; i < MG_PICK_COUNT; i++) {
        const MGPickup *pk = &mg_picks[mg.stage][i];
        uint16_t bit = (uint16_t)(1u << i);
        MGItem *item;
        if (!pk->x || (mg.pick_mask & bit)) continue;
        if (pk->kind == MG_K_LIFE && mg.life_pickups_used >= MG_LIFE_PICKUP_LIMIT) continue;
        if (mg_abs((int16_t)(pk->x - px)) > 220) continue;
        if (mg_pickup_active((uint8_t)(i + 1))) continue;
        item = mg_drop_trinket(pk->x, (int16_t)pk->y, pk->kind);
        if (item) item->source = (uint8_t)(i + 1);
    }
    for (i = 0; i < MG_SECRET_COUNT; i++) {
        const MGSecret *s = &level->secrets[i];
        uint8_t source = (uint8_t)(MG_PICK_COUNT + i + 1);
        MGItem *item;
        if (!s->x || (mg.secret_mask & (1u << i))) continue;
        if (mg_abs((int16_t)(s->x - px)) > 220 || mg_pickup_active(source)) continue;
        item = mg_drop(s->x, s->y, s->type == 1 ? MG_I_SEED : MG_I_ROSE_GOLD);
        if (item) item->source = source;
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
                    mg_palette(PAL_ALLY, mg_ally_pal(ally_type));
                    mg.rescue->data0 = ally_type;
                    mg.rescue->data1 = i;
                    mg_frame(mg.rescue, 0, 0);
                    ng_sprite_group_set_pos(&mg.cage, (int16_t)(mg.rescue->x - 16), (int16_t)(mg.rescue->y - 32));
                    ng_sprite_group_set_visible(&mg.cage, 1);
                }
                break;
            }
        }
    }

    /* The guardian only shows itself once the gate is open. Walking into
     * the open gate carries her to its lair (mg_warp_begin); a valley with
     * no gate opens the arena at the end of the road instead. */
    if (!mg.boss_active && mg.gate_unlocked && mg.state == MG_PLAY) {
        if (level->gate_x) {
            if (px >= (int16_t)level->gate_x + 8) mg_warp_begin();
        } else if (px > mg.arena_left + 32 && mg_boss_arrive()) {
            mg_boss_announce();
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

/*
 * One throw from her hand, at `high` pixels above her feet. A special
 * weapon, while it has ammunition, goes first; otherwise it's her own
 * thorn, doubled while a sheaf lasts. The thorn crown sends it out bigger
 * and faster. Returns 0 only when too many are already in the air.
 */
static uint8_t NEOGEO_USER mg_throw(int16_t high)
{
    NGCharacter *p = mg.player;
    int16_t dir = (int16_t)(mg.facing ? -1 : 1);
    int16_t y = (int16_t)(p->y + high);
    MGShot *s;

    if (mg.weapon && mg.weapon_ammo) {
        if (mg_shots_in_flight() >= 4) return 0;
        if (mg.weapon == MG_W_SPREAD) {
            mg_fire(p->x, y, (int16_t)(dir * 6), -2, 0, MG_T_PETAL);
            mg_fire(p->x, y, (int16_t)(dir * 7), 0, 0, MG_T_PETAL);
            mg_fire(p->x, y, (int16_t)(dir * 6), 2, 0, MG_T_PETAL);
        } else if (mg.weapon == MG_W_PIERCE) {
            s = mg_fire(p->x, y, (int16_t)(dir * 10), 0, 0, MG_T_STAR);
            if (s) { s->mode = MG_SHOT_PIERCE; s->life = 60; }
        } else {
            /* Only one wind leaf out at a time: it has to come home. */
            uint8_t i;
            for (i = 0; i < MG_SHOTS; i++)
                if (mg.shots[i].life && mg.shots[i].mode == MG_SHOT_GALE) return 0;
            s = mg_fire(p->x, y, (int16_t)(dir * 8), 0, 0, MG_T_LEAF);
            if (s) { s->mode = MG_SHOT_GALE; s->life = 110; }
        }
        if (--mg.weapon_ammo == 0) mg.weapon = MG_W_NONE;
    } else {
        /* Her own thorn throw never runs out. A sheaf of thorns amplifies
         * it: while the count lasts, every throw sends two. */
        int16_t rate = (int16_t)(mg.crown ? 9 : 6);
        uint8_t kind = (uint8_t)(mg.crown ? MG_T_THORN1 : MG_T_THORN0);
        if (mg_shots_in_flight() >= (mg.thorns ? 5 : (mg.crown ? 4 : 3))) return 0;
        if (mg.thorns) {
            mg_fire(p->x, (int16_t)(y - 5), (int16_t)(dir * rate), 0, 0, kind);
            mg_fire(p->x, (int16_t)(y + 5), (int16_t)(dir * rate), 0, 0, kind);
            mg.thorns--;
        } else {
            mg_fire(p->x, y, (int16_t)(dir * rate), 0, 0, kind);
        }
    }
    mg.cast = 10;
    mg.hud_dirty = 1;
    playSFX(SOUND_SFX_2); /* thorn toss */
    return 1;
}

static void NEOGEO_USER mg_controls(void)
{
    uint16_t joy = mg_input();
    uint16_t pressed = (uint16_t)(joy & (uint16_t)(~mg.previous_joy));
    NGCharacter *p = mg.player;
    int16_t vx = 0;

    if (mg.climb_cooldown) mg.climb_cooldown--;
    if (mg.dash_wait) mg.dash_wait--;
    if (mg.dash) mg.dash--;
    if (mg.hurt > HURT_LOCK || (mg.state != MG_PLAY && mg.state != MG_BONUS)) {
        /* A knock eases off over a few frames instead of sliding her
         * seventy pixels at full speed for the whole stagger. */
        if (mg.hurt > HURT_LOCK) p->vx_fp -= p->vx_fp / 6;
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
            /* She can turn on the vine to throw the other way. */
            if (joy & JOY_LEFT) mg.facing = 1;
            else if (joy & JOY_RIGHT) mg.facing = 0;
            if (pressed & BUTTON_A) {
                mg_climb_end();
                p->vy_fp = -JUMP_SPEED;
                p->vx_fp = mg.facing ? -WALK_SPEED : WALK_SPEED;
                mg.airborne = 1;
                playSFX(SOUND_SFX_15);
            }
            if (pressed & BUTTON_B) mg_throw(-26);
        }
        mg.previous_joy = joy;
        return;
    }

    {
        const MGVine *v = mg_vine_at(p->x, p->y);
        uint8_t can_climb = v && (((joy & JOY_UP) && p->y > v->top) ||
                                  ((joy & JOY_DOWN) && p->y < v->bottom));
        if (!mg.climb_cooldown && mg.state == MG_PLAY && !mg.boss_active && can_climb) {
            /* Keep the hand anchor on the same vine through every pose. */
            ng_char_set_pos(p, (int16_t)(v->x + 16), p->y);
            mg.climbing = 1;
            mg.crouch_timer = 0;
            mg.drop = 0;
            p->vy_fp = 0;
            p->vx_fp = 0;
            ng_physics_set_gravity(p, 0, 0);
            playSFX(SOUND_SFX_11);
            mg.previous_joy = joy;
            return;
        }
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
            mg.shake = 20;
            mg_hint("THE GATE OPENS. THE GUARDIAN WAITS", PAL_GOLD, 150);
            mg.previous_joy = joy;
            return;
        }
    }

    mg_hazard_disable_check(pressed);

    /* Track joystick motions for combo super move (Down, Forward + B) */
    if (mg.combo_timer && --mg.combo_timer == 0) mg.combo_buffer[0] = mg.combo_buffer[1] = 0;
    if (pressed & JOY_DOWN) { mg.combo_buffer[0] = 1; mg.combo_timer = 24; }
    if ((pressed & (JOY_LEFT | JOY_RIGHT)) && mg.combo_buffer[0]) mg.combo_buffer[1] = 1;

    /*
     * Walk and run.  She leans into a step and coasts out of it instead of
     * snapping between nought and full speed, which is what made her look
     * like a sprite being dragged rather than a girl running.
     */
    {
        int16_t speed = (int16_t)(mg.swift ? (WALK_SPEED * 3) / 2 : WALK_SPEED);
        int16_t want = 0;
        int16_t have = (int16_t)p->vx_fp;
        /* On ice she's slow to get going and slower to stop. */
        uint8_t slick = (uint8_t)(mg_mech() == MG_M_ICE && !mg.on_ledge && !mg.airborne);
        int16_t accel = (int16_t)(slick ? 36 : WALK_ACCEL);
        int16_t brake = (int16_t)(slick ? 14 : WALK_BRAKE);

        if (joy & JOY_LEFT) {
            want = (int16_t)-speed;
            mg.facing = 1;
        } else if (joy & JOY_RIGHT) {
            want = speed;
            mg.facing = 0;
        }

        if (want > have) {
            have = (int16_t)(have + (want > 0 ? accel : brake));
            if (have > want) have = want;
        } else if (want < have) {
            have = (int16_t)(have - (want < 0 ? accel : brake));
            if (have < want) have = want;
        }
        vx = have;
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
        } else if (ng_physics_is_grounded(p) || mg.on_ledge || p->y >= MG_GROUND_Y - 4) {
            p->vy_fp = mg.spring ? -((JUMP_SPEED * 5) / 4) : -JUMP_SPEED;
            if (mg_mech() == MG_M_WATER) p->vy_fp = (p->vy_fp * 3) / 4;
            mg.airborne = 1;
            playSFX(SOUND_SFX_15);
        }
    }
    /* Let go of A on the way up and the jump is cut short: a tap hops, a
     * hold clears the shelf.  This is most of what makes a jump feel meant. */
    if (!(joy & BUTTON_A) && p->vy_fp < -(2 * NG_FP_ONE)) {
        p->vy_fp = -(2 * NG_FP_ONE);
    }

    /* Attack (B button) */
    if (pressed & BUTTON_B) {
        /* Check special combo: Down -> Forward + B = Rose Blossom Surge */
        if (mg.combo_buffer[0] && mg.combo_buffer[1] && !mg.super_surge && !mg.dash_wait) {
            mg.super_surge = MG_SURGE_TIME;
            mg.surge_struck_boss = 0;
            mg.combo_buffer[0] = mg.combo_buffer[1] = 0;
            mg.dash = 0;
            playSFX(SOUND_SFX_13);   /* the bloom gathers */
        } else {
            /* Check melee distance to nearest enemy or boss */
            uint8_t melee = 0;
            if (mg.boss && mg_abs((int16_t)(mg.boss->x - p->x)) < 48 &&
                mg_abs((int16_t)(mg.boss->y - p->y)) < 52) {
                mg_boss_damage(mg_strike());
                melee = 1;
            }
            if (!melee) {
                uint8_t i;
                for (i = 0; i < MG_ENEMIES; i++) {
                    NGCharacter *e = mg.enemies[i].body;
                    if (!e || mg.enemies[i].mood == MG_MOOD_DYING ||
                        mg_abs((int16_t)(e->x - p->x)) >= 44) continue;
                    /* Standing, she cuts at chest height; kneeling, at the
                     * ground, where the low creatures actually are. */
                    if (mg.crouch_timer) {
                        if (e->y < p->y - 26) continue;
                    } else if (e->y < p->y - 52) {
                        continue;
                    }
                    mg_enemy_damage(&mg.enemies[i], mg_strike());
                    melee = 1;
                    break;
                }
            }
            /* Kneeling, the throw skims the road -- the only way to reach a
             * slime. Up close it's the whip; at range, the thorn. */
            if (melee) {
                mg.attack = 12;
                playSFX(SOUND_SFX_1); /* whip crack */
            } else {
                mg_throw((int16_t)(mg.crouch_timer ? -8 : -26));
            }
        }
    }

    /* Secret Art (D button) */
    if (pressed & BUTTON_D) {
        mg_secret_art();
    }

    /*
     * The dash: a kick of dust where she pushes off, a trail of it behind
     * her, and the last few frames easing back towards a run instead of
     * snapping to a stop. Jumping out of it keeps the speed.
     */
    if ((pressed & BUTTON_C) && !mg.dash_wait && !mg.super_surge &&
        (ng_physics_is_grounded(p) || mg.on_ledge)) {
        mg.dash = MG_DASH_TIME;
        mg.dash_wait = 30;
        mg_burst((int16_t)(p->x + (mg.facing ? 10 : -10)), (int16_t)(p->y - 4), MG_T_DUST, 2, -1);
        playSFX(SOUND_SFX_15);
    }
    if (mg.dash) {
        int16_t speed = (int16_t)(mg.dash > 4 ? DASH_SPEED
                                  : WALK_SPEED + ((DASH_SPEED - WALK_SPEED) * mg.dash) / 5);
        vx = mg.facing ? (int16_t)-speed : speed;
        if ((mg.dash % 4) == 0 && !mg.airborne)
            mg_burst((int16_t)(p->x + (mg.facing ? 12 : -12)), (int16_t)(p->y - 18),
                     (uint8_t)((mg.dash & 4) ? MG_T_PETAL : MG_T_LEAF), 1, -1);
    }

    /*
     * The Rose Blossom Surge. A beat of stillness while the bloom gathers
     * around her, then she spins through the road trailing petals: every
     * creature in her path is struck, the guardian once, and she can't be
     * touched until it's over.
     */
    if (mg.super_surge) {
        mg.super_surge--;
        if (mg.super_surge >= MG_SURGE_TIME - MG_SURGE_WINDUP) {
            vx = 0;
            if (mg.super_surge == MG_SURGE_TIME - MG_SURGE_WINDUP) {
                mg.shake = 6;
                playSFX(SOUND_SFX_15);
                playSFX(SOUND_SFX_1);
            } else if ((mg.super_surge & 1) == 0) {
                mg_burst(p->x, (int16_t)(p->y - 30), MG_T_PETAL, 2, -2);
            }
        } else {
            uint8_t i;
            vx = (int16_t)(mg.facing ? -DASH_SPEED * 3 / 2 : DASH_SPEED * 3 / 2);
            if ((mg.super_surge % 3) == 0)
                mg_burst((int16_t)(p->x + (mg.facing ? 14 : -14)), (int16_t)(p->y - 24),
                         (uint8_t)((mg.super_surge & 4) ? MG_T_PETAL : MG_T_LEAF), 1, -1);
            for (i = 0; i < MG_ENEMIES; i++) {
                NGCharacter *e = mg.enemies[i].body;
                if (e && mg_abs((int16_t)(e->x - p->x)) < 40 &&
                    mg_abs((int16_t)(e->y - p->y)) < 56)
                    mg_enemy_damage(&mg.enemies[i], 3);
            }
            if (!mg.surge_struck_boss && mg.boss &&
                mg_abs((int16_t)(mg.boss->x - p->x)) < 52) {
                mg_boss_damage(4);
                mg.surge_struck_boss = 1;
            }
            if (mg.super_surge == 0) mg.dash_wait = 60;
        }
    }

    p->vx_fp = vx;
    mg.previous_joy = joy;
}

/*
 * Each guardian's arena: two stands of cover laid out for how that
 * guardian fights -- high ground to clear a pounce, low stumps to duck a
 * charge, ledges to get above a slithering wyrm. {x offset, y, width}.
 */
static const int16_t mg_arena_layout[10][2][3] = {
    /* BEETLE    */ {{ 40, 144, 64}, {216, 144, 64}},
    /* TOAD      */ {{ 56, 120, 64}, {200, 120, 64}},
    /* LEVIATHAN */ {{ 24, 112, 96}, {232, 144, 64}},
    /* JACKAL    */ {{ 48, 104, 64}, {208, 104, 64}},
    /* OWL       */ {{ 32, 136, 64}, {224, 104, 64}},
    /* SMOGGAR   */ {{ 24, 144, 64}, {232, 144, 64}},
    /* VULTURE   */ {{ 96, 120, 128}, {  0,   0,  0}},
    /* EEL       */ {{ 40, 140, 64}, {200, 116, 64}},
    /* WYRM      */ {{ 24, 116, 64}, {232, 116, 64}},
    /* HYENA     */ {{ 64, 136, 64}, {192, 112, 64}},
};

static void NEOGEO_USER mg_arena_setup(uint8_t style)
{
    uint8_t j;
    if (style > 9) style = 5;
    for (j = 0; j < 2; j++) {
        mg.arena[j].x = (int16_t)(mg.arena_left + mg_arena_layout[style][j][0]);
        mg.arena[j].y = mg_arena_layout[style][j][1];
        mg.arena[j].width = mg_arena_layout[style][j][2];
    }
}

/* A lob: rises, then falls under its own weight and bursts on the road. */
static void NEOGEO_USER mg_lob(int16_t x, int16_t y, int16_t vx, int16_t vy, uint8_t kind)
{
    MGShot *s = mg_fire(x, y, vx, vy, 1, kind);
    if (s) { s->mode = MG_SHOT_ARC; s->life = 120; }
}

/* A lob that lands near where she stands now. */
static void NEOGEO_USER mg_lob_at(NGCharacter *b, NGCharacter *p, int16_t y_off, uint8_t kind)
{
    int16_t vx = (int16_t)((p->x - b->x) / 36);
    if (vx > 5) vx = 5;
    if (vx < -5) vx = -5;
    mg_lob(b->x, (int16_t)(b->y + y_off), vx, -5, kind);
}

static void NEOGEO_USER mg_boss_hover(NGCharacter *b, int16_t tx, int16_t ty, int16_t speed)
{
    b->vx_fp = (b->x < tx - 6) ? speed : ((b->x > tx + 6) ? -speed : 0);
    b->vy_fp = (b->y < ty - 4) ? speed / 2 : ((b->y > ty + 4) ? -speed / 2 : 0);
}

/*
 * The guardians. Each has a cycle of its own moves built around one
 * signature attack, always telegraphed before it lands and followed by a
 * window to strike back. Below half health a guardian is enraged: the
 * cycle runs faster and its signature attack comes with an extra beat.
 */
static void NEOGEO_USER mg_boss_ai(NGCharacter *p)
{
    NGCharacter *b = mg.boss;
    int16_t b_dx = (int16_t)(p->x - b->x);
    int8_t dir = (int8_t)(b_dx < 0 ? -1 : 1);
    uint8_t style = (uint8_t)b->data0;
    uint8_t rage = mg.boss_rage;
    uint8_t frame = 0xFFu;   /* 0xFF: pick idle, walk or jump from how it moves */
    uint8_t face = (uint8_t)(b_dx < 0);
    uint8_t harmless = 0;          /* its recovery beat: touching it doesn't hurt */
    uint16_t t;

    if (mg.boss_hurt) mg.boss_hurt--;
    if (!rage && b->hp * 2 <= b->max_hp) {
        mg.boss_rage = rage = 1;
        mg.shake = 12;
        playSFX(SOUND_SFX_14);
        mg_hint("THE GUARDIAN IS ENRAGED", PAL_WARN, 90);
    }
    /*
     * Near the end it falters: its colour pulses dim, it moves at half
     * pace, and now and then it stops to pant, sweating -- the opening to
     * finish it. Touching it while it pants doesn't hurt.
     */
    if (b->hp * 4 <= b->max_hp) {
        if (rage == 1) {
            mg.boss_rage = rage = 2;
            mg_hint("THE GUARDIAN IS FALTERING", PAL_GOLD, 90);
        }
        if ((mg.tick & 7) == 0)
            mg_shade_bank(PAL_BOSS, mg_boss_pal(style), (uint8_t)((mg.tick & 8) ? 4 : 2), 0);
        if ((mg.tick % 150u) < 40u) {
            b->vx_fp = 0;
            if (style == MG_B_OWL || style == MG_B_VULTURE) b->vy_fp = (b->y < MG_BOSS_SKY_Y + 20) ? 60 : 0;
            if ((mg.tick % 12u) == 0u) mg_burst(b->x, (int16_t)(b->y - 84), MG_T_DRIP, 1, -1);
            mg_frame(b, (uint8_t)((mg.tick & 32) ? MG_BF_HURT : MG_BF_IDLE), face);
            return;
        }
    }
    mg.boss_timer = (uint16_t)(mg.boss_timer + (rage == 2 ? (mg.tick & 1) : 1 + (rage && (mg.tick & 1))));

    switch (style) {
    case MG_B_BEETLE:
        t = (uint16_t)(mg.boss_timer % 200u);
        if (t < 40) { b->vx_fp = dir * 180; }
        else if (t < 64) {
            b->vx_fp = 0; frame = MG_BF_WINDUP;
            if (t == 40) { mg.boss_direction = dir; mg_hint("THE SAW REVS - GET CLEAR", PAL_WARN, 30); }
        } else if (t < 100) { b->vx_fp = mg.boss_direction * 760; frame = MG_BF_ATTACK; face = (uint8_t)(mg.boss_direction < 0); }
        else if (t < 124) {
            b->vx_fp = 0; harmless = 1;
            if (rage && t == 110) mg.boss_direction = (int16_t)-mg.boss_direction;
        } else if (rage && t < 160) { b->vx_fp = mg.boss_direction * 760; frame = MG_BF_ATTACK; face = (uint8_t)(mg.boss_direction < 0); }
        else { b->vx_fp = 0; harmless = 1; }
        break;

    case MG_B_TOAD: {
        uint8_t hops = (uint8_t)(rage ? 4 : 3);
        t = (uint16_t)(mg.boss_timer % 200u);
        if (t < hops * 40u) {
            uint8_t k = (uint8_t)(t % 40u);
            if (k == 0) { b->vy_fp = -5 * NG_FP_ONE - 128; b->vx_fp = dir * 320; playSFX(SOUND_SFX_15); }
            if (k == 30) {
                mg.shake = 5; b->vx_fp = 0; frame = MG_BF_SPECIAL;
                mg_lob(b->x, (int16_t)(b->y - 40), (int16_t)(-3), -4, MG_T_SPIT);
                mg_lob(b->x, (int16_t)(b->y - 40), 3, -4, MG_T_SPIT);
                if (rage) mg_lob_at(b, p, -40, MG_T_SPIT);
                playSFX(SOUND_SFX_5);
            }
        } else { b->vx_fp = 0; harmless = 1; }
        break;
    }

    case MG_B_LEVIATHAN: case MG_B_EEL: {
        uint8_t shot = (uint8_t)(style == MG_B_EEL ? MG_T_SPIT : MG_T_OIL);
        t = (uint16_t)(mg.boss_timer % 220u);
        if (t < 80) {
            if (b->x < mg.arena_left + 40) mg.boss_direction = 1;
            else if (b->x > mg.arena_left + 280) mg.boss_direction = -1;
            else if (!mg.boss_direction) mg.boss_direction = dir;
            b->vx_fp = mg.boss_direction * 220; face = (uint8_t)(mg.boss_direction < 0);
        } else if (t < 100) {
            b->vx_fp = 0; frame = MG_BF_WINDUP;
            if (t == 80) mg_hint("IT REARS UP", PAL_WARN, 24);
        } else if (t >= 100 && t <= 120) {
            frame = MG_BF_SPECIAL;
            b->vx_fp = 0;
            if (t == 100 || (rage && t == 112)) {
                mg_fire(b->x, (int16_t)(b->y - 50), (int16_t)(dir * 4), -1, 1, shot);
                mg_fire(b->x, (int16_t)(b->y - 50), (int16_t)(dir * 4), 0, 1, shot);
                mg_fire(b->x, (int16_t)(b->y - 50), (int16_t)(dir * 4), 1, 1, shot);
                playSFX(SOUND_SFX_8);
            }
        } else if (t > 120 && t < 160) {
            if (t == 121) mg.boss_direction = dir;
            b->vx_fp = mg.boss_direction * 700; frame = MG_BF_ATTACK; face = (uint8_t)(mg.boss_direction < 0);
        } else if (t >= 160) { b->vx_fp = 0; harmless = 1; }
        break;
    }

    case MG_B_JACKAL: case MG_B_HYENA:
        t = (uint16_t)(mg.boss_timer % 200u);
        if (t < 135) {
            uint8_t k = (uint8_t)(t % 45u);
            if (k == 0) {
                /* a pounce aimed at where she is right now */
                b->vy_fp = -5 * NG_FP_ONE; b->vx_fp = dir * 540; frame = MG_BF_ATTACK;
                playSFX(SOUND_SFX_14);
            } else if (k > 30) { b->vx_fp = 0; }
        } else if (t == 140) {
            frame = MG_BF_SPECIAL;
            if (style == MG_B_JACKAL) {
                mg_fire(b->x, (int16_t)(b->y - 20), -5, 0, 1, MG_T_FIRE);
                mg_fire(b->x, (int16_t)(b->y - 20), 5, 0, 1, MG_T_FIRE);
                if (rage) mg_lob_at(b, p, -40, MG_T_FIRE);
            } else {
                mg_lob_at(b, p, -30, MG_T_SPIT);
                if (rage) mg_lob(b->x, (int16_t)(b->y - 30), (int16_t)(dir * 2), -6, MG_T_SPIT);
            }
            playSFX(SOUND_SFX_8);
        } else if (t > 150) { b->vx_fp = dir * 120; harmless = (uint8_t)(t < 175); }
        break;

    case MG_B_OWL:
        t = (uint16_t)(mg.boss_timer % 240u);
        if (t < 180) {
            mg_boss_hover(b, (int16_t)(p->x - dir * 24), MG_BOSS_SKY_Y, 260);
            if ((t % (rage ? 40u : 60u)) == 30u) {
                mg_fire(b->x, (int16_t)(b->y + 10), 0, 3, 1, MG_T_ICE);
                if (rage) {
                    mg_fire((int16_t)(b->x - 24), (int16_t)(b->y + 10), 0, 3, 1, MG_T_ICE);
                    mg_fire((int16_t)(b->x + 24), (int16_t)(b->y + 10), 0, 3, 1, MG_T_ICE);
                }
                playSFX(SOUND_SFX_8);
            }
        } else if (t < 186) {
            b->vx_fp = 0; b->vy_fp = 0; frame = MG_BF_WINDUP;
            if (t == 180) { mg.boss_direction = dir; mg_hint("IT FOLDS ITS WINGS", PAL_WARN, 20); }
        } else if (t < 214) {
            b->vx_fp = mg.boss_direction * 420; b->vy_fp = 520; frame = MG_BF_ATTACK;
            if (b->y >= MG_GROUND_Y - 4) b->vy_fp = 0;
        } else { mg_boss_hover(b, b->x, MG_BOSS_SKY_Y, 300); harmless = 1; }
        break;

    case MG_B_VULTURE:
        t = (uint16_t)(mg.boss_timer % 220u);
        if (t < 160) {
            if (b->x < mg.arena_left + 40) mg.boss_direction = 1;
            else if (b->x > mg.arena_left + 280) mg.boss_direction = -1;
            else if (!mg.boss_direction) mg.boss_direction = 1;
            b->vx_fp = mg.boss_direction * 300;
            b->vy_fp = (b->y < MG_BOSS_SKY_Y - 4) ? 120 : ((b->y > MG_BOSS_SKY_Y + 4) ? -120 : 0);
            face = (uint8_t)(mg.boss_direction < 0);
            if ((t % (rage ? 32u : 50u)) == 20u && mg_abs(b_dx) < 60) {
                mg_fire(b->x, (int16_t)(b->y + 16), 0, 3, 1, MG_T_BOLT);
                playSFX(SOUND_SFX_6);
            }
        } else if (t < 168) {
            b->vx_fp = 0; b->vy_fp = 0; frame = MG_BF_WINDUP;
            if (t == 160) { mg.boss_direction = dir; mg_hint("IT DIVES", PAL_WARN, 20); }
        } else if (t < 196) {
            b->vx_fp = mg.boss_direction * 620; b->vy_fp = 600; frame = MG_BF_ATTACK;
            face = (uint8_t)(mg.boss_direction < 0);
            if (b->y >= MG_GROUND_Y - 4) b->vy_fp = 0;
        } else {
            b->vx_fp = mg.boss_direction * 200;
            b->vy_fp = (b->y > MG_BOSS_SKY_Y) ? -400 : 0;
            harmless = 1;
        }
        break;

    case MG_B_WYRM:
        t = (uint16_t)(mg.boss_timer % 220u);
        if (t < 40) {
            b->vx_fp = 0; frame = MG_BF_SPECIAL;
            if (t == 20 || (rage && t == 34)) {
                /* the sludge fountain: three lobs, fanned */
                mg_lob(b->x, (int16_t)(b->y - 60), -2, -6, MG_T_SPIT);
                mg_lob(b->x, (int16_t)(b->y - 60), 0, -7, MG_T_SPIT);
                mg_lob(b->x, (int16_t)(b->y - 60), 2, -6, MG_T_SPIT);
                mg.shake = 4;
                playSFX(SOUND_SFX_5);
            }
        } else if (t < 100) { b->vx_fp = dir * (rage ? 640 : 520); }
        else if (t < 116) { b->vx_fp = 0; }
        else if (t == 116) { b->vy_fp = -4 * NG_FP_ONE; b->vx_fp = dir * 400; frame = MG_BF_ATTACK; playSFX(SOUND_SFX_14); }
        else if (t > 150) { b->vx_fp = 0; harmless = 1; }
        break;

    default: /* MG_B_SMOGGAR */
        t = (uint16_t)(mg.boss_timer % 240u);
        if (t < 60) { b->vx_fp = dir * 140; }
        else if (t < 130) {
            b->vx_fp = 0;
            if (t == 60 || t == 90 || t == 120 || (rage && t == 105)) {
                frame = MG_BF_ATTACK; mg_lob_at(b, p, -60, MG_T_TRASH); playSFX(SOUND_SFX_5);
            }
        } else if (t < 190) {
            b->vx_fp = 0;
            if (t == 150 || t == 162 || t == 174) {
                frame = MG_BF_SPECIAL; mg_fire(b->x, (int16_t)(b->y - 48), (int16_t)(dir * 5), 0, 1, MG_T_OIL);
                playSFX(SOUND_SFX_8);
            }
        } else if (t < 204) {
            b->vx_fp = 0;
            if (t == 190) { mg.boss_direction = dir; mg_hint("GUARDIAN CHARGING - TAKE COVER", PAL_WARN, 30); }
        } else if (t < 232) { b->vx_fp = mg.boss_direction * 640; face = (uint8_t)(mg.boss_direction < 0); }
        else { b->vx_fp = 0; harmless = 1; }
        break;
    }

    /* Grounded guardians stay in their arena. */
    if (b->x < mg.arena_left + 16 && b->vx_fp < 0) b->vx_fp = 0;
    if (b->x > mg.arena_left + 304 && b->vx_fp > 0) b->vx_fp = 0;

    /* Hit: the recoil pose. Otherwise, where no move chose a pose, the way
     * it's moving does: in the air it's jumping (or, for the fliers,
     * beating its wings); on the ground it steps or stands. */
    if (mg.boss_hurt > 6) {
        frame = MG_BF_HURT;
    } else if (frame == 0xFFu) {
        uint8_t flies = (uint8_t)(style == MG_B_OWL || style == MG_B_VULTURE);
        if (flies) frame = (uint8_t)(((mg.boss_timer / 10) & 1) ? MG_BF_WALK : MG_BF_IDLE);
        else if (b->vy_fp < -64 || b->y < MG_GROUND_Y - 8) frame = MG_BF_JUMP;
        else if (b->vx_fp) frame = (uint8_t)(((mg.boss_timer / 10) & 1) ? MG_BF_WALK : MG_BF_IDLE);
        else frame = MG_BF_IDLE;
    }
    mg_frame(b, frame, face);

    /*
     * Having struck her, a guardian gives ground for a moment -- steps back
     * and can't hurt her -- so a fight pinned in a corner opens up again
     * instead of one touch following another.
     */
    if (mg.boss_backoff) {
        mg.boss_backoff--;
        harmless = 1;
        if (style != MG_B_OWL && style != MG_B_VULTURE)
            b->vx_fp = (b->x < p->x) ? -384 : 384;
    }

    /*
     * Touching it hurts, and knocks her clear with a little hop. She is
     * never pushed out of it: winded, or dashing, she passes straight
     * through -- the way out of a corner -- and in the air only its body
     * counts, so a jump can clear it.
     */
    {
        int16_t reach = (mg.airborne || !ng_physics_is_grounded(p)) ? MG_BOSS_TOUCH_AIR : MG_BOSS_TOUCH;
        if (!harmless && !mg.boss_hurt && mg_abs((int16_t)(b->x - p->x)) < reach &&
            mg_abs((int16_t)(b->y - p->y)) < 36) {
            mg_player_hit(b->x);
            if (mg.state == MG_PLAY && mg.hurt == 90) {
                if (ng_physics_is_grounded(p)) p->vy_fp = -2 * NG_FP_ONE;
                mg.boss_backoff = MG_BOSS_BACKOFF;
            }
        }
    }
}

/*
 * Her invulnerability flash: her own colours, whichever heroine she is,
 * lifted a few steps -- a glint, not a change of costume. (The golden sun
 * palette recoloured every pixel one hue, turning her into a gold cut-out.)
 */
static void NEOGEO_USER mg_hurt_red_palette(void)
{
    const uint16_t *src = mg_hero_normal_pal();
    uint16_t pal[16];
    uint8_t i;
    pal[0] = src[0];
    for (i = 1; i < 16; i++) {
        uint16_t c = src[i];
        uint8_t r = (uint8_t)((c >> 8) & 15u), g = (uint8_t)((c >> 4) & 15u), b = (uint8_t)(c & 15u);
        r = (uint8_t)(r + ((15u - r) >> 1));
        pal[i] = (uint16_t)(((uint16_t)r << 8) | ((uint16_t)(g >> 1) << 4) | (uint16_t)(b >> 1));
    }
    mg_palette(PAL_HERO, pal);
}

static void NEOGEO_USER mg_hurt_palette(void)
{
    const uint16_t *src = mg_hero_normal_pal();
    uint16_t pal[16];
    uint8_t i;
    pal[0] = src[0];
    for (i = 1; i < 16; i++) {
        uint16_t c = src[i];
        uint8_t r = (uint8_t)((c >> 8) & 15u), g = (uint8_t)((c >> 4) & 15u), b = (uint8_t)(c & 15u);
        r = (uint8_t)(r + 3u > 15u ? 15u : r + 3u);
        g = (uint8_t)(g + 3u > 15u ? 15u : g + 3u);
        b = (uint8_t)(b + 3u > 15u ? 15u : b + 3u);
        pal[i] = (uint16_t)((c & 0x7000u) | ((uint16_t)r << 8) | ((uint16_t)g << 4) | b);
    }
    mg_palette(PAL_HERO, pal);
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
        /* The wind leaf is always being pulled home: it slows, turns, and
         * comes back to her hand, cutting anything in its path both ways. */
        if (s->mode == MG_SHOT_GALE) {
            if ((s->life % 3) == 0) {
                if (p->x > s->x && s->vx < 8) s->vx++;
                else if (p->x < s->x && s->vx > -8) s->vx--;
                s->vy = (int16_t)(((p->y - 26) > s->y) ? 1 : (((p->y - 26) < s->y) ? -1 : 0));
            }
            if (s->life < 96 && mg_abs((int16_t)(p->x - s->x)) < 14 &&
                mg_abs((int16_t)((p->y - 26) - s->y)) < 28) {
                s->life = 1;   /* caught */
            }
        }
        /* A lob falls under its own weight and bursts where it lands. */
        if (s->mode == MG_SHOT_ARC) {
            if ((s->life & 3) == 0 && s->vy < 6) s->vy++;
            if (s->vy > 0 && s->y >= MG_GROUND_Y - 6) {
                mg_burst(s->x, (int16_t)(MG_GROUND_Y - 8), MG_T_DRIP, 2, -2);
                s->life = 1;
            }
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
        if (s->hostile && mg.boss_active) {
            for (j = 0; j < 2; j++) {
                const MGPlatform *cover = &mg.arena[j];
                if (!cover->width) continue;
                if (s->x + 8 >= cover->x && s->x <= cover->x + cover->width &&
                    s->y + 8 >= cover->y && s->y < cover->y + 32) {
                    s->life = 0;
                    ng_sprite_group_set_visible(&s->sprite, 0);
                    ng_sprite_group_flush(&s->sprite);
                    break;
                }
            }
            if (!s->life) continue;
        }
        if (!s->hostile) {
            /* A seed or a wind leaf passes straight through; anything else
             * stops at the first thing it hits. The target's own hurt timer
             * keeps a passing shot from striking the same creature twice. */
            uint8_t through = (uint8_t)(s->mode != MG_SHOT_PLAIN);
            uint8_t power = (uint8_t)((mg.might ? 2 : 1) + (s->mode == MG_SHOT_PIERCE ? 1 : 0));
            for (j = 0; j < MG_ENEMIES; j++) {
                MGEnemy *e = &mg.enemies[j];
                if (e->body && e->mood != MG_MOOD_DYING &&
                    mg_abs((int16_t)(e->body->x - s->x)) < 24 &&
                    mg_abs((int16_t)(e->body->y - s->y)) < 32) {
                    mg_enemy_damage(e, power);
                    if (!through) { s->life = 0; break; }
                }
            }
            if (s->life && mg.boss && mg_abs((int16_t)(mg.boss->x - s->x)) < 40 &&
                mg_abs((int16_t)(mg.boss->y - s->y)) < 48) {
                mg_boss_damage((uint8_t)(s->mode == MG_SHOT_PIERCE ? 2 : 1));
                if (!through) s->life = 0;
            }
        } else {
            if (mg_abs((int16_t)(p->x - s->x)) < 20 &&
                mg_abs((int16_t)((p->y - 24) - s->y)) < 24) {
                mg_player_hit((int16_t)(s->x - s->vx * 8));
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
        /* Uncollected map items may be streamed again on backtracking.
         * Never discard the key or mark a failed allocation as collected. */
        if (!it->key && mg_abs((int16_t)(it->x - p->x)) > 260) {
            it->life = 0;
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
            if (it->source && it->source <= MG_PICK_COUNT) {
                mg.pick_mask |= (uint16_t)(1u << (it->source - 1));
            } else if (it->source > MG_PICK_COUNT) {
                mg.secret_mask |= (uint8_t)(1u << (it->source - MG_PICK_COUNT - 1));
            }
            ng_sprite_group_set_visible(&it->sprite, 0);
            ng_sprite_group_flush(&it->sprite);
            if (it->key) {
                mg.has_key = 1;
                mg.score += 2000u;
                playSFX(SOUND_SFX_13);
                mg_hint("THE GOLDEN SUN KEY IS YOURS", PAL_GOLD, 150);
            } else if (it->trinket) {
                /* Every kind of thing she picks up says a different word. */
                switch (it->kind) {
                case MG_K_GOLD:
                    mg.score += 500u;
                    if (mg.coins < 99) mg.coins++;
                    playSFX(SOUND_SFX_11);
                    break;
                case MG_K_SILVER:
                    mg.score += 200u;
                    if (mg.coins < 99) mg.coins++;
                    playSFX(SOUND_SFX_11);
                    break;
                case MG_K_FLOWER:
                    mg.score += 250u;
                    if (mg.flowers < 99) mg.flowers++;
                    playSFX(SOUND_SFX_12);
                    break;
                case MG_K_CRITTER:
                    mg.score += 750u;
                    if (mg.critters < 99) mg.critters++;
                    playSFX(SOUND_SFX_9);
                    mg_hint("A FOREST FRIEND IS FREE", PAL_SKY, 90);
                    break;
                case MG_K_LIFE:
                    if (mg.lives < MAX_LIVES) mg.lives++;
                    mg.life_pickups_used++;
                    playSFX(SOUND_SFX_13);
                    playSFX(SOUND_SFX_12);
                    mg_hint("EXTRA LIFE!", PAL_GOLD, 120);
                    break;
                case MG_K_SWIFT:
                    mg.swift = POWER_TIME;
                    playSFX(SOUND_SFX_15);
                    mg_hint("SWIFT WIND: SHE RUNS LIGHT", PAL_SKY, 90);
                    break;
                case MG_K_MIGHT:
                    mg.might = POWER_TIME;
                    playSFX(SOUND_SFX_4);
                    mg_hint("THORN MIGHT: HER STRIKE BITES", PAL_GOLD, 90);
                    break;
                case MG_K_VEIL:
                    mg.veil = VEIL_TIME;
                    playSFX(SOUND_SFX_8);
                    mg_hint("MIST VEIL: NOTHING CAN TOUCH HER", PAL_SKY, 90);
                    break;
                case MG_K_SPRING:
                    mg.spring = POWER_TIME;
                    playSFX(SOUND_SFX_15);
                    playSFX(SOUND_SFX_11);
                    mg_hint("SPRING BUD: SHE JUMPS THE CANOPY", PAL_SKY, 90);
                    break;
                case MG_K_CROWN:
                    mg.crown = POWER_TIME;
                    playSFX(SOUND_SFX_13);
                    mg_hint("THORN CROWN: HER THROW GROWS", PAL_GOLD, 90);
                    break;
                case MG_K_THORNS:
                    mg.thorns = (uint8_t)(mg.thorns + MG_THORNS_REFILL > MG_THORNS_MAX
                                          ? MG_THORNS_MAX : mg.thorns + MG_THORNS_REFILL);
                    playSFX(SOUND_SFX_11);
                    mg_hint("THORN SHEAF: TWO AT A TIME", PAL_SKY, 60);
                    break;
                case MG_K_SPREAD:
                case MG_K_PIERCE:
                case MG_K_GALE:
                    mg.weapon = (uint8_t)(it->kind == MG_K_SPREAD ? MG_W_SPREAD
                                        : it->kind == MG_K_PIERCE ? MG_W_PIERCE : MG_W_GALE);
                    mg.weapon_ammo = MG_WEAPON_AMMO;
                    playSFX(SOUND_SFX_13);
                    playSFX(SOUND_SFX_15);
                    mg_hint(it->kind == MG_K_SPREAD ? "PETAL FAN: THREE AT ONCE"
                          : it->kind == MG_K_PIERCE ? "GOLDEN SEED: IT GOES THROUGH"
                          : "WIND LEAF: IT COMES BACK", PAL_GOLD, 120);
                    break;
                case MG_K_BLOOM:
                    if (mg.art < MAX_ART) mg.art++;
                    playSFX(SOUND_SFX_13);
                    mg_hint("A BLOOM BUD: ONE MORE SECRET ART", PAL_GOLD, 120);
                    break;
                default:  /* the elder's charm */
                    mg.score += 300u;
                    playSFX(SOUND_SFX_6);
                    mg_hint(mg.boss_active ? mg_boss_hint[mg.stage]
                                           : mg_secret_hint[mg.stage], PAL_GOLD, 180);
                    break;
                }
            } else if (it->kind == MG_I_HEART) {
                if (p->hp < MAX_HP) p->hp++;
                mg.score += 200u;
                playSFX(SOUND_SFX_12);
            } else if (it->kind == MG_I_ROSE_RED) {
                if (mg.art < MAX_ART) mg.art++;
                mg.score += 500u;
                playSFX(SOUND_SFX_13);
            } else {
                mg.score += 1000u;
                playSFX(SOUND_SFX_11);
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
        if (e->mood == MG_MOOD_DYING) {
            /* Up a few pixels, then down past the road and out of sight,
             * drifting away from her. Nothing else touches it now. */
            NGCharacter *b = e->body;
            uint8_t f = ++e->move_timer;
            int16_t vy = (int16_t)(-4 + f / 3);
            if (vy > 7) vy = 7;
            ng_char_set_pos(b, (int16_t)(b->x + ((f & 1) ? e->heading : 0)), (int16_t)(b->y + vy));
            if (b->y > MG_GROUND_Y + 80 || f > 90) {
                ng_chars_remove(b);
                e->body = 0;
            }
            continue;
        }
        e->timer++;
        if (e->hurt) e->hurt--;
        if (e->body->y > MG_GROUND_Y + 20) {          /* gone down a pit */
            ng_chars_remove(e->body);
            e->body = 0;
            continue;
        }

        int16_t dx = (int16_t)(p->x - e->body->x);
        if (!e->posted) {
            NGCharacter *b = e->body;
            int8_t dir;
            uint8_t nf = mg_enemy_frames(e->type);
            uint8_t flip;
            /* Turning waits until she's a dozen pixels past it: facing her
             * from directly above or beside, it used to flip every frame,
             * juddering left and right on the spot. */
            if (dx < -12) e->face = -1;
            else if (dx > 12) e->face = 1;
            dir = e->face;
            flip = (uint8_t)(dir < 0);
            uint8_t flier = mg_enemy_flies(e->type);

            if (e->mood == 0 && mg_abs(dx) >= 220 && e->type != MG_E_WRAITH) {
                /* Not noticed her yet: it keeps to its own patch of the
                 * valley instead of marching at her from off-screen. */
                int16_t off = (int16_t)(b->x - e->home);
                if (off > 48) e->heading = -1;
                else if (off < -48) e->heading = 1;
                b->vx_fp = (e->type == MG_E_VINESTING) ? 0 : e->heading * mg_pace(110);
                if (flier) b->vy_fp = (int32_t)(((e->timer >> 5) & 1) ? -40 : 40);
                mg_frame(b, (uint8_t)((e->timer / 16) % nf), (uint8_t)(e->heading < 0));
            } else {
                if (e->mood == 0) e->mood = 1;
                switch (e->type) {
                case MG_E_WRAITH: {
                    /* Straight for her, through the air, from wherever it came. */
                    int16_t ty = (int16_t)(p->y - 30);
                    b->vx_fp = dir * mg_pace(300);
                    b->vy_fp = (b->y < ty - 4) ? 180 : ((b->y > ty + 4) ? -180 : 0);
                    mg_frame(b, (uint8_t)((e->timer / 6) % nf), flip);
                    break;
                }
                case MG_E_BEETLE:
                    /* Stalk, rev the saw, then commit to a charge it can't
                     * steer out of -- and stand winded after it. */
                    if (e->mood == 1) {
                        b->vx_fp = dir * mg_pace(200);
                        mg_frame(b, (uint8_t)((e->timer / 8) % nf), flip);
                        if (e->move_timer) e->move_timer--;
                        else if (mg_abs(dx) < 130) { e->mood = 2; e->move_timer = 24; e->heading = dir; }
                    } else if (e->mood == 2) {
                        b->vx_fp = 0;
                        mg_frame(b, (uint8_t)(((e->timer / 3) & 1) ? nf - 1 : 0), (uint8_t)(e->heading < 0));
                        if (--e->move_timer == 0) { e->mood = 3; e->move_timer = 36; playSFX(SOUND_SFX_14); }
                    } else if (e->mood == 3) {
                        b->vx_fp = e->heading * mg_pace(820);
                        mg_frame(b, (uint8_t)((e->timer / 3) % nf), (uint8_t)(e->heading < 0));
                        if (--e->move_timer == 0) { e->mood = 4; e->move_timer = 30; }
                    } else {
                        b->vx_fp = 0;
                        mg_frame(b, 0, (uint8_t)(e->heading < 0));
                        if (--e->move_timer == 0) { e->mood = 1; e->move_timer = 40; }
                    }
                    break;
                case MG_E_CROW: case MG_E_SMOGBAT: case MG_E_PLASTICBAT: {
                    /* Circle high over her, fold the wings and dive, then
                     * climb back out: the dive is the moment to strike it. */
                    int16_t speed = (int16_t)(e->type == MG_E_SMOGBAT ? 460
                                   : e->type == MG_E_PLASTICBAT ? 400 : (mg.stage == 8 ? 540 : 420));
                    if (e->mood == 1) {
                        int16_t ty = (int16_t)(p->y - 96);
                        b->vx_fp = dir * mg_pace((int16_t)(speed / 2));
                        b->vy_fp = (b->y < ty - 6) ? 120 : ((b->y > ty + 6) ? -120 : 0);
                        if (e->move_timer) e->move_timer--;
                        else if (mg_abs(dx) < 80) { e->mood = 2; e->move_timer = 40; e->heading = dir; playSFX(SOUND_SFX_14); }
                        if (e->type == MG_E_PLASTICBAT && mg_abs(dx) < 24 && (e->timer % 70) == 0) {
                            mg_fire(b->x, b->y, 0, 3, 1, MG_T_TRASH);
                            playSFX(SOUND_SFX_5);
                        }
                    } else if (e->mood == 2) {
                        b->vx_fp = e->heading * mg_pace(speed);
                        b->vy_fp = 420;
                        if (b->y >= p->y - 16 || --e->move_timer == 0) { e->mood = 3; e->move_timer = 40; }
                    } else {
                        b->vx_fp = e->heading * mg_pace((int16_t)(speed / 2));
                        b->vy_fp = -320;
                        if (b->y <= p->y - 90 || --e->move_timer == 0) { e->mood = 1; e->move_timer = 50; }
                    }
                    if (e->type == MG_E_CROW && nf >= 6) {
                        /* Wing-beats while it circles and climbs, wings
                         * folded for the dive. */
                        mg_frame(b, (uint8_t)(e->mood == 2 ? 4 : (e->timer / (e->mood == 3 ? 3 : 5)) % 4),
                                 (uint8_t)(e->mood == 1 ? flip : e->heading < 0));
                    } else {
                        mg_frame(b, (uint8_t)((e->timer / (e->mood == 2 ? 3 : 6)) % nf),
                                 (uint8_t)(e->mood == 1 ? flip : e->heading < 0));
                    }
                    break;
                }
                case MG_E_DRONE: case MG_E_POACHDRONE:
                    if (e->type == MG_E_DRONE && mg.stage == 7) {
                        /* Sunken Reef: the drone's body drifts as a jellyfish. */
                        int16_t ty = (int16_t)(p->y - 40 + (((e->timer >> 5) & 1) ? -24 : 24));
                        b->vx_fp = dir * mg_pace(110);
                        b->vy_fp = (b->y < ty) ? 60 : -60;
                        mg_frame(b, (uint8_t)((e->timer / 14) % nf), flip);
                    } else {
                        /* Hover out of reach and keep a firing distance:
                         * back off if she closes in, drift in if she runs. */
                        int16_t ty = (int16_t)(p->y - 70 + (((e->timer >> 4) & 1) ? -6 : 6));
                        int16_t ad = mg_abs(dx);
                        b->vx_fp = ad < 100 ? -dir * mg_pace(200) : (ad > 170 ? dir * mg_pace(200) : 0);
                        b->vy_fp = (b->y < ty - 4) ? 100 : ((b->y > ty + 4) ? -100 : 0);
                        mg_frame(b, (uint8_t)((e->timer / 10) % nf), flip);
                        if ((e->timer % (e->type == MG_E_DRONE ? 110 : 120)) == 55 && ad < 210) {
                            int16_t aim = (int16_t)((p->y - 24) - b->y > 20 ? 1 : 0);
                            mg_fire(b->x, b->y, (int16_t)(dir * 4), aim, 1,
                                    (uint8_t)(e->type == MG_E_DRONE ? MG_T_BOLT : MG_T_SPIT));
                            playSFX(SOUND_SFX_6);
                        }
                    }
                    break;
                case MG_E_JELLYFISH: case MG_E_ACIDMOTH: case MG_E_CHEMFLY: {
                    /* Never charges her: a drift that hangs at head height,
                     * so she has to go around it or strike it down. The moth
                     * swings wide and lazy, the fly quick and tight. */
                    int16_t speed = (int16_t)(e->type == MG_E_CHEMFLY ? 130 : (e->type == MG_E_ACIDMOTH ? 70 : 90));
                    uint8_t period = (uint8_t)(e->type == MG_E_ACIDMOTH ? 6 : (e->type == MG_E_CHEMFLY ? 3 : 5));
                    int16_t swing = (int16_t)(e->type == MG_E_ACIDMOTH ? 40 : 20);
                    int16_t ty = (int16_t)(p->y - 40 + (((e->timer >> period) & 1) ? -swing : swing));
                    b->vx_fp = dir * mg_pace(speed);
                    b->vy_fp = (b->y < ty) ? 70 : -70;
                    mg_frame(b, (uint8_t)((e->timer / 12) % nf), flip);
                    break;
                }
                case MG_E_TOXICCRAB:
                    /* Sideways bursts with a pause between: faster when she's close. */
                    if ((e->timer % 40) < 22) {
                        b->vx_fp = dir * mg_pace((int16_t)(mg_abs(dx) < 70 ? 640 : 420));
                        mg_frame(b, (uint8_t)((e->timer / 4) % nf), flip);
                    } else {
                        b->vx_fp = 0;
                        mg_frame(b, 0, flip);
                    }
                    break;
                case MG_E_SLAGGOLEM:
                    /* Slow, heavy, and it pounds the road when she's close:
                     * a shockwave runs out both ways along the ground. */
                    if (e->mood == 1) {
                        b->vx_fp = dir * mg_pace(140);
                        mg_frame(b, (uint8_t)((e->timer / 12) % nf), flip);
                        if (e->move_timer) e->move_timer--;
                        else if (mg_abs(dx) < 64) { e->mood = 2; e->move_timer = 30; }
                    } else if (e->mood == 2) {
                        b->vx_fp = 0;
                        mg_frame(b, (uint8_t)(nf - 1), flip);
                        if (--e->move_timer == 0) {
                            mg.shake = 6;
                            mg_fire(b->x, (int16_t)(b->y - 6), -3, 0, 1, MG_T_SPIT);
                            mg_fire(b->x, (int16_t)(b->y - 6), 3, 0, 1, MG_T_SPIT);
                            playSFX(SOUND_SFX_10);
                            e->mood = 3; e->move_timer = 40;
                        }
                    } else {
                        b->vx_fp = 0;
                        mg_frame(b, 0, flip);
                        if (--e->move_timer == 0) { e->mood = 1; e->move_timer = 70; }
                    }
                    break;
                case MG_E_SEWERRAT:
                    /* Trots toward her, then breaks into a sprint up close. */
                    if (mg_abs(dx) < 110) {
                        b->vx_fp = dir * mg_pace(560);
                        mg_frame(b, (uint8_t)((e->timer / 4) % nf), flip);
                    } else {
                        b->vx_fp = dir * mg_pace(260);
                        mg_frame(b, (uint8_t)((e->timer / 8) % nf), flip);
                    }
                    break;
                case MG_E_VINESTING:
                    /* Rooted where it grows, turning to face her and lashing
                     * faster the closer she dares to come. */
                    b->vx_fp = 0;
                    b->vy_fp = 0;
                    mg_frame(b, (uint8_t)((e->timer / 20) % nf), flip);
                    if ((e->timer % (mg_abs(dx) < 36 ? 60 : 100)) == 50 && mg_abs(dx) < 64) {
                        mg_fire(b->x, (int16_t)(b->y - 20), (int16_t)(dir * 3), 0, 1, MG_T_SPIT);
                        playSFX(SOUND_SFX_5);
                    }
                    break;
                default:
                    /* The hoppers: slime, goblin, worm and spore goblin. */
                    if ((e->timer % (uint16_t)(78 - mg.stage * 4)) == 0 &&
                        (ng_physics_is_grounded(b) || b->y >= MG_GROUND_Y - 4)) {
                        b->vy_fp = -4 * NG_FP_ONE;
                        b->vx_fp = dir * mg_pace(240);
                    }
                    mg_frame(b, (uint8_t)((e->timer / 12) % nf), flip);
                    if ((e->type == MG_E_SLIME || e->type == MG_E_SPOREGOB)
                        && (e->timer % 150) == 75 && mg_abs(dx) < 160) {
                        mg_fire(b->x, (int16_t)(b->y - 12), (int16_t)(dir * 3), -1, 1, MG_T_SPIT);
                        playSFX(SOUND_SFX_5);
                    }
                    if (e->type == MG_E_GOBLIN && mg.stage == 9 && (e->timer % 130) == 65 && mg_abs(dx) < 170) {
                        /* Golden Savanna: the poacher throws a snaring net. */
                        mg_fire(b->x, (int16_t)(b->y - 14), (int16_t)(dir * 3), 0, 1, MG_T_SPIT);
                        playSFX(SOUND_SFX_5);
                    }
                    break;
                }
            }
            /* Walkers stop at a pit's lip and turn back, instead of
             * marching off into it. */
            if (!flier && b->vx_fp && !mg_over_pit(b->x, 0) &&
                mg_over_pit((int16_t)(b->x + (b->vx_fp > 0 ? 14 : -14)), 0)) {
                b->vx_fp = 0;
                e->heading = (int8_t)(-e->heading);
            }
            /* Flyers keep to the sky band: never above the screen, never
             * grinding along the road unless they're diving. */
            if (flier) {
                if (b->y < 24 && b->vy_fp < 0) b->vy_fp = 0;
                if (e->mood != 2 && b->y > MG_GROUND_Y - 24 && b->vy_fp > 0) b->vy_fp = -60;
            }
        }
        if (mg_abs((int16_t)(e->body->x - p->x)) < 20 &&
            mg_abs((int16_t)(e->body->y - p->y)) < 26) {
            /*
             * Coming down on top of a soft creature squashes it and bounces
             * her back up; anything else is a hit she has to answer for.
             */
            int16_t feet = (int16_t)(p->y - mg.player_prev_y);
            if (!e->hurt && mg_soft_enemy(e->type) && p->vy_fp > 0 && feet >= 0 &&
                p->y < e->body->y - 6) {
                mg_enemy_damage(e, 10);
                p->vy_fp = -STOMP_KICK;
                mg.score += 150u;
                playSFX(SOUND_SFX_3);
            } else if (!e->hurt) {
                mg_player_hit(e->body->x);
                /* Shove the creature off: being cornered by one slime was death.
                 * (A touch in the bonus round clears the field, this one too.) */
                if (e->body) {
                    e->body->vx_fp = dx < 0 ? 900 : -900;
                    e->hurt = 20;
                }
            }
        }
    }

    if (mg.boss && mg.boss_active) mg_boss_ai(p);

    /*
     * Invulnerability read-out.  Blinking her in and out looked like a
     * dropped sprite, so she stays on screen and her palette pulses to the
     * sunlight one instead: readable, and it never hides where she is.
     */
    p->visible = 1;
    if (mg.hurt) {
        mg.hurt--;
        if (mg.hurt > 82) {
            /* the blow itself: a flash of red */
            if (mg.hurt_lit != 2) {
                mg.hurt_lit = 2;
                mg_hurt_red_palette();
            }
        } else if (mg.hurt > 20) {
            uint8_t lit = (uint8_t)((mg.hurt & 8) != 0);
            if (lit != mg.hurt_lit) {
                mg.hurt_lit = lit;
                if (lit) mg_hurt_palette();
                else mg_palette(PAL_HERO, mg_hero_normal_pal());
            }
        } else if (mg.hurt_lit) {
            mg.hurt_lit = 0;
            mg_palette(PAL_HERO, mg_hero_normal_pal());
        }
    } else if (mg.veil) {
        /* The mist veil: she fades in and out slowly, on purpose. */
        p->visible = (uint8_t)((mg.veil & 16) ? 0 : 1);
    }

    /* Power-ups run down whether or not she is fighting. */
    if (mg.swift && --mg.swift == 0) mg.hud_dirty = 1;
    if (mg.might && --mg.might == 0) mg.hud_dirty = 1;
    if (mg.veil && --mg.veil == 0) mg.hud_dirty = 1;
    if (mg.spring && --mg.spring == 0) mg.hud_dirty = 1;
    if (mg.crown && --mg.crown == 0) mg.hud_dirty = 1;
    if (mg.flash && --mg.flash == 0) mg_palette(PAL_HERO, mg_hero_normal_pal());
    /* The second wave used to repeat the same petal curtain as the first;
     * now it's the Secret Art's own signature -- a ring of gold expanding
     * from where she stands, nothing else in the game looks like it. */
    if (mg.art_wave && --mg.art_wave == 0)
        mg_secret_art_ring(mg.player->x, (int16_t)(mg.player->y - 30));
}

/* ------------------------------------------------------------------ */
/*  HUD Drawing                                                       */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER mg_hud_static(void)
{
    uint8_t i;
    /* Face portrait avatar: 2x2 sprite strips (slots SLOT_HUD .. SLOT_HUD + 1) */
    /* Her face at the top left, in its own palette: Maiya's or Luna's. */
    mg_palette(PAL_FACE, mg.hero_choice ? mg_face_alt_pal : mg_face_pal);
    ng_sprite_group_init(&mg.hud[0], SLOT_HUD, 2, 2,
                         (uint16_t)(mg.hero_choice ? MG_FACE_ALT_TILE : MG_FACE_TILE), PAL_FACE);
    ng_sprite_group_set_pos(&mg.hud[0], 16, 8);
    ng_sprite_group_upload(&mg.hud[0]);

    /* Her health used to be five separate heart sprites here; at this scale
     * they read as loose dots instead of a status, so a solid FIX bar next
     * to her face takes their place -- it drains visibly, left instead of
     * blinking off one at a time. */
    ng_fix_putc(7, 1, 'H', PAL_GOLD);
    ng_fix_putc(8, 1, 'P', PAL_GOLD);

    for (i = 0; i < MG_TRAY_SLOTS; i++) {
        ng_sprite_group_init(&mg.tray[i], (uint16_t)(SLOT_TRAY + i * 2), 2, 2,
                             mg_item_tiles[MG_I_HEART], PAL_ITEM);
        ng_sprite_group_set_scale(&mg.tray[i], 0x7F, 0x7F);
        ng_sprite_group_set_visible(&mg.tray[i], 0);
        ng_sprite_group_upload(&mg.tray[i]);
    }

    /* Her halo, for the moment the valley carries her up. */
    ng_sprite_group_init(&mg.hud[10], (uint16_t)(SLOT_HUD + 12), 1, 1, MG_TOOL_TILE + MG_T_HALO, PAL_TOOL);
    ng_sprite_group_set_visible(&mg.hud[10], 0);
    ng_sprite_group_upload(&mg.hud[10]);

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
                    (char)(i < mg.lives ? GLYPH_HEART : ' '), PAL_HP_LO);
    }
}

/*
 * The framed bars: a rounded gold (or, for a guardian, rusted) frame with a
 * shaded fill that drains a pixel at a time. Each cap carries five pixels
 * of fill and each middle cell eight, drawn from tiles the FIX builder puts
 * above tile 255 -- left caps, then middle cells, then right caps.
 */
static void NEOGEO_USER mg_draw_bar(uint8_t col, uint8_t row, uint8_t cells, uint8_t px, uint8_t pal)
{
    uint8_t i, lv;
    lv = (uint8_t)(px > 5 ? 5 : px);
    ng_fix_put_tile(col, row, (uint16_t)(MG_BAR_TILE + lv), pal);
    px = (uint8_t)(px - lv);
    for (i = 0; i < cells; i++) {
        lv = (uint8_t)(px > 8 ? 8 : px);
        ng_fix_put_tile((uint8_t)(col + 1 + i), row, (uint16_t)(MG_BAR_CELL_TILE + lv), pal);
        px = (uint8_t)(px - lv);
    }
    lv = (uint8_t)(px > 5 ? 5 : px);
    ng_fix_put_tile((uint8_t)(col + 1 + cells), row, (uint16_t)(MG_BAR_RCAP_TILE + lv), pal);
}

/* Green above sixty percent, amber above thirty, red below. */
static uint8_t NEOGEO_USER mg_bar_tier(uint8_t px, uint8_t full, uint8_t hi, uint8_t mid, uint8_t lo)
{
    if ((uint16_t)px * 10u > (uint16_t)full * 6u) return hi;
    if ((uint16_t)px * 10u > (uint16_t)full * 3u) return mid;
    return lo;
}

static void NEOGEO_USER mg_draw_hp_bar(void)
{
    mg_draw_bar(MG_HP_BAR_COL, 1, MG_HP_BAR_CELLS, mg.hp_px,
                mg_bar_tier(mg.hp_px, MG_HP_BAR_PX, PAL_HP_HI, PAL_HP_MID, PAL_HP_LO));
}

/* The guardian's health, top centre, only while the arena fight is on. It
 * clears itself the moment the fight ends. */
static void NEOGEO_USER mg_draw_boss_bar(void)
{
    static uint8_t shown = 0;
    if (!mg.boss_active || !mg.boss || !mg.boss->max_hp) {
        if (shown) {
            ng_fix_clear_rect(0, ROW_POWER, 40, 1, PAL_TEXT);
            shown = 0;
        }
        return;
    }
    if (!shown) {
        ng_fix_puts(MG_BOSS_BAR_LABEL_COL, ROW_POWER, "BOSS", PAL_WARN);
        shown = 1;
    }
    mg_draw_bar(MG_BOSS_BAR_COL, ROW_POWER, MG_BOSS_BAR_CELLS, mg.boss_px,
                mg_bar_tier(mg.boss_px, MG_BOSS_BAR_PX, PAL_BOSS_HP_HI, PAL_BOSS_HP_MID, PAL_BOSS_HP_LO));
}

/* Both bars slide toward the real value each frame instead of jumping:
 * a pixel a frame as health drains, two as it refills. */
static void NEOGEO_USER mg_bars_step(void)
{
    uint8_t hp = mg.player ? mg.player->hp : 0;
    uint8_t target = (uint8_t)(((uint16_t)hp * MG_HP_BAR_PX) / MAX_HP);
    if (mg.hp_px != target) {
        if (mg.hp_px > target) mg.hp_px--;
        else mg.hp_px = (uint8_t)(mg.hp_px + 2 > target ? target : mg.hp_px + 2);
        mg_draw_hp_bar();
    }
    if (mg.boss_active && mg.boss && mg.boss->max_hp) {
        target = (uint8_t)(((uint16_t)mg.boss->hp * MG_BOSS_BAR_PX) / mg.boss->max_hp);
        if (mg.boss_px != target) {
            if (mg.boss_px > target) mg.boss_px--;
            else mg.boss_px = (uint8_t)(mg.boss_px + 2 > target ? target : mg.boss_px + 2);
            mg_draw_boss_bar();
        }
    }
}

/* The clock under her HP bar; it turns to the warning ink once she's
 * been told to hurry. The attract demo plays without one. */
static void NEOGEO_USER mg_draw_clock(void)
{
    if (mg.demo) return;
    /* The bonus round keeps its own time, and counts what she has cleared. */
    if (mg.state == MG_BONUS) {
        ng_fix_puts(7, ROW_CLOCK, "TIME", PAL_GOLD);
        mg_number(12, ROW_CLOCK, (uint32_t)((mg.state_timer + 59u) / 60u), 3, PAL_TEXT);
        ng_fix_puts(7, ROW_CLOCK + 1, "HITS", PAL_GOLD);
        mg_number(12, ROW_CLOCK + 1, mg.bonus_hits, 3, PAL_TEXT);
        return;
    }
    ng_fix_puts(7, ROW_CLOCK, "TIME", PAL_GOLD);
    mg_number(12, ROW_CLOCK, mg.clock, 3, mg.clock <= MG_HURRY_AT ? PAL_WARN : PAL_TEXT);
}

static void NEOGEO_USER mg_update_hud(void)
{
    mg_draw_clock();
    mg_draw_hp_bar();
    mg_draw_boss_bar();
    ng_sprite_group_set_visible(&mg.hud[9], mg.has_key && !mg.gate_unlocked);
    ng_sprite_group_flush(&mg.hud[9]);
    mg_number(28, ROW_SCORE, mg.score, 6, PAL_TEXT);
    mg.score_shown = mg.score;
    mg_draw_lives();
    mg_draw_tray();
}

/*
 * The collection tray, bottom left.  Each slot is the real pick-up art,
 * shrunk by the hardware to sixteen pixels, with its count in FIX digits
 * beside it: the art charges, the coins, the flowers, the freed friends,
 * the key while she carries it, and any power she is running on, with its
 * seconds left.  Empty slots take no sprite at all.
 */
/*
 * One tray slot: the pick-up's own art, shrunk small and tucked hard into
 * the left edge, with its count right against it -- a corner badge, not a
 * second HUD bar.
 */
static uint16_t NEOGEO_USER mg_tray_slot(uint8_t slot, uint16_t x, uint16_t tile, uint8_t pal,
                                        uint32_t count, uint8_t digits)
{
    NGSpriteGroup *g = &mg.tray[slot];
    if (x > 296u) {   /* the row is full: never draw past the screen edge */
        ng_sprite_group_set_visible(g, 0);
        ng_sprite_group_flush(g);
        return x;
    }
    ng_sprite_group_set_tile_base(g, tile);
    ng_sprite_group_set_palette(g, pal);
    ng_sprite_group_set_scale(g, MG_TRAY_ICON_SCALE, MG_TRAY_ICON_SCALE);
    ng_sprite_group_set_pos(g, (int16_t)x, (int16_t)(ROW_TRAY * 8 - 2));
    ng_sprite_group_set_visible(g, 1);
    ng_sprite_group_flush(g);
    if (digits) {
        mg_number((uint8_t)((x + MG_TRAY_ICON_PX + 1u) / 8u), ROW_TRAY, count, digits, PAL_TEXT);
    }
    return (uint16_t)(x + MG_TRAY_ICON_PX + digits * 8u + 2u);
}

static void NEOGEO_USER mg_draw_tray(void)
{
    uint8_t slot = 0;
    uint16_t x = 0;

    ng_fix_clear_rect(0, ROW_TRAY, 40, 1, PAL_TEXT);

    x = mg_tray_slot(slot++, x, mg_item_tiles[MG_I_ROSE_RED], PAL_ITEM, mg.art, 1);
    if (mg.weapon) {
        static const uint8_t icon[4] = { 0, MG_K_SPREAD, MG_K_PIERCE, MG_K_GALE };
        x = mg_tray_slot(slot++, x, mg_trinket_tiles[icon[mg.weapon]], PAL_TRINKET, mg.weapon_ammo, 2);
    } else if (mg.thorns) {
        x = mg_tray_slot(slot++, x, mg_trinket_tiles[MG_K_THORNS], PAL_TRINKET, mg.thorns, 2);
    }
    x = mg_tray_slot(slot++, x, mg_trinket_tiles[MG_K_GOLD], PAL_TRINKET, mg.coins, 2);
    x = mg_tray_slot(slot++, x, mg_trinket_tiles[MG_K_FLOWER], PAL_TRINKET, mg.flowers, 2);
    x = mg_tray_slot(slot++, x, mg_trinket_tiles[MG_K_CRITTER], PAL_TRINKET, mg.critters, 2);
    if (mg.has_key && !mg.gate_unlocked) {
        x = mg_tray_slot(slot++, x, mg_item_tiles[MG_I_GEM], PAL_ITEM, 0, 0);
    }
    if (mg.swift)  x = mg_tray_slot(slot++, x, mg_trinket_tiles[MG_K_SWIFT], PAL_TRINKET, (mg.swift + 59u) / 60u, 1);
    if (mg.might)  x = mg_tray_slot(slot++, x, mg_trinket_tiles[MG_K_MIGHT], PAL_TRINKET, (mg.might + 59u) / 60u, 1);
    if (mg.veil)   x = mg_tray_slot(slot++, x, mg_trinket_tiles[MG_K_VEIL], PAL_TRINKET, (mg.veil + 59u) / 60u, 1);
    if (mg.spring) x = mg_tray_slot(slot++, x, mg_trinket_tiles[MG_K_SPRING], PAL_TRINKET, (mg.spring + 59u) / 60u, 1);
    if (mg.crown)  x = mg_tray_slot(slot++, x, mg_trinket_tiles[MG_K_CROWN], PAL_TRINKET, (mg.crown + 59u) / 60u, 1);

    for (; slot < MG_TRAY_SLOTS; slot++) {
        ng_sprite_group_set_visible(&mg.tray[slot], 0);
        ng_sprite_group_flush(&mg.tray[slot]);
    }
}

/* ------------------------------------------------------------------ */
/*  Main Per-Frame Update                                             */
/* ------------------------------------------------------------------ */
/*
 * The valley's own obstacle, each frame of play.
 *  - Rotten ledges: stand on one too long and it crumbles away, dust
 *    trickling off it first as the warning; it grows back a while later.
 *  - Underwater: now and then a bubble rises from her.
 */
static void NEOGEO_USER mg_stage_mechanics(void)
{
    uint8_t mech = mg_mech(), i;
    NGCharacter *p = mg.player;

    if (mech == MG_M_CRUMBLE && !mg.boss_active) {
        for (i = 0; i < MG_PLATFORM_COUNT; i++) {
            if (mg.ledge_gone[i]) {
                if (--mg.ledge_gone[i] == 0) mg.ledge_stand[i] = 0;
                continue;
            }
            if (mg.on_ledge && mg.ledge_index == i) {
                const MGPlatform *pl = &mg_levels[mg.stage].platforms[i];
                if (++mg.ledge_stand[i] >= MG_CRUMBLE_AFTER) {
                    mg.ledge_gone[i] = MG_CRUMBLE_BACK;
                    mg.on_ledge = 0;
                    mg_burst((int16_t)(pl->x + pl->width / 2), pl->y, MG_T_DUST, 4, 1);
                    playSFX(SOUND_SFX_10);
                } else if (mg.ledge_stand[i] > 18 && (mg.ledge_stand[i] & 3) == 0) {
                    mg_burst((int16_t)(pl->x + 8 + (mg_rand() % (pl->width - 16))), (int16_t)(pl->y + 18),
                             MG_T_DUST, 1, 2);
                }
            } else if (mg.ledge_stand[i]) {
                mg.ledge_stand[i]--;
            }
        }
    }

    if (mech == MG_M_WATER && (mg.tick % 50u) == 0u)
        mg_burst(p->x, (int16_t)(p->y - 50), MG_T_SPIT, 1, -2);
}

/*
 * The mission clock. One second per sixty frames of play (cards, the boss
 * introduction and the bonus round don't count). Past MG_WRAITH_AT the smog
 * sends wraiths in from whichever screen edge she isn't facing, two at a
 * time at most, so the only real escape is to get moving.
 */
static void NEOGEO_USER mg_clock_tick(void)
{
    NGCharacter *p = mg.player;
    if (mg.demo || !mg.clock) return;

    if (++mg.clock_sub >= 60) {
        mg.clock_sub = 0;
        mg.clock--;
        mg_draw_clock();
        if (mg.clock == MG_HURRY_AT) {
            mg_hint("HURRY UP! THE SMOG IS GATHERING", PAL_WARN, 150);
            playSFX(SOUND_SFX_14);
        } else if (mg.clock == MG_WRAITH_AT) {
            mg_hint("THE SMOG WRAITHS ARE COMING", PAL_WARN, 120);
        } else if (mg.clock == 0) {
            /* Time is up: the smog takes this life, whatever protects her. */
            mg_hint("TIME UP", PAL_WARN, 120);
            mg.hurt = 0; mg.veil = 0; mg.dash = 0; mg.super_surge = 0;
            p->hp = 1;
            mg_player_damage();
            return;
        }
    }

    if (mg.clock <= MG_WRAITH_AT && !mg.boss_active) {
        if (mg.wraith_timer) {
            mg.wraith_timer--;
        } else {
            uint8_t i, alive = 0;
            for (i = 0; i < MG_ENEMIES; i++)
                if (mg.enemies[i].body && mg.enemies[i].type == MG_E_WRAITH) alive++;
            if (alive < 2) {
                int16_t x = (int16_t)(mg.wraith_side ? mg.camera.x + 330 : mg.camera.x - 10);
                if (mg_spawn_enemy(MG_E_WRAITH, x, (int16_t)(40 + (mg_rand() & 63)), 0)) {
                    mg.wraith_side ^= 1;
                    playSFX(SOUND_SFX_8);
                }
            }
            mg.wraith_timer = (uint8_t)(mg.clock <= 10 ? 70 : 140);
        }
    }
}

/* Ground hazards burn, spike or poison the heroine while she stands in them. */
static void NEOGEO_USER mg_hazard_check(void)
{
    const MGLevel *level = &mg_levels[mg.stage];
    NGCharacter *p = mg.player;
    uint8_t i;

    if (p->y < MG_GROUND_Y - 4 || mg.on_ledge || mg.climbing) return;
    for (i = 0; i < MG_HAZARD_COUNT; i++) {
        const MGHazard *hz = &level->hazards[i];
        if (mg.hazard_disabled_mask & (uint8_t)(1u << i)) continue;
        if (hz->type && hz->type != MG_H_PIT &&
            p->x > hz->x && p->x < (int16_t)(hz->x + hz->width)) {
            mg_player_damage();
            if (mg.state == MG_PLAY) p->vy_fp = -3 * NG_FP_ONE;
            return;
        }
    }
}

/* A clear callout before she's close enough to actually get hurt, once
 * per hazard per visit -- checked from any height (a ledge above a fire
 * patch still deserves the warning) rather than only while grounded, the
 * way the damage check above needs to be. Sludge reads as "toxic" rather
 * than a burn or a puncture, since it's the one that's meant to look like
 * pollution rather than open flame or metal spikes. */
static void NEOGEO_USER mg_hazard_warn_check(void)
{
    const MGLevel *level = &mg_levels[mg.stage];
    NGCharacter *p = mg.player;
    uint8_t i;

    if (mg.state != MG_PLAY) return;
    for (i = 0; i < MG_HAZARD_COUNT; i++) {
        const MGHazard *hz = &level->hazards[i];
        uint8_t bit = (uint8_t)(1u << i);
        int16_t near_x = (int16_t)(hz->x - 50);
        if (!hz->type || (mg.hazard_warn_mask & bit) || (mg.hazard_disabled_mask & bit)) continue;
        if (p->x < near_x || p->x > (int16_t)(hz->x + hz->width)) continue;
        mg.hazard_warn_mask |= bit;
        mg_hint(hz->type == MG_H_PIT ? "A BREAK IN THE ROAD - JUMP IT"
              : hz->type == MG_H_FIRE ? "HAZARD: OPEN FLAME AHEAD"
              : hz->type == MG_H_SPIKES ? "HAZARD: SHARP SPIKES AHEAD"
              : hz->type == MG_H_TOXIC ? "HAZARD: TOXIC - STAND CLOSE, PRESS UP"
              : "TOXIC SLUDGE AHEAD - KEEP CLEAR", PAL_WARN, 90);
        /* A pit also plants its own caution sign at its near edge (see
         * mg_draw_hazards) -- a real board in the world, always there,
         * rather than text that only shows up once and times out. */
        return;
    }
}

/*
 * The last two valleys leave one patch of pollution that a hazard alone
 * cannot explain away: it can be shut off for good, not just avoided.
 * Standing next to it and pressing Up -- the same input that turns the
 * Golden Sun Key at a gate -- clears it, but only once she's actually
 * found something on this road: the fairy's hint says as much. That
 * fits a level she has already walked through in one direction: the fix
 * sits behind her, back where the road started, so clearing it means
 * choosing to backtrack for it rather than stumbling onto it.
 */
static void NEOGEO_USER mg_hazard_disable_check(uint16_t pressed)
{
    const MGLevel *level = &mg_levels[mg.stage];
    NGCharacter *p = mg.player;
    uint8_t i;

    if (mg.state != MG_PLAY || !(pressed & JOY_UP) || mg.climbing) return;
    for (i = 0; i < MG_HAZARD_COUNT; i++) {
        const MGHazard *hz = &level->hazards[i];
        uint8_t bit = (uint8_t)(1u << i);
        int16_t reach = (int16_t)(hz->x - 24);
        if (hz->type != MG_H_TOXIC || (mg.hazard_disabled_mask & bit)) continue;
        if (p->x < reach || p->x > (int16_t)(hz->x + hz->width + 24)) continue;
        if (!mg.secret_mask) {
            mg_hint("NOTHING SHE CARRIES CAN LIFT THIS", PAL_TEXT, 90);
            return;
        }
        mg.hazard_disabled_mask |= bit;
        mg.score += 1000u;
        mg.hud_dirty = 1;
        playSFX(SOUND_SFX_13);
        mg_hint("THE POISON FADES", PAL_SKY, 120);
        return;
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

    if (grounded && mg.airborne && !mg.climbing) {
        mg.airborne = 0;
    } else if (!grounded && !mg.climbing && p->vy_fp > NG_FP_ONE) {
        mg.airborne = 1;
    }

    if (mg.climbing) {
        /*
         * Hand over hand: the frame follows how far up the vine she is, so
         * she only moves when she is actually climbing, and stops still
         * when the stick is centred.
         */
        static const uint8_t reach[2] = { MG_F_JUMP4, MG_F_JUMP3 };
        uint8_t step = (uint8_t)(((uint16_t)p->y / 12u) & 1u);
        mg_frame(p, reach[step], mg.facing);
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
        static const uint8_t spin[4] = { MG_F_SWEEP1, MG_F_SWEEP2, MG_F_SWEEP3, MG_F_SPIN };
        mg_frame(p, (uint8_t)(mg.super_surge >= MG_SURGE_TIME - MG_SURGE_WINDUP
                              ? MG_F_SWEEP0 : spin[(mg.super_surge / 2) & 3]), mg.facing);
    } else if (mg.dash && grounded) {
        mg_frame(p, (uint8_t)(MG_F_RUN0 + ((mg.dash / 2) % 3)), mg.facing);
    } else if (mg.attack) {
        mg.attack--;
        mg_frame(p, (uint8_t)(mg.attack > 6 ? MG_F_ATK1 : MG_F_ATK2), mg.facing);
    } else if (mg.cast) {
        mg.cast--;
        mg_frame(p, (uint8_t)(mg.cast > 5 ? MG_F_CAST1 : MG_F_CAST2), mg.facing);
    } else if (!grounded) {
        mg_frame(p, (uint8_t)(p->vy_fp < 0 ? MG_F_JUMP1 : MG_F_JUMP3), mg.facing);
    } else if (p->vx_fp != 0) {
        mg.walk_distance = (uint16_t)((mg.walk_distance + mg_abs((int16_t)p->vx_fp)) % (40u * NG_FP_ONE));
        mg_frame(p, (uint8_t)(MG_F_WALK0 + ((mg.walk_distance / (10u * NG_FP_ONE)) % 4u)), mg.facing);
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

        /* Walk away and come back and he will tell her again, every time. */
        if (mg_abs((int16_t)(n->x - mg.player->x)) > TALK_RANGE) {
            mg.npc_mask &= (uint8_t)~(1u << which);
            continue;
        }
        if (mg.npc_mask & (uint8_t)(1u << which)) continue;

        mg.npc_mask |= (uint8_t)(1u << which);
        mg_hint(mg_npcs[mg.stage][which].line, PAL_SKY, 150);
        mg_burst(mg.player->x, (int16_t)(mg.player->y - 60), MG_T_STAR, 4, -1);
        playSFX(SOUND_SFX_6);
        if (mg.score < 0xFFFF0000u) mg.score += 100u;
        mg.hud_dirty = 1;
    }
}

static void NEOGEO_USER mg_rescue_check(void)
{
    uint8_t type;

    if (!mg.rescue || mg_abs((int16_t)(mg.player->x - mg.rescue->x)) >= 32) return;

    type = mg.rescue->data0;
    mg.rescue_mask |= (uint8_t)(1u << mg.rescue->data1);
    mg.cage_x = (int16_t)(mg.rescue->x - 16);
    mg.cage_y = (int16_t)(mg.rescue->y - 32);
    mg.cage_open = 70;
    mg_burst(mg.rescue->x, (int16_t)(mg.rescue->y - 30), MG_T_STAR, 4, -3);
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

    ng_sprite_hide_all();
    ng_fix_clear();
    setBACKDROP(0x8000);
    waitVbl();
    mg_ui_palettes();
    mg_palette(PAL_ALLY, mg_sunboy_pal);
    mg_music(SOUND_TRACK_I);

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
 * The bonus round between valleys: the blight sends a line of creatures
 * down the road from both ends and Maiya answers with thorns. Every one
 * cleared is points; clear enough and the valley gives a life back. One
 * touch and the round is over on the spot -- no health lost, no reward.
 */
enum { MG_BONUS_TIME = 1500, MG_BONUS_CARD = 150, MG_BONUS_OUTRO = 90, MG_BONUS_PERFECT = 8 };

static void NEOGEO_USER mg_bonus_clear_creatures(uint8_t poof)
{
    uint8_t i;
    for (i = 0; i < MG_ENEMIES; i++) {
        if (!mg.enemies[i].body) continue;
        if (poof) mg_burst(mg.enemies[i].body->x, (int16_t)(mg.enemies[i].body->y - 10), MG_T_DUST, 2, -1);
        ng_chars_remove(mg.enemies[i].body);
        mg.enemies[i].body = 0;
    }
    for (i = 0; i < MG_SHOTS; i++) mg.shots[i].life = 0;
}

static void NEOGEO_USER mg_bonus_enter(uint8_t next_stage)
{
    uint8_t i;

    /* Rebuild ownership and physics, not just the label over the last arena. */
    mg_scene(next_stage, 0);
    mg.state = MG_BONUS;
    mg.next_stage = next_stage;
    mg.state_timer = MG_BONUS_TIME;
    mg.bonus_hits = 0;
    mg.bonus_shots = 0;
    mg.bonus_timer = 0;
    mg.boss = 0;
    mg.boss_active = 0;
    mg.rescue = 0;
    mg.entrance = 0;
    mg.kills = 0;
    mg_background((uint8_t)((next_stage + 2u) % MG_LEVEL_COUNT), 1);
    ng_camera_snap(&mg.camera, 0, 0);
    ng_level_set_scroll(0, 0);

    mg_bonus_clear_creatures(0);
    for (i = 0; i < MG_ITEMS; i++) {
        mg.items[i].life = 0;
        ng_sprite_group_set_visible(&mg.items[i].sprite, 0);
        ng_sprite_group_flush(&mg.items[i].sprite);
    }

    mg_music(SOUND_TRACK_G);
    ng_fix_clear_rect(1, ROW_HINT, 38, 10, PAL_TEXT);
    mg_centre(ROW_CARD, "BONUS ROUND", PAL_GOLD);
    mg_centre(ROW_CARD + 2, "THORN THEM - DON'T LET ONE TOUCH HER", PAL_TEXT);
    mg.hud_dirty = 1;
    playSFX(SOUND_SFX_13);
}

/* A creature reached her: the round ends here, and the reward with it. */
static void NEOGEO_USER mg_bonus_caught(void)
{
    if (mg.bonus_timer) return;          /* already winding down */
    mg.bonus_timer = MG_BONUS_OUTRO;
    mg.player->vx_fp = 0;
    mg.shake = 8;
    playSFX(SOUND_SFX_16);
    mg_bonus_clear_creatures(1);
    ng_fix_clear_rect(1, ROW_CARD, 38, 3, PAL_TEXT);
    mg_centre(ROW_CARD, "CAUGHT!", PAL_WARN);
    mg_centre(ROW_CARD + 2, "THE BONUS ROUND IS OVER", PAL_TEXT);
}

static void NEOGEO_USER mg_bonus_frame(void)
{
    NGCharacter *p = mg.player;

    /* Winding down after a touch: she stumbles, stands, and the round closes. */
    if (mg.bonus_timer) {
        p->vx_fp = 0;
        mg_frame(p, (uint8_t)(mg.bonus_timer > MG_BONUS_OUTRO - 30 ? MG_F_HURT1 : MG_F_IDLE0), mg.facing);
        mg_world_step();
        if (--mg.bonus_timer == 0) mg_interlude(mg.next_stage);
        return;
    }

    if (ng_feedback_is_hitstop()) {
        mg_world_step();
        return;
    }

    /* Alternate the approach side as the player crosses the bonus field. */
    if ((mg.state_timer % 90u) == 0u) {
        uint8_t kind = (uint8_t)((mg.state_timer / 90u) & 1u ? MG_E_SLIME : MG_E_BEETLE);
        MGEnemy *e = mg_spawn_enemy(kind, p->x < 160 ? 292 : 28, MG_GROUND_Y, 0);
        /* They come for her from the moment they appear: an unaware one
         * would keep to its patch off at the far end of the field. */
        if (e) e->mood = 1;
    }

    mg_controls();
    mg_animate_player();
    mg_world_step();
    mg_update_entities();
    if (mg.state != MG_BONUS || mg.bonus_timer) return;

    /* Every creature cleared here is worth a coin's weight in points. */
    if (mg.kills != mg.bonus_hits) {
        mg.score += (uint32_t)(mg.kills - mg.bonus_hits) * 300u;
        mg.bonus_hits = mg.kills;
        mg.hud_dirty = 1;
    }
    if ((mg.state_timer % 60u) == 0u) mg.hud_dirty = 1;
    if (mg.state_timer == MG_BONUS_TIME - MG_BONUS_CARD) ng_fix_clear_rect(1, ROW_CARD, 38, 3, PAL_TEXT);
    if (mg.hud_dirty) {
        mg_update_hud();
        mg.hud_dirty = 0;
    }

    if (--mg.state_timer == 0) {
        mg_bonus_clear_creatures(1);
        if (mg.bonus_hits >= MG_BONUS_PERFECT && mg.lives < MAX_LIVES) {
            mg.lives++;
            mg_centre(ROW_CARD, "PERFECT! ONE LIFE RETURNED", PAL_GOLD);
            playSFX(SOUND_SFX_12);
        } else {
            mg_centre(ROW_CARD, "WELL DONE!", PAL_GOLD);
        }
        playSFX(SOUND_SFX_13);
        mg.bonus_timer = MG_BONUS_OUTRO;
    }
}

/*
 * Through the gate. The guardian's lair isn't further down the road: the
 * open gate is a door to it. She walks into the doorway, sparks rise
 * around her, she brightens until she is only light and is gone; the
 * whole valley fades to white, the lair is set up behind it, and as the
 * colour comes back she steps out of a ring of gold, facing the guardian.
 * Counted down in mg.state_timer.
 */
enum {
    MG_WARP_TIME = 140,      /* walking into the doorway until 111      */
    MG_WARP_GLOW = 110,      /* she brightens until 91                  */
    MG_WARP_GONE = 90,       /* only light left: she vanishes           */
    MG_WARP_WHITE = 70,      /* the screen is white from here           */
    MG_WARP_SWAP = 69,       /* the lair is set up behind it            */
    MG_WARP_APPEAR = 56,     /* she steps out of the light              */
    MG_WARP_CLEAR = 45,      /* the colour is fully back                */
};

static void NEOGEO_USER mg_warp_begin(void)
{
    NGCharacter *p = mg.player;
    uint8_t i;
    mg.state = MG_WARP;
    mg.state_timer = MG_WARP_TIME;
    mg.hurt = 0; mg.hurt_lit = 0; mg.dash = 0; mg.super_surge = 0;
    mg.attack = 0; mg.flash = 0; mg.veil = 0;
    mg_climb_end();
    p->visible = 1;
    p->vx_fp = 0;
    mg.facing = 0;
    mg_palette(PAL_HERO, mg_hero_normal_pal());
    for (i = 0; i < MG_SHOTS; i++) if (mg.shots[i].hostile) mg.shots[i].life = 0;
    ng_fix_clear_rect(1, ROW_HINT, 38, 1, PAL_TEXT);
    playSFX(SOUND_SFX_13);
}

static void NEOGEO_USER mg_warp_swap(void)
{
    NGCharacter *p = mg.player;
    uint8_t i;
    mg_boss_arrive();
    /* The glow on her and on the gate is put back under the white, so both
     * come up in their own colours with everything else. */
    mg_palette(PAL_HERO, mg_hero_normal_pal());
    mg_palette(PAL_GATE, mg_gate_pal);
    /* at the arena's left edge, as far from the guardian as it lets her */
    ng_char_set_pos(p, (int16_t)(mg.arena_left + 20), MG_GROUND_Y);
    p->vx_fp = 0;
    p->vy_fp = 0;
    mg.facing = 0;
    mg.shake = 0;
    mg.shake_x = 0;
    ng_camera_snap(&mg.camera, mg.arena_left, 0);
    ng_level_set_scroll(mg.arena_left, 0);
    for (i = 0; i < MG_SHOTS; i++) {
        mg.shots[i].life = 0;
        ng_sprite_group_set_visible(&mg.shots[i].sprite, 0);
        ng_sprite_group_flush(&mg.shots[i].sprite);
    }
    for (i = 0; i < MG_ITEMS; i++) {
        mg.items[i].life = 0;
        ng_sprite_group_set_visible(&mg.items[i].sprite, 0);
        ng_sprite_group_flush(&mg.items[i].sprite);
    }
    mg.hud_dirty = 1;
}

static void NEOGEO_USER mg_warp_frame(void)
{
    NGCharacter *p = mg.player;
    const MGLevel *level = &mg_levels[mg.stage];
    uint16_t t = mg.state_timer;
    int16_t door = (int16_t)(level->gate_x + 16);

    p->vx_fp = 0;
    if (t > MG_WARP_WHITE) {
        /* The road is still live around her until the white comes down. */
        mg_update_entities();
        if (t > MG_WARP_GLOW) {
            if (p->x != door) {
                ng_char_set_pos(p, (int16_t)(p->x + (p->x < door ? 1 : -1)), p->y);
                mg_frame(p, (uint8_t)(MG_F_WALK0 + ((mg.tick / 5) & 7)), (uint8_t)(p->x > door));
            } else {
                mg_frame(p, MG_F_IDLE0, 0);
            }
        } else if (t > MG_WARP_GONE) {
            uint8_t k = (uint8_t)(((MG_WARP_GLOW - t) * 16u) / (MG_WARP_GLOW - MG_WARP_GONE - 1));
            if (t == MG_WARP_GLOW) playSFX(SOUND_SFX_6);
            mg_frame(p, MG_F_CAST1, 0);
            mg_whiten_bank(PAL_HERO, mg_hero_normal_pal(), k);
            mg_whiten_bank(PAL_GATE, mg_gate_pal, (uint8_t)(k / 2u));
        } else if (t == MG_WARP_GONE) {
            mg_burst(p->x, (int16_t)(p->y - 30), MG_T_SPARK, 4, -4);
            playSFX(SOUND_SFX_12);
        }
        /* Sparks rise out of the doorway the whole time she's in it. */
        if ((t % 3u) == 0u)
            mg_burst((int16_t)(door + (int16_t)(mg_rand() % 28u) - 14), (int16_t)(MG_GROUND_Y - 8 - (mg_rand() & 31)),
                     MG_T_SPARK, 1, -3);
        if (t <= MG_WARP_GONE) p->visible = 0;
        if (t < MG_WARP_GONE) {
            if (t == MG_WARP_GONE - 1) mg_fade_begin();
            if ((t & 1u) == 0u)
                mg_fade_set((uint8_t)(((MG_WARP_GONE - 1 - t) * 16u) / (MG_WARP_GONE - 1 - MG_WARP_WHITE)));
        }
    } else {
        if (t == MG_WARP_WHITE) mg_fade_set(16);
        if (t == MG_WARP_SWAP) mg_warp_swap();
        if (t < MG_WARP_SWAP && t >= MG_WARP_CLEAR && (((t & 1u) == 0u) || t == MG_WARP_CLEAR))
            mg_fade_set((uint8_t)(((t - MG_WARP_CLEAR) * 16u) / (MG_WARP_SWAP - 1 - MG_WARP_CLEAR)));
        p->visible = (uint8_t)(t <= MG_WARP_APPEAR);
        if (t == MG_WARP_APPEAR) {
            mg_secret_art_ring(p->x, (int16_t)(p->y - 30));
            playSFX(SOUND_SFX_12);
        }
        mg_frame(p, (uint8_t)(t > MG_WARP_APPEAR - 8 ? MG_F_LAND : MG_F_IDLE0), 0);
        if (mg.boss) {
            mg_frame(mg.boss, MG_BF_IDLE, 1);
            mg.boss->vx_fp = 0;
        }
    }
    mg_world_step();
    if (mg.hud_dirty && t < MG_WARP_CLEAR) {
        mg_update_hud();
        mg.hud_dirty = 0;
    }
    if (--mg.state_timer == 0) {
        if (mg.boss) mg_boss_announce();
        else mg.state = MG_PLAY;
    }
}

/*
 * The continue card.  On the arcade board it asks for a coin the way a
 * cabinet does; on the console there is no coin slot, so it offers the
 * choice on the FIX layer and the clock chooses EXIT if nobody does.
 */
static void NEOGEO_USER mg_continue_card(void)
{
    ng_fix_clear_rect(1, ROW_CARD, 38, 13, PAL_TEXT);
    mg_centre(ROW_CARD + 2, "GAME OVER", PAL_WARN);
    mg_centre(ROW_CARD + 4, "CONTINUE?", PAL_GOLD);
#ifdef NG_AES
    ng_fix_puts(10, ROW_CARD + 8, mg.over_pick == 0 ? ">" : " ", PAL_GOLD);
    ng_fix_puts(12, ROW_CARD + 8, "CONTINUE", mg.over_pick == 0 ? PAL_GOLD : PAL_TEXT);
    ng_fix_puts(23, ROW_CARD + 8, mg.over_pick == 1 ? ">" : " ", PAL_GOLD);
    ng_fix_puts(25, ROW_CARD + 8, "EXIT", mg.over_pick == 1 ? PAL_GOLD : PAL_TEXT);
    mg_centre(ROW_CARD + 10, "SAVE THE EARTH: PRESS A BUTTON", PAL_SKY);
#else
    if (read_p1credit() > 0) {
        mg_centre(ROW_CARD + 8, "PRESS START: SAVE THE EARTH", PAL_GOLD);
    } else {
        mg_centre(ROW_CARD + 8, "PLEASE INSERT COIN", PAL_TEXT);
        mg_centre(ROW_CARD + 10, "SAVE THE EARTH", PAL_SKY);
    }
#endif
    ng_fix_puts(14, ROW_CARD + 12, "CONTINUES LEFT", PAL_SKY);
    mg_number(29, ROW_CARD + 12, mg.continues, 1, PAL_GOLD);
    mg_number(19, ROW_CARD + 6, (mg.state_timer + 59u) / 60u, 2, PAL_WARN);
}

void NEOGEO_USER maiya_frame(void)
{
    mg_fade_commit();      /* first thing after the blank: see mg_fade_set() */
    mg.tick++;
    if (!mg.player) return;
    mg_animate_water();

    if (mg.state == MG_INTRO) {
        /* Mission card: the world is live behind it, any button skips. */
        uint16_t joy = mg_input();
        uint16_t pressed = (uint16_t)(joy & (uint16_t)(~mg.previous_joy));
        mg.previous_joy = joy;

        mg.player->vx_fp = 0;
        mg_spawn();
        if (mg.entrance) {
            /* She is still falling in: let her land before anything else. */
            if (ng_physics_is_grounded(mg.player) || mg.on_ledge ||
                mg.player->y >= MG_GROUND_Y - 2) {
                mg.entrance = 0;
                playSFX(SOUND_SFX_9);
                mg.shake = 8;
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
            ng_fix_clear_rect(1, ROW_CARD, 38, 9, PAL_TEXT);
            mg.state = MG_PLAY;
            mg.state_timer = 0;
        }
        return;
    }

    if (mg.state == MG_PLAY) {
        if (ng_feedback_is_hitstop()) {
            mg_world_step();
            return;
        }
        mg_controls();
        mg_spawn();
        mg_animate_player();
        mg_world_step();
        mg_update_entities();
        mg_hazard_check();
        mg_hazard_warn_check();
        mg_npc_check();
        mg_rescue_check();

        mg_clock_tick();
        mg_bars_step();
        mg_stage_mechanics();
        /* Fell into a pit: that's the life, however much health is left. */
        if (mg.player->y > MG_GROUND_Y + 20 && mg.state == MG_PLAY) {
            mg.hurt = 0; mg.veil = 0; mg.dash = 0; mg.super_surge = 0;
            mg.player->hp = 1;
            mg_player_damage();
        }
        if (mg.hint_timer && --mg.hint_timer == 0) {
            ng_fix_clear_rect(1, ROW_HINT, 38, 1, PAL_TEXT);
        }
        if (mg.hud_dirty) {
            mg_update_hud();
            mg.hud_dirty = 0;
        }
        /* The FIX follows the game: score the moment it moves, the tray
         * once a second while a power runs down. */
        if (mg.score != mg.score_shown) {
            mg_number(28, ROW_SCORE, mg.score, 6, PAL_TEXT);
            mg.score_shown = mg.score;
        }
        /* Real skill earns more lives than the map ever hands out: a
         * milestone every so many points, on top of the capped hidden
         * pickup, however far that's already been spent. */
        if (mg.score >= mg.next_life_score) {
            mg.next_life_score += MG_BONUS_LIFE_SCORE_STEP;
            if (mg.lives < MAX_LIVES) {
                mg.lives++;
                mg.hud_dirty = 1;
                playSFX(SOUND_SFX_13);
                playSFX(SOUND_SFX_12);
                mg_hint("EXTRA LIFE!", PAL_GOLD, 120);
            }
        }
        if ((mg.swift || mg.might || mg.veil || mg.spring || mg.crown) &&
            (mg.tick % 60u) == 0u) mg_draw_tray();
        return;
    }

    if (mg.state == MG_CLEAR) {
        /*
         * Guardian defeated. She comes down if she was in the air, takes a
         * breath, hops once for joy and then holds her victory pose -- one
         * pose, held, not flicked back and forth with standing.
         */
        NGCharacter *p = mg.player;
        uint8_t grounded = (uint8_t)(ng_physics_is_grounded(p) || mg.on_ledge || p->y >= MG_GROUND_Y - 2);
        p->visible = 1;
        p->vx_fp = 0;
        if (mg.win_wait) mg.win_wait--;
        if (mg.win_step == 0) {
            mg_frame(p, grounded ? MG_F_IDLE0 : MG_F_JUMP3, mg.facing);
            if (grounded && mg.state_timer <= 196) {
                mg.win_step = 1;
                mg.win_wait = 8;
                p->vy_fp = -3 * NG_FP_ONE;
                playSFX(SOUND_SFX_15);
            }
        } else if (mg.win_step == 1) {
            mg_frame(p, MG_F_WIN, mg.facing);
            if (!mg.win_wait && grounded && p->vy_fp >= 0) {
                mg.win_step = 2;
                mg_burst(p->x, (int16_t)(p->y - 4), MG_T_DUST, 2, -1);
            }
        } else {
            mg_frame(p, MG_F_WIN, mg.facing);
            if ((mg.state_timer % 20u) == 0u)
                mg_burst((int16_t)(p->x + (int16_t)(mg_rand() % 40u) - 20), (int16_t)(p->y - 64),
                         MG_T_SPARK, 1, -1);
        }
        if (mg.boss) {
            NGCharacter *b = mg.boss;
            if (!mg.boss_down && b->vy_fp >= 0 && b->y >= MG_GROUND_Y - 2) {
                /* It hits the road and rolls onto its back, still. */
                mg.boss_down = 1;
                b->vx_fp = 0;
                mg_frame(b, MG_BF_DEAD, b->flip_x);
                mg.shake = 8;
                playSFX(SOUND_SFX_10);
                mg_burst(b->x, (int16_t)(b->y - 8), MG_T_DUST, 4, -1);
            }
            if (mg.boss_down) {
                b->vx_fp = 0;
                if ((mg.state_timer % 14u) == 0u && mg.state_timer > 40u)
                    mg_burst(b->x, (int16_t)(b->y - 40), MG_T_DUST, 1, -2);
                /* At the very end it darkens away into the ground rather
                 * than blinking out. */
                if (mg.state_timer <= 40u && (mg.state_timer % 10u) == 0u)
                    mg_shade_bank(PAL_BOSS, mg_boss_pal((uint8_t)b->data0), (uint8_t)(mg.state_timer / 14u), 1);
                if (mg.state_timer <= 2u) b->visible = 0;
            }
        }
        mg_world_step();
        if (mg.hud_dirty) {
            mg_update_hud();
            mg.hud_dirty = 0;
        }
        if (--mg.state_timer == 0) {
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
                ng_fix_clear_rect(1, ROW_CARD, 38, 13, PAL_TEXT);
                mg_centre(ROW_CARD + 2, "CONGRATULATIONS!", PAL_GOLD);
                mg_centre(ROW_CARD + 4, "EARTH IS RESTORED", PAL_SKY);
                mg_centre(ROW_CARD + 6, "MAIYA AND SUNBOY SAVED THE VALLEY", PAL_TEXT);
                mg_centre(ROW_CARD + 10, "EAGLE SOFTWARE 1996", PAL_GOLD);
            }
        }
        return;
    }

    if (mg.state == MG_DEAD) {
        /*
         * She goes down on one knee for a breath; then the valley lifts her
         * -- the halo settles over her head, her colours turn to sunlight,
         * and she rises out of frame, arms open, with sparks falling from
         * her all the way up.
         */
        NGCharacter *p = mg.player;
        p->visible = 1;
        if (mg.state_timer > ANGEL_TIME) {
            mg_frame(p, (uint8_t)(mg.state_timer > ANGEL_TIME + 20 ? MG_F_HURT1 : MG_F_DOWN), mg.facing);
        } else {
            if (mg.state_timer == ANGEL_TIME) {
                mg_palette(PAL_HERO, mg_hero_sun_pal);
                playSFX(SOUND_SFX_13);
            }
            mg_frame(p, (uint8_t)((mg.tick / 14) & 1 ? MG_F_WIN : MG_F_JUMP3), mg.facing);
            ng_char_set_pos(p, p->x, (int16_t)(p->y - 1));
            if ((mg.tick & 15) == 0) mg_sparks(p->x, (int16_t)(p->y - 10));
            /* the halo rides two pixels above her hair */
            ng_sprite_group_set_pos(&mg.hud[10], (int16_t)(p->x - mg.camera.x - 8),
                                    (int16_t)(p->y - 72 + ((mg.tick >> 3) & 1)));
            ng_sprite_group_set_visible(&mg.hud[10], 1);
            ng_sprite_group_flush(&mg.hud[10]);
        }
        mg_world_step();
        if (--mg.state_timer == 0) {
            ng_sprite_group_set_visible(&mg.hud[10], 0);
            ng_sprite_group_flush(&mg.hud[10]);
            mg.angel = 0;
            if (mg.lives > 0) {
                mg_scene(mg.stage, 1);
            } else if (mg.continues) {
                /* The valley waits: ten seconds, and one of three continues. */
                mg.state = MG_OVER;
                mg.state_timer = CONTINUE_TIME;
                mg.over_pick = 0;
                NEO_REGISTER8(BIOS_PLAYER1_MODE) = 2;
                mg_continue_card();
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

    if (mg.state == MG_WARP) {
        mg_warp_frame();
        return;
    }

    if (mg.state == MG_BOSS_INTRO) {
        /* She holds her ground and hears him out before the fight opens. */
        uint16_t joy = mg_input();
        uint16_t pressed = (uint16_t)(joy & (uint16_t)(~mg.previous_joy));
        mg.previous_joy = joy;

        mg.player->vx_fp = 0;
        mg_frame(mg.player, MG_F_IDLE0, mg.facing);
        if (mg.boss) {
            mg.boss->vx_fp = 0;
            mg_frame(mg.boss, (uint8_t)((mg.tick / 20) % 2), mg.facing);
        }
        mg_world_step();

        if (--mg.state_timer == 0 ||
            (pressed & (BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_D))) {
            ng_fix_clear_rect(1, ROW_CARD, 38, 9, PAL_TEXT);
            mg.state = MG_PLAY;
            mg.state_timer = 0;
        }
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
#ifdef NG_AES
        uint16_t pressed = (uint16_t)(joy & (uint16_t)(~mg.previous_joy));
#endif
        uint8_t go = 0;
        mg.previous_joy = joy;

        mg.player->vx_fp = 0;
        mg.player->visible = 0;      /* she has already gone up; the road waits */
        mg_world_step();

        /* Keep ten visible before the last nine seconds count down. */
        if ((mg.state_timer % 60u) == 0u) {
            mg_number(19, ROW_CARD + 6, (uint32_t)(mg.state_timer / 60u), 2, PAL_WARN);
            if (mg.state_timer) playSFX(SOUND_SFX_6);
        }

#ifdef NG_AES
        /* Console: pick CONTINUE or EXIT; the clock picks EXIT for you. */
        if (pressed & (JOY_LEFT | JOY_RIGHT | JOY_UP | JOY_DOWN)) {
            mg.over_pick ^= 1u;
            mg_continue_card();
            playSFX(SOUND_SFX_11);
        }
        if (!mg.demo && (pressed & (BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_D))) {
            if (mg.over_pick == 0) go = 1;
            else mg.state_timer = 1;
        }
#else
        /*
         * Arcade: a coin on the board, then Start, buys the run back.
         *
         * Start never reaches this loop as a joystick bit -- it is
         * BIOS-level, not part of the P1 word -- so this does not poll for
         * it directly.  Entering this state sets BIOS_PLAYER1_MODE to 2,
         * and the BIOS's own VBlank IO poll watches Start on its own: on a
         * real press with a credit in, it takes the credit, calls our
         * PLAYER_START back, and PLAYER_START sets the mode to 1.  Seeing 1
         * here again is exactly "she paid and pressed Start."
         */
        if (!mg.demo && NEO_REGISTER8(BIOS_PLAYER1_MODE) == 1) go = 1;
        if (((mg.state_timer & 31u) == 0u) && !mg.demo) mg_continue_card();
#endif

        if (go) {
            mg.continues--;
            mg.lives = 3;
            mg.art = MAX_ART;
            NEO_REGISTER8(BIOS_PLAYER1_MODE) = 1;
            ng_fix_clear_rect(1, ROW_CARD, 38, 13, PAL_TEXT);
            playSFX(SOUND_SFX_13);
            mg_hint("MAIYA: FOR THE EARTH, ONCE MORE", PAL_GOLD, 150);
            mg_scene(mg.stage, 1);
            return;
        }

        if (mg.state_timer && --mg.state_timer == 0) {
            NEO_REGISTER8(BIOS_PLAYER1_MODE) = 3;
            mg.session_over = 1;
            mg.state = MG_DONE;
            ng_fix_clear_rect(1, ROW_CARD, 38, 13, PAL_TEXT);
            mg_centre(ROW_CARD + 4, "GAME OVER", PAL_WARN);
            mg_centre(ROW_CARD + 6, "THE EARTH STILL WAITS FOR YOU", PAL_SKY);
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
