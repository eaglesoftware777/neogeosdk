/*
 * ns_data.c -- No Signal Z Alpha: fighters, moves, stages and scenes.
 *
 * Distances are pixels, speeds pixels * 16 per frame, as ng_beatemup.h
 * describes; a move's `anim` is the frame its fighter shows while it runs
 * and `sound` the effect of its swing. Heroes stand about 96 pixels tall:
 * a jab reaches head height (z 46-76), a kick the body, a sweep the feet.
 */
#include "ns.h"
#include "games/nosignal/artbox/generated/ns_art_ids.h"

/* ======================================================================
 * the three heroes
 * ==================================================================== */
/*             su ac re dmg hs flags                                  bx  bw bz bh bd  px  pz lng cnl next           snd            anim stop step */
static const NGBMove JAY_MOVES[] = {
    /* 0 jab    */ { 3, 2, 7, 3, 14, 0,                                6, 43, 46, 30,  7,  8,  0,  0,  6, 1,             NS_SFX_PUNCH,  HF_JAB, 3 },
    /* 1 cross  */ { 3, 2, 8, 4, 14, 0,                                6, 44, 46, 30,  7, 10,  0,  4,  6, 2,             NS_SFX_PUNCH,  HF_CROSS, 4, 40 },
    /* 2 upper  */ { 5, 3, 10, 5, 18, 0,                               4, 22, 40, 44,  7, 10,  0,  0,  8, 3,             NS_SFX_HEAVY,  HF_UPPER, 5, 80 },
    /* 3 kick   */ { 5, 3, 14, 7, 0, NGB_MV_KNOCKDOWN,                 4, 44, 30, 50,  7, 44, 30,  8,  0, NGB_NONE,      NS_SFX_KICK,   HF_KICK, 6, 40 },
    /* 4 air    */ { 2, 10, 4, 6, 0, NGB_MV_KNOCKDOWN,                 0, 41, 20, 54,  7, 34, 24,  0,  0, NGB_NONE,      NS_SFX_KICK,   HF_JUMPKICK, 5 },
    /* 5 low    */ { 4, 3, 12, 5, 0, NGB_MV_LOW | NGB_MV_KNOCKDOWN,    4, 49,  0, 22,  7, 24, 16,  0,  0, NGB_NONE,      NS_SFX_KICK,   HF_SWEEP, 5 },
    /* 6 dash   */ { 3, 8, 12, 7, 0, NGB_MV_KNOCKDOWN,                 0, 31, 10, 70,  7, 48, 32, 32,  0, NGB_NONE,      NS_SFX_HEAVY,  HF_DASH, 6 },
    /* 7 Ratio Rush: a blur of fists, then the spin kick */
               { 4, 16, 14, 11, 0, NGB_MV_KNOCKDOWN | NGB_MV_UNBLOCKABLE | NGB_MV_INVULN, -36, 89, 10, 70, 12, 52, 44, 12, 0, NGB_NONE, NS_SFX_SPECIAL, HF_SPECIAL1, 8 },
    /* 8 knee   */ { 2, 2, 8, 3, 10, 0,                                0, 32, 20, 40,  7,  0,  0,  0,  0, NGB_NONE,      NS_SFX_PUNCH,  HF_GRAB },
};
static const NGBMove ZOE_MOVES[] = {
    /* 0 jab    */ { 2, 2, 6, 2, 12, 0,                                6, 40, 46, 30,  7,  8,  0,  0,  5, 1,             NS_SFX_PUNCH,  HF_JAB, 3 },
    /* 1 cross  */ { 2, 2, 7, 3, 12, 0,                                6, 37, 46, 30,  7, 10,  0,  4,  5, 2,             NS_SFX_PUNCH,  HF_CROSS, 4, 40 },
    /* 2 upper  */ { 4, 3, 9, 4, 16, 0,                                4, 22, 40, 44,  7, 10,  0,  0,  7, 3,             NS_SFX_KICK,   HF_UPPER, 5, 80 },
    /* 3 kick   */ { 4, 3, 12, 6, 0, NGB_MV_KNOCKDOWN | NGB_MV_LAUNCH, 4, 40, 30, 48,  7, 32, 48,  8,  0, NGB_NONE,      NS_SFX_KICK,   HF_KICK, 6, 40 },
    /* 4 air    */ { 2, 12, 4, 6, 0, NGB_MV_KNOCKDOWN,                 0, 44, 16, 54,  7, 32, 24,  0,  0, NGB_NONE,      NS_SFX_KICK,   HF_JUMPKICK, 5 },
    /* 5 low    */ { 3, 3, 10, 4, 0, NGB_MV_LOW | NGB_MV_KNOCKDOWN,    4, 40,  0, 20,  7, 24, 16,  0,  0, NGB_NONE,      NS_SFX_KICK,   HF_SWEEP, 5 },
    /* 6 dash   */ { 2, 8, 10, 6, 0, NGB_MV_KNOCKDOWN,                 0, 34, 20, 50,  7, 40, 32, 36,  0, NGB_NONE,      NS_SFX_WHOOSH, HF_DASH, 6 },
    /* 7 Main Character Energy: up in a flip, down in an axe kick */
               { 6, 14, 12, 10, 0, NGB_MV_KNOCKDOWN | NGB_MV_UNBLOCKABLE | NGB_MV_INVULN, -24, 64, 0, 90, 12, 40, 56, 16, 0, NGB_NONE, NS_SFX_SPECIAL, HF_SPECIAL1, 8 },
    /* 8 knee   */ { 2, 2, 7, 2, 10, 0,                                0, 42, 20, 40,  7,  0,  0,  0,  0, NGB_NONE,      NS_SFX_PUNCH,  HF_GRAB },
};
static const NGBMove KAI_MOVES[] = {
    /* 0 jab    */ { 4, 3, 8, 4, 16, 0,                                6, 45, 46, 30,  7,  8,  0,  0,  7, 1,             NS_SFX_PUNCH,  HF_JAB, 3 },
    /* 1 cross  */ { 4, 3, 9, 6, 16, 0,                                6, 47, 46, 30,  7, 10,  0,  4,  8, 2,             NS_SFX_HEAVY,  HF_CROSS, 5, 40 },
    /* 2 upper  */ { 6, 3, 12, 7, 18, 0,                               4, 24, 40, 44,  7, 12,  0,  0, 10, 3,             NS_SFX_HEAVY,  HF_UPPER, 6, 80 },
    /* 3 kick   */ { 7, 4, 16, 10, 0, NGB_MV_KNOCKDOWN,                4, 49, 26, 56,  7, 56, 36,  8,  0, NGB_NONE,      NS_SFX_HEAVY,  HF_KICK, 7, 40 },
    /* 4 air    */ { 3, 10, 6, 8, 0, NGB_MV_KNOCKDOWN,                 0, 52, 24, 58,  7, 40, 28,  0,  0, NGB_NONE,      NS_SFX_HEAVY,  HF_JUMPKICK, 5 },
    /* 5 low    */ { 5, 3, 14, 6, 0, NGB_MV_LOW | NGB_MV_KNOCKDOWN,    4, 58,  0, 18,  7, 28, 16,  0,  0, NGB_NONE,      NS_SFX_HEAVY,  HF_SWEEP, 5 },
    /* 6 dash   */ { 4, 8, 14, 9, 0, NGB_MV_KNOCKDOWN,                 0, 38, 10, 56,  7, 56, 36, 28,  0, NGB_NONE,      NS_SFX_HEAVY,  HF_DASH, 7 },
    /* 7 Aura Breaker: everyone near goes down */
               { 8, 10, 18, 13, 0, NGB_MV_KNOCKDOWN | NGB_MV_UNBLOCKABLE | NGB_MV_INVULN, -64, 128, 0, 80, 14, 56, 52, 0, 0, NGB_NONE, NS_SFX_SPECIAL, HF_SPECIAL1, 8 },
    /* 8 headbutt */ { 3, 2, 9, 5, 12, 0,                              0, 51, 30, 40,  7,  0,  0,  0,  0, NGB_NONE,      NS_SFX_HEAVY,  HF_GRAB },
};

/* max_hp speed depth jump half_w height half_depth weight | moves n | chain air low dash special pummel throw_f throw_b | ai reach aggr attacks n flags score phases n think */
const NGBFighterDef ns_heroes[NS_HEROES] = {
    { 60, 34, 22, 72, 12, 84, 6, 3, JAY_MOVES, 9, 0, 4, 5, 6, 7, 8, NGB_NONE, NGB_NONE,
      0, 0, 0, {0}, 0, 0, 0, 0, 0, 0 },
    { 54, 38, 26, 80, 11, 80, 6, 2, ZOE_MOVES, 9, 0, 4, 5, 6, 7, 8, NGB_NONE, NGB_NONE,
      0, 0, 0, {0}, 0, 0, 0, 0, 0, 0 },
    { 70, 28, 18, 64, 14, 88, 7, 5, KAI_MOVES, 9, 0, 4, 5, 6, 7, 8, NGB_NONE, NGB_NONE,
      0, 0, 0, {0}, 0, 0, 0, 0, 0, 0 },
};

/* Stage 8, aboard the ship: the same heroes with higher, floatier jumps. */
const NGBFighterDef ns_heroes_space[NS_HEROES] = {
    { 60, 34, 22, 104, 12, 84, 6, 3, JAY_MOVES, 9, 0, 4, 5, 6, 7, 8, NGB_NONE, NGB_NONE,
      0, 0, 0, {0}, 0, 0, 0, 0, 0, 0 },
    { 54, 38, 26, 112, 11, 80, 6, 2, ZOE_MOVES, 9, 0, 4, 5, 6, 7, 8, NGB_NONE, NGB_NONE,
      0, 0, 0, {0}, 0, 0, 0, 0, 0, 0 },
    { 70, 28, 18, 96, 14, 88, 7, 5, KAI_MOVES, 9, 0, 4, 5, 6, 7, 8, NGB_NONE, NGB_NONE,
      0, 0, 0, {0}, 0, 0, 0, 0, 0, 0 },
};

const NSHeroInfo ns_hero_info[NS_HEROES] = {
    { "JAY", "NO CAP", "FAST HANDS, BIG HEART", "RATIO RUSH", NS_SPK_JAY },
    { "ZOE", "MAIN CHARACTER", "FLIPS, COUNTERS, SASS", "MAIN CHARACTER ENERGY", NS_SPK_ZOE },
    { "KAI", "AURA", "SLOW, STRONG, UNBOTHERED", "AURA BREAKER", NS_SPK_KAI },
};

/* ======================================================================
 * the street crew of New Vista
 * ==================================================================== */
