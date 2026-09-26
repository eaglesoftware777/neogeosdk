#ifndef MAIYA_LEVELS_H
#define MAIYA_LEVELS_H

#include <stdint.h>

/*
 * The stages themselves are authored in games/maiya/levels/, one JSON file
 * a stage; games/maiya/tools/levels.py checks them and writes
 * maiya_levels_data.h, whose initialisers the tables below are built from
 * (and MG_LEVEL_COUNT). These are the tables' sizes.
 */
#define MG_PLATFORM_COUNT  16
#define MG_ENCOUNTER_COUNT 24
#define MG_ARCHER_COUNT    6
#define MG_HAZARD_COUNT    8
#define MG_SECRET_COUNT    6
#define MG_RESCUE_COUNT    4
#define MG_DECOR_COUNT     13
#define MG_VINE_COUNT      3
#define MG_NPC_COUNT       3
#define MG_PICK_COUNT      10

#include "maiya_levels_data.h"

/* Enemy kinds an encounter can raise. */
#define MG_E_SLIME   0   /* hops, squirts toxic sludge                    */
#define MG_E_BEETLE  1   /* armored, charges along ground                 */
#define MG_E_CROW    2   /* swoops down from ledges                       */
#define MG_E_GOBLIN  3   /* scavenger patrols, swings club, throws scrap  */
#define MG_E_WORM    4   /* pops out of pipe, spits flame/acid            */
#define MG_E_DRONE   5   /* hovers, tracks player, fires pulse lasers     */
#define MG_E_PAIR    6   /* two enemies at once                           */
#define MG_E_JELLYFISH 7 /* drifts on a slow current, never charges her   */
#define MG_E_TOXICCRAB 8 /* scuttles sideways, sudden charge               */
#define MG_E_ACIDMOTH  9 /* wide sine-wave flight, never charges          */
#define MG_E_SEWERRAT  10 /* patrols and swings, blocks low attacks       */
#define MG_E_SMOGBAT   11 /* swoops in fast from off-screen                */
#define MG_E_POACHDRONE 12 /* hovers on patrol, fires a capture net        */
#define MG_E_CHEMFLY   13 /* small and fast, drifts in tight loops         */
#define MG_E_PLASTICBAT 14 /* swoops and dive-bombs                        */
#define MG_E_SLAGGOLEM 15 /* slow, armoured, takes a real beating to fell  */
#define MG_E_VINESTING 16 /* rooted in place, lashes anything that nears   */
#define MG_E_SPOREGOB  17 /* hops like the others, spits a slow toxic puff */
#define MG_E_WRAITH    18 /* smog wraith: comes for her when time runs short */

/* Guardians of the Blight Syndicate. */
#define MG_B_BEETLE    0   /* Chainsaw Beetle (Emerald Forest)            */
#define MG_B_TOAD      1   /* Sludge Toad (Sacred Falls)                  */
#define MG_B_LEVIATHAN 2   /* Toxic Leviathan (Coral Coast)               */
#define MG_B_JACKAL    3   /* Inferno Jackal (Golden Autumn Grove)        */
#define MG_B_OWL       4   /* Blizzard Owl (Crystal Grotto)               */
#define MG_B_SMOGGAR   5   /* Lord Smoggar (Ancient World Tree)           */
#define MG_B_VULTURE   6   /* Iron Vulture (Rio Negro Works, first form)  */
#define MG_B_EEL       7   /* Reef Eel, the Leviathan's body (Sunken Reef)*/
#define MG_B_WYRM      8   /* Cave Wyrm, the Toad's body (Silver Cave)    */
#define MG_B_HYENA     9   /* Blight Hyena, the Jackal's body (Savanna)   */

/* Hazards */
#define MG_H_FIRE   1
#define MG_H_SPIKES 2
#define MG_H_SLUDGE 3
/* Left behind in the last two valleys; unlike the other three, this one
 * can be shut off for good -- see mg_hazard_disable_check(). */
#define MG_H_TOXIC  4
#define MG_H_PIT    5   /* a break in the road: falling in costs the life */

#define MG_GROUND_Y 192

typedef struct {
    int16_t x, y, width;
} MGPlatform;

typedef struct {
    int16_t x;
    uint8_t type;
} MGEncounter;

typedef struct {
    int16_t x, y;            /* a thrower posted on a ledge; y is ledge top */
} MGArcher;

typedef struct {
    int16_t x, width;
    uint8_t type;
} MGHazard;

typedef struct {
    int16_t x, y;
    uint8_t type;            /* 0: golden rose, 1: sun gem, 2: secret chest */
} MGSecret;

