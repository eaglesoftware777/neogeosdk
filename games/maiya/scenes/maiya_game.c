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
#include "maiya_feel.h"     /* hitstop and shake: every tuning value */
#include <stddef.h>

#pragma GCC optimize ("O2")

void NEOGEO_USER waitVbl(void);

volatile uint8_t maiya_console_start;   /* Start pressed: see user.c */

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

/* Overlapping copies (the compiler's own for shifting the score table
 * down a line): from the end when moving a block up in memory. */
void *NEOGEO_USER memmove(void *destination, const void *source, size_t count)
{
    volatile uint8_t *out = (volatile uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    if (out <= in) {
        while (count--) *out++ = *in++;
    } else {
        out += count;
        in += count;
        while (count--) *--out = *--in;
    }
    return destination;
}

/* ------------------------------------------------------------------ */
/*  Budgets and slots                                                 */
/* ------------------------------------------------------------------ */
enum {
    MG_ENEMIES = 8, MG_SHOTS = 6,
    MG_ROAD_ENEMIES = 4, /* at most this many at once on the road; the sky takes all eight */ MG_SPARKS = 12, MG_ITEMS = 4,
    MG_LEDGE_BLOCKS = 9, MG_HAZARD_BLOCKS = 6, MG_DECOR_SLOTS = 6, MG_SIGNS = 2,
    MG_NPC_SLOTS = 2, MG_FRONT_SLOTS = 3,
    MG_BULLETS = 16,     /* the bonus round's cannon fire (in the ledges' sprites) */

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
    SLOT_SIGN = 254,     /* 2 pit caution signs (2 strips each)     */
    SLOT_HAZARD = 270,   /* 6 blocks (12 strips) fire/spikes/sludge */
    SLOT_DECOR = 83,     /* 6 props behind the character pool      */
    SLOT_VINE = 264,     /* 3 climbing vines (2 strips each)        */
    SLOT_FRONT = 282,    /* 3 foreground props, in front of the cast */
    SLOT_HUD = 290,      /* face avatar, hearts, halo, key          */
    SLOT_TITLE = 310,    /* attract mode key visual                 */
    SLOT_TRAY = 330,     /* 12 shrunk pick-up icons (2 strips each) */
    SLOT_FX = 356,       /* her special moves' light (4 strips), over her */
    SLOT_GLIDER = 360,   /* her hang glider / parachute over a healed valley (6 strips) */
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
    PAL_JELLYFISH = 48, PAL_TOXICCRAB = 49, PAL_ACIDMOTH = 50, PAL_DARTFROG = 51,
    PAL_SMOGBAT = 52, PAL_POACHDRONE = 53, PAL_CHEMFLY = 54, PAL_PLASTICBAT = 55,
    PAL_SLAGGOLEM = 56, PAL_VINESTING = 57, PAL_SPOREGOB = 58, PAL_WRAITH = 59,
    PAL_HAZARD = 61, PAL_FACE = 43, PAL_PIT = 66,
    PAL_BLOCK_ROT = 67,  /* a rotten ledge: the valley's set, greyed and darker */
    PAL_FX = 64,         /* the light of her special moves */
    PAL_FOLK = 48,       /* the healed valley's people, a bank a kind (48..51, the creatures' banks: none are left) */
    /* The Sky Road's fliers. */
    PAL_RHINO = 65, PAL_DRAGONFLY = 68, PAL_GNAT = 69, PAL_GUNSHIP = 70,
    PAL_LANDMARK = 71,   /* the great tree or mountain the gate stands in (71..73) */
    /* The polluters. */
    PAL_BAGOCTO = 74, PAL_BINOCTO = 75, PAL_SAWBOT = 76, PAL_DRILLBOT = 77, PAL_TORCHBOT = 78,
    PAL_SMOGSTACK = 79, PAL_SLUDGEBARREL = 80,
    /* FIX inks for the guardian's bar: dirty and toxic rather than the
     * clean traffic-light colours of her own -- it's the blight's health. */
    PAL_BOSS_HP_HI = 11, PAL_BOSS_HP_MID = 14, PAL_BOSS_HP_LO = 15,

    /* Character kinds. */
    K_PLAYER = 0, K_ENEMY = 1, K_BOSS = 2, K_ALLY = 3, K_EAGLE = 4,

    /* Game states. */
    MG_INTRO = 0, MG_PLAY, MG_CLEAR, MG_BONUS, MG_DEAD, MG_OVER, MG_ENDING, MG_DONE,
    MG_INTERLUDE, MG_BOSS_INTRO, MG_WARP, MG_NAME, MG_TABLE, MG_TOUR,

    HERO_STRIPS = 5, HERO_ROWS = 4, HERO_STRIDE = 5,
    EAGLE_STRIPS = 8, EAGLE_ROWS = 3,
    BOSS_STRIPS = 8, BOSS_ROWS = 6, BOSS_STRIDE = 8,

    WALK_SPEED = 512, DASH_SPEED = 1280, JUMP_SPEED = 5 * NG_FP_ONE + 160,
    /* Hold B to run: faster, and a running jump carries higher. A second
     * press of A in the air is one more, smaller jump. */
    RUN_SPEED = 800, RUN_JUMP_SPEED = 6 * NG_FP_ONE, AIR_JUMP_SPEED = 5 * NG_FP_ONE,
    /* Landing on a creature bounces her; holding A bounces her high -- a
     * creature is a step to somewhere a jump alone won't reach. */
    STOMP_HIGH = 6 * NG_FP_ONE + 128,
    MG_JUMP_GRACE = 6,   /* frames a jump is still taken after leaving a ledge, or before landing */
    /* The Rising Bloom (forward, down, down-forward + B): three frames
     * gathering, then up in a spin of petals, untouchable for the first
     * part of the climb; then a wait before the next. */
    MG_RISE_TIME = 22, MG_RISE_SAFE = 12, MG_RISE_WAIT = 45,
    RISE_SPEED = 6 * NG_FP_ONE + 64,
    CLIMB_SPEED = 320,
    WALK_ACCEL = 112,    /* she leans into a run instead of snapping to it */
    WALK_BRAKE = 96,
    MAX_HP = 5, MAX_LIVES = 7, MAX_ART = 3,
    /* The hidden extra life is one trinket among ten, but unlike the rest
     * it hands out a life -- worth capping across the whole run, not just
     * the one mission it sits in, or a player who farms deaths-and-retries
     * on an early valley could stack lives without ever earning them. */
    MG_LIFE_PICKUP_LIMIT = 3,
    MG_DASH_TIME = 9,    /* a quick burst, not a long slide */
    /* Every mission runs against the clock: at MG_HURRY_AT seconds left the
     * valley warns her, from MG_WRAITH_AT the smog wraiths come for her,
     * and at nought the smog takes the life. */
    MG_LEVEL_SECONDS = 240, MG_HURRY_AT = 40, MG_WRAITH_AT = 25,
    /* Where a flying guardian's feet ride: its 96-pixel body then fills the
     * upper screen instead of hanging above the top edge. */
    MG_BOSS_SKY_Y = 110,
    /* A guardian's health is set in stomps (its stage file's "hp"): a
     * stomp, or the Rising Bloom, takes this much; a thorn takes one. */
    MG_STOMP_BLOW = 4,
    ROW_CLOCK = 3,
    MG_STAGE_TIME_COL = 22,         /* STAGE mm:ss:ff, right of the clock */
    /* What each valley throws at her besides its creatures. */
    MG_M_NONE = 0, MG_M_CRUMBLE = 3, MG_M_ICE = 4, MG_M_WATER = 5, MG_M_FLIGHT = 6,
    /* What lies at the bottom of a valley's pits (its stage file's "pit"). */
    MG_PIT_WATER = 0, MG_PIT_FIRE = 1, MG_PIT_TOXIC = 2, MG_PIT_VOID = 3,
    /* The ledge set a valley's shelves are built of (its stage file's "blocks"). */
    MG_BLOCKS_GRASS = 0, MG_BLOCKS_MOSS = 1, MG_BLOCKS_SAND = 2, MG_BLOCKS_AUTUMN = 3,
    MG_BLOCKS_SNOW = 4, MG_BLOCKS_BARK = 5, MG_BLOCKS_RUST = 6, MG_BLOCKS_CORAL = 7,
    MG_BLOCKS_STONE = 8, MG_BLOCKS_SAVANNA = 9,
    MG_CRUMBLE_AFTER = 45, MG_CRUMBLE_BACK = 180,
    /* The Surge: a beat to gather, then some 100 px of spin (it ran 180
     * and carried her into pits). */
    MG_SURGE_TIME = 22, MG_SURGE_WINDUP = 6, MG_SURGE_SPEED = 1600,
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
    ROW_SCORE = 2, ROW_LIVES = 4, ROW_POWER = 5, ROW_COMBO = 6, ROW_HINT = 7, ROW_CARD = 8,
    MG_PAUSE_ROW = 12,               /* PAUSE, in the mission card's space */

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
    MG_CAM_LEAD_RATE = 1, /* ... swinging over this many px a frame        */
    MG_CAM_DEAD = 16,    /* she moves this far either way before it follows */
    MG_CAM_FOLLOW = 255, /* and then it follows exactly (whole pixels)    */
    MG_BOSS_TOUCH = 36,  /* closer than this on the ground, a guardian hurts */
    MG_BOSS_TOUCH_AIR = 24, /* in the air only its body does: she can jump it */
    MG_BOSS_BACKOFF = 40,   /* frames a guardian gives ground after a touch */
    MG_MOOD_DYING = 0xEE, /* a beaten creature on its way off the screen   */
    MG_MOOD_STUNNED = 0xE0, /* armour stomped: dazed, stars round its head    */
    MG_MOOD_SHELL = 0xE1,   /* ...and kicked: sliding, bowling others over   */
    SIT_DELAY = 70,      /* frames of crouching before Maiya sits down    */
    TALK_RANGE = 34,     /* how close a villager will speak up            */
    POWER_TIME = 480,    /* swiftness and might last eight seconds        */
    STOMP_KICK = 4 * NG_FP_ONE,  /* the hop she takes off a squashed slime */
    VEIL_TIME = 300,     /* the mist veil hides her for five              */
    LILY_TIME = 1800,    /* a sky lily's second jump lasts thirty seconds  */
    /* Kneel (Down held a moment), then Up + A: a leap straight up, well
     * above any running jump -- the way to what hangs out of reach. */
    MG_LEAP_KNEEL = 8, MG_LEAP_WINDOW = 14, LEAP_SPEED = 7 * NG_FP_ONE + 96,
    ANGEL_TIME = 150,    /* how long she rises before the valley resets   */
    MAX_CONTINUES = 3,   /* the cabinet allows three, then the run is over */
    CONTINUE_TIME = 600  /* ten seconds on the clock to decide            */
};

/* A shot of the bonus round's cannon: position and speed in 1/16 px. */
typedef struct {
    int16_t x, y, vx, vy;
    uint8_t life;
} MGBullet;

typedef struct {
    NGCharacter *body;
    uint16_t timer;
    uint8_t type, hurt, posted;
    uint8_t mood;          /* 0 unaware, 1 hunting, 2+ a creature-specific move in progress */
    uint8_t move_timer;    /* frames left in the current move                              */
    int8_t  heading;       /* direction a committed move (charge, dive) is locked to        */
    int8_t  face;          /* which way it faces: only turns once she's clearly past it     */
    int16_t home;          /* where it was placed: the centre of its patrol                */
    uint8_t form, slot_i;  /* flying in a wave: its formation (MG_FORM_*) and place in it  */
    int16_t base_y;        /* the wave's height                                            */
    uint16_t age;          /* frames since its wave came                                   */
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
    NGSpriteGroup signs[MG_SIGNS];   /* their own sprites: a sign never waits for a hazard block */
    NGSpriteGroup decor[MG_DECOR_SLOTS];
    NGSpriteGroup vines[MG_VINE_COUNT];
    NGSpriteGroup front[MG_FRONT_SLOTS];
    NGSpriteGroup gate;
    const uint16_t *block_tiles;
    NGCharacter *npcs[MG_NPC_SLOTS];
    NGSpriteGroup cage;
    NGSpriteGroup fx;                /* a special move's light, drawn over her */
    NGSpriteGroup glider;            /* her hang glider, then parachute, over a healed valley */
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
    uint16_t hazard_warn_mask;         /* one bit per hazard: already warned */
    uint16_t hazard_disabled_mask;     /* one bit per hazard: shut off for good */
    uint16_t boss_timer, state_timer, clear_bonus;
    int16_t  boss_home;
    int16_t arena_left, boss_direction;
    MGPlatform arena[2];
    uint16_t walk_distance;
    uint8_t climb_cooldown;
    uint8_t  stage, state, lives, art, kills, rescue_mask;
    uint8_t  hurt, coyote, jump_buffer, drop, boss_hurt, boss_active;
    uint8_t  attack, combo, dash, dash_wait, cast, super_surge, sitting;
    uint8_t  facing, notice, hud_dirty, session_over;
    uint8_t  has_key, gate_unlocked, gate_shown, key_taken;
    uint8_t  climbing, crouch_timer, npc_mask, npc_live, npc_here;
    uint16_t swift, might, veil;      /* power-ups, in frames              */
    uint16_t spring, crown;           /* higher jump, bigger thorns        */
    uint16_t lily;                    /* the sky lily: a second jump in the air, in frames */
    uint8_t  leap_window;             /* frames left to leap high after kneeling */
    uint8_t  voice_next, voice_delay; /* a line waiting to be spoken, and when */
    uint8_t  fx_kind, fx_time;        /* the light over her (MG_LIGHT_*) and for how long */
    uint8_t  flying;                  /* the Sky Road: on the eagle's back          */
    int16_t  fly_x;                   /* how far the sky has carried the view       */
    uint32_t wave_mask;               /* the spawn script's waves already sent      */
    uint8_t  ship_part[3];            /* the dreadnought: its stacks' and bridge's health */
    uint8_t  ship_target;             /* the part a blow is aimed at (0xFF: the next one) */
    uint8_t  ship_z;                  /* its distance as it comes in (0: arrived)   */
    uint8_t  rush_i;                  /* the returning guardian being fought (0xFF: the stage's own) */
    uint8_t  boss_style_now;          /* the guardian in the arena now (MG_B_*)     */
    uint8_t  boss_phase;              /* Lord Smoggar's last stand: 0, 1, 2         */
    uint8_t  end_page;                /* the ending: which page                     */
    uint16_t end_timer;               /* ...and how long it has been up             */
    uint8_t  combo_n, combo_t, combo_show; /* creatures beaten in a row, time left to add one, its read-out */
    uint8_t  name_buf[3], name_pos, name_row;  /* the high score name being entered, and its place */
    uint8_t  music_next, music_wait;  /* a track waiting for the fade out to finish; the fade under way (1 out, 2 in) */
    uint8_t  music_level;             /* the music's volume now, as the fade has it */
    uint8_t  art_pose, leaping;       /* the Secret Art's pose, frames left; rising from the high leap */
    uint8_t  arts_known;              /* the Secret Arts she has learned, a bit each (MG_ART_*) */
    uint8_t  veil_lit;                /* her mist-veil colours are on               */
    uint8_t  tour_phase, tour_folk_n; /* the healed valley: 0 flying over it, 1 with the elder */
    uint16_t tour_t;                  /* ...frames into the phase                    */
    int16_t  tour_x;                  /* ...where the view has got to                */
    NGCharacter *tour_folk[8];        /* ...the people freed, out on the road; the elder last */
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
    uint8_t  bonus_life, bonus_safe;  /* hits she can still take in the round; frames she is safe after one */
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
    int8_t   cam_dir;                /* the way she last really ran: the look-ahead's side */
    uint8_t  difficulty;             /* the operator's setting: 0 easy .. 3 expert     */
    uint8_t  boss_backoff;           /* a guardian that just struck her steps back    */
    uint16_t held_press;             /* buttons pressed during a hitstop, not yet seen */
    uint32_t time_seen;              /* the stage clock as the HUD last counted it    */
    uint8_t  time_digit[6];          /* m m s s f f: carried a frame at a time          */
    char     time_shown[14];         /* what the HUD's STAGE line shows now             */
    uint32_t kinds_met;              /* creature kinds she has met this game (MG_E_*)   */
    uint8_t  swimming;               /* steered through the water (the reef's road)     */
    uint8_t  air_jump;               /* 1 while her second jump is still unspent         */
    uint8_t  air_dash;               /* 1 while her dash in the air is still unspent     */
    uint8_t  spin;                   /* frames left of the second jump's spin            */
    uint8_t  stomp_chain;            /* creatures landed on since she last touched ground */
    uint8_t  rising, rise_wait;      /* the Rising Bloom: frames left, frames to the next */
    uint8_t  rise_hit;               /* 1 once it has struck the guardian this time      */
    uint8_t  dp_step, dp_timer;      /* how far into forward, down, down-forward she is  */
    int8_t   dp_dir;                 /* ...and which way "forward" was                   */
    uint8_t  jump_cut;               /* 1 while letting go of A can still cut this jump  */
    uint8_t  rain_timer;             /* frames to the arena's next thing from above      */
    int16_t  rain_x;                 /* ...and where it will fall (0: nothing warned yet) */
    int16_t  arena_home;             /* the sliding arena ledge's centre                 */
    uint8_t  vault;                  /* 1 while she is in the valley's hidden vault      */
    uint8_t  vault_done, charm_seen; /* the vault visited; the elder's charm found       */
    uint8_t  vault_left;             /* treasures still to come in the vault             */
    uint16_t vault_timer;            /* frames left in the vault                         */
    int16_t  cam_y;                  /* the view's height: below 0 up a tall climb       */
    MGBullet bullets[MG_BULLETS];    /* the bonus round's cannon fire                   */
    NGSpriteGroup bullet_spr[MG_BULLETS];
    NGSpriteGroup cannon;            /* the blight cannon hovering over the bonus field */
    uint8_t  cannon_angle;           /* where its next ring of fire starts              */
    uint8_t  bonus_coins;            /* her coin count as the round began               */
} MGState;

static MGState mg;

/* A world height on the screen, with the view raised up a tall climb. */
#define MG_SY(y) ((int16_t)((y) - mg.cam_y))

/* ------------------------------------------------------------------ */
/*  Math helpers (random numbers: the engine's ng_rand)               */
/* ------------------------------------------------------------------ */
static int16_t NEOGEO_USER mg_abs(int16_t value)
{
    return value < 0 ? (int16_t)-value : value;
}

/* ------------------------------------------------------------------ */
/*  Palettes and text                                                 */
/* ------------------------------------------------------------------ */
/*
 * Every colour goes through the engine's palette screen (ng_palette_fx.h):
 * each bank she loads is kept in mg_pal_base, shown from mg_pal_out, and
 * reaches palette RAM in the vertical blank, in maiya_vblank(). Her fades
 * -- the white-out through a guardian's gate, the chooser rising out of
 * white -- lift all of it together, and a bank loaded behind a fade comes
 * up with it instead of flashing through.
 */
#define MG_PAL_BANKS 81u   /* up to the last polluter's bank */
static uint16_t mg_pal_base[MG_PAL_BANKS * 16u];
static uint16_t mg_pal_out[MG_PAL_BANKS * 16u];
static uint8_t mg_pal_open;
static uint8_t mg_tint_t, mg_tint_art;   /* the Secret Art's colour shake: frames left, which art */

/* The screen is taken over on first use, from the colours showing then. */
static void NEOGEO_USER mg_pal_screen(void)
{
    if (mg_pal_open) return;
    mg_pal_open = 1;
    ng_palfx_screen_init(mg_pal_base, mg_pal_out, MG_PAL_BANKS);
}

/* Sleep only in the normal supervisor game loop. Masking the flag check
 * prevents a VBlank between the test and STOP from delaying another frame.
 * STOP atomically restores interrupt acceptance before sleeping. */
static void NEOGEO_USER mg_wait_vblank(void)
{
    uint16_t status;
    __asm__ volatile ("move.w %%sr,%0" : "=d" (status));
    if ((status & 0xe700u) != 0x2000u) {
        waitVbl();
        return;
    }
    __asm__ volatile (
        "move.w %%sr,%%d0\n\t"
        "move.w #0x2700,%%sr\n\t"
        "1: tst.w %c[flag]\n\t"
        "bne.s 2f\n\t"
        "stop #0x2000\n\t"
        "move.w #0x2700,%%sr\n\t"
        "bra.s 1b\n\t"
        "2: clr.w %c[flag]\n\t"
        "addq.l #1,%c[counter]\n\t"
        "move.w %%d0,%%sr\n\t"
        : : [flag] "i" (USER_WORKRAM),
            [counter] "i" (USER_WORKRAM + 32)
        : "d0", "cc", "memory");
}

/* The frame boundary: the vertical blank, and in it the colours that
 * changed since the last one. Every frame of hers passes through here. */
void NEOGEO_USER maiya_vblank(void)
{
    mg_wait_vblank();
    ng_palfx_vblank();
}

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

static void NEOGEO_USER mg_palette(uint8_t bank, const uint16_t *colors)
{
    mg_pal_screen();
    ng_palfx_screen_load(bank, colors);
}

static void NEOGEO_USER mg_backdrop(uint16_t color)
{
    mg_pal_screen();
    ng_palfx_screen_backdrop(color);
}

/* Luna's light: a bank with its red and blue swapped -- Maiya's gold turns
 * sky blue, her rose lilac. */
static void NEOGEO_USER mg_moon_bank(uint8_t bank, const uint16_t *src)
{
    uint16_t out[16];
    uint8_t i;
    for (i = 0; i < 16u; i++) {
        uint16_t c = src[i];
        out[i] = (uint16_t)((c & 0x80F0u) | ((c >> 8) & 0x000Fu) | ((c & 0x000Fu) << 8) |
                            ((c >> 2) & 0x1000u) | ((c & 0x1000u) << 2) | (c & 0x2000u));
    }
    mg_palette(bank, out);
}

/* One bank lifted toward white, for a glow on a single sprite. */
static void NEOGEO_USER mg_whiten_bank(uint8_t bank, const uint16_t *src, uint8_t k)
{
    uint8_t lut[32];
    uint16_t out[16];
    mg_lut_for(lut, k);
    mg_blend(out, src, lut);
    mg_palette(bank, out);
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

/* value / 10, and value % 10 in *rem, by two 68000 divides: the upper
 * half first, then its remainder with the lower half (the quotient then
 * fits a word). A 32-bit division from the library costs several times as
 * much, and a HUD number takes one a digit. */
static uint32_t NEOGEO_USER mg_div10(uint32_t value, uint8_t *rem)
{
    uint16_t hi = (uint16_t)(value >> 16);
    uint32_t mid = ((uint32_t)(uint16_t)(hi % 10u) << 16) | (uint16_t)value;
    __asm__ ("divu.w #10,%0" : "+d" (mid));
    *rem = (uint8_t)(mid >> 16);
    return ((uint32_t)(uint16_t)(hi / 10u) << 16) | (uint16_t)mid;
}

static void NEOGEO_USER mg_number(uint8_t x, uint8_t y, uint32_t value, uint8_t digits, uint8_t pal)
{
    char text[10];
    uint8_t i, d;
    text[digits] = 0;
    for (i = digits; i--;) {
        value = mg_div10(value, &d);
        text[i] = (char)('0' + d);
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


/* The road's music levels (ADPCM-B, SSG, FM), for a pause to put back. */
static void NEOGEO_USER mg_music_levels(void)
{
    if (mg.demo && !maiya_dip_demo_sound()) ng_pause_set_music_levels(0x00, 0x00, 0x00);
    else ng_pause_set_music_levels(0xB8, 0x00, 0x00);
}

enum { MG_MUSIC_LEVEL = 0xB8 };   /* the music's volume under the effects (ADPCM-B) */

static void NEOGEO_USER mg_music(uint8_t track)
{
    mg.music_wait = 0;        /* a switch still fading out gives way to this one */
    if (mg.music_on && mg.music_track == track) return;
    mg.music_track = track;
    mg.music_on = 1;
    isZ80Ready(); soundSceneReset();
    /* ADPCM-A level is six bits: 64 masks to 0 and mutes every effect.
     * The attract demo stays silent when the operator turned DEMO SOUND off. */
    isZ80Ready();
    if (mg.demo && !maiya_dip_demo_sound()) soundApplyMix(0x00, 0x00, 0x00, 0x00);
    else soundApplyMix(0x3C, MG_MUSIC_LEVEL, 0x00, 0x00);
    mg.music_level = MG_MUSIC_LEVEL;
    mg_music_levels();
    isZ80Ready(); soundSetADPCMBLoop(1);
    isZ80Ready(); playSFXB(track);
}

/*
 * Her voice, and the thanks of those she frees: the voice bank's lines
 * (spoken by games/maiya/tools/make_voices.py), which follow the sixteen
 * effects in the ADPCM-A bank, in its file order.
 */
enum {
    MG_VOICE_RISE, MG_VOICE_SURGE, MG_VOICE_ART, MG_VOICE_LEAP, MG_VOICE_LILY,
    MG_VOICE_FREE, MG_VOICE_START, MG_VOICE_RETRY, MG_VOICE_WIN,
    MG_VOICE_ELDER, MG_VOICE_MAIDEN, MG_VOICE_SPIRIT, MG_VOICE_SUNBOY,
    MG_VOICE_LUNA,      /* Luna's own words for the first nine, in their order */
};

static void NEOGEO_USER mg_voice(uint8_t line)
{
    /* Luna speaks for herself, in her own voice */
    if (mg.hero_choice && line <= MG_VOICE_WIN) line = (uint8_t)(line + MG_VOICE_LUNA);
    isZ80Ready();
    playVoiceSample((uint8_t)(SOUND_SFX_COUNT + line));
}

/* A line spoken a moment from now: after a music change settles, or once
 * she has had her say. */
static void NEOGEO_USER mg_voice_later(uint8_t line, uint8_t frames)
{
    mg.voice_next = line;
    mg.voice_delay = frames;
}

/*
 * A change of music without a cut: the music (ADPCM-B) fades out, the next
 * track starts while it is silent and fades back in -- a step of the
 * music's volume every other frame, through the SDK's volume wrapper, so
 * the effects stay at their own level throughout. Used for the guardian's
 * theme. The pause mutes the music the same way and gives it back.
 */
enum { MG_FADE_STEP = 16 };

static void NEOGEO_USER mg_music_to(uint8_t track)
{
    if (!mg.music_on || mg.music_track == track || mg.demo) { mg_music(track); return; }
    mg.music_next = track;
    mg.music_wait = 1;                         /* fading out */
}

static void NEOGEO_USER mg_music_tick(void)
{
    if (!mg.music_wait || (mg.tick & 1u)) return;
    if (mg.music_wait == 1) {
        mg.music_level = (uint8_t)(mg.music_level > MG_FADE_STEP ? mg.music_level - MG_FADE_STEP : 0);
        soundSetADPCMBVolume(mg.music_level);
        if (!mg.music_level) {
            mg.music_track = mg.music_next;
            isZ80Ready(); soundSetADPCMBLoop(1);
            isZ80Ready(); playSFXB(mg.music_next);
            mg.music_wait = 2;                 /* ...and in */
        }
    } else {
        mg.music_level = (uint8_t)(mg.music_level + MG_FADE_STEP >= MG_MUSIC_LEVEL ? MG_MUSIC_LEVEL
                                                                               : mg.music_level + MG_FADE_STEP);
        soundSetADPCMBVolume(mg.music_level);
        if (mg.music_level == MG_MUSIC_LEVEL) mg.music_wait = 0;
    }
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

    mg_tint_t = 0;          /* a new painting: an art's colour shake is over */

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
    case 7:  /* Sunken Reef */
        far_tile = MG_BG7_TILE; road_tile = MG_GROUND7_TILE;
        pal = restored ? mg_bg7_pal : mg_bg7_blight_pal;
        far_map = mg_bg7_map; road_map = mg_ground7_map; count = MG_BG7_BANKS;
        break;
    case 8:  /* Silver Cave */
        far_tile = MG_BG8_TILE; road_tile = MG_GROUND8_TILE;
        pal = restored ? mg_bg8_pal : mg_bg8_blight_pal;
        far_map = mg_bg8_map; road_map = mg_ground8_map; count = MG_BG8_BANKS;
        break;
    case 9:  /* Golden Savanna */
        far_tile = MG_BG9_TILE; road_tile = MG_GROUND9_TILE;
        pal = restored ? mg_bg9_pal : mg_bg9_blight_pal;
        far_map = mg_bg9_map; road_map = mg_ground9_map; count = MG_BG9_BANKS;
        break;
    case 10: /* the Sky Road: open sky over the snow peaks */
        far_tile = MG_BG10_TILE; road_tile = MG_GROUND10_TILE;
        pal = restored ? mg_bg10_pal : mg_bg10_blight_pal;
        far_map = mg_bg10_map; road_map = mg_ground10_map; count = MG_BG10_BANKS;
        break;
    case 11: /* the Smog Citadel: Lord Smoggar's works */
        far_tile = MG_BG11_TILE; road_tile = MG_GROUND11_TILE;
        pal = restored ? mg_bg11_pal : mg_bg11_blight_pal;
        far_map = mg_bg11_map; road_map = mg_ground11_map; count = MG_BG11_BANKS;
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
    mg_tint_t = 0;
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
    /* Up a tall climb the painting stays put, its ground band included (it
     * is the painting's own foot: dropped with the road, it would leave a
     * gap under the sky); the ledges and the road's hazards scroll by. */
    ng_sprite_group_set_pos(&mg.far, (int16_t)(-(camera_x / 2) & 511), 0);
    ng_sprite_group_flush(&mg.far);
    ng_sprite_group_set_pos(&mg.road, (int16_t)(-camera_x & 511), MG_GROUND_Y);
    ng_sprite_group_flush(&mg.road);
}

/* Ledge set per valley: forest turf, mossy falls, coast sand, autumn earth,
 * grotto snow, world-tree bark, the works' rust, reef coral, mine stone,
 * savanna earth. */
static const uint8_t mg_stage_blocks[MG_LEVEL_COUNT] = MG_BLOCKS_TABLE;

static const uint16_t *NEOGEO_USER mg_block_set(uint8_t stage, const uint16_t **pal)
{
    switch (mg_stage_blocks[stage]) {
    case MG_BLOCKS_MOSS:    *pal = mg_block_moss_pal;    return mg_block_moss_tiles;
    case MG_BLOCKS_SAND:    *pal = mg_block_sand_pal;    return mg_block_sand_tiles;
    case MG_BLOCKS_AUTUMN:  *pal = mg_block_autumn_pal;  return mg_block_autumn_tiles;
    case MG_BLOCKS_SNOW:    *pal = mg_block_snow_pal;    return mg_block_snow_tiles;
    case MG_BLOCKS_BARK:    *pal = mg_block_bark_pal;    return mg_block_bark_tiles;
    case MG_BLOCKS_RUST:    *pal = mg_block_rust_pal;    return mg_block_rust_tiles;
    case MG_BLOCKS_CORAL:   *pal = mg_block_coral_pal;   return mg_block_coral_tiles;
    case MG_BLOCKS_STONE:   *pal = mg_block_stone_pal;   return mg_block_stone_tiles;
    case MG_BLOCKS_SAVANNA: *pal = mg_block_savanna_pal; return mg_block_savanna_tiles;
    default:                *pal = mg_block_grass_pal;   return mg_block_grass_tiles;
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
static const uint8_t mg_stage_mech[MG_LEVEL_COUNT] = MG_STAGE_MECH_TABLE;   /* each stage file's "mechanic" */

/* What lies at the bottom of each valley's pits. */
static const uint8_t mg_stage_pit[MG_LEVEL_COUNT] = MG_PIT_TABLE;
/* What a valley climbs: vines, a rope ladder, a wooden ladder, a chain,
 * kelp, a frozen vine (its stage file's "climb"). */
static const uint8_t mg_stage_climb[MG_LEVEL_COUNT] = MG_CLIMB_TABLE;
/* How far above the screen a valley's upper tier reaches (its stage file's
 * "upper"); 0: the valley is one screen high. */
static const uint16_t mg_stage_upper[MG_LEVEL_COUNT] = MG_UPPER_TABLE;


/* Who stands posted on a valley's ledges (its stage file's "posted"). */
static const uint8_t mg_stage_posted[MG_LEVEL_COUNT] = MG_POSTED_TABLE;

static const uint16_t *NEOGEO_USER mg_pit_art(const uint16_t **pal)
{
    switch (mg_stage_pit[mg.stage]) {
    case MG_PIT_FIRE:  *pal = mg_pit_fire_pal;  return mg_pit_fire_tiles;
    case MG_PIT_TOXIC: *pal = mg_pit_toxic_pal; return mg_pit_toxic_tiles;
    case MG_PIT_VOID:  *pal = mg_pit_void_pal;  return mg_pit_void_tiles;
    default:           *pal = mg_pit_water_pal; return mg_pit_water_tiles;
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

/* Underwater she falls slowly and floats through her jumps; swimming, the
 * water's own pull is in the steering (mg_swim), not in gravity. */
static void NEOGEO_USER mg_player_gravity(void)
{
    if (mg.swimming || mg.flying) ng_physics_set_gravity(mg.player, 0, 4 * NG_FP_ONE);
    else if (mg_mech() == MG_M_WATER) ng_physics_set_gravity(mg.player, 34, 3 * NG_FP_ONE);
    else ng_physics_set_gravity(mg.player, 64, 6 * NG_FP_ONE);
}

/*
 * Her stroke through the reef: the stick steers her eight ways, the water
 * holds her back (a push settles at a pixel and a half a frame), still she
 * sinks slowly to the floor (about a quarter of a pixel a frame), and she
 * can't rise past the surface just under the HUD. The tide (current_x) is
 * set each frame.
 */
#define MG_SWIM_TOP 84   /* the highest her feet go: her head just clears the HUD */
static const NGMoveParams mg_swim = { 48, 3, 3 * NG_FP_ONE, -9, 0, 0, MG_SWIM_TOP };

static const MGPlatform *NEOGEO_USER mg_platform(uint8_t index)
{
    if (mg.state == MG_BONUS) return 0;
    if (mg.boss_active || mg.vault) return index < 2 ? &mg.arena[index] : 0;
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
    uint8_t road = (uint8_t)(!mg.boss_active && mg.state != MG_BONUS);
    uint32_t rotten = road ? mg_levels[mg.stage].rotten : 0u;

    for (i = 0; i < MG_PLATFORM_COUNT; i++) {
        const MGPlatform *pl = mg_platform(i);
        uint8_t blocks;
        int16_t scr, y, shake = 0;
        uint8_t rot = (uint8_t)((rotten >> i) & 1u);

        /* A rotten ledge that gave way isn't there to stand on, but is
         * still seen falling for a moment after it goes. */
        if (!pl && road && mg.ledge_gone[i] > MG_CRUMBLE_BACK - 40) pl = &mg_levels[mg.stage].platforms[i];
        if (!pl || !pl->width) continue;
        scr = (int16_t)(pl->x - camera_x);
        if (scr > 336 || (int16_t)(scr + pl->width) < -16) continue;
        y = pl->y;
        if (rot && mg.ledge_gone[i]) {
            uint8_t fall = (uint8_t)(MG_CRUMBLE_BACK - mg.ledge_gone[i]);
            y = (int16_t)(y + ((uint16_t)fall * fall) / 6u);
            if (y > 224) continue;
        } else if (rot && mg.ledge_stand[i] > 12) {
            shake = (int16_t)((mg.tick & 2u) ? 1 : -1);   /* it trembles under her */
        }
        /* Up a tall climb the ledges below are out of sight (and the other
         * way round): they mustn't take blocks from the ones in view. */
        if (MG_SY(y) > 224 || MG_SY(y) < -32) continue;

        blocks = (uint8_t)((pl->width + 16) / 32);
        if (blocks < 2) blocks = 2;
        for (k = 0; k < blocks && used < MG_LEDGE_BLOCKS; k++) {
            int16_t bx = (int16_t)(scr + k * 32 + shake);
            NGSpriteGroup *g = &mg.ledges[used];
            uint8_t piece = k == 0 ? 0 : (k + 1 == blocks ? 2 : 1);

            if (bx > 336 || bx < -32) continue;
            ng_sprite_group_set_tile_base(g, mg.block_tiles[piece]);
            ng_sprite_group_set_palette(g, rot ? PAL_BLOCK_ROT : PAL_BLOCK);
            ng_sprite_group_set_pos(g, bx, MG_SY(y));
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
    uint8_t i, k, used = 0, signs = 0;

    for (i = 0; i < MG_HAZARD_COUNT; i++) {
        const MGHazard *hz = &level->hazards[i];
        uint8_t blocks;
        int16_t scr = (int16_t)(hz->x - camera_x);

        if (!hz->type || (mg.hazard_disabled_mask & (uint16_t)(1u << i))) continue;
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
            ng_sprite_group_set_pos(g, bx, MG_SY(hz->type == MG_H_PIT ? MG_GROUND_Y : MG_GROUND_Y - 32));
            ng_sprite_group_set_visible(g, 1);
            ng_sprite_group_flush(g);
            used++;
        }
        /* A pit reads as a texture change more than a hole at a glance, so
         * a caution sign stands planted at its near edge -- a board and a
         * post, not another line of HUD text. The signs have sprites of
         * their own: sharing the hazards' pool, a sign was the first thing
         * dropped when a pit and its neighbours filled it, and it vanished
         * or flickered just as she came up to it. */
        if (hz->type == MG_H_PIT && signs < MG_SIGNS) {
            int16_t sx = (int16_t)(scr - 28);
            if (sx >= -32 && sx <= 336) {
                NGSpriteGroup *g = &mg.signs[signs++];
                ng_sprite_group_set_pos(g, sx, MG_SY(MG_GROUND_Y - 32));
                ng_sprite_group_set_visible(g, 1);
                ng_sprite_group_flush(g);
            }
        }
    }
    for (; used < MG_HAZARD_BLOCKS; used++) {
        ng_sprite_group_set_visible(&mg.hazards[used], 0);
        ng_sprite_group_flush(&mg.hazards[used]);
    }
    for (; signs < MG_SIGNS; signs++) {
        ng_sprite_group_set_visible(&mg.signs[signs], 0);
        ng_sprite_group_flush(&mg.signs[signs]);
    }
}

/*
 * Scenery: grass, blossoms, saplings, lanterns and signposts taken from a
 * small pool as the road scrolls past, so a valley feels lived in without
 * spending a sprite on ground that is off-screen.
 */
/*
 * The valley's end: the gate stands in the foot of a great landmark -- the
 * forest's ancient tree, a cliff with its fall, the grotto's ice peak, the
 * citadel's tower (landmark_art.py) -- 192 x 176, the gate in the middle of
 * its foot. The view stops with it (mg_camera_follow): nothing lies past
 * the gate. It is drawn in the decoration's sprites, behind the cast; while
 * it is in view the few props of the road ahead of it wait.
 */
static NGSpriteGroup mg_landmark_g;
static uint8_t mg_landmark_on;
enum { MG_LANDMARK_W = 192, MG_LANDMARK_H = 176, MG_LANDMARK_LEFT = 80 };

static void NEOGEO_USER mg_landmark_setup(void)
{
    uint8_t lm = mg_landmark_of[mg.stage < MG_LEVEL_COUNT ? mg.stage : 0], k;
    mg_landmark_on = 0;
    if (lm >= MG_LANDMARKS) return;
    for (k = 0; k < mg_landmark_banks[lm]; k++)
        mg_palette((uint8_t)(PAL_LANDMARK + k), mg_landmark_pals[lm] + (uint16_t)k * 16u);
    ng_sprite_group_init(&mg_landmark_g, SLOT_DECOR, MG_LANDMARK_W / 16, MG_LANDMARK_H / 16,
                         mg_landmark_tiles[lm], PAL_LANDMARK);
    ng_sprite_group_set_tile_stride(&mg_landmark_g, MG_LANDMARK_W / 16);
    ng_sprite_group_set_palette_map(&mg_landmark_g, mg_landmark_maps[lm]);
    ng_sprite_group_set_visible(&mg_landmark_g, 0);
}

/* 1 while the landmark is in view (and has the decoration's sprites). */
static uint8_t NEOGEO_USER mg_landmark_draw(int16_t camera_x)
{
    const MGLevel *level = &mg_levels[mg.stage];
    int16_t scr = (int16_t)((int16_t)level->gate_x - MG_LANDMARK_LEFT - camera_x);
    uint8_t i;
    uint8_t want = (uint8_t)(level->gate_x && mg_landmark_of[mg.stage] < MG_LANDMARKS &&
                             !mg.boss_active && !mg.vault && mg.state != MG_BONUS &&
                             scr > -MG_LANDMARK_W && scr < NG_SCREEN_W);
    if (want) {
        if (!mg_landmark_on) {
            for (i = 0; i < MG_DECOR_SLOTS; i++) {
                ng_sprite_group_set_visible(&mg.decor[i], 0);
                ng_sprite_group_flush(&mg.decor[i]);
            }
            ng_sprite_group_mark_dirty(&mg_landmark_g, NG_SGF_DIRTY_ALL);
            mg_landmark_on = 1;
        }
        ng_sprite_group_set_pos(&mg_landmark_g, scr, MG_SY(MG_GROUND_Y + 8 - MG_LANDMARK_H));
        ng_sprite_group_set_visible(&mg_landmark_g, 1);
        ng_sprite_group_flush(&mg_landmark_g);
        return 1;
    }
    if (mg_landmark_on) {
        ng_sprite_group_set_visible(&mg_landmark_g, 0);
        ng_sprite_group_flush(&mg_landmark_g);
        for (i = 0; i < MG_DECOR_SLOTS; i++) ng_sprite_group_mark_dirty(&mg.decor[i], NG_SGF_DIRTY_ALL);
        mg_landmark_on = 0;
    }
    return 0;
}

static void NEOGEO_USER mg_draw_decor(int16_t camera_x)
{
    uint8_t i, used = 0;

    if (mg_landmark_draw(camera_x)) return;

    for (i = 0; i < MG_DECOR_COUNT && used < MG_DECOR_SLOTS; i++) {
        const MGDecor *d = &mg_decor[mg.stage][i];
        int16_t scr = (int16_t)(d->x - camera_x);
        NGSpriteGroup *g;

        if (!d->x || scr < -32 || scr > 336) continue;
        g = &mg.decor[used];
        ng_sprite_group_set_tile_base(g, mg_decor_tiles[d->kind]);
        ng_sprite_group_set_pos(g, scr, MG_SY(d->y));
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
            ng_sprite_group_set_pos(&mg.vines[i], scr, MG_SY(v->top));
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
        ng_sprite_group_set_pos(&mg.gate, scr, MG_SY(MG_GROUND_Y - 48));
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
    int16_t lead = plane >= 0 ? (int16_t)((uint16_t)plane % 420u) : (int16_t)(plane % 420);
    uint8_t i;

    if (mg.flying) {        /* no road in the sky, so nothing grows in front of it */
        for (i = 0; i < MG_FRONT_SLOTS; i++) {
            ng_sprite_group_set_visible(&mg.front[i], 0);
            ng_sprite_group_flush(&mg.front[i]);
        }
        return;
    }

    for (i = 0; i < MG_FRONT_SLOTS; i++) {
        /* Wrap only in the off-screen gap, never through the playfield. */
        int16_t scr = (int16_t)(i * 140 + 420 - lead + 40);   /* > 0 */
        while (scr >= 420) scr = (int16_t)(scr - 420);
        scr = (int16_t)(scr - 40);

        if (scr < -40 || scr > 340) {
            ng_sprite_group_set_visible(&mg.front[i], 0);
        } else {
            ng_sprite_group_set_pos(&mg.front[i], scr, MG_SY(MG_GROUND_Y - 16));
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
                                MG_SY(mg.rescue->y - 32));
        ng_sprite_group_set_visible(&mg.cage, 1);
    } else if (mg.cage_open) {
        /* Stays open and fully visible for its whole run, then simply
         * clears -- it used to blink for its last 12 frames (toggling on
         * bit 2 of the countdown), which read as broken, not as fading. */
        mg.cage_open--;
        ng_sprite_group_set_tile_base(&mg.cage, mg_prop_tiles[MG_P_CHEST_OPEN]);
        ng_sprite_group_set_pos(&mg.cage, (int16_t)(mg.cage_x - camera_x), MG_SY(mg.cage_y));
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

        if (p->life && !ng_feedback_is_hitstop()) {
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
            uint8_t sc = (uint8_t)(48u + (uint16_t)((uint16_t)p->life * 207u) / (uint16_t)p->shrink);
            int16_t in = (int16_t)(8 - (sc >> 5));
            ng_sprite_group_set_scale(&p->sprite, sc, sc);
            ng_sprite_group_set_pos(&p->sprite, (int16_t)(scr_x + in), MG_SY(p->y + in));
            ng_sprite_group_set_visible(&p->sprite, 1);
        } else {
            ng_sprite_group_set_scale(&p->sprite, NG_SPRITE_FULL_XSCALE, NG_SPRITE_FULL_YSCALE);
            ng_sprite_group_set_pos(&p->sprite, scr_x, MG_SY(p->y));
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

    if (mg.boss_active || mg.state == MG_BONUS || mg.vault) {
        int16_t left = mg.boss_active ? mg.arena_left : (mg.vault ? (int16_t)(level->width - NG_SCREEN_W) : 0);
        if (p->x < left + 20) { ng_char_set_pos(p, left + 20, p->y); p->vx_fp = 0; }
        if (p->x > left + 300) { ng_char_set_pos(p, left + 300, p->y); p->vx_fp = 0; }
        if (mg.boss) {
            NGCharacter *b = mg.boss;
            if (b->x < left + 52) { ng_char_set_pos(b, left + 52, b->y); b->vx_fp = 0; }
            if (b->x > left + 268) { ng_char_set_pos(b, left + 268, b->y); b->vx_fp = 0; }
        }
    }

    if (p->x < 16) { ng_char_set_pos(p, 16, p->y); p->vx_fp = 0; }
    if (!mg.vault && p->x > (int16_t)(level->width - 16)) {
        ng_char_set_pos(p, (int16_t)(level->width - 16), p->y);
        p->vx_fp = 0;
    }

    /* A sealed gate is a wall: the guardian waits behind it. */
    if (level->gate_x && !mg.gate_unlocked && !mg.vault && p->x > (int16_t)(level->gate_x - 14)) {
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

/* During a hitstop nothing thinks, but the stick is still read: a button
 * pressed (even tapped and let go) while the world is held lands on the
 * first frame after it. */
static uint16_t NEOGEO_USER mg_input(void);
static void NEOGEO_USER mg_hold_input(void)
{
    uint16_t joy = mg_input();
    mg.held_press |= (uint16_t)(joy & (uint16_t)(~mg.previous_joy));
    mg.previous_joy = joy;
}

static void NEOGEO_USER mg_impact_sfx(uint16_t id)
{
    playSFX((uint8_t)id);
}

/*
 * The camera: the engine's (ng_camera_update), with what is Maiya's own
 * around it. Tuning is set in mg_scene: a window of +-MG_CAM_DEAD px around
 * the spot she is kept at, MG_CAM_LEAD px of look-ahead that swings over
 * MG_CAM_LEAD_RATE px a frame, and a follow that is exact, in whole
 * pixels, so she never judders against the valley.
 */
static void NEOGEO_USER mg_camera_follow(void)
{
    NGCharacter *p = mg.player;
    int16_t left = 0, right = (int16_t)(mg_levels[mg.stage].width - 1);

    /* Where it may go: the valley; the arena while its guardian fights;
     * the bonus round's one screen. (The bounds are inclusive: the right
     * one is the last pixel, so the view ends exactly at the edge.) */
    if (mg.boss_active) {
        left = mg.arena_left;
        right = (int16_t)(mg.arena_left + NG_SCREEN_W - 1);
    } else if (mg.vault) {
        left = (int16_t)(mg_levels[mg.stage].width - NG_SCREEN_W);
        right = (int16_t)(left + NG_SCREEN_W - 1);
    } else if (mg.state == MG_BONUS) {
        right = NG_SCREEN_W - 1;
    } else if (mg_levels[mg.stage].gate_x && mg.state != MG_TOUR) {
        /* the valley ends with the gate's landmark: nothing past it shows */
        right = (int16_t)(mg_levels[mg.stage].gate_x - MG_LANDMARK_LEFT + MG_LANDMARK_W - 1);
    }
    ng_camera_set_bounds(&mg.camera, left, 0, right, NG_SCREEN_H - 1);

    /* The look-ahead keeps to the way she last really ran: standing still
     * doesn't take it back, and a stagger doesn't swing it. */
    if (mg.hurt <= HURT_LOCK) {
        if (p->vx_fp > 128) mg.cam_dir = 1;
        else if (p->vx_fp < -128) mg.cam_dir = -1;
    }

    if (mg.state == MG_TOUR) {
        mg.camera.x = mg.tour_x;            /* the healed valley's own slow pan */
    } else if (mg.flying && !mg.boss_active) {
        /* On the Sky Road the sky carries the view along at a pixel a
         * frame (not through a hitstop or a pause), to the guardian. */
        if (mg.state == MG_PLAY && !ng_feedback_is_hitstop() && !ng_pause_is_on() &&
            mg.fly_x < mg.arena_left) mg.fly_x++;
        mg.camera.x = mg.fly_x;
    } else {
        /* Held where it is through a hitstop or a pause; its shake runs on. */
        mg.camera.mode = (ng_feedback_is_hitstop() || ng_pause_is_on()) ? NG_CAM_FREE : NG_CAM_FOLLOW;
        /* Vertically it never moves: aimed at the screen's middle row, and
         * its bounds hold it at 0 anyway. */
        ng_camera_update(&mg.camera, p->x, NG_SCREEN_H / 2, mg.cam_dir);
    }

    /* Her own knock-back jolt, on top of any impact shake -- neither may
     * show past the edge of the valley or the arena. */
    if (mg.shake) {
        mg.shake--;
        mg.shake_x = (int16_t)((mg.shake & 2u) ? 2 : -2);
    } else {
        mg.shake_x = 0;
    }
    mg.camera.x = (int16_t)(mg.camera.x + mg.shake_x);
    if (mg.camera.x < mg.camera.bound_left) mg.camera.x = mg.camera.bound_left;
    if (mg.camera.x > mg.camera.bound_right) mg.camera.x = mg.camera.bound_right;

    /* On the wing she stays inside the view: the sky's left edge pushes
     * her along, and she can't fly out of the right or the bottom (the
     * top is ng_move's). */
    if (mg.flying && mg.state == MG_PLAY) {
        int16_t lo = (int16_t)(mg.camera.x + 40), hi = (int16_t)(mg.camera.x + 288);
        if (p->x < lo) { ng_char_set_pos(p, lo, p->y); if (p->vx_fp < NG_FP_ONE) p->vx_fp = NG_FP_ONE; }
        if (p->x > hi) { ng_char_set_pos(p, hi, p->y); if (p->vx_fp > 0) p->vx_fp = 0; }
        if (p->y > 212) { ng_char_set_pos(p, p->x, 212); if (p->vy_fp > 0) p->vy_fp = 0; }
    }

    /*
     * Up a tall climb the view rises with her, easing, once she is above
     * the ordinary ledges, and comes back down as she does; on the road,
     * and on any valley one screen high, it never moves.
     */
    {
        int16_t want = 0;
        uint16_t upper = mg.stage < MG_LEVEL_COUNT ? mg_stage_upper[mg.stage] : 0u;
        if (upper && !mg.boss_active && !mg.vault && mg.state == MG_PLAY) {
            want = (int16_t)(p->y - 64);
            if (want > 0) want = 0;
            if (want < -(int16_t)upper) want = (int16_t)-upper;
        }
        /* A jump of the view (back out of the vault) is taken at once. */
        if (mg_abs((int16_t)(want - mg.cam_y)) > 112) mg.cam_y = want;
        if (want != mg.cam_y) {
            int16_t step = (int16_t)((want - mg.cam_y) / 6);
            if (!step) step = (int16_t)(want > mg.cam_y ? 1 : -1);
            mg.cam_y = (int16_t)(mg.cam_y + step);
        }
    }
}

/*
 * Engine hook, runs right before the characters are drawn: follow the
 * heroine with the camera and move every world-space sprite with it.
 */
/*
 * The light of a special move, drawn over her for as long as it lasts: the
 * Rising Bloom's whirl of petals, the Surge's trail (streaming behind her,
 * mirrored when she faces left), the Secret Art's widening sun ring and the
 * high leap's burst from her feet.
 */
enum { MG_LIGHT_NONE, MG_LIGHT_WHIRL, MG_LIGHT_TRAIL, MG_LIGHT_SUN, MG_LIGHT_BURST, MG_LIGHT_AURA };

static void NEOGEO_USER mg_light(uint8_t kind, uint8_t frames)
{
    mg.fx_kind = kind;
    mg.fx_time = frames;
}

static void NEOGEO_USER mg_draw_light(int16_t camera_x)
{
    NGCharacter *p = mg.player;
    uint16_t frame;
    int16_t x, y;
    uint8_t flip = 0;

    if (!mg.fx_time || !p || mg.state == MG_BONUS) {
        if (mg.fx_time) mg.fx_time = 0;
        ng_sprite_group_set_visible(&mg.fx, 0);
        ng_sprite_group_flush(&mg.fx);
        return;
    }
    mg.fx_time--;
    x = (int16_t)(p->x - camera_x - 32);
    y = (int16_t)(p->y - 60);
    switch (mg.fx_kind) {
    case MG_LIGHT_WHIRL:
        frame = (uint16_t)(MG_FX_WHIRL0 + (mg.tick >> 2) % 3u);
        y = (int16_t)(p->y - 64);
        break;
    case MG_LIGHT_TRAIL:
        frame = (uint16_t)(MG_FX_TRAIL0 + ((mg.tick >> 2) & 1u));
        flip = mg.facing;
        x = (int16_t)(x + (flip ? 30 : -30));
        y = (int16_t)(p->y - 58);
        break;
    case MG_LIGHT_SUN:
        frame = (uint16_t)(MG_FX_SUN0 + (mg.fx_time > 20 ? 0u : (mg.fx_time > 10 ? 1u : 2u)));
        break;
    case MG_LIGHT_AURA:
        frame = (uint16_t)(MG_FX_AURA0 + ((mg.tick >> 3) & 1u));
        y = (int16_t)(p->y - 62);
        break;
    default:
        frame = (uint16_t)(MG_FX_BURST0 + ((mg.tick >> 2) & 1u));
        y = (int16_t)(p->y - 62);
        break;
    }
    ng_sprite_group_set_tile_base(&mg.fx, mg_fx_tiles[frame]);
    ng_sprite_group_set_flip(&mg.fx, flip, 0);
    ng_sprite_group_set_pos(&mg.fx, x, MG_SY(y));
    ng_sprite_group_set_visible(&mg.fx, 1);
    ng_sprite_group_flush(&mg.fx);
}

static void NEOGEO_USER mg_falls_step(void);
static void NEOGEO_USER mg_art_step(void);

static void NEOGEO_USER mg_before_draw_hook(void)
{
    mg_falls_step();
    mg_art_step();
    if (mg.player) mg_camera_follow();
    if (mg.eagle && mg.player) {
        ng_char_set_pos(mg.eagle, mg.player->x, mg.player->y);
        mg.eagle->visible = (uint8_t)(mg.player->visible && mg.state != MG_DEAD && mg.state != MG_OVER);
    }
    ng_level_set_scroll(mg.camera.x, mg.cam_y);
    mg_scroll_scenery(mg.camera.x);
    if (mg.state == MG_BONUS || mg.state == MG_ENDING) {
        mg_update_sparks(mg.camera.x);
        return;
    }
    if (mg.state == MG_TOUR) {
        /* healed: the ledges, the climbs and the flowers, but no fire, no
         * sludge, no pits, no gate */
        mg_draw_ledges(mg.camera.x);
        mg_draw_vines(mg.camera.x);
        mg_draw_decor(mg.camera.x);
        mg_draw_front(mg.camera.x);
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
    mg_draw_light(mg.camera.x);
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
    case MG_B_AIRSHIP:   return mg_airship_tiles;
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
    case MG_B_AIRSHIP:   return mg_airship_pal;
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
    case MG_E_DARTFROG: return mg_dartfrog_tiles;
    case MG_E_SMOGBAT: return mg_smogbat_tiles;
    case MG_E_POACHDRONE: return mg_poachdrone_tiles;
    case MG_E_CHEMFLY: return mg_chemfly_tiles;
    case MG_E_PLASTICBAT: return mg_plasticbat_tiles;
    case MG_E_SLAGGOLEM: return mg_slaggolem_tiles;
    case MG_E_VINESTING: return mg_vinesting_tiles;
    case MG_E_SPOREGOB: return mg_sporegob_tiles;
    case MG_E_WRAITH: return mg_acidmoth_tiles;
    case MG_E_RHINO: return mg_rhino_tiles;
    case MG_E_DRAGONFLY: return mg_dragonfly_tiles;
    case MG_E_GNAT: return mg_gnat_tiles;
    case MG_E_GUNSHIP: return mg_gunship_tiles;
    case MG_E_BAGOCTO: return mg_bagocto_tiles;
    case MG_E_BINOCTO: return mg_binocto_tiles;
    case MG_E_SAWBOT: return mg_sawbot_tiles;
    case MG_E_DRILLBOT: return mg_drillbot_tiles;
    case MG_E_TORCHBOT: return mg_torchbot_tiles;
    case MG_E_SMOGSTACK: return mg_smogstack_tiles;
    case MG_E_SLUDGEBARREL: return mg_sludgebarrel_tiles;
    default:          return mg_slime_tiles;
    }
}

/* The newer creatures' canvases, which their sprites must be drawn at:
 * tiles are stored a row at a time, the canvas width apart, so drawing a
 * 48-wide crab two tiles wide scrambled it, and a tall one lost its feet.
 * The first six have their sizes set by hand in mg_character. */
/* ...and the body a hit is measured on, from what the art actually covers. */
#define MG_CANVAS(K) *w = MG_##K##_W; *h = MG_##K##_H; *bw = MG_##K##_BODY_W; *bh = MG_##K##_BODY_H
static void NEOGEO_USER mg_enemy_canvas(uint8_t type, uint8_t *w, uint8_t *h, uint8_t *bw, uint8_t *bh)
{
    switch (type) {
    case MG_E_JELLYFISH:  MG_CANVAS(JELLYFISH);  break;
    case MG_E_TOXICCRAB:  MG_CANVAS(TOXICCRAB);  break;
    case MG_E_ACIDMOTH:
    case MG_E_WRAITH:     MG_CANVAS(ACIDMOTH);   break;
    case MG_E_DARTFROG:   MG_CANVAS(DARTFROG);   break;
    case MG_E_SMOGBAT:    MG_CANVAS(SMOGBAT);    break;
    case MG_E_POACHDRONE: MG_CANVAS(POACHDRONE); break;
    case MG_E_CHEMFLY:    MG_CANVAS(CHEMFLY);    break;
    case MG_E_PLASTICBAT: MG_CANVAS(PLASTICBAT); break;
    case MG_E_SLAGGOLEM:  MG_CANVAS(SLAGGOLEM);  break;
    case MG_E_VINESTING:  MG_CANVAS(VINESTING);  break;
    case MG_E_SPOREGOB:   MG_CANVAS(SPOREGOB);   break;
    case MG_E_RHINO:      MG_CANVAS(RHINO);      break;
    case MG_E_DRAGONFLY:  MG_CANVAS(DRAGONFLY);  break;
    case MG_E_GNAT:       MG_CANVAS(GNAT);       break;
    case MG_E_GUNSHIP:    MG_CANVAS(GUNSHIP);    break;
    case MG_E_BAGOCTO:    MG_CANVAS(BAGOCTO);    break;
    case MG_E_BINOCTO:    MG_CANVAS(BINOCTO);    break;
    case MG_E_SAWBOT:     MG_CANVAS(SAWBOT);     break;
    case MG_E_DRILLBOT:   MG_CANVAS(DRILLBOT);   break;
    case MG_E_TORCHBOT:   MG_CANVAS(TORCHBOT);   break;
    case MG_E_SMOGSTACK:  MG_CANVAS(SMOGSTACK);  break;
    case MG_E_SLUDGEBARREL: MG_CANVAS(SLUDGEBARREL); break;
    default:              *w = 32u; *h = 32u; *bw = 24u; *bh = 24u; break;
    }
}
#undef MG_CANVAS

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
    case MG_E_DARTFROG: return PAL_DARTFROG;
    case MG_E_SMOGBAT: return PAL_SMOGBAT;
    case MG_E_POACHDRONE: return PAL_POACHDRONE;
    case MG_E_CHEMFLY: return PAL_CHEMFLY;
    case MG_E_PLASTICBAT: return PAL_PLASTICBAT;
    case MG_E_SLAGGOLEM: return PAL_SLAGGOLEM;
    case MG_E_VINESTING: return PAL_VINESTING;
    case MG_E_SPOREGOB: return PAL_SPOREGOB;
    case MG_E_WRAITH: return PAL_WRAITH;
    case MG_E_RHINO: return PAL_RHINO;
    case MG_E_DRAGONFLY: return PAL_DRAGONFLY;
    case MG_E_GNAT: return PAL_GNAT;
    case MG_E_GUNSHIP: return PAL_GUNSHIP;
    case MG_E_BAGOCTO: return PAL_BAGOCTO;
    case MG_E_BINOCTO: return PAL_BINOCTO;
    case MG_E_SAWBOT: return PAL_SAWBOT;
    case MG_E_DRILLBOT: return PAL_DRILLBOT;
    case MG_E_TORCHBOT: return PAL_TORCHBOT;
    case MG_E_SMOGSTACK: return PAL_SMOGSTACK;
    case MG_E_SLUDGEBARREL: return PAL_SLUDGEBARREL;
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
        tile = mg_eagle_tiles[frame % MG_EAGLE_FRAMES];
    } else if (c->kind == K_BOSS) {
        const uint16_t *bt = mg_boss_tiles(c->data0);
        tile = bt[frame % (c->data0 == MG_B_AIRSHIP ? MG_AIRSHIP_FRAMES : MG_BOSS_FRAMES)];
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
        ng_char_set_sprite(c, NG_SPR_CHAR_FIRST, EAGLE_STRIPS, EAGLE_ROWS, mg_eagle_tiles[0], palette);
        ng_char_set_tile_stride(c, EAGLE_STRIPS);
        c->sprite_offset_x = -64;      /* stood on its talons, wings spread either side */
        c->sprite_offset_y = -46;
        ng_char_set_body(c, -18, -32, 36, 32);
    } else if (kind == K_BOSS && subtype == MG_B_AIRSHIP) {
        /* 256 x 96, standing on the middle of its keel */
        ng_char_set_sprite(c, NG_SPR_CHAR_FIRST, 16, 6, mg_airship_tiles[0], palette);
        ng_char_set_tile_stride(c, 16);
        c->sprite_offset_x = -128;
        c->sprite_offset_y = -96;
        ng_char_set_body(c, -120, -76, 238, 64);
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
        } else if (subtype >= MG_E_JELLYFISH) {
            /* Standing on its canvas's foot line, as the first six do. */
            uint8_t w, h, body_w, body_h;
            mg_enemy_canvas(subtype, &w, &h, &body_w, &body_h);
            strips = (uint8_t)(w >> 4); rows = (uint8_t)(h >> 4);
            ox = (int16_t)-(int16_t)(w >> 1); oy = (int16_t)(2 - (int16_t)h);
            bw = body_w; bh = body_h;
            bx = (int16_t)-(bw >> 1); by = (int16_t)-bh;
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
/*  Showers from the sky                                              */
/* ------------------------------------------------------------------ */
/*
 * Flowers, leaves and drops of clean water falling over the whole screen.
 * A piece lives on the screen, not in the valley -- the view pans under
 * it -- sways and tumbles as it comes down, and at a height of its own it
 * is gone: a flower or a leaf shrinks away, a drop splashes. Over a healed
 * valley each one gone brings the painting's colours up a step
 * (mg_glow_*). There the shower has the tray's 24 sprites, which the scene
 * hides; in play it takes the 14 above the glider, which nothing uses.
 *
 * A Secret Art sends its pieces the other way: each appears at the edge of
 * the screen, gathers there a moment, then flies onto the creature it was
 * called for and strikes it (mg_art_strike).
 */
enum {
    MG_FALLS = 24, SLOT_RAIN = 366, MG_RAIN_SLOTS = 14,
    MG_FALL_LEAF = 4, MG_FALL_WATER = 5,   /* below 4: a flower of that kind */
    MG_FALL_THORN = 6, MG_FALL_ROSE = 7, MG_FALL_ICE = 8, MG_FALL_SPARK = 9, MG_FALL_STAR = 10,
    MG_FALL_FADE = 10,
    MG_AIM_BOSS = MG_ENEMIES + 1, MG_AIM_SPOT = MG_ENEMIES + 2
};

typedef struct {
    int16_t x, y;            /* on screen, in eighths of a pixel */
    int8_t  vx;              /* eighths of a pixel a frame */
    uint8_t vy;
    uint8_t live, kind, t, end, fade, sway;
    uint8_t aim;             /* an art's piece: 1 + the creature it flies at, or
                              * MG_AIM_BOSS / MG_AIM_SPOT; 0 for one that falls */
    uint8_t wait;            /* ...frames it gathers before it flies */
    int16_t tx, ty;          /* ...where it strikes, like x and y */
    NGCharacter *who;        /* ...and the one it was called for */
    NGSpriteGroup sprite;
} MGFall;

static void NEOGEO_USER mg_art_strike(MGFall *f);
static void NEOGEO_USER mg_art_track(MGFall *f);

static MGFall mg_falls[MG_FALLS];
static uint8_t mg_falls_n;       /* sprites the shower has: 0 while there is none */
static uint8_t mg_falls_gone;    /* pieces gone since it began (stops at 255) */

/* Shrinking away, frame by frame to the last. */
static const uint8_t mg_fall_scale[MG_FALL_FADE] = { 24, 48, 72, 96, 120, 144, 168, 192, 216, 240 };

static void NEOGEO_USER mg_falls_begin(uint16_t slot, uint8_t count)
{
    uint8_t i;
    mg_falls_n = count;
    mg_falls_gone = 0;
    for (i = 0; i < count; i++) {
        mg_falls[i].live = 0;
        ng_sprite_group_init(&mg_falls[i].sprite, (uint16_t)(slot + i), 1, 1,
                             MG_TOOL_TILE + MG_T_FLOWER, PAL_TOOL);
        ng_sprite_group_set_visible(&mg_falls[i].sprite, 0);
        ng_sprite_group_upload(&mg_falls[i].sprite);
    }
}

static void NEOGEO_USER mg_falls_end(void)
{
    uint8_t i;
    for (i = 0; i < mg_falls_n; i++) {
        mg_falls[i].live = 0;
        ng_sprite_group_set_visible(&mg_falls[i].sprite, 0);
        ng_sprite_group_flush(&mg_falls[i].sprite);
    }
    mg_falls_n = 0;
}

/* One more piece from above the screen, anywhere across it; none when
 * every sprite is falling already. */
static void NEOGEO_USER mg_fall_spawn(uint8_t kind)
{
    uint8_t i;
    for (i = 0; i < mg_falls_n; i++) {
        MGFall *f = &mg_falls[i];
        if (f->live) continue;
        f->live = 1;
        f->kind = kind;
        f->t = 0;
        f->fade = 0;
        f->sway = (uint8_t)ng_rand();
        f->x = (int16_t)(((int16_t)ng_rand_range(352u) - 16) * 8);
        f->y = -16 * 8;
        f->vx = (int8_t)(-2 - (int8_t)(ng_rand() & 3u));    /* the breeze of her passing */
        f->aim = 0;
        f->wait = 0;
        if (kind == MG_FALL_WATER) f->vy = (uint8_t)(22u + (ng_rand() & 7u));
        else if (kind == MG_FALL_LEAF) f->vy = (uint8_t)(9u + (ng_rand() & 3u));
        else f->vy = (uint8_t)(7u + (ng_rand() & 3u));
        f->end = (uint8_t)(64u + ng_rand_range(144u));
        ng_sprite_group_set_scale(&f->sprite, NG_SPRITE_FULL_XSCALE, NG_SPRITE_FULL_YSCALE);
        return;
    }
}

/* Half flowers of the four kinds, the rest leaves and clean water. */
static void NEOGEO_USER mg_fall_any(void)
{
    uint8_t r = (uint8_t)(ng_rand() & 15u);
    mg_fall_spawn((uint8_t)(r < 8u ? (r & 3u) : (r < 11u ? MG_FALL_LEAF : MG_FALL_WATER)));
}

static void NEOGEO_USER mg_falls_step(void)
{
    uint8_t i;
    for (i = 0; i < mg_falls_n; i++) {
        MGFall *f = &mg_falls[i];
        NGSpriteGroup *g = &f->sprite;
        int16_t sx, sy;
        uint16_t tile;
        uint8_t turn = 0, flip = 0;
        if (!f->live) continue;
        f->t++;
        if (f->fade) {
            if (--f->fade == 0) {
                f->live = 0;
                if (mg_falls_gone < 255u) mg_falls_gone++;
                ng_sprite_group_set_visible(g, 0);
                ng_sprite_group_flush(g);
                continue;
            }
        } else if (f->aim) {
            if (f->wait) {
                f->wait--;                       /* gathering at the edge */
            } else {
                /* flying at it, a quarter of the way closer each frame */
                int16_t dx, dy;
                mg_art_track(f);
                dx = (int16_t)(f->tx - f->x);
                dy = (int16_t)(f->ty - f->y);
                f->x = (int16_t)(f->x + (dx >> 2) + (dx > 0 ? 8 : (dx < 0 ? -8 : 0)));
                f->y = (int16_t)(f->y + (dy >> 2) + (dy > 0 ? 8 : (dy < 0 ? -8 : 0)));
                if (dx > -96 && dx < 96 && dy > -96 && dy < 96) {
                    mg_art_strike(f);
                    f->aim = 0;
                    f->fade = MG_FALL_FADE;
                }
            }
        } else {
            f->x = (int16_t)(f->x + f->vx);
            f->y = (int16_t)(f->y + f->vy);
            if ((f->y >> 3) >= (int16_t)f->end) f->fade = MG_FALL_FADE;
        }
        sx = (int16_t)(f->x >> 3);
        sy = (int16_t)(f->y >> 3);
        if (f->kind > MG_FALL_WATER) {
            /* an art's weapons: the thorn spins, the rest glint as they come */
            static const uint8_t art_tile[5] = { MG_T_THORN0, MG_T_ROSE, MG_T_ICE, MG_T_SPARK, MG_T_STAR };
            tile = art_tile[f->kind - MG_FALL_THORN];
            if (f->kind == MG_FALL_THORN) turn = (uint8_t)((f->t >> 1) & 1u);
            else flip = (uint8_t)((f->t >> 3) & 1u);
        } else if (f->kind == MG_FALL_WATER) {
            tile = (uint16_t)(f->fade ? MG_T_SPLASH : MG_T_WATER);
        } else {
            /* a flower turns over every eight frames and back, a leaf
             * flutters twice as fast; both swing side to side */
            uint8_t k = (uint8_t)(f->t + f->sway);
            uint8_t leaf = (uint8_t)(f->kind == MG_FALL_LEAF);
            turn = (uint8_t)((k >> (leaf ? 2 : 3)) & 1u);
            flip = (uint8_t)((k >> (leaf ? 3 : 4)) & 1u);
            tile = (uint16_t)(leaf ? MG_T_FALL_LEAF : MG_T_FLOWER + f->kind * 2u);
            if (!f->aim) sx = (int16_t)(sx + ng_trig_mul(leaf ? 4 : 7, ng_sin((uint8_t)(f->sway + f->t * 3u))));
        }
        {
            /* an art's piece grows out of nothing as it appears; going, a
             * piece shrinks toward its own middle (a drop splashes instead) */
            uint8_t k = 0;
            if (f->aim && f->t < MG_FALL_FADE) k = f->t;
            else if (f->fade && f->kind != MG_FALL_WATER) k = f->fade;
            if (k) {
                uint8_t sc = mg_fall_scale[k - 1u];
                int16_t in = (int16_t)(8 - (sc >> 5));
                ng_sprite_group_set_scale(g, sc, sc);
                sx = (int16_t)(sx + in);
                sy = (int16_t)(sy + in);
            } else {
                ng_sprite_group_set_scale(g, NG_SPRITE_FULL_XSCALE, NG_SPRITE_FULL_YSCALE);
            }
        }
        ng_sprite_group_set_tile_base(g, (uint16_t)(MG_TOOL_TILE + tile + turn));
        ng_sprite_group_set_flip(g, flip, 0);
        ng_sprite_group_set_pos(g, sx, sy);
        ng_sprite_group_set_visible(g, 1);
        ng_sprite_group_flush(g);
    }
}

/*
 * The healed valley's painting coming up: from hazy -- three quarters as
 * bright, half its colour drained -- through its own colours at glow 8, to
 * richer and lighter than it was painted at 16. Worked out from the colours
 * it was loaded with, four banks a frame, through tables made once a step.
 */
static uint16_t mg_glow_src[16u * 16u];
static uint8_t mg_glow_lut[32];
static int8_t mg_glow_sat[63];
static uint8_t mg_glow_now, mg_glow_bank;

#define MG_T3(n) n, n, n
static const uint8_t mg_third[94] = {   /* (r + g + b) / 3 */
    MG_T3(0), MG_T3(1), MG_T3(2), MG_T3(3), MG_T3(4), MG_T3(5), MG_T3(6), MG_T3(7),
    MG_T3(8), MG_T3(9), MG_T3(10), MG_T3(11), MG_T3(12), MG_T3(13), MG_T3(14), MG_T3(15),
    MG_T3(16), MG_T3(17), MG_T3(18), MG_T3(19), MG_T3(20), MG_T3(21), MG_T3(22), MG_T3(23),
    MG_T3(24), MG_T3(25), MG_T3(26), MG_T3(27), MG_T3(28), MG_T3(29), MG_T3(30), 31
};
#undef MG_T3

static void NEOGEO_USER mg_glow_tables(uint8_t glow)
{
    uint8_t v, lift = (uint8_t)(glow > 8u ? glow - 8u : 0u);
    uint8_t sat = (uint8_t)(glow <= 8u ? 8u + glow : 16u + ((glow - 8u) >> 1));   /* sixteenths */
    uint16_t light = 0, step = (uint16_t)(48u + 2u * (glow < 8u ? glow : 8u));   /* sixty-fourths */
    int16_t d = (int16_t)(-31 * (int16_t)sat);
    for (v = 0; v < 32u; v++) {
        uint8_t b = (uint8_t)(light >> 6);
        light = (uint16_t)(light + step);
        /* the middle tones lifted, the lightest and darkest left be */
        b = (uint8_t)(b + (((((31u - b) * b) >> 6) * lift) >> 3));
        mg_glow_lut[v] = (uint8_t)(b > 31u ? 31u : b);
    }
    for (v = 0; v < 63u; v++) {
        mg_glow_sat[v] = (int8_t)(d >> 4);
        d = (int16_t)(d + sat);
    }
}

static uint8_t NEOGEO_USER mg_glow_channel(uint8_t c, uint8_t grey)
{
    int16_t v = (int16_t)(grey + mg_glow_sat[c + 31u - grey]);
    if (v < 0) v = 0;
    if (v > 31) v = 31;
    return mg_glow_lut[v];
}

static void NEOGEO_USER mg_glow_apply(uint8_t k)
{
    const uint16_t *src = &mg_glow_src[(uint16_t)k * 16u];
    uint16_t out[16];
    uint8_t i;
    out[0] = src[0];
    for (i = 1; i < 16u; i++) {
        uint16_t c = src[i];
        uint8_t r = (uint8_t)(((c >> 7) & 0x1Eu) | ((c >> 14) & 1u));
        uint8_t g = (uint8_t)(((c >> 3) & 0x1Eu) | ((c >> 13) & 1u));
        uint8_t b = (uint8_t)(((c << 1) & 0x1Eu) | ((c >> 12) & 1u));
        uint8_t grey = mg_third[r + g + b];
        r = mg_glow_channel(r, grey);
        g = mg_glow_channel(g, grey);
        b = mg_glow_channel(b, grey);
        out[i] = (uint16_t)(((uint16_t)(r & 1u) << 14) | ((uint16_t)(g & 1u) << 13) |
                            ((uint16_t)(b & 1u) << 12) | ((uint16_t)(r >> 1) << 8) |
                            ((uint16_t)(g >> 1) << 4) | (uint16_t)(b >> 1));
    }
    mg_palette((uint8_t)(PAL_BG + k), out);
}

/* The painting as just loaded, shown hazy at once. */
static void NEOGEO_USER mg_glow_begin(void)
{
    uint16_t i;
    for (i = 0; i < 16u * 16u; i++) mg_glow_src[i] = mg_pal_base[(uint16_t)PAL_BG * 16u + i];
    mg_glow_now = 0;
    mg_glow_tables(0);
    for (i = 0; i < 16u; i++) mg_glow_apply((uint8_t)i);
    mg_glow_bank = 16;
}

/* A step toward `want` once the last one is all on screen. */
static void NEOGEO_USER mg_glow_step(uint8_t want)
{
    uint8_t k;
    if (want > 16u) want = 16u;
    if (mg_glow_bank >= 16u && want > mg_glow_now) {
        mg_glow_now++;
        mg_glow_tables(mg_glow_now);
        mg_glow_bank = 0;
    }
    for (k = 0; k < 4u && mg_glow_bank < 16u; k++) mg_glow_apply(mg_glow_bank++);
}

/*
 * A Secret Art's colour shake: the painting swings between two tints of the
 * art's own light, strong at first and dying away, and then its colours
 * come back as they were. Half the banks a frame, from the colours it had
 * when the art began (kept in mg_glow_src, which only the healed valley
 * uses otherwise). As it dies down, the art's own flowers, water, light or
 * leaves drift down over the valley.
 */
enum { MG_ART_BLOSSOM, MG_ART_RAIN, MG_ART_SUN, MG_ART_FROST, MG_ART_GALE, MG_ARTS };
enum { MG_TINT_TIME = 48 };
static const uint8_t mg_tint_rgb[MG_ARTS][2][3] = {
    { { 31, 26, 8 },  { 31, 14, 22 } },   /* the rose storm: gold, then rose */
    { { 8, 24, 31 },  { 22, 30, 31 } },   /* the rain: clear blue, then foam */
    { { 31, 24, 4 },  { 31, 14, 4 } },    /* the sun: gold, then flame */
    { { 18, 26, 31 }, { 31, 31, 31 } },   /* the frost: ice, then snow */
    { { 12, 30, 10 }, { 30, 28, 8 } },    /* the gale: leaf, then straw */
};
/* ...and what drifts down after it */
static const uint8_t mg_art_after[MG_ARTS][2] = {
    { 0, 3 }, { MG_FALL_WATER, MG_FALL_WATER }, { 2, MG_FALL_STAR }, { 1, MG_FALL_STAR }, { MG_FALL_LEAF, 2 },
};
static uint8_t mg_tint_lut[3][32];

static void NEOGEO_USER mg_tint_begin(uint8_t art)
{
    uint16_t i;
    if (!mg_tint_t)
        for (i = 0; i < 16u * 16u; i++) mg_glow_src[i] = mg_pal_base[(uint16_t)PAL_BG * 16u + i];
    mg_tint_art = art;
    mg_tint_t = MG_TINT_TIME;
}

static void NEOGEO_USER mg_tint_apply(uint8_t k)
{
    const uint16_t *src = &mg_glow_src[(uint16_t)k * 16u];
    uint16_t out[16];
    uint8_t i;
    out[0] = src[0];
    for (i = 1; i < 16u; i++) {
        uint16_t c = src[i];
        uint8_t r = mg_tint_lut[0][((c >> 7) & 0x1Eu) | ((c >> 14) & 1u)];
        uint8_t g = mg_tint_lut[1][((c >> 3) & 0x1Eu) | ((c >> 13) & 1u)];
        uint8_t b = mg_tint_lut[2][((c << 1) & 0x1Eu) | ((c >> 12) & 1u)];
        out[i] = (uint16_t)(((uint16_t)(r & 1u) << 14) | ((uint16_t)(g & 1u) << 13) |
                            ((uint16_t)(b & 1u) << 12) | ((uint16_t)(r >> 1) << 8) |
                            ((uint16_t)(g >> 1) << 4) | (uint16_t)(b >> 1));
    }
    mg_palette((uint8_t)(PAL_BG + k), out);
}

static void NEOGEO_USER mg_art_step(void)
{
    uint8_t k, first;
    if (!mg_tint_t) return;
    mg_tint_t--;
    if (!mg_tint_t) {
        for (k = 0; k < 16u; k++) mg_palette((uint8_t)(PAL_BG + k), &mg_glow_src[(uint16_t)k * 16u]);
        return;
    }
    if (mg_tint_t < 30u && (mg_tint_t & 1u)) mg_fall_spawn(mg_art_after[mg_tint_art][(mg_tint_t >> 1) & 1u]);
    first = (uint8_t)(mg_tint_t & 1u);
    if (first) {
        /* the other tint every eight frames, weaker each time */
        const uint8_t *rgb = mg_tint_rgb[mg_tint_art][(mg_tint_t >> 3) & 1u];
        uint8_t strength = (uint8_t)((mg_tint_t * 3u) >> 4), c, v;
        if (strength > 7u) strength = 7u;
        for (c = 0; c < 3u; c++) {
            int16_t acc = (int16_t)(rgb[c] * strength);    /* (tint - v) * strength */
            for (v = 0; v < 32u; v++) {
                int16_t o = (int16_t)(v + (acc >> 4));
                mg_tint_lut[c][v] = (uint8_t)(o < 0 ? 0 : (o > 31 ? 31 : o));
                acc = (int16_t)(acc - strength);
            }
        }
    }
    for (k = (uint8_t)(first ? 0u : 8u); k < (uint8_t)(first ? 8u : 16u); k++) mg_tint_apply(k);
}

/* ------------------------------------------------------------------ */
/*  Shots, Sparks, Items & Hazards                                    */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER mg_sparks(int16_t x, int16_t y);

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
        /* A bubble never falls: it drifts up, slows and shrinks away. */
        if (tile == MG_T_BUBBLE) p->shrink = p->life;
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
    uint8_t i, k = 0, turn = (uint8_t)(ng_rand() & 7u);
    uint8_t want = (uint8_t)(6u + (ng_rand() & 1u));
    uint8_t petal = (uint8_t)((ng_rand() & 3u) == 0u);
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
            ng_sprite_group_set_tile_base(&p->sprite, (uint16_t)(MG_TOOL_TILE + ((ng_rand() & 1u) ? MG_T_PETAL : MG_T_LEAF)));
            p->x = (int16_t)(x - 8); p->y = (int16_t)(y - 12);
            p->vx = (int16_t)((ng_rand() & 1u) ? 1 : -1); p->vy = -2;
            p->life = 34;
            p->shrink = 0;
        } else {
            uint8_t d = (uint8_t)((k - 1u + turn) & 7u);
            uint8_t fast = (uint8_t)(ng_rand() & 1u);
            ng_sprite_group_set_tile_base(&p->sprite, (uint16_t)(MG_TOOL_TILE + MG_T_DUST));
            p->x = (int16_t)(x - 8 + rx[d] * 2); p->y = (int16_t)(y - 8 + ry[d] * 2);
            p->vx = (int16_t)(rx[d] + (fast ? rx[d] / 2 : 0));
            p->vy = (int16_t)(ry[d] - 1);
            p->life = (uint8_t)(16u + ng_rand_range(12u));
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
                     type == MG_E_ACIDMOTH || type == MG_E_CHEMFLY ||
                     type == MG_E_RHINO || type == MG_E_DRAGONFLY || type == MG_E_GNAT);
}

/* The shot takes its own tile: spit looks like spit and fire like fire,
 * not every projectile borrowing the thorn it was first set up with. */
/*
 * Luna does all Maiya does, the same way and as hard; only what she throws
 * and what flies about her are her own: lilac where Maiya throws thorns,
 * white daisies for petals, stars where petals scatter, her light in moon
 * colours (mg_moon_bank), her own voice (mg_voice).
 */
static uint8_t NEOGEO_USER mg_her_tile(uint8_t tile)
{
    if (!mg.hero_choice) return tile;
    switch (tile) {
    case MG_T_THORN0: return (uint8_t)(MG_T_FLOWER + 6);   /* a lilac, face on */
    case MG_T_THORN1: return (uint8_t)(MG_T_FLOWER + 7);   /* ...the crown's, turned */
    case MG_T_PETAL:  return MG_T_STAR;
    case MG_T_LEAF:   return (uint8_t)(MG_T_FLOWER + 2);   /* a daisy */
    default:          return tile;
    }
}

static MGShot *NEOGEO_USER mg_fire(int16_t x, int16_t y, int16_t vx, int16_t vy, uint8_t hostile, uint8_t kind)
{
    uint8_t i;
    for (i = 0; i < MG_SHOTS; i++) {
        MGShot *p = &mg.shots[i];
        if (p->life) continue;
        p->x = x; p->y = y; p->vx = vx; p->vy = vy;
        p->life = 90; p->hostile = hostile; p->kind = kind; p->mode = MG_SHOT_PLAIN;
        ng_sprite_group_set_tile_base(&p->sprite, (uint16_t)(MG_TOOL_TILE + (hostile ? kind : mg_her_tile(kind))));
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
static void NEOGEO_USER mg_stage_time_reset(void);
static void NEOGEO_USER mg_stage_time_final(void);
static void NEOGEO_USER mg_stage_rank(void);
static void NEOGEO_USER mg_combo_add(void);
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
        if (e->type == MG_E_GOBLIN || e->type == MG_E_WRAITH) {
            /*
             * The goblin and the smog wraith go the classic way: a spark
             * where the blow landed, then it pops up, turns over and drops
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
        } else {
            /* Every other creature bursts into dust where it stood (with a
             * spark, unless it's an insect) -- never shown on its back. */
            mg_dust_burst(x, (int16_t)(y - 14));
            if (!mg_enemy_is_bug(e->type)) mg_burst(x, (int16_t)(y - 20), MG_T_SPARK, 1, -1);
            ng_chars_remove(b);
            e->body = 0;
        }
        ng_impact_event(MG_IMPACT_KILL, 0, 0, &mg.camera, SOUND_SFX_10, 0, 0, 0, 0);
        mg.score += 250u;
        mg.kills++;
        mg_combo_add();

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
    } else {
        mg_hit_burst(e->body->x, (int16_t)(e->body->y - 20));
        if (damage >= MG_HEAVY_DAMAGE) ng_feedback_hitstop(MG_HITSTOP_HEAVY_HIT);
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

/* A rotten ledge's colours: each pulled halfway to its own grey, with a
 * touch of mould green, as bright as before -- darkening a set that is
 * dark already (the grove's lacquered beam) left a black shape. */
static void NEOGEO_USER mg_rot_bank(uint8_t bank, const uint16_t *src)
{
    uint16_t pal[16];
    uint8_t i;
    pal[0] = src[0];
    for (i = 1; i < 16; i++) {
        uint16_t c = src[i];
        uint8_t r = (uint8_t)((c >> 8) & 15u), g = (uint8_t)((c >> 4) & 15u), b = (uint8_t)(c & 15u);
        uint8_t grey = (uint8_t)((r + g + b) / 3u);
        r = (uint8_t)((r + grey) >> 1);
        g = (uint8_t)(((g + grey) >> 1) + 1u);
        b = (uint8_t)((b + grey) >> 1);
        if (g > 15u) g = 15u;
        pal[i] = (uint16_t)(((uint16_t)r << 8) | ((uint16_t)g << 4) | b);
    }
    mg_palette(bank, pal);
}

/*
 * The smog dreadnought, the Sky Road's guardian, is fought part by part:
 * its two smokestacks first (thorns, the swoop, the talons from above),
 * then -- both stacks gone -- the bridge behind its shark's eye opens and
 * is the last target. Its armour turns everything else. Its health bar is
 * the three parts' together. {x, y of the part's middle from the keel's,
 * half its width, half its height}.
 */
enum { MG_SHIP_STACK_L, MG_SHIP_STACK_R, MG_SHIP_BRIDGE, MG_SHIP_PARTS };
static const int8_t mg_ship_box[MG_SHIP_PARTS][4] = {
    { 40, -84, 8, 13 }, { 60, -86, 8, 13 }, { -91, -58, 16, 9 },
};

static uint8_t NEOGEO_USER mg_ship_open(uint8_t part)
{
    return (uint8_t)(mg.ship_part[part] &&
                     (part != MG_SHIP_BRIDGE || (!mg.ship_part[MG_SHIP_STACK_L] && !mg.ship_part[MG_SHIP_STACK_R])));
}

/* The part a blow at (x, y) lands on, open or not; 0xFF: none. */
static uint8_t NEOGEO_USER mg_ship_part_at(int16_t x, int16_t y)
{
    NGCharacter *b = mg.boss;
    uint8_t i;
    for (i = 0; i < MG_SHIP_PARTS; i++) {
        const int8_t *k = mg_ship_box[i];
        if (!mg.ship_part[i]) continue;
        if (mg_abs((int16_t)(x - (b->x + k[0]))) <= k[2] + 4 && mg_abs((int16_t)(y - (b->y + k[1]))) <= k[3] + 4)
            return i;
    }
    return 0xFFu;
}

static uint8_t NEOGEO_USER mg_ship_in_hull(int16_t x, int16_t y)
{
    NGCharacter *b = mg.boss;
    /* (from behind its nose, so a shot at the bridge reaches it) */
    return (uint8_t)(x > b->x - 100 && x < b->x + 118 && y > b->y - 76 && y < b->y - 12);
}

/* A part is wrecked: fire and a jolt, the hull's paint burns one step on,
 * and once both stacks are gone the bridge is bared. */
static void NEOGEO_USER mg_ship_part_down(uint8_t part)
{
    NGCharacter *b = mg.boss;
    const int8_t *k = mg_ship_box[part];
    mg_burst((int16_t)(b->x + k[0]), (int16_t)(b->y + k[1]), MG_T_FIRE, 4, -2);
    mg_burst((int16_t)(b->x + k[0]), (int16_t)(b->y + k[1] - 8), MG_T_DUST, 3, -2);
    mg.shake = 12;
    playSFX(SOUND_SFX_10);
    if (part != MG_SHIP_BRIDGE && !mg.ship_part[MG_SHIP_STACK_L] && !mg.ship_part[MG_SHIP_STACK_R])
        mg_hint("ITS BRIDGE IS BARE - STRIKE THE EYE!", PAL_GOLD, 150);
}

static uint8_t NEOGEO_USER mg_boss_spawn(uint8_t style, uint8_t stomps);
static void NEOGEO_USER mg_boss_announce(void);

static void NEOGEO_USER mg_boss_damage(uint8_t damage)
{
    NGCharacter *b = mg.boss ? mg.boss : mg.eagle;
    if (!b || mg.boss_hurt || mg.state != MG_PLAY) return;
    if (b == mg.boss && b->data0 == MG_B_AIRSHIP) {
        /* the blow lands on the part it was aimed at, or the next one open */
        uint8_t part = mg.ship_target;
        mg.ship_target = 0xFFu;
        if (part >= MG_SHIP_PARTS || !mg_ship_open(part)) {
            for (part = 0; part < MG_SHIP_PARTS && !mg_ship_open(part); part++) {}
            if (part == MG_SHIP_PARTS || mg.ship_z) return;
        }
        if (damage > mg.ship_part[part]) damage = mg.ship_part[part];
        mg.ship_part[part] = (uint8_t)(mg.ship_part[part] - damage);
        if (!mg.ship_part[part] && damage < b->hp) mg_ship_part_down(part);
    }
    mg.boss_hurt = 12;
    mg_hit_burst(b->x, (int16_t)(b->y - 40));
    if (damage >= b->hp) {
        /* The last blow: held longer, shaken harder, no flash -- it drains
         * to grey below. (A hit's 6-frame flash is always over by now:
         * boss_hurt keeps blows 12 frames apart.) */
        ng_impact_event(MG_IMPACT_BOSS_DOWN, 0, 0, &mg.camera, SOUND_SFX_4, 0, 0, 0, 0);
        ng_feedback_hitstop(MG_HITSTOP_BOSS_DOWN);
    } else {
        ng_impact_event(MG_IMPACT_BOSS_HIT, b == mg.boss ? PAL_BOSS : 0,
                        b == mg.boss ? mg_boss_pal((uint8_t)b->data0) : 0,
                        &mg.camera, SOUND_SFX_4, 0, 0, 0, 0);
    }

    if (damage >= b->hp) {
        /*
         * The works keep two guardians.  When the Iron Vulture falls, Lord
         * Smoggar himself climbs out of the plant, at full strength, and the
         * fight goes on where it stood.  Rio Negro Works is always stage 6
         * regardless of how many valleys follow it, since it is the only one
         * the Vulture ever guards.
         */
        if (mg.rush_i != 0xFFu && mg.boss) {
            /* One of the rush beaten: the next steps into its own lair --
             * and after the last, the stage's own guardian. */
            int16_t bx = b->x;
            uint8_t i, next = (uint8_t)(mg.rush_i + 1u);
            ng_chars_remove(mg.boss);
            mg.boss = 0;
            mg_sparks(bx, (int16_t)(MG_GROUND_Y - 40));
            playSFX(SOUND_SFX_10);
            mg.score += 3000u;
            mg.hud_dirty = 1;
            for (i = 0; i < MG_SHOTS; i++) mg.shots[i].life = 0;
            if (next < MG_RUSH_COUNT && mg_rush[mg.stage][next] != 0xFFu) {
                mg.rush_i = next;
                mg_boss_spawn(mg_rush[mg.stage][next], 3);
            } else {
                mg.rush_i = 0xFFu;
                mg_boss_spawn(mg_levels[mg.stage].boss_style, mg_levels[mg.stage].boss_hp);
            }
            if (mg.boss) {
                mg.boss_hurt = 40;
                mg_boss_announce();
            }
            return;
        }
        if (mg.stage == 6 && b->data0 == MG_B_VULTURE && mg.boss) {
            int16_t bx = b->x;
            ng_chars_remove(mg.boss);
            mg.boss = 0;
            mg_sparks(bx, (int16_t)(MG_GROUND_Y - 40));
            playSFX(SOUND_SFX_10);
            mg_palette(PAL_BOSS, mg_boss_pal(MG_B_SMOGGAR));
            mg.boss = mg_character(K_BOSS, bx, MG_GROUND_Y, PAL_BOSS, NG_RENDER_BAND_ENEMY, MG_B_SMOGGAR);
            if (mg.boss) {
                mg.boss->hp = mg.boss->max_hp = 7 * MG_STOMP_BLOW;   /* seven stomps */
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
        mg_voice_later(MG_VOICE_WIN, 50);
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
        mg.shake = MG_SHAKE_BOSS_DOWN;
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
        if (!mg.demo) { mg_stage_time_final(); mg_stage_rank(); }
    } else {
        b->hp -= damage;
        mg.hud_dirty = 1;
    }
}

/*
 * What landing on a creature does -- besides her thorns and whip, this is
 * how the blight is beaten, and each kind has its own answer:
 *   POP    squashed at one stomp (the soft ones, and anything in the air
 *          caught from above);
 *   FLIP   armour: the first stomp leaves it dazed (upright -- an insect
 *          is never shown on its back), the second ends it -- or a touch
 *          from the side kicks it away, sliding along the road and
 *          bowling over whatever it meets (the one way to deal with the
 *          creatures that can't be stomped);
 *   TOUGH  the slag golem: three stomps, each one staggering it;
 *   HURT   poison skin, thorns, smog: landing on it hurts her.
 */
enum { MG_STOMP_POP, MG_STOMP_FLIP, MG_STOMP_TOUGH, MG_STOMP_HURT };

static uint8_t NEOGEO_USER mg_stomp_rule(uint8_t type)
{
    switch (type) {
    case MG_E_BEETLE: case MG_E_TOXICCRAB:                   return MG_STOMP_FLIP;
    case MG_E_SLAGGOLEM: case MG_E_RHINO: case MG_E_GUNSHIP: return MG_STOMP_TOUGH;
    /* the polluters' machines and the bin bag are armoured; the torch
     * bot's hull is too hot to land on */
    case MG_E_BINOCTO: case MG_E_SAWBOT: case MG_E_DRILLBOT: case MG_E_SMOGSTACK:
    case MG_E_SLUDGEBARREL:                                  return MG_STOMP_TOUGH;
    case MG_E_TORCHBOT:                                      return MG_STOMP_HURT;
    case MG_E_DARTFROG: case MG_E_VINESTING: case MG_E_WRAITH: return MG_STOMP_HURT;
    default:                                                 return MG_STOMP_POP;
    }
}

/* The whip: twice a thorn's bite, twice again while Thorn Might lasts. */
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
    if (mg.hurt || mg.veil || mg.dash || mg.super_surge || mg.art_pose || mg.state != MG_PLAY) return 0;
    if (mg.rising > MG_RISE_TIME - MG_RISE_SAFE) return 0;
    mg.hurt = 90;
    if (mg.attempt_hits < 255) mg.attempt_hits++;
    mg_climb_end();
    mg.attack = mg.combo = mg.cast = 0;
    mg.hud_dirty = 1;
    playSFX(SOUND_SFX_16); /* player hurt */
    ng_feedback_hitstop(MG_HITSTOP_HURT);
    /* A short red flash on her bank alone, over whatever colours she has on. */
    ng_palfx_flash_red(PAL_HERO, ng_palfx_screen_colors(PAL_HERO), MG_HURT_FLASH);
    /* Two small sparks where the blow lands -- it happens on every hit,
     * so it stays small and quick; her bar and her glint say the rest. */
    mg_burst(p->x, (int16_t)(p->y - 30), MG_T_SPARK, 2, -1);
    mg.shake = MG_SHAKE_HURT;
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
        mg.swift = mg.might = mg.veil = mg.spring = mg.crown = mg.lily = 0;
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
 * Secret Arts. Five, one to each kind of valley (its stage file's art
 * "kind"): the Rose Blossom Storm she knows from the start -- the forest,
 * the World Tree, the citadel -- and four she learns from a spirit orb in
 * a hidden vault: the Purifying Rain (the falls, the coast, the reef), the
 * Sunflare Dance (the grove, the works), the Frost Petal Storm (the grotto,
 * the silver cave) and the Leaf Gale (the savanna, the sky road). In a
 * valley she uses its own art once she knows it, the rose storm until
 * then; the roses she gathers are the charges.
 *
 * Each goes the same way. Her colours flare and the valley's shake in the
 * art's light (mg_art_step), a ring of gold goes out from her, and the
 * art's pieces appear at the edges of the screen -- in a halo all round it
 * for the roses and the frost, across the sky for the rain, out of the sun
 * in its corner, from both sides for the gale -- gather a moment, and fly
 * onto every creature on it. A weak one is beaten, however far into the
 * journey. A tough one -- the armoured, and from the fifth valley on the
 * shelled and the machines too -- is left
 * dazed on its last strength, held a long while by the frost; the
 * guardian takes a heavy blow. She can't be touched while it lasts.
 */
static const uint8_t mg_art_pieces[MG_ARTS][4] = {
    { MG_FALL_THORN, 0, MG_FALL_ROSE, 3 },                   /* thorns, roses, lilac */
    { MG_FALL_WATER, MG_FALL_WATER, 1, MG_FALL_WATER },      /* clean water, a daisy */
    { MG_FALL_SPARK, MG_FALL_STAR, 2, MG_FALL_SPARK },       /* sunlight, buttercups */
    { MG_FALL_ICE, 1, MG_FALL_STAR, MG_FALL_ICE },           /* ice, snow-white daisies */
    { MG_FALL_LEAF, MG_FALL_LEAF, 2, MG_FALL_LEAF },         /* leaves, a buttercup */
};
static uint8_t mg_art_live;      /* the art let loose last */
static uint16_t mg_art_hit;      /* who its pieces have struck: a bit a creature, 0x100 the guardian */

static uint8_t NEOGEO_USER mg_art_now(void)
{
    uint8_t k = mg_art_kind[mg.stage < MG_LEVEL_COUNT ? mg.stage : 0];
    return (uint8_t)(((mg.arts_known >> k) & 1u) ? k : MG_ART_BLOSSOM);
}

/* Where the target is now, if it is still the one the piece was called for. */
static void NEOGEO_USER mg_art_track(MGFall *f)
{
    NGCharacter *b = 0;
    int16_t up = 22;
    if (f->aim == MG_AIM_BOSS) { b = mg.boss; up = 46; }
    else if (f->aim && f->aim <= MG_ENEMIES) b = mg.enemies[f->aim - 1u].body;
    if (b && b == f->who) {
        f->tx = (int16_t)((b->x - mg.camera.x - 8) * 8);
        f->ty = (int16_t)((MG_SY(b->y) - up) * 8);
    }
}

static uint8_t NEOGEO_USER mg_enemy_base_hp(uint8_t type);

static void NEOGEO_USER mg_art_strike(MGFall *f)
{
    MGEnemy *e;
    NGCharacter *b;
    uint16_t bit;
    if (f->aim == MG_AIM_BOSS) {
        if (mg.boss && mg.boss == f->who && mg.boss_active && !(mg_art_hit & 0x100u)) {
            mg_art_hit |= 0x100u;
            mg_boss_damage(6);
        }
        return;
    }
    if (!f->aim || f->aim > MG_ENEMIES) return;
    bit = (uint16_t)(1u << (f->aim - 1u));
    e = &mg.enemies[f->aim - 1u];
    b = e->body;
    if (!b || b != f->who || e->mood == MG_MOOD_DYING || (mg_art_hit & bit)) return;
    mg_art_hit |= bit;
    if ((mg_stomp_rule(e->type) == MG_STOMP_TOUGH || (mg.stage >= 4u && mg_enemy_base_hp(e->type) >= 2u)) && b->hp > 1u) {
        b->hp = 1;
        b->vx_fp = b->vy_fp = 0;
        e->hurt = 20;
        e->mood = MG_MOOD_STUNNED;
        e->move_timer = (uint8_t)(mg_art_live == MG_ART_FROST ? 200u : 70u);
        mg_hit_burst(b->x, (int16_t)(b->y - 20));
        playSFX(SOUND_SFX_4);
    } else {
        e->hurt = 0;
        mg_enemy_damage(e, 99);
    }
}

/* Where each of the art's pieces appears, on screen. */
static void NEOGEO_USER mg_art_start(uint8_t art, uint8_t i, int16_t *x, int16_t *y)
{
    switch (art) {
    case MG_ART_RAIN:  *x = (int16_t)(12 + i * 22); *y = (int16_t)(4 + (i & 1u) * 12); break;
    case MG_ART_SUN:   *x = (int16_t)(300 - (i & 3u) * 22 - (i >> 2) * 8); *y = (int16_t)(6 + (i >> 2) * 16); break;
    case MG_ART_GALE:  *x = (int16_t)((i & 1u) ? 312 : 0); *y = (int16_t)(36 + (i >> 1) * 22); break;
    default: {
        /* a halo round the whole screen */
        uint8_t a = (uint8_t)(i * 18u + 192u);
        *x = (int16_t)(152 + ng_trig_mul(148, ng_cos(a)));
        *y = (int16_t)(100 + ng_trig_mul(96, ng_sin(a)));
        break;
    }
    }
}

static void NEOGEO_USER mg_secret_art(void)
{
    uint8_t aims[MG_ENEMIES];
    uint8_t i, n = 0, bossn = 0, art;
    if (mg.art == 0 || mg.state != MG_PLAY || mg.art_pose) return;
    mg.art--;
    mg.hud_dirty = 1;
    art = mg_art_now();
    mg_art_live = art;
    mg_art_hit = 0;
    playSFX(SOUND_SFX_13); /* art power surge */

    /* Her own colours only lift for a moment -- a long recolour read as a
     * costume change. The valley's shake with the art's light instead. */
    mg.flash = 12;
    mg_palette(PAL_HERO, mg_hero_sun_pal);
    mg.attack = 22;
    mg.shake = 24;
    playSFX(SOUND_SFX_9);          /* the clear ring of the purification */
    mg_voice(MG_VOICE_ART);
    mg.art_pose = 36;
    mg_light(MG_LIGHT_SUN, 30);
    mg_secret_art_ring(mg.player->x, (int16_t)(mg.player->y - 30));
    mg_tint_begin(art);

    /* every creature on the screen, and the guardian (four pieces its own) */
    for (i = 0; i < MG_ENEMIES; i++) {
        NGCharacter *b = mg.enemies[i].body;
        int16_t sx;
        if (!b || mg.enemies[i].mood == MG_MOOD_DYING) continue;
        sx = (int16_t)(b->x - mg.camera.x);
        if (sx < -8 || sx > 328) continue;
        aims[n++] = (uint8_t)(i + 1u);
    }
    if (mg.boss && mg.boss_active) bossn = 4;
    mg_falls_begin(SLOT_RAIN, MG_RAIN_SLOTS);
    for (i = 0; i < MG_RAIN_SLOTS; i++) {
        MGFall *f = &mg_falls[i];
        int16_t x, y;
        mg_art_start(art, i, &x, &y);
        f->live = 1;
        f->t = 0;
        f->fade = 0;
        f->sway = (uint8_t)ng_rand();
        f->kind = mg_art_pieces[art][i & 3u];
        f->x = (int16_t)(x * 8);
        f->y = (int16_t)(y * 8);
        f->vx = 0;
        f->vy = 12;
        f->end = 220;
        /* round the halo they go one after another; the rest in a rush */
        f->wait = (uint8_t)((art == MG_ART_BLOSSOM || art == MG_ART_FROST) ? 12u + i : 12u + (i & 7u));
        if (i < bossn) {
            f->aim = MG_AIM_BOSS;
            f->who = mg.boss;
        } else if (n) {
            f->aim = aims[(uint8_t)(i - bossn) % n];
            f->who = mg.enemies[f->aim - 1u].body;
        } else {
            /* nothing here: the pieces cleanse the air where they meet */
            f->aim = MG_AIM_SPOT;
            f->who = 0;
            f->tx = (int16_t)((32 + (int16_t)ng_rand_range(256u)) * 8);
            f->ty = (int16_t)((40 + (int16_t)ng_rand_range(140u)) * 8);
        }
        mg_art_track(f);
    }

    mg_hint(art == mg_art_kind[mg.stage] ? mg_art_words[mg.stage] : mg_art_words[0], PAL_GOLD, 100);
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
    mg.voice_delay = 0;     /* nothing said in the last scene carries over */
    mg.combo_n = mg.combo_t = mg.combo_show = 0;
    mg_backdrop(0x8000);
    ng_fix_clear();
    ng_sprite_hide_all();
    maiya_vblank();
    ng_game_engine_init();
    mg_music_levels();   /* the init forgot them; the music may play on unchanged */
    ng_game_engine_set_hooks(0, mg_collision_hook, 0, mg_before_draw_hook, 0);
    /* A heavy blow holds the whole valley still for a few frames, and an
     * impact event's sound plays through the ordinary effect call. */
    ng_game_engine_set_hitstop_freeze(1);
    ng_feedback_set_sfx_hook(mg_impact_sfx);

    mg_ui_palettes();
    if (mg.stage != stage) mg.rescue_mask = 0;
    mg.stage = stage; mg.state = MG_INTRO;
    mg.flying = (uint8_t)(mg_mech() == MG_M_FLIGHT);
    mg.fly_x = 0;
    mg.wave_mask = 0;
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
        mg.vault_done = 0; mg.charm_seen = 0;
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
    ng_palfx_screen_stop(); mg.win_step = 0; mg.win_wait = 0; mg.cam_dir = 1;
    mg_stage_time_reset();
    mg.held_press = 0;
    for (i = 0; i < MG_PLATFORM_COUNT; i++) { mg.ledge_stand[i] = 0; mg.ledge_gone[i] = 0; }
    mg.gate_shown = 0;
    mg.npc_mask = 0; mg.npc_live = 0; mg.npc_here = 0;
    mg.swift = mg.might = mg.veil = 0; mg.spring = mg.crown = 0; mg.lily = 0; mg.leap_window = 0;
    mg.art_pose = 0; mg.leaping = 0; mg.veil_lit = 0;
    mg.flash = 0; mg.angel = 0; mg.hurt_lit = 0; mg.art_wave = 0;
    for (i = 0; i < MG_NPC_SLOTS; i++) mg.npcs[i] = 0;
    mg.notice = 0; mg.facing = 0; mg.kills = 0;
    mg.state_timer = 0;
    mg.hint_timer = 0;
    mg.previous_joy = poll_joystick();

    ng_level_set_world_bounds(0, 0, (int16_t)level->width, 224);
    /* The road, in stretches: every pit is a gap in the ground itself.
     * (The sky has none.) */
    if (!mg.flying) {
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
    mg_palette(PAL_DARTFROG, mg_dartfrog_pal);
    mg_palette(PAL_SMOGBAT, mg_smogbat_pal);
    mg_palette(PAL_POACHDRONE, mg_poachdrone_pal);
    mg_palette(PAL_CHEMFLY, mg_chemfly_pal);
    mg_palette(PAL_PLASTICBAT, mg_plasticbat_pal);
    mg_palette(PAL_SLAGGOLEM, mg_slaggolem_pal);
    mg_palette(PAL_VINESTING, mg_vinesting_pal);
    mg_palette(PAL_SPOREGOB, mg_sporegob_pal);
    mg_palette(PAL_RHINO, mg_rhino_pal);
    mg_palette(PAL_DRAGONFLY, mg_dragonfly_pal);
    mg_palette(PAL_GNAT, mg_gnat_pal);
    mg_palette(PAL_GUNSHIP, mg_gunship_pal);
    mg_palette(PAL_BAGOCTO, mg_bagocto_pal);
    mg_palette(PAL_BINOCTO, mg_binocto_pal);
    mg_palette(PAL_SAWBOT, mg_sawbot_pal);
    mg_palette(PAL_DRILLBOT, mg_drillbot_pal);
    mg_palette(PAL_TORCHBOT, mg_torchbot_pal);
    mg_palette(PAL_SMOGSTACK, mg_smogstack_pal);
    mg_palette(PAL_SLUDGEBARREL, mg_sludgebarrel_pal);
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
        mg_rot_bank(PAL_BLOCK_ROT, block_pal);           /* rotten: faded toward grey-green */
    }

    /* Load corrupted stage background */
    mg_background(level->background, 0);

    /*
     * Maiya arrives the way she leaves: out of the sky.  A fresh run drops
     * her in above the road; a respawn uses the same entrance at the
     * position where she fell, keeping the key and gate progress.
     */
    mg.entrance = (uint8_t)!mg.flying;
    mg.player = mg_character(K_PLAYER, mg.flying ? 96 : 64, mg.flying ? 130 : -24,
                             PAL_HERO, NG_RENDER_BAND_PLAYER, 0);
    if (!mg.player) return;
    mg.player->hp = mg.player->max_hp = MAX_HP;
    ng_physics_attach(mg.player, NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS);
    mg_player_gravity();
    if (mg.flying) {
        /*
         * On the Sky Road she kneels on the sun eagle's back. Her position
         * is the eagle's talons -- what comes down on a creature from above
         * -- so she is drawn higher, and her body takes in the bird.
         */
        mg.player->sprite_offset_x = -44;
        mg.player->sprite_offset_y = -88;
        ng_char_set_body(mg.player, -24, -70, 48, 70);
        mg.eagle = mg_character(K_EAGLE, mg.player->x, mg.player->y, PAL_EAGLE, NG_RENDER_BAND_ENEMY, 0);
    }
    mg_frame(mg.player, MG_F_IDLE0, 0);
    mg.player_prev_y = mg.player->y;

    /* Camera setup */
    ng_camera_init(&mg.camera);
    ng_camera_set_bounds(&mg.camera, 0, 0, (int16_t)(level->width - 1), NG_SCREEN_H - 1);
    ng_camera_set_dead_zone(&mg.camera, MG_CAM_DEAD, 0);
    ng_camera_set_look_ahead(&mg.camera, MG_CAM_LEAD, 0, MG_CAM_LEAD_RATE);
    ng_camera_set_follow_speed(&mg.camera, MG_CAM_FOLLOW);
    mg.camera.look_ahead_cur_x = MG_CAM_LEAD;   /* she sets out looking right */
    mg.shake = 0;
    mg.shake_x = 0;
    mg.clear_bonus = 0;   /* no clear card up yet */
    mg.vault = 0;
    mg.cam_y = 0;
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
    for (i = 0; i < MG_SIGNS; i++) {
        ng_sprite_group_init(&mg.signs[i], (uint16_t)(SLOT_SIGN + i * 2), 2, 2,
                             mg_hazard_tiles[MG_HZ_SIGN0], PAL_HAZARD);
        ng_sprite_group_set_visible(&mg.signs[i], 0);
    }
    for (i = 0; i < MG_DECOR_SLOTS; i++) {
        ng_sprite_group_init(&mg.decor[i], (uint16_t)(SLOT_DECOR + i * 2), 2, 2,
                             mg_decor_tiles[MG_D_GRASS], PAL_DECOR);
        ng_sprite_group_set_visible(&mg.decor[i], 0);
    }
    for (i = 0; i < MG_VINE_COUNT; i++) {
        const MGVine *v = &mg_vines[stage][i];
        uint8_t rows = v->x ? (uint8_t)((v->bottom - v->top + 15) / 16) : 1;
        if (rows > 16) rows = 16;
        ng_sprite_group_init(&mg.vines[i], (uint16_t)(SLOT_VINE + i * 2), 2, 16,
                             mg_decor_tiles[mg_stage_climb[stage]], PAL_DECOR);
        ng_sprite_group_set_active_rows(&mg.vines[i], rows);
        ng_sprite_group_set_visible(&mg.vines[i], 0);
    }
    mg_palette(PAL_FRONT, mg_front_pal);
    for (i = 0; i < MG_FRONT_SLOTS; i++) {
        /* A frond, then the valley's own stone, then a frond again. */
        uint16_t tile = (i == 1) ? mg_front_tiles[MG_FR_STONE_GRASS + mg_stage_blocks[stage]]
                                 : mg_front_tiles[MG_FR_FERN];
        ng_sprite_group_init(&mg.front[i], (uint16_t)(SLOT_FRONT + i * 2), 2, 3,
                             tile, PAL_FRONT);
        ng_sprite_group_set_visible(&mg.front[i], 0);
    }

    ng_sprite_group_init(&mg.gate, SLOT_GATE, 2, 3, mg_gate_tiles[0], PAL_GATE);
    ng_sprite_group_set_visible(&mg.gate, 0);
    mg_landmark_setup();

    ng_sprite_group_init(&mg.cage, SLOT_CAGE, 2, 2, mg_prop_tiles[MG_P_CHEST], PAL_PROP);
    ng_sprite_group_set_visible(&mg.cage, 0);

    if (mg.hero_choice) mg_moon_bank(PAL_FX, mg_fx_pal);
    else mg_palette(PAL_FX, mg_fx_pal);
    ng_sprite_group_init(&mg.fx, SLOT_FX, 4, 4, mg_fx_tiles[0], PAL_FX);
    ng_sprite_group_set_visible(&mg.fx, 0);
    mg.fx_time = 0;

    /* Reserve the mandatory key before streaming optional pickups. */
    if (mg.flying) mg.gate_unlocked = 1;   /* no gate in the sky: its guardian waits at the end */
    else if (!mg.has_key && !mg.gate_unlocked) mg_drop_key((int16_t)mg_key_pos[stage][0], (int16_t)mg_key_pos[stage][1]);

    /* Spawn initial wave immediately so enemies are on-screen from frame 1 */
    mg_spawn();

    maiya_vblank();
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
        uint8_t at = 8, n = (uint8_t)(stage + 1);
        if (n >= 10) stage_msg[at++] = (char)('0' + n / 10u);
        stage_msg[at++] = (char)('0' + n % 10u);
        stage_msg[at++] = ':';
        stage_msg[at++] = ' ';
        while (level->name[k] && k < 18) {
            stage_msg[at + k] = level->name[k];
            k++;
        }
        stage_msg[at + k] = '\0';
        mg_hint(stage_msg, PAL_GOLD, 120);
    }

    if (mg.entrance || mg.flying) {
        playSFX(SOUND_SFX_13);       /* the sun answers her */
        playSFX(SOUND_SFX_15);
        mg_centre(ROW_CARD + 6, retry ? "MAIYA: I AM NOT DONE YET!"
                                      : "MAIYA: THE VALLEY CALLED ME!", PAL_SKY);
        {
            static const char *const warn[] = {
                0, 0, 0,
                "ROTTEN LEDGES CRUMBLE - KEEP MOVING", "THE ROAD IS ICE - SHE WILL SLIDE",
                "UNDERWATER - SWIM ANY WAY, A TO STROKE",
                "ON THE SUN EAGLE: B THORNS, C SWOOP",
            };
            uint8_t m = mg_mech();
            if (m && m < sizeof(warn) / sizeof(warn[0]) && warn[m]) mg_centre(ROW_CARD + 8, warn[m], PAL_GOLD);
        }
    }

    mg.music_on = 0;
    mg.music_wait = 0;
    mg_music(level->music);
    if (mg.entrance || mg.flying) mg_voice_later(retry ? MG_VOICE_RETRY : MG_VOICE_START, 40);
}

uint8_t NEOGEO_USER maiya_hero_choice(void);

/* ------------------------------------------------------------------ */
/*  Saved data: the score table and a few totals                      */
/* ------------------------------------------------------------------ */
/*
 * Kept in the header's save block (ng_save_*), which an arcade board's
 * system ROM holds in backup RAM between sessions. Bump MG_SAVE_VERSION
 * whenever MGSave changes shape: an old block then reads as not Maiya's
 * and starts afresh instead of being misread.
 */
enum { MG_SAVE_VERSION = 1, MG_SCORES = 10 };

typedef struct {
    uint8_t  name[3];
    uint8_t  stage;              /* the mission reached, from 1 */
    uint32_t score;
} MGScore;

typedef struct {
    MGScore  top[MG_SCORES];     /* best first */
    uint8_t  best_stage;         /* furthest mission anyone has reached */
    uint8_t  clears;             /* times the whole journey was finished */
    uint16_t plays;              /* games started on this board */
} MGSave;

static MGSave *NEOGEO_USER mg_saved(void)
{
    return (MGSave *)ng_save_data();
}

void NEOGEO_USER maiya_save_reset(void)
{
    static const char names[MG_SCORES][3] = {
        {'M','A','I'}, {'L','U','N'}, {'S','U','N'}, {'E','G','L'}, {'R','O','S'},
        {'F','O','X'}, {'O','W','L'}, {'F','E','R'}, {'L','E','A'}, {'S','E','D'},
    };
    MGSave *sv;
    uint8_t i;
    ng_save_format(MG_SAVE_VERSION, (uint16_t)sizeof(MGSave));
    sv = mg_saved();
    for (i = 0; i < MG_SCORES; i++) {
        sv->top[i].name[0] = (uint8_t)names[i][0];
        sv->top[i].name[1] = (uint8_t)names[i][1];
        sv->top[i].name[2] = (uint8_t)names[i][2];
        sv->top[i].stage = (uint8_t)(i < 3 ? 3 - i : 1);
        sv->top[i].score = (uint32_t)(50000u - (uint32_t)i * 5000u);
    }
    ng_save_commit();
}

void NEOGEO_USER maiya_save_check(void)
{
    if (!ng_save_valid(MG_SAVE_VERSION, (uint16_t)sizeof(MGSave))) maiya_save_reset();
}

/* ------------------------------------------------------------------ */
/*  The operator's settings (software DIPs, see neogeo_mvs.c)         */
/* ------------------------------------------------------------------ */
/* A console always reads the defaults: 3 lives, 3 continues, NORMAL,
 * demo sound on, how-to-play shown. */
/* The system's copy of the soft DIPs holds the table's special list (all
 * 0xFF) ahead of the options once a system ROM has filled it in; a system
 * that never did leaves zeros, which would read as one life and no
 * continues. Then the table's own defaults stand. */
static uint8_t NEOGEO_USER mg_dips_set(void)
{
    return (uint8_t)(*(const volatile uint8_t *)BIOS_GAME_DIP == 0xFFu);
}

/* Three lives unless the operator chose otherwise. */
static uint8_t NEOGEO_USER mg_dip_lives(void)
{
    static const uint8_t lives[5] = { 1, 2, 3, 4, 5 };
    uint8_t o = ng_dip_option(0);
    return (mg_dips_set() && o < 5) ? lives[o] : 3;
}

static uint8_t NEOGEO_USER mg_dip_continues(void)
{
    static const uint8_t continues[4] = { 0, 1, 3, 5 };
    uint8_t o = ng_dip_option(1);
    if (!mg_dips_set()) return 3;
    return o < 4 ? continues[o] : MAX_CONTINUES;
}

/* 0 easy, 1 normal, 2 hard, 3 expert */
static uint8_t NEOGEO_USER mg_dip_difficulty(void)
{
    uint8_t o = ng_dip_option(2);
    return (mg_dips_set() && o < 4) ? o : 1;
}

uint8_t NEOGEO_USER maiya_dip_demo_sound(void)
{
    return (uint8_t)(ng_dip_option(3) == 0);
}

static uint8_t NEOGEO_USER mg_dip_how_to_play(void)
{
    return (uint8_t)(ng_dip_option(4) == 0);
}

void NEOGEO_USER maiya_boot(void)
{
    mg.kinds_met = 0;
    mg.lives = mg_dip_lives(); mg.art = MAX_ART; mg.score = 0; mg.rescue_mask = 0;
    mg.arts_known = 1u << MG_ART_BLOSSOM;
    mg.continues = mg_dip_continues();
    mg.difficulty = mg_dip_difficulty();
    mg.coins = mg.flowers = mg.critters = 0;
    mg.hero_choice = maiya_hero_choice();
    mg.session_over = 0;
    mg.life_pickups_used = 0;
    mg.next_life_score = MG_BONUS_LIFE_SCORE_FIRST;
    mg.thorns = 0; mg.weapon = MG_W_NONE; mg.weapon_ammo = 0;
    {
        /* a real game (the attract demo starts through maiya_demo_begin) */
        MGSave *sv = mg_saved();
        maiya_save_check();
        if (sv->plays < 0xFFFFu) sv->plays++;
        ng_save_commit();
    }
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
/*
 * The attract rounds show three valleys in turn -- the Emerald Forest, the
 * Crystal Grotto (played as Luna) and the Rio Negro Works. The very first
 * round walks the forest from its start; every later one drops her in
 * somewhere along the road, a different place each time, well short of
 * the gate. The demo never goes through the gate: no guardian is shown.
 */
static const uint8_t mg_demo_stages[3] = { 0, 4, 6 };

void NEOGEO_USER maiya_demo_begin(void)
{
    static uint8_t demo_round = 0;
    uint8_t demo_stage = mg_demo_stages[demo_round % 3u];

    mg.demo = 1;
    mg.kinds_met = 0;
    mg.lives = 3; mg.art = MAX_ART; mg.score = 0; mg.rescue_mask = 0;
    mg.arts_known = (1u << MG_ARTS) - 1u;   /* the attract shows each valley's own */
    mg.coins = mg.flowers = mg.critters = 0;
    mg.continues = 0;
    mg.session_over = 0;
    mg.life_pickups_used = 0;
    mg.next_life_score = MG_BONUS_LIFE_SCORE_FIRST;
    mg.thorns = 0; mg.weapon = MG_W_NONE; mg.weapon_ammo = 0;
    mg.hero_choice = (uint8_t)(demo_stage == 4u);     /* the grotto is Luna's */
    mg_scene(demo_stage, 0);
    if (demo_round) {
        /* somewhere along the road: on it, clear of breaks and hazards,
         * with a good stretch left before the gate */
        const MGLevel *lv = &mg_levels[demo_stage];
        int16_t span = (int16_t)(lv->gate_x - 1800 - 300);
        uint8_t tries;
        for (tries = 0; tries < 16u && span > 0; tries++) {
            int16_t x = (int16_t)(300 + (int16_t)ng_rand_range((uint16_t)span));
            uint8_t k, clear = (uint8_t)!mg_over_pit(x, 48);
            for (k = 0; k < MG_HAZARD_COUNT && clear; k++) {
                const MGHazard *hz = &lv->hazards[k];
                if (hz->type && x > hz->x - 60 && x < hz->x + hz->width + 60) clear = 0;
            }
            if (!clear) continue;
            ng_char_set_pos(mg.player, x, mg.player->y);
            ng_camera_snap(&mg.camera, (int16_t)(x - 100), 0);
            break;
        }
    }
    ng_fix_clear_rect(1, ROW_HINT, 38, 9, PAL_TEXT);
    mg_centre(ROW_CARD + 2, "ATTRACT MODE", PAL_GOLD);
    mg.state_timer = 80;
    demo_round++;
}

void NEOGEO_USER maiya_demo_end(void)
{
    mg.demo = 0;
    mg.session_over = 0;
    soundStopAll();
    ng_sprite_hide_all();
    ng_fix_clear();
}

/* The demo never ends on a game over: it just picks the valley up again.
 * Nor does it ever show a guardian: it stops short of the gate. */
uint8_t NEOGEO_USER maiya_demo_spent(void)
{
    const MGLevel *lv = &mg_levels[mg.stage];
    return (uint8_t)(mg.state == MG_OVER || mg.state == MG_DONE || mg.state == MG_ENDING ||
                     mg.state == MG_WARP || mg.state == MG_BOSS_INTRO || mg.state == MG_CLEAR ||
                     mg.boss_active ||
                     (mg.player && lv->gate_x && mg.player->x > (int16_t)(lv->gate_x - 160)));
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

/* Waits out the logo a frame at a time; 1 as soon as a Start (or, on an
 * arcade board, a credit) is waiting -- the logo gives way at once rather
 * than swallowing the press. */
static uint8_t NEOGEO_USER mg_logo_wait(uint8_t frames)
{
    while (frames--) {
        maiya_vblank();
        if (maiya_start_pending()) return 1;
    }
    return 0;
}

static uint8_t NEOGEO_USER mg_logo_word(const uint8_t *word, uint8_t len,
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
        if (mg_logo_wait(3)) return 1;
    }
    return 0;
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
    mg_backdrop(BLACK);
    ng_fix_init();
    ng_fix_clear();
    mg_ui_palettes();
    maiya_vblank();

    /* FM alone under the logo: three rising notes and a held fifth. */
    soundStopAll();
    mg.music_on = 0;
    soundSetADPCMBVolume(0x00);
    soundSetFMVolume(0x0C);
    playFMTrack(SOUND_FM_TRACK_1);

    if (mg_logo_word(mg_logo_eagle, 5, 10, 10, PAL_GOLD) ||
        mg_logo_word(mg_logo_software, 8, 4, 17, PAL_TEXT)) goto done;

    for (i = 0; i < 8; i++) {
        ng_fix_putc((uint8_t)(9 + i * 3), 24, (char)GLYPH_SPARK, PAL_GOLD);
        if (mg_logo_wait(2)) goto done;
    }
    ng_fix_puts(16, 27, "PRESENTS", PAL_SKY);

    mg_logo_wait(120);

done:
    soundStopAll();
    ng_fix_clear();
    maiya_vblank();
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
#define MG_CHOOSER_FADE       8u   /* frames it takes to rise out of white */

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
    mg_centre(8,  "LEFT / RIGHT: WALK   HOLD B: RUN", PAL_TEXT);
    mg_centre(9,  "UP: CLIMB A VINE, OR TURN THE KEY", PAL_TEXT);
    mg_centre(10, "A: JUMP (HOLD FOR HEIGHT)", PAL_TEXT);
    mg_centre(11, "RUN, THEN JUMP: HIGHER STILL", PAL_TEXT);
    mg_centre(12, "KNEEL, THEN UP + A: A HIGH LEAP", PAL_TEXT);
    mg_centre(13, "B: THROW A THORN, OR STRIKE UP CLOSE", PAL_TEXT);
    mg_centre(14, "C: DASH      D: SECRET ART", PAL_TEXT);
    mg_centre(16, "DOWN, FORWARD + B: ROSE BLOSSOM SURGE", PAL_SKY);
    mg_centre(17, "FWD, DOWN, DOWN-FWD + B: RISING BLOOM", PAL_SKY);
    mg_centre(18, "LAND ON A CREATURE TO BEAT IT TOO", PAL_SKY);
    mg_centre(19, "A SKY LILY: UP + A IN THE AIR, ONE MORE", PAL_SKY);
    mg_centre(22, "PRESS ANY BUTTON TO BEGIN", PAL_GOLD);

    poll_joystick_edge();
    /* The confirm that brought her here shouldn't also skip this: give it
     * a moment to let go before a press counts. */
    for (i = 0; i < 20; i++) { maiya_vblank(); poll_joystick_edge(); }

    for (;;) {
        maiya_vblank();
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
    for (i = 0; i < 20; i++) { maiya_vblank(); poll_joystick_edge(); }

    for (;;) {
        maiya_vblank();
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
    ng_palfx_screen_fade_in(NG_PALFX_WHITE, MG_CHOOSER_FADE);
    maiya_vblank();

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
    for (i = 0; i < MG_CHOOSER_FADE; i++) {
        ng_palette_fx_update();
        maiya_vblank();
        t++;
        mg_chooser_tick(t, 0);
    }

    poll_joystick_edge();
    /* The same Start press that opened this screen is still fresh on a
     * real cabinet's own change-detection; without this pause it could
     * read as an immediate confirm and blow straight through to
     * gameplay, which looked exactly like Start not doing anything. */
    for (;;) {
        maiya_vblank();
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
        maiya_vblank();
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
    if (mg_dip_how_to_play()) mg_show_how_to_play();
    mg_show_intro_story();
}

/* ------------------------------------------------------------------ */
/*  Spawning: Enemies, Allies & Guardians                             */
/* ------------------------------------------------------------------ */
/* How hard the creatures push, by valley: 1 is a stroll, 4 is the citadel. */
static int16_t NEOGEO_USER mg_pace(int16_t base)
{
    /* 10/12/14/16/18/20 by valley at NORMAL; the operator's difficulty
     * slows it two steps (EASY) or quickens it two or four (HARD, EXPERT). */
    int16_t scale = (int16_t)(10 + mg.stage * 2 + ((int16_t)mg.difficulty - 1) * 2);
    if (scale < 8) scale = 8;
    return (int16_t)((base * scale) / 16);
}

/* How often a creature attacks, shortened by the valley: the same move
 * comes round about 40% sooner by the last one, and a little sooner still
 * on the harder settings. `base` in frames. */
static uint16_t NEOGEO_USER mg_rate(uint16_t base)
{
    uint16_t cut = (uint16_t)(mg.stage * 4u + (mg.difficulty >= 2 ? 6u : 0u));
    return (uint16_t)(base - (uint16_t)((base * cut) / 100u));
}

/* Creatures that live in the air: they hover and swoop instead of falling
 * to the road, so they're spawned without gravity. */
static uint8_t NEOGEO_USER mg_enemy_flies(uint8_t type)
{
    return (uint8_t)(type == MG_E_CROW || type == MG_E_DRONE || type == MG_E_JELLYFISH ||
                     type == MG_E_ACIDMOTH || type == MG_E_SMOGBAT || type == MG_E_POACHDRONE ||
                     type == MG_E_CHEMFLY || type == MG_E_PLASTICBAT || type == MG_E_WRAITH ||
                     (type >= MG_E_RHINO && type <= MG_E_GUNSHIP));   /* the Sky Road's; the polluters walk */
}

/* How many hits each kind takes before the valley's own difficulty is added. */
static uint8_t NEOGEO_USER mg_enemy_base_hp(uint8_t type)
{
    switch (type) {
    case MG_E_SLAGGOLEM: return 4;
    case MG_E_BEETLE:    return 3;
    case MG_E_DRONE: case MG_E_TOXICCRAB:
    case MG_E_POACHDRONE: case MG_E_VINESTING: return 2;
    case MG_E_RHINO: case MG_E_GUNSHIP: return 3;
    case MG_E_BAGOCTO: return 2;
    case MG_E_BINOCTO: case MG_E_TORCHBOT: case MG_E_SLUDGEBARREL: return 3;
    case MG_E_SAWBOT: case MG_E_DRILLBOT: return 4;
    case MG_E_SMOGSTACK: return 5;
    default:             return 1;
    }
}

/*
 * A drift that rises and falls in a true sine wave (ng_trig) about a height:
 * `swing` px each way, one beat a turn of `beat` (256 steps), at the wave's
 * own speed -- 2 pi swing / period, in 8.8 -- plus a pull back onto the
 * curve if it has wandered off it.
 */
static void NEOGEO_USER mg_wave(NGCharacter *b, int16_t centre, int16_t swing, int16_t speed, uint8_t beat)
{
    int16_t pull = (int16_t)(centre + ng_trig_mul(swing, ng_sin(beat)) - b->y);
    if (pull > 16) pull = 16;
    if (pull < -16) pull = -16;
    b->vy_fp = (int16_t)(ng_trig_mul(speed, ng_cos(beat)) + (pull << 3));
}

/* The first time she meets a kind of creature in a game, a line on the
 * hint row names it and how to beat it (the forest's slime and beetle,
 * met in the first seconds, need no introduction). */
static void NEOGEO_USER mg_kind_hint(uint8_t type)
{
    static const char *const hint[MG_E_SLUDGEBARREL + 1] = {
        [MG_E_CROW]       = "CROW: IT DIVES - STRIKE AS IT SWOOPS",
        [MG_E_GOBLIN]     = "GOBLIN: IT HURLS SCRAP FROM LEDGES",
        [MG_E_WORM]       = "WORM: IT SPITS FROM ITS PIPE",
        [MG_E_DRONE]      = "DRONE: IT HOVERS AND FIRES PULSES",
        [MG_E_JELLYFISH]  = "JELLYFISH: LAND ON IT TO POP IT",
        [MG_E_TOXICCRAB]  = "TOXIC CRAB: IT CHARGES UP CLOSE",
        [MG_E_ACIDMOTH]   = "ACID MOTH: IT WEAVES - WAIT, THEN HIT",
        [MG_E_DARTFROG]   = "DART FROG: POISON SKIN - DON'T STOMP",
        [MG_E_SMOGBAT]    = "SMOG BAT: IT DIVES OUT OF THE HAZE",
        [MG_E_POACHDRONE] = "POACHER DRONE: DODGE ITS NET",
        [MG_E_CHEMFLY]    = "CHEM FLY: IT LOOPS - HIT IT MID-LOOP",
        [MG_E_PLASTICBAT] = "PLASTIC BAT: IT DROPS TRASH ON YOU",
        [MG_E_SLAGGOLEM]  = "SLAG GOLEM: JUMP ITS SHOCKWAVE",
        [MG_E_VINESTING]  = "VINE STING: ROOTED - KEEP OUT OF REACH",
        [MG_E_SPOREGOB]   = "SPORE GOBLIN: DODGE ITS TOXIC PUFF",
        [MG_E_RHINO]      = "HORN BEETLE: IT RAMS - THREE BLOWS",
        [MG_E_DRAGONFLY]  = "DRAGONFLY: IT WEAVES IN A LINE",
        [MG_E_GNAT]       = "GNATS: A SWARM - SCATTER THEM",
        [MG_E_GUNSHIP]    = "GUNSHIP: IT FIRES - STRIKE FROM ABOVE",
        [MG_E_BAGOCTO]    = "BAG OCTOPUS: IT FLINGS PLASTIC",
        [MG_E_BINOCTO]    = "BIN OCTOPUS: TOUGH, FLINGS TWO CANS",
        [MG_E_SAWBOT]     = "SAW BOT: IT REVS, THEN CHARGES",
        [MG_E_DRILLBOT]   = "DRILL BOT: IT BURSTS - JUMP OVER",
        [MG_E_TORCHBOT]   = "TORCH BOT: FIRE UP CLOSE, HIT IT FAR",
        [MG_E_SMOGSTACK]  = "SMOG STACK: DODGE ITS SMOG BALLS",
        [MG_E_SLUDGEBARREL] = "SLUDGE BARREL: IT SPITS UP AND OVER",
    };
    uint32_t bit;
    if (type > MG_E_SLUDGEBARREL || type == MG_E_WRAITH || mg.demo || mg.state != MG_PLAY) return;
    bit = (uint32_t)1u << type;
    if (mg.kinds_met & bit) return;
    mg.kinds_met |= bit;
    if (hint[type]) mg_hint(hint[type], PAL_WARN, 120);
}

static MGEnemy *NEOGEO_USER mg_spawn_enemy(uint8_t type, int16_t x, int16_t y, uint8_t posted)
{
    uint8_t slot;
    MGEnemy *e;
    uint8_t pal = mg_enemy_palette(type);

    {
        uint8_t cap = (uint8_t)(mg.flying ? MG_ENEMIES : MG_ROAD_ENEMIES);
        for (slot = 0; slot < cap; slot++) if (!mg.enemies[slot].body) break;
        if (slot == cap) return 0;
    }

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
    if (mg.difficulty >= 2) e->body->hp++;   /* HARD and EXPERT */
    if (mg_stomp_rule(type) == MG_STOMP_TOUGH) e->body->hp = (uint8_t)(3u + (mg.difficulty >= 2));
    if (type == MG_E_GNAT || type == MG_E_DRAGONFLY) e->body->hp = 1;   /* one of a flight: one blow */
    /* The polluters are the valleys' real foes: tougher the further she has
     * come, a point more every three valleys. */
    if (type >= MG_E_BAGOCTO)
        e->body->hp = (uint8_t)(mg_enemy_base_hp(type) + mg.stage / 3u + (mg.difficulty >= 2));
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
    e->timer = (uint16_t)(slot * 29 + (ng_rand() & 31));
    e->hurt = 0;
    e->home = x;
    e->mood = 0;
    e->move_timer = 0;
    e->heading = 1;
    e->face = (int8_t)((mg.player && mg.player->x < x) ? -1 : 1);
    e->form = 0;
    e->slot_i = 0;
    e->base_y = y;
    e->age = 0;
    mg_kind_hint(type);
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
        if (p->width && p->y >= 64 && x >= p->x && x < (int16_t)(p->x + p->width)) return p->y;
    }
    return MG_GROUND_Y;
}

/*
 * The guardian and its lair: the road's creatures and shots are cleared,
 * the guardian stands at the far end, the arena's cover and backdrop go
 * up. Returns 0 if there was no character slot for it.
 */
static void NEOGEO_USER mg_ship_approach(NGCharacter *b);

/*
 * A guardian takes the arena: its palette and body, its cover and its
 * lair behind (the open sky, on the Sky Road), on the wing if it flies,
 * with `stomps` of health (x MG_STOMP_BLOW, then the operator's
 * difficulty: 80%, 100%, 120% or 140%).
 */
static uint8_t NEOGEO_USER mg_boss_spawn(uint8_t style, uint8_t stomps)
{
    const MGLevel *level = &mg_levels[mg.stage];
    uint8_t flies = (uint8_t)(style == MG_B_OWL || style == MG_B_VULTURE || style == MG_B_AIRSHIP);

    mg.boss_home = (int16_t)(level->width - 160);
    mg_palette(PAL_BOSS, mg_boss_pal(style));
    mg.boss = mg_character(K_BOSS, (int16_t)(level->width - 70), MG_GROUND_Y, PAL_BOSS, NG_RENDER_BAND_ENEMY, style);
    if (!mg.boss) return 0;
    mg.boss_style_now = style;
    mg.boss_active = 1;
    mg.boss_timer = 0;
    mg.boss_rage = 0;
    mg.boss_direction = 0;
    mg.boss_backoff = 0;
    mg.boss_phase = 0;
    mg.boss_px = 0;          /* the guardian's bar fills in as it appears */
    mg_arena_setup(style);
    if (mg.flying) {
        /* fought in the open sky: no cover, and the sky stays behind */
        mg.arena[0].width = 0;
        mg.arena[1].width = 0;
    } else {
        mg_arena_background(style);
    }
    if (style == MG_B_OWL || style == MG_B_VULTURE) ng_char_set_pos(mg.boss, mg.boss->x, MG_BOSS_SKY_Y);
    mg.boss->hp = mg.boss->max_hp =
        (uint8_t)(((uint16_t)stomps * MG_STOMP_BLOW * (8u + mg.difficulty * 2u)) / 10u);
    ng_physics_attach(mg.boss, NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS);
    if (flies) ng_physics_set_gravity(mg.boss, 0, 8 * NG_FP_ONE);   /* they fight on the wing */
    else ng_physics_set_gravity(mg.boss, 56, 6 * NG_FP_ONE);
    if (style == MG_B_AIRSHIP) {
        /* its health, shared out: each stack three tenths, the bridge the rest */
        uint8_t hp = mg.boss->hp;
        mg.ship_part[MG_SHIP_STACK_L] = mg.ship_part[MG_SHIP_STACK_R] = (uint8_t)((hp * 3u) / 10u);
        mg.ship_part[MG_SHIP_BRIDGE] = (uint8_t)(hp - 2u * mg.ship_part[MG_SHIP_STACK_L]);
        mg.ship_target = 0xFFu;
        mg.ship_z = 110;
        mg_ship_approach(mg.boss);
    }
    mg.boss->vx_fp = 0;
    return 1;
}

/* The guardian meets her at the end of the road: the road's creatures and
 * shots are cleared first. A stage with a rush (its file's "rush") sends
 * its returning guardians first, three stomps each. */
static uint8_t NEOGEO_USER mg_boss_arrive(void)
{
    const MGLevel *level = &mg_levels[mg.stage];
    uint8_t i;
    for (i = 0; i < MG_ENEMIES; i++) {
        if (mg.enemies[i].body) ng_chars_remove(mg.enemies[i].body);
        mg.enemies[i].body = 0;
    }
    for (i = 0; i < MG_SHOTS; i++) mg.shots[i].life = 0;
    if (mg_rush[mg.stage][0] != 0xFFu) {
        mg.rush_i = 0;
        return mg_boss_spawn(mg_rush[mg.stage][0], 3);
    }
    mg.rush_i = 0xFFu;
    return mg_boss_spawn(level->boss_style, level->boss_hp);
}

/* It speaks, she answers, and the fight's music starts. */
static void NEOGEO_USER mg_boss_announce(void)
{
    uint8_t k = mg.stage;
    if (mg.rush_i != 0xFFu) {
        /* one of the rush: the valley it guarded first has its words */
        for (k = 0; k < MG_LEVEL_COUNT && mg_levels[k].boss_style != mg.boss_style_now; k++) {}
        if (k == MG_LEVEL_COUNT) k = mg.stage;
    }
    playSFX(SOUND_SFX_14); /* boss roar */
    mg.state = MG_BOSS_INTRO;
    mg.state_timer = 210;
    ng_fix_clear_rect(1, ROW_CARD, 38, 9, PAL_TEXT);
    mg_centre(ROW_CARD, mg_levels[k].guardian, PAL_WARN);
    mg_centre(ROW_CARD + 2, mg_boss_taunt[k], PAL_WARN);
    mg_centre(ROW_CARD + 5, "MAIYA", PAL_GOLD);
    mg_centre(ROW_CARD + 7, mg_boss_reply[k], PAL_SKY);
    mg_music_to(SOUND_TRACK_H);
}

static void NEOGEO_USER mg_warp_begin(void);

/*
 * The spawn script (a stage file's "waves"): a flight of creatures sent
 * when the view's right edge reaches its x, flying its formation about its
 * height -- in a row, a wave, a V, dropping from above to dive at her,
 * looping, swarming, charging, or keeping pace while firing (mg_form_step).
 * A wave whose place is already well behind the view (a retry further on)
 * is dropped, not sent late.
 */
static void NEOGEO_USER mg_wave_scan(void)
{
    const MGWave *w = mg_waves[mg.stage];
    int16_t edge = (int16_t)(mg.camera.x + NG_SCREEN_W);
    uint8_t i, k;

    if (!w[0].x) return;
    for (i = 0; i < MG_WAVE_COUNT; i++, w++) {
        uint32_t bit = (uint32_t)1u << i;
        if (!w->x) break;
        if (mg.wave_mask & bit) continue;
        if (w->x > edge + 16) break;              /* in x order: the rest lie further on */
        mg.wave_mask |= bit;
        if ((int16_t)(w->x + 200) < edge) continue;
        for (k = 0; k < w->count; k++) {
            int16_t x = (int16_t)(edge + 24), y = w->y;
            MGEnemy *e;
            switch (w->form) {
            case MG_FORM_VEE:
                x = (int16_t)(x + ((k + 1) >> 1) * 26);
                y = (int16_t)(y + ((k & 1) ? 1 : -1) * ((k + 1) >> 1) * 18);
                break;
            case MG_FORM_DIVE:
                x = (int16_t)(mg.camera.x + 150 + k * 56);
                y = -30;
                break;
            case MG_FORM_SWARM:
                x = (int16_t)(x + (k % 3) * 20);
                y = (int16_t)(y + (int16_t)((k * 23) % 50) - 25);
                break;
            case MG_FORM_CHARGE:
                x = (int16_t)(x + k * 60);
                break;
            default:
                x = (int16_t)(x + k * 30);
                break;
            }
            e = mg_spawn_enemy(w->type, x, y, 0);
            if (!e) break;
            e->form = w->form;
            e->slot_i = k;
            e->base_y = (w->form == MG_FORM_DIVE) ? w->y : y;
            e->mood = 0;
            e->move_timer = 0;
            ng_physics_set_gravity(e->body, 0, 8 * NG_FP_ONE);
        }
    }
}

/* What comes into reach along the road: waves, posted throwers, pickups,
 * secrets, villagers and captives. */
static void NEOGEO_USER mg_spawn_scan(const MGLevel *level, int16_t px)
{
    uint8_t i;

    mg_wave_scan();

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
            mg.encounter_mask |= bit;
        } else if (mg_spawn_enemy(en->type, en->x, mg_ledge_y_at(level, en->x), 0)) {
            /* (a road already full keeps it waiting for a place, until she
             * is well past it: it used to be lost) */
            mg.encounter_mask |= bit;
        }
    }

    /* Posted on the ledges: whichever creature the stage file names. */
    for (i = 0; i < MG_ARCHER_COUNT; i++) {
        const MGArcher *a = &level->archers[i];
        uint16_t bit = (uint16_t)(1u << i);
        if (!a->x || (mg.archer_mask & bit) || a->x > px + 300 || a->x + 300 < px) continue;
        if (mg_stage_posted[mg.stage] == 0xFFu) { mg.archer_mask |= bit; continue; }
        if (mg_spawn_enemy(mg_stage_posted[mg.stage], a->x, a->y, 1)) mg.archer_mask |= bit;
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
}

static void NEOGEO_USER mg_spawn(void)
{
    const MGLevel *level = &mg_levels[mg.stage];
    int16_t px = mg.player->x;

    if (mg.boss_active) return;

    /* The road ahead is looked over every other frame: nothing there needs
     * the frame it comes into reach, and the scan is one of a frame's
     * bigger costs. */
    if ((mg.tick & 1u) == 0u) mg_spawn_scan(level, px);

    /* The guardian only shows itself once the gate is open. Walking into
     * the open gate carries her to its lair (mg_warp_begin); a valley with
     * no gate opens the arena at the end of the road instead. */
    if (!mg.boss_active && mg.gate_unlocked && mg.state == MG_PLAY) {
        if (level->gate_x) {
            if (px >= (int16_t)level->gate_x + 8) mg_warp_begin();
        } else if ((mg.flying ? mg.fly_x >= mg.arena_left : px > mg.arena_left + 32) && mg_boss_arrive()) {
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
    /* It never falls in: a break in the road or a hazard just ahead is
     * taken at a run and jumped. */
    {
        const MGLevel *lv = &mg_levels[mg.stage];
        uint8_t danger = (uint8_t)(mg_over_pit((int16_t)(p->x + 30), 0) || mg_over_pit((int16_t)(p->x + 60), 0));
        for (i = 0; i < MG_HAZARD_COUNT && !danger; i++) {
            const MGHazard *hz = &lv->hazards[i];
            if (hz->type && hz->type != MG_H_PIT && p->x + 64 > hz->x && p->x < hz->x) danger = 1;
        }
        if (danger) {
            /* A held through the jump (letting go would cut it short); on
             * the ground with it still held from the last, let go a frame
             * so the next press counts */
            uint8_t footing = (uint8_t)(ng_physics_is_grounded(p) || mg.on_ledge || p->y >= MG_GROUND_Y - 4);
            joy = (uint16_t)((joy & ~(JOY_UP | JOY_LEFT | BUTTON_A)) | JOY_RIGHT | BUTTON_B);
            if (!(footing && p->vy_fp >= 0 && (mg.previous_joy & BUTTON_A))) joy |= BUTTON_A;
        } else if (mg_over_pit((int16_t)(p->x + 110), 0)) {
            joy |= BUTTON_B;                     /* getting up to a run first */
        }
    }
    return joy;
}

static uint16_t NEOGEO_USER mg_input(void)
{
    return mg.demo ? mg_demo_joystick() : poll_joystick();
}

static void NEOGEO_USER mg_enemy_defeat(MGEnemy *e);

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

/*
 * On the sun eagle's back. The stick steers her eight ways through the sky
 * (ng_move: a steady push settles at about two pixels a frame, and let go
 * she keeps pace with the sky -- its current is the scroll); A beats the
 * wings for a quick climb; B throws a thorn ahead; C is a swoop, a burst
 * forward she can't be touched in, knocking down what she meets; D is the
 * Secret Art. Coming down on a creature from above, the talons strike it
 * (mg_enemy_contact, as a stomp).
 */
/* (`top` keeps her head just under the HUD.) */
static const NGMoveParams mg_fly_sky = { 64, 3, 3 * NG_FP_ONE, 0, NG_FP_ONE, 0, 88 };
static const NGMoveParams mg_fly_still = { 64, 3, 3 * NG_FP_ONE, 0, 0, 0, 88 };   /* the guardian's sky */

static void NEOGEO_USER mg_fly_controls(NGCharacter *p, uint16_t joy, uint16_t pressed)
{
    int8_t dx = (int8_t)((joy & JOY_RIGHT) ? 1 : ((joy & JOY_LEFT) ? -1 : 0));
    int8_t dy = (int8_t)((joy & JOY_DOWN) ? 1 : ((joy & JOY_UP) ? -1 : 0));

    mg.facing = 0;
    if (mg.dash) {
        p->vx_fp = (int16_t)((mg.boss_active ? 0 : NG_FP_ONE) + 4 * NG_FP_ONE);
        p->vy_fp = 0;
    } else {
        ng_move_steer(p, dx, dy, mg.boss_active ? &mg_fly_still : &mg_fly_sky);
    }
    if (pressed & BUTTON_A) {
        p->vy_fp = -3 * NG_FP_ONE;           /* a wing beat */
        mg.spin = 16;                        /* ...the eagle's wings go quicker */
        playSFX(SOUND_SFX_15);
    }
    if (pressed & BUTTON_B) mg_throw(-52);   /* from her hands, kneeling on its back */
    if ((pressed & BUTTON_C) && !mg.dash_wait) {
        mg.dash = 14;
        mg.dash_wait = 40;
        mg_light(MG_LIGHT_TRAIL, 14);
        playSFX(SOUND_SFX_15);
    }
    if (pressed & BUTTON_D) mg_secret_art();
}

static void NEOGEO_USER mg_controls(void)
{
    uint16_t joy = mg_input();
    uint16_t pressed = (uint16_t)((joy & (uint16_t)(~mg.previous_joy)) | mg.held_press);
    NGCharacter *p = mg.player;
    int16_t vx = 0;

    mg.held_press = 0;

    /* The reef's road is swum; its guardian is fought standing on the floor. */
    {
        uint8_t swim = (uint8_t)(mg_mech() == MG_M_WATER && mg.state == MG_PLAY &&
                                 !mg.boss_active && !mg.climbing);
        if (swim != mg.swimming) {
            mg.swimming = swim;
            mg_player_gravity();
        }
    }

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

    if (mg.flying) {
        mg_fly_controls(p, joy, pressed);
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
            if ((pressed & BUTTON_B) && mg.state != MG_BONUS) mg_throw(-26);
        }
        mg.previous_joy = joy;
        return;
    }

    {
        const MGVine *v = mg_vine_at(p->x, p->y);
        uint8_t can_climb = v && (((joy & JOY_UP) && p->y > v->top) ||
                                  ((joy & JOY_DOWN) && p->y < v->bottom));
        if (!mg.climb_cooldown && mg.state == MG_PLAY && !mg.boss_active && can_climb && !mg.swimming) {
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

    if (mg.swimming) {
        /*
         * Swimming: the stick steers her any of eight ways; A is a stroke,
         * a kick up and on in a spray of bubbles; the tide rocks her gently
         * back and forth. Down takes her through a ledge instead of onto it.
         */
        NGMoveParams swim = mg_swim;
        int8_t sx = (int8_t)((joy & JOY_RIGHT) ? 1 : ((joy & JOY_LEFT) ? -1 : 0));
        int8_t sy = (int8_t)((joy & JOY_DOWN) ? 1 : ((joy & JOY_UP) ? -1 : 0));
        uint8_t flags;

        if (sx) mg.facing = (uint8_t)(sx < 0);
        if (sy > 0) mg.drop = 4;
        swim.current_x = ng_trig_mul(72, ng_sin((uint8_t)(mg.tick >> 1)));
        if (pressed & BUTTON_A) {
            p->vy_fp = -3 * NG_FP_ONE;
            p->vx_fp += mg.facing ? -NG_FP_ONE : NG_FP_ONE;
            mg_burst(p->x, (int16_t)(p->y - 24), MG_T_BUBBLE, 2, -2);
            playSFX(SOUND_SFX_15);
        }
        flags = ng_move_steer(p, sx, sy, &swim);
        if (flags & NG_MOVE_SURFACED) {
            /* Breaking the surface: a spray, and back down. */
            mg_burst(p->x, (int16_t)(p->y - 50), MG_T_BUBBLE, 3, -3);
            playSFX(SOUND_SFX_5);
        }
        vx = (int16_t)p->vx_fp;
    } else
    /*
     * Walk and run.  She leans into a step and coasts out of it instead of
     * snapping between nought and full speed, which is what made her look
     * like a sprite being dragged rather than a girl running.
     */
    {
        /* B held: she runs. */
        int16_t speed = (int16_t)((joy & BUTTON_B) ? RUN_SPEED : WALK_SPEED);
        int16_t want = 0;
        int16_t have = (int16_t)p->vx_fp;
        /* On ice she's slow to get going and slower to stop. */
        uint8_t slick = (uint8_t)(mg_mech() == MG_M_ICE && !mg.on_ledge && !mg.airborne);
        int16_t accel = (int16_t)(slick ? 36 : WALK_ACCEL);
        if (mg.swift) speed = (int16_t)((speed * 5) / 4);
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
    if ((joy & JOY_DOWN) && !vx && !mg.swimming && (ng_physics_is_grounded(p) || mg.on_ledge ||
                                    p->y >= MG_GROUND_Y - 4)) {
        if (mg.crouch_timer < 255) mg.crouch_timer++;
        vx = 0;
    } else if (mg.crouch_timer) {
        mg.crouch_timer = 0;
    }
    mg.sitting = mg.crouch_timer > SIT_DELAY;
    if (mg.crouch_timer >= MG_LEAP_KNEEL) mg.leap_window = MG_LEAP_WINDOW;
    else if (mg.leap_window) mg.leap_window--;

    /*
     * Jump & drop through ledges (swimming, A is the stroke). A jump
     * pressed a few frames before she lands is kept and taken as she
     * lands, and one pressed a few frames after she runs off a ledge still
     * counts: the jump happens when the player meant it, not only on the
     * exact frame the ground allows it. Running (B held, and up to speed)
     * she jumps higher. Kneeling a moment, then Up and A, she leaps
     * straight up, higher still. In the air, with a sky lily in hand, Up
     * and A again is one more, smaller jump (a plain one, and never a
     * third).
     */
    if (!mg.swimming) {
        uint8_t footing = (uint8_t)(ng_physics_is_grounded(p) || mg.on_ledge || p->y >= MG_GROUND_Y - 4);
        if (footing && p->vy_fp >= 0) {
            mg.coyote = MG_JUMP_GRACE;
            mg.air_jump = 1;
            mg.air_dash = 1;
            mg.stomp_chain = 0;
            mg.jump_cut = 0;
        } else if (mg.coyote) {
            mg.coyote--;
        }
        if (pressed & BUTTON_A) mg.jump_buffer = MG_JUMP_GRACE;
        else if (mg.jump_buffer) mg.jump_buffer--;

        if (mg.jump_buffer) {
            if ((joy & JOY_DOWN) && mg.on_ledge) {
                mg.drop = 12;
                mg.jump_buffer = 0;
            } else if (mg.coyote) {
                int16_t v = (int16_t)(((joy & BUTTON_B) && mg_abs(vx) > WALK_SPEED + 64) ? RUN_JUMP_SPEED : JUMP_SPEED);
                if ((joy & JOY_UP) && mg.leap_window) {
                    /* the high leap: straight up out of the kneel */
                    v = LEAP_SPEED;
                    vx = 0;
                    mg.leap_window = 0;
                    mg_burst(p->x, (int16_t)(p->y - 4), MG_T_SPARK, 3, -2);
                    playSFX(SOUND_SFX_13);
                    mg_voice(MG_VOICE_LEAP);
                    mg.leaping = 1;
                    mg_light(MG_LIGHT_BURST, 16);
                }
                if (mg.spring) v = (int16_t)((v * 5) / 4);
                if (mg_mech() == MG_M_WATER) v = (int16_t)((v * 3) / 4);
                p->vy_fp = -v;
                mg.airborne = 1;
                mg.coyote = 0;
                mg.jump_buffer = 0;
                mg.jump_cut = 1;
                playSFX(SOUND_SFX_15);
            } else if ((pressed & BUTTON_A) && (joy & JOY_UP) && mg.air_jump && mg.lily) {
                p->vy_fp = -AIR_JUMP_SPEED;
                mg.jump_cut = 1;
                mg.air_jump = 0;
                mg.jump_buffer = 0;
                mg_burst(p->x, (int16_t)(p->y - 4), mg_her_tile(MG_T_PETAL), 2, 1);
                playSFX(SOUND_SFX_15);
            }
        }
    }
    /* Let go of A on the way up and the jump is cut short: a tap hops, a
     * hold clears the shelf.  This is most of what makes a jump feel meant. */
    if (!(joy & BUTTON_A) && mg.jump_cut && !mg.swimming && p->vy_fp < -(2 * NG_FP_ONE)) {
        p->vy_fp = -(2 * NG_FP_ONE);
    }

    /*
     * The Rising Bloom, her one strike up close: forward, down, down-
     * forward and B. She gathers for three frames, then springs up in a
     * spin of petals; on the way up it knocks out whatever is in front of
     * her and overhead -- even the poison frog and the vine sting, which
     * can't be stomped -- and strikes a guardian once. It rises only so
     * high, she is open as she comes down, and it needs a moment before
     * the next.
     */
    if (mg.rise_wait) mg.rise_wait--;
    if (mg.dp_timer) mg.dp_timer--;
    else mg.dp_step = 0;
    if (!mg.swimming) {
        uint16_t fwd = (uint16_t)(mg.dp_dir < 0 ? JOY_LEFT : JOY_RIGHT);
        if (pressed & (JOY_LEFT | JOY_RIGHT) && (mg.dp_step == 0 || mg.dp_step == 1)) {
            if (mg.dp_step == 0 || !(pressed & fwd)) {
                mg.dp_dir = (int8_t)((pressed & JOY_LEFT) ? -1 : 1);
                mg.dp_step = 1;
                mg.dp_timer = 18;
            }
        } else if (mg.dp_step == 1 && (pressed & JOY_DOWN)) {
            mg.dp_step = 2;
            mg.dp_timer = 18;
        } else if (mg.dp_step == 2 && (joy & fwd)) {
            mg.dp_step = 3;
            mg.dp_timer = 12;
        }
        if (mg.dp_step == 3 && (pressed & BUTTON_B) && !mg.rising && !mg.rise_wait && mg.state != MG_BONUS) {
            mg.rising = MG_RISE_TIME;
            mg.rise_hit = 0;
            mg.facing = (uint8_t)(mg.dp_dir < 0);
            mg.dp_step = 0;
            mg.crouch_timer = 0;
            mg.dash = 0;
            playSFX(SOUND_SFX_1);
            mg_voice(MG_VOICE_RISE);
            mg_light(MG_LIGHT_WHIRL, MG_RISE_TIME);
        }
    }

    /*
     * B: her thorn, or the whip up close (and held, she runs). Kneeling,
     * the throw skims the road. Down, forward + B is the Rose Blossom
     * Surge. Landing on a creature beats it too (mg_enemy_contact).
     */
    /* (The bonus round is dodged, not fought: her weapons rest there.) */
    if ((pressed & BUTTON_B) && !mg.rising && !mg.super_surge && mg.state != MG_BONUS) {
        if (mg.combo_buffer[0] && mg.combo_buffer[1] && !mg.dash_wait) {
            mg.super_surge = MG_SURGE_TIME;
            mg.surge_struck_boss = 0;
            mg.combo_buffer[0] = mg.combo_buffer[1] = 0;
            mg.dash = 0;
            playSFX(SOUND_SFX_13);   /* the bloom gathers */
            mg_voice(MG_VOICE_SURGE);
            mg_light(MG_LIGHT_TRAIL, MG_SURGE_TIME);
        } else {
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
            if (melee) {
                mg.attack = 12;
                playSFX(SOUND_SFX_1); /* whip crack */
            } else {
                mg_throw((int16_t)(mg.crouch_timer ? -8 : -26));
            }
        }
    }

    /* Secret Art (D button) */
    if ((pressed & BUTTON_D) && mg.state != MG_BONUS) {
        mg_secret_art();
    }

    if (mg.rising) {
        int8_t dir = (int8_t)(mg.facing ? -1 : 1);
        mg.rising--;
        vx = (int16_t)(dir * 320);
        if (mg.rising > MG_RISE_TIME - 3) {
            vx = 0;
            p->vy_fp = 0;
        } else if (mg.rising == MG_RISE_TIME - 3) {
            p->vy_fp = -RISE_SPEED;
            mg.jump_cut = 0;
            mg.airborne = 1;
            mg.coyote = 0;
            mg_burst(p->x, (int16_t)(p->y - 10), mg_her_tile(MG_T_PETAL), 3, -3);
            playSFX(SOUND_SFX_15);
        }
        if (mg.rising <= MG_RISE_TIME - 3 && mg.rising > MG_RISE_TIME - 16) {
            int16_t hx = (int16_t)(p->x + dir * 14);
            uint8_t i;
            if ((mg.rising & 3u) == 0u) mg_burst(hx, (int16_t)(p->y - 50), mg_her_tile(MG_T_PETAL), 1, -1);
            for (i = 0; i < MG_ENEMIES; i++) {
                MGEnemy *e = &mg.enemies[i];
                NGCharacter *b = e->body;
                if (!b || e->mood == MG_MOOD_DYING || e->type == MG_E_WRAITH) continue;
                if (mg_abs((int16_t)(b->x - hx)) >= 28 || b->y < p->y - 84 || b->y > p->y + 6) continue;
                if (mg_stomp_rule(e->type) == MG_STOMP_TOUGH && b->hp > 1) {
                    if (!e->hurt) { b->hp--; e->hurt = 30; e->mood = 3; e->move_timer = 40; playSFX(SOUND_SFX_4); }
                } else {
                    mg_enemy_defeat(e);
                    mg.score += 200u;
                }
            }
            if (!mg.rise_hit && mg.boss && mg.boss_active &&
                mg_abs((int16_t)(mg.boss->x - hx)) < 40 && mg.boss->y > p->y - 90 && mg.boss->y < p->y + 90) {
                mg.rise_hit = 1;
                mg_boss_damage(MG_STOMP_BLOW);
                mg.boss_hurt = 60;
            }
        }
        if (!mg.rising) mg.rise_wait = MG_RISE_WAIT;
    }

    /*
     * The dash: a kick of dust where she pushes off, a trail of it behind
     * her, and the last few frames easing back towards a run instead of
     * snapping to a stop. Jumping out of it keeps the speed.
     */
    if ((pressed & BUTTON_C) && !mg.dash_wait && !mg.super_surge && !mg.rising &&
        (ng_physics_is_grounded(p) || mg.on_ledge || mg.air_dash)) {
        if (!(ng_physics_is_grounded(p) || mg.on_ledge)) {
            mg.air_dash = 0;                  /* one in the air, held level */
            if (p->vy_fp > 0) p->vy_fp = 0;
        }
        mg.dash = MG_DASH_TIME;
        mg.dash_wait = 24;
        mg_burst((int16_t)(p->x + (mg.facing ? 10 : -10)), (int16_t)(p->y - 4), MG_T_DUST, 2, -1);
        playSFX(SOUND_SFX_15);
    }
    if (mg.dash) {
        int16_t speed = (int16_t)(mg.dash > 3 ? DASH_SPEED
                                  : WALK_SPEED + ((DASH_SPEED - WALK_SPEED) * mg.dash) / 4);
        if (mg.airborne && p->vy_fp > 0) p->vy_fp = 0;   /* an air dash runs level */
        vx = mg.facing ? (int16_t)-speed : speed;
        if ((mg.dash % 4) == 0 && !mg.airborne)
            mg_burst((int16_t)(p->x + (mg.facing ? 12 : -12)), (int16_t)(p->y - 18),
                     mg_her_tile((uint8_t)((mg.dash & 4) ? MG_T_PETAL : MG_T_LEAF)), 1, -1);
    }

    /*
     * The Rose Blossom Surge. A beat of stillness while the bloom gathers
     * around her, then she spins a short way along the road trailing
     * petals: every creature in her path is struck, the guardian once, and
     * she can't be touched until it's over. On the road she stops at a
     * pit's edge and finishes the spin there.
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
                mg_burst(p->x, (int16_t)(p->y - 30), mg_her_tile(MG_T_PETAL), 2, -2);
            }
        } else {
            uint8_t i;
            int8_t dir = (int8_t)(mg.facing ? -1 : 1);
            int16_t speed = (int16_t)(mg.super_surge > 3 ? MG_SURGE_SPEED : DASH_SPEED / 2);
            if (!mg.airborne && mg_over_pit((int16_t)(p->x + dir * 20), 0)) speed = 0;
            vx = (int16_t)(dir * speed);
            if ((mg.super_surge % 3) == 0)
                mg_burst((int16_t)(p->x + (mg.facing ? 14 : -14)), (int16_t)(p->y - 24),
                         mg_her_tile((uint8_t)((mg.super_surge & 4) ? MG_T_PETAL : MG_T_LEAF)), 1, -1);
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
    mg.arena_home = mg.arena[0].x;
    mg.rain_timer = 150;
    mg.rain_x = 0;
}

static void NEOGEO_USER mg_stomp_bounce(NGCharacter *p);

/*
 * Each arena is its own place to fight in. The toad's, the leviathan's,
 * the eel's and the wyrm's first ledge slides back and forth (carrying her
 * if she stands on it); and every arena drops its own hazard from above --
 * sawdust, sludge, oil, embers, icicles, rocks -- a puff at the top giving
 * half a second's warning of where. It comes quicker once the guardian is
 * enraged. The cover ledges still shelter her from it.
 */
static const uint8_t mg_arena_rain[10] = {
    MG_T_DUST, MG_T_DRIP, MG_T_OIL, MG_T_FIRE, MG_T_ICE,
    MG_T_SPIT, MG_T_BOLT, MG_T_OIL, MG_T_TRASH, MG_T_FIRE,
};

static void NEOGEO_USER mg_arena_step(NGCharacter *p, uint8_t style)
{
    if (style > 9) return;
    if (style == MG_B_TOAD || style == MG_B_LEVIATHAN || style == MG_B_EEL || style == MG_B_WYRM) {
        int16_t x = (int16_t)(mg.arena_home + ng_trig_mul(44, ng_sin((uint8_t)(mg.tick >> 1))));
        int16_t dx = (int16_t)(x - mg.arena[0].x);
        if (dx && mg.on_ledge && mg.ledge_index == 0)
            ng_char_set_pos(p, (int16_t)(p->x + dx), p->y);   /* she rides along */
        mg.arena[0].x = x;
    }
    if (mg.rain_timer) {
        mg.rain_timer--;
        return;
    }
    if (!mg.rain_x) {
        /* The warning: where it will fall, near where she is. */
        int16_t x = (int16_t)(p->x - 40 + (int16_t)ng_rand_range(80));
        if (x < mg.arena_left + 24) x = (int16_t)(mg.arena_left + 24);
        if (x > mg.arena_left + 296) x = (int16_t)(mg.arena_left + 296);
        mg.rain_x = x;
        mg.rain_timer = 30;
        mg_burst(x, 44, MG_T_DUST, 2, 0);
    } else {
        MGShot *s = mg_fire(mg.rain_x, 44, 0, 1, 1, mg_arena_rain[style]);
        if (s) { s->mode = MG_SHOT_ARC; s->life = 120; }
        mg.rain_x = 0;
        {
            /* quicker when enraged, further on, and in Smoggar's later phases */
            int16_t wait = (int16_t)((mg.boss_rage ? 60 : 110) - mg.stage * 3 - mg.boss_phase * 10);
            mg.rain_timer = (uint8_t)(wait < 18 ? 18 : wait);
        }
    }
}

/*
 * Landing on its head is the only way to hurt it -- when it's open: not
 * while it winds up or strikes, when it is all edge and teeth, nor once
 * enraged while it makes its special move; always in its recovery beats,
 * and while it pants, near the end.
 * A landing that isn't welcome hurts her; one while it is still shaking
 * off the last just bounces her off. A stomp that lands throws her high
 * and clear, and it shakes itself free for a moment. Returns 1 if she
 * landed on it this frame.
 */
static uint8_t NEOGEO_USER mg_boss_stomp(NGCharacter *p, NGCharacter *b, uint8_t open)
{
    int16_t top = (int16_t)(b->y + b->body_y);
    if (p->vy_fp <= 0 || mg.player_prev_y > top + 8 || p->y < top - 2 ||
        mg_abs((int16_t)(p->x - b->x)) >= (int16_t)(b->body_w / 2 + 8)) return 0;
    if (mg.boss_hurt) {
        p->vy_fp = -STOMP_KICK;
        p->vx_fp = (p->x < b->x) ? -700 : 700;
    } else if (!open) {
        mg_player_hit(b->x);
        p->vy_fp = -STOMP_KICK;
    } else {
        mg_boss_damage(MG_STOMP_BLOW);
        mg.boss_hurt = 50;
        mg_stomp_bounce(p);
        p->vx_fp = (p->x < b->x) ? -900 : 900;
        mg.boss_backoff = 30;
    }
    return 1;
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
/*
 * Coming in from the distance, small and high, growing as it nears
 * (ng_shrink_tab, the depth effect's scale), until it takes station ahead
 * of her. Nothing hurts it, or her, on the way.
 */
static void NEOGEO_USER mg_ship_approach(NGCharacter *b)
{
    uint8_t s;
    if (mg.ship_z) mg.ship_z--;
    s = ng_shrink_tab[mg.ship_z];
    b->scale_x = s;
    b->scale_y = s;
    b->sprite_offset_x = (int16_t)-(((uint16_t)128u * s) >> 8);
    b->sprite_offset_y = (int16_t)-(((uint16_t)96u * s) >> 8);
    b->sprite_dirty = 1;
    b->vx_fp = 0;
    b->vy_fp = 0;
    ng_char_set_pos(b, (int16_t)(mg.arena_left + 220 + mg.ship_z), (int16_t)(140 - mg.ship_z / 2));
    mg_frame(b, 0, 0);
}

/*
 * The dreadnought's fight: it rides a slow swell ahead of her, lobs shells
 * from its gondola's ports at her, puffs smog from its stacks that drifts
 * back at her, and now and then looses a cloud of gnats. With its stacks
 * gone it comes lower and quicker, firing faster. Landing on a stack (or,
 * bared, the bridge) with the talons is a blow; on the deck she only
 * bounces off; flying into the hull hurts.
 */
static void NEOGEO_USER mg_stomp_bounce(NGCharacter *p);

static void NEOGEO_USER mg_ship_ai(NGCharacter *b, NGCharacter *p)
{
    uint8_t down = (uint8_t)(!mg.ship_part[MG_SHIP_STACK_L] + !mg.ship_part[MG_SHIP_STACK_R]);
    uint8_t rage = (uint8_t)(down == 2);
    uint16_t t;
    uint8_t i;

    if (mg.boss_hurt) mg.boss_hurt--;
    if (mg.boss_backoff) mg.boss_backoff--;
    if (mg.ship_z) { mg_ship_approach(b); return; }
    t = ++mg.boss_timer;

    mg_boss_hover(b, (int16_t)(mg.arena_left + 220 + ng_trig_mul(rage ? 40 : 24, ng_sin((uint8_t)(t >> 1)))),
                  (int16_t)(rage ? 150 + ng_trig_mul(30, ng_sin((uint8_t)t)) : 140), 160);
    mg_frame(b, down, 0);

    /* shells from the gondola: one at her, two either side */
    if ((t % mg_rate(rage ? 70 : 110)) == 0u) {
        mg_lob_at(b, p, -18, MG_T_OIL);
        mg_lob((int16_t)(b->x - 30), (int16_t)(b->y - 18), -3, -4, MG_T_OIL);
        mg_lob((int16_t)(b->x + 10), (int16_t)(b->y - 18), -1, -5, MG_T_OIL);
        playSFX(SOUND_SFX_8);
    }
    /* smog from the stacks, drifting back at her */
    if ((t % mg_rate(160)) == 80u) {
        for (i = 0; i < 2; i++) {
            if (!mg.ship_part[i]) continue;
            mg_fire((int16_t)(b->x + mg_ship_box[i][0]), (int16_t)(b->y + mg_ship_box[i][1] - 14), -2, 0, 1, MG_T_DUST);
        }
    }
    /* a cloud of gnats */
    if ((t % 360u) == 200u) {
        for (i = 0; i < 3u; i++) {
            MGEnemy *e = mg_spawn_enemy(MG_E_GNAT, (int16_t)(b->x - 70), (int16_t)(b->y - 50 + i * 16), 0);
            if (!e) break;
            e->form = MG_FORM_SWARM;
            e->slot_i = i;
            e->age = 50;
            ng_physics_set_gravity(e->body, 0, 8 * NG_FP_ONE);
        }
    }

    /* the talons coming down on it */
    if (p->vy_fp > 0 && mg_abs((int16_t)(p->x - b->x)) < 124) {
        uint8_t part = mg_ship_part_at(p->x, (int16_t)(p->y + 4));
        if (part != 0xFFu) {
            const int8_t *k = mg_ship_box[part];
            int16_t top = (int16_t)(b->y + k[1] - k[3]);
            if (mg.player_prev_y <= top + 8 && p->y >= top - 2) {
                if (mg_ship_open(part) && !mg.boss_hurt) {
                    mg.ship_target = part;
                    mg_boss_damage(MG_STOMP_BLOW);
                    mg.boss_hurt = 40;
                }
                mg_stomp_bounce(p);
                return;
            }
        }
        if (mg.player_prev_y <= b->y - 72 && p->y >= b->y - 76 && p->y < b->y - 60) {
            p->vy_fp = -STOMP_KICK;          /* the deck: she only bounces off */
            return;
        }
    }
    /* the swoop into an open part */
    if (mg.dash && !mg.boss_hurt) {
        uint8_t part = mg_ship_part_at(p->x, (int16_t)(p->y - 30));
        if (part != 0xFFu && mg_ship_open(part)) {
            mg.ship_target = part;
            mg_boss_damage(MG_STOMP_BLOW);
            mg.boss_hurt = 30;
        }
        return;
    }
    /* flying into the hull */
    if (!mg.boss_backoff && mg_ship_in_hull(p->x, (int16_t)(p->y - 20))) {
        mg_player_hit(b->x);
        if (mg.state == MG_PLAY && mg.hurt == 90) mg.boss_backoff = MG_BOSS_BACKOFF;
    }
}

/*
 * Lord Smoggar at the end of the road fights in three phases, by his
 * health: at two thirds the smog rises -- two smog wraiths come for her
 * and the arena's fall comes quicker; at a third he is the smog itself --
 * enraged from then on, with smog bats diving out of the haze.
 */
static void NEOGEO_USER mg_smoggar_phase(NGCharacter *b)
{
    uint16_t third = (uint16_t)(b->hp * 3u);
    uint8_t want = (uint8_t)(third <= b->max_hp ? 2 : (third <= (uint16_t)(b->max_hp * 2u) ? 1 : 0));
    uint8_t i;

    if (want <= mg.boss_phase) return;
    mg.boss_phase = want;
    mg.shake = 16;
    playSFX(SOUND_SFX_14);
    mg_burst(b->x, (int16_t)(b->y - 60), MG_T_DUST, 4, -2);
    if (want == 1) {
        mg_hint("SMOGGAR: THE SMOG RISES!", PAL_WARN, 150);
        for (i = 0; i < 2u; i++)
            mg_spawn_enemy(MG_E_WRAITH, (int16_t)(mg.arena_left + (i ? 300 : 20)), (int16_t)(MG_GROUND_Y - 90), 0);
    } else {
        mg_hint("SMOGGAR: I AM THE SMOG ITSELF!", PAL_WARN, 150);
        mg.boss_rage = 1;
        for (i = 0; i < 2u; i++)
            mg_spawn_enemy(MG_E_SMOGBAT, (int16_t)(mg.arena_left + 60 + i * 200), 40, 0);
    }
}

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

    if (style == MG_B_AIRSHIP) { mg_ship_ai(b, p); return; }
    if (style == MG_B_SMOGGAR && mg.stage + 1u == MG_LEVEL_COUNT && mg.rush_i == 0xFFu) mg_smoggar_phase(b);
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
            mg_arena_step(p, style);
            mg_boss_stomp(p, b, 1);     /* panting: the opening to finish it */
            return;
        }
    }
    mg.boss_timer = (uint16_t)(mg.boss_timer + (rage == 2 ? (mg.tick & 1) : 1 + (rage && (mg.tick & 1))));
    mg_arena_step(p, style);

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
            if ((uint16_t)(t % (uint16_t)(rage ? 40u : 60u)) == 30u) {
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
            if ((uint16_t)(t % (uint16_t)(rage ? 32u : 50u)) == 20u && mg_abs(b_dx) < 60) {
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

    /* Landing on its head: see mg_boss_stomp. */
    if (mg_boss_stomp(p, b, (uint8_t)(harmless || (frame != MG_BF_ATTACK && frame != MG_BF_WINDUP &&
                                                  !(rage && frame == MG_BF_SPECIAL))))) return;

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
/* The mist veil's colours: hers, each lifted halfway to a pale sky blue. */
static void NEOGEO_USER mg_veil_palette(void)
{
    const uint16_t *src = mg_hero_normal_pal();
    uint16_t pal[16];
    uint8_t i;
    pal[0] = src[0];
    for (i = 1; i < 16; i++) {
        uint16_t c = src[i];
        uint8_t r = (uint8_t)((c >> 8) & 15u), g = (uint8_t)((c >> 4) & 15u), b = (uint8_t)(c & 15u);
        r = (uint8_t)((r + 11u) >> 1); g = (uint8_t)((g + 14u) >> 1); b = (uint8_t)((b + 15u) >> 1);
        pal[i] = (uint16_t)(((uint16_t)r << 8) | ((uint16_t)g << 4) | b);
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

/* The bounce off a creature she landed on: holding A, a high one. A
 * bounce gives back her second jump and her dash in the air, and every
 * creature landed on before she touches ground is worth twice the last. */
static void NEOGEO_USER mg_stomp_bounce(NGCharacter *p)
{
    p->vy_fp = (mg.previous_joy & BUTTON_A) ? -STOMP_HIGH : -STOMP_KICK;
    mg.jump_cut = 0;
    mg.air_jump = 1;
    mg.air_dash = 1;
    mg.airborne = 1;
    mg.coyote = 0;
    if (mg.stomp_chain < 7) mg.stomp_chain++;
    mg.score += (uint32_t)100u << mg.stomp_chain;
    mg.hud_dirty = 1;
}

/* Beat a creature outright (a stomp, a sliding shell): its hurt timer
 * would otherwise let the blow pass unnoticed. */
static void NEOGEO_USER mg_enemy_defeat(MGEnemy *e)
{
    e->hurt = 0;
    if (e->body) e->body->hp = 1;
    mg_enemy_damage(e, 99);
}

/* Dazed, stars round its head, until it shakes itself out of it -- sooner
 * in the later valleys, trembling first; and once kicked, sliding along the
 * road, bowling over every creature it meets, whatever its skin, and gone
 * off the screen. */
static void NEOGEO_USER mg_shell_step(MGEnemy *e)
{
    NGCharacter *b = e->body;
    if (e->mood == MG_MOOD_STUNNED) {
        b->vx_fp = 0;
        if (e->move_timer) e->move_timer--;
        if ((e->move_timer & 31u) == 16u) mg_burst(b->x, (int16_t)(b->y - 30), MG_T_STAR, 2, -1);
        if (e->move_timer < 50 && (e->timer & 2))
            ng_char_set_pos(b, (int16_t)(b->x + ((e->timer & 4) ? 1 : -1)), b->y);
        if (!e->move_timer) e->mood = 1;
        mg_frame(b, 0, (uint8_t)(e->face < 0));
    } else {
        uint8_t i;
        b->vx_fp = e->heading * 1280;
        mg_frame(b, (uint8_t)((e->timer >> 2) & 1u), (uint8_t)(e->heading < 0));
        for (i = 0; i < MG_ENEMIES; i++) {
            MGEnemy *o = &mg.enemies[i];
            if (o == e || !o->body || o->mood == MG_MOOD_DYING) continue;
            if (mg_abs((int16_t)(o->body->x - b->x)) < 22 && mg_abs((int16_t)(o->body->y - b->y)) < 30) {
                mg_enemy_defeat(o);
                if (mg.stomp_chain < 7) mg.stomp_chain++;
                mg.score += (uint32_t)100u << mg.stomp_chain;
            }
        }
        if (b->x < mg.camera.x - 80 || b->x > mg.camera.x + 400) {
            ng_chars_remove(b);
            e->body = 0;
        }
    }
}

/*
 * She meets a creature. Coming down onto it from above is a stomp (see
 * mg_stomp_rule); anything else is a touch: a shell on its back is kicked
 * away, a dash bumps the creature aside, and otherwise it hurts her.
 */
static void NEOGEO_USER mg_enemy_contact(MGEnemy *e, NGCharacter *p)
{
    NGCharacter *b = e->body;
    int16_t left = (int16_t)(b->x + b->body_x), top = (int16_t)(b->y + b->body_y);
    int16_t right = (int16_t)(left + b->body_w), bottom = (int16_t)(top + b->body_h);
    uint8_t over_x = (uint8_t)(p->x + 10 > left && p->x - 10 < right);
    uint8_t from_above = (uint8_t)(over_x && p->vy_fp > 0 && mg.player_prev_y <= top + 6 && p->y >= top - 2);
    uint8_t touch = (uint8_t)(over_x && p->y > top + 4 && p->y - 44 < bottom);

    if (e->mood == MG_MOOD_DYING || (!from_above && !touch)) return;
    if (from_above) {
        switch (mg_stomp_rule(e->type)) {
        case MG_STOMP_FLIP:
            if (e->mood == MG_MOOD_STUNNED) {
                mg_enemy_defeat(e);
            } else {
                /* Dazed -- or a sliding one, stopped where it is. */
                e->mood = MG_MOOD_STUNNED;
                e->move_timer = (uint8_t)(220u - mg.stage * 12u);
                b->vx_fp = 0;
                mg_burst(b->x, (int16_t)(b->y - 30), MG_T_STAR, 3, -2);
                playSFX(SOUND_SFX_3);
            }
            mg_stomp_bounce(p);
            break;
        case MG_STOMP_TOUGH:
            if (e->hurt) { mg_stomp_bounce(p); break; }
            if (b->hp <= 1) {
                mg_enemy_defeat(e);
            } else {
                b->hp--;
                e->hurt = 24;
                e->mood = 3;             /* staggered: stands still, then comes on */
                e->move_timer = 40;
                mg.shake = 4;
                playSFX(SOUND_SFX_4);
            }
            mg_stomp_bounce(p);
            break;
        case MG_STOMP_HURT:
            mg_player_hit(b->x);
            p->vy_fp = -STOMP_KICK;
            break;
        default:
            mg_enemy_defeat(e);
            playSFX(SOUND_SFX_3);
            mg_stomp_bounce(p);
            break;
        }
        return;
    }
    if (e->hurt) return;
    if (e->mood == MG_MOOD_STUNNED) {
        /* Kicked: it slides off away from her. */
        e->mood = MG_MOOD_SHELL;
        e->heading = (int8_t)(p->x < b->x ? 1 : -1);
        e->hurt = 12;
        mg.score += 100u;
        mg.hud_dirty = 1;
        playSFX(SOUND_SFX_4);
        return;
    }
    if (e->mood == MG_MOOD_SHELL) return;
    if (mg.dash && mg.flying) {
        /* The eagle's swoop: what it meets goes down (the armoured ones
         * take a blow each pass). */
        if (mg_stomp_rule(e->type) == MG_STOMP_TOUGH && b->hp > 1) {
            b->hp--;
            e->hurt = 24;
            playSFX(SOUND_SFX_4);
        } else {
            mg_enemy_defeat(e);
            mg.score += 200u;
        }
        return;
    }
    if (mg.dash && mg_stomp_rule(e->type) != MG_STOMP_HURT) {
        /* A dash bumps it aside, no harm to either. */
        b->vx_fp = p->x < b->x ? 1100 : -1100;
        e->hurt = 24;
        return;
    }
    mg_player_hit(b->x);
    /* Shove the creature off: being cornered by one slime was death.
     * (A touch in the bonus round clears the field, this one too.) */
    if (e->body) {
        e->body->vx_fp = p->x < e->body->x ? 900 : -900;
        e->hurt = 20;
    }
}

/*
 * One creature of a wave, flying its formation. Speeds are in the world,
 * so a creature that "hangs" keeps pace with the sky (`pace`, nothing once
 * the view stops for the guardian). It is gone once it has flown out of
 * the view.
 */
static void NEOGEO_USER mg_form_step(MGEnemy *e, NGCharacter *p)
{
    NGCharacter *b = e->body;
    int16_t cam = mg.camera.x, sx = (int16_t)(b->x - cam);
    int16_t pace = (int16_t)(mg.boss_active ? 0 : NG_FP_ONE);
    uint16_t age = ++e->age;
    uint8_t flip = 1;                              /* they come at her from the right */

    switch (e->form) {
    case MG_FORM_SINE:
        b->vx_fp = (int16_t)(pace - 3 * NG_FP_ONE);
        mg_wave(b, e->base_y, 34, 850, (uint8_t)(age * 4u + e->slot_i * 40u));
        break;
    case MG_FORM_DIVE:
        if (e->mood == 0) {
            /* drop in and hang a moment, then straight at where she is */
            b->vx_fp = pace;
            b->vy_fp = (int16_t)((e->base_y - b->y) * 12);
            if (age > 36u + e->slot_i * 14u) {
                uint8_t ang = ng_atan2((int16_t)((p->y - 30) - b->y), (int16_t)(p->x - b->x));
                e->mood = 1;
                b->vx_fp = (int16_t)(pace + ng_trig_mul(4 * NG_FP_ONE, ng_cos(ang)));
                b->vy_fp = ng_trig_mul(4 * NG_FP_ONE, ng_sin(ang));
            }
        }
        flip = (uint8_t)(b->vx_fp < pace);
        break;
    case MG_FORM_CIRCLE:
        if (e->mood == 0) {
            b->vx_fp = (int16_t)(pace - 3 * NG_FP_ONE);
            b->vy_fp = (int16_t)((e->base_y - b->y) * 12);
            if (sx < 230) { e->mood = 1; e->move_timer = 0; e->home = (int16_t)(sx - 56); }
        } else if (e->mood == 1) {
            /* a loop and a half round a point that keeps pace with the sky */
            uint8_t ang = (uint8_t)(e->move_timer * 3u);
            int16_t s = ng_sin(ang);
            b->vx_fp = 0;
            b->vy_fp = 0;
            ng_char_set_pos(b, (int16_t)(cam + e->home + ng_trig_mul(56, ng_cos(ang))),
                            (int16_t)(e->base_y - ng_trig_mul(40, s)));
            flip = (uint8_t)(s > 0);
            if (++e->move_timer >= 128) e->mood = 2;
        } else {
            b->vx_fp = (int16_t)(pace - 4 * NG_FP_ONE);
            b->vy_fp = 0;
        }
        break;
    case MG_FORM_SWARM:
        if (age < 50u) {
            b->vx_fp = (int16_t)(pace - 3 * NG_FP_ONE);
            b->vy_fp = (int16_t)((e->base_y - b->y) * 8);
        } else if (age < 330u) {
            /* closing in on her, each with its own jitter */
            int16_t rx = (int16_t)(b->vx_fp - pace + (p->x > b->x ? 20 : -20));
            int16_t vy = (int16_t)(b->vy_fp + ((p->y - 30) > b->y ? 20 : -20));
            if (rx > 3 * NG_FP_ONE / 2) rx = 3 * NG_FP_ONE / 2;
            if (rx < -3 * NG_FP_ONE / 2) rx = -3 * NG_FP_ONE / 2;
            if (vy > 3 * NG_FP_ONE / 2) vy = 3 * NG_FP_ONE / 2;
            if (vy < -3 * NG_FP_ONE / 2) vy = -3 * NG_FP_ONE / 2;
            b->vx_fp = (int16_t)(pace + rx);
            b->vy_fp = (int16_t)(vy + ng_trig_mul(80, ng_sin((uint8_t)(age * 9u + e->slot_i * 60u))));
        } else {
            b->vx_fp = (int16_t)(pace - 4 * NG_FP_ONE);
        }
        flip = (uint8_t)(b->vx_fp < pace);
        break;
    case MG_FORM_CHARGE:
        if (e->mood == 0) {
            b->vx_fp = (int16_t)(pace - 2 * NG_FP_ONE);
            b->vy_fp = (int16_t)((e->base_y - b->y) * 8);
            if (sx < 262) { e->mood = 1; e->move_timer = 50; }
        } else if (e->mood == 1) {
            /* it squares up to her height, shaking, then rams across */
            int16_t vy = (int16_t)(((p->y - 24) - b->y) * 8);
            if (vy > 2 * NG_FP_ONE) vy = 2 * NG_FP_ONE;
            if (vy < -2 * NG_FP_ONE) vy = -2 * NG_FP_ONE;
            b->vx_fp = pace;
            b->vy_fp = vy;
            if (e->move_timer & 2) ng_char_set_pos(b, (int16_t)(b->x + ((e->move_timer & 4) ? 1 : -1)), b->y);
            if (--e->move_timer == 0) { e->mood = 2; playSFX(SOUND_SFX_7); }
        } else {
            b->vx_fp = (int16_t)(pace - 5 * NG_FP_ONE);
            b->vy_fp = 0;
        }
        break;
    case MG_FORM_HOVER:
        if (e->mood == 0) {
            b->vx_fp = (int16_t)(pace - 2 * NG_FP_ONE);
            b->vy_fp = (int16_t)((e->base_y - b->y) * 8);
            if (sx < 236) e->mood = 1;
        } else if (e->mood == 1) {
            /* keeping pace ahead of her, bobbing, firing at her now and then */
            b->vx_fp = pace;
            mg_wave(b, e->base_y, 10, 250, (uint8_t)(age * 3u));
            if ((age % mg_rate(96)) == 0u && sx < 300) {
                uint8_t ang = ng_atan2((int16_t)((p->y - 30) - (b->y - 20)), (int16_t)(p->x - b->x));
                mg_fire((int16_t)(b->x - 24), (int16_t)(b->y - 20), ng_trig_mul(3, ng_cos(ang)),
                        ng_trig_mul(3, ng_sin(ang)), 1, MG_T_BOLT);
                playSFX(SOUND_SFX_6);
            }
            if (age > 480u) e->mood = 2;
        } else {
            b->vx_fp = (int16_t)(pace - 3 * NG_FP_ONE);
        }
        break;
    default:   /* MG_FORM_LINE, MG_FORM_VEE: straight across, holding station */
        b->vx_fp = (int16_t)(pace - 3 * NG_FP_ONE);
        b->vy_fp = (int16_t)((e->base_y - b->y) * 16);
        break;
    }
    if (sx < -96 || sx > 520 || b->y > 280 || (age > 90u && b->y < -64)) {
        ng_chars_remove(b);
        e->body = 0;
        return;
    }
    mg_frame(b, (uint8_t)((age / 6u) % mg_enemy_frames(e->type)), flip);
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
            if (s->life && mg.boss && mg.boss->data0 == MG_B_AIRSHIP) {
                /* a part takes it; the armour turns it with a spark */
                uint8_t part = mg_ship_part_at(s->x, s->y);
                if (part != 0xFFu && mg_ship_open(part)) {
                    mg.ship_target = part;
                    mg_boss_damage((uint8_t)(s->mode == MG_SHOT_PIERCE ? 2 : 1));
                    if (!through) s->life = 0;
                } else if (mg_ship_in_hull(s->x, s->y)) {
                    mg_burst(s->x, s->y, MG_T_SPARK, 1, -1);
                    s->life = 0;
                }
            } else if (s->life && mg.boss && mg_abs((int16_t)(mg.boss->x - s->x)) < 40 &&
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
            ng_sprite_group_set_pos(&s->sprite, scr_x, MG_SY(s->y));
            ng_sprite_group_set_visible(&s->sprite, 1);
            ng_sprite_group_flush(&s->sprite);
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
                case MG_K_LILY:
                    mg.lily = LILY_TIME;
                    mg.air_jump = 1;
                    playSFX(SOUND_SFX_13);
                    playSFX(SOUND_SFX_12);
                    mg_hint("SKY LILY: UP + A IN THE AIR, ONE MORE", PAL_GOLD, 150);
                    mg_voice(MG_VOICE_LILY);
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
                case MG_K_ART:
                    /* a spirit orb: the valley's own Secret Art is hers now */
                    mg.arts_known |= (uint8_t)(1u << mg_art_kind[mg.stage]);
                    if (mg.art < MAX_ART) mg.art++;
                    mg.score += 2000u;
                    playSFX(SOUND_SFX_13);
                    playSFX(SOUND_SFX_9);
                    {
                        /* (built here: initialised data isn't copied to RAM) */
                        char line[40];
                        const char *name = "NEW SECRET ART: ";
                        uint8_t k = 0;
                        while (*name) line[k++] = *name++;
                        name = mg_art_name[mg.stage];
                        while (*name && k < 38u) line[k++] = *name++;
                        line[k] = 0;
                        mg_hint(line, PAL_GOLD, 180);
                    }
                    mg_light(MG_LIGHT_SUN, 30);
                    break;
                case MG_K_BLOOM:
                    if (mg.art < MAX_ART) mg.art++;
                    playSFX(SOUND_SFX_13);
                    mg_hint("A BLOOM BUD: ONE MORE SECRET ART", PAL_GOLD, 120);
                    break;
                default:  /* the elder's charm: it tells her where the hideout is */
                    mg.score += 300u;
                    mg.charm_seen = 1;
                    playSFX(SOUND_SFX_6);
                    mg_hint(mg.boss_active ? mg_boss_hint[mg.stage]
                                           : mg_hideout[mg.stage].hint, PAL_GOLD, 240);
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

        /* A gentle bob, 2 px each way, out of step from one to the next;
         * where it is picked up doesn't move. */
        ng_sprite_group_set_pos(&it->sprite, scr_x,
                                MG_SY(it->y + ng_trig_mul(2, ng_sin((uint8_t)(mg.tick * 4u + (uint16_t)it->x)))));
        ng_sprite_group_set_visible(&it->sprite, 1);
        ng_sprite_group_flush(&it->sprite);
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
        /* A straggler left far behind (or waiting far ahead) goes: the road
         * holds only four at once, and one left alive back there kept every
         * creature after it from coming at all. */
        if (!e->form && e->type != MG_E_WRAITH && !mg.boss_active &&
            mg_abs((int16_t)(e->body->x - p->x)) > 480) {
            ng_chars_remove(e->body);
            e->body = 0;
            continue;
        }
        if (e->body->y > MG_GROUND_Y + 20) {          /* gone down a pit */
            ng_chars_remove(e->body);
            e->body = 0;
            continue;
        }

        int16_t dx = (int16_t)(p->x - e->body->x);
        if (e->mood == MG_MOOD_STUNNED || e->mood == MG_MOOD_SHELL) {
            mg_shell_step(e);
            if (!e->body) continue;
        } else if (e->form) {
            mg_form_step(e, p);
            if (!e->body) continue;
        } else if (!e->posted) {
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

            if (e->mood == 0 && mg_abs(dx) >= (int16_t)(220 - mg.stage * 8) && e->type != MG_E_WRAITH) {
                /* Not noticed her yet: it keeps to its own patch of the
                 * valley instead of marching at her from off-screen. */
                int16_t off = (int16_t)(b->x - e->home);
                if (off > 48) e->heading = -1;
                else if (off < -48) e->heading = 1;
                b->vx_fp = (e->type == MG_E_VINESTING) ? 0 : e->heading * mg_pace(110);
                if (flier) b->vy_fp = (int32_t)(((e->timer >> 5) & 1) ? -40 : 40);
                mg_frame(b, (uint8_t)((uint16_t)(e->timer / 16u) % (uint16_t)nf), (uint8_t)(e->heading < 0));
            } else {
                if (e->mood == 0) e->mood = 1;
                switch (e->type) {
                case MG_E_WRAITH: {
                    /* Straight for her, through the air, from wherever it came. */
                    int16_t ty = (int16_t)(p->y - 30);
                    b->vx_fp = dir * mg_pace(300);
                    b->vy_fp = (b->y < ty - 4) ? 180 : ((b->y > ty + 4) ? -180 : 0);
                    mg_frame(b, (uint8_t)((uint16_t)(e->timer / 6u) % (uint16_t)nf), flip);
                    break;
                }
                case MG_E_BEETLE: case MG_E_SAWBOT: case MG_E_DRILLBOT: {
                    /* Stalk, rev the saw, then commit to a charge it can't
                     * steer out of -- and stand winded after it. The
                     * loggers' saw bot stalks and charges harder, sparks
                     * flying off the blade as it revs; the miners' drill bot
                     * bursts fastest and shortest, drill first. */
                    uint8_t saw = (uint8_t)(e->type == MG_E_SAWBOT), drill = (uint8_t)(e->type == MG_E_DRILLBOT);
                    if (e->mood == 1) {
                        b->vx_fp = dir * mg_pace((int16_t)(saw ? 240 : (drill ? 170 : 200)));
                        mg_frame(b, (uint8_t)((uint16_t)(e->timer / 8u) % (uint16_t)nf), flip);
                        if (e->move_timer) e->move_timer--;
                        else if (mg_abs(dx) < 130) {
                            e->mood = 2; e->move_timer = (uint8_t)(saw ? 20 : (drill ? 14 : 24)); e->heading = dir;
                        }
                    } else if (e->mood == 2) {
                        b->vx_fp = 0;
                        mg_frame(b, (uint8_t)(((e->timer / 3) & 1) ? nf - 1 : 0), (uint8_t)(e->heading < 0));
                        if (saw && (e->move_timer & 3u) == 0u)
                            mg_burst((int16_t)(b->x + e->heading * 22), (int16_t)(b->y - 26), MG_T_SPARK, 1, -1);
                        if (--e->move_timer == 0) { e->mood = 3; e->move_timer = (uint8_t)(drill ? 26 : 36); playSFX(SOUND_SFX_14); }
                    } else if (e->mood == 3) {
                        b->vx_fp = e->heading * mg_pace((int16_t)(saw ? 960 : (drill ? 1120 : 820)));
                        mg_frame(b, (uint8_t)((uint16_t)(e->timer / 3u) % (uint16_t)nf), (uint8_t)(e->heading < 0));
                        if (--e->move_timer == 0) { e->mood = 4; e->move_timer = 30; }
                    } else {
                        b->vx_fp = 0;
                        mg_frame(b, 0, (uint8_t)(e->heading < 0));
                        if (--e->move_timer == 0) { e->mood = 1; e->move_timer = 40; }
                    }
                    break;
                }
                case MG_E_TORCHBOT:
                    /* The burners' machine: it tramps at her and, within
                     * reach, stands and breathes a short tongue of fire --
                     * then must stop to let its torch cool. */
                    if (e->mood == 1) {
                        b->vx_fp = dir * mg_pace(170);
                        mg_frame(b, (uint8_t)((e->timer >> 3) & 1u), flip);
                        if (e->move_timer) e->move_timer--;
                        else if (mg_abs(dx) < 100) { e->mood = 2; e->move_timer = 20; e->heading = dir; }
                    } else if (e->mood == 2) {
                        b->vx_fp = 0;
                        mg_frame(b, 0, (uint8_t)(e->heading < 0));
                        if ((e->move_timer & 3u) == 0u) {
                            MGShot *s = mg_fire((int16_t)(b->x + e->heading * 24), (int16_t)(b->y - 22),
                                                (int16_t)(e->heading * 5), (int16_t)((e->move_timer & 4u) ? -1 : 0), 1, MG_T_FIRE);
                            if (s) s->life = 16;
                            playSFX(SOUND_SFX_10);
                        }
                        if (--e->move_timer == 0) { e->mood = 3; e->move_timer = 60; }
                    } else {
                        b->vx_fp = 0;
                        mg_frame(b, 0, flip);
                        if (--e->move_timer == 0) { e->mood = 1; e->move_timer = 30; }
                    }
                    break;
                case MG_E_SMOGSTACK:
                    /* A walking chimney: it plods her way and now and then
                     * belches a ball of smog straight at her. */
                    b->vx_fp = dir * mg_pace(110);
                    mg_frame(b, (uint8_t)((e->timer >> 4) & 1u), flip);
                    if ((e->timer % mg_rate(100)) == 50u && mg_abs(dx) < 220) {
                        uint8_t aim = ng_atan2((int16_t)((p->y - 24) - (b->y - 44)), dx);
                        mg_fire(b->x, (int16_t)(b->y - 44), (int16_t)((ng_trig_mul(6, ng_cos(aim)) + 1) >> 1),
                                (int16_t)((ng_trig_mul(6, ng_sin(aim)) + 1) >> 1), 1, MG_T_DUST);
                        playSFX(SOUND_SFX_5);
                    }
                    break;
                case MG_E_SLUDGEBARREL:
                    /* A leaking drum: it waddles at her in fits and starts,
                     * and spits a gob of sludge up and over when she's near. */
                    if ((e->timer % 48u) < 30u) {
                        b->vx_fp = dir * mg_pace(260);
                        mg_frame(b, (uint8_t)((e->timer >> 3) & 1u), flip);
                    } else {
                        b->vx_fp = 0;
                        mg_frame(b, 0, flip);
                    }
                    if ((e->timer % mg_rate(90)) == 40u && mg_abs(dx) < 150) {
                        MGShot *s = mg_fire(b->x, (int16_t)(b->y - 30), (int16_t)(dir * 3), -4, 1, MG_T_SPIT);
                        if (s) s->mode = MG_SHOT_ARC;
                        playSFX(SOUND_SFX_5);
                    }
                    break;
                case MG_E_BAGOCTO: case MG_E_BINOCTO:
                    /* The trash octopuses drag themselves at her and fling
                     * their junk up and over -- the bin bag two at a time. */
                    b->vx_fp = dir * mg_pace((int16_t)(e->type == MG_E_BAGOCTO ? 150 : 120));
                    mg_frame(b, (uint8_t)((e->timer >> 4) & 1u), flip);
                    if ((e->timer % mg_rate(e->type == MG_E_BAGOCTO ? 110 : 90)) == 45u && mg_abs(dx) < 180) {
                        MGShot *s = mg_fire(b->x, (int16_t)(b->y - 26), (int16_t)(dir * 3), -4, 1, MG_T_TRASH);
                        if (s) s->mode = MG_SHOT_ARC;
                        if (e->type == MG_E_BINOCTO) {
                            s = mg_fire(b->x, (int16_t)(b->y - 26), (int16_t)(dir * 2), -5, 1, MG_T_TRASH);
                            if (s) s->mode = MG_SHOT_ARC;
                        }
                        playSFX(SOUND_SFX_5);
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
                        if (e->type == MG_E_PLASTICBAT && mg_abs(dx) < 24 && (e->timer % mg_rate(70)) == 0) {
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
                        mg_frame(b, (uint8_t)(e->mood == 2 ? 4 : (uint16_t)(e->timer / (uint16_t)(e->mood == 3 ? 3 : 5)) % 4u),
                                 (uint8_t)(e->mood == 1 ? flip : e->heading < 0));
                    } else {
                        mg_frame(b, (uint8_t)((uint16_t)(e->timer / (uint16_t)(e->mood == 2 ? 3 : 6)) % (uint16_t)nf),
                                 (uint8_t)(e->mood == 1 ? flip : e->heading < 0));
                    }
                    break;
                }
                case MG_E_DRONE: case MG_E_POACHDRONE:
                    if (e->type == MG_E_DRONE && mg.stage == 7) {
                        /* Sunken Reef: the drone's body drifts as a jellyfish,
                         * 24 px up and down on a 64-frame beat. */
                        b->vx_fp = dir * mg_pace(110);
                        mg_wave(b, (int16_t)(p->y - 40), 24, 603, (uint8_t)(e->timer << 2));
                        mg_frame(b, (uint8_t)((uint16_t)(e->timer / 14u) % (uint16_t)nf), flip);
                    } else {
                        /* Hover out of reach and keep a firing distance:
                         * back off if she closes in, drift in if she runs. */
                        int16_t ty = (int16_t)(p->y - 70 + (((e->timer >> 4) & 1) ? -6 : 6));
                        int16_t ad = mg_abs(dx);
                        b->vx_fp = ad < 100 ? -dir * mg_pace(200) : (ad > 170 ? dir * mg_pace(200) : 0);
                        b->vy_fp = (b->y < ty - 4) ? 100 : ((b->y > ty + 4) ? -100 : 0);
                        mg_frame(b, (uint8_t)((uint16_t)(e->timer / 10u) % (uint16_t)nf), flip);
                        if ((e->timer % mg_rate(e->type == MG_E_DRONE ? 110 : 120)) == 55 && ad < 210) {
                            /* Straight at her middle, 4 px a frame whichever
                             * way that is (each part rounded to the pixel). */
                            uint8_t aim = ng_atan2((int16_t)((p->y - 24) - b->y), dx);
                            mg_fire(b->x, b->y, (int16_t)((ng_trig_mul(8, ng_cos(aim)) + 1) >> 1),
                                    (int16_t)((ng_trig_mul(8, ng_sin(aim)) + 1) >> 1), 1,
                                    (uint8_t)(e->type == MG_E_DRONE ? MG_T_BOLT : MG_T_SPIT));
                            playSFX(SOUND_SFX_6);
                        }
                    }
                    break;
                case MG_E_JELLYFISH: case MG_E_ACIDMOTH: case MG_E_CHEMFLY: {
                    /* Never charges her: a drift that hangs at head height,
                     * so she has to go around it or strike it down. The moth
                     * rises and falls in a wide, lazy wave (40 px, a 128-frame
                     * beat), the jellyfish in a gentler one (20 px, 64), and
                     * the fly turns quick little loops (8 px, 32 frames) as
                     * it goes -- true sine and cosine curves (ng_trig),
                     * steered back to head height if they wander. */
                    int16_t speed = (int16_t)(e->type == MG_E_CHEMFLY ? 130 : (e->type == MG_E_ACIDMOTH ? 70 : 90));
                    int16_t head = (int16_t)(p->y - 40), pull;
                    b->vx_fp = dir * mg_pace(speed);
                    if (e->type == MG_E_CHEMFLY) {
                        uint8_t turn = (uint8_t)(e->timer << 3);
                        /* round the loop at 2 pi r / T = 1.57 px a frame (8.8) */
                        b->vx_fp = (int16_t)(b->vx_fp + ng_trig_mul(402, ng_cos(turn)));
                        pull = (int16_t)(head - b->y);
                        b->vy_fp = (int16_t)(ng_trig_mul(402, ng_sin(turn)) +
                                             (pull > 16 ? 48 : (pull < -16 ? -48 : 0)));
                    } else if (e->type == MG_E_ACIDMOTH) {
                        mg_wave(b, head, 40, 503, (uint8_t)(e->timer << 1));
                    } else {
                        mg_wave(b, head, 20, 503, (uint8_t)(e->timer << 2));
                    }
                    mg_frame(b, (uint8_t)((uint16_t)(e->timer / 12u) % (uint16_t)nf), flip);
                    break;
                }
                case MG_E_TOXICCRAB:
                    /* Sideways bursts with a pause between: faster when she's close. */
                    if ((e->timer % 40) < 22) {
                        b->vx_fp = dir * mg_pace((int16_t)(mg_abs(dx) < 70 ? 640 : 420));
                        mg_frame(b, (uint8_t)((uint16_t)(e->timer / 4u) % (uint16_t)nf), flip);
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
                        mg_frame(b, (uint8_t)((uint16_t)(e->timer / 12u) % (uint16_t)nf), flip);
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
                case MG_E_DARTFROG: {
                    /* Small quick hops her way, sitting between them; up
                     * close it puffs its poison. Sitting shows frame 0, the
                     * leap frame 1. Its skin is the poison: landing on it
                     * hurts her (it isn't one of the soft creatures). */
                    uint8_t sitting = (uint8_t)(ng_physics_is_grounded(b) || b->y >= MG_GROUND_Y - 2);
                    if (sitting) b->vx_fp = 0;
                    if ((e->timer % 46u) == 0u && sitting) {
                        b->vy_fp = -3 * NG_FP_ONE - NG_FP_ONE / 2;
                        b->vx_fp = dir * mg_pace(300);
                    }
                    mg_frame(b, (uint8_t)(sitting ? 0 : 1), flip);
                    if ((e->timer % mg_rate(120)) == 60u && sitting && mg_abs(dx) < 120) {
                        mg_fire(b->x, (int16_t)(b->y - 10), (int16_t)(dir * 2), -2, 1, MG_T_SPIT);
                        playSFX(SOUND_SFX_5);
                    }
                    break;
                }
                case MG_E_VINESTING:
                    /* Rooted where it grows, turning to face her and lashing
                     * faster the closer she dares to come. */
                    b->vx_fp = 0;
                    b->vy_fp = 0;
                    mg_frame(b, (uint8_t)((uint16_t)(e->timer / 20u) % (uint16_t)nf), flip);
                    if ((e->timer % mg_rate(mg_abs(dx) < 36 ? 60 : 100)) == 30u && mg_abs(dx) < 64) {
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
                    mg_frame(b, (uint8_t)((uint16_t)(e->timer / 12u) % (uint16_t)nf), flip);
                    if ((e->type == MG_E_SLIME || e->type == MG_E_SPOREGOB)
                        && (e->timer % mg_rate(150)) == 75 && mg_abs(dx) < 160) {
                        mg_fire(b->x, (int16_t)(b->y - 12), (int16_t)(dir * 3), -1, 1, MG_T_SPIT);
                        playSFX(SOUND_SFX_5);
                    }
                    if (e->type == MG_E_GOBLIN && mg.stage == 9 && (e->timer % mg_rate(130)) == 65 && mg_abs(dx) < 170) {
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
        if (e->body) mg_enemy_contact(e, p);
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
            /* the blow itself: her red flash (mg_player_damage) runs out */
            mg.hurt_lit = 2;
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
        /* The mist veil: no flicker -- she is drawn in pale, misty colours
         * inside a soft ring of light for as long as it lasts (the colours
         * put back now and then, should anything else have set hers). */
        if (!mg.veil_lit || (mg.tick & 31u) == 0u) { mg_veil_palette(); mg.veil_lit = 1; }
        if (!mg.fx_time || mg.fx_kind == MG_LIGHT_AURA) mg_light(MG_LIGHT_AURA, 2);
    }

    /* Power-ups run down whether or not she is fighting. */
    if (mg.swift && --mg.swift == 0) mg.hud_dirty = 1;
    if (mg.might && --mg.might == 0) mg.hud_dirty = 1;
    if (mg.veil && --mg.veil == 0) {
        mg.hud_dirty = 1;
        if (mg.veil_lit) { mg.veil_lit = 0; mg_palette(PAL_HERO, mg_hero_normal_pal()); }
    }
    if (mg.spring && --mg.spring == 0) mg.hud_dirty = 1;
    if (mg.crown && --mg.crown == 0) mg.hud_dirty = 1;
    if (mg.lily && --mg.lily == 0) mg.hud_dirty = 1;
    if (mg.flash && --mg.flash == 0) mg_palette(PAL_HERO, mg_hero_normal_pal());
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
    uint8_t target = (uint8_t)((uint16_t)((uint16_t)hp * MG_HP_BAR_PX) / (uint16_t)MAX_HP);
    if (mg.hp_px != target) {
        if (mg.hp_px > target) mg.hp_px--;
        else mg.hp_px = (uint8_t)(mg.hp_px + 2 > target ? target : mg.hp_px + 2);
        mg_draw_hp_bar();
    }
    if (mg.boss_active && mg.boss && mg.boss->max_hp) {
        target = (uint8_t)((uint16_t)((uint16_t)mg.boss->hp * MG_BOSS_BAR_PX) / (uint16_t)mg.boss->max_hp);
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
        ng_fix_puts(7, ROW_CLOCK + 1, "GOLD", PAL_GOLD);
        mg_number(12, ROW_CLOCK + 1, mg.bonus_hits, 3, PAL_TEXT);
        ng_fix_puts(7, ROW_CLOCK + 2, "LIFE", PAL_GOLD);
        mg_number(12, ROW_CLOCK + 2, mg.bonus_life, 3, mg.bonus_life > 1u ? PAL_TEXT : PAL_WARN);
        return;
    }
    ng_fix_puts(7, ROW_CLOCK, "TIME", PAL_GOLD);
    mg_number(12, ROW_CLOCK, mg.clock, 3, mg.clock <= MG_HURRY_AT ? PAL_WARN : PAL_TEXT);
}

/*
 * The stage clear timer: the engine's stage clock (ng_game_time), which
 * Maiya runs only while she is on the road -- not on the mission card, the
 * warp, a guardian's entrance, a fall, the clear, a pause or a hitstop --
 * shown as minutes, seconds and frames. It counts frames at 60 a second,
 * so on a real board (about 59.18 frames a second) its seconds run a
 * little slow against a wall clock: see docs/game_time.md.
 */
static uint32_t mg_best_time[MG_LEVEL_COUNT];   /* RAM only, 0 = none yet */

/* Frames as "mm:ss:ff", by subtraction (at the clear, not every frame). */
static void NEOGEO_USER mg_time_text(char *out, uint32_t frames)
{
    uint8_t mm = 0, ss = 0;
    while (frames >= 3600u && mm < 99u) { frames -= 3600u; mm++; }
    if (frames >= 3600u) frames = 3599u;
    while (frames >= 60u) { frames -= 60u; ss++; }
    out[0] = '0'; out[1] = '0'; out[3] = '0'; out[4] = '0'; out[6] = '0'; out[7] = '0';
    while (mm >= 10u) { mm -= 10u; out[0]++; }
    out[1] = (char)('0' + mm);
    while (ss >= 10u) { ss -= 10u; out[3]++; }
    out[4] = (char)('0' + ss);
    while (frames >= 10u) { frames -= 10u; out[6]++; }
    out[7] = (char)('0' + frames);
    out[2] = ':'; out[5] = ':'; out[8] = 0;
}

/* The HUD's STAGE line starts over (a new scene, or the FIX was cleared). */
static void NEOGEO_USER mg_stage_time_reset(void)
{
    uint8_t i;
    mg.time_seen = 0;
    for (i = 0; i < 6; i++) mg.time_digit[i] = 0;
    for (i = 0; i < 14; i++) mg.time_shown[i] = 0;
}

/* The HUD's STAGE line: the clock is carried forward digit by digit (a
 * frame at a time, no division) and only the characters that changed are
 * written to the FIX layer -- normally one or two a frame. */
static void NEOGEO_USER mg_draw_stage_time(void)
{
    static const char label[6] = "STAGE";
    uint32_t now = ng_game_time_stage_frame();
    uint8_t *d = mg.time_digit;
    char text[14], one[2];
    uint8_t i;

    if (now < mg.time_seen) mg_stage_time_reset();
    while (mg.time_seen < now) {
        mg.time_seen++;
        if (d[0] == 9 && d[1] == 9 && d[2] == 5 && d[3] == 9 && d[4] == 5 && d[5] == 9) continue;
        if (++d[5] < 10) continue;
        d[5] = 0;
        if (++d[4] < 6) continue;
        d[4] = 0;
        if (++d[3] < 10) continue;
        d[3] = 0;
        if (++d[2] < 6) continue;
        d[2] = 0;
        if (++d[1] < 10) continue;
        d[1] = 0;
        d[0]++;
    }
    for (i = 0; i < 5; i++) text[i] = label[i];
    text[5] = ' ';
    text[6] = (char)('0' + d[0]); text[7] = (char)('0' + d[1]); text[8] = ':';
    text[9] = (char)('0' + d[2]); text[10] = (char)('0' + d[3]); text[11] = ':';
    text[12] = (char)('0' + d[4]); text[13] = (char)('0' + d[5]);
    one[1] = 0;
    for (i = 0; i < 14; i++) {
        if (text[i] == mg.time_shown[i]) continue;
        mg.time_shown[i] = one[0] = text[i];
        ng_fix_puts((uint8_t)(MG_STAGE_TIME_COL + i), ROW_CLOCK, one, i < 5 ? PAL_GOLD : PAL_TEXT);
    }
}

/* The clear: the final time under the victory lines, and the best. */
/*
 * The combo: every creature beaten within a second and two thirds of the
 * last adds one, shown at the right under the HUD; when the chain breaks it
 * pays out n x n x 50 (a chain of ten, 5000) and the total stays up a
 * moment.
 */
enum { MG_COMBO_TIME = 100, MG_COMBO_SHOW = 90, MG_COMBO_COL = 26 };

static void NEOGEO_USER mg_combo_add(void)
{
    if (mg.state != MG_PLAY) return;
    if (mg.combo_t) { if (mg.combo_n < 99u) mg.combo_n++; }
    else mg.combo_n = 1;
    mg.combo_t = MG_COMBO_TIME;
    if (mg.combo_n >= 2u) {
        mg.combo_show = 0;
        ng_fix_clear_rect(MG_COMBO_COL, ROW_COMBO, 13, 1, PAL_TEXT);
        ng_fix_puts(MG_COMBO_COL, ROW_COMBO, "COMBO X", PAL_GOLD);
        mg_number(MG_COMBO_COL + 7, ROW_COMBO, mg.combo_n, 2, PAL_TEXT);
    }
}

static void NEOGEO_USER mg_combo_tick(void)
{
    if (mg.combo_show && --mg.combo_show == 0)
        ng_fix_clear_rect(MG_COMBO_COL, ROW_COMBO, 13, 1, PAL_TEXT);
    if (!mg.combo_t || --mg.combo_t) return;
    if (mg.combo_n >= 2u) {
        uint16_t bonus = (uint16_t)((uint16_t)mg.combo_n * mg.combo_n * 50u);
        mg.score += bonus;
        ng_fix_clear_rect(MG_COMBO_COL, ROW_COMBO, 13, 1, PAL_TEXT);
        ng_fix_putc(MG_COMBO_COL, ROW_COMBO, '+', PAL_GOLD);
        mg_number(MG_COMBO_COL + 1, ROW_COMBO, bonus, 5, PAL_GOLD);
        mg.combo_show = MG_COMBO_SHOW;
        playSFX(SOUND_SFX_12);
    }
    mg.combo_n = 0;
}

/*
 * The stage's rank, on the clear card: two points each for time (inside
 * par -- the road's length at 120 px a second, plus a minute for the
 * guardian; the sky road's flight is its own length), for health (no hit
 * taken and no life lost; one for three hits or fewer), and for secrets
 * (every one found and the vault; one for half). Six is S, five A, three
 * or four B, one or two C, none D, worth 10000, 5000, 2000, 500 or 0.
 */
static void NEOGEO_USER mg_stage_rank(void)
{
    static const char ranks[5] = { 'S', 'A', 'B', 'C', 'D' };
    static const uint16_t bonus[5] = { 10000u, 5000u, 2000u, 500u, 0u };
    const MGLevel *lv = &mg_levels[mg.stage];
    uint16_t used = (uint16_t)(MG_LEVEL_SECONDS - mg.clock);
    uint16_t par = (uint16_t)(lv->width / 120u + 60u + (mg.flying ? 40u : 0u));
    uint8_t pts = 0, have = 0, found = 0, i, r;
    char line[] = "RANK X";

    if (used <= par) pts += 2; else if (used <= par + par / 2u) pts += 1;
    if (!mg.level_falls && !mg.attempt_hits) pts += 2; else if (!mg.level_falls && mg.attempt_hits <= 3u) pts += 1;
    for (i = 0; i < MG_SECRET_COUNT; i++) {
        if (!lv->secrets[i].x) continue;
        have++;
        if (mg.secret_mask & (1u << i)) found++;
    }
    if (mg_hideout[mg.stage].x) { have++; if (mg.vault_done) found++; }
    if (have && found == have) pts += 2; else if (have && found * 2u >= have) pts += 1;

    r = (uint8_t)(pts >= 6 ? 0 : pts == 5 ? 1 : pts >= 3 ? 2 : pts >= 1 ? 3 : 4);
    line[5] = ranks[r];
    mg_centre(ROW_CARD + 11, line, r == 0 ? PAL_GOLD : (r <= 2 ? PAL_SKY : PAL_TEXT));
    mg.score += bonus[r];
}

static void NEOGEO_USER mg_stage_time_final(void)
{
    uint32_t t = ng_game_time_stage_frame();
    char line[24] = "CLEAR TIME ";
    char *p = line + 11;
    uint8_t best = (uint8_t)(!mg_best_time[mg.stage] || t < mg_best_time[mg.stage]);

    ng_game_time_stage_run(0);
    mg_time_text(p, t);
    mg_centre(ROW_CARD + 6, line, PAL_TEXT);
    if (best) {
        mg_best_time[mg.stage] = t;
        mg_centre(ROW_CARD + 8, "NEW BEST TIME!", PAL_GOLD);
    } else {
        line[0] = 'B'; line[1] = 'E'; line[2] = 'S'; line[3] = 'T'; line[4] = ' ';
        mg_time_text(line + 5, mg_best_time[mg.stage]);
        mg_centre(ROW_CARD + 8, line, PAL_SKY);
    }
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
    if (mg.lily)   x = mg_tray_slot(slot++, x, mg_trinket_tiles[MG_K_LILY], PAL_TRINKET, (mg.lily + 59u) / 60u, 2);
    if (mg.crown)  x = mg_tray_slot(slot++, x, mg_trinket_tiles[MG_K_CROWN], PAL_TRINKET, (mg.crown + 59u) / 60u, 1);

    for (; slot < MG_TRAY_SLOTS; slot++) {
        ng_sprite_group_set_visible(&mg.tray[slot], 0);
        ng_sprite_group_flush(&mg.tray[slot]);
    }
}

/* ------------------------------------------------------------------ */
/*  Main Per-Frame Update                                             */
/*
 * The hidden vault. Each valley has one hideout: a spot where kneeling for
 * half a second takes her to a vault of treasure -- the guardian's ground
 * at the end of the valley, behind the sealed gate, before it has come:
 * two shelves, gold and silver coming in waves, a life among them -- for
 * ten seconds, then back to the spot. Once a valley.
 * It is hard to find on purpose: the spot glints only now and then, until
 * the elder's charm (itself hidden) tells her where it is; then it sparkles.
 */
enum { MG_VAULT_TIME = 600, MG_VAULT_ITEMS = 12 };

static void NEOGEO_USER mg_vault_clear_items(void)
{
    uint8_t i;
    for (i = 0; i < MG_ITEMS; i++) {
        mg.items[i].life = 0;
        ng_sprite_group_set_visible(&mg.items[i].sprite, 0);
        ng_sprite_group_flush(&mg.items[i].sprite);
    }
}

static void NEOGEO_USER mg_hideout_step(void)
{
    const MGHideout *h = &mg_hideout[mg.stage];
    NGCharacter *p = mg.player;
    int16_t vx = (int16_t)(mg_levels[mg.stage].width - NG_SCREEN_W);   /* the arena's ground, behind the gate */

    if (mg.vault) {
        static const int16_t spot[MG_VAULT_ITEMS][2] = {
            {60, 164}, {112, 164}, {56, 106}, {100, 106}, {170, 164}, {200, 74},
            {246, 74}, {230, 164}, {280, 164}, {140, 164}, {224, 74}, {20, 164},
        };
        static const uint8_t kind[MG_VAULT_ITEMS] = {
            MG_K_GOLD, MG_K_SILVER, MG_K_GOLD, MG_K_GOLD, MG_K_FLOWER, MG_K_GOLD,
            MG_K_SILVER, MG_K_GOLD, MG_K_GOLD, MG_K_SILVER, MG_K_LIFE, MG_K_GOLD,
        };
        if (mg.vault_timer) mg.vault_timer--;
        if (mg.vault_left && (mg.tick % 10u) == 0u) {
            uint8_t k = (uint8_t)(MG_VAULT_ITEMS - mg.vault_left);
            uint8_t what = kind[k];
            if (what == MG_K_LIFE && mg.life_pickups_used >= MG_LIFE_PICKUP_LIMIT) what = MG_K_GOLD;
            /* the first, in a valley whose art she hasn't learned: its spirit orb */
            if (k == 0u && !((mg.arts_known >> mg_art_kind[mg.stage]) & 1u)) what = MG_K_ART;
            if (mg_drop_trinket((int16_t)(vx + spot[k][0]), spot[k][1], what)) {
                mg.vault_left--;
                mg_burst((int16_t)(vx + spot[k][0] + 16), (int16_t)(spot[k][1] + 8), MG_T_STAR, 1, -1);
            }
        }
        /* (the count waits while a longer word -- a new art -- is up) */
        if ((mg.vault_timer % 60u) == 0u && mg.vault_timer && mg.hint_timer < 20u) {
            char line[] = "THE VAULT: 00";
            uint8_t s = (uint8_t)(mg.vault_timer / 60u);
            line[11] = (char)('0' + s / 10u);
            line[12] = (char)('0' + s % 10u);
            mg_hint(line, PAL_GOLD, 70);
        }
        if (!mg.vault_timer) {
            /* Back where she knelt, the vault closed behind her. */
            mg.vault = 0;
            mg_vault_clear_items();
            ng_char_set_pos(p, h->x, h->y);
            p->vx_fp = p->vy_fp = 0;
            ng_camera_snap(&mg.camera, (int16_t)(h->x - NG_SCREEN_W / 2), 0);
            mg_burst(p->x, (int16_t)(p->y - 30), MG_T_STAR, 4, -2);
            playSFX(SOUND_SFX_13);
            mg_hint("BACK ON THE ROAD", PAL_SKY, 90);
        }
        return;
    }
    if (mg.vault_done || mg.boss_active || !h->x || mg.state != MG_PLAY) return;
    if ((mg.tick % (mg.charm_seen ? 50u : 420u)) == 0u)
        mg_burst(h->x, (int16_t)(h->y - 6), MG_T_STAR, (uint8_t)(mg.charm_seen ? 2 : 1), -1);
    if (mg.crouch_timer >= 30 && mg_abs((int16_t)(p->x - h->x)) < 14 && mg_abs((int16_t)(p->y - h->y)) <= 2) {
        mg.vault = 1;
        mg.vault_done = 1;
        mg.vault_timer = MG_VAULT_TIME;
        mg.vault_left = MG_VAULT_ITEMS;
        mg.arena[0].x = (int16_t)(vx + 40);  mg.arena[0].y = 136; mg.arena[0].width = 96;
        mg.arena[1].x = (int16_t)(vx + 184); mg.arena[1].y = 104; mg.arena[1].width = 96;
        mg_vault_clear_items();
        mg_burst(p->x, (int16_t)(p->y - 30), MG_T_STAR, 4, -2);
        ng_char_set_pos(p, (int16_t)(vx + 40), MG_GROUND_Y);
        p->vx_fp = p->vy_fp = 0;
        mg.crouch_timer = 0;
        mg.sitting = 0;
        ng_camera_snap(&mg.camera, vx, 0);
        playSFX(SOUND_SFX_13);
        playSFX(SOUND_SFX_12);
        mg_hint("A HIDDEN VAULT! GRAB ALL YOU CAN", PAL_GOLD, 60);
    }
}

/* ------------------------------------------------------------------ */
/*
 * The valley's own obstacle, each frame of play.
 *  - Rotten ledges (grey, marked "rotten" in the stage file): stand on
 *    one and it trembles, dust trickling off it, then gives way and drops
 *    out of sight; it grows back a while later.
 *  - Underwater: now and then a bubble rises from her.
 */
static void NEOGEO_USER mg_stage_mechanics(void)
{
    uint8_t mech = mg_mech(), i;
    NGCharacter *p = mg.player;
    const MGLevel *lv = &mg_levels[mg.stage < MG_LEVEL_COUNT ? mg.stage : 0];

    if (lv->rotten && !mg.boss_active && mg.state == MG_PLAY) {
        for (i = 0; i < MG_PLATFORM_COUNT; i++) {
            const MGPlatform *pl = &lv->platforms[i];
            if (!((lv->rotten >> i) & 1u)) continue;
            if (mg.ledge_gone[i]) {
                if (--mg.ledge_gone[i] == 0) {
                    /* It grows back: a puff of leaves where it stands again. */
                    mg.ledge_stand[i] = 0;
                    mg_burst((int16_t)(pl->x + pl->width / 2), (int16_t)(pl->y + 8), MG_T_LEAF, 3, -1);
                }
                continue;
            }
            if (mg.on_ledge && mg.ledge_index == i) {
                if (++mg.ledge_stand[i] >= MG_CRUMBLE_AFTER) {
                    /* It gives way and drops out from under her. */
                    mg.ledge_gone[i] = MG_CRUMBLE_BACK;
                    mg.on_ledge = 0;
                    mg_burst((int16_t)(pl->x + pl->width / 2), (int16_t)(pl->y + 10), MG_T_DUST, 3, 1);
                    playSFX(SOUND_SFX_10);
                } else if (mg.ledge_stand[i] > 12 && (mg.ledge_stand[i] & 3) == 0) {
                    mg_burst((int16_t)(pl->x + 8 + ng_rand_range((uint16_t)(pl->width - 16))), (int16_t)(pl->y + 18),
                             MG_T_DUST, 1, 2);
                }
            } else if (mg.ledge_stand[i]) {
                mg.ledge_stand[i]--;
            }
        }
    }

    if (mech == MG_M_WATER && (mg.tick % 50u) == 0u)
        mg_burst(p->x, (int16_t)(p->y - 50), MG_T_BUBBLE, 1, -2);
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
                if (mg_spawn_enemy(MG_E_WRAITH, x, (int16_t)(40 + (ng_rand() & 63)), 0)) {
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
        if (mg.hazard_disabled_mask & (uint16_t)(1u << i)) continue;
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
        uint16_t bit = (uint16_t)(1u << i);
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
        uint16_t bit = (uint16_t)(1u << i);
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

    if (mg.flying) {
        /* Kneeling on the eagle's back; it beats its wings, quicker after
         * a climb, and glides through a swoop. */
        mg_frame(p, (uint8_t)(mg.hurt > HURT_LOCK ? MG_F_HURT0 : MG_F_RIDE), 0);
        if (mg.spin) mg.spin--;
        if (mg.eagle)
            mg_frame(mg.eagle, (uint8_t)(mg.dash ? 1u : ((mg.tick >> (mg.spin ? 2 : 3)) % MG_EAGLE_FRAMES)), 0);
        return;
    }
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
        /* a moment down and she is gathered to spring: the high leap's ready */
        mg_frame(p, (uint8_t)(mg.crouch_timer >= MG_LEAP_KNEEL ? MG_F_LEAP0 : MG_F_CROUCH), mg.facing);
        return;
    }
    if (mg.leaping) {
        if (p->vy_fp < 0 && !grounded) { mg_frame(p, MG_F_LEAP1, mg.facing); return; }
        mg.leaping = 0;
    }
    if (mg.spin) mg.spin--;
    if (mg.hurt > HURT_LOCK) {
        mg_frame(p, MG_F_HURT0, mg.facing);
    } else if (mg.rising) {
        /* Gathered low, the uppercut, then stretched straight up at the top. */
        mg_frame(p, (uint8_t)(mg.rising > MG_RISE_TIME - 3 ? MG_F_RISE0
                              : (mg.rising > MG_RISE_TIME - 14 ? MG_F_RISE1 : MG_F_RISE2)), mg.facing);
    } else if (mg.art_pose) {
        /* The Secret Art: her hands to the sky, then her palms to the ground. */
        mg.art_pose--;
        mg_frame(p, (uint8_t)(mg.art_pose > 18 ? MG_F_ART0 : MG_F_ART1), mg.facing);
    } else if (mg.super_surge) {
        mg_frame(p, (uint8_t)(mg.super_surge >= MG_SURGE_TIME - MG_SURGE_WINDUP ? MG_F_RISE0 : MG_F_SURGE),
                 mg.facing);
    } else if (mg.dash && grounded) {
        mg_frame(p, (uint8_t)(MG_F_RUN0 + ((mg.dash / 2) % 3)), mg.facing);
    } else if (mg.attack) {
        mg.attack--;
        mg_frame(p, (uint8_t)(mg.attack > 6 ? MG_F_ATK1 : MG_F_ATK2), mg.facing);
    } else if (mg.cast) {
        mg.cast--;
        mg_frame(p, (uint8_t)(mg.cast > 5 ? MG_F_CAST1 : MG_F_CAST2), mg.facing);
    } else if (mg.swimming && !grounded) {
        /* She swims lying along the water, kicking; still, a slow glide. */
        static const uint8_t stroke[4] = { MG_F_SWIM0, MG_F_SWIM1, MG_F_SWIM2, MG_F_SWIM1 };
        uint8_t moving = (uint8_t)(mg_abs((int16_t)p->vx_fp) > 96 || mg_abs((int16_t)p->vy_fp) > 96);
        mg_frame(p, stroke[(mg.tick >> (moving ? 3 : 5)) & 3u], mg.facing);
    } else if (!grounded) {
        mg_frame(p, (uint8_t)(p->vy_fp < 0 ? MG_F_JUMP1 : MG_F_JUMP3), mg.facing);
    } else if (p->vx_fp != 0) {
        uint16_t walked = (uint16_t)(mg.walk_distance + mg_abs((int16_t)p->vx_fp));
        while (walked >= 40u * NG_FP_ONE) walked = (uint16_t)(walked - 40u * NG_FP_ONE);
        mg.walk_distance = walked;
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
    /* She tells the captive it's free; it thanks her in its own voice. */
    mg_voice(MG_VOICE_FREE);
    if (type <= 3u) mg_voice_later((uint8_t)(MG_VOICE_ELDER + type), 60);

    if (type == 0) {
        mg_hint("ELDER: STRIKE OR STOMP ITS HEAD!", PAL_GOLD, 120);
    } else if (type == 1) {
        if (mg.player->hp < MAX_HP) mg.player->hp++;
        mg_hint("MAIDEN: HEALTH RESTORED!", PAL_SKY, 120);
    } else if (type == 2) {
        /* The spirit hands her a sky lily too: a second jump for a while. */
        if (mg.art < MAX_ART) mg.art++;
        mg.lily = LILY_TIME;
        mg.air_jump = 1;
        mg.hud_dirty = 1;
        mg_hint("SPIRIT: SECRET ART AND A SKY LILY!", PAL_GOLD, 120);
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
    mg_backdrop(0x8000);
    maiya_vblank();
    mg_ui_palettes();
    mg_palette(PAL_ALLY, mg_elder_pal);
    mg_music(SOUND_TRACK_I);

    /*
     * The card for what comes next: that valley's own painting, dimmed,
     * behind Sunboy; a map of the whole journey -- every mission a
     * stop, the ones behind her starred, the next one her mark -- and the
     * next mission's number and name.
     */
    if (!done) {
        uint8_t i;
        mg_background(mg_levels[next_stage].background, 0);
        for (i = 0; i < 16u; i++)
            mg_shade_bank((uint8_t)(PAL_BG + i), &mg_pal_base[(uint16_t)(PAL_BG + i) * 16u], 2, 0);
    }

    /* the elder briefs her: the next valley's pollution, and its foe */
    ng_sprite_group_init(&boy, SLOT_TITLE, 2, 3, mg_elder_tiles[0], PAL_ALLY);
    ng_sprite_group_set_pos(&boy, 144, 26);
    ng_sprite_group_upload(&boy);

    mg_centre(2, "THE ELDER'S COUNSEL", PAL_GOLD);
    {
        uint8_t k, col = (uint8_t)((40u - (MG_LEVEL_COUNT * 2u - 1u)) / 2u);
        for (k = 0; k < MG_LEVEL_COUNT; k++) {
            uint8_t c = (uint8_t)(k < next_stage ? '*' : (k == next_stage ? 'M' : 'O'));
            ng_fix_putc((uint8_t)(col + k * 2u), 11, c,
                        k < next_stage ? PAL_GOLD : (k == next_stage ? PAL_SKY : PAL_TEXT));
            if (k + 1u < MG_LEVEL_COUNT) ng_fix_putc((uint8_t)(col + k * 2u + 1u), 11, '-', PAL_TEXT);
        }
        if (!done) {
            char line[40] = "MISSION ";
            uint8_t at = 8, n = (uint8_t)(next_stage + 1u), j = 0;
            if (n >= 10u) line[at++] = (char)('0' + n / 10u);
            line[at++] = (char)('0' + n % 10u);
            line[at++] = ':';
            line[at++] = ' ';
            while (mg_levels[next_stage].name[j] && at < 38u) line[at++] = mg_levels[next_stage].name[j++];
            line[at] = '\0';
            mg_centre(12, line, PAL_GOLD);
        }
    }
    if (!done) {
        mg_centre(14, mg_briefing[next_stage][0], PAL_SKY);
        mg_centre(16, mg_briefing[next_stage][1], PAL_TEXT);
    }

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
enum { MG_BONUS_TIME = 1500, MG_BONUS_CARD = 150, MG_BONUS_OUTRO = 90, MG_BONUS_PERFECT = 8,
       MG_BONUS_ARRIVE = 180,   /* the drone keeps away this long, then flies in...          */
       MG_BONUS_OPEN = 270,     /* ...and fires its first shot here, four and a half seconds in */
       MG_BONUS_LIVES = 3, MG_BONUS_SAFE = 90 };

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

/* The cannon's fire gone from the field. */
static void NEOGEO_USER mg_bonus_fire_stop(void)
{
    uint8_t i;
    for (i = 0; i < MG_BULLETS; i++) {
        if (mg.bullets[i].life) mg_burst((int16_t)(mg.bullets[i].x >> 4), (int16_t)(mg.bullets[i].y >> 4), MG_T_DUST, 1, 0);
        mg.bullets[i].life = 0;
        ng_sprite_group_set_visible(&mg.bullet_spr[i], 0);
        ng_sprite_group_flush(&mg.bullet_spr[i]);
    }
    ng_sprite_group_set_visible(&mg.cannon, 0);
    ng_sprite_group_flush(&mg.cannon);
}

/*
 * The blight cannon: the poachers' drone. It keeps away for the first
 * three seconds, flies in over the field, and then fires rings of shots in
 * every direction, each ring turned a little from the last so the gaps
 * wander. The rounds grow harder one by one -- the first a sparse, slow
 * ring every second and a half; each later round, and the harder operator
 * settings, a shot more, a little quicker, a little sooner, and from the
 * fourth step on one aimed at her between rings. She can take three shots
 * (a moment's safety after each); the third ends the round.
 */
static void NEOGEO_USER mg_bonus_cannon(NGCharacter *p)
{
    int8_t step = (int8_t)(mg.next_stage / 2u) - 1 + ((int8_t)mg.difficulty - 1);
    uint8_t ring, period;
    int16_t speed, cx, cy = 64;
    uint16_t age = (uint16_t)(MG_BONUS_TIME - mg.state_timer);
    uint8_t i, k;

    if (step < 0) step = 0;
    if (step > 6) step = 6;
    ring = (uint8_t)(4 + step);
    period = (uint8_t)(96 - step * 8);
    speed = (int16_t)(14 + step * 2);                      /* 1/16 px a frame */
    cx = (int16_t)(160 + ng_trig_mul(110, ng_sin((uint8_t)(mg.state_timer >> 1))));

    if (age < MG_BONUS_ARRIVE) {
        ng_sprite_group_set_visible(&mg.cannon, 0);
        ng_sprite_group_flush(&mg.cannon);
    } else {
        /* it drops in from above the screen, then holds its height */
        if (age < MG_BONUS_OPEN) cy = (int16_t)(-40 + (int16_t)((age - MG_BONUS_ARRIVE) * 104u / (MG_BONUS_OPEN - MG_BONUS_ARRIVE)));
        ng_sprite_group_set_pos(&mg.cannon, (int16_t)(cx - (int16_t)(MG_POACHDRONE_W / 2u)), (int16_t)(cy - 24));
        ng_sprite_group_set_tile_base(&mg.cannon, mg_poachdrone_tiles[(mg.state_timer >> 3) & 1u]);
        ng_sprite_group_set_visible(&mg.cannon, 1);
        ng_sprite_group_flush(&mg.cannon);
    }

    if (age >= MG_BONUS_OPEN && ((age - MG_BONUS_OPEN) % period) == 0u) {
        for (k = 0, i = 0; k < ring && i < MG_BULLETS; i++) {
            MGBullet *b = &mg.bullets[i];
            uint8_t a = (uint8_t)(mg.cannon_angle + (uint8_t)((256u * k) / ring));
            if (b->life) continue;
            b->x = (int16_t)(cx << 4); b->y = (int16_t)(cy << 4);
            b->vx = ng_trig_mul(speed, ng_cos(a));
            b->vy = ng_trig_mul(speed, ng_sin(a));
            b->life = 220;
            k++;
        }
        mg.cannon_angle = (uint8_t)(mg.cannon_angle + 11u);
        playSFX(SOUND_SFX_5);
    }
    if (step >= 3 && age >= MG_BONUS_OPEN && ((age - MG_BONUS_OPEN) % period) == period / 2u) {
        /* One aimed at her. */
        for (i = 0; i < MG_BULLETS; i++) {
            MGBullet *b = &mg.bullets[i];
            uint8_t a;
            if (b->life) continue;
            a = ng_atan2((int16_t)((p->y - 24) - cy), (int16_t)(p->x - cx));
            b->x = (int16_t)(cx << 4); b->y = (int16_t)(cy << 4);
            b->vx = ng_trig_mul((int16_t)(speed + 6), ng_cos(a));
            b->vy = ng_trig_mul((int16_t)(speed + 6), ng_sin(a));
            b->life = 220;
            break;
        }
    }
    if (mg.bonus_safe) mg.bonus_safe--;
    for (i = 0; i < MG_BULLETS; i++) {
        MGBullet *b = &mg.bullets[i];
        int16_t x, y;
        if (!b->life) continue;
        b->x = (int16_t)(b->x + b->vx);
        b->y = (int16_t)(b->y + b->vy);
        b->life--;
        x = (int16_t)(b->x >> 4); y = (int16_t)(b->y >> 4);
        if (x < -16 || x > 336 || y < -16 || y > MG_GROUND_Y + 8 || !b->life) {
            b->life = 0;
            ng_sprite_group_set_visible(&mg.bullet_spr[i], 0);
        } else {
            ng_sprite_group_set_pos(&mg.bullet_spr[i], (int16_t)(x - 8), (int16_t)(y - 8));
            ng_sprite_group_set_visible(&mg.bullet_spr[i], 1);
            /* her body, not the air around her */
            if (mg_abs((int16_t)(x - p->x)) < 7 && y > p->y - 40 && y < p->y - 4) {
                b->life = 0;
                ng_sprite_group_set_visible(&mg.bullet_spr[i], 0);
                mg_bonus_caught();
            }
        }
        ng_sprite_group_flush(&mg.bullet_spr[i]);
    }
    /* Gold falls to the road now and then, somewhere she has to go for it. */
    if (age > MG_BONUS_CARD && (age % 50u) == 25u)
        mg_drop_trinket((int16_t)(24 + ng_rand_range(256)), 164, MG_K_GOLD);
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
    mg.bonus_life = MG_BONUS_LIVES;
    mg.bonus_safe = 0;
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

    /* The blight cannon: the poachers' drone hovering over the field,
     * its fire in the sprites the ledges would use (there are none here). */
    for (i = 0; i < MG_BULLETS; i++) {
        mg.bullets[i].life = 0;
        ng_sprite_group_init(&mg.bullet_spr[i], (uint16_t)(SLOT_LEDGE + i), 1, 1,
                             (uint16_t)(MG_TOOL_TILE + MG_T_BOLT), PAL_TOOL);
        ng_sprite_group_set_visible(&mg.bullet_spr[i], 0);
        ng_sprite_group_flush(&mg.bullet_spr[i]);
    }
    ng_sprite_group_init(&mg.cannon, SLOT_DECOR, (uint8_t)(MG_POACHDRONE_W / 16u), (uint8_t)(MG_POACHDRONE_H / 16u),
                         mg_poachdrone_tiles[0], PAL_POACHDRONE);
    ng_sprite_group_set_tile_stride(&mg.cannon, (uint16_t)(MG_POACHDRONE_W / 16u));
    mg.cannon_angle = 0;
    mg.bonus_coins = mg.coins;

    mg_music(SOUND_TRACK_G);
    ng_fix_clear_rect(1, ROW_HINT, 38, 10, PAL_TEXT);
    mg_centre(ROW_CARD, "BONUS ROUND", PAL_GOLD);
    mg_centre(ROW_CARD + 2, "DODGE THE CANNON - GRAB THE GOLD", PAL_TEXT);
    mg_centre(ROW_CARD + 4, "SHE CAN TAKE THREE HITS", PAL_SKY);
    mg.hud_dirty = 1;
    playSFX(SOUND_SFX_13);
}

/* A creature reached her: the round ends here, and the reward with it. */
static void NEOGEO_USER mg_bonus_caught(void)
{
    if (mg.bonus_timer || mg.bonus_safe) return;   /* winding down, or safe a moment after a hit */
    if (mg.bonus_life > 1u) {
        mg.bonus_life--;
        mg.bonus_safe = MG_BONUS_SAFE;
        mg.hurt = MG_BONUS_SAFE;                   /* her colours pulse while she is safe */
        mg.shake = 4;
        playSFX(SOUND_SFX_16);
        mg.hud_dirty = 1;
        return;
    }
    mg.bonus_life = 0;
    mg.bonus_timer = MG_BONUS_OUTRO;
    mg.player->vx_fp = 0;
    mg.shake = 8;
    playSFX(SOUND_SFX_16);
    mg_bonus_clear_creatures(1);
    mg_bonus_fire_stop();
    ng_fix_clear_rect(1, ROW_CARD, 38, 3, PAL_TEXT);
    mg_centre(ROW_CARD, "HIT!", PAL_WARN);
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
        mg_hold_input();
        mg_world_step();
        return;
    }

    mg_controls();
    mg_animate_player();
    mg_world_step();
    mg_update_entities();
    mg_bonus_cannon(p);
    if (mg.state != MG_BONUS || mg.bonus_timer) return;

    /* Every coin grabbed here counts toward the reward. */
    mg.bonus_hits = (uint8_t)(mg.coins - mg.bonus_coins);
    if ((mg.state_timer % 60u) == 0u) mg.hud_dirty = 1;
    if (mg.state_timer == MG_BONUS_TIME - MG_BONUS_CARD) ng_fix_clear_rect(1, ROW_CARD, 38, 5, PAL_TEXT);
    if (mg.hud_dirty) {
        mg_update_hud();
        mg.hud_dirty = 0;
    }

    if (--mg.state_timer == 0) {
        /* She came through untouched: that alone is worth points; enough
         * gold on top returns a life. */
        mg_bonus_clear_creatures(1);
        mg_bonus_fire_stop();
        mg.score += 3000u;
        if (mg.bonus_life == MG_BONUS_LIVES) mg.score += 2000u;   /* not a single hit */
        mg.hud_dirty = 1;
        if (mg.bonus_hits >= MG_BONUS_PERFECT && mg.lives < MAX_LIVES) {
            mg.lives++;
            mg_centre(ROW_CARD, "PERFECT! ONE LIFE RETURNED", PAL_GOLD);
            playSFX(SOUND_SFX_12);
        } else {
            mg_centre(ROW_CARD, mg.bonus_life == MG_BONUS_LIVES ? "UNTOUCHED! WELL DONE!" : "YOU MADE IT THROUGH!", PAL_GOLD);
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
            mg_burst((int16_t)(door + (int16_t)ng_rand_range(28u) - 14), (int16_t)(MG_GROUND_Y - 8 - (ng_rand() & 31)),
                     MG_T_SPARK, 1, -3);
        if (t <= MG_WARP_GONE) p->visible = 0;
        /* all white by MG_WARP_WHITE */
        if (t == MG_WARP_GONE - 1)
            ng_palfx_screen_fade_out(NG_PALFX_WHITE, MG_WARP_GONE - MG_WARP_WHITE);
    } else {
        if (t == MG_WARP_SWAP) mg_warp_swap();
        /* and the colour all back by MG_WARP_CLEAR + 1 */
        if (t == MG_WARP_SWAP - 1)
            ng_palfx_screen_fade_in(NG_PALFX_WHITE, MG_WARP_SWAP - 1 - MG_WARP_CLEAR);
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
    if (!ng_sys_is_mvs()) {
        ng_fix_puts(10, ROW_CARD + 8, mg.over_pick == 0 ? ">" : " ", PAL_GOLD);
        ng_fix_puts(12, ROW_CARD + 8, "CONTINUE", mg.over_pick == 0 ? PAL_GOLD : PAL_TEXT);
        ng_fix_puts(23, ROW_CARD + 8, mg.over_pick == 1 ? ">" : " ", PAL_GOLD);
        ng_fix_puts(25, ROW_CARD + 8, "EXIT", mg.over_pick == 1 ? PAL_GOLD : PAL_TEXT);
        mg_centre(ROW_CARD + 10, "SAVE THE EARTH: PRESS A BUTTON", PAL_SKY);
    } else if (read_p1credit() > 0) {
        mg_centre(ROW_CARD + 8, "PRESS START: SAVE THE EARTH", PAL_GOLD);
    } else {
        mg_centre(ROW_CARD + 8, "PLEASE INSERT COIN", PAL_TEXT);
        mg_centre(ROW_CARD + 10, "SAVE THE EARTH", PAL_SKY);
    }
    ng_fix_puts(14, ROW_CARD + 12, "CONTINUES LEFT", PAL_SKY);
    mg_number(29, ROW_CARD + 12, mg.continues, 1, PAL_GOLD);
    mg_number(19, ROW_CARD + 6, (mg.state_timer + 59u) / 60u, 2, PAL_WARN);
}

/*
 * Start pauses the road and resumes it (never the attract demo): the
 * engine's pause (ng_pause) holds the world, mutes the music, and PAUSE
 * shows on the FIX layer while it lasts.
 */
static void NEOGEO_USER mg_pause_toggle(void)
{
    int on = !ng_pause_is_on();
    ng_pause_set(on);
    playSFX(SOUND_SFX_11);
    /* the music rests while she waits, and comes back when play resumes */
    if (!mg.demo) soundSetADPCMBVolume(on ? 0 : mg.music_level);
    if (on) mg_centre(MG_PAUSE_ROW, "PAUSE", PAL_GOLD);
    else ng_fix_clear_rect(1, MG_PAUSE_ROW, 38, 1, PAL_TEXT);
}

/* ------------------------------------------------------------------ */
/*  The healed valley                                                 */
/* ------------------------------------------------------------------ */
/*
 * After the guardian falls and she has her moment, the valley is shown
 * healed: its painting in its clean colours, the fire, sludge and pits gone,
 * and Maiya on a hang glider, sailing over it as the view pans from one
 * end to the other while flowers, leaves and drops of clean water fall
 * from the sky all over it; as they go the valley's colours come up,
 * brighter than before. The people she freed are out on the road,
 * hopping for joy under hearts. At the end the glider opens out into a
 * parachute and brings her down beside the elder, who thanks
 * her and tells her what her work has given back (its stage file's
 * "healed"), with Sunboy's words; then on to the bonus round, if there is
 * one, and the elder's briefing for the next valley. A button moves it on.
 * On the Sky Road she flies it on the eagle's back.
 */
enum { MG_TOUR_SPEED = 6, MG_TOUR_ELDER = 330 };

static NGCharacter *NEOGEO_USER mg_tour_person(uint8_t type, int16_t x)
{
    NGCharacter *c;
    if (mg.tour_folk_n >= 8u || type > 3u) return 0;
    c = mg_character(K_ALLY, x, MG_GROUND_Y, (uint8_t)(PAL_FOLK + type), NG_RENDER_BAND_NPC, type);
    if (c) mg.tour_folk[mg.tour_folk_n++] = c;
    return c;
}

static void NEOGEO_USER mg_tour_begin(void)
{
    const MGLevel *lv = &mg_levels[mg.stage];
    NGCharacter *p = mg.player;
    uint8_t i;

    for (i = 0; i < MG_ENEMIES; i++) {
        if (mg.enemies[i].body) ng_chars_remove(mg.enemies[i].body);
        mg.enemies[i].body = 0;
    }
    if (mg.boss) { ng_chars_remove(mg.boss); mg.boss = 0; }
    if (mg.rescue) { ng_chars_remove(mg.rescue); mg.rescue = 0; }
    for (i = 0; i < MG_NPC_SLOTS; i++) { if (mg.npcs[i]) ng_chars_remove(mg.npcs[i]); mg.npcs[i] = 0; }
    /* (a thorn still in the air at the win would hang there, frozen) */
    for (i = 0; i < MG_SHOTS; i++) { mg.shots[i].life = 0; ng_sprite_group_set_visible(&mg.shots[i].sprite, 0); ng_sprite_group_flush(&mg.shots[i].sprite); }
    for (i = 0; i < MG_ITEMS; i++) { mg.items[i].life = 0; ng_sprite_group_set_visible(&mg.items[i].sprite, 0); ng_sprite_group_flush(&mg.items[i].sprite); }
    for (i = 0; i < MG_HAZARD_BLOCKS; i++) { ng_sprite_group_set_visible(&mg.hazards[i], 0); ng_sprite_group_flush(&mg.hazards[i]); }
    for (i = 0; i < MG_SIGNS; i++) { ng_sprite_group_set_visible(&mg.signs[i], 0); ng_sprite_group_flush(&mg.signs[i]); }
    for (i = 0; i < 16u; i++) { if (i != 10u) { ng_sprite_group_set_visible(&mg.hud[i], 0); ng_sprite_group_flush(&mg.hud[i]); } }
    for (i = 0; i < MG_TRAY_SLOTS; i++) { ng_sprite_group_set_visible(&mg.tray[i], 0); ng_sprite_group_flush(&mg.tray[i]); }
    ng_sprite_group_set_visible(&mg.gate, 0); ng_sprite_group_flush(&mg.gate);
    ng_sprite_group_set_visible(&mg.cage, 0); ng_sprite_group_flush(&mg.cage);
    mg.fx_time = 0;
    mg.boss_active = 0;
    mg.arena_bg = 0;
    mg.cam_y = 0;
    mg_background(lv->background, 1);          /* its own painting, in its clean colours */
    mg_glow_begin();                            /* ...still hazy, for now */
    mg_falls_begin(SLOT_TRAY, MG_FALLS);        /* the shower has the tray's sprites */
    ng_fix_clear();

    /* the people she freed, and the valley's villagers, out on the road */
    mg.tour_folk_n = 0;
    for (i = 0; i < 4u; i++) mg_palette((uint8_t)(PAL_FOLK + i), mg_ally_pal(i));
    for (i = 0; i < 4u; i++) if (lv->rescue_x[i]) mg_tour_person(lv->rescue_type[i], (int16_t)lv->rescue_x[i]);
    for (i = 0; i < MG_NPC_COUNT; i++) if (mg_npcs[mg.stage][i].x) mg_tour_person(mg_npcs[mg.stage][i].type, mg_npcs[mg.stage][i].x);

    /* she takes to the air on her hang glider (on the Sky Road, the eagle) */
    ng_physics_set_gravity(p, 0, 0);
    p->vx_fp = p->vy_fp = 0;
    p->visible = 1;
    mg.veil = 0;
    mg.veil_lit = 0;
    mg_palette(PAL_HERO, mg_hero_normal_pal());
    ng_sprite_group_set_visible(&mg.hud[10], 0);
    ng_sprite_group_flush(&mg.hud[10]);
    mg_palette(PAL_EAGLE, mg_glider_pal);
    ng_sprite_group_init(&mg.glider, SLOT_GLIDER, 6, 3, mg_glider_tiles[0], PAL_EAGLE);
    ng_sprite_group_set_visible(&mg.glider, 0);
    mg.facing = 0;
    mg.tour_x = 0;
    mg.tour_t = 0;
    mg.tour_phase = 0;
    mg.state = MG_TOUR;
    mg_music(SOUND_TRACK_I);
    mg_centre(4, lv->name, PAL_GOLD);
    mg_centre(5, "IS HEALED", PAL_SKY);
    mg.previous_joy = mg_input();
}

static void NEOGEO_USER mg_tour_next(void)
{
    uint8_t next = (uint8_t)(mg.stage + 1u);
    ng_sprite_group_set_visible(&mg.glider, 0);
    ng_sprite_group_flush(&mg.glider);
    mg_falls_end();
    /* Every other valley ends with a bonus round first. */
    if ((mg.stage & 1u) == 1u) mg_bonus_enter(next);
    else mg_interlude(next);
}

static void NEOGEO_USER mg_tour_frame(void)
{
    const MGLevel *lv = &mg_levels[mg.stage];
    NGCharacter *p = mg.player;
    uint16_t joy = mg_input();
    uint16_t pressed = (uint16_t)(joy & (uint16_t)(~mg.previous_joy));
    int16_t end = (int16_t)(lv->width - NG_SCREEN_W);
    uint8_t i;
    mg.previous_joy = joy;
    mg.tour_t++;

    if (mg.tour_phase == 0) {
        /* over the valley, end to end */
        int16_t bob = (int16_t)ng_trig_mul(4, ng_sin((uint8_t)(mg.tick * 3u)));
        mg.tour_x = (int16_t)(mg.tour_x + MG_TOUR_SPEED);
        if (mg.tour_x >= end) mg.tour_x = end;
        ng_char_set_pos(p, (int16_t)(mg.tour_x + 96), (int16_t)((mg.flying ? 130 : 110) + bob));
        /* hanging from the glider's bar, arms up */
        mg_frame(p, (uint8_t)(mg.flying ? MG_F_RIDE : MG_F_LEAP1), 0);
        if (mg.tour_t == 90u) mg_centre(7, mg_healed[mg.stage][0], PAL_TEXT);
        if (mg.tour_t == 170u) mg_centre(8, mg_healed[mg.stage][1], PAL_TEXT);
        /* the sky strews the valley she has healed */
        if ((mg.tick & 1u) == 0u) mg_fall_any();
        if (mg.tour_x >= end && mg.tour_t > 240u) {
            /* the elder waits at the end of the road */
            NGCharacter *elder = mg_tour_person(0, (int16_t)(end + 224));
            if (elder) mg_frame(elder, 0, 1);
            mg.tour_phase = 1;
            mg.tour_t = 0;
            ng_fix_clear_rect(1, 4, 38, 6, PAL_TEXT);
        }
    } else {
        /* the glider opens into a parachute; she drifts down, swaying,
         * beside him, and he speaks */
        int16_t floor = (int16_t)(mg.flying ? 150 : MG_GROUND_Y);
        int16_t tx = (int16_t)(end + 150);
        if (p->x < tx) ng_char_set_pos(p, (int16_t)(p->x + 1), p->y);
        if (p->y < floor) ng_char_set_pos(p, p->x, (int16_t)(p->y + 1));
        if ((mg.tick & 7u) == 0u) mg_fall_any();     /* the last of the shower */
        if (mg.flying) mg_frame(p, MG_F_RIDE, 0);
        else mg_frame(p, (uint8_t)(p->y < floor ? MG_F_LEAP1 : (mg.tour_t < 200u ? MG_F_IDLE0 : MG_F_WIN)), 0);
        if (mg.tour_t == 70u) {
            mg_centre(ROW_CARD - 3, "ELDER: WELL DONE, MAIYA!", PAL_GOLD);
            mg_centre(ROW_CARD - 1, "YOUR COURAGE HAS HEALED THIS VALLEY", PAL_SKY);
            mg_voice(MG_VOICE_ELDER);
        }
        if (mg.tour_t == 150u) {
            mg_centre(ROW_CARD + 1, mg_sunboy_line[mg.stage][0], PAL_TEXT);
            mg_centre(ROW_CARD + 2, mg_sunboy_line[mg.stage][1], PAL_TEXT);
        }
        if (mg.tour_t >= MG_TOUR_ELDER || (mg.tour_t > 90u && (pressed & (BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_D)))) {
            mg_tour_next();
            return;
        }
    }
    /* the glider over her hands (her grip at its bar), the parachute as she
     * comes down, and nothing once she is on the road */
    if (!mg.flying && p->visible && (mg.tour_phase == 0 || p->y < (int16_t)MG_GROUND_Y)) {
        int16_t sway = (int16_t)(mg.tour_phase ? ng_trig_mul(3, ng_sin((uint8_t)(mg.tick * 4u))) : 0);
        ng_sprite_group_set_tile_base(&mg.glider, mg_glider_tiles[mg.tour_phase ? 1 : 0]);
        ng_sprite_group_set_pos(&mg.glider, (int16_t)(p->x - mg.camera.x - 48 + sway), MG_SY(p->y - 104));
        ng_sprite_group_set_visible(&mg.glider, 1);
    } else {
        ng_sprite_group_set_visible(&mg.glider, 0);
    }
    ng_sprite_group_flush(&mg.glider);
    /* the people hop for joy, hearts rising over them */
    for (i = 0; i < mg.tour_folk_n; i++) {
        NGCharacter *c = mg.tour_folk[i];
        uint8_t hop = (uint8_t)(((mg.tick + i * 19u) & 31u) < 6u);
        int16_t sx = (int16_t)(c->x - mg.camera.x);
        mg_frame(c, (uint8_t)((mg.tick / 12u + i) & 1u), (uint8_t)(c->x > p->x));
        ng_char_set_pos(c, c->x, (int16_t)(MG_GROUND_Y - (hop ? 4 : 0)));
        if (sx > 0 && sx < 320 && ((mg.tick + i * 13u) % 40u) == 0u)
            mg_burst(c->x, (int16_t)(MG_GROUND_Y - 52), MG_T_HEART, 1, -1);
    }
    /* each piece gone brings the colours up; with the elder, all the way */
    mg_glow_step((uint8_t)(mg.tour_phase ? 16u : mg_falls_gone >> 2));
    mg_world_step();
}

/* ------------------------------------------------------------------ */
/*  High scores: the name entry and the table                         */
/* ------------------------------------------------------------------ */
/*
 * A session that ends with a score good enough for the table asks for
 * three letters -- up and down change the letter, A (or right) takes it, B
 * (or left) goes back; thirty seconds and it takes what is there -- and
 * then shows the table, the new line in gold, before the attract returns.
 * The table lives in the save (backup RAM on a cabinet).
 */
static const char mg_name_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.! ";
enum { MG_NAME_CHARS = sizeof(mg_name_chars) - 1, MG_NAME_TIME = 1800, MG_TABLE_TIME = 360 };

static void NEOGEO_USER mg_show_scores(void)
{
    MGSave *sv = mg_saved();
    uint8_t i;
    ng_fix_clear();
    mg_centre(4, "THE BEST NATURE GIRLS", PAL_GOLD);
    ng_fix_puts(8, 7, "NAME", PAL_SKY);
    ng_fix_puts(16, 7, "MISSION", PAL_SKY);
    ng_fix_puts(26, 7, "SCORE", PAL_SKY);
    for (i = 0; i < MG_SCORES; i++) {
        uint8_t row = (uint8_t)(9u + i * 2u), pal = (uint8_t)(i == mg.name_row ? PAL_GOLD : PAL_TEXT);
        mg_number(4, row, (uint32_t)(i + 1u), 2, pal);
        ng_fix_putc(9, row, sv->top[i].name[0], pal);
        ng_fix_putc(10, row, sv->top[i].name[1], pal);
        ng_fix_putc(11, row, sv->top[i].name[2], pal);
        mg_number(19, row, sv->top[i].stage, 2, pal);
        mg_number(26, row, sv->top[i].score, 7, pal);
    }
    mg.state = MG_TABLE;
    mg.state_timer = MG_TABLE_TIME;
}

static void NEOGEO_USER mg_name_draw(void)
{
    uint8_t i;
    for (i = 0; i < 3u; i++) {
        ng_fix_putc((uint8_t)(18u + i * 2u), 14, mg.name_buf[i], i == mg.name_pos ? PAL_GOLD : PAL_TEXT);
        ng_fix_putc((uint8_t)(18u + i * 2u), 15, i == mg.name_pos ? '^' : ' ', PAL_SKY);
    }
}

/* The session is over: to the name entry if the score makes the table,
 * otherwise straight back to the attract. */
static void NEOGEO_USER mg_session_end(void)
{
    MGSave *sv = mg_saved();
    maiya_save_check();
    mg.name_row = 0xFFu;
    if (mg.demo || mg.score <= sv->top[MG_SCORES - 1].score) {
        mg.session_over = 1;
        mg.state = MG_DONE;
        mg.state_timer = 0;
        return;
    }
    ng_sprite_hide_all();
    ng_fix_clear();
    mg_centre(6, "A NEW HIGH SCORE!", PAL_GOLD);
    mg_number(16, 8, mg.score, 7, PAL_TEXT);
    mg_centre(11, "ENTER YOUR NAME", PAL_SKY);
    mg_centre(20, "UP, DOWN: LETTER   A: NEXT   B: BACK", PAL_TEXT);
    mg.name_buf[0] = 'A'; mg.name_buf[1] = 'A'; mg.name_buf[2] = 'A';
    mg.name_pos = 0;
    mg_name_draw();
    mg.state = MG_NAME;
    mg.state_timer = MG_NAME_TIME;
    mg.previous_joy = mg_input();
    mg_music(SOUND_TRACK_G);
}

static void NEOGEO_USER mg_name_commit(void)
{
    MGSave *sv = mg_saved();
    uint8_t i, at = 0;
    while (at < MG_SCORES && sv->top[at].score >= mg.score) at++;
    if (at < MG_SCORES) {
        for (i = MG_SCORES - 1; i > at; i--) sv->top[i] = sv->top[i - 1];
        sv->top[at].name[0] = mg.name_buf[0];
        sv->top[at].name[1] = mg.name_buf[1];
        sv->top[at].name[2] = mg.name_buf[2];
        sv->top[at].stage = (uint8_t)(mg.stage + 1u);
        sv->top[at].score = mg.score;
        ng_save_commit();
    }
    mg.name_row = at;
    playSFX(SOUND_SFX_13);
    mg_show_scores();
}

static void NEOGEO_USER mg_name_frame(void)
{
    uint16_t joy = mg_input();
    uint16_t pressed = (uint16_t)(joy & (uint16_t)(~mg.previous_joy));
    uint8_t c = 0;
    mg.previous_joy = joy;

    while (c < MG_NAME_CHARS && mg_name_chars[c] != (char)mg.name_buf[mg.name_pos]) c++;
    if (pressed & JOY_UP) { c = (uint8_t)(c + 1u >= MG_NAME_CHARS ? 0 : c + 1u); playSFX(SOUND_SFX_11); }
    if (pressed & JOY_DOWN) { c = (uint8_t)(c ? c - 1u : MG_NAME_CHARS - 1u); playSFX(SOUND_SFX_11); }
    mg.name_buf[mg.name_pos] = (uint8_t)mg_name_chars[c < MG_NAME_CHARS ? c : 0];
    if ((pressed & (BUTTON_B | JOY_LEFT)) && mg.name_pos) mg.name_pos--;
    if (pressed & (BUTTON_A | JOY_RIGHT)) {
        playSFX(SOUND_SFX_15);
        if (++mg.name_pos >= 3u) { mg.name_pos = 2; mg_name_commit(); return; }
    }
    mg_name_draw();
    if ((mg.state_timer % 60u) == 0u) mg_number(34, 3, (uint32_t)(mg.state_timer / 60u), 2, PAL_WARN);
    if (--mg.state_timer == 0) mg_name_commit();
}

/* ------------------------------------------------------------------ */
/*  The ending                                                        */
/* ------------------------------------------------------------------ */
/*
 * After the last guardian: the smog lifts; then each valley she walked,
 * healed, passes by in its own restored colours with its name; then the
 * two heroines, and the credits. A button moves a page on (after its first
 * second). The journey is counted in the save as one more clear.
 */
enum { MG_END_OPEN = 0, MG_END_VALLEYS = MG_LEVEL_COUNT - 1 };

static void NEOGEO_USER mg_ending_page(void)
{
    uint8_t page = mg.end_page;
    ng_fix_clear_rect(1, ROW_CARD - 2, 38, 16, PAL_TEXT);
    ng_fix_clear_rect(1, ROW_HINT, 38, 1, PAL_TEXT);
    mg.end_timer = 0;
    if (page == MG_END_OPEN) {
        mg_centre(ROW_CARD + 2, "LORD SMOGGAR IS GONE", PAL_GOLD);
        mg_centre(ROW_CARD + 5, "THE SMOG LIFTS FROM EVERY VALLEY", PAL_SKY);
    } else if (page <= MG_END_VALLEYS) {
        const MGLevel *lv = &mg_levels[page - 1u];
        mg_background(lv->background, 1);
        mg_centre(ROW_CARD + 2, lv->name, PAL_GOLD);
        mg_centre(ROW_CARD + 4, "IS HEALED", PAL_SKY);
    } else if (page == MG_END_VALLEYS + 1u) {
        mg_background(mg_levels[MG_LEVEL_COUNT - 1u].background, 1);
        mg_centre(ROW_CARD + 1, "MAIYA AND LUNA", PAL_GOLD);
        mg_centre(ROW_CARD + 3, "SUPER NATURE GIRLS", PAL_SKY);
        mg_centre(ROW_CARD + 6, "THE VALLEYS WILL REMEMBER YOU", PAL_TEXT);
        mg_voice_later(MG_VOICE_SUNBOY, 20);
    } else {
        mg_centre(ROW_CARD - 1, "A GAME BY EAGLE SOFTWARE", PAL_GOLD);
        mg_centre(ROW_CARD + 2, "MUSIC AND SOUND", PAL_SKY);
        mg_centre(ROW_CARD + 3, "JUHANI JUNKALA", PAL_TEXT);
        mg_centre(ROW_CARD + 5, "VOICES", PAL_SKY);
        mg_centre(ROW_CARD + 6, "THE LJ SPEECH RECORDINGS", PAL_TEXT);
        mg_centre(ROW_CARD + 9, "THANK YOU FOR PLAYING", PAL_GOLD);
        mg_centre(ROW_CARD + 12, "THE END", PAL_WARN);
    }
}

static void NEOGEO_USER mg_ending_begin(void)
{
    uint8_t i;
    mg.state = MG_ENDING;
    mg.end_page = MG_END_OPEN;
    for (i = 0; i < MG_ENEMIES; i++) {
        if (mg.enemies[i].body) ng_chars_remove(mg.enemies[i].body);
        mg.enemies[i].body = 0;
    }
    if (mg.boss) { ng_chars_remove(mg.boss); mg.boss = 0; }
    if (mg.rescue) { ng_chars_remove(mg.rescue); mg.rescue = 0; }
    for (i = 0; i < MG_SHOTS; i++) mg.shots[i].life = 0;
    mg.boss_active = 0;
    mg.player->visible = 0;
    ng_physics_set_gravity(mg.player, 0, 0);
    mg.player->vy_fp = 0;
    ng_char_set_pos(mg.player, 200, 100);
    for (i = 0; i < 16u; i++) { ng_sprite_group_set_visible(&mg.hud[i], 0); ng_sprite_group_flush(&mg.hud[i]); }
    for (i = 0; i < MG_TRAY_SLOTS; i++) { ng_sprite_group_set_visible(&mg.tray[i], 0); ng_sprite_group_flush(&mg.tray[i]); }
    /* the road's own things go too: only the valleys themselves pass by */
    for (i = 0; i < MG_LEDGE_BLOCKS; i++) { ng_sprite_group_set_visible(&mg.ledges[i], 0); ng_sprite_group_flush(&mg.ledges[i]); }
    for (i = 0; i < MG_HAZARD_BLOCKS; i++) { ng_sprite_group_set_visible(&mg.hazards[i], 0); ng_sprite_group_flush(&mg.hazards[i]); }
    for (i = 0; i < MG_SIGNS; i++) { ng_sprite_group_set_visible(&mg.signs[i], 0); ng_sprite_group_flush(&mg.signs[i]); }
    for (i = 0; i < MG_DECOR_SLOTS; i++) { ng_sprite_group_set_visible(&mg.decor[i], 0); ng_sprite_group_flush(&mg.decor[i]); }
    for (i = 0; i < MG_VINE_COUNT; i++) { ng_sprite_group_set_visible(&mg.vines[i], 0); ng_sprite_group_flush(&mg.vines[i]); }
    for (i = 0; i < MG_FRONT_SLOTS; i++) { ng_sprite_group_set_visible(&mg.front[i], 0); ng_sprite_group_flush(&mg.front[i]); }
    for (i = 0; i < MG_ITEMS; i++) { mg.items[i].life = 0; ng_sprite_group_set_visible(&mg.items[i].sprite, 0); ng_sprite_group_flush(&mg.items[i].sprite); }
    ng_sprite_group_set_visible(&mg.gate, 0); ng_sprite_group_flush(&mg.gate);
    ng_sprite_group_set_visible(&mg.cage, 0); ng_sprite_group_flush(&mg.cage);
    mg.fx_time = 0;
    ng_fix_clear();
    mg_music(SOUND_TRACK_I);
    mg_voice_later(MG_VOICE_WIN, 30);
    if (!mg.demo) {
        MGSave *sv = mg_saved();
        maiya_save_check();
        if (sv->clears < 255u) sv->clears++;
        ng_save_commit();
    }
    mg_ending_page();
}

static void NEOGEO_USER mg_ending_frame(void)
{
    uint16_t joy = mg_input();
    uint16_t pressed = (uint16_t)(joy & (uint16_t)(~mg.previous_joy));
    uint16_t length = mg.end_page == MG_END_OPEN ? 240u
                    : (mg.end_page <= MG_END_VALLEYS ? 150u : (mg.end_page == MG_END_VALLEYS + 1u ? 300u : 600u));
    mg.previous_joy = joy;

    /* the view drifts along the healed valley behind the words */
    mg.player->visible = 0;
    mg.player->vx_fp = 0;
    mg.player->vy_fp = 0;
    ng_char_set_pos(mg.player, (int16_t)(mg.player->x + 1), 100);
    mg_world_step();

    if (++mg.end_timer >= length ||
        (mg.end_timer > 60u && (pressed & (BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_D)))) {
        if (mg.end_page >= MG_END_VALLEYS + 2u) {
            mg_session_end();
            return;
        }
        mg.end_page++;
        mg_ending_page();
    }
}

void NEOGEO_USER maiya_frame(void)
{
    if (mg.player && !mg.demo && (mg.state == MG_PLAY || mg.state == MG_BONUS) &&
        (NEO_REGISTER8(BIOS_STATCHANGE) & 0x01u))   /* P1 Start, just pressed */
        mg_pause_toggle();
    /* Paused: nothing of hers moves either -- creatures, sparks, the water,
     * the clock, the camera -- the stick is only read, so no press is left
     * over for the first frame back. */
    if (ng_pause_is_on()) {
        mg.previous_joy = mg_input();
        return;
    }

    mg.tick++;
    if (mg.voice_delay && --mg.voice_delay == 0) mg_voice(mg.voice_next);
    mg_music_tick();
    if (!mg.player) return;
    ng_game_time_stage_run((uint8_t)(mg.state == MG_PLAY));
    if (!mg.demo && (mg.state == MG_PLAY || mg.state == MG_INTRO || mg.state == MG_CLEAR ||
                     mg.state == MG_DEAD || mg.state == MG_WARP || mg.state == MG_BOSS_INTRO))
        mg_draw_stage_time();
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
        /* Hitstop: creatures, villagers, allies, the guardian and every
         * timer hold still (their logic is skipped, not their drawing). */
        if (ng_feedback_is_hitstop()) {
            mg_hold_input();
            mg_world_step();
            return;
        }
        mg_controls();
        if (!mg.vault) mg_spawn();       /* the road waits while she's in the vault */
        mg_animate_player();
        mg_world_step();
        mg_update_entities();
        mg_hideout_step();
        mg_hazard_check();
        mg_hazard_warn_check();
        mg_npc_check();
        mg_rescue_check();

        mg_clock_tick();
        mg_combo_tick();
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
                /* The guardian's clear bonus often crosses the line: then
                 * it's the last line of the clear card, not a hint crowded
                 * in above it. */
                if (mg.clear_bonus) mg_centre(ROW_CARD + 10, "EXTRA LIFE!", PAL_GOLD);
                else mg_hint("EXTRA LIFE!", PAL_GOLD, 120);
            }
        }
        if ((mg.swift || mg.might || mg.veil || mg.spring || mg.crown || mg.lily) &&
            (mg.tick % 60u) == 0u) mg_draw_tray();
        return;
    }

    if (mg.state == MG_CLEAR) {
        if (ng_feedback_is_hitstop()) {   /* the last blow's hold */
            mg_world_step();
            return;
        }
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
        if (mg.flying) {
            /* On the wing: she and the eagle ride the sun, the guardian
             * falls out of the sky. */
            p->vy_fp = (int16_t)(((mg.state_timer >> 4) & 1u) ? 96 : -96);
            mg_frame(p, MG_F_RIDE, 0);
            if (mg.eagle) mg_frame(mg.eagle, (uint8_t)((mg.tick >> 3) % MG_EAGLE_FRAMES), 0);
            if (mg.boss && !mg.boss_down) ng_physics_set_gravity(mg.boss, 48, 6 * NG_FP_ONE);
            if (mg.boss && mg.boss->data0 == MG_B_AIRSHIP && (mg.state_timer & 7u) == 0u)
                mg_burst((int16_t)(mg.boss->x - 100 + (int16_t)ng_rand_range(200u)),
                         (int16_t)(mg.boss->y - 20 - (int16_t)ng_rand_range(50u)), MG_T_FIRE, 2, -1);
        } else if (mg.win_step == 0) {
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
                mg_burst((int16_t)(p->x + (int16_t)ng_rand_range(40u) - 20), (int16_t)(p->y - 64),
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
                mg_tour_begin();       /* the healed valley, then on (mg_tour_next) */
            } else {
                mg_ending_begin();
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
                                    MG_SY(p->y - 72 + ((mg.tick >> 3) & 1)));
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
                mg.state_timer = 180;        /* then the name entry, if it made the table */
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
        if (mg.flying) mg.player->vy_fp = 0;
        mg_frame(mg.player, (uint8_t)(mg.flying ? MG_F_RIDE : MG_F_IDLE0), mg.facing);
        if (mg.boss && mg.boss->data0 == MG_B_AIRSHIP) {
            mg_ship_approach(mg.boss);       /* it comes in out of the distance as it speaks */
        } else if (mg.boss) {
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
                mg_ending_begin();
            }
        }
        return;
    }

    if (mg.state == MG_OVER) {
        uint16_t joy = mg_input();
        uint16_t pressed = (uint16_t)(joy & (uint16_t)(~mg.previous_joy));
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

        if (!ng_sys_is_mvs()) {
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
        } else {
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
        }

        if (go) {
            mg.continues--;
            mg.lives = mg_dip_lives();
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
            mg.state = MG_DONE;
            mg.state_timer = 180;
            ng_fix_clear_rect(1, ROW_CARD, 38, 13, PAL_TEXT);
            mg_centre(ROW_CARD + 4, "GAME OVER", PAL_WARN);
            mg_centre(ROW_CARD + 6, "THE EARTH STILL WAITS FOR YOU", PAL_SKY);
        }
        return;
    }

    if (mg.state == MG_ENDING) {
        mg_ending_frame();
        return;
    }
    if (mg.state == MG_TOUR) {
        mg_tour_frame();
        return;
    }
    if (mg.state == MG_NAME) {
        mg_name_frame();
        return;
    }
    if (mg.state == MG_TABLE) {
        if (--mg.state_timer == 0) mg.session_over = 1;
        return;
    }
    if (mg.state == MG_DONE) {
        mg.player->vx_fp = 0;
        if (mg.state_timer && --mg.state_timer == 0) mg_session_end();
        else if (!mg.state_timer && !mg.session_over) mg_session_end();
    }
}