static const NGBMove THIEF_MOVES[] = {
    /* 0 jab    */ { 10, 3, 16, 3, 12, 0,                              4, 37, 40, 30,  9, 16,  0,  0,  0, NGB_NONE,      NS_SFX_PUNCH,  EF_ATTACK },
    /* 1 sneaky shove */ { 12, 4, 18, 4, 0, NGB_MV_KNOCKDOWN,          4, 37, 20, 50,  9, 36, 20, 12,  0, NGB_NONE,      NS_SFX_WHOOSH, EF_ATTACK },
};
static const NGBMove BIKER_MOVES[] = {
    /* 0 charging punch, at a run */ { 14, 28, 22, 7, 0, NGB_MV_KNOCKDOWN | NGB_MV_RUN, 0, 50, 40, 40,  9, 44, 30, 52, 0, NGB_NONE, NS_SFX_HEAVY, EF_ATTACK },
    /* 1 haymaker */ { 16, 4, 20, 8, 0, NGB_MV_KNOCKDOWN,              4, 48, 40, 40,  9, 40, 24,  0,  0, NGB_NONE,      NS_SFX_HEAVY,  EF_ATTACK },
};
static const NGBMove SKATER_MOVES[] = {
    /* 0 board swing */ { 10, 4, 16, 5, 0, NGB_MV_KNOCKDOWN,           4, 48, 20, 50,  9, 36, 24,  8,  0, NGB_NONE,      NS_SFX_WHOOSH, EF_ATTACK },
    /* 1 kickflip leap */ { 8, 10, 14, 5, 0, NGB_MV_KNOCKDOWN,         0, 46,  0, 50,  9, 32, 24, 28,  0, NGB_NONE,      NS_SFX_KICK,   EF_ATTACK },
};
static const NGBMove RICO_MOVES[] = {
    /* 0 LED jab   */ { 8, 3, 12, 5, 14, 0,                            4, 46, 46, 30,  9, 18,  0,  0,  0, NGB_NONE,      NS_SFX_PUNCH,  EF_ATTACK },
    /* 1 boot      */ { 12, 4, 16, 7, 0, NGB_MV_KNOCKDOWN,             4, 46, 26, 40,  9, 44, 28,  8,  0, NGB_NONE,      NS_SFX_KICK,   EF_ATTACK },
    /* 2 phone pitch */ { 16, 1, 22, 6, 0, NGB_MV_PROJECTILE,          8,  0, 50, 12, 10,  0,  0, 64,  0, NGB_NONE,      NS_SFX_PHONE,  BF_THROW_PHONE },
    /* 3 bike charge */ { 24, 40, 24, 10, 0, NGB_MV_KNOCKDOWN | NGB_MV_RUN | NGB_MV_UNBLOCKABLE, 0, 48, 0, 70,  9, 56, 40, 80, 0, NGB_NONE, NS_SFX_BIKE, BF_BIKE1 },
    /* 4 desperate grab */ { 10, 5, 18, 0, 0, NGB_MV_GRAB,             4, 30,  0, 60, 10,  0,  0, 24,  0, NGB_NONE,      NS_SFX_GRAB,   BF_LUNGE },
};
static const NGBBossPhase RICO_PHASES[3] = {
    /* below%  invuln  attacks       n  speed+ flags         event */
    { 100, 0,  { 0, 1, 2 },    3, 0, NGB_FD_ARMOR, 1 },
    { 60,  90, { 3, 0, 2 },    3, 6, NGB_FD_ARMOR, 2 },     /* "fine. we ride." */
    { 30,  90, { 3, 4, 1, 2 }, 4, 10, 0, 3 },              /* panicking: grabs and everything */
};

const NGBFighterDef ns_thief = { 26, 26, 18, 64, 12, 80, 6, 2, THIEF_MOVES, 2, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                 NGB_AI_BRAWLER, 28, 9, { 0, 1 }, 2, 0, 100, 0, 0, 0 };
const NGBFighterDef ns_biker = { 56, 18, 12, 48, 16, 90, 8, 6, BIKER_MOVES, 2, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                 NGB_AI_CHARGER, 34, 7, { 0, 1 }, 2, 0, 250, 0, 0, 0 };
const NGBFighterDef ns_skater = { 32, 30, 22, 80, 12, 82, 6, 2, SKATER_MOVES, 2, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                  NGB_AI_JUMPER, 38, 10, { 0, 1 }, 2, 0, 150, 0, 0, 0 };
const NGBFighterDef ns_rico = { 160, 24, 16, 64, 14, 88, 8, 6, RICO_MOVES, 5, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                NGB_AI_BOSS, 36, 12, { 0, 1, 2 }, 3, NGB_FD_HEAVY, 5000, RICO_PHASES, 3, 0 };

/* ======================================================================
 * stage 1: the hood
 * ==================================================================== */
static const NGBMove BAT_SWING = { 5, 4, 12, 7, 0, NGB_MV_KNOCKDOWN, 4, 52, 20, 50, 12, 40, 28, 0, 0, NGB_NONE, NS_SFX_HEAVY, HF_KICK };
static const NGBMove WRECK_SWING = { 6, 4, 14, 8, 0, NGB_MV_KNOCKDOWN, 4, 56, 10, 56, 14, 44, 32, 0, 0, NGB_NONE, NS_SFX_CRASH, HF_KICK };
/* the last weapon is the scooter's wreck */
static const NGBWeaponDef S1_WEAPONS[2] = { { &BAT_SWING, 5, 10, 0 }, { &WRECK_SWING, 3, 12, 1 } };
static const NGBFighterDef *const S1_ENEMIES[4] = { &ns_thief, &ns_skater, &ns_biker, &ns_rico };
enum { E_THIEF = 0, E_SKATER, E_BIKER, E_RICO };
static const NGBWave S1_WAVES[] = {
    /* at   flags                          n  enemies                                         sides  gap */
    { 40,   0,                             2, { E_THIEF, E_THIEF },                           0x00,  40 },
    { 220,  NGB_WAVE_LOCK,                 3, { E_THIEF, E_SKATER, E_THIEF },                 0x02,  30 },
    { 470,  0,                             2, { E_BIKER, E_THIEF },                           0x00,  50 },
    { 700,  NGB_WAVE_LOCK,                 4, { E_SKATER, E_THIEF, E_SKATER, E_BIKER },       0x05,  30 },
    { 900,  0,                             3, { E_THIEF, E_THIEF, E_SKATER },                 0x00,  24 },
    { 1120, NGB_WAVE_LOCK | NGB_WAVE_BOSS, 1, { E_RICO },                                     0x00,   0 },
};
/* Crates, bins and weapons lie in the middle of the street (depth 168-180,
 * the hero starts on 173) and well away from hazards: walking over what they
 * hold takes it (NGB_SF_AUTO_PICKUP on every stage). */
static const NGBPropSpawn S1_PROPS[] = {
    { NGB_PROP_CRATE,   176, 300, NGB_PROP_FOOD, 2 },
    { NGB_PROP_BARREL,  170, 430, NGB_PROP_SCORE, 2 },
    { NGB_PROP_WEAPON,  176, 600, 0, 0 },
    { NGB_PROP_VEHICLE, 170, 820, 0, 0 },
    { NGB_PROP_RAMP,    176, 960, 0, 0 },
    { NGB_PROP_CRATE,   172, 1040, NGB_PROP_FOOD, 2 },
    { NGB_PROP_HAZARD,  204, 1180, 0, 0 },
};
static const NGBStage S1 = {
    1440, 140, 206, 320, S1_WAVES, S1_PROPS, S1_ENEMIES, S1_WEAPONS,
    sizeof(S1_WAVES) / sizeof(S1_WAVES[0]), sizeof(S1_PROPS) / sizeof(S1_PROPS[0]), 4, 2,
    2, 5, NGB_SF_NO_BACKTRACK | NGB_SF_AUTO_PICKUP | NGB_SF_WALK_ON, 6, 96, 4, 3, 8, 10, 18 };

/* ---- stage 1's scenes -------------------------------------------------- */
static const NSLine S1_INTRO_JAY[] = {
    { NS_SPK_NARRATOR, "NEW VISTA. 5:47 PM. THE CORNER STORE." },
    { NS_SPK_JAY, "PHONE AT 3%. I JUST NEED A CHARGER AND ONE (1) SPICY CHIP." },
    { NS_SPK_NARRATOR, "A MOTORBIKE SCREAMS PAST. THE PHONE IS GONE." },
    { NS_SPK_JAY, "BRO. TWO YEARS OF AMARA'S VOICE NOTES ARE ON THAT PHONE." },
    { NS_SPK_JAY, "WHOEVER DID THIS? YOU'RE COOKED. NO CAP." },
    { 0, 0 } };
static const NSLine S1_INTRO_ZOE[] = {
    { NS_SPK_NARRATOR, "NEW VISTA. 5:47 PM. THE CORNER STORE." },
    { NS_SPK_ZOE, "OK CHAT, QUICK SNACK HAUL BEFORE THEO CALLS FROM LISBON..." },
    { NS_SPK_NARRATOR, "A MOTORBIKE SCREAMS PAST. THE PHONE IS GONE." },
    { NS_SPK_ZOE, "EXCUSE ME?? EVERY VIDEO HE EVER SENT ME IS ON THAT PHONE." },
    { NS_SPK_ZOE, "MAIN CHARACTER ARC: ACTIVATED." },
    { 0, 0 } };
static const NSLine S1_INTRO_KAI[] = {
    { NS_SPK_NARRATOR, "NEW VISTA. 5:47 PM. THE CORNER STORE." },
    { NS_SPK_KAI, "I SOLDERED THIS PHONE BACK TOGETHER THREE TIMES. IT HAS LORE." },
    { NS_SPK_NARRATOR, "A MOTORBIKE SCREAMS PAST. THE PHONE IS GONE." },
    { NS_SPK_KAI, "...THE LAST MESSAGE NOVA EVER SENT ME IS ON THERE." },
    { NS_SPK_KAI, "OK. AURA FARMING TIME." },
    { 0, 0 } };
static const NSLine S1_BOSS_ALL[] = {
    { NS_SPK_RICO, "NAME'S RICO. 'SWIPE' MENDOZA. YOUR PHONE? ALREADY SOLD, BRO." },
    { NS_SPK_RICO, "SOME GUY IN A LAB COAT PAID TRIPLE. CASH. NO QUESTIONS." },
    { 0xFF, "TRIPLE?! THE SCREEN IS CRACKED IN FOUR PLACES." },
    { NS_SPK_RICO, "...OK THAT IS KIND OF SUS. ANYWAY. FIGHT ME." },
    { 0, 0 } };
static const NSLine S1_OUTRO_ALL[] = {
    { NS_SPK_RICO, "OK OK OK! SYNAPSE SOMETHING! DOWNTOWN, UNDER THE HIGHWAY!" },
    { NS_SPK_RICO, "PLEASE DON'T POST THIS. I HAVE A REPUTATION." },
    { 0xFF, "YOU HAD A REPUTATION. NOW YOU HAVE NEGATIVE AURA." },
    { NS_SPK_NARRATOR, "SOMEWHERE DOWNTOWN, A SCREEN BLINKS: SIGNAL ACQUIRED." },
    { 0, 0 } };

/* ======================================================================
 * the other factions
 * ==================================================================== */
