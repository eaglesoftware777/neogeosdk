#ifndef MAIYA_LEVELS_H
#define MAIYA_LEVELS_H

#include <stdint.h>

#define MG_LEVEL_COUNT     10
#define MG_PLATFORM_COUNT  16
#define MG_ENCOUNTER_COUNT 24
#define MG_ARCHER_COUNT    6
#define MG_HAZARD_COUNT    8
#define MG_SECRET_COUNT    6

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
    uint16_t rescue_x[4];    /* locations of friendly allies to rescue */
    uint8_t  rescue_type[4]; /* 0: Elder, 1: Maiden, 2: Spirit, 3: Sunboy */
    MGSecret secrets[MG_SECRET_COUNT];
} MGLevel;

/*
 * Authentic Arcade Adventure Level Design (3840 - 4800 pixels long).
 * High verticality with multi-tier nature ledges, drop-through platforming (Down + A),
 * hidden secrets, friendly rescue NPCs, and an Ancient Nature Gate requiring the
 * Golden Sun Key before entering the Guardian battle arena (2 - 4 minutes of gameplay).
 */
static const MGLevel mg_levels[MG_LEVEL_COUNT] = {
    /* Mission 1: Sunlit Emerald Forest — Ancient towering canopy & blooming flora */
    {"EMERALD FOREST", "CHAINSAW BEETLE", 3840, 0, 1, MG_B_BEETLE, 16, 3200,
     {{320,140,96},{480,104,80},{660,68,96},{860,140,80},{1080,104,96},{1280,68,80},
      {1500,140,96},{1720,104,80},{1940,68,96},{2160,140,96},{2380,104,80},{2580,68,96},
      {2800,140,96},{3000,104,80},{3360,140,96},{3580,104,80}},
     {{180,MG_E_SLIME},{480,MG_E_BEETLE},{780,MG_E_SLIME},{1100,MG_E_BEETLE},{1420,MG_E_SLIME},{1740,MG_E_BEETLE},
      {2060,MG_E_SLIME},{2380,MG_E_BEETLE},{2700,MG_E_SLIME},{3020,MG_E_BEETLE},{3380,MG_E_SLIME},{3620,MG_E_BEETLE},
      {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
      {0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
     {{510,104},{1310,68},{1970,68},{2610,68},{3030,104},{0,0}},
     {{760,48,MG_H_SPIKES},{1600,48,MG_H_SPIKES},{2280,48,MG_H_SPIKES},{2900,48,MG_H_SPIKES},
      {0,0,0},{0,0,0},{0,0,0},{0,0,0}},
     {650, 1650, 2450, 2950}, {0, 1, 2, 3}, /* Elder, Maiden, Spirit, Sunboy */
     {{510, 96, 2}, {700, 60, 0}, {1320, 60, 1}, {2000, 60, 2}, {2620, 60, 0}, {3040, 96, 1}}},

    /* Mission 2: Valley of Sacred Falls — Cascading mountain streams & misty crags */
    {"VALLEY OF FALLS", "SLUDGE TOAD", 4096, 1, 2, MG_B_TOAD, 18, 3450,
     {{280,144,96},{460,112,80},{660,76,96},{880,144,96},{1100,108,80},{1320,72,96},
      {1540,144,96},{1760,108,80},{1980,72,96},{2200,144,96},{2420,108,80},{2640,72,96},
      {2860,144,96},{3080,108,80},{3300,72,96},{3650,144,96}},
     {{200,MG_E_SLIME},{360,MG_E_BEETLE},{680,MG_E_GOBLIN},{840,MG_E_SLIME},{1160,MG_E_BEETLE},{1480,MG_E_GOBLIN},
      {1640,MG_E_SLIME},{1960,MG_E_BEETLE},{2120,MG_E_GOBLIN},{2440,MG_E_SLIME},{2760,MG_E_BEETLE},{2920,MG_E_GOBLIN},
      {3240,MG_E_SLIME},{3400,MG_E_BEETLE},{3720,MG_E_GOBLIN},{0,0},{0,0},{0,0},
      {0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
     {{490,112},{1350,72},{2010,72},{2670,72},{3330,72},{0,0}},
     {{780,48,MG_H_SLUDGE},{1440,48,MG_H_SLUDGE},{2100,48,MG_H_SLUDGE},{2780,48,MG_H_SLUDGE},
      {0,0,0},{0,0,0},{0,0,0},{0,0,0}},
     {850, 1700, 2500, 3100}, {2, 1, 0, 3}, /* Spirit, Maiden, Elder, Sunboy */
     {{320, 136, 2}, {700, 68, 1}, {1360, 64, 0}, {2020, 64, 2}, {2680, 64, 1}, {3340, 64, 0}}},

    /* Mission 3: Azure Coral Coast — Turquoise sea waves, coral reefs & golden sand */
    {"AZURE COAST", "TOXIC LEVIATHAN", 4352, 2, 3, MG_B_LEVIATHAN, 20, 3700,
     {{300,144,96},{500,108,80},{720,72,96},{940,144,96},{1160,108,80},{1380,72,96},
      {1600,144,96},{1820,108,80},{2040,72,96},{2260,144,96},{2480,108,80},{2700,72,96},
      {2920,144,96},{3140,108,80},{3380,72,96},{3880,144,96}},
     {{220,MG_E_BEETLE},{380,MG_E_GOBLIN},{540,MG_E_CROW},{860,MG_E_JELLYFISH},{1020,MG_E_BEETLE},{1180,MG_E_GOBLIN},
      {1500,MG_E_CROW},{1660,MG_E_PAIR},{1820,MG_E_BEETLE},{2140,MG_E_GOBLIN},{2300,MG_E_CROW},{2460,MG_E_JELLYFISH},
      {2780,MG_E_BEETLE},{2940,MG_E_GOBLIN},{3100,MG_E_PAIR},{3420,MG_E_JELLYFISH},{3580,MG_E_BEETLE},{3740,MG_E_GOBLIN},
      {0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
     {{530,108},{1410,72},{2070,72},{2730,72},{3410,72},{0,0}},
     {{840,48,MG_H_SPIKES},{1500,48,MG_H_SPIKES},{2160,48,MG_H_SPIKES},{2820,48,MG_H_SPIKES},
      {0,0,0},{0,0,0},{0,0,0},{0,0,0}},
     {900, 1850, 2700, 3350}, {1, 0, 2, 3}, /* Maiden, Elder, Spirit, Sunboy */
     {{340, 136, 0}, {760, 64, 1}, {1420, 64, 2}, {2080, 64, 0}, {2740, 64, 1}, {3420, 64, 2}}},

    /* Mission 4: Golden Autumn Grove — Amber canopy, rustic oaks & woodland hollows */
    {"AUTUMN GROVE", "INFERNO JACKAL", 4480, 3, 4, MG_B_JACKAL, 24, 3840,
     {{300,140,96},{520,104,80},{740,68,96},{960,140,96},{1180,104,80},{1400,68,96},
      {1620,140,96},{1840,104,80},{2060,68,96},{2280,140,96},{2500,104,80},{2720,68,96},
      {2940,140,96},{3160,104,80},{3400,68,96},{4000,140,96}},
     {{240,MG_E_GOBLIN},{400,MG_E_CROW},{560,MG_E_BEETLE},{720,MG_E_WORM},{880,MG_E_GOBLIN},{1200,MG_E_CROW},
      {1360,MG_E_BEETLE},{1520,MG_E_PAIR},{1680,MG_E_GOBLIN},{1840,MG_E_CROW},{2160,MG_E_BEETLE},{2320,MG_E_WORM},
      {2480,MG_E_GOBLIN},{2640,MG_E_CROW},{2800,MG_E_PAIR},{3120,MG_E_WORM},{3280,MG_E_GOBLIN},{3440,MG_E_CROW},
      {3600,MG_E_BEETLE},{3760,MG_E_WORM},{0,0},{0,0},{0,0},{0,0}},
     {{550,104},{1430,68},{2090,68},{2750,68},{3430,68},{0,0}},
     {{860,64,MG_H_FIRE},{1520,64,MG_H_FIRE},{2180,64,MG_H_FIRE},{2840,64,MG_H_FIRE},
      {0,0,0},{0,0,0},{0,0,0},{0,0,0}},
     {1000, 1950, 2750, 3500}, {0, 2, 1, 3}, /* Elder, Spirit, Maiden, Sunboy */
     {{340, 132, 2}, {780, 60, 0}, {1440, 60, 1}, {2100, 60, 2}, {2760, 60, 0}, {3440, 60, 1}}},

    /* Mission 5: Crystal Grotto Springs — Subterranean waters & glowing emerald crystals */
    {"CRYSTAL GROTTO", "BLIZZARD OWL", 4608, 4, 5, MG_B_OWL, 26, 3960,
     {{320,144,96},{540,108,80},{760,72,96},{980,144,96},{1200,108,80},{1420,72,96},
      {1640,144,96},{1860,108,80},{2080,72,96},{2300,144,96},{2520,108,80},{2740,72,96},
      {2960,144,96},{3180,108,80},{3420,72,96},{4120,144,96}},
     {{260,MG_E_CROW},{420,MG_E_WORM},{580,MG_E_DRONE},{740,MG_E_GOBLIN},{900,MG_E_CROW},{1060,MG_E_WORM},
      {1220,MG_E_DRONE},{1380,MG_E_PAIR},{1540,MG_E_CROW},{1700,MG_E_WORM},{1860,MG_E_DRONE},{2180,MG_E_GOBLIN},
      {2340,MG_E_CROW},{2500,MG_E_WORM},{2660,MG_E_PAIR},{2820,MG_E_GOBLIN},{2980,MG_E_CROW},{3140,MG_E_WORM},
      {3300,MG_E_DRONE},{3460,MG_E_GOBLIN},{3620,MG_E_CROW},{3780,MG_E_PAIR},{0,0},{0,0}},
     {{570,108},{1450,72},{2110,72},{2770,72},{3450,72},{0,0}},
     {{880,32,MG_H_SPIKES},{1540,32,MG_H_SPIKES},{2200,32,MG_H_SPIKES},{2860,32,MG_H_SPIKES},
      {3500,32,MG_H_SPIKES},{0,0,0},{0,0,0},{0,0,0}},
     {1100, 2100, 3050, 3650}, {2, 1, 0, 3}, /* Spirit, Maiden, Elder, Sunboy */
     {{360, 136, 1}, {800, 64, 2}, {1460, 64, 0}, {2120, 64, 1}, {2780, 64, 2}, {3460, 64, 0}}},

    /* Mission 6: Sacred World Tree — Great ancient canopy & floating sakura blossoms */
    {"SACRED WORLD TREE", "LORD SMOGGAR", 4800, 5, 3, MG_B_SMOGGAR, 32, 4160,
     {{320,144,96},{560,104,80},{800,64,96},{1040,144,96},{1280,104,80},{1520,64,96},
      {1760,144,96},{2000,104,80},{2240,64,96},{2480,144,96},{2720,104,80},{2960,64,96},
      {3200,144,96},{3440,104,80},{3700,64,96},{4300,144,96}},
     {{280,MG_E_DRONE},{460,MG_E_WORM},{640,MG_E_PAIR},{820,MG_E_CROW},{1000,MG_E_GOBLIN},{1180,MG_E_DRONE},
      {1360,MG_E_WORM},{1540,MG_E_PAIR},{1720,MG_E_CROW},{1900,MG_E_GOBLIN},{2080,MG_E_DRONE},{2260,MG_E_WORM},
      {2440,MG_E_PAIR},{2620,MG_E_CROW},{2800,MG_E_PAIR},{2980,MG_E_DRONE},{3160,MG_E_WORM},{3340,MG_E_PAIR},
      {3520,MG_E_CROW},{3700,MG_E_GOBLIN},{3880,MG_E_DRONE},{4040,MG_E_PAIR},{4200,MG_E_PAIR},{4400,MG_E_CROW}},
     {{590,104},{1310,104},{2030,104},{2750,104},{3470,104},{0,0}},
     {{920,48,MG_H_FIRE},{1640,48,MG_H_SPIKES},{2360,48,MG_H_FIRE},{3080,48,MG_H_SPIKES},
      {3800,48,MG_H_FIRE},{0,0,0},{0,0,0},{0,0,0}},
     {1200, 2200, 3200, 3700}, {0, 1, 2, 3}, /* Elder, Maiden, Spirit, Sunboy */
     {{360, 136, 2}, {840, 56, 0}, {1560, 56, 1}, {2280, 56, 2}, {3000, 56, 0}, {3740, 56, 1}}},

    /* Mission 7: Rio Negro Works -- an abandoned chemical plant deep in the
     * rainforest, still bleeding into the river.  The most poisoned place in
     * the game: every creature the blight has, posted drones on every
     * gantry, sludge and fire along the bank, and two guardians at the end. */
    {"RIO NEGRO WORKS", "IRON VULTURE", 5120, 6, 7, MG_B_VULTURE, 30, 4440,
     {{300,144,96},{520,104,96},{760,64,96},{1000,144,80},{1220,104,96},{1460,64,96},
      {1700,144,96},{1940,104,80},{2180,64,96},{2420,144,96},{2660,104,96},{2900,64,80},
      {3140,144,96},{3380,104,96},{3640,64,96},{4560,144,112}},
     {{240,MG_E_SLAGGOLEM},{400,MG_E_DRONE},{560,MG_E_WORM},{720,MG_E_PAIR},{880,MG_E_SEWERRAT},{1040,MG_E_DRONE},
      {1200,MG_E_WORM},{1360,MG_E_PAIR},{1520,MG_E_SLAGGOLEM},{1680,MG_E_SEWERRAT},{1840,MG_E_DRONE},{2000,MG_E_PAIR},
      {2160,MG_E_WORM},{2320,MG_E_SEWERRAT},{2480,MG_E_PAIR},{2640,MG_E_DRONE},{2800,MG_E_SLAGGOLEM},{2960,MG_E_PAIR},
      {3120,MG_E_WORM},{3280,MG_E_DRONE},{3440,MG_E_PAIR},{3600,MG_E_SEWERRAT},{3760,MG_E_PAIR},{4000,MG_E_PAIR}},
     {{550,104},{1250,104},{1970,104},{2690,104},{3410,104},{4590,144}},
     {{640,64,MG_H_SLUDGE},{1120,48,MG_H_FIRE},{1600,64,MG_H_SLUDGE},{2080,48,MG_H_FIRE},
      {2560,64,MG_H_SLUDGE},{3040,48,MG_H_FIRE},{3520,64,MG_H_SLUDGE},{4000,48,MG_H_SLUDGE}},
     {900, 1900, 2900, 3900}, {2, 1, 0, 3}, /* Spirit, Maiden, Elder, Sunboy */
     {{350, 136, 2}, {800, 56, 0}, {1500, 56, 1}, {2220, 56, 2}, {2940, 56, 0}, {3680, 56, 1}}},

    /* Mission 8: Sunken Reef -- with Sunboy free, she keeps going: the tide
     * pools and coral where the works' runoff still settles. */
    {"SUNKEN REEF", "REEF EEL", 4352, 2, 3, MG_B_EEL, 32, 3700,
     {{300,144,96},{500,108,80},{720,72,96},{940,144,96},{1160,108,80},{1380,72,96},
      {1600,144,96},{1820,108,80},{2040,72,96},{2260,144,96},{2480,108,80},{2700,72,96},
      {2920,144,96},{3140,108,80},{3380,72,96},{3880,144,96}},
     {{220,MG_E_BEETLE},{380,MG_E_GOBLIN},{540,MG_E_DRONE},{860,MG_E_TOXICCRAB},{1020,MG_E_BEETLE},{1180,MG_E_GOBLIN},
      {1500,MG_E_DRONE},{1660,MG_E_PAIR},{1820,MG_E_BEETLE},{2140,MG_E_GOBLIN},{2300,MG_E_DRONE},{2460,MG_E_TOXICCRAB},
      {2780,MG_E_BEETLE},{2940,MG_E_GOBLIN},{3100,MG_E_PAIR},{3420,MG_E_TOXICCRAB},{3580,MG_E_BEETLE},{3740,MG_E_GOBLIN},
      {0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
     {{530,108},{1410,72},{2070,72},{2730,72},{3410,72},{0,0}},
     {{840,48,MG_H_SPIKES},{1500,48,MG_H_SPIKES},{2160,48,MG_H_SPIKES},{2820,48,MG_H_SPIKES},
      {0,0,0},{0,0,0},{0,0,0},{0,0,0}},
     {900, 1850, 2700, 3350}, {1, 0, 2, 3}, /* Maiden, Elder, Spirit, Sunboy */
     {{340, 136, 0}, {760, 64, 1}, {1420, 64, 2}, {2080, 64, 0}, {2740, 64, 1}, {3420, 64, 2}}},

    /* Mission 9: Silver Cave -- a hollow of glowing crystal plants the
     * blight followed underground. */
    {"SILVER CAVE", "CAVE WYRM", 4608, 4, 5, MG_B_WYRM, 34, 3960,
     {{320,144,96},{540,108,80},{760,72,96},{980,144,96},{1200,108,80},{1420,72,96},
      {1640,144,96},{1860,108,80},{2080,72,96},{2300,144,96},{2520,108,80},{2740,72,96},
      {2960,144,96},{3180,108,80},{3420,72,96},{4120,144,96}},
     {{260,MG_E_SMOGBAT},{420,MG_E_WORM},{580,MG_E_DRONE},{740,MG_E_GOBLIN},{900,MG_E_SMOGBAT},{1060,MG_E_WORM},
      {1220,MG_E_DRONE},{1380,MG_E_PAIR},{1540,MG_E_SMOGBAT},{1700,MG_E_WORM},{1860,MG_E_DRONE},{2180,MG_E_GOBLIN},
      {2340,MG_E_SMOGBAT},{2500,MG_E_WORM},{2660,MG_E_PAIR},{2820,MG_E_GOBLIN},{2980,MG_E_SMOGBAT},{3140,MG_E_WORM},
      {3300,MG_E_DRONE},{3460,MG_E_GOBLIN},{3620,MG_E_SMOGBAT},{3780,MG_E_PAIR},{0,0},{0,0}},
     {{570,108},{1450,72},{2110,72},{2770,72},{3450,72},{0,0}},
     {{160,32,MG_H_TOXIC},{880,32,MG_H_SPIKES},{1540,32,MG_H_SPIKES},{2200,32,MG_H_SPIKES},
      {2860,32,MG_H_SPIKES},{3500,32,MG_H_SPIKES},{0,0,0},{0,0,0}},
     {1100, 2100, 3050, 3650}, {2, 1, 0, 3}, /* Spirit, Maiden, Elder, Sunboy */
     {{360, 136, 1}, {800, 64, 2}, {1460, 64, 0}, {2120, 64, 1}, {2780, 64, 2}, {3460, 64, 0}}},

    /* Mission 10: Golden Savanna -- the last valley: open grassland where
     * hunters have moved in on the herds. */
    {"GOLDEN SAVANNA", "BLIGHT HYENA", 4480, 3, 4, MG_B_HYENA, 36, 3840,
     {{300,140,96},{520,104,80},{740,68,96},{960,140,96},{1180,104,80},{1400,68,96},
      {1620,140,96},{1840,104,80},{2060,68,96},{2280,140,96},{2500,104,80},{2720,68,96},
      {2940,140,96},{3160,104,80},{3400,68,96},{4000,140,96}},
     {{240,MG_E_GOBLIN},{400,MG_E_POACHDRONE},{560,MG_E_BEETLE},{720,MG_E_WORM},{880,MG_E_GOBLIN},{1200,MG_E_POACHDRONE},
      {1360,MG_E_BEETLE},{1520,MG_E_PAIR},{1680,MG_E_GOBLIN},{1840,MG_E_POACHDRONE},{2160,MG_E_BEETLE},{2320,MG_E_WORM},
      {2480,MG_E_GOBLIN},{2640,MG_E_POACHDRONE},{2800,MG_E_PAIR},{3120,MG_E_WORM},{3280,MG_E_GOBLIN},{3440,MG_E_POACHDRONE},
      {3600,MG_E_BEETLE},{3760,MG_E_WORM},{0,0},{0,0},{0,0},{0,0}},
     {{550,104},{1430,68},{2090,68},{2750,68},{3430,68},{0,0}},
     {{160,32,MG_H_TOXIC},{860,64,MG_H_FIRE},{1520,64,MG_H_FIRE},{2180,64,MG_H_FIRE},
      {2840,64,MG_H_FIRE},{0,0,0},{0,0,0},{0,0,0}},
     {1000, 1950, 2750, 3500}, {0, 2, 1, 3}, /* Elder, Spirit, Maiden, Sunboy */
     {{340, 132, 2}, {780, 60, 0}, {1440, 60, 1}, {2100, 60, 2}, {2760, 60, 0}, {3440, 60, 1}}},
};

/* ---------------------------------------------------------------- */
/*  Scenery, villagers, climbing vines and the Sun Key               */
/* ---------------------------------------------------------------- */
#define MG_DECOR_COUNT 13
#define MG_VINE_COUNT  3
#define MG_NPC_COUNT   3

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

static const MGDecor mg_decor[MG_LEVEL_COUNT][MG_DECOR_COUNT] = {
    /* rows 1-6 follow; row 7 is the works, at the end of the table */
    {{150,160,MG_D_GRASS}, {487,160,MG_D_FLOWERS}, {825,160,MG_D_BUSH}, {1162,160,MG_D_MUSHROOM}, {1500,160,MG_D_SAPLING}, {1837,160,MG_D_LANTERN}, {2175,160,MG_D_SIGN}, {2512,160,MG_D_ROCK}, {2850,160,MG_D_GRASS}, {504,72,MG_D_MUSHROOM}, {1112,72,MG_D_SAPLING}, {1760,72,MG_D_LANTERN}, {3080,160,MG_D_LANTERN}},  /* EMERALD FOREST */
    {{150,160,MG_D_FALL}, {518,160,MG_D_GRASS}, {887,160,MG_D_LILY}, {1256,160,MG_D_BUSH}, {1625,160,MG_D_LANTERN}, {1993,160,MG_D_ROCK}, {2362,160,MG_D_FLOWERS}, {2731,160,MG_D_SAPLING}, {3100,160,MG_D_FALL}, {484,80,MG_D_BUSH}, {1132,76,MG_D_LANTERN}, {1800,76,MG_D_ROCK}, {3330,160,MG_D_LANTERN}},  /* VALLEY OF FALLS */
    {{150,160,MG_D_ROCK}, {550,160,MG_D_GRASS}, {950,160,MG_D_LILY}, {1350,160,MG_D_SIGN}, {1750,160,MG_D_LANTERN}, {2150,160,MG_D_BUSH}, {2550,160,MG_D_FLOWERS}, {2950,160,MG_D_ROCK}, {3350,160,MG_D_ROCK}, {524,76,MG_D_SIGN}, {1192,76,MG_D_LANTERN}, {1860,76,MG_D_BUSH}, {3580,160,MG_D_LANTERN}},  /* AZURE COAST */
    {{150,160,MG_D_LANTERN}, {567,160,MG_D_FLOWERS}, {985,160,MG_D_SIGN}, {1402,160,MG_D_BUSH}, {1820,160,MG_D_GRASS}, {2237,160,MG_D_SAPLING}, {2655,160,MG_D_MUSHROOM}, {3072,160,MG_D_LANTERN}, {3490,160,MG_D_LANTERN}, {544,72,MG_D_BUSH}, {1212,72,MG_D_GRASS}, {1880,72,MG_D_SAPLING}, {3720,160,MG_D_LANTERN}},  /* AUTUMN GROVE */
    {{150,160,MG_D_ROCK}, {582,160,MG_D_SAPLING}, {1015,160,MG_D_LANTERN}, {1447,160,MG_D_GRASS}, {1880,160,MG_D_SIGN}, {2312,160,MG_D_BUSH}, {2745,160,MG_D_ROCK}, {3177,160,MG_D_FLOWERS}, {3610,160,MG_D_ROCK}, {564,76,MG_D_GRASS}, {1232,76,MG_D_SIGN}, {1900,76,MG_D_BUSH}, {3840,160,MG_D_LANTERN}},  /* CRYSTAL GROTTO */
    {{150,160,MG_D_MUSHROOM}, {607,160,MG_D_BUSH}, {1065,160,MG_D_SAPLING}, {1522,160,MG_D_GRASS}, {1980,160,MG_D_FLOWERS}, {2437,160,MG_D_LILY}, {2895,160,MG_D_LANTERN}, {3352,160,MG_D_MUSHROOM}, {3810,160,MG_D_MUSHROOM}, {584,72,MG_D_GRASS}, {1312,72,MG_D_FLOWERS}, {2040,72,MG_D_LILY}, {4040,160,MG_D_LANTERN}},  /* SACRED WORLD TREE */
    {{160,160,MG_D_MUSHROOM}, {520,160,MG_D_ROCK}, {900,160,MG_D_SAPLING}, {1280,160,MG_D_MUSHROOM}, {1660,160,MG_D_ROCK}, {2040,160,MG_D_FALL}, {2420,160,MG_D_SAPLING}, {2800,160,MG_D_LILY}, {3180,160,MG_D_ROCK}, {540,72,MG_D_MUSHROOM}, {1240,72,MG_D_ROCK}, {2680,72,MG_D_SAPLING}, {4320,160,MG_D_LANTERN}},  /* RIO NEGRO WORKS */
    {{150,160,MG_D_ROCK}, {550,160,MG_D_GRASS}, {950,160,MG_D_LILY}, {1350,160,MG_D_SIGN}, {1750,160,MG_D_LANTERN}, {2150,160,MG_D_BUSH}, {2550,160,MG_D_FLOWERS}, {2950,160,MG_D_ROCK}, {3350,160,MG_D_ROCK}, {524,76,MG_D_SIGN}, {1192,76,MG_D_LANTERN}, {1860,76,MG_D_BUSH}, {3580,160,MG_D_LANTERN}},  /* SUNKEN REEF */
    {{150,160,MG_D_ROCK}, {582,160,MG_D_SAPLING}, {1015,160,MG_D_LANTERN}, {1447,160,MG_D_GRASS}, {1880,160,MG_D_SIGN}, {2312,160,MG_D_BUSH}, {2745,160,MG_D_ROCK}, {3177,160,MG_D_FLOWERS}, {3610,160,MG_D_ROCK}, {564,76,MG_D_GRASS}, {1232,76,MG_D_SIGN}, {1900,76,MG_D_BUSH}, {3840,160,MG_D_LANTERN}},  /* SILVER CAVE */
    {{150,160,MG_D_LANTERN}, {567,160,MG_D_FLOWERS}, {985,160,MG_D_SIGN}, {1402,160,MG_D_BUSH}, {1820,160,MG_D_GRASS}, {2237,160,MG_D_SAPLING}, {2655,160,MG_D_MUSHROOM}, {3072,160,MG_D_LANTERN}, {3490,160,MG_D_LANTERN}, {544,72,MG_D_BUSH}, {1212,72,MG_D_GRASS}, {1880,72,MG_D_SAPLING}, {3720,160,MG_D_LANTERN}},  /* GOLDEN SAVANNA */
};

static const MGVine mg_vines[MG_LEVEL_COUNT][MG_VINE_COUNT] = {
    {{692,68,192}, {1304,68,192}, {1972,68,192}},
    {{692,76,192}, {1352,72,192}, {2012,72,192}},
    {{752,72,192}, {1412,72,192}, {2072,72,192}},
    {{772,68,192}, {1432,68,192}, {2092,68,192}},
    {{792,72,192}, {1452,72,192}, {2112,72,192}},
    {{832,64,192}, {1552,64,192}, {2272,64,192}},
    {{792,64,192},{1492,64,192},{2212,64,192}},
    {{752,72,192}, {1412,72,192}, {2072,72,192}},
    {{792,72,192}, {1452,72,192}, {2112,72,192}},
    {{772,68,192}, {1432,68,192}, {2092,68,192}},
};

static const MGNpc mg_npcs[MG_LEVEL_COUNT][MG_NPC_COUNT] = {
    {{420,0,"THE GATE OPENS TO THE SUN KEY"}, {1600,1,"CLIMB THE VINES TO THE CANOPY"}, {2680,2,"HOLD DOWN TO KNEEL AND REST"}},
    {{420,1,"THE FALLS HIDE A GOLDEN ROSE"}, {1725,0,"DOWN THEN FORWARD AND B: SURGE"}, {2930,2,"SPIRITS GUARD THE HIGH LEDGES"}},
    {{420,0,"THE TIDE CARRIES THE BLIGHT IN"}, {1850,2,"UP AT A VINE TO CLIMB IT"}, {3180,1,"SEEK THE KEY ABOVE THE SAND"}},
    {{420,1,"AUTUMN KEEPS ITS SEEDS ALIVE"}, {1920,0,"STRIKE THE GUARDIAN'S CORE"}, {3320,2,"D CALLS THE ROSE WIND STRIKE"}},
    {{420,2,"COLD SLEEPS, IT DOES NOT DIE"}, {1980,0,"THE KEY WAITS ON THE HIGH SHELF"}, {3440,1,"WARM STONES MARK THE SAFE PATH"}},
    {{420,0,"THE WORLD TREE STILL BREATHES"}, {2080,3,"MAIYA! THE LAST GATE IS NEAR"}, {3640,2,"ONE ROOT, ONE VALLEY, ONE SUN"}},
    {{420,0,"THE WORKS POISON THE WHOLE RIVER"}, {2220,2,"TWO GUARDIANS HOLD THE GATE"}, {3920,3,"MAIYA, I AM RIGHT BEHIND IT"}},
    {{420,0,"THE REEF STILL FEELS THE POISON"}, {1850,2,"UP AT A VINE TO CLIMB IT"}, {3180,1,"THE KEY GLEAMS ABOVE THE SAND"}},
    {{420,2,"A SECRET FOUND CAN LIFT THE POISON"}, {1980,0,"THE KEY WAITS ON THE HIGH SHELF"}, {3440,1,"WARM STONES MARK THE SAFE PATH"}},
    {{420,1,"A SECRET FOUND CAN LIFT THE POISON"}, {1920,0,"STRIKE THE GUARDIAN'S CORE"}, {3320,2,"D CALLS THE ROSE WIND STRIKE"}},
};

/* Where the Golden Sun Key waits: always on a high shelf, never on the road. */
static const uint16_t mg_key_pos[MG_LEVEL_COUNT][2] = {
    {2012, 42},   /* EMERALD FOREST */
    {2052, 46},   /* VALLEY OF FALLS */
    {2112, 46},   /* AZURE COAST */
    {2132, 42},   /* AUTUMN GROVE */
    {2152, 46},   /* CRYSTAL GROTTO */
    {2312, 38},   /* SACRED WORLD TREE */
    {2252, 38},   /* RIO NEGRO WORKS */
    {2112, 46},   /* SUNKEN REEF */
    {2152, 46},   /* SILVER CAVE */
    {2132, 42},   /* GOLDEN SAVANNA */
};

/* ---------------------------------------------------------------- */
/*  Secret Arts, elder lore and what Sunboy says between missions    */
/* ---------------------------------------------------------------- */

/* The Secret Art Maiya carries in each valley: the roses she gathers
 * charge it, and D unleashes it. */
static const char *const mg_art_name[MG_LEVEL_COUNT] = {
    "ROSE WIND STRIKE",
    "PURIFYING RAIN",
    "TIDE BLOSSOM",
    "SUNFLARE DANCE",
    "FROST PETAL STORM",
    "WORLD TREE BLOOM",
    "RIVER OF LIGHT",
    "REEF CURRENT CALL",
    "CAVERN LIGHT BLOOM",
    "SAVANNA WIND CHASE",
};

/* What she calls out as each art breaks over the valley. */
static const char *const mg_art_words[MG_LEVEL_COUNT] = {
    "MAIYA: ROSE WIND, CARRY ME!",
    "MAIYA: RAIN, WASH IT CLEAN!",
    "MAIYA: TIDE, TAKE IT BACK!",
    "MAIYA: SUN, BURN THE BLIGHT!",
    "MAIYA: FROST, HOLD THEM STILL!",
    "MAIYA: WORLD TREE, BLOOM!",
    "MAIYA: RIVER, RUN CLEAR AGAIN!",
    "MAIYA: CURRENT, CARRY THE FILTH AWAY!",
    "MAIYA: LIGHT, FIND WHAT'S HIDDEN!",
    "MAIYA: WIND, CLEAR THEIR TRAPS!",
};

/* What the elder's charm whispers about this valley's hidden places. */
static const char *const mg_secret_hint[MG_LEVEL_COUNT] = {
    "A ROSE SLEEPS ABOVE THE ARCH",
    "BEHIND THE FALLS: A SUN SEED",
    "THE OLD PIER HIDES A COIN HOARD",
    "SHAKE THE BLOSSOM TREE FOR GOLD",
    "ICE KEEPS A FRIEND: FREE IT",
    "THE ROOT HOLLOW HOLDS ONE LIFE",
    "A LIFE HANGS ABOVE THE FIRST TANK",
    "THE REEF SHELTERS ONE LAST PEARL",
    "THE DEEP VEIN GLINTS WITH GOLD",
    "THE BAOBAB HOLLOW HIDES A CHARM",
};

/* And how the guardian at the end of it can be broken. */
static const char *const mg_boss_hint[MG_LEVEL_COUNT] = {
    "STRIKE THE BEETLE WHEN IT LANDS",
    "THE TOAD GULPS: HIT ITS THROAT",
    "THE LEVIATHAN RISES, THEN STALLS",
    "THE JACKAL BURNS OUT: THEN HIT",
    "THE OWL BLINKS BETWEEN GUSTS",
    "SMOGGAR'S CORE OPENS AS IT ROARS",
    "THE VULTURE FALLS, THEN SMOGGAR",
    "THE EEL COILS: STRIKE BETWEEN LUNGES",
    "THE WYRM GLOWS BEFORE IT BITES",
    "THE HYENA TIRES AFTER THE POUNCE",
};

/* Sunboy, held in the crystal until Rio Negro Works, cheers her on from
 * freedom for the three valleys after. */
static const char *const mg_sunboy_line[MG_LEVEL_COUNT][2] = {
    {"MAIYA! THE FOREST BREATHES AGAIN", "THE MARSH FALLS ARE NEXT. HURRY"},
    {"THE WATER RUNS CLEAR. I FELT IT", "TAKE THE COAST ROAD. I AM WEAKER"},
    {"THE SEA IS SINGING AGAIN", "THE GROVE IS BURNING. GO THERE"},
    {"AUTUMN KEEPS ITS SEED. WELL DONE", "THE GROTTO HOLDS MY CRYSTAL"},
    {"I CAN SEE YOU THROUGH THE ICE", "TWO VALLEYS LEFT. THE WORLD TREE"},
    {"THE TREE BREATHES. I AM SO CLOSE", "THE WORKS ON THE RIVER. LAST ONE"},
    {"YOU FREED ME! I FELT THE CHAINS BREAK", "THREE MORE VALLEYS NEED US. LET'S GO"},
    {"THE REEF REMEMBERS YOUR KINDNESS", "THE CAVE IS NEXT. I'LL LIGHT THE WAY"},
    {"THE CAVE BREATHES CLEAN AIR AGAIN", "ONE VALLEY LEFT: THE OPEN SAVANNA"},
    {"THE HERD RUNS FREE. WE DID IT", "THANK YOU, SUPER NATURE GIRL"},
};

/* Collectibles strung along the road: coins for the count, a flower and a
 * caged forest friend for the mission tally, the elder's charm for a hint,
 * and one extra life kept where only a vine can reach. */
#define MG_PICK_COUNT 10

typedef struct {
    int16_t x;
    uint8_t y, kind;         /* kind indexes the MG_K_* trinket set */
} MGPickup;

static const MGPickup mg_picks[MG_LEVEL_COUNT][MG_PICK_COUNT] = {
    {{260,168,MG_K_SILVER}, {885,168,MG_K_SPRING}, {1510,168,MG_K_CROWN}, {2135,168,MG_K_SILVER}, {512,70,MG_K_GOLD}, {1128,70,MG_K_GOLD}, {1066,166,MG_K_FLOWER}, {2133,164,MG_K_CRITTER}, {1690,166,MG_K_CHARM}, {648,40,MG_K_LIFE}},  /* EMERALD FOREST */
    {{260,168,MG_K_SILVER}, {947,168,MG_K_SPRING}, {1635,168,MG_K_CROWN}, {2322,168,MG_K_SILVER}, {492,78,MG_K_GOLD}, {1148,74,MG_K_GOLD}, {1150,166,MG_K_FLOWER}, {2300,164,MG_K_CRITTER}, {1815,166,MG_K_CHARM}, {648,48,MG_K_LIFE}},  /* VALLEY OF FALLS */
    {{260,168,MG_K_SILVER}, {1010,168,MG_K_SPRING}, {1760,168,MG_K_CROWN}, {2510,168,MG_K_SILVER}, {532,74,MG_K_GOLD}, {1208,74,MG_K_GOLD}, {1233,166,MG_K_FLOWER}, {2466,164,MG_K_CRITTER}, {1940,166,MG_K_CHARM}, {708,44,MG_K_LIFE}},  /* AZURE COAST */
    {{260,168,MG_K_SILVER}, {1045,168,MG_K_SPRING}, {1830,168,MG_K_CROWN}, {2615,168,MG_K_SILVER}, {552,70,MG_K_GOLD}, {1228,70,MG_K_GOLD}, {1280,166,MG_K_FLOWER}, {2560,164,MG_K_CRITTER}, {2010,166,MG_K_CHARM}, {728,40,MG_K_LIFE}},  /* AUTUMN GROVE */
    {{260,168,MG_K_SILVER}, {1075,168,MG_K_SPRING}, {1890,168,MG_K_CROWN}, {2705,168,MG_K_SILVER}, {572,74,MG_K_GOLD}, {1248,74,MG_K_GOLD}, {1320,166,MG_K_FLOWER}, {2640,164,MG_K_CRITTER}, {2070,166,MG_K_CHARM}, {748,44,MG_K_LIFE}},  /* CRYSTAL GROTTO */
    {{260,168,MG_K_SILVER}, {1125,168,MG_K_SPRING}, {1990,168,MG_K_CROWN}, {2855,168,MG_K_SILVER}, {592,70,MG_K_GOLD}, {1328,70,MG_K_GOLD}, {1386,166,MG_K_FLOWER}, {2773,164,MG_K_CRITTER}, {2170,166,MG_K_CHARM}, {788,36,MG_K_LIFE}},  /* SACRED WORLD TREE */
    {{260,168,MG_K_SILVER}, {1100,168,MG_K_SPRING}, {1940,168,MG_K_CROWN}, {2780,168,MG_K_SILVER}, {552,70,MG_K_GOLD}, {1252,70,MG_K_GOLD}, {1480,166,MG_K_FLOWER}, {2960,164,MG_K_CRITTER}, {2310,166,MG_K_CHARM}, {748,36,MG_K_LIFE}},  /* RIO NEGRO WORKS */
    {{260,168,MG_K_SILVER}, {1010,168,MG_K_SPRING}, {1760,168,MG_K_CROWN}, {2510,168,MG_K_SILVER}, {532,74,MG_K_GOLD}, {1208,74,MG_K_GOLD}, {1233,166,MG_K_FLOWER}, {2466,164,MG_K_CRITTER}, {1940,166,MG_K_CHARM}, {708,44,MG_K_LIFE}},  /* SUNKEN REEF */
    {{260,168,MG_K_SILVER}, {1075,168,MG_K_SPRING}, {1890,168,MG_K_CROWN}, {2705,168,MG_K_SILVER}, {572,74,MG_K_GOLD}, {1248,74,MG_K_GOLD}, {1320,166,MG_K_FLOWER}, {2640,164,MG_K_CRITTER}, {2070,166,MG_K_CHARM}, {748,44,MG_K_LIFE}},  /* SILVER CAVE */
    {{260,168,MG_K_SILVER}, {1045,168,MG_K_SPRING}, {1830,168,MG_K_CROWN}, {2615,168,MG_K_SILVER}, {552,70,MG_K_GOLD}, {1228,70,MG_K_GOLD}, {1280,166,MG_K_FLOWER}, {2560,164,MG_K_CRITTER}, {2010,166,MG_K_CHARM}, {728,40,MG_K_LIFE}},  /* GOLDEN SAVANNA */
};

/* What each guardian says on the arena threshold, and what she says back. */
static const char *const mg_boss_taunt[MG_LEVEL_COUNT] = {
    "I CUT DOWN WHAT TOOK CENTURIES TO GROW!",
    "I DROWNED YOUR SPRINGS IN MY OWN FILTH!",
    "I FED THE LIVING REEF TO MY HUNGER!",
    "I BURNED THE GROVE TO ASH AND RULE!",
    "I FROZE THE GROTTO'S HEART TO STONE!",
    "I DRINK THE WORLD TREE DRY FOR POWER!",
    "I BUILT THIS PLANT TO CHOKE YOUR RIVER!",
    "I CHOKED THE REEF WITH MY OWN WASTE!",
    "I HOARD THE CAVE'S LIGHT FOR MYSELF!",
    "I HUNT WHAT ISN'T MINE TO TAKE!",
};

static const char *const mg_boss_reply[MG_LEVEL_COUNT] = {
    "THE FOREST WILL OUTLIVE YOUR BLADES!",
    "THE WATER REMEMBERS ITS OWN COLOR!",
    "THE TIDE WILL CARRY YOU OUT INSTEAD!",
    "ASH IS WHERE NEW ROOTS BEGIN!",
    "WINTER ENDS. I AM ITS SPRING!",
    "ITS ROOTS RUN DEEPER THAN YOUR GREED!",
    "YOUR SMOKE ENDS HERE, IRON VULTURE!",
    "THE TIDE WASHES YOUR FILTH AWAY!",
    "HER LIGHT WAS NEVER YOURS TO KEEP!",
    "THESE PLAINS AREN'T YOURS TO HUNT!",
};

/* Lord Smoggar's true face, once the Iron Vulture falls in Rio Negro Works. */
#define MG_SMOGGAR_TAUNT "YOUR PRINCE FEEDS MY ROOTS NOW, GUARDIAN!"
#define MG_SMOGGAR_REPLY "GIVE ME BACK MY BROTHER, SMOGGAR!"

#endif
