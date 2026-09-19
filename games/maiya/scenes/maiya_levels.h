#ifndef MAIYA_LEVELS_H
#define MAIYA_LEVELS_H

#include <stdint.h>

#define MG_LEVEL_COUNT     7
#define MG_PLATFORM_COUNT  8
#define MG_ENCOUNTER_COUNT 12
#define MG_ARCHER_COUNT    3
#define MG_HAZARD_COUNT    4
#define MG_SECRET_COUNT    2

/* Enemy kinds an encounter can raise. */
#define MG_E_SLIME   0   /* hops, squirts toxic sludge                    */
#define MG_E_BEETLE  1   /* armored, charges along ground                 */
#define MG_E_CROW    2   /* swoops down from ledges                       */
#define MG_E_GOBLIN  3   /* scavenger patrols, swings club, throws scrap  */
#define MG_E_WORM    4   /* pops out of pipe, spits flame/acid            */
#define MG_E_DRONE   5   /* hovers, tracks player, fires pulse lasers     */
#define MG_E_PAIR    6   /* two enemies at once                           */

/* The 7 Guardians / Bosses of the Blight Syndicate */
#define MG_B_BEETLE    0   /* Chainsaw Beetle (Greenroot Forest)          */
#define MG_B_TOAD      1   /* Sludge Toad (Mudriver Marsh)                */
#define MG_B_VULTURE   2   /* Iron Vulture (Smokestack City)              */
#define MG_B_JACKAL    3   /* Inferno Jackal (Burning Drylands)           */
#define MG_B_OWL       4   /* Blizzard Owl (Frozen Sky Peaks)             */
#define MG_B_LEVIATHAN 5   /* Toxic Leviathan (Dark Sea of Waste)         */
#define MG_B_SMOGGAR   6   /* Lord Smoggar (Black Core Citadel)           */

/* Hazards */
#define MG_H_FIRE   1
#define MG_H_SPIKES 2
#define MG_H_SLUDGE 3

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
    uint8_t background, music, boss_style;
    uint8_t boss_hp;
    MGPlatform platforms[MG_PLATFORM_COUNT];
    MGEncounter encounters[MG_ENCOUNTER_COUNT];
    MGArcher archers[MG_ARCHER_COUNT];
    MGHazard hazards[MG_HAZARD_COUNT];
    uint16_t rescue_x[2];    /* locations of friendly allies to rescue */
    uint8_t  rescue_type[2]; /* 0: Elder, 1: Maiden, 2: Spirit, 3: Sunboy */
    MGSecret secrets[MG_SECRET_COUNT];
} MGLevel;

/*
 * Level lengths match classic 1990s arcade Shinobi levels (1760 - 2880 pixels).
 * Multi-tier routes: player jumps up onto one-way ledges and drops through with
 * Down + A. Every stage ends in a sealed 320-pixel guardian battle arena.
 */
