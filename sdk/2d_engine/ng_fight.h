#ifndef NG_FIGHT_H
#define NG_FIGHT_H
/*
 * ng_fight: a deterministic one-on-one fighting-game core (opt-in C module,
 * -DNG_FIGHT=1 in GAME_ENGINE_DEFINES).
 *
 * The core owns the simulation only: buffered two-player input with motion
 * recognition, data-driven fighters (ROM tables), the fighter state
 * machine, physics, push/hurt/hit boxes, guard levels, throws, projectiles,
 * the four meters (life, power, guard, spirit), rounds and a CPU opponent.
 * Rendering, sound and the FIX HUD stay with the game: each frame it reads
 * NGFMatch and drains the event queue.
 *
 * One ngf_match_step() is one VBlank. No heap, no floating point; positions
 * are Q8 (pixels * 256). Coordinates: x grows right, h is the height above
 * the floor (grows up). Boxes in data face right and are mirrored here.
 */
#include <stdint.h>
#include "ng_defs.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ---- input ---------------------------------------------------------- */
/* Raw bits: the BIOS controller byte (P1CURRENT layout). */
#define NGF_RAW_UP     0x01u
#define NGF_RAW_DOWN   0x02u
#define NGF_RAW_LEFT   0x04u
#define NGF_RAW_RIGHT  0x08u
#define NGF_BTN_A      0x10u   /* light punch */
#define NGF_BTN_B      0x20u   /* light kick  */
#define NGF_BTN_C      0x40u   /* heavy punch */
#define NGF_BTN_D      0x80u   /* heavy kick  */
#define NGF_BTN_ALL    0xF0u

#define NGF_INPUT_FRAMES 32u    /* ring length: must be a power of two */

/* Motions (numpad notation, facing right). */
enum {
    NGF_MOTION_NONE = 0,
    NGF_MOTION_QCF,         /* 236 */
    NGF_MOTION_QCB,         /* 214 */
    NGF_MOTION_DP,          /* 623 */
    NGF_MOTION_RDP,         /* 421 */
    NGF_MOTION_HCF,         /* 41236 */
    NGF_MOTION_HCB,         /* 63214 */
    NGF_MOTION_DQCF,        /* 236236 */
    NGF_MOTION_DQCB,        /* 214214 */
    NGF_MOTION_CHARGE_BF,   /* [4] 6 */
    NGF_MOTION_CHARGE_DU,   /* [2] 8 */
    NGF_MOTION_360,         /* all four cardinals */
    NGF_MOTION_720,         /* two full 360 rotations */
    NGF_MOTION_DASH,        /* 6 5 6 */
    NGF_MOTION_BACKDASH,    /* 4 5 4 */
    NGF_MOTION_FWD,         /* holding forward (command normal / throw) */
    NGF_MOTION_BACK,        /* holding back (throw) */
    NGF_MOTION_DOWN_FWD,    /* holding 3 */
    NGF_MOTION_COUNT
};

typedef struct {
    uint8_t raw[NGF_INPUT_FRAMES];     /* raw controller bits per frame */
    uint8_t press[NGF_INPUT_FRAMES];   /* buttons pressed on that frame */
    uint8_t pending, release_pending, motion_frames;
    uint8_t press_age[4], release_age[4];
    uint8_t head;                      /* index of the newest frame */
    uint8_t released;                  /* buttons released this frame */
    uint8_t charge_back, charge_down;  /* frames held (saturating) */
    uint8_t back_release, down_release;/* charge when last released */
    uint8_t back_age, down_age;        /* frames since that release */
} NGFInput;

void    NEOGEO_USER ngf_input_reset(NGFInput *in);
void    NEOGEO_USER ngf_input_push(NGFInput *in, uint8_t raw, uint8_t facing_left);
/* Numpad direction (1-9) of a frame, `ago` frames back, for a facing. */
uint8_t NEOGEO_USER ngf_input_dir(const NGFInput *in, uint8_t ago, uint8_t facing_left);
uint8_t NEOGEO_USER ngf_input_motion(const NGFInput *in, uint8_t motion, uint8_t facing_left, uint8_t window);
/* All buttons of `mask` pressed within the last `buffer` frames. */
uint8_t NEOGEO_USER ngf_input_pressed(const NGFInput *in, uint8_t mask, uint8_t buffer);
uint8_t NEOGEO_USER ngf_input_released(const NGFInput *in, uint8_t mask, uint8_t buffer);
void    NEOGEO_USER ngf_input_consume(NGFInput *in, uint8_t mask);
/* Raw bits for a numpad direction, for a facing (CPU and replay input). */
uint8_t NEOGEO_USER ngf_raw_from_numpad(uint8_t numpad, uint8_t facing_left);

