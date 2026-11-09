#ifndef CX_SIM_H
#define CX_SIM_H

#include <stdint.h>
#include "ng_soccer.h"

#ifndef CX_CODE
#define CX_CODE __attribute__((section("neogeo_user")))
#endif

#define CX_UP    0x0001u
#define CX_DOWN  0x0002u
#define CX_LEFT  0x0004u
#define CX_RIGHT 0x0008u
#define CX_A     0x0010u
#define CX_B     0x0020u
#define CX_C     0x0040u
#define CX_D     0x0080u
#define CX_START 0x0100u

/* The pitch, in units; the screen draws it through the tables in cx_proj.h.
 * x runs from the left goal line to the right one, y from the far touchline
 * to the near one. Positions and speeds are Q4: 16 to a unit. Team 0 attacks
 * the right goal in the first half. */
#define CX_Q        16
/* One instruction each (ng_soccer): written as plain C, 16-bit operands are promoted to
 * int, and a 68000 has no 32-bit multiply or divide, so the compiler calls a library
 * routine of several hundred cycles for a / 5 or a * s. On the host they are plain C. */
static inline int16_t cx_div(int16_t a, int16_t b) { return ngs_div(a, b); }
static inline int32_t cx_mul(int16_t a, int16_t b) { return ngs_mul(a, b); }
/* v times a direction of +1 or -1 */
static inline int16_t cx_sgn(int16_t v, int16_t s) { return (int16_t)(s > 0 ? v : -v); }

#define CX_LEN      1536
#define CX_WID      704
#define CX_MID_Y    352
#define CX_POST     88            /* half the goal mouth */
#define CX_BAR      40            /* crossbar height in units */
#define CX_BOX_D    240           /* penalty area depth */
#define CX_BOX_W    300           /* half its width */
#define CX_SPOT     160           /* penalty spot from the goal line */
#define CX_SIX_D    84            /* six-yard box depth */
#define CX_SIX_W    150           /* half its width */
#define CX_WALL_D   130           /* the wall's distance: 9.15 m of a 105 m pitch */
#define CX_SQUADS   48            /* nations: tools/nations.py; cx_nations.c checks they agree */
#define CX_MEN      22            /* 0-10 team 0 (0 the keeper), 11-21 team 1 */
#define CX_HALF_FRAMES 9000       /* the default: 2.5 real minutes a half, a 5-minute match; the clock shows 45 */
#define CX_NOBODY   0xFFu

enum { CX_M_RUN, CX_M_KICK, CX_M_SLIDE, CX_M_DOWN, CX_M_HEAD, CX_M_CELEB, CX_M_THROW,
       CX_M_DIVE, CX_M_HOLD, CX_M_TACKLE, CX_M_BEATEN, CX_M_FEINT, CX_M_OFF, CX_M_SAD, CX_M_WIND,
       CX_M_ROLL, CX_M_ARGUE, CX_M_FLY, CX_M_VOLLEY, CX_M_BIKE, CX_M_THROWN };

/* What sort of footballer a man is: how the computer plays him, and what he does best. */
enum { CX_T_PLAIN, CX_T_SPEED, CX_T_DRIBBLE, CX_T_TARGET, CX_T_PLAYMAKER, CX_T_SHOOTER, CX_T_STOPPER, CX_T_SETPIECE };
/* How a kick looked, for the picture. */
enum { CX_K_PASS, CX_K_LONG, CX_K_SHOT };
enum { CX_CK_SPOT, CX_CK_NEAR, CX_CK_FAR, CX_CK_SHORT };     /* corner deliveries */
enum { CX_TH_FEET, CX_TH_LINE, CX_TH_LONG };                 /* throw-ins */
/* How a goal was scored: what the ball last did, kept for the captions and the crowd. */
enum { CX_G_SHOT, CX_G_LONG, CX_G_TAPIN, CX_G_HEADER, CX_G_DIVING, CX_G_VOLLEY, CX_G_BIKE, CX_G_CHIP,
       CX_G_FREEKICK, CX_G_PENALTY, CX_G_REBOUND, CX_G_OWN };

/* How a side celebrates: each nation has its own way (tools/nations.py). */
enum { CX_C_RUN, CX_C_KNEE, CX_C_SHIRT, CX_C_BABY, CX_C_DANCE, CX_C_HIPS, CX_C_BELLY, CX_C_FLIP, CX_C_SAMBA, CX_C_PRAY,
       CX_C_SCREAM, CX_C_SKY, CX_C_SKATE, CX_C_JUMP };