typedef struct {
    const char *name;
    const char *guardian;
    uint16_t width;
    uint8_t background, music, boss_style;   /* music: ADPCM-B track, 1.wav = 0 */
    uint8_t boss_hp;
    uint16_t gate_x;         /* Ancient Nature Gate location blocking boss arena */
    MGPlatform platforms[MG_PLATFORM_COUNT];
    MGEncounter encounters[MG_ENCOUNTER_COUNT];
    MGArcher archers[MG_ARCHER_COUNT];
    MGHazard hazards[MG_HAZARD_COUNT];
    uint16_t rescue_x[MG_RESCUE_COUNT];    /* locations of friendly allies to rescue */
    uint8_t  rescue_type[MG_RESCUE_COUNT]; /* 0: Elder, 1: Maiden, 2: Spirit, 3: Sunboy */
    MGSecret secrets[MG_SECRET_COUNT];
} MGLevel;

/*
 * Authentic Arcade Adventure Level Design (3840 - 4800 pixels long).
 * High verticality with multi-tier nature ledges, drop-through platforming (Down + A),
 * hidden secrets, friendly rescue NPCs, and an Ancient Nature Gate requiring the
 * Golden Sun Key before entering the Guardian battle arena (2 - 4 minutes of gameplay).
 */
static const MGLevel mg_levels[MG_LEVEL_COUNT] = MG_LEVELS_TABLE;

/* ---------------------------------------------------------------- */
/*  Scenery, villagers, climbing vines and the Sun Key               */
/* ---------------------------------------------------------------- */
typedef struct {
    int16_t x;
    uint8_t y, kind;         /* kind indexes the MG_D_* decoration set */
} MGDecor;

typedef struct {
    int16_t x;               /* left edge of a climbable vine column   */
    uint8_t top, bottom;     /* the shelf it reaches and the road below */
} MGVine;

typedef struct {
    int16_t x;
    uint8_t type;            /* 0: Elder, 1: Valley Girl, 2: Spirit, 3: Sunboy */
    const char *line;
} MGNpc;

static const MGDecor mg_decor[MG_LEVEL_COUNT][MG_DECOR_COUNT] = MG_DECOR_TABLE;

static const MGVine mg_vines[MG_LEVEL_COUNT][MG_VINE_COUNT] = MG_VINES_TABLE;

static const MGNpc mg_npcs[MG_LEVEL_COUNT][MG_NPC_COUNT] = MG_NPCS_TABLE;

/* Where the Golden Sun Key waits: always on a high shelf, never on the road. */
static const uint16_t mg_key_pos[MG_LEVEL_COUNT][2] = MG_KEY_TABLE;

/* ---------------------------------------------------------------- */
/*  Secret Arts, elder lore and what Sunboy says between missions    */
/* ---------------------------------------------------------------- */

/* The Secret Art Maiya carries in each valley: the roses she gathers
 * charge it, and D unleashes it. */
static const char *const mg_art_name[MG_LEVEL_COUNT] = MG_ART_NAME_TABLE;

/* What she calls out as each art breaks over the valley. */
static const char *const mg_art_words[MG_LEVEL_COUNT] = MG_ART_WORDS_TABLE;

/* What the elder's charm whispers about this valley's hidden places. */
static const char *const mg_secret_hint[MG_LEVEL_COUNT] = MG_SECRET_HINT_TABLE;

/* And how the guardian at the end of it can be broken. */
static const char *const mg_boss_hint[MG_LEVEL_COUNT] = MG_BOSS_HINT_TABLE;

/* Sunboy, held in the crystal until Rio Negro Works, cheers her on from
 * freedom for the three valleys after. */
static const char *const mg_sunboy_line[MG_LEVEL_COUNT][2] = MG_SUNBOY_TABLE;

/* Collectibles strung along the road: coins for the count, a flower and a
 * caged forest friend for the mission tally, the elder's charm for a hint,
 * and one extra life kept where only a vine can reach. */
typedef struct {
    int16_t x;
    uint8_t y, kind;         /* kind indexes the MG_K_* trinket set */
} MGPickup;

static const MGPickup mg_picks[MG_LEVEL_COUNT][MG_PICK_COUNT] = MG_PICKS_TABLE;

/* What each guardian says on the arena threshold, and what she says back. */
static const char *const mg_boss_taunt[MG_LEVEL_COUNT] = MG_BOSS_TAUNT_TABLE;

static const char *const mg_boss_reply[MG_LEVEL_COUNT] = MG_BOSS_REPLY_TABLE;

/* Lord Smoggar's true face, once the Iron Vulture falls in Rio Negro Works. */
#define MG_SMOGGAR_TAUNT "YOUR PRINCE FEEDS MY ROOTS NOW, GUARDIAN!"
#define MG_SMOGGAR_REPLY "GIVE ME BACK MY BROTHER, SMOGGAR!"

#endif
