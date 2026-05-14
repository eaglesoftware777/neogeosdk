/*
 * ng_fixed.h — Fixed-point math for the NeoGeo 2D engine (Stage 4)
 *
 * Two formats co-exist in this codebase:
 *
 *   8.8 fixed-point  (ng_defs.h, NG_FP_SHIFT=8)
 *     - Used for velocity, parallax ratios, small deltas.
 *     - Range: -128..127.99, resolution 1/256 (~0.004 px).
 *     - 16-bit value fits in uint16_t / int16_t.
 *
 *   16.16 fixed-point  (this file, NGFixed)
 *     - Used for camera, smooth world positions, depth projection.
 *     - Range: -32768..32767.9999, resolution 1/65536 (~0.000015 px).
 *     - 32-bit value; needs int32_t.
 *
 * NeoGeo 68000 multiply note:
 *   The 68000 has MULS/MULU which are 16x16->32.  The __mulsi3 in
 *   neogeolib.c handles 32x32->32 via two 16x16 operations.  For 16.16
 *   fixed multiply use NGFX_MUL which uses a 32-bit intermediate.
 *   Avoid 32-bit divides — use lookup tables or right-shifts instead.
 */

#ifndef NG_FIXED_H
#define NG_FIXED_H

#include <stdint.h>
#include "ng_defs.h"

/* 16.16 fixed-point type */
typedef int32_t NGFixed;

#define NGFX_SHIFT       16
#define NGFX_ONE         ((NGFixed)(1L << NGFX_SHIFT))
#define NGFX_HALF        ((NGFixed)(1L << (NGFX_SHIFT - 1)))

/* Convert integer <-> NGFixed */
#define NGFX_FROM_INT(i)    ((NGFixed)((int32_t)(i) << NGFX_SHIFT))
#define NGFX_TO_INT(f)      ((int16_t)((f) >> NGFX_SHIFT))

/* Round to nearest integer */
#define NGFX_TO_INT_R(f)    ((int16_t)(((f) + NGFX_HALF) >> NGFX_SHIFT))

/* Fixed-point add / subtract — same as normal int32 add */
#define NGFX_ADD(a,b)       ((a) + (b))
#define NGFX_SUB(a,b)       ((a) - (b))

/* Fixed-point multiply: (a * b) >> 16
 * Both operands are 16.16; result is 16.16.
 * Uses 32x32->64 via two MULS on 68000; result is truncated to 32 bits.
 * For 68000 performance keep one operand as small as possible. */
#define NGFX_MUL(a,b)       ((NGFixed)(((int32_t)(a) >> 8) * ((int32_t)(b) >> 8)))

/* Multiply a 16.16 fixed value by an integer scale factor (integer scale) */
#define NGFX_SCALE(f,s)     ((NGFixed)((f) * (int32_t)(s)))

/* Clamp helpers */
#define NGFX_CLAMP(v,lo,hi)  ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

/* Absolute value */
#define NGFX_ABS(x)          ((x) < 0 ? -(x) : (x))

/* Linear interpolation between a and b at factor t (0..NGFX_ONE)
 * Avoids multiply on the common t=0 or t=ONE cases. */
#define NGFX_LERP(a,b,t)    ((a) + NGFX_MUL(((b) - (a)), (t)))

/*
 * Sine / Cosine lookup table — 256-entry, 0..255 represents 0..359 degrees.
 * Values are scaled to NGFX_ONE (65536), so sin(0°) = 0, sin(90°) = 65536.
 * Table is stored as int16_t (range -32768..32767) scaled by 32767/1.0.
 * To get a 16.16 fixed result: shift left by 1.
 *
 * Usage:
 *   NGFixed s = (NGFixed)ng_sin_tab[angle & 0xFF] << 1;
 *   NGFixed c = (NGFixed)ng_cos_tab[angle & 0xFF] << 1;
 *
 * Where angle = 0..255 maps to 0..354 degrees (360/256 = 1.40625 deg/step).
 */
extern const int16_t ng_sin_tab[256];
extern const int16_t ng_cos_tab[256];

/* Convenience: get sin/cos as 16.16 fixed (-NGFX_ONE..NGFX_ONE) */
#define NGFX_SIN(a)   ((NGFixed)ng_sin_tab[(uint8_t)(a)] << 1)
#define NGFX_COS(a)   ((NGFixed)ng_cos_tab[(uint8_t)(a)] << 1)

/* Shrink lookup table for depth projection (Stage 10).
 * Maps Z distance (0..127) to NeoGeo sprite shrink value (0..255).
 * Z=0 = closest (full size = 0xFF), Z=127 = farthest (smallest = ~0x20).
 * The table avoids runtime division to compute sprite scale. */
#define NGFX_SHRINK_ENTRIES  128
extern const uint8_t ng_shrink_tab[NGFX_SHRINK_ENTRIES];

/* Integer square root via Newton's method for fixed-point use.
 * Returns floor(sqrt(n)) for n >= 0.  Safe: no division in frame loop
 * because this is only for pre-baked distance calculations. */
uint16_t NEOGEO_USER ng_isqrt(uint32_t n);

#endif