/* Extra time for a tie that must be settled. */
enum { CX_ET_NORMAL, CX_ET_GOLDEN, CX_ET_NONE };
enum { CX_P_KICKOFF, CX_P_PLAY, CX_P_GOAL, CX_P_THROW, CX_P_GOALKICK, CX_P_CORNER, CX_P_FREEKICK,
       CX_P_PENALTY, CX_P_HALF, CX_P_FULL, CX_P_ETBREAK, CX_P_PENS, CX_P_OVER, CX_P_VAR, CX_P_STOP };

/* Events of one tick (CxWorld.ev), for sound, the camera and the captions. */
#define CX_EV_KICK    0x00000001u
#define CX_EV_PASS    0x00000002u
#define CX_EV_GOAL    0x00000004u
#define CX_EV_SAVE    0x00000008u
#define CX_EV_WHISTLE 0x00000010u
#define CX_EV_TACKLE  0x00000020u
#define CX_EV_POST    0x00000040u
#define CX_EV_OUT     0x00000080u
#define CX_EV_BOUNCE  0x00000100u
#define CX_EV_FOUL    0x00000200u
#define CX_EV_CARD    0x00000400u
#define CX_EV_OFFSIDE 0x00000800u
#define CX_EV_FEINT   0x00001000u
#define CX_EV_ADV     0x00002000u
#define CX_EV_ADDED   0x00004000u
#define CX_EV_SUB     0x00008000u
#define CX_EV_HANDBALL 0x00010000u
#define CX_EV_DIVE    0x00020000u
#define CX_EV_INJURY  0x00040000u
#define CX_EV_SHOT    0x00080000u
#define CX_EV_MISS    0x00100000u    /* a shot that went wide */
#define CX_EV_NUTMEG  0x00200000u
#define CX_EV_FUMBLE  0x00400000u
#define CX_EV_SPECIAL 0x00800000u    /* a volley, an overhead kick, a diving header */
#define CX_EV_THROW   0x01000000u    /* a throw-in left his hands */
#define CX_EV_CHALLENGE 0x02000000u  /* a side has asked for a review */
#define CX_EV_BACKHEEL 0x04000000u
#define CX_EV_THROUGH 0x08000000u
#define CX_EV_ONETWO  0x10000000u
#define CX_EV_CLAIM   0x20000000u    /* a keeper caught a cross */
#define CX_EV_PUNCH   0x40000000u    /* a keeper punched one clear */
#define CX_CHARGE_MAX 26             /* frames of B to a full-power shot */
#define CX_VAR_FRAMES 240

/* How a one-on-one ended and why (CxWorld.duel_res, duel_why). */
enum { CX_DUEL_NONE, CX_DUEL_BEAT, CX_DUEL_STEP, CX_DUEL_FAIL, CX_DUEL_LOST };
enum { CX_WHY_NONE, CX_WHY_TIMING, CX_WHY_COMMITTED, CX_WHY_SKILL, CX_WHY_CLOSE, CX_WHY_FAR, CX_WHY_READ };
enum { CX_WAY_SWERVE, CX_WAY_UP, CX_WAY_DOWN, CX_WAY_BACK };

/* Which laws a match is played under. Retro keeps the 1990s reading (a tackle
 * from behind is only a booking, no video, three substitutes); modern is
 * stricter and may have video review. */
#define CX_RULES_RETRO  0u
#define CX_RULES_MODERN 1u
#define CX_VAR_OFF      0u
#define CX_VAR_SHORT    1u
#define CX_VAR_FULL     2u
#define CX_VAR_CHALLENGE 3u      /* only when a side asks: two chances a match */

typedef struct {
    int16_t x, y;                 /* Q4 */
    int16_t vx, vy;
    int16_t hx, hy;               /* home position in the team's shape, units, attacking right */
    int16_t tx, ty;               /* where his last thought sent him, units */
    uint8_t team, role, state, dir;      /* dir 0-7: 0 right, 2 down (toward the camera), 4 left, 6 up */
    uint8_t timer, anim, card, line;     /* card: yellows shown; line: 0 keeper 1 back 2 middle 3 front */
    uint8_t pace, shot, ctrl, tack;      /* attributes, 1-16 */
    uint8_t squad, fig, skin, hair;      /* who he is in the roster, and how he looks */
    uint8_t sta, type;                   /* stamina, 255 fresh; what kind of player he is */
    int16_t gx, gy;                      /* the speed he is trying to reach: he gets there by momentum */
    uint8_t run_t, stride;               /* frames at it, walk to run; distance into the next step */
    uint8_t kick, aimv;                  /* how his last kick looked; a wound-up shot's aim */
    uint8_t mark;                        /* the opponent he marks when his side defends (CX_NOBODY: his zone) */
    uint8_t build;                       /* 0 as drawn, 1 tall (drawn a size up), 2 slight (a size down) */
    uint8_t pad[20];                     /* to 64 bytes: the 68000 finds w->m[i] with one shift; at 42
                                          * every access was a multiply, a third of the movement's time */
} CxMan;