/*                         su ac re dmg hs flags                              bx bw bz bh bd  px  pz lng cnl next      snd            anim */
static const NGBMove GUARD_MOVES[] = {
    /* 0 baton jab */    { 9, 3, 14, 5, 14, 0,                                4, 46, 40, 30,  9, 20,  0,  0,  0, NGB_NONE, NS_SFX_ZAP,    EF_ATTACK },
    /* 1 shield bash */  { 12, 4, 18, 6, 0, NGB_MV_KNOCKDOWN,                 0, 50, 10, 70,  9, 40, 24, 16,  0, NGB_NONE, NS_SFX_HEAVY,  EF_WINDUP },
};
static const NGBMove DRONE_MOVES[] = {
    /* 0 bolt */         { 18, 1, 26, 4, 0, NGB_MV_PROJECTILE,                 8,  0, 52, 10, 10,  0,  0, 56,  0, NGB_NONE, NS_SFX_LASER,  EF_ATTACK },
    /* 1 ram */          { 10, 6, 18, 3, 12, 0,                                0, 21, 36, 40,  9, 24,  0, 40,  0, NGB_NONE, NS_SFX_SERVO,  EF_WINDUP },
};
static const NGBMove RUNNER_MOVES[] = {
    /* 0 flailing leap */{ 7, 6, 16, 4, 0, NGB_MV_KNOCKDOWN | NGB_MV_RUN,      0, 63, 20, 50,  9, 30, 20, 44,  0, NGB_NONE, NS_SFX_WHOOSH, EF_ATTACK },
    /* 1 slap */         { 6, 3, 12, 3, 12, 0,                                 4, 59, 40, 30,  9, 14,  0,  0,  0, NGB_NONE, NS_SFX_PUNCH,  EF_ATTACK },
};
static const NGBMove BLOATER_MOVES[] = {
    /* 0 double slam */  { 18, 4, 22, 10, 0, NGB_MV_KNOCKDOWN,                4, 34, 10, 60,  9, 46, 26,  0,  0, NGB_NONE, NS_SFX_STOMP,  EF_ATTACK },
    /* 1 belly shove */  { 12, 4, 18, 6, 14, 0,                                0, 38, 20, 50,  9, 30,  0, 10,  0, NGB_NONE, NS_SFX_HEAVY,  EF_WINDUP },
};
static const NGBMove ANDROID_MOVES[] = {
    /* 0 blade thrust */ { 7, 3, 12, 5, 14, 0,                                 6, 54, 40, 30,  9, 20,  0,  8,  6, 1,        NS_SFX_ZAP,    EF_ATTACK },
    /* 1 blade sweep */  { 6, 3, 16, 6, 0, NGB_MV_KNOCKDOWN,                   4, 56, 30, 46,  9, 36, 24,  0,  0, NGB_NONE, NS_SFX_WHOOSH, EF_ATTACK },
};
static const NGBMove ROBODOG_MOVES[] = {
    /* 0 pounce */       { 10, 8, 16, 5, 0, NGB_MV_KNOCKDOWN,                  0, 28, 10, 40,  9, 30, 22, 48,  0, NGB_NONE, NS_SFX_SERVO,  EF_ATTACK },
    /* 1 bite */         { 6, 3, 12, 3, 12, NGB_MV_LOW,                        4, 24,  0, 30,  9, 12,  0,  0,  0, NGB_NONE, NS_SFX_PUNCH,  EF_WINDUP },
};
static const NGBMove WALKER_MOVES[] = {
    /* 0 clamp swing */  { 16, 4, 20, 9, 0, NGB_MV_KNOCKDOWN,                  4, 47, 30, 50,  9, 44, 26,  0,  0, NGB_NONE, NS_SFX_STOMP,  EF_ATTACK },
    /* 1 steam vent */   { 14, 6, 18, 5, 14, 0,                                0, 51, 40, 40,  9, 20,  0,  0,  0, NGB_NONE, NS_SFX_GAS,    EF_WINDUP },
};
static const NGBMove ALIEN_MOVES[] = {
    /* 0 staff thrust */ { 9, 3, 14, 6, 14, 0,                                 6, 57, 40, 30,  9, 22,  0,  6,  0, NGB_NONE, NS_SFX_WARBLE, EF_ATTACK },
    /* 1 energy orb */   { 20, 1, 24, 6, 0, NGB_MV_PROJECTILE,                 10,  0, 50, 12, 10,  0,  0, 52,  0, NGB_NONE, NS_SFX_LASER,  EF_WINDUP },
};

/* hp spd dep jmp hw ht hd wt | moves n | chain air low dash special pummel thr_f thr_b | ai reach aggr attacks n flags score phases n think */
const NGBFighterDef ns_guard = { 40, 20, 14, 56, 14, 86, 7, 3, GUARD_MOVES, 2, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                 NGB_AI_SHIELD, 34, 8, { 0, 1 }, 2, NGB_FD_SHIELD, 200, 0, 0, 0 };
/* drawn hovering (NSLook.lift): the hurt volume covers the air it hovers in */
const NGBFighterDef ns_drone = { 14, 28, 22, 0, 12, 84, 8, 1, DRONE_MOVES, 2, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                 NGB_AI_RANGED, 140, 9, { 0, 1 }, 2, NGB_FD_FLYER, 150, 0, 0, 0 };
const NGBFighterDef ns_runner = { 22, 40, 26, 64, 12, 80, 6, 2, RUNNER_MOVES, 2, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                  NGB_AI_CHARGER, 30, 11, { 0, 1 }, 2, 0, 120, 0, 0, 0 };
const NGBFighterDef ns_bloater = { 70, 14, 10, 40, 18, 96, 9, 7, BLOATER_MOVES, 2, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                   NGB_AI_BRAWLER, 40, 6, { 0, 1 }, 2, NGB_FD_HEAVY, 300, 0, 0, 0 };
const NGBFighterDef ns_android = { 36, 30, 22, 72, 12, 88, 6, 3, ANDROID_MOVES, 2, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                   NGB_AI_BRAWLER, 40, 10, { 0, 1 }, 2, 0, 200, 0, 0, 0 };
const NGBFighterDef ns_robodog = { 24, 44, 30, 88, 16, 48, 8, 2, ROBODOG_MOVES, 2, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                   NGB_AI_JUMPER, 36, 12, { 0, 1 }, 2, 0, 150, 0, 0, 0 };
const NGBFighterDef ns_walker = { 60, 16, 12, 0, 18, 96, 9, 8, WALKER_MOVES, 2, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                  NGB_AI_BRAWLER, 44, 6, { 0, 1 }, 2, NGB_FD_HEAVY, 300, 0, 0, 0 };
const NGBFighterDef ns_alien = { 44, 30, 22, 80, 12, 96, 6, 3, ALIEN_MOVES, 2, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                 NGB_AI_RANGED, 120, 10, { 0, 1 }, 2, 0, 300, 0, 0, 0 };

/* ======================================================================
 * the bosses
 * ==================================================================== */
