#ifndef NG_BEATEMUP_H
#define NG_BEATEMUP_H
/*
 * ng_beatemup: a deterministic 2.5D belt-scrolling brawler core (v1.7.7
 * genre engine). Selected with GAME_GENRE_ENGINE = beatemup in game.mk.
 *
 * Unlike ng_fight (one-on-one), this runs one player against a crowd on a
 * scrolling street:
 *   - space: ground x along the street, depth y across it, height z above
 *     the ground, kept apart; the screen y is the game's (horizon + y - z),
 *     and things are drawn far to near (ng_beatemup_draw_list);
 *   - the player: walk in eight directions, double-tap dash, jump, attack
 *     chains (A), aerial and low attacks, dash attacks, a special (D) that
 *     costs health, grabs (C, or walking into someone) with pummels and
 *     throws forward and back, weapon pick-ups with durability, food
 *     (taken with C, or by walking over it with NGB_SF_AUTO_PICKUP);
 *   - hits: startup / active / recovery frames per move, hit, hurt and
 *     grab volumes in all three axes, hitstop, hitstun, knockback,
 *     launches and juggles, knockdowns with invulnerable get-ups, combo
 *     counting, shields that block from the front, boss armour;
 *   - the crowd: a director hands out a few attack tokens; the others wait
 *     on spaced slots on both sides, so they don't all stand on one spot;
 *     attacks are telegraphed (an event before each), attackers back off
 *     after striking, and a player who keeps getting hit is knocked down
 *     and given room instead of being juggled forever;
 *   - stages: waves spawned off-screen as the camera reaches them, scene
 *     locks until a wave is beaten, a reserve when too many are out,
 *     bosses with phases (health thresholds, invulnerable transitions,
 *     new attacks), props (crates, barrels, weapons, food, hazards, ramps);
 *   - a rideable light vehicle (an electric kick scooter in the first
 *     game, but nothing here names it): mount, accelerate, steer across the
 *     belt, hop, ram at speed, take damage, break into a swingable wreck
 *     (with NGB_SF_WALK_ON: walk onto it to ride, jump off with B);
 *
 * Enemy kinds come from data (NGBFighterDef: brawler, charger, ranged,
 * shield, grappler, jumper, boss) with an optional callback for anything
 * a game needs beyond them. The core never touches video RAM or sound; the
 * game draws from the context and turns events into sounds and effects.
 * Everything lives in the caller's NGBeatEmUp (1728 bytes): no heap, no
 * floating point, no hidden globals, no recursion. Positions are Q8 pixels.
 * One ng_beatemup_update() is one frame.
 */