/* The ball is ng_soccer's: position, speed and height in Q4, swerve, owner, the last
 * man to touch it, kind (a CX_G_ for the goal it would be), striker, fast. */
typedef NGSBall CxBall;

/* One thing the referee's record keeps: the review reads this, never a guess. */
enum { CX_LOG_NONE, CX_LOG_GOAL, CX_LOG_FOUL, CX_LOG_PEN, CX_LOG_OFFSIDE, CX_LOG_CARD, CX_LOG_RED };
/* What a side's ratings, the level and the match make of its play: worked out
 * through ng_tune at kick-off and a few times a second, never per man per frame. */
typedef struct {
    uint8_t level;                /* 0..7: the chosen level plus the cup's climb (a player's side: 3) */
    int8_t nudge;                 /* rating shift for the score and the clock; 0 on a player's side */
    uint8_t think;                /* frames between the decisions of its man on the ball */
    uint8_t bite;                 /* chance in 256 of going in for the ball when close */
    uint8_t chase;                /* speed its man pressing the ball adds */
    int8_t keep;                  /* its keeper's skill shift */
    int8_t aim;                   /* added to a shot's spread (less is truer) */
    int8_t pace;                  /* added to its outfielders' running speed */
    uint8_t look;                 /* mask of frames between a man's positional thoughts (15 or 7) */
    uint8_t cpu;                  /* 1: the CPU plays this side */
    uint8_t rate[6];              /* the side's ratings 0..31: speed, power, press, skill, defend, keeper */
} CxBrain;

typedef struct {
    uint16_t clock;
    uint8_t type, man, other, half;
    int16_t x, y, margin;
} CxLog;
#define CX_LOG_N 16