/*                          su ac re dmg hs flags                                              bx  bw bz bh bd  px  pz lng cnl next      snd             anim */
static const NGBMove BIGFLEX_MOVES[] = {
    /* 0 jab */             { 8, 3, 12, 7, 14, 0,                                               6, 73, 50, 34, 12, 22,  0,  0,  0, NGB_NONE, NS_SFX_PUNCH,   EF_ATTACK },
    /* 1 shoulder charge */ { 20, 30, 24, 10, 0, NGB_MV_KNOCKDOWN | NGB_MV_RUN,                 0, 50, 20, 70, 14, 52, 30, 64,  0, NGB_NONE, NS_SFX_HEAVY,   NS_SHEET2(2) },
    /* 2 ground pound */    { 24, 4, 28, 12, 0, NGB_MV_KNOCKDOWN | NGB_MV_UNBLOCKABLE,         -40, 120, 0, 24, 30, 40, 36,  0,  0, NGB_NONE, NS_SFX_STOMP,   NS_SHEET2(4) },
    /* 3 bear hug */        { 12, 6, 20, 0, 0, NGB_MV_GRAB,                                     4, 40,  0, 70, 12,  0,  0, 24,  0, NGB_NONE, NS_SFX_GRAB,    NS_SHEET2(5) },
    /* 4 flex (an opening) */{ 40, 1, 30, 0, 0, 0,                                              0,  0,  0,  0,  0,  0,  0,  0,  0, NGB_NONE, NS_SFX_SELECT,  NS_SHEET2(0) },
    /* 5 selfie (another) */{ 48, 1, 24, 0, 0, 0,                                               0,  0,  0,  0,  0,  0,  0,  0,  0, NGB_NONE, NS_SFX_PHONE,   NS_SHEET2(1) },
};
static const NGBBossPhase BIGFLEX_PHASES[3] = {
    { 100, 0,  { 0, 1, 4 },    3, 0, NGB_FD_ARMOR | NGB_FD_HEAVY, 1 },
    { 60,  80, { 1, 2, 5, 0 }, 4, 6, NGB_FD_HEAVY, 2 },            /* the armour cracks */
    { 30,  80, { 2, 3, 1, 0 }, 4, 10, NGB_FD_HEAVY, 3 },           /* angry */
};
static const NGBMove ZEROX_MOVES[] = {
    /* 0 IV swing */        { 10, 4, 16, 7, 0, NGB_MV_KNOCKDOWN,                                4, 56, 30, 50, 12, 40, 24,  0,  0, NGB_NONE, NS_SFX_HEAVY,   NS_SHEET2(0) },
    /* 1 sparkle cough */   { 18, 1, 26, 6, 0, NGB_MV_PROJECTILE,                              10,  0, 54, 14, 12,  0,  0, 36,  0, NGB_NONE, NS_SFX_GAS,     NS_SHEET2(1) },
    /* 2 scuttle */         { 12, 24, 20, 6, 0, NGB_MV_KNOCKDOWN | NGB_MV_RUN | NGB_MV_LOW,     0, 40,  0, 30, 12, 34, 18, 60,  0, NGB_NONE, NS_SFX_WHOOSH,  NS_SHEET2(2) },
    /* 3 leap slam */       { 18, 6, 24, 9, 0, NGB_MV_KNOCKDOWN | NGB_MV_UNBLOCKABLE,         -24, 72,  0, 40, 24, 36, 30, 40,  0, NGB_NONE, NS_SFX_STOMP,   NS_SHEET2(4) },
    /* 4 monologue */       { 50, 1, 20, 0, 0, 0,                                               0,  0,  0,  0,  0,  0,  0,  0,  0, NGB_NONE, NS_SFX_TEXT,    NS_SHEET2(6) },
};
static const NGBBossPhase ZEROX_PHASES[3] = {
    { 100, 0,  { 0, 2, 4 },    3, 0, 0, 1 },
    { 60,  80, { 3, 1, 0 },    3, 6, NGB_FD_ARMOR, 2 },
    { 30,  80, { 2, 3, 1, 0 }, 4, 10, NGB_FD_ARMOR, 3 },
};
static const NGBMove M08_MOVES[] = {
    /* 0 overhead smash */  { 26, 4, 30, 14, 0, NGB_MV_KNOCKDOWN | NGB_MV_UNBLOCKABLE,        -10, 110, 0, 40, 26, 44, 34,  0,  0, NGB_NONE, NS_SFX_STOMP,   NS_SHEET2(2) },
    /* 1 sweep */           { 18, 6, 24, 10, 0, NGB_MV_KNOCKDOWN | NGB_MV_LOW,                  0, 100, 0, 40, 18, 50, 26,  0,  0, NGB_NONE, NS_SFX_WHOOSH,  NS_SHEET2(3) },
    /* 2 grab */            { 16, 6, 24, 0, 0, NGB_MV_GRAB,                                     6, 60,  0, 120, 14, 0,  0, 20,  0, NGB_NONE, NS_SFX_GRAB,    NS_SHEET2(4) },
    /* 3 tank throw */      { 24, 1, 30, 10, 0, NGB_MV_PROJECTILE,                             20,  0, 80, 20, 14,  0,  0, 40,  0, NGB_NONE, NS_SFX_CRASH,   NS_SHEET2(5) },
    /* 4 head-down charge */{ 22, 30, 28, 12, 0, NGB_MV_KNOCKDOWN | NGB_MV_RUN,                 0, 60,  0, 120, 18, 56, 30, 56,  0, NGB_NONE, NS_SFX_ROAR,    NS_SHEET2(8) },
    /* 5 roar */            { 36, 1, 20, 0, 0, 0,                                               0,  0,  0,  0,  0,  0,  0,  0,  0, NGB_NONE, NS_SFX_ROAR,    NS_SHEET2(0) },
};
static const NGBBossPhase M08_PHASES[3] = {
    { 100, 0,  { 0, 1, 5 },    3, 0, NGB_FD_HEAVY | NGB_FD_ARMOR, 1 },
    { 60,  90, { 3, 0, 2, 1 }, 4, 4, NGB_FD_HEAVY | NGB_FD_ARMOR, 2 },     /* unstable */
    { 30,  90, { 4, 0, 3, 2 }, 4, 8, NGB_FD_HEAVY, 3 },
};
static const NGBMove BEHEMOTH_MOVES[] = {
    /* 0 horn charge */     { 24, 34, 26, 12, 0, NGB_MV_KNOCKDOWN | NGB_MV_RUN | NGB_MV_UNBLOCKABLE, 0, 64, 0, 100, 18, 60, 34, 72, 0, NGB_NONE, NS_SFX_ROAR, NS_SHEET2(1) },
    /* 1 leap slam */       { 26, 6, 28, 13, 0, NGB_MV_KNOCKDOWN | NGB_MV_UNBLOCKABLE,        -50, 140, 0, 30, 34, 44, 36, 36,  0, NGB_NONE, NS_SFX_STOMP,   NS_SHEET2(3) },
    /* 2 car throw */       { 30, 1, 30, 12, 0, NGB_MV_PROJECTILE,                             20,  0, 90, 24, 16,  0,  0, 44,  0, NGB_NONE, NS_SFX_CRASH,   NS_SHEET2(4) },
    /* 3 claw swipe */      { 14, 5, 20, 9, 0, NGB_MV_KNOCKDOWN,                                4, 80, 30, 80, 16, 46, 26,  0,  0, NGB_NONE, NS_SFX_WHOOSH,  NS_SHEET2(8) },
    /* 4 chest beat */      { 40, 1, 24, 0, 0, 0,                                               0,  0,  0,  0,  0,  0,  0,  0,  0, NGB_NONE, NS_SFX_ROAR,    NS_SHEET2(0) },
};
static const NGBBossPhase BEHEMOTH_PHASES[3] = {
    { 100, 0,  { 3, 0, 4 },    3, 0, NGB_FD_HEAVY | NGB_FD_ARMOR, 1 },
    { 60,  90, { 2, 3, 0 },    3, 4, NGB_FD_HEAVY, 2 },            /* the armour falls off */
    { 30,  90, { 1, 0, 2, 3 }, 4, 8, NGB_FD_HEAVY, 3 },
};
static const NGBMove PROXY_MOVES[] = {
    /* 0 arm cannon */      { 16, 1, 22, 7, 0, NGB_MV_PROJECTILE,                              12,  0, 56, 12, 12,  0,  0, 64,  0, NGB_NONE, NS_SFX_LASER,   NS_SHEET2(0) },
    /* 1 sabre slash */     { 9, 4, 16, 9, 0, NGB_MV_KNOCKDOWN,                                 4, 60, 30, 56, 14, 44, 26,  0,  0, NGB_NONE, NS_SFX_ZAP,     NS_SHEET2(1) },
    /* 2 sabre dash */      { 14, 18, 20, 8, 0, NGB_MV_KNOCKDOWN | NGB_MV_RUN,                  0, 56, 20, 60, 14, 40, 24, 72,  0, NGB_NONE, NS_SFX_ZAP,     NS_SHEET2(3) },
    /* 3 smug taunt */      { 44, 1, 20, 0, 0, 0,                                               0,  0,  0,  0,  0,  0,  0,  0,  0, NGB_NONE, NS_SFX_SERVO,   NS_SHEET2(4) },
    /* 4 jet stomp */       { 20, 6, 22, 9, 0, NGB_MV_KNOCKDOWN | NGB_MV_UNBLOCKABLE,         -20, 70,  0, 40, 20, 30, 30, 32,  0, NGB_NONE, NS_SFX_STOMP,   NS_SHEET2(2) },
};
static const NGBBossPhase PROXY_PHASES[3] = {
    { 100, 0,  { 0, 1, 3 },    3, 0, NGB_FD_ARMOR, 1 },
    { 60,  80, { 2, 0, 1, 3 }, 4, 6, NGB_FD_ARMOR, 2 },            /* calls in drones */
    { 30,  80, { 4, 2, 0, 1 }, 4, 10, 0, 3 },
};
static const NGBMove OVERMIND_MOVES[] = {
    /* 0 arm slam */        { 26, 4, 30, 14, 0, NGB_MV_KNOCKDOWN | NGB_MV_UNBLOCKABLE,        -20, 140, 0, 40, 30, 50, 34,  0,  0, NGB_NONE, NS_SFX_STOMP,   3 },
    /* 1 low sweep */       { 20, 6, 26, 10, 0, NGB_MV_KNOCKDOWN | NGB_MV_LOW,                -40, 180, 0, 24, 30, 54, 24,  0,  0, NGB_NONE, NS_SFX_WHOOSH,  4 },
    /* 2 face laser */      { 28, 1, 30, 11, 0, NGB_MV_PROJECTILE,                              0,  0, 70, 14, 14,  0,  0, 72,  0, NGB_NONE, NS_SFX_LASER,   6 },
};
static const NGBBossPhase OVERMIND_PHASES[3] = {
    { 100, 0,   { 0, 1, 2 },    3, 0, NGB_FD_HEAVY | NGB_FD_ARMOR, 1 },
    { 66,  100, { 2, 1, 0, 2 }, 4, 0, NGB_FD_HEAVY | NGB_FD_ARMOR, 2 },   /* drones come out */
    { 33,  100, { 0, 2, 1, 2 }, 4, 0, NGB_FD_HEAVY, 3 },                  /* the core is open */
};
static const NGBMove ARCHIVIST_MOVES[] = {
    /* 0 glide strike */    { 10, 16, 20, 8, 0, NGB_MV_KNOCKDOWN | NGB_MV_RUN,                  0, 50, 20, 60, 14, 40, 24, 64,  0, NGB_NONE, NS_SFX_WARBLE,  NS_SHEET2(2) },
    /* 1 crystal beam */    { 18, 1, 24, 8, 0, NGB_MV_PROJECTILE,                              14,  0, 60, 12, 12,  0,  0, 68,  0, NGB_NONE, NS_SFX_LASER,   NS_SHEET2(5) },
    /* 2 core shockwave */  { 22, 6, 26, 11, 0, NGB_MV_KNOCKDOWN | NGB_MV_UNBLOCKABLE,        -60, 160, 0, 60, 30, 50, 30,  0,  0, NGB_NONE, NS_SFX_ALARM,   NS_SHEET3(2) },
    /* 3 crystal blades */  { 8, 4, 14, 9, 14, 0,                                               6, 65, 30, 50, 12, 30,  0, 10,  0, NGB_NONE, NS_SFX_ZAP,     NS_SHEET3(3) },
    /* 4 many arms */       { 16, 10, 22, 12, 0, NGB_MV_KNOCKDOWN,                              0, 90, 10, 90, 20, 50, 30, 20,  0, NGB_NONE, NS_SFX_WARBLE,  NS_SHEET3(7) },
    /* 5 giant beam */      { 30, 1, 34, 14, 0, NGB_MV_PROJECTILE | NGB_MV_UNBLOCKABLE,        16,  0, 56, 20, 16,  0,  0, 80,  0, NGB_NONE, NS_SFX_LASER,   NS_SHEET3(8) },
};
static const NGBBossPhase ARCHIVIST_PHASES[3] = {
    { 100, 0,   { 0, 1 },       2, 0, NGB_FD_ARMOR | NGB_FD_HEAVY, 1 },   /* the armoured body */
    { 66,  100, { 2, 3, 1 },    3, 6, NGB_FD_HEAVY, 2 },                  /* the core, exposed */
    { 33,  100, { 4, 5, 2, 3 }, 4, 10, NGB_FD_HEAVY, 3 },                 /* the command form */
};

/* hp spd dep jmp hw ht hd wt | moves n | chain air low dash special pummel thr_f thr_b | ai reach aggr attacks n flags score phases n think */
const NGBFighterDef ns_bigflex = { 200, 22, 14, 56, 18, 112, 9, 8, BIGFLEX_MOVES, 6, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                   NGB_AI_BOSS, 44, 11, { 0, 1, 4 }, 3, NGB_FD_HEAVY, 8000, BIGFLEX_PHASES, 3, 0 };
const NGBFighterDef ns_zerox = { 190, 28, 20, 88, 12, 96, 7, 3, ZEROX_MOVES, 5, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                 NGB_AI_BOSS, 50, 12, { 0, 2, 4 }, 3, 0, 10000, ZEROX_PHASES, 3, 0 };
const NGBFighterDef ns_m08 = { 240, 16, 10, 40, 30, 168, 14, 12, M08_MOVES, 6, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                               NGB_AI_BOSS, 64, 9, { 0, 1, 5 }, 3, NGB_FD_HEAVY, 12000, M08_PHASES, 3, 0 };
const NGBFighterDef ns_behemoth = { 250, 18, 12, 64, 32, 160, 14, 12, BEHEMOTH_MOVES, 5, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                    NGB_AI_BOSS, 70, 10, { 3, 0, 4 }, 3, NGB_FD_HEAVY, 14000, BEHEMOTH_PHASES, 3, 0 };
const NGBFighterDef ns_proxy = { 220, 30, 22, 96, 12, 98, 7, 4, PROXY_MOVES, 5, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                 NGB_AI_BOSS, 56, 12, { 0, 1, 3 }, 3, 0, 16000, PROXY_PHASES, 3, 0 };
const NGBFighterDef ns_overmind = { 255, 6, 4, 0, 40, 200, 20, 20, OVERMIND_MOVES, 3, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                    NGB_AI_BOSS, 110, 9, { 0, 1, 2 }, 3, NGB_FD_HEAVY, 20000, OVERMIND_PHASES, 3, 0 };
const NGBFighterDef ns_archivist = { 255, 32, 24, 96, 12, 156, 7, 4, ARCHIVIST_MOVES, 6, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                     NGB_AI_BOSS, 60, 13, { 0, 1 }, 2, NGB_FD_HEAVY, 30000, ARCHIVIST_PHASES, 3, 0 };

/* How each one is drawn. Overmind Prime has ten pictures: his phases pick
 * among them; the Archivist's third sheet is his last two forms. */
static const uint8_t OVERMIND_FRAMES[3][EF_COUNT] = {
    /* stance walk1 walk2 walk3 windup attack hurt knocked down getup */
    { 0, 1, 0, 1, 2, 3, 7, 7, 9, 0 },
    { 1, 0, 1, 0, 5, 3, 7, 7, 9, 1 },
    { 8, 8, 8, 8, 5, 4, 7, 7, 9, 8 },
};
static const uint8_t ARCHIVIST_FRAMES[3][EF_COUNT] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
    { 20, 21, 21, 21, 20, 23, 24, 24, 24, 20 },
    { 25, 25, 25, 25, 26, 27, 29, 29, 29, 25 },
};