/* ---- fighter data (ROM) ------------------------------------------- */
typedef struct { int8_t x; int8_t y; uint8_t w; uint8_t h; } NGFBox;   /* y: bottom above feet */
typedef struct { NGFBox box[3]; uint8_t count; uint8_t pad; } NGFHurtSet;

#define NGF_FF_ACTIVE   0x01u   /* the move's hitbox is live */
#define NGF_FF_REHIT    0x02u   /* a new hit may land from this frame */
#define NGF_FF_SPAWN    0x04u   /* spawn the move's projectile on entry */
#define NGF_FF_INVULN   0x08u   /* strike invulnerable */
#define NGF_FF_AIRBORNE 0x10u   /* leaves the floor: dx/dy are its speeds (1/8 px/frame) */
#define NGF_FF_COUNTER  0x20u   /* a strike landing now starts the move's `follow` instead */

typedef struct {
    uint8_t sprite;   /* frame in the fighter's sprite sheet */
    uint8_t time;     /* frames shown */
    uint8_t hurt;     /* hurt set */
    uint8_t flags;    /* NGF_FF_* */
    int8_t  dx;       /* forward motion per frame (px) */
    int8_t  dy;       /* NGF_FF_AIRBORNE: launch speed */
} NGFFrame;

typedef struct { uint16_t first; uint8_t count; uint8_t loop; } NGFAnim;  /* loop: frame to loop from, 0xFF = hold last */

/* Standard animations: the first NGF_A_COUNT entries of every fighter. */
enum {
    NGF_A_IDLE, NGF_A_WALK_F, NGF_A_WALK_B, NGF_A_CROUCH, NGF_A_JUMP,
    NGF_A_GUARD, NGF_A_GUARD_LOW, NGF_A_HIT, NGF_A_HIT_LOW, NGF_A_FALL,
    NGF_A_DOWN, NGF_A_GETUP, NGF_A_DASH, NGF_A_BACKDASH, NGF_A_THROWN,
    NGF_A_WIN, NGF_A_LOSE, NGF_A_INTRO, NGF_A_DIZZY,
    NGF_A_COUNT
};

enum { NGF_KIND_NORMAL, NGF_KIND_COMMAND, NGF_KIND_SPECIAL, NGF_KIND_SUPER,
       NGF_KIND_THROW, NGF_KIND_EVADE, NGF_KIND_GUARD_ATTACK };
#define NGF_STANCE_STAND   0x01u
#define NGF_STANCE_CROUCH  0x02u
#define NGF_STANCE_AIR     0x04u
#define NGF_STANCE_CLOSE   0x08u   /* only within the fighter's close range */
#define NGF_STANCE_FAR     0x10u   /* only outside it */
#define NGF_STANCE_GROUND  (NGF_STANCE_STAND | NGF_STANCE_CROUCH)
enum { NGF_LV_MID, NGF_LV_HIGH, NGF_LV_LOW, NGF_LV_OVERHEAD, NGF_LV_THROW, NGF_LV_UNBLOCKABLE };
#define NGF_ON_KNOCKDOWN  0x01u
#define NGF_ON_LAUNCH     0x02u
#define NGF_ON_WALLBOUNCE 0x04u
#define NGF_CANCEL_CHAIN   0x01u   /* into a later normal */
#define NGF_CANCEL_SPECIAL 0x02u
#define NGF_CANCEL_SUPER   0x04u

