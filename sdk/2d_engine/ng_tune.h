/*
 * ng_tune.h — Ratings into game numbers, by difficulty level and by situation
 *
 * A sports or fighting game describes its sides with ratings (speed, passing,
 * defending, keeping...) and needs numbers to play with: frames between CPU
 * decisions, a tackle chance, an aiming error, a reaction delay. ng_tune keeps
 * the two apart. Ratings are 0..31. Each number is a CURVE: for every difficulty
 * level, the value at rating 0 and the value at rating 31, and a shape between
 * them. So level and team strength combine in one place, as data, and a level
 * is tuned by editing two bytes rather than code:
 *
 *   static const NGTuneSpan k_think_span[8] = {
 *       {18, 13}, {15, 11}, {13, 9}, {11, 7}, {9, 6}, {7, 4}, {5, 3}, {4, 2} };
 *   static const NGTuneCurve k_think = { k_think_span, 8, NG_TUNE_STRAIGHT };
 *   frames = ng_tune_value(&k_think, level, team_rating);
 *
 * A value may fall as the rating rises (a delay, an error): give at0 > at31.
 *
 * A NUDGE shifts ratings while a match is played: by the score gap (a side two
 * goals down plays a touch sharper, one cruising three up a touch lazier), by
 * whether the end is near (a one-goal game in the last minutes is tighter), and
 * by how far a cup run has gone (each round a little harder). The nudge is added
 * to the rating before the curve, so it moves along the same tuned curve:
 *
 *   int8_t n = ng_tune_nudge(&k_nudge, my_goals - their_goals, late, round);
 *   frames = ng_tune_value(&k_think, level, team_rating + n);
 *
 * Evaluate at a restart or a few times a second, not per man per frame: a value
 * costs one MULS. No division anywhere. The module is plain C with no hardware
 * access, so a game's host-side tests can compile it too (define NG_TUNE_CODE
 * empty), and it is linked only into a game that calls it.
 */

#ifndef NG_TUNE_H
#define NG_TUNE_H

#include <stdint.h>

#ifndef NG_TUNE_CODE
#define NG_TUNE_CODE __attribute__((section("neogeo_user")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NG_TUNE_RATING_MAX 31

/* How a curve runs from at0 to at31. */
enum {
    NG_TUNE_STRAIGHT = 0,   /* even steps */
    NG_TUNE_EASE_IN  = 1,   /* little change among weak ratings, most among the best */
    NG_TUNE_EASE_OUT = 2,   /* most change among weak ratings: the top of the scale flattens */
    NG_TUNE_STEP     = 3    /* below 16 gives at0, 16 and up gives at31 */
};

typedef struct {
    uint8_t at0;            /* the value for a rating of 0 */
    uint8_t at31;           /* the value for a rating of 31 */
} NGTuneSpan;

typedef struct {
    const NGTuneSpan *span; /* one span per level */
    uint8_t levels;         /* how many spans; a higher level reads the last one */
    uint8_t shape;          /* NG_TUNE_* */
} NGTuneCurve;

/* Score gaps from -3 (three or more down) to +3 (three or more up). */
#define NG_TUNE_GAPS 7

typedef struct {
    int8_t early[NG_TUNE_GAPS];   /* rating shift by score gap, own minus other, -3..+3 */
    int8_t late[NG_TUNE_GAPS];    /* the same once the end is near */
    int8_t per_round;             /* added for every round of a cup already played */
    int8_t round_cap;             /* the round part never goes past this */
} NGTuneNudge;

/* The curve's value for a rating at a level. The rating is clamped to 0..31
 * first, so a nudged rating can be passed as it is. */
uint8_t NG_TUNE_CODE ng_tune_value(const NGTuneCurve *c, uint8_t level, int16_t rating);

/* A rating clamped to 0..31. */
uint8_t NG_TUNE_CODE ng_tune_clamp(int16_t rating);

/* A 1..16 attribute (the scale many games store) as a 0..31 rating. */
uint8_t NG_TUNE_CODE ng_tune_from16(uint8_t attribute);

/* The shift for a situation. gap: own score minus the other's; late: nonzero once
 * the end is near; round: rounds of a cup already played (0 in a friendly). */
int8_t NG_TUNE_CODE ng_tune_nudge(const NGTuneNudge *n, int16_t gap, uint8_t late, uint8_t round);

/* A level that climbs through a cup: base, plus one for every `every` rounds
 * played, never past levels - 1. every = 0 keeps the base. */
uint8_t NG_TUNE_CODE ng_tune_level(uint8_t base, uint8_t round, uint8_t every, uint8_t levels);

/* Several numbers at once: out[i] = curves[i] at level for ratings[i] + nudge. */
void NG_TUNE_CODE ng_tune_fill(uint8_t *out, const NGTuneCurve *const *curves, const uint8_t *ratings,
                               uint8_t n, uint8_t level, int8_t nudge);

#ifdef __cplusplus
}
#endif

#endif