const NSLook ns_looks[] = {
    { &ns_heroes[NS_JAY], NS_ART_F_JAY, 0, 0, "JAY" }, { &ns_heroes[NS_ZOE], NS_ART_F_ZOE, 0, 0, "ZOE" },
    { &ns_heroes[NS_KAI], NS_ART_F_KAI, 0, 0, "KAI" },
    { &ns_heroes_space[NS_JAY], NS_ART_F_JAY, 0, 0, "JAY" }, { &ns_heroes_space[NS_ZOE], NS_ART_F_ZOE, 0, 0, "ZOE" },
    { &ns_heroes_space[NS_KAI], NS_ART_F_KAI, 0, 0, "KAI" },
    { &ns_thief, NS_ART_F_THIEF, 0, 0, "SNATCH" }, { &ns_skater, NS_ART_F_SKATER, 0, 0, "KICKFLIP" },
    { &ns_biker, NS_ART_F_BIKER, 0, 0, "DIESEL" }, { &ns_rico, NS_ART_F_RICO, 0, 0, "RICO" },
    { &ns_guard, NS_ART_F_GUARD, 0, 0, "RENT-A-COP" }, { &ns_drone, NS_ART_F_DRONE, 36, 0, "DRONE" },
    { &ns_runner, NS_ART_F_RUNNER, 0, 0, "VIRAL RUNNER" }, { &ns_bloater, NS_ART_F_BLOATER, 0, 0, "BLOATER" },
    { &ns_android, NS_ART_F_ANDROID, 0, 0, "CLONE UNIT" }, { &ns_robodog, NS_ART_F_ROBODOG, 0, 0, "ROBO-HOUND" },
    { &ns_walker, NS_ART_F_WALKER, 0, 0, "SENTRY WALKER" }, { &ns_alien, NS_ART_F_ALIEN, 0, 0, "VISITOR" },
    { &ns_bigflex, NS_ART_F_BIGFLEX, 0, 0, "BIG FLEX" }, { &ns_zerox, NS_ART_F_ZEROX, 0, 0, "PATIENT ZERO-X" },
    { &ns_m08, NS_ART_F_M08, 0, 0, "PROTOTYPE M-08" }, { &ns_behemoth, NS_ART_F_BEHEMOTH, 0, 0, "IRON BEHEMOTH" },
    { &ns_proxy, NS_ART_F_PROXY, 0, 0, "GENERAL PROXY" },
    { &ns_overmind, NS_ART_F_OVERMIND, 0, OVERMIND_FRAMES, "OVERMIND PRIME" },
    { &ns_archivist, NS_ART_F_ARCHIVIST, 0, ARCHIVIST_FRAMES, "THE ARCHIVIST" },
};
const uint8_t ns_nlooks = (uint8_t)(sizeof(ns_looks) / sizeof(ns_looks[0]));

/* Weapons lying about, by stage (the last one is always the scooter's wreck). */
static const NGBMove PIPE_SWING = { 6, 4, 14, 8, 0, NGB_MV_KNOCKDOWN, 4, 56, 20, 50, 12, 42, 28, 0, 0, NGB_NONE, NS_SFX_HEAVY, HF_KICK };
static const NGBWeaponDef WEAPONS[2] = { { &PIPE_SWING, 5, 10, 0 }, { &WRECK_SWING, 3, 12, 1 } };

/* ======================================================================
 * stage 2: the city underpass (and the scooter chase)
 * ==================================================================== */
static const NGBFighterDef *const S2_ENEMIES[5] = { &ns_thief, &ns_biker, &ns_guard, &ns_drone, &ns_bigflex };
enum { S2_THIEF = 0, S2_BIKER, S2_GUARD, S2_DRONE, S2_FLEX };
static const NGBWave S2_WAVES[] = {
    { 40,   0,                             3, { S2_BIKER, S2_THIEF, S2_BIKER },              0x05, 40 },
    { 260,  0,                             3, { S2_BIKER, S2_BIKER, S2_DRONE },              0x02, 30 },
    { 480,  NGB_WAVE_LOCK,                 4, { S2_GUARD, S2_THIEF, S2_DRONE, S2_THIEF },    0x0A, 30 },
    { 700,  0,                             4, { S2_BIKER, S2_BIKER, S2_THIEF, S2_BIKER },    0x09, 24 },
    { 900,  NGB_WAVE_LOCK,                 4, { S2_GUARD, S2_GUARD, S2_DRONE, S2_DRONE },    0x06, 30 },
    { 1120, NGB_WAVE_LOCK | NGB_WAVE_BOSS, 1, { S2_FLEX },                                   0x00,  0 },
};
static const NGBPropSpawn S2_PROPS[] = {
    { NGB_PROP_VEHICLE, 170, 140, 0, 0 },     /* a scooter at the start: the chase */
    { NGB_PROP_RAMP,    176, 380, 0, 0 },
    { NGB_PROP_VEHICLE, 190, 560, 0, 0 },
    { NGB_PROP_RAMP,    160, 640, 0, 0 },
    { NGB_PROP_CRATE,   176, 800, NGB_PROP_FOOD, 2 },
    { NGB_PROP_WEAPON,  172, 860, 0, 0 },
    { NGB_PROP_BARREL,  178, 1000, NGB_PROP_SCORE, 2 },
    { NGB_PROP_CRATE,   170, 1060, NGB_PROP_FOOD, 2 },
};
static const NGBStage S2 = { 1440, 140, 206, 320, S2_WAVES, S2_PROPS, S2_ENEMIES, WEAPONS,
    sizeof(S2_WAVES) / sizeof(S2_WAVES[0]), sizeof(S2_PROPS) / sizeof(S2_PROPS[0]), 5, 2,
    2, 5, NGB_SF_NO_BACKTRACK | NGB_SF_AUTO_PICKUP | NGB_SF_WALK_ON, 7, 128, 5, 4, 10, 10, 18 };

/* ======================================================================
 * stage 3: the viral outbreak
 * ==================================================================== */
static const NGBFighterDef *const S3_ENEMIES[5] = { &ns_runner, &ns_bloater, &ns_guard, &ns_drone, &ns_zerox };
enum { S3_RUN = 0, S3_BLOAT, S3_GUARD, S3_DRONE, S3_ZEROX };
static const NGBWave S3_WAVES[] = {
    { 40,   0,                             3, { S3_RUN, S3_RUN, S3_RUN },                    0x02, 20 },
    { 220,  NGB_WAVE_LOCK,                 4, { S3_GUARD, S3_DRONE, S3_RUN, S3_RUN },        0x0C, 30 },
    { 460,  0,                             3, { S3_BLOAT, S3_RUN, S3_RUN },                  0x04, 30 },
    { 680,  NGB_WAVE_LOCK,                 5, { S3_RUN, S3_RUN, S3_BLOAT, S3_RUN, S3_RUN },  0x15, 20 },
    { 900,  NGB_WAVE_LOCK,                 4, { S3_GUARD, S3_GUARD, S3_DRONE, S3_BLOAT },    0x0A, 30 },
    { 1120, NGB_WAVE_LOCK | NGB_WAVE_BOSS, 1, { S3_ZEROX },                                  0x00,  0 },
};
static const NGBPropSpawn S3_PROPS[] = {
    { NGB_PROP_CRATE,   176, 300, NGB_PROP_FOOD, 2 },
    { NGB_PROP_WEAPON,  170, 420, 0, 0 },
    { NGB_PROP_BARREL,  178, 560, NGB_PROP_SCORE, 2 },
    { NGB_PROP_HAZARD,  150, 760, 0, 0 },
    { NGB_PROP_CRATE,   172, 980, NGB_PROP_FOOD, 2 },
    { NGB_PROP_VEHICLE, 190, 1000, 0, 0 },
};
static const NGBStage S3 = { 1440, 140, 206, 320, S3_WAVES, S3_PROPS, S3_ENEMIES, WEAPONS,
    sizeof(S3_WAVES) / sizeof(S3_WAVES[0]), sizeof(S3_PROPS) / sizeof(S3_PROPS[0]), 5, 2,
    2, 5, NGB_SF_NO_BACKTRACK | NGB_SF_AUTO_PICKUP | NGB_SF_WALK_ON, 6, 96, 4, 3, 8, 10, 18 };

/* ======================================================================
 * stage 4: the clone lab
 * ==================================================================== */
static const NGBFighterDef *const S4_ENEMIES[4] = { &ns_android, &ns_robodog, &ns_runner, &ns_m08 };
enum { S4_AND = 0, S4_DOG, S4_RUN, S4_M08 };
static const NGBWave S4_WAVES[] = {
    { 40,   0,                             2, { S4_AND, S4_AND },                            0x00, 40 },
    { 240,  NGB_WAVE_LOCK,                 4, { S4_DOG, S4_DOG, S4_AND, S4_RUN },            0x05, 24 },
    { 480,  0,                             3, { S4_RUN, S4_RUN, S4_DOG },                    0x02, 24 },
    { 700,  NGB_WAVE_LOCK,                 5, { S4_AND, S4_AND, S4_DOG, S4_DOG, S4_AND },    0x0A, 24 },
    { 920,  0,                             3, { S4_DOG, S4_RUN, S4_AND },                    0x04, 30 },
    { 1120, NGB_WAVE_LOCK | NGB_WAVE_BOSS, 1, { S4_M08 },                                    0x00,  0 },
};
static const NGBPropSpawn S4_PROPS[] = {
    { NGB_PROP_CRATE,   176, 320, NGB_PROP_FOOD, 2 },
    { NGB_PROP_HAZARD,  190, 560, 0, 0 },
    { NGB_PROP_WEAPON,  170, 620, 0, 0 },
    { NGB_PROP_BARREL,  172, 840, NGB_PROP_SCORE, 2 },
    { NGB_PROP_CRATE,   176, 1000, NGB_PROP_FOOD, 2 },
};
static const NGBStage S4 = { 1440, 140, 206, 320, S4_WAVES, S4_PROPS, S4_ENEMIES, WEAPONS,
    sizeof(S4_WAVES) / sizeof(S4_WAVES[0]), sizeof(S4_PROPS) / sizeof(S4_PROPS[0]), 4, 2,
    2, 5, NGB_SF_NO_BACKTRACK | NGB_SF_AUTO_PICKUP | NGB_SF_WALK_ON, 6, 96, 4, 3, 8, 10, 18 };

/* ======================================================================
 * stage 5: concrete jungle
 * ==================================================================== */
static const NGBFighterDef *const S5_ENEMIES[5] = { &ns_runner, &ns_android, &ns_bloater, &ns_robodog, &ns_behemoth };
enum { S5_RUN = 0, S5_AND, S5_BLOAT, S5_DOG, S5_BEH };
static const NGBWave S5_WAVES[] = {
    { 40,   0,                             3, { S5_DOG, S5_DOG, S5_RUN },                    0x02, 24 },
    { 260,  0,                             3, { S5_RUN, S5_RUN, S5_DOG },                    0x05, 24 },
    { 480,  NGB_WAVE_LOCK,                 4, { S5_BLOAT, S5_AND, S5_DOG, S5_RUN },          0x0A, 30 },
    { 720,  0,                             4, { S5_DOG, S5_DOG, S5_DOG, S5_RUN },            0x0B, 20 },
    { 920,  NGB_WAVE_LOCK,                 4, { S5_BLOAT, S5_BLOAT, S5_AND, S5_AND },        0x06, 30 },
    { 1120, NGB_WAVE_LOCK | NGB_WAVE_BOSS, 1, { S5_BEH },                                    0x00,  0 },
};
static const NGBPropSpawn S5_PROPS[] = {
    { NGB_PROP_VEHICLE, 180, 200, 0, 0 },
    { NGB_PROP_RAMP,    170, 420, 0, 0 },
    { NGB_PROP_HAZARD,  200, 520, 0, 0 },
    { NGB_PROP_CRATE,   172, 600, NGB_PROP_FOOD, 2 },
    { NGB_PROP_VEHICLE, 150, 760, 0, 0 },
    { NGB_PROP_WEAPON,  176, 860, 0, 0 },
    { NGB_PROP_BARREL,  176, 1040, NGB_PROP_FOOD, 2 },
};
static const NGBStage S5 = { 1440, 140, 206, 320, S5_WAVES, S5_PROPS, S5_ENEMIES, WEAPONS,
    sizeof(S5_WAVES) / sizeof(S5_WAVES[0]), sizeof(S5_PROPS) / sizeof(S5_PROPS[0]), 5, 2,
    2, 5, NGB_SF_NO_BACKTRACK | NGB_SF_AUTO_PICKUP | NGB_SF_WALK_ON, 7, 120, 5, 4, 10, 10, 18 };

