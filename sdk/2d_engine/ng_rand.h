/*
 * ng_rand.h — The engine's random numbers: one deterministic generator
 *
 * A single 32-bit xorshift state (shifts 13, 17, 5) serves the whole game
 * and the engine alike (ng_depthfx draws from it too), so one
 * ng_rand_seed() replays every random choice in the same order.
 *
 *   ng_rand_seed(s)    start the sequence; seed 0 is taken as a fixed
 *                      non-zero seed (xorshift never leaves zero)
 *   ng_rand()          next value, 0..65535 (the state's upper half)
 *   ng_rand_range(n)   next value scaled to 0..n-1 by one 16x16 multiply,
 *                      (ng_rand() * n) >> 16: no divide. n = 0 gives 0.
 *
 * Never seeded -- or with its RAM cleared since -- it starts as seed 0.
 * No divide anywhere; ng_rand_range costs one MULU, so keep it out of
 * inner loops. C engine only.
 */

#ifndef NG_RAND_H
#define NG_RAND_H

#include <stdint.h>
#include "ng_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

void     NEOGEO_USER ng_rand_seed(uint32_t seed);
uint16_t NEOGEO_USER ng_rand(void);
uint16_t NEOGEO_USER ng_rand_range(uint16_t n);

#ifdef __cplusplus
}
#endif
#endif
