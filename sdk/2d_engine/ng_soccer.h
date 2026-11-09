#ifndef NG_SOCCER_H
#define NG_SOCCER_H
/*
 * ng_soccer: association football's ball, lines and championship for a Neo
 * Geo game (v1.7.8 genre engine). Selected with GAME_GENRE_ENGINE = soccer in
 * game.mk; Century XI: Road to 2030 (games/centuryxi) is built on it.
 *
 * What is here is the part every football game needs and that has no look of
 * its own:
 *   - word-sized arithmetic the 68000 does in one instruction (ngs_mul,
 *     ngs_div): written as plain C, a 16-bit a * b or a / b is promoted to
 *     int and becomes a library call of several hundred cycles;
 *   - the ball in world space (NGSBall): position, velocity, height and
 *     vertical speed in Q4 pitch units, gravity and air drag, bounces that
 *     take pace off it, the grass's grip (a struck ball skims until it slows),
 *     and a swerve that turns its flight whichever way it flies, with the
 *     fractions of each frame's push kept so small bends add up;
 *   - passes weighted for that grass: the speed that brings a ground ball to
 *     rest a little past a point d units away, so it arrives firm;
 *   - where a ball in the air comes down to a given height, and when, for
 *     keepers and the men who go up for it;
 *   - the goal lines: goal, posts and bar (with the rebound), wide, and the
 *     touchlines;
 *   - offside: the second-last defender's line (or the ball, or halfway),
 *     and the attackers past it when the ball is played;
 *   - a 32-side championship (NGSCup): up to 64 nations, the 32 who qualify
 *     by strength plus luck, eight groups of four, 48 group matches, round of
 *     16, quarter-finals, semi-finals, third place and the final (64 in all),
 *     tables at 3/1/0 points with a fixed tie-break, the scorers, and the
 *     human side's bookings carried from match to match with suspensions.
 *
 * Players, tactics, the computer's thinking, the referee's manner, cameras,
 * drawing and sound are the game's: the engine never touches video RAM or the
 * sound driver. Everything lives in the caller's structures: no heap, no
 * floating point, no hidden globals, no recursion, no 32-bit divide.
 *
 * Units. The pitch runs x from the left goal line (0) to the right one (len)
 * and y from the far touchline (0) to the near one (wid); the goal mouth is
 * centred on wid / 2. Ball positions and speeds are Q4 (16 to a unit), a
 * speed being Q4 units a frame. One ngs_ball_fly() is one frame.
 */