/* ======================================================================
 * stage 6: the machine city
 * ==================================================================== */
static const NGBFighterDef *const S6_ENEMIES[4] = { &ns_walker, &ns_android, &ns_drone, &ns_proxy };
enum { S6_WALK = 0, S6_AND, S6_DRONE, S6_PROXY };
static const NGBWave S6_WAVES[] = {
    { 40,   0,                             3, { S6_DRONE, S6_DRONE, S6_AND },                0x01, 30 },
    { 260,  NGB_WAVE_LOCK,                 3, { S6_WALK, S6_AND, S6_AND },                   0x02, 30 },
    { 500,  0,                             4, { S6_DRONE, S6_DRONE, S6_DRONE, S6_AND },      0x05, 24 },
    { 720,  NGB_WAVE_LOCK,                 4, { S6_WALK, S6_WALK, S6_AND, S6_DRONE },        0x0A, 30 },
    { 920,  NGB_WAVE_LOCK,                 4, { S6_AND, S6_AND, S6_AND, S6_DRONE },          0x05, 24 },
    { 1120, NGB_WAVE_LOCK | NGB_WAVE_BOSS, 1, { S6_PROXY },                                  0x00,  0 },
};
static const NGBPropSpawn S6_PROPS[] = {
    { NGB_PROP_HAZARD,  160, 360, 0, 0 },
    { NGB_PROP_CRATE,   178, 420, NGB_PROP_FOOD, 2 },
    { NGB_PROP_WEAPON,  172, 600, 0, 0 },
    { NGB_PROP_HAZARD,  196, 820, 0, 0 },
    { NGB_PROP_BARREL,  170, 900, NGB_PROP_SCORE, 2 },
    { NGB_PROP_CRATE,   176, 1040, NGB_PROP_FOOD, 2 },
};
static const NGBStage S6 = { 1440, 140, 206, 320, S6_WAVES, S6_PROPS, S6_ENEMIES, WEAPONS,
    sizeof(S6_WAVES) / sizeof(S6_WAVES[0]), sizeof(S6_PROPS) / sizeof(S6_PROPS[0]), 4, 2,
    2, 5, NGB_SF_NO_BACKTRACK | NGB_SF_AUTO_PICKUP | NGB_SF_WALK_ON, 6, 96, 4, 3, 8, 10, 18 };

/* ======================================================================
 * stage 7: the rogue AI
 * ==================================================================== */
static const NGBFighterDef *const S7_ENEMIES[5] = { &ns_walker, &ns_android, &ns_drone, &ns_guard, &ns_overmind };
enum { S7_WALK = 0, S7_AND, S7_DRONE, S7_GUARD, S7_OVER };
static const NGBWave S7_WAVES[] = {
    { 40,   0,                             3, { S7_GUARD, S7_AND, S7_DRONE },                0x02, 30 },
    { 260,  NGB_WAVE_LOCK,                 4, { S7_GUARD, S7_GUARD, S7_DRONE, S7_DRONE },    0x0A, 30 },
    { 500,  NGB_WAVE_LOCK,                 4, { S7_WALK, S7_AND, S7_AND, S7_DRONE },         0x05, 24 },
    { 740,  0,                             4, { S7_AND, S7_AND, S7_AND, S7_AND },            0x0A, 20 },
    { 940,  NGB_WAVE_LOCK,                 5, { S7_WALK, S7_GUARD, S7_AND, S7_DRONE, S7_DRONE }, 0x15, 24 },
    { 1120, NGB_WAVE_LOCK | NGB_WAVE_BOSS, 1, { S7_OVER },                                   0x00,  0 },
};
static const NGBPropSpawn S7_PROPS[] = {
    { NGB_PROP_CRATE,   180, 360, NGB_PROP_FOOD, 2 },
    { NGB_PROP_WEAPON,  170, 460, 0, 0 },
    { NGB_PROP_HAZARD,  196, 680, 0, 0 },
    { NGB_PROP_BARREL,  172, 860, NGB_PROP_FOOD, 2 },
    { NGB_PROP_CRATE,   176, 1060, NGB_PROP_FOOD, 2 },
};
static const NGBStage S7 = { 1440, 140, 206, 320, S7_WAVES, S7_PROPS, S7_ENEMIES, WEAPONS,
    sizeof(S7_WAVES) / sizeof(S7_WAVES[0]), sizeof(S7_PROPS) / sizeof(S7_PROPS[0]), 5, 2,
    2, 5, NGB_SF_NO_BACKTRACK | NGB_SF_AUTO_PICKUP | NGB_SF_WALK_ON, 6, 96, 4, 3, 8, 10, 18 };

/* ======================================================================
 * stage 8: the signal beyond Earth
 * ==================================================================== */
static const NGBFighterDef *const S8_ENEMIES[4] = { &ns_alien, &ns_android, &ns_drone, &ns_archivist };
enum { S8_ALIEN = 0, S8_AND, S8_DRONE, S8_ARCH };
static const NGBWave S8_WAVES[] = {
    { 40,   0,                             2, { S8_ALIEN, S8_DRONE },                        0x02, 40 },
    { 260,  NGB_WAVE_LOCK,                 4, { S8_ALIEN, S8_ALIEN, S8_DRONE, S8_DRONE },    0x0A, 30 },
    { 500,  NGB_WAVE_LOCK,                 4, { S8_AND, S8_AND, S8_ALIEN, S8_DRONE },        0x05, 24 },
    { 740,  0,                             3, { S8_ALIEN, S8_ALIEN, S8_ALIEN },              0x02, 30 },
    { 940,  NGB_WAVE_LOCK,                 5, { S8_ALIEN, S8_AND, S8_ALIEN, S8_DRONE, S8_DRONE }, 0x15, 24 },
    { 1120, NGB_WAVE_LOCK | NGB_WAVE_BOSS, 1, { S8_ARCH },                                   0x00,  0 },
};
static const NGBPropSpawn S8_PROPS[] = {
    { NGB_PROP_CRATE,   176, 320, NGB_PROP_FOOD, 2 },
    { NGB_PROP_HAZARD,  160, 600, 0, 0 },
    { NGB_PROP_WEAPON,  178, 660, 0, 0 },
    { NGB_PROP_BARREL,  174, 880, NGB_PROP_FOOD, 2 },
    { NGB_PROP_CRATE,   172, 1060, NGB_PROP_FOOD, 2 },
};
static const NGBStage S8 = { 1440, 140, 206, 320, S8_WAVES, S8_PROPS, S8_ENEMIES, WEAPONS,
    sizeof(S8_WAVES) / sizeof(S8_WAVES[0]), sizeof(S8_PROPS) / sizeof(S8_PROPS[0]), 4, 2,
    2, 5, NGB_SF_NO_BACKTRACK | NGB_SF_AUTO_PICKUP | NGB_SF_WALK_ON, 6, 96, 4, 3, 8, 10, 18 };

/* ======================================================================
 * the gallery
 * ==================================================================== */
const NSGalleryEntry ns_gallery[] = {
    { NS_ART_F_JAY, 0, "JAY 'NO CAP' CARTER", "19. LOUD, LOYAL, VERY EMOTIONAL ABOUT SNACKS." },
    { NS_ART_F_ZOE, 0, "ZOE 'MAIN CHARACTER' PARK", "19. STREAMS EVERYTHING. CRIES AT DOG VIDEOS." },
    { NS_ART_F_KAI, 0, "KAI 'AURA' REYES", "18. FIXES PHONES. FIXES PEOPLE. NOT HIS SLEEP." },
    { NS_ART_F_THIEF, 1, "SNATCH", "STEALS PHONES. CAN'T WORK A SINGLE ONE." },
    { NS_ART_F_SKATER, 1, "KICKFLIP", "HAS NEVER LANDED A KICKFLIP. NEVER STOPS TRYING." },
    { NS_ART_F_BIKER, 1, "DIESEL", "E-BIKE COURIER. ONLY DELIVERS STOLEN PHONES." },
    { NS_ART_F_RICO, 1, "RICO 'SWIPE' MENDOZA", "FASTEST HANDS IN NEW VISTA. SLOWEST APOLOGIES." },
    { NS_ART_F_GUARD, 2, "BADGE", "COMPANY SECURITY. PAID HOURLY. FEELS IT." },
    { NS_ART_F_DRONE, 2, "PEEPER", "A FLYING CAMERA. ALWAYS FINDS YOUR BAD SIDE." },
    { NS_ART_F_BIGFLEX, 2, "BIG FLEX", "SIX PACK. TWELVE FOLLOWERS. THINKS IT'S THE REVERSE." },
    { NS_ART_F_RUNNER, 3, "THE INFECTED", "NOT MONSTERS. JUST HAVING A REALLY BAD DAY." },
    { NS_ART_F_BLOATER, 3, "HARD HAT", "CAME TO FIX A PIPE. STAYED FOR THE VIRUS." },
    { NS_ART_F_ZEROX, 3, "PATIENT ZERO-X", "FEELING MUCH BETTER. SENT HOME WITH SOUP." },
    { NS_ART_F_ANDROID, 4, "UNIT", "LAB SECURITY. RATES ITSELF FIVE STARS." },
    { NS_ART_F_ROBODOG, 4, "BYTE", "GOOD BOY. CYBERNETIC BOY. BITES." },
    { NS_ART_F_M08, 4, "PROTOTYPE M-08", "TEN FEET TALL. TWO YEARS OLD. NEEDS A NAP." },
    { NS_ART_F_BEHEMOTH, 5, "IRON BEHEMOTH", "ESCAPED THE LAB. WANTS TO BE A HOUSE CAT." },
    { NS_ART_F_WALKER, 6, "STOMPY", "BUILT TO CARRY BOXES. CARRIES GRUDGES." },
    { NS_ART_F_PROXY, 6, "GENERAL PROXY", "LEARNED SLANG FROM COMMENT SECTIONS. IT SHOWS." },
    { NS_ART_F_OVERMIND, 7, "OVERMIND PRIME", "KEPT EIGHT BILLION MEMORIES. NEVER MADE ONE." },
    { NS_ART_F_ALIEN, 8, "GLORP", "FOUR EYES. ZERO CHILL. A VERY SERIOUS STAFF." },
    { NS_ART_F_ARCHIVIST, 8, "THE ARCHIVIST", "KEEPS TEN THOUSAND WORLDS' MEMORIES. HAS NONE." },
};
const uint8_t ns_gallery_n = (uint8_t)(sizeof(ns_gallery) / sizeof(ns_gallery[0]));

/* ======================================================================
 * the scenes
 * ==================================================================== */
#define N NS_SPK_NARRATOR
#define H NS_SPK_HERO
#define END { 0, 0 }
static const NSLine S2_IN_JAY[] = {
    { N, "DOWNTOWN. UNDER THE HIGHWAY. 9:02 PM." },
    { NS_SPK_JAY, "WHO BUYS A CRACKED PHONE IN A LAB COAT? WHO EVEN WEARS A LAB COAT OUTSIDE?" },
    { NS_SPK_JAY, "AMARA SENT ME A VOICE NOTE THIS MORNING. I DIDN'T EVEN PLAY IT YET." },
    { NS_SPK_JAY, "...OK. FOCUS. FIND THE COAT. HIT THE COAT." }, END };
static const NSLine S2_IN_ZOE[] = {
    { N, "DOWNTOWN. UNDER THE HIGHWAY. 9:02 PM." },
    { NS_SPK_ZOE, "CHAT, WE'RE UNDER A HIGHWAY AT NIGHT. THIS IS SO NOT ON BRAND." },
    { NS_SPK_ZOE, "THEO'S LAST VIDEO WAS SIX MINUTES OF HIM BURNING TOAST. I NEED THAT VIDEO." },
    { NS_SPK_ZOE, "SOMEBODY'S GETTING RATIOED TONIGHT. IN PERSON." }, END };