static const MGLevel mg_levels[MG_LEVEL_COUNT] = {
    /* Mission 1: Greenroot Forest — Ancient canopy, blooming flora */
    {"GREENROOT FOREST", "CHAINSAW BEETLE", 1760, 0, 1, MG_B_BEETLE, 16,
     {{360,136,96},{520,104,64},{700,144,80},{880,144,96},{1080,104,80},{1300,144,96},{0,0,0},{0,0,0}},
     {{190,MG_E_SLIME},{340,MG_E_BEETLE},{480,MG_E_SLIME},{640,MG_E_BEETLE},{800,MG_E_CROW},{940,MG_E_SLIME},
      {1100,MG_E_BEETLE},{1260,MG_E_SLIME},{1380,MG_E_PAIR},{0,0},{0,0},{0,0}},
     {{0,0},{0,0},{0,0}},
     {{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
     {520, 1150}, {0, 1}, /* Elder & Maiden */
     {{440, 96, 2}, {1100, 96, 0}}},

    /* Mission 2: Mudriver Marsh — Murky waters, poisonous fog */
    {"MUDRIVER MARSH", "SLUDGE TOAD", 1984, 1, 2, MG_B_TOAD, 18,
     {{180,144,96},{340,104,96},{560,144,64},{760,120,96},{980,144,96},{1160,104,112},{1400,144,80},{1550,120,96}},
     {{260,MG_E_SLIME},{400,MG_E_WORM},{560,MG_E_SLIME},{720,MG_E_CROW},{880,MG_E_WORM},{1000,MG_E_SLIME},
      {1150,MG_E_CROW},{1300,MG_E_WORM},{1450,MG_E_PAIR},{1560,MG_E_SLIME},{0,0},{0,0}},
     {{390,104},{1200,104},{0,0}},
     {{1060,48,MG_H_SLUDGE},{0,0,0},{0,0,0},{0,0,0}},
     {660, 1400}, {2, 1}, /* Spirit & Maiden */
     {{360, 96, 2}, {1180, 96, 1}}},

    /* Mission 3: Smokestack City — Industrial steel girders and furnaces */
    {"SMOKESTACK CITY", "IRON VULTURE", 2176, 2, 3, MG_B_VULTURE, 20,
     {{230,144,96},{470,120,96},{710,144,112},{950,104,80},{1190,144,80},{1420,120,96},{1640,144,96},{0,0,0}},
     {{280,MG_E_GOBLIN},{430,MG_E_DRONE},{600,MG_E_GOBLIN},{760,MG_E_DRONE},{900,MG_E_CROW},{1040,MG_E_GOBLIN},
      {1200,MG_E_DRONE},{1320,MG_E_GOBLIN},{1490,MG_E_PAIR},{1640,MG_E_DRONE},{1740,MG_E_GOBLIN},{0,0}},
     {{500,120},{1450,120},{0,0}},
     {{730,48,MG_H_SPIKES},{1210,48,MG_H_SPIKES},{0,0,0},{0,0,0}},
     {700, 1530}, {0, 2}, /* Elder & Spirit */
     {{490, 112, 0}, {1440, 112, 2}}},

    /* Mission 4: Burning Drylands — Scorched volcanic earth and lava */
    {"BURNING DRYLANDS", "INFERNO JACKAL", 2368, 3, 4, MG_B_JACKAL, 24,
     {{200,144,96},{420,104,112},{700,144,80},{940,104,96},{1200,144,80},{1440,104,112},{1720,144,96},{1920,120,80}},
     {{290,MG_E_BEETLE},{470,MG_E_WORM},{640,MG_E_GOBLIN},{810,MG_E_BEETLE},{970,MG_E_WORM},{1140,MG_E_GOBLIN},
      {1320,MG_E_BEETLE},{1490,MG_E_WORM},{1660,MG_E_PAIR},{1820,MG_E_BEETLE},{1950,MG_E_PAIR},{0,0}},
     {{460,104},{1480,104},{0,0}},
     {{1100,64,MG_H_FIRE},{1600,48,MG_H_FIRE},{0,0,0},{0,0,0}},
     {730, 1710}, {2, 1}, /* Spirit & Maiden */
     {{440, 96, 2}, {1460, 96, 0}}},

    /* Mission 5: Frozen Sky Peaks — Crystalline glacial peaks */
    {"FROZEN SKY PEAKS", "BLIZZARD OWL", 2560, 4, 5, MG_B_OWL, 26,
     {{260,144,96},{550,120,80},{830,144,112},{1110,104,96},{1400,144,96},{1640,120,80},{1880,144,96},{2110,144,64}},
     {{340,MG_E_CROW},{530,MG_E_DRONE},{730,MG_E_CROW},{910,MG_E_DRONE},{1100,MG_E_SLIME},{1290,MG_E_CROW},
      {1460,MG_E_DRONE},{1640,MG_E_PAIR},{1830,MG_E_CROW},{1980,MG_E_DRONE},{2130,MG_E_PAIR},{0,0}},
     {{1150,104},{0,0},{0,0}},
     {{620,32,MG_H_SPIKES},{1000,48,MG_H_SPIKES},{1550,32,MG_H_SPIKES},{2020,32,MG_H_SPIKES}},
     {790, 1820}, {0, 2}, /* Elder & Spirit */
     {{570, 112, 1}, {1660, 112, 2}}},

    /* Mission 6: Dark Sea of Waste — Polluted ocean & oily coral */
    {"DARK SEA OF WASTE", "TOXIC LEVIATHAN", 2688, 1, 6, MG_B_LEVIATHAN, 28,
     {{250,144,80},{550,104,112},{850,144,96},{1160,120,96},{1480,144,80},{1740,104,112},{2010,144,96},{2250,120,64}},
     {{320,MG_E_SLIME},{530,MG_E_WORM},{750,MG_E_DRONE},{980,MG_E_SLIME},{1180,MG_E_WORM},{1400,MG_E_DRONE},
      {1570,MG_E_SLIME},{1770,MG_E_PAIR},{1950,MG_E_DRONE},{2100,MG_E_WORM},{2220,MG_E_PAIR},{2320,MG_E_DRONE}},
     {{590,104},{1780,104},{0,0}},
     {{700,48,MG_H_SLUDGE},{1300,64,MG_H_SLUDGE},{0,0,0},{0,0,0}},
     {820, 1930}, {2, 1}, /* Spirit & Maiden */
     {{570, 96, 0}, {1760, 96, 2}}},

    /* Mission 7: Black Core Citadel — Final fortress of the Blight Syndicate */
    {"BLACK CORE CITADEL", "LORD SMOGGAR", 2880, 5, 7, MG_B_SMOGGAR, 32,
     {{240,144,96},{570,104,112},{900,144,96},{1230,104,112},{1570,144,96},{1870,104,112},{2170,144,96},{2450,120,64}},
     {{340,MG_E_DRONE},{570,MG_E_GOBLIN},{800,MG_E_BEETLE},{1030,MG_E_DRONE},{1270,MG_E_PAIR},{1490,MG_E_GOBLIN},
      {1720,MG_E_BEETLE},{1940,MG_E_DRONE},{2150,MG_E_PAIR},{2310,MG_E_GOBLIN},{2420,MG_E_PAIR},{2520,MG_E_PAIR}},
     {{610,104},{1270,104},{1910,104}},
     {{1400,32,MG_H_FIRE},{2000,48,MG_H_SPIKES},{0,0,0},{0,0,0}},
     {960, 2160}, {0, 3}, /* Elder & Sunboy! */
     {{590, 96, 2}, {1890, 96, 0}}},
};

#endif
