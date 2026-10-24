/*
 * ng_rand.c — The engine's random numbers (see ng_rand.h)
 *
 * Built for size whatever the engine's own optimisation level: every game
 * links this module, and at this size it costs no more ROM than the
 * starfield's private xorshift it replaced.
 */
#pragma GCC optimize ("Os")

#include "ng_rand.h"

/* What seed 0 (or a state never seeded) stands for. */
#define NG_RAND_SEED0 0x2545F491u

/* In work RAM, which is not loaded with initial values: it starts as
 * whatever the game's RAM clear left, and zero is taken as seed 0. */
static uint32_t ng_rand_state;

void NEOGEO_USER ng_rand_seed(uint32_t seed)
{
    ng_rand_state = seed;   /* 0 is remapped when it is drawn from */
}

uint16_t NEOGEO_USER ng_rand(void)
{
    uint32_t x = ng_rand_state, t;
    if (!x) x = NG_RAND_SEED0;
    t = x << 8;
    __asm__ ("" : "+d" (t));   /* two short shifts, not a count in a spare register */
    x ^= t << 5;
    x ^= x >> 17;
    x ^= x << 5;
    ng_rand_state = x;
#if defined(__m68k__)
    __asm__ ("swap %0" : "+d" (x));   /* the upper half, in the low word */
    return (uint16_t)x;
#else
    return (uint16_t)(x >> 16);
#endif
}

uint16_t NEOGEO_USER ng_rand_range(uint16_t n)
{
    uint16_t v = ng_rand();
#if defined(__m68k__)
    uint32_t r;
    /* one MULU (16 x 16 -> 32), then its upper half: (v * n) >> 16 */
    __asm__ ("mulu.w %2,%0\n\tswap %0" : "=d" (r) : "0" (v), "dm" (n));
    return (uint16_t)r;
#else
    return (uint16_t)(((uint32_t)v * n) >> 16);
#endif
}