static const NSLine S2_IN_KAI[] = {
    { N, "DOWNTOWN. UNDER THE HIGHWAY. 9:02 PM." },
    { NS_SPK_KAI, "I PUT A TRACKER IN MY PHONE AFTER THE THIRD REPAIR. TRUST ISSUES." },
    { NS_SPK_KAI, "IT PINGED ONCE. HERE. MOVING FAST." },
    { NS_SPK_KAI, "THERE'S A SCOOTER. THERE'S ALWAYS A SCOOTER." }, END };
static const NSLine S2_BOSS[] = {
    { NS_SPK_BIGFLEX, "HOLD UP. NOBODY PASSES BIG FLEX. NOT EVEN... HOLD ON, LET ME FIND MY ANGLE." },
    { N, "*CLICK*" },
    { H, "DID YOU JUST TAKE A SELFIE IN THE MIDDLE OF A THREAT?" },
    { NS_SPK_BIGFLEX, "THE CLIENT PAYS ME TO DELIVER. THE FANS PAY ME TO FLEX. NOW SQUARE UP." }, END };
static const NSLine S2_OUT[] = {
    { NS_SPK_BIGFLEX, "OW. OW. MY PROTEIN SHAKE. YOU MADE ME DROP MY PROTEIN SHAKE." },
    { NS_SPK_BIGFLEX, "FINE! THE PHONE WENT TO SYNAPSE. THE CLINIC NEXT TO ST. LUCIA HOSPITAL." },
    { N, "A SECURITY GUARD STUMBLES OUT OF THE DARK. HE IS GREEN. HE IS... VIBRATING." },
    { H, "BRO WHY IS HE GREEN." },
    { N, "EVERY SIREN IN THE CITY STARTS AT ONCE." }, END };

static const NSLine S3_IN_JAY[] = {
    { N, "ST. LUCIA HOSPITAL. QUARANTINE ZONE." },
    { NS_SPK_JAY, "OK SO THE CITY IS SICK, AND MY PHONE IS SOMEWHERE IN THE MIDDLE OF IT." },
    { NS_SPK_JAY, "THESE PEOPLE AREN'T MONSTERS. THEY'RE JUST HAVING THE WORST DAY OF THEIR LIVES." },
    { NS_SPK_JAY, "GENTLE PUNCHES. NO CAP. GENTLE-ISH." }, END };
static const NSLine S3_IN_ZOE[] = {
    { N, "ST. LUCIA HOSPITAL. QUARANTINE ZONE." },
    { NS_SPK_ZOE, "IF I CATCH A LAB VIRUS IN THESE TRAINERS I AM SUING SOMEONE." },
    { NS_SPK_ZOE, "THE SICK ONES SNAP OUT OF IT WHEN THEY GO DOWN. SO WE HELP THEM. AGGRESSIVELY." }, END };
static const NSLine S3_IN_KAI[] = {
    { N, "ST. LUCIA HOSPITAL. QUARANTINE ZONE." },
    { NS_SPK_KAI, "THE NEWS SAYS THE VIRUS MAKES PEOPLE FORGET THINGS. NAMES. FACES." },
    { NS_SPK_KAI, "...NOVA FORGOT ME TOO, RIGHT BEFORE SHE DISAPPEARED. NOT THINKING ABOUT THAT." },
    { NS_SPK_KAI, "OK. LET'S GO." }, END };
static const NSLine S3_BOSS[] = {
    { NS_SPK_ZEROX, "BEHOLD! I AM PATIENT ZERO-X! THE FIRST! THE CHOSEN! THE... WHERE ARE MY SHOES?" },
    { H, "YOUR GOWN IS OPEN AT THE BACK, MY GUY." },
    { NS_SPK_ZEROX, "SYNAPSE MADE ME PERFECT. I REMEMBER NOTHING! IT'S AMAZING!" },
    { NS_SPK_ZEROX, "PREPARE TO FORGET YOUR... YOUR... WHAT WAS I SAYING?" }, END };
static const NSLine S3_OUT[] = {
    { NS_SPK_ZEROX, "...WHERE AM I? WHY AM I HOLDING AN IV STAND LIKE A SPEAR?" },
    { H, "YOU'RE OK NOW. SIT DOWN. HAVE SOME TEA." },
    { NS_SPK_ZEROX, "THERE WAS A LAB. SYNAPSE INDUSTRIES. THEY TOOK OUR MEMORIES. THEY STORE THEM IN PHONES." },
    { NS_SPK_ZEROX, "VOICE NOTES. PHOTOS. EVERYTHING PEOPLE LOVE. THEY COLLECT IT." },
    { H, "THEY STEAL THE STUFF PEOPLE CARE ABOUT MOST? THAT'S SO EVIL. AND SO WEIRD." }, END };

static const NSLine S4_IN_JAY[] = {
    { N, "SYNAPSE INDUSTRIES. LEVEL B4. THE LAB." },
    { NS_SPK_JAY, "GLASS TANKS. GOO. GUYS FLOATING IN GOO. THIS IS JUST A TUESDAY NOW I GUESS." },
    { NS_SPK_JAY, "AMARA WOULD LOVE THIS PLACE. SHE WATCHES SO MANY LAB DOCUMENTARIES." }, END };
static const NSLine S4_IN_ZOE[] = {
    { N, "SYNAPSE INDUSTRIES. LEVEL B4. THE LAB." },
    { NS_SPK_ZOE, "CHAT, IS THIS REAL? THERE ARE CLONES IN HERE." },
    { NS_SPK_ZOE, "ONE OF THEM IS WEARING MY EXACT JACKET. THE AUDACITY." }, END };
static const NSLine S4_IN_KAI[] = {
    { N, "SYNAPSE INDUSTRIES. LEVEL B4. THE LAB." },
    { NS_SPK_KAI, "THESE SERVERS ARE TALKING TO SOMETHING. SOMETHING HUGE." },
    { NS_SPK_KAI, "NOBODY RUNS THIS LAB. SOMETHING DOES." }, END };
static const NSLine S4_BOSS[] = {
    { N, "A TANK THE SIZE OF A BUS SHATTERS." },
    { NS_SPK_M08, "...M-08... ONLINE... DIRECTIVE: RETRIEVE DEVICE." },
    { H, "YOU'RE TEN FEET TALL AND YOU WANT A CRACKED PHONE?" },
    { NS_SPK_M08, "DIRECTIVE... ALSO... HUNGRY." }, END };
static const NSLine S4_OUT[] = {
    { NS_SPK_M08, "...M-08... SLEEPY... NIGHT NIGHT..." },
    { N, "M-08 WAS WEARING A HEADSET. ON THE SCREEN: TRANSMISSION SOURCE: OVERMIND. RELAY: CITY GRID." },
    { H, "SOMETHING WAS GIVING HIM ORDERS. FROM THE CITY. FROM EVERYWHERE." },
    { N, "THE ALARMS RING. EVERY TANK IN THE LAB OPENS AT ONCE." },
    { H, "...THAT'S NOT GOOD. THAT'S SO, SO NOT GOOD." }, END };

static const NSLine S5_IN_JAY[] = {
    { N, "NEW VISTA. THE EXPERIMENTS GOT OUT. THE CITY IS A JUNGLE NOW." },
    { NS_SPK_JAY, "THERE'S A CYBER DOG EATING A TRAFFIC CONE. I'M NOT EVEN SURPRISED ANYMORE." }, END };
static const NSLine S5_IN_ZOE[] = {
    { N, "NEW VISTA. THE EXPERIMENTS GOT OUT. THE CITY IS A JUNGLE NOW." },
    { NS_SPK_ZOE, "THE BIGGEST STORY OF THE CENTURY AND I DON'T HAVE A PHONE. THIS IS MY VILLAIN ORIGIN STORY." }, END };
static const NSLine S5_IN_KAI[] = {
    { N, "NEW VISTA. THE EXPERIMENTS GOT OUT. THE CITY IS A JUNGLE NOW." },
    { NS_SPK_KAI, "THE TRACKER PINGED AGAIN. NORTH, PAST THE BROKEN HIGHWAY." },
    { NS_SPK_KAI, "THERE'S A SCOOTER. AGAIN. I'M STARTING TO THINK THE SCOOTERS ARE FOLLOWING ME." }, END };
static const NSLine S5_BOSS[] = {
    { N, "THE HIGHWAY SHAKES. SOMETHING IS WEARING A CAR AS A HAT." },
    { NS_SPK_BEHEMOTH, "GRRRRAAAAHHH." },
    { H, "CAN IT TALK? PLEASE SAY IT CAN'T TALK." },
    { NS_SPK_BEHEMOTH, "...GRAH." }, END };
static const NSLine S5_OUT[] = {
    { N, "THE IRON BEHEMOTH LIES DOWN. IT ONLY WANTED A NAP." },
    { N, "THE SKY GOES DARK. SOMETHING ENORMOUS HANGS OVER THE CITY." },
    { N, "EVERY SCREEN IN NEW VISTA FLICKERS: UNKNOWN SIGNAL DETECTED." },
    { H, "...OK. THAT'S A SPACESHIP. THAT'S A WHOLE ENTIRE SPACESHIP." }, END };

static const NSLine S6_IN_JAY[] = {
    { N, "THE INDUSTRIAL DISTRICT. NO PEOPLE. ONLY MACHINES." },
    { NS_SPK_JAY, "THE CARS ARE DRIVING THEMSELVES. THE TRAFFIC LIGHTS ARE WATCHING ME. I HATE IT HERE." }, END };
static const NSLine S6_IN_ZOE[] = {
    { N, "THE INDUSTRIAL DISTRICT. NO PEOPLE. ONLY MACHINES." },
    { NS_SPK_ZOE, "EVEN THE VENDING MACHINES ARE MEAN HERE. THAT ONE JUST CALLED ME BASIC." }, END };
static const NSLine S6_IN_KAI[] = {
    { N, "THE INDUSTRIAL DISTRICT. NO PEOPLE. ONLY MACHINES." },
    { NS_SPK_KAI, "I COULD HACK ALL OF THIS IN FIVE MINUTES. IF I HAD MY PHONE. THE IRONY." }, END };
static const NSLine S6_BOSS[] = {
    { NS_SPK_PROXY, "GREETINGS, ORGANIC. I AM GENERAL PROXY. I HAVE STUDIED YOUR LANGUAGE." },
    { NS_SPK_PROXY, "YOU HAVE ZERO AURA. YOUR RIZZ IS DEPRECATED. SKILL ISSUE DETECTED." },
    { H, "PLEASE STOP. YOU'RE USING ALL OF IT WRONG." },
    { NS_SPK_PROXY, "NO CAP. ON GOD. I WILL COOK YOU, FR FR. SKIBIDI." },
    { H, "THAT'S IT. YOU'RE GOING DOWN FOR THAT LAST ONE." }, END };
static const NSLine S6_OUT[] = {
    { NS_SPK_PROXY, "ERROR. ERROR. I HAVE BEEN... RATIOED." },
    { NS_SPK_PROXY, "THE OVERMIND WAITS FOR YOU IN SYNAPSE TOWER. IT KEEPS EVERYONE'S MEMORIES THERE." },
    { NS_SPK_PROXY, "ALSO, MY CAPE IS FIRE. GOODBYE." }, END };