#include <stdint.h>
#include "ng_defs.h"
#ifdef __cplusplus
extern "C" {
#endif

#define NGS_Q          16          /* Q4: sixteen to a pitch unit */
#define NGS_NOBODY     0xFFu       /* a ball with no owner, a fixture with no winner */
#define NGS_OK         0
#define NGS_ERR_ARG    (-1)
#define NGS_ERR_STATE  (-2)

/* ------------------------------------------------------------ arithmetic */

/* Inlined at every level, -O0 included: an out-of-line copy would land outside the
 * game's code section. */
#if defined(__GNUC__)
#define NGS_INLINE static inline __attribute__((always_inline))
#else
#define NGS_INLINE static inline
#endif

/* a / b, both 16-bit: one DIVS. The quotient must fit 16 bits. */
NGS_INLINE int16_t ngs_div(int16_t a, int16_t b)
{
#ifdef __m68k__
    int32_t r = a;
    __asm__ ("divs.w %1,%0" : "+d" (r) : "dmi" (b) : "cc");
    return (int16_t)r;
#else
    return (int16_t)(a / b);
#endif
}
/* a * b, 16 by 16 to 32 bits: one MULS. */
NGS_INLINE int32_t ngs_mul(int16_t a, int16_t b)
{
#ifdef __m68k__
    int32_t r = a;
    __asm__ ("muls.w %1,%0" : "+d" (r) : "dmi" (b) : "cc");
    return r;
#else
    return (int32_t)a * b;
#endif
}
NGS_INLINE int16_t ngs_abs(int16_t v) { return (int16_t)(v < 0 ? -v : v); }
NGS_INLINE int16_t ngs_clamp(int16_t v, int16_t lo, int16_t hi) { return v < lo ? lo : (v > hi ? hi : v); }
/* The integer square root, a bit at a time (no divide). */
uint16_t NEOGEO_USER ngs_isqrt32(int32_t v);

/* ------------------------------------------------------------ the pitch */

typedef struct {
    int16_t len;            /* goal line to goal line, units */
    int16_t wid;            /* touchline to touchline, units */
    int16_t post;           /* half the goal mouth, units */
    int16_t bar;            /* the crossbar's height, units */
    int16_t gravity;        /* Q4 a frame, each frame: 3 suits a 1536-unit pitch at 60 frames a second */
} NGSPitch;

/* ------------------------------------------------------------ the ball */

typedef struct {
    int16_t x, y, h;        /* Q4; h is the height above the grass */
    int16_t vx, vy, vh;     /* Q4 a frame */
    int16_t curve;          /* swerve: a push square to its flight of curve/4 Q a frame, clockwise when positive */
    int16_t from_x;         /* where it was last struck, units */
    uint8_t owner;          /* the man who has it, or NGS_NOBODY */
    uint8_t last;           /* the last man to touch it */
    uint8_t spin;           /* turns with distance rolled: for the picture */
    uint8_t kind;           /* the game's own note of what the strike was */
    uint8_t striker;        /* who struck it at goal */
    uint8_t fast;           /* struck hard: the grass bites a quarter as often until it slows */
    int16_t bend_rx, bend_ry;   /* the part of the swerve not yet added, in 1024ths */
} NGSBall;

/* Events from ngs_ball_fly(). */
#define NGS_FLY_BOUNCE 0x01u

/* One frame of a free ball (owner NGS_NOBODY; the game moves an owned one):
 * moves it, turns it by its swerve, and in the air pulls it down and drags
 * it, on the grass bounces it (taking a quarter of its pace), halves the
 * swerve and lets the grass grip, a struck (fast) ball only every fourth
 * frame. `frame` is any counter that goes up by one a frame. */
uint8_t NEOGEO_USER ngs_ball_fly(NGSBall *b, int16_t gravity, uint16_t frame);
/* The grass's grip on one speed component for one frame. */
int16_t NEOGEO_USER ngs_grip(int16_t v);
/* Strikes the ball from where it is toward (tx, ty) units at `speed` (Q4 a
 * frame along its line) with vertical speed `vh`: clears swerve, owner and
 * the fast flag and notes `from` as last and striker. */
void NEOGEO_USER ngs_ball_strike(NGSBall *b, uint8_t from, int16_t tx, int16_t ty, int16_t speed, int16_t vh);
/* A swerve of `amount` that turns the flight toward (dx, dy), and one that
 * pushes the ball toward +y when it flies right and -y when it flies left. */
int16_t NEOGEO_USER ngs_bend_toward(const NGSBall *b, int16_t dx, int16_t dy, int16_t amount);
NGS_INLINE int16_t ngs_bend_y(const NGSBall *b, int16_t push_y) { return (int16_t)(b->vx >= 0 ? push_y : -push_y); }

/* A ground pass over d units: the speed that leaves it rolling to rest
 * about 40 units past the spot under ngs_grip, so it arrives firm. */
int16_t NEOGEO_USER ngs_pass_speed(int16_t d);

/* A free ball in the air: the frames until it drops to `meet_h` (Q4) above
 * the grass, or to the grass when it never climbs that high, and where (in
 * units) through *mx, *my. 0 when it is owned or on the grass; at most 90. */
uint8_t NEOGEO_USER ngs_ball_meet(const NGSBall *b, int16_t meet_h, int16_t gravity, int16_t *mx, int16_t *my);

/* ------------------------------------------------------------ the lines */

enum {
    NGS_LINE_NONE = 0,      /* in play */
    NGS_LINE_GOAL,          /* under the bar between the posts */
    NGS_LINE_FRAME,         /* struck a post or the bar: ngs_frame_rebound() */
    NGS_LINE_WIDE,          /* over a goal line: a corner or a goal kick */
    NGS_LINE_TOUCH          /* over a touchline: a throw-in */
};
/* Where the ball is against the lines; *right is 1 at the right-hand goal line. */
uint8_t NEOGEO_USER ngs_ball_line(const NGSBall *b, const NGSPitch *p, uint8_t *right);
/* A ball at a goal line `off` units from the goal's centre: does it meet a post or the bar? */
uint8_t NEOGEO_USER ngs_frame_test(const NGSBall *b, const NGSPitch *p, int16_t off);
/* ...then it comes back off it: half its pace, a little sideways from `rnd`. */
void NEOGEO_USER ngs_frame_rebound(NGSBall *b, const NGSPitch *p, uint8_t right, uint16_t rnd);

/* ------------------------------------------------------------ offside */

/* The defending side's positions measured toward the goal being attacked
 * (x for a side attacking right, -x for one attacking left), the ball's and
 * the halfway line's measured the same way: the offside line. */
int16_t NEOGEO_USER ngs_offside_line(const int16_t *def, uint8_t n, int16_t ball, int16_t halfway);
/* The attackers (measured the same way, -32768 for any to leave out) more
 * than `margin` past the line, a bit each; the one nearest past it within
 * the margin (a close call) through *close and by how much through *by
 * (NGS_NOBODY and 0 when none). */
uint16_t NEOGEO_USER ngs_offside_mask(const int16_t *att, uint8_t n, int16_t line, int16_t margin,
                                      uint8_t *close, int16_t *by);

/* ------------------------------------------------------------ the championship */

#define NGS_CUP_SIDES     32u
#define NGS_CUP_GROUPS    8u
#define NGS_CUP_MATCHES   64u
#define NGS_CUP_GROUP_MATCHES 48u
#define NGS_CUP_SCORERS   16u
#define NGS_CUP_SQUAD     16u      /* squad numbers whose bookings are carried */
#define NGS_CUP_NATIONS_MAX 64u

/* What the fixture index is: 0-47 groups, 48-55 round of 16, 56-59
 * quarter-finals, 60-61 semi-finals, 62 third place, 63 the final. */
enum { NGS_ROUND_GROUP, NGS_ROUND_16, NGS_ROUND_QUARTER, NGS_ROUND_SEMI, NGS_ROUND_THIRD, NGS_ROUND_FINAL };

typedef struct { uint8_t played, wins, draws, losses; uint16_t scored, conceded; uint8_t points; } NGSStanding;
typedef struct { uint8_t home, away, goals[2], winner, played; } NGSFixture;
typedef struct { uint8_t team, squad, goals; } NGSScorer;
typedef struct {
    NGSStanding table[NGS_CUP_SIDES];   /* by the slot a side was drawn into (groups[]), not by nation */
    NGSFixture fixture[NGS_CUP_MATCHES];
    uint8_t groups[NGS_CUP_SIDES];      /* the draw: group g is groups[g*4 .. g*4+3] */
    uint8_t human;                      /* the player's nation */
    uint8_t generated;                  /* fixtures known so far: 48, 56, 60, 62, 64 */
    uint8_t completed;                  /* results recorded */
    uint8_t champion;                   /* NGS_NOBODY until the final is played */
    uint32_t seed;
    NGSScorer scorers[NGS_CUP_SCORERS]; /* goalscorers, in no order: ngs_cup_top() sorts */
    uint8_t yellows[NGS_CUP_SQUAD];     /* the human side's bookings carried, by squad number */
    uint16_t suspended;                 /* the human side's men out of the next match, a bit by squad number */
} NGSCup;                               /* 810 bytes on the 68000; a save format: change it with a new version */

/* A nation's strength, any scale up to 63, for the qualifying draw. */
typedef uint8_t (*NGSPowerFn)(uint8_t nation);
/* A result for a fixture the human side does not play: goals for each side,
 * drawn with the cup's own generator through *seed. */
typedef void (*NGSResultFn)(uint8_t home, uint8_t away, uint32_t *seed, uint8_t *home_goals, uint8_t *away_goals);

/* Starts a championship: the human side's nation always plays; the other 31
 * of `nations` (32..64) qualify by strength * 4 plus up to 27 of luck, so the
 * strong nearly always get there. Then the draw and the 48 group fixtures.
 * A seed of 0 is taken as 1. */
int8_t NEOGEO_USER ngs_cup_init(NGSCup *c, uint8_t human, uint32_t seed, uint8_t nations, NGSPowerFn power);
/* The next fixture the human side plays, playing every computer fixture before it
 * with `result`; their goals go mostly to the forwards (squad numbers 9 and
 * 10, then 8, 7, 6, 5 and 3). -1 once the final is played. */
int16_t NEOGEO_USER ngs_cup_next(NGSCup *c, NGSResultFn result);
/* Records a result. `winner` is a nation, or NGS_NOBODY for a drawn group
 * match; a knock-out draw needs the side that went through (extra time or
 * penalties). Returns 1, or 0 for an unknown, played or inconsistent result. */
uint8_t NEOGEO_USER ngs_cup_result(NGSCup *c, uint8_t index, uint8_t home_goals, uint8_t away_goals, uint8_t winner);
/* A group's four in order: points, goal difference, goals scored, then the
 * order of the draw (deterministic). */
void NEOGEO_USER ngs_cup_ranking(const NGSCup *c, uint8_t group, uint8_t out[4]);
/* A nation's slot in the draw: its line in table[]. */
uint8_t NEOGEO_USER ngs_cup_slot(const NGSCup *c, uint8_t nation);
/* Which round a fixture index belongs to (NGS_ROUND_*). */
uint8_t NEOGEO_USER ngs_cup_round(uint8_t index);
/* A goal to a man: his line, else a free line, else the line of the least of
 * the others when he would now equal it. */
void NEOGEO_USER ngs_cup_scorer(NGSCup *c, uint8_t team, uint8_t squad);
/* The leading scorers, at most n, most goals first: returns how many. */
uint8_t NEOGEO_USER ngs_cup_top(const NGSCup *c, NGSScorer *out, uint8_t n);
/* After a match of the human side: cards[squad] is 0, 1 (booked) or 2 (sent
 * off). A man who sat this match out has served his ban; two bookings or a
 * red and he sits out the next. */
void NEOGEO_USER ngs_cup_discipline(NGSCup *c, const uint8_t cards[NGS_CUP_SQUAD]);
/* The cup's own generator (xorshift32): for the game's results through NGSResultFn. */
uint32_t NEOGEO_USER ngs_cup_roll(NGSCup *c);

#ifdef __cplusplus
}
#endif
#endif