typedef struct {
    uint8_t anim;          /* animation index */
    uint8_t kind;          /* NGF_KIND_* */
    uint8_t motion;        /* NGF_MOTION_* */
    uint8_t buttons;       /* NGF_BTN_* mask (two bits: pressed together) */
    uint8_t stance;        /* NGF_STANCE_* */
    uint8_t level;         /* NGF_LV_* */
    uint8_t damage, guard_damage, chip;
    uint8_t hitstun, blockstun, pushback;
    uint8_t on_hit;        /* NGF_ON_* */
    uint8_t cancel;        /* NGF_CANCEL_* this move may cancel into */
    uint8_t meter;         /* power gained on hit (half on block) */
    uint8_t cost;          /* supers: power stocks; guard attack: spirit */
    uint8_t hitstop;
    uint8_t projectile;    /* 1-based projectile index, 0 = none */
    uint8_t sound, hit_sound;
    int8_t  launch_vx, launch_vy;   /* knockdown trajectory, px/frame */
    uint8_t window;        /* motion leniency in frames, 0 = default */
    uint8_t range;         /* throws: reach; CPU: preferred distance */
    NGFBox  hit;
    uint8_t follow;        /* 1-based move started by a countered strike, 0 = none */
    uint8_t pad;
} NGFMove;

typedef struct {
    uint8_t anim;          /* animation index */
    int8_t  speed;         /* px/frame, forward */
    int8_t  spawn_x, spawn_y;
    uint8_t damage, chip, hitstun, blockstun, level, life, on_hit, hit_sound;
    NGFBox  box;
} NGFProjectileDef;

typedef struct {
    uint8_t aggression;    /* 0-255: approach and attack rate */
    uint8_t range;         /* preferred distance (px) */
    uint8_t jump;          /* 0-255: jump-in rate */
    uint8_t poke, sweep, anti_air, special, rush, throw_move, super_move; /* move indices, 0xFF none */
} NGFCpuProfile;

typedef struct {
    const char *name;
    uint16_t life;
    int16_t  walk_fwd, walk_back;      /* Q8 px/frame */
    int16_t  dash, backdash_vx, backdash_vy;
    int16_t  jump_vx, jump_vy, gravity;/* Q8 */
    uint8_t  push_w, push_h, crouch_h, close_range;
    uint8_t  guard_max, pad;
    const NGFAnim *anims;
    const NGFFrame *frames;
    const NGFHurtSet *hurt;
    const NGFMove *moves;
    const NGFProjectileDef *projectiles;
    uint8_t  anim_count, move_count, projectile_count, hurt_count;
    NGFCpuProfile cpu_profile;
} NGFFighterDef;

/* ---- runtime ------------------------------------------------------- */
enum { NGF_S_IDLE, NGF_S_CROUCH, NGF_S_JUMP, NGF_S_ATTACK, NGF_S_BLOCK, NGF_S_HIT,
       NGF_S_FALL, NGF_S_DOWN, NGF_S_GETUP, NGF_S_DASH, NGF_S_BACKDASH,
       NGF_S_THROWN, NGF_S_DIZZY, NGF_S_KO, NGF_S_WIN, NGF_S_LOSE, NGF_S_INTRO };

#define NGF_POWER_STOCK 128u
#define NGF_POWER_MAX   (3u * NGF_POWER_STOCK)
#define NGF_SPIRIT_MAX  128u

typedef struct {
    const NGFFighterDef *def;
    int32_t  x, h;                     /* Q8 */
    int16_t  vx, vh;                   /* Q8 per frame */
    int16_t  push_vx;                  /* Q8, decays */
    uint8_t  facing_left, state, state_timer, grounded;
    uint8_t  anim, frame, frame_timer, move_hit;
    const NGFMove *move;
    uint8_t  move_index, air_used, block_low, counter;
    uint16_t life, power;
    uint8_t  guard, spirit, combo, palette;
    uint16_t combo_damage;
    uint8_t  wins, invuln, stun_frames, cpu;
    NGFInput input;
} NGFFighter;

typedef struct {
    const NGFProjectileDef *def;
    int32_t x, h;
    uint8_t owner, facing_left, life, frame, frame_timer, active;
} NGFProjectile;

#define NGF_PROJECTILES 4u
#define NGF_EVENTS      8u
enum { NGF_EV_NONE, NGF_EV_HIT, NGF_EV_BLOCK, NGF_EV_THROW, NGF_EV_TECH, NGF_EV_GUARD_BREAK,
       NGF_EV_KO, NGF_EV_MOVE, NGF_EV_PROJECTILE, NGF_EV_ROUND, NGF_EV_FIGHT, NGF_EV_TIME_UP,
       NGF_EV_LAND, NGF_EV_WIN, NGF_EV_COUNTER, NGF_EV_SUPER_FREEZE, NGF_EV_STRIKER, NGF_EV_TAG };
typedef struct { uint8_t type, who, data, pad; int16_t x, y; } NGFEvent;