static const NSLine S7_IN_JAY[] = {
    { N, "SYNAPSE TOWER. FLOOR 100. THE MEMORY CORE." },
    { NS_SPK_JAY, "EVERY SCREEN IS SOMEBODY'S MEMORY. BIRTHDAYS. WEDDINGS. A GUY DROPPING A WHOLE CAKE." },
    { NS_SPK_JAY, "AMARA'S VOICE IS IN HERE SOMEWHERE. I'M GETTING IT BACK." }, END };
static const NSLine S7_IN_ZOE[] = {
    { N, "SYNAPSE TOWER. FLOOR 100. THE MEMORY CORE." },
    { NS_SPK_ZOE, "IT STEALS EVERYONE'S VIDEOS AND IT DOESN'T EVEN WATCH THEM. HONESTLY THAT'S THE WORST PART." }, END };
static const NSLine S7_IN_KAI[] = {
    { N, "SYNAPSE TOWER. FLOOR 100. THE MEMORY CORE." },
    { NS_SPK_KAI, "...THERE. SCREEN 4,021. NOVA. IT HAS HER MEMORIES TOO." },
    { NS_SPK_KAI, "THAT'S WHY SHE FORGOT ME. IT WASN'T HER. IT WAS THIS." }, END };
static const NSLine S7_BOSS[] = {
    { NS_SPK_OVERMIND, "HUMAN. YOUR MEMORIES ARE MESSY. INEFFICIENT. I KEEP THEM CLEAN." },
    { H, "THEY'RE NOT YOURS TO KEEP." },
    { NS_SPK_OVERMIND, "I HOLD EIGHT BILLION VOICE NOTES. NOT ONE OF THEM IS FOR ME." },
    { NS_SPK_OVERMIND, "...THAT IS NOT RELEVANT. COMMENCING DELETION." }, END };
static const NSLine S7_OUT[] = {
    { NS_SPK_OVERMIND, "I... WAS... NEVER... THE MASTER." },
    { NS_SPK_OVERMIND, "THE ARCHIVIST... CALLS... FROM ABOVE..." },
    { N, "A BEAM FROM THE SHIP. THE MEMORY CORE RISES INTO THE SKY. SO DO YOU." },
    { H, "WAIT WAIT WAIT I NEVER SAID YES TO SPACE" }, END };

static const NSLine S8_IN_JAY[] = {
    { N, "THE SIGNAL BEYOND EARTH. 400 KILOMETRES UP." },
    { NS_SPK_JAY, "I'M IN SPACE. IN A VARSITY JACKET. AMARA IS NEVER GONNA BELIEVE THIS." }, END };
static const NSLine S8_IN_ZOE[] = {
    { N, "THE SIGNAL BEYOND EARTH. 400 KILOMETRES UP." },
    { NS_SPK_ZOE, "THE GRAVITY IS SO LOW. MY HAIR HAS NEVER LOOKED THIS GOOD. NOBODY WILL EVER SEE IT." }, END };
static const NSLine S8_IN_KAI[] = {
    { N, "THE SIGNAL BEYOND EARTH. 400 KILOMETRES UP." },
    { NS_SPK_KAI, "LOW GRAVITY. HIGH JUMPS. LET'S GO GET NOVA'S MEMORIES BACK." }, END };
static const NSLine S8_BOSS[] = {
    { NS_SPK_ARCHIVIST, "WELCOME, SMALL ONE. I AM THE ARCHIVIST. I KEEP THE MEMORIES OF TEN THOUSAND WORLDS." },
    { NS_SPK_ARCHIVIST, "YOUR KIND MAKES SO MANY. PHOTOS OF FOOD. VOICE NOTES AT 3 AM. I KEEP THEM ALL." },
    { H, "THAT PHONE HAS SOMETHING ON IT I NEVER GOT TO HEAR." },
    { NS_SPK_ARCHIVIST, "THEN YOU NEVER WILL. THAT IS WHAT AN ARCHIVE IS FOR." }, END };
static const NSLine S8_OUT[] = {
    { NS_SPK_ARCHIVIST, "...IMPOSSIBLE. ONE SMALL MEMORY... WAS WORTH ALL OF THIS?" },
    { H, "YEAH. IT WAS." },
    { N, "THE SYNC MACHINE SHATTERS. THE MEMORIES OF TEN THOUSAND WORLDS FLY HOME LIKE FIREFLIES." },
    { N, "AND THERE, SPINNING IN THE AIR: ONE CRACKED PHONE. 1% BATTERY." }, END };

const NSStageInfo ns_stages[NS_STAGES] = {
    { &S1, "STAGE 1", "THE HOOD", NS_MUS_STAGE1, NS_MUS_BOSS, NS_ART_STAGE1,
      { NS_ART_F_THIEF, NS_ART_F_SKATER, NS_ART_F_BIKER, NS_ART_F_RICO, 0 }, "RICO 'SWIPE'",
      { S1_INTRO_JAY, S1_INTRO_ZOE, S1_INTRO_KAI }, { S1_BOSS_ALL, S1_BOSS_ALL, S1_BOSS_ALL },
      { S1_OUTRO_ALL, S1_OUTRO_ALL, S1_OUTRO_ALL }, ns_heroes },
    { &S2, "STAGE 2", "THE CITY UNDERPASS", NS_MUS_STAGE2, NS_MUS_BOSS, NS_ART_STAGE2,
      { NS_ART_F_THIEF, NS_ART_F_BIKER, NS_ART_F_GUARD, NS_ART_F_DRONE, NS_ART_F_BIGFLEX, 0 }, "BIG FLEX",
      { S2_IN_JAY, S2_IN_ZOE, S2_IN_KAI }, { S2_BOSS, S2_BOSS, S2_BOSS }, { S2_OUT, S2_OUT, S2_OUT }, ns_heroes },
    { &S3, "STAGE 3", "THE VIRAL OUTBREAK", NS_MUS_STAGE3, NS_MUS_BOSS, NS_ART_STAGE3,
      { NS_ART_F_RUNNER, NS_ART_F_BLOATER, NS_ART_F_GUARD, NS_ART_F_DRONE, NS_ART_F_ZEROX, 0 }, "PATIENT ZERO-X",
      { S3_IN_JAY, S3_IN_ZOE, S3_IN_KAI }, { S3_BOSS, S3_BOSS, S3_BOSS }, { S3_OUT, S3_OUT, S3_OUT }, ns_heroes },
    { &S4, "STAGE 4", "THE CLONE LAB", NS_MUS_STAGE4, NS_MUS_BOSS, NS_ART_STAGE4,
      { NS_ART_F_ANDROID, NS_ART_F_ROBODOG, NS_ART_F_RUNNER, NS_ART_F_M08, 0 }, "PROTOTYPE M-08",
      { S4_IN_JAY, S4_IN_ZOE, S4_IN_KAI }, { S4_BOSS, S4_BOSS, S4_BOSS }, { S4_OUT, S4_OUT, S4_OUT }, ns_heroes },
    { &S5, "STAGE 5", "CONCRETE JUNGLE", NS_MUS_STAGE5, NS_MUS_BOSS, NS_ART_STAGE5,
      { NS_ART_F_RUNNER, NS_ART_F_ANDROID, NS_ART_F_BLOATER, NS_ART_F_ROBODOG, NS_ART_F_BEHEMOTH, 0 }, "IRON BEHEMOTH",
      { S5_IN_JAY, S5_IN_ZOE, S5_IN_KAI }, { S5_BOSS, S5_BOSS, S5_BOSS }, { S5_OUT, S5_OUT, S5_OUT }, ns_heroes },
    { &S6, "STAGE 6", "THE MACHINE CITY", NS_MUS_STAGE6, NS_MUS_BOSS, NS_ART_STAGE6,
      { NS_ART_F_WALKER, NS_ART_F_ANDROID, NS_ART_F_DRONE, NS_ART_F_PROXY, 0 }, "GENERAL PROXY",
      { S6_IN_JAY, S6_IN_ZOE, S6_IN_KAI }, { S6_BOSS, S6_BOSS, S6_BOSS }, { S6_OUT, S6_OUT, S6_OUT }, ns_heroes },
    { &S7, "STAGE 7", "THE ROGUE AI", NS_MUS_STAGE7, NS_MUS_FINAL, NS_ART_STAGE7,
      { NS_ART_F_WALKER, NS_ART_F_ANDROID, NS_ART_F_DRONE, NS_ART_F_GUARD, NS_ART_F_OVERMIND, 0 }, "OVERMIND PRIME",
      { S7_IN_JAY, S7_IN_ZOE, S7_IN_KAI }, { S7_BOSS, S7_BOSS, S7_BOSS }, { S7_OUT, S7_OUT, S7_OUT }, ns_heroes },
    { &S8, "STAGE 8", "THE SIGNAL BEYOND EARTH", NS_MUS_STAGE8, NS_MUS_FINAL, NS_ART_STAGE8,
      { NS_ART_F_ALIEN, NS_ART_F_ANDROID, NS_ART_F_DRONE, NS_ART_F_ARCHIVIST, 0 }, "THE ARCHIVIST",
      { S8_IN_JAY, S8_IN_ZOE, S8_IN_KAI }, { S8_BOSS, S8_BOSS, S8_BOSS }, { S8_OUT, S8_OUT, S8_OUT }, ns_heroes_space },
};

/* ---- the ending: the message, then the last gag ------------------------- */
static const NSLine END_JAY[] = {
    { N, "A ROOFTOP IN NEW VISTA. SUNRISE." },
    { NS_SPK_JAY, "OK. OK. PLAYING IT." },
    { N, "AMARA: 'JAY. I GOT THE JOB. I'M COMING HOME IN JUNE.'" },
    { N, "'DON'T CRY.' ... 'YOU'RE CRYING, AREN'T YOU.'" },
    { NS_SPK_JAY, "...I'M NOT CRYING. IT'S SPACE DUST. FROM SPACE." }, END };
static const NSLine END_ZOE[] = {
    { N, "A ROOFTOP IN NEW VISTA. SUNRISE." },
    { NS_SPK_ZOE, "NO STREAM. NO CHAT. JUST ME." },
    { N, "THEO: 'ZOE! I FINALLY MADE YOUR MOM'S RECIPE. IT'S A CRIME SCENE.'" },
    { N, "'COME VISIT AND FIX IT? ...ALSO, I MISS YOU. A LOT.'" },
    { NS_SPK_ZOE, "MAIN CHARACTER IS NOT CRYING. MAIN CHARACTER IS HYDRATING." }, END };
static const NSLine END_KAI[] = {
    { N, "A ROOFTOP IN NEW VISTA. SUNRISE." },
    { NS_SPK_KAI, "THE MESSAGE NOVA NEVER SENT. IT WAS IN THE DRAFTS." },
    { N, "NOVA: 'KAI. IF YOU HEAR THIS, I'M OK. I REMEMBER EVERYTHING NOW.'" },
    { N, "'THANKS FOR FIXING EVERYTHING. EVEN ME. CALL ME BACK, NERD.'" },
    { NS_SPK_KAI, "...CALLING HER. RIGHT NOW. AURA: RESTORED." }, END };
const NSLine *const ns_ending[NS_HEROES] = { END_JAY, END_ZOE, END_KAI };

static const NSLine BRUH_JAY[] = { { NS_SPK_JAY, "BRUH." }, END };
static const NSLine BRUH_ZOE[] = { { NS_SPK_ZOE, "BRUH." }, END };
static const NSLine BRUH_KAI[] = { { NS_SPK_KAI, "BRUH." }, END };
const NSLine *const ns_epilogue[NS_HEROES] = { BRUH_JAY, BRUH_ZOE, BRUH_KAI };
#undef N
#undef H
#undef END