typedef struct CxWorld {
    CxMan m[CX_MEN];
    CxBall ball;
    uint32_t ev;
    uint16_t frame, clock, phase_t;
    uint16_t half_frames;         /* a half in frames: the match-length option */
    uint16_t fpm;                 /* frames a shown minute */
    uint16_t limit;               /* the frame the half ends, added time included */
    uint16_t stopped;             /* frames the ball was dead this half */
    uint16_t off_mask[2];         /* men offside when their side last played the ball forward */
    int16_t off_margin;           /* units past the line of the man the assistant did not flag */
    int16_t foul_x, foul_y;       /* where the advantage will come back to */
    uint8_t off_man;              /* the marginal man (CX_NOBODY: none) */
    uint8_t off_live;             /* and he has played the ball */
    uint8_t phase, half, team[2], score[2];
    uint8_t ctrl[2];              /* each side's controlled man (CX_NOBODY: the CPU plays it) */
    uint8_t human[2];             /* 1: a player holds that side */
    uint8_t restart_team, restart_man, kickoff_team, stadium;
    uint8_t et;                   /* a tie that must be settled: extra time, then penalties */
    uint8_t et_mode;              /* CX_ET_ */
    uint8_t goal_kind;            /* the last goal's CX_G_ */
    uint8_t celeb;                /* how the scorer celebrates */
    uint8_t save_t;               /* frames since a save: a goal now is a rebound */
    uint8_t ref_style;            /* 0 lenient, 1 fair, 2 card-happy */
    uint8_t injured;              /* the man the physio is coming for (CX_NOBODY: nobody) */
    uint8_t form[2], tactic[2], press[2];        /* formation; 0 defend 1 balanced 2 attack; 0 sit 1 press */
    uint8_t sub_role[2], sub_in[2];              /* a substitution waiting for the next stoppage */
    uint8_t scorer;
    uint8_t classic_controls;
    uint8_t latch;                /* buttons a player pressed while a restart was being set up */
    uint8_t rules, var;
    uint8_t hold_t, last_owner;   /* frames the man on the ball has had it */
    uint8_t level;                /* 0..7 (levels 1..8): the CPU's level as chosen; brain[] holds what it plays at */
    uint8_t added;                /* minutes the board showed */
    uint8_t adv_t, adv_team, adv_man, adv_card; /* advantage: frames left, who has it, whose card follows */
    uint8_t duel_a, duel_d, duel_res, duel_why; /* the one-on-one now, and the last result */
    uint8_t chord[2], chord_btn[2];      /* a first button held back a moment for a feint */
    uint8_t aim;                  /* a set piece's aim: 0 centre, 1 far post, 2 near post */
    uint8_t card_man, card_red;   /* the last card, for the caption */
    uint8_t subs[2];
    uint8_t shots[2], fouls[2], yellows[2], reds[2];
    uint8_t log_n;
    uint8_t mental[2];            /* 0 sits deep, 1 balanced, 2 chases the game */
    uint8_t chase[2][2];          /* each side's two men nearest the ball */
    uint8_t pass_to;              /* who a pass is meant for */
    uint8_t wall_man[2], wall_t[2], wall_go[2]; /* a one-two: who passed, how long he runs, and "give it back" */
    uint8_t runner[2];            /* the man making the run this moment */
    uint8_t challenges[2];        /* reviews a side may still ask for */
    uint8_t wrong;                /* the referee's last call was a mistake: the record knows */
    uint8_t wrong_card;           /* and the man booked for it */
    uint8_t asked;                /* the side that asked for this review */
    uint8_t pend_phase, pend_team;/* play has stopped: the restart that will follow */
    int16_t pend_x, pend_y;
    uint8_t stop_t;
    uint8_t momentum[2];          /* who is on top: the terraces follow it */
    uint8_t review;               /* a video review waiting: the log entry's type */
    uint8_t review_keep;          /* the review's answer: 1 the call stands */
    uint8_t review_man;           /* the man it concerns */
    uint8_t review_team;          /* the side it favours when it is overturned */
    int16_t review_x, review_y;   /* where play restarts if it is */
    CxLog log[CX_LOG_N];
    uint32_t rng;
    uint8_t keeper_foul;          /* this turn's foul was a hand laid on a keeper */
    uint8_t stop_ev;              /* this half's hold-ups, for the added time: 4 a foul, card, sub or goal, 1 a throw-in */
    uint8_t fk_wall[5];           /* a free kick's wall, its men */
    uint8_t fk_wall_n;            /* how many (0: no wall) */
    uint8_t fk_t;                 /* frames since the kick: the wall is live while it jumps */
    uint8_t fk_read;              /* the keeper read the free kick: the wall did not unsight him */
    int16_t fk_wx, fk_wy, fk_nx, fk_ny;  /* where the wall stands (its middle) and which way it runs */
    CxBrain brain[2];
    uint8_t round;                /* cup rounds already played: the CPU sharpens as a run goes on */
    uint8_t think_t[2];           /* frames until each side's man on the ball decides again */
    uint8_t charge[2];            /* a player's shot being wound up: frames B has been held (0: none) */
    uint8_t aim_on;               /* a player's set piece in the aim view (D): the stick moves the target */
    int16_t aim_x, aim_y;         /* the target, in units */
    uint8_t aerial_t[2];          /* a player asked for a ball in the air: frames he keeps going for it */
    uint8_t aerial_shot[2];       /* and whether at goal (B) or on to a team-mate (A) */
    uint8_t meet_t;               /* a ball in the air: frames until it drops to head height (0: none) */
    int16_t meet_x, meet_y;       /* and where, in units */
    uint8_t recv_dir[2];          /* a player's pass: the stick's way as he played it (0xFF at rest);
                                   * 0xFE once he points somewhere else and steers the receiver himself */
    uint8_t first_go[2];          /* a player's call as a pass comes to his man: 1 pass it on first time, 2 shoot first time */
    uint8_t first_dir[2];         /* and the stick's way for that pass */
    uint8_t keep_manual[2];       /* a player has taken his keeper for the shot coming in (stick or button) */
    uint8_t celeb_last[2];        /* each side's last celebration: the next one is another */
} CxWorld;

/* A shoot-out: five each, then one each until it splits. Nothing here ever
 * forces a winner: it ends only when the numbers do. */
typedef struct {
    uint8_t score[2], taken[2];
    uint8_t turn;                 /* the side to kick */
    uint8_t over, winner;
    uint8_t order;                /* who took first */
    uint32_t rng;
} CxShootout;

/* Eight-team knock-out: the Classic Arcade Cup. team[0] is the player's side. */
typedef struct CxCup {
    uint8_t team[8];
    uint8_t round;                /* 3: the cup is over */
    uint8_t won;
    uint8_t foe;
    uint8_t pad;
} CxCup;