enum { NGF_M_INTRO, NGF_M_FIGHT, NGF_M_KO, NGF_M_TIME_UP, NGF_M_ROUND_END, NGF_M_OVER };
enum { NGF_MODE_1V1 = 0, NGF_MODE_3V3 = 1, NGF_MODE_TAG = 2 };

typedef struct {
    uint8_t seq[16];          /* queued raw inputs */
    uint8_t len, pos, wait, level, hold, pad;
} NGFCpu;

typedef struct {
    const NGFFighterDef *def;
    int32_t x, h;                     /* Q8 */
    int16_t vx;                       /* Q8 */
    uint8_t facing_left, state, state_timer, active;
    uint8_t anim, frame, frame_timer, cooldown;
    uint8_t owner, move_index, move_hit;
} NGFStriker;

typedef struct {
    NGFFighter f[2];
    NGFProjectile proj[NGF_PROJECTILES];
    NGFEvent ev[NGF_EVENTS];
    NGFCpu cpu[2];
    uint8_t ev_count, state, round, winner;   /* winner: 0/1, 2 = draw, 0xFF none */
    uint16_t state_timer, clock;             /* clock: frames left in the round */
    uint8_t seconds, hitstop, rounds_to_win, round_winner;
    int16_t stage_w, camera_x, screen_w, floor_y;
    uint16_t rng;
    uint32_t frame;
    uint8_t super_freeze;                     /* frames remaining in super freeze */
    uint8_t super_who;                        /* attacker who activated super */
    uint8_t zoom_scale;                       /* SCB2 shrink factor (192..255) */
    uint8_t match_mode;                       /* NGF_MODE_* */
    const NGFFighterDef *team[2][3];          /* 3v3 / tag rosters */
    uint8_t team_count[2];                    /* members per team */
    uint8_t team_active[2];                   /* current active member */
    uint16_t tag_reserve[2];                  /* off-screen partner reserve life */
    uint16_t team_life[2][3];
    const NGFFighterDef *assist[2];
    uint8_t assist_uses[2];
    NGFStriker striker[2];                    /* striker summon actors */
} NGFMatch;

#define NGF_ROUND_SECONDS 60u
#define NGF_SECOND_FRAMES 60u

void     NEOGEO_USER ngf_match_init(NGFMatch *m, const NGFFighterDef *p1, const NGFFighterDef *p2,
                                    int16_t stage_w, uint16_t seed);
void     NEOGEO_USER ngf_round_start(NGFMatch *m);
/* One frame: raw controller bits for each side (ignored for CPU sides). */
void     NEOGEO_USER ngf_match_step(NGFMatch *m, uint8_t raw1, uint8_t raw2);
void     NEOGEO_USER ngf_set_cpu(NGFMatch *m, uint8_t who, uint8_t level);  /* level 0 = human */
uint8_t  NEOGEO_USER ngf_cpu_input(NGFMatch *m, uint8_t who);
uint8_t  NEOGEO_USER ngf_pop_event(NGFMatch *m, NGFEvent *out);
const NGFFrame *NEOGEO_USER ngf_fighter_frame(const NGFFighter *f);
const NGFFrame *NEOGEO_USER ngf_projectile_frame(const NGFMatch *m, const NGFProjectile *p);
int16_t  NEOGEO_USER ngf_fighter_px(const NGFFighter *f);   /* world x, px */
int16_t  NEOGEO_USER ngf_fighter_py(const NGFFighter *f);   /* height, px */
uint16_t NEOGEO_USER ngf_distance(const NGFMatch *m);
uint32_t NEOGEO_USER ngf_match_hash(const NGFMatch *m);
void     NEOGEO_USER ngf_match_set_mode(NGFMatch *m, uint8_t mode);
void     NEOGEO_USER ngf_team_set(NGFMatch *m, uint8_t who, const NGFFighterDef *f1,
                                  const NGFFighterDef *f2, const NGFFighterDef *f3);
void     NEOGEO_USER ngf_striker_set(NGFMatch *m, uint8_t who, const NGFFighterDef *def);
uint8_t  NEOGEO_USER ngf_call_striker(NGFMatch *m, uint8_t who);
uint8_t  NEOGEO_USER ngf_tag_switch(NGFMatch *m, uint8_t who);

#ifdef __cplusplus
}
#endif
#endif