#include <stdint.h>
#include "ng_defs.h"
#ifdef __cplusplus
extern "C" {
#endif

#define NGB_MAX_ENEMIES   10u
#define NGB_MAX_PROPS     12u
#define NGB_MAX_SHOTS     8u
#define NGB_EVENT_QUEUE   64u     /* power of two */
#define NGB_NONE          0xFFu
#define NGB_PLAYER        0u      /* fighter index of the player; enemies are 1.. */
#define NGB_FIGHTERS      (1u + NGB_MAX_ENEMIES)

enum { NGB_OK = 0, NGB_ERR_ARG = -1, NGB_ERR_FULL = -2, NGB_ERR_STATE = -3 };

/* Input */
#define NGB_IN_UP      0x01u
#define NGB_IN_DOWN    0x02u
#define NGB_IN_LEFT    0x04u
#define NGB_IN_RIGHT   0x08u
#define NGB_BTN_A      0x10u   /* attack */
#define NGB_BTN_B      0x20u   /* jump */
#define NGB_BTN_C      0x40u   /* grab, throw, pick up, mount */
#define NGB_BTN_D      0x80u   /* special */

/* ---- moves -------------------------------------------------------------- */
#define NGB_MV_KNOCKDOWN  0x01u   /* knocks down */
#define NGB_MV_LAUNCH     0x02u   /* sends up: can be juggled */
#define NGB_MV_LOW        0x04u   /* hits someone lying down */
#define NGB_MV_INVULN     0x08u   /* the attacker can't be hit in startup and active frames */
#define NGB_MV_GRAB       0x10u   /* takes hold instead of hitting (grapplers) */
#define NGB_MV_UNBLOCKABLE 0x20u  /* goes through shields */
#define NGB_MV_PROJECTILE 0x40u   /* fires a shot on its first active frame */
#define NGB_MV_RUN        0x80u   /* the attacker charges (lunge every active frame) */

typedef struct {
    uint8_t startup, active, recovery;   /* frames */
    uint8_t damage, hitstun, flags;      /* NGB_MV_* */
    int8_t box_x;                        /* hit box front edge distance from the feet (facing right) */
    uint8_t box_w;                       /* ... and its length forward */
    uint8_t box_z, box_h;                /* height above the ground and tall */
    uint8_t box_depth;                   /* half depth */
    int8_t push_x, push_z;               /* knockback and launch speeds, pixels * 16 per frame */
    int8_t lunge;                        /* attacker moves forward, pixels * 16 per active frame */
    uint8_t cancel;                      /* from this frame the next chain move may start (0: none) */
    uint8_t next;                        /* the chain's next move (NGB_NONE) */
    uint8_t sound, anim;                 /* the game's ids */
    /* v1.7.7 additions, last so that older tables (which leave them out) keep their behaviour */
    uint8_t hitstop;                     /* frames attacker and target hold on contact (0: 2 + damage / 4, at most 6) */
    int8_t step;                         /* while winding up, the attacker steps towards the nearest foe in front
                                            (in its lane, within 64 pixels of the reach) at up to pixels * 16 a
                                            frame, until the blow will reach it 4 pixels deep; then it plants */
} NGBMove;

/* Enemy behaviour */
enum { NGB_AI_BRAWLER = 0, NGB_AI_CHARGER, NGB_AI_RANGED, NGB_AI_SHIELD, NGB_AI_GRAPPLER, NGB_AI_JUMPER, NGB_AI_BOSS };

#define NGB_FD_HEAVY      0x01u   /* can't be thrown or grabbed */
#define NGB_FD_SHIELD     0x02u   /* blocks from the front while not attacking */
#define NGB_FD_ARMOR      0x04u   /* light hits don't stagger (boss) */
#define NGB_FD_FLYER      0x08u   /* drones: stay up, can't be grabbed */

typedef struct {
    uint8_t below_pct;                   /* this phase starts when health drops below this % */
    uint8_t invuln;                      /* frames of invulnerable transition */
    uint8_t attacks[4], nattacks;        /* move indices */
    uint8_t speed_add;                   /* pixels * 16 */
    uint8_t flags;                       /* NGB_FD_* while in this phase */
    uint8_t event;                       /* passed to NGB_EV_BOSS_PHASE for the game */
} NGBBossPhase;

typedef struct NGBeatEmUp NGBeatEmUp;
/* Optional per-kind callback, run once a frame for each enemy of that kind
 * after its built-in behaviour (NULL: none). */
typedef void (*NGBThink)(NGBeatEmUp *b, uint8_t fighter);

typedef struct {
    uint8_t max_hp, speed, depth_speed, jump;   /* speeds: pixels * 16 per frame */
    uint8_t half_w, height, half_depth;         /* hurt volume */
    uint8_t weight;                              /* throw distance divisor, grab escape */
    const NGBMove *moves;
    uint8_t nmoves;
    /* player moves (enemies use attacks[]) */
    uint8_t chain, air, low, dash, special, pummel, throw_fwd, throw_back;
    /* enemies */
    uint8_t ai, reach, aggression;              /* reach: pixels; aggression 0..15 */
    uint8_t attacks[4], nattacks;
    uint8_t flags;                               /* NGB_FD_* */
    uint16_t score;
    const NGBBossPhase *phases;
    uint8_t nphases;
    NGBThink think;
} NGBFighterDef;

/* ---- props and stages --------------------------------------------------- */
enum {
    NGB_PROP_NONE = 0,
    NGB_PROP_CRATE,     /* breaks; drops `contents` (a prop kind) */
    NGB_PROP_BARREL,    /* breaks; drops contents; rolls when thrown */
    NGB_PROP_WEAPON,    /* `contents` = weapon index; picked up with C */
    NGB_PROP_FOOD,      /* heals `contents` hit points when picked up */
    NGB_PROP_SCORE,     /* `contents` * 100 points */
    NGB_PROP_VEHICLE,   /* ridden with C */
    NGB_PROP_HAZARD,    /* fire, a live cable: hurts whoever touches it every 32 frames */
    NGB_PROP_RAMP       /* a vehicle at speed jumps off it */
};

typedef struct {
    const NGBMove *move;                /* the swing (NULL: thrown only) */
    uint8_t durability;                 /* hits before it breaks */
    uint8_t throw_damage;
    uint8_t anim;                       /* the game's id */
} NGBWeaponDef;

typedef struct {
    uint8_t kind;
    uint8_t y;                           /* depth, pixels */
    uint16_t x;                          /* pixels */
    uint8_t contents, hp;
} NGBPropSpawn;

#define NGB_WAVE_LOCK   0x01u   /* the camera stops until this wave is beaten */
#define NGB_WAVE_BOSS   0x02u   /* the stage is clear when this wave is beaten */
typedef struct {
    uint16_t at_x;                       /* camera x that starts it */
    uint8_t flags;                       /* NGB_WAVE_* */
    uint8_t count;                       /* spawns used */
    uint8_t enemy[8];                    /* NGBStage.enemies indices */
    uint8_t sides;                       /* bit i set: spawn i comes from the left */
    uint8_t gap;                         /* frames between spawns */
} NGBWave;

#define NGB_SF_NO_BACKTRACK  0x01u       /* the camera never scrolls back */
#define NGB_SF_AUTO_PICKUP   0x02u       /* walking over food or points takes them, and a weapon too
                                            when the hands are empty (C still picks up as well) */
#define NGB_SF_WALK_ON       0x04u       /* walking onto the vehicle rides it (a held weapon is dropped);
                                            B or C jumps off it, clear of it (no hop on B then) */
typedef struct {
    uint16_t length;                     /* pixels */
    uint8_t y_min, y_max;                /* the walkable band, pixels */
    uint16_t view_w;                     /* visible width (320) */
    const NGBWave *waves;
    const NGBPropSpawn *props;
    const NGBFighterDef *const *enemies;
    const NGBWeaponDef *weapons;
    uint8_t nwaves, nprops, nenemies, nweapons;
    uint8_t max_attackers;               /* attack tokens (2) */
    uint8_t max_active;                  /* enemies out at once (<= NGB_MAX_ENEMIES) */
    uint8_t flags;                       /* NGB_SF_* */
    uint8_t camera_max;                  /* camera speed, pixels per frame (6) */
    uint8_t vehicle_max, vehicle_accel;  /* pixels * 16 per frame, and per frame squared */
    uint8_t vehicle_durability, vehicle_ram;  /* hits, ram damage */
    /* v1.7.7 additions (0: the last blow holds like any other) */
    uint8_t ko_hitstop;                  /* the blow that beats an enemy holds at least this long */
    uint8_t boss_ko_hitstop;             /* ... and a boss */
} NGBStage;

/* ---- runtime -------------------------------------------------------------- */
enum {
    NGB_ST_NONE = 0, NGB_ST_IDLE, NGB_ST_WALK, NGB_ST_RUN, NGB_ST_JUMP, NGB_ST_ATTACK,
    NGB_ST_GRAB, NGB_ST_GRABBED, NGB_ST_THROWN, NGB_ST_HITSTUN, NGB_ST_FALL, NGB_ST_DOWN,
    NGB_ST_GETUP, NGB_ST_RIDE, NGB_ST_BLOCK, NGB_ST_DEAD
};
enum { NGB_AI_WAIT = 0, NGB_AI_APPROACH, NGB_AI_ENGAGE, NGB_AI_RETREAT, NGB_AI_ENTER };

typedef struct {
    int32_t x, y, z, vx, vy, vz;         /* Q8 */
    const NGBFighterDef *def;
    uint8_t alive, state, facing;        /* facing 0 right, 1 left */
    uint8_t hp, move, move_frame, chain_buf;
    uint8_t hitstun, hitstop, invuln, juggle;
    uint8_t hold;                        /* fighter held, or holding this one (NGB_NONE) */
    uint8_t hold_frames, pummels;
    uint8_t weapon;                      /* prop held (NGB_NONE) */
    uint8_t vehicle;                     /* prop ridden (NGB_NONE) */
    uint8_t ai, ai_timer, token, slot;
    uint8_t phase, hits_taken;
    uint8_t serial;                      /* bumped each move: one hit per move per target */
    uint8_t last_hit[NGB_FIGHTERS];      /* serial of the attacker's move that last hit this one */
    uint16_t timer;                      /* frames in this state */
    uint8_t air_done, wave;
    uint16_t stride;                     /* pixels * 16 walked or run on the ground (wraps): walk cycles by distance */
    uint8_t hit_z;                       /* height above the feet, pixels, where the last blow landed */
} NGBFighter;

typedef struct {
    int32_t x, y, z, vx, vz;             /* Q8 */
    uint8_t kind, contents, hp, state;   /* state: 0 lying, 1 held, 2 flying, 3 ridden */
    uint8_t owner, timer;
    int16_t speed;                       /* vehicle speed, pixels * 16 */
} NGBProp;

typedef struct {
    int32_t x, y, z, vx;                 /* Q8 */
    uint8_t alive, owner, damage, timer;
} NGBShot;

enum {
    NGB_EV_NONE = 0,
    NGB_EV_HIT,          /* a = attacker, b = target, c = damage, value = height of the contact above
                            the target's feet, pixels (the middle of the blow's reach on its body) */
    NGB_EV_BLOCKED,      /* a = attacker, b = target */
    NGB_EV_KO,           /* b = target, value = score */
    NGB_EV_KNOCKDOWN,    /* b = target */
    NGB_EV_GETUP,        /* b = target */
    NGB_EV_GRAB,         /* a = holder, b = held */
    NGB_EV_ESCAPE,       /* a = holder, b = who broke free */
    NGB_EV_THROW,        /* a = thrower, b = thrown, c = 1 backwards */
    NGB_EV_TELEGRAPH,    /* a = enemy, c = move: it is about to strike */
    NGB_EV_SPECIAL,      /* a = fighter */
    NGB_EV_JUMP, NGB_EV_LAND, NGB_EV_DASH,   /* a = fighter */
    NGB_EV_COMBO,        /* value = hits */
    NGB_EV_PICKUP,       /* a = fighter, b = prop */
    NGB_EV_FOOD,         /* b = prop, c = hit points */
    NGB_EV_SCORE_ITEM,   /* value = points */
    NGB_EV_BREAK,        /* b = prop: a crate or barrel broke */
    NGB_EV_WEAPON_BREAK, /* a = fighter */
    NGB_EV_MOUNT, NGB_EV_DISMOUNT,           /* a = fighter, b = prop */
    NGB_EV_RAM,          /* a = rider, b = target */
    NGB_EV_VEHICLE_BREAK,/* b = prop */
    NGB_EV_SHOT,         /* a = owner */
    NGB_EV_WAVE,         /* c = wave index */
    NGB_EV_WAVE_CLEAR,   /* c = wave index */
    NGB_EV_GO,           /* the camera moves on */
    NGB_EV_BOSS_PHASE,   /* a = boss, c = phase, value = the phase's event */
    NGB_EV_PLAYER_DOWN,  /* lives left in c */
    NGB_EV_RESPAWN,
    NGB_EV_GAME_OVER,
    NGB_EV_STAGE_CLEAR
};
typedef struct { uint8_t type, a, b, c; uint16_t value; } NGBEvent;

enum { NGB_PLAYING = 0, NGB_PLAYER_OUT, NGB_GAME_OVER, NGB_CLEAR };

struct NGBeatEmUp {
    const NGBStage *stage;
    NGBFighter f[NGB_FIGHTERS];
    NGBProp prop[NGB_MAX_PROPS];
    NGBShot shot[NGB_MAX_SHOTS];
    int32_t camera_x;                    /* Q8 */
    int32_t lock_left, lock_right;       /* camera limits while a wave holds it */
    uint8_t locked, wave_next, wave_live;
    uint8_t spawn_pending, spawn_wave, spawn_timer;
    uint8_t status, lives;
    uint32_t score;
    uint16_t combo, combo_timer;
    uint8_t in_dirs, in_buttons, in_pressed;
    uint8_t tap_dir, tap_timer;          /* double-tap detection */
    uint8_t attack_cool;                 /* frames before another enemy may start an attack */
    uint8_t special_cool;
    uint8_t respawn_timer;
    uint32_t tick, rng;
    uint8_t ev_head, ev_count;
    uint16_t ev_dropped;
    NGBEvent ev[NGB_EVENT_QUEUE];
};

/* ---- calls ----------------------------------------------------------------- */
/* Start a stage with a player (lives: 0 means 3). */
int      NEOGEO_USER ng_beatemup_init(NGBeatEmUp *b, const NGBStage *stage, const NGBFighterDef *player,
                                      uint8_t lives, uint32_t seed);
/* Held directions and buttons this frame (presses are worked out here). */
void     NEOGEO_USER ng_beatemup_input(NGBeatEmUp *b, uint8_t dirs, uint8_t buttons);
void     NEOGEO_USER ng_beatemup_update(NGBeatEmUp *b);
/* Add an enemy (a wave does this; a boss phase or a callback may too):
 * from_left 1 enters from the left edge of the view, 0 the right, 2 at x. */
int      NEOGEO_USER ng_beatemup_spawn(NGBeatEmUp *b, const NGBFighterDef *def, int16_t x, uint8_t y, uint8_t from);
int      NEOGEO_USER ng_beatemup_add_prop(NGBeatEmUp *b, uint8_t kind, int16_t x, uint8_t y, uint8_t contents, uint8_t hp);
/* After NGB_EV_GAME_OVER: lives refilled, play on where it stopped. */
void     NEOGEO_USER ng_beatemup_continue(NGBeatEmUp *b, uint8_t lives);
uint8_t  NEOGEO_USER ng_beatemup_status(const NGBeatEmUp *b);
uint8_t  NEOGEO_USER ng_beatemup_event(NGBeatEmUp *b, NGBEvent *out);
/* Everything to draw, far to near: 0 the player, 1.. enemies,
 * 0x40 + i props, 0x80 + i shots. Returns how many. */
uint8_t  NEOGEO_USER ng_beatemup_draw_list(const NGBeatEmUp *b, uint8_t *ids, uint8_t max);
uint8_t  NEOGEO_USER ng_beatemup_enemies_alive(const NGBeatEmUp *b);
/* Where a fighter's move is: NGB_PH_NONE (no move), _STARTUP (winding up),
 * _ACTIVE (can hit) or _RECOVERY, and *frame the frames spent in that phase
 * (frame may be NULL). A game picks anticipation, contact and follow-through
 * pictures from it. A held weapon's swing counts; a throw's recovery is NONE. */
enum { NGB_PH_NONE = 0, NGB_PH_STARTUP, NGB_PH_ACTIVE, NGB_PH_RECOVERY };
uint8_t  NEOGEO_USER ng_beatemup_move_phase(const NGBeatEmUp *b, uint8_t fighter, uint8_t *frame);
uint32_t NEOGEO_USER ng_beatemup_hash(const NGBeatEmUp *b);

#ifdef __cplusplus
}
#endif
#endif