/* A match: team_l plays left to right first; humans: bit 0 team 0, bit 1 team 1. */
void CX_CODE cx_init(CxWorld *w, uint8_t team_l, uint8_t team_r, uint8_t humans, uint8_t stadium);
void CX_CODE cx_tick(CxWorld *w, uint16_t held0, uint16_t pressed0, uint16_t held1, uint16_t pressed1);
/* Which goal a team attacks in the current half: 1 the right one. */
static inline uint8_t cx_attacks_right(const CxWorld *w, uint8_t team)
{
    return (uint8_t)((team == 0) ^ ((w->half & 1u) != 0));
}
/* Asks for a substitution: bench 0-4 for the man in slot 1-10, at the next stoppage. 0 refused. */
uint8_t CX_CODE cx_sub_request(CxWorld *w, uint8_t team, uint8_t role, uint8_t bench);
/* Makes any waiting substitution now, the ball being dead. */
void CX_CODE cx_subs_apply(CxWorld *w);
/* The side's shape, 0 4-4-2, 1 4-3-3, 2 3-5-2, 3 5-3-2. */
void CX_CODE cx_set_shape(CxWorld *w, uint8_t team, uint8_t form);
/* A match's length in real minutes for the whole game, extra time and golden goal. */
void CX_CODE cx_set_length(CxWorld *w, uint8_t minutes, uint8_t et_mode);
/* A side asks for a review of the call against it. 0: it may not. */
uint8_t CX_CODE cx_var_challenge(CxWorld *w, uint8_t team);
/* Is a review open to this side now? */
uint8_t CX_CODE cx_var_open(const CxWorld *w, uint8_t team);
uint8_t CX_CODE cx_man_type(uint8_t team, uint8_t squad);
uint8_t CX_CODE cx_nation_celebration(uint8_t team);
uint8_t CX_CODE cx_kit_ink(uint8_t nation, uint8_t kit);
uint8_t CX_CODE cx_squad_count(void);
uint8_t CX_CODE cx_squad_power(uint8_t id);
uint8_t CX_CODE cx_squad_pace(uint8_t id);
uint8_t CX_CODE cx_squad_style(uint8_t id);
uint8_t CX_CODE cx_squad_stat(uint8_t id, uint8_t which);
void CX_CODE cx_squad_name(uint8_t id, char *dst);
void CX_CODE cx_squad_code(uint8_t id, char *dst);
void CX_CODE cx_man_name(uint8_t team, uint8_t squad, char *dst);
uint16_t CX_CODE cx_man_voice(uint8_t team, uint8_t squad);
uint16_t CX_CODE cx_chant_voice(uint8_t nation);
extern uint16_t cx_absent[2];
void CX_CODE cx_lineup(uint16_t absent, uint8_t out[11]);
uint16_t CX_CODE cx_nation_flag(uint8_t nation, uint8_t which);
/* A roster man's figure, skin and hair. */
uint8_t CX_CODE cx_man_look(uint8_t team, uint8_t squad, uint8_t *skin, uint8_t *hair);
const uint16_t *CX_CODE cx_kit_words(uint8_t nation, uint8_t kit);
const uint16_t *CX_CODE cx_skin_words(uint8_t i);
const uint16_t *CX_CODE cx_hair_words(uint8_t i);
/* The kit a side wears against another (0 home, 1 away). */
uint8_t CX_CODE cx_kit_for(uint8_t team, uint8_t foe);
void CX_CODE cx_cup_init(CxCup *c, uint8_t human_team);
void CX_CODE cx_cup_apply(CxCup *c, uint8_t human_won);
/* The score of a game nobody plays: strength decides, the seed varies it. */
void CX_CODE cx_quick_result(uint8_t a, uint8_t b, uint32_t *seed, uint8_t *ga, uint8_t *gb);

void CX_CODE cx_shootout_init(CxShootout *s, uint8_t first, uint32_t seed);
/* Does the kick go in? aim and dive 0 centre 1 up 2 down; a side's ratings decide
 * the close ones. Records it and moves the turn on. */
uint8_t CX_CODE cx_shootout_kick(CxShootout *s, uint8_t aim, uint8_t dive, uint8_t taker_skill, uint8_t keeper_skill);
/* One penalty's outcome: the same column is a save unless it is struck well, a different
 * one goes in unless it flies wide. Then the record of a kick already decided. */
uint8_t CX_CODE cx_pen_roll(uint8_t aim, uint8_t dive, uint8_t taker_skill, uint8_t keeper_skill, uint32_t *dice);
void CX_CODE cx_shootout_record(CxShootout *s, uint8_t scored);
/* A penalty the show took in its own view: the sim hears the outcome. */
void CX_CODE cx_penalty_taken(CxWorld *w, uint8_t scored);
/* Plays out a shoot-out nobody takes part in. */
void CX_CODE cx_shootout_auto(CxShootout *s, uint8_t a, uint8_t b);

#endif
