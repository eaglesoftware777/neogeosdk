/*
 * ng_trig.h — Sine, cosine and atan2 in fixed point, no division
 *
 * Angles are one byte: 256 steps to the full turn, so they wrap for free
 * (64 = a quarter turn, 128 = half). Angle 0 points along +x and 64 along
 * +y -- on screen, where y grows downwards, that is straight down.
 *
 * Results are 1.14 fixed point: 16384 (NG_TRIG_ONE) is 1.0. 1.14 rather
 * than 1.15 because 1.0 itself has to fit in an int16_t; rather than 8.8
 * because a quarter of a degree is still resolved at 1/16384, and a 1.14
 * value times a 16-bit length is one MULS and a shift by 14:
 *
 *   dx = ng_trig_mul(radius, ng_cos(a));
 *   dy = ng_trig_mul(radius, ng_sin(a));
 *
 * ng_atan2(dy, dx) is the inverse: the angle (dx, dy) points at, to
 * within one step, from sign tests, shifts and additions only (octant
 * folding, then eight rounds of CORDIC). atan2(0, 0) is 0.
 *
 * The tables live in ng_trig_table.c, which tools/gen_trig.py writes:
 * never edit it by hand. The module is linked only into a game that
 * calls it. (ng_fixed.h has its own older sine table, in 16.16 steps.)
 */

#ifndef NG_TRIG_H
#define NG_TRIG_H

#include <stdint.h>
#include "macro.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NG_TRIG_ONE    16384   /* 1.0 in 1.14 */
#define NG_TRIG_SHIFT  14

extern const int16_t  ng_trig_sin_tab[256];
extern const uint16_t ng_trig_atan_tab[8];

int16_t NEOGEO_USER ng_sin(uint8_t a);
int16_t NEOGEO_USER ng_cos(uint8_t a);
uint8_t NEOGEO_USER ng_atan2(int16_t dy, int16_t dx);

/* v * t >> 14 for a 1.14 value t: one MULS. */
int16_t NEOGEO_USER ng_trig_mul(int16_t v, int16_t t);

#ifdef __cplusplus
}
#endif
#endif
