/*
 * ng_trig.c — Sine, cosine and atan2 in fixed point (see ng_trig.h)
 */

#include "ng_trig.h"

int16_t NEOGEO_USER ng_sin(uint8_t a)
{
    return ng_trig_sin_tab[a];
}

int16_t NEOGEO_USER ng_cos(uint8_t a)
{
    return ng_trig_sin_tab[(uint8_t)(a + 64u)];
}

int16_t NEOGEO_USER ng_trig_mul(int16_t v, int16_t t)
{
#if defined(__m68k__)
    int32_t r = v;
    __asm__ ("muls.w %1,%0" : "+d" (r) : "dm" (t));
    return (int16_t)(r >> NG_TRIG_SHIFT);
#else
    return (int16_t)(((int32_t)v * t) >> NG_TRIG_SHIFT);
#endif
}

/*
 * Fold the vector into the first octant (0 <= y <= x), remembering how,
 * scale it up so the shifts below keep their precision, then turn it onto
 * the x axis by +-atan(2^-i) for i = 0..7 (CORDIC), adding up the turns.
 * The folding is undone on the angle at the end. Worked in 32 bits: the
 * rotations grow the vector by about 1.65.
 */
uint8_t NEOGEO_USER ng_atan2(int16_t dy, int16_t dx)
{
    int32_t x = dx, y = dy, t;
    int16_t z = 0, a;
    uint8_t base = 0, mirror = 0, swap = 0, i;

    if (x == 0 && y == 0) return 0;
    if (x < 0) { x = -x; y = -y; base = 128u; }
    if (y < 0) { y = -y; mirror = 1u; }
    if (y > x) { t = x; x = y; y = t; swap = 1u; }
    while (x < 0x100000L) { x <<= 1; y <<= 1; }

    for (i = 0; i < 8u; i++) {
        if (y > 0) {
            t = x + (y >> i);
            y -= x >> i;
            z = (int16_t)(z + ng_trig_atan_tab[i]);
        } else {
            t = x - (y >> i);
            y += x >> i;
            z = (int16_t)(z - ng_trig_atan_tab[i]);
        }
        x = t;
    }

    a = (int16_t)((z + 128) >> 8);     /* 1/256ths of a step to steps, rounded */
    if (swap) a = (int16_t)(64 - a);
    if (mirror) a = (int16_t)-a;
    return (uint8_t)(a + base);
}
