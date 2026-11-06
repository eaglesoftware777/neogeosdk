/*
 * ng_rand host tests: the sequence a seed gives is fixed (a game replays
 * the same random choices from the same seed on every build), seed 0 is
 * remapped to a working seed, and ng_rand_range(n) stays below n.
 *
 * The golden values come from xorshift32 (13, 17, 5), upper 16 bits,
 * worked out independently of this code. On the 68000 the upper half and
 * the range multiply are single instructions (SWAP, MULU); the host
 * builds the same arithmetic in C.
 */
#include <stdio.h>
#include "../sdk/2d_engine/ng_rand.h"

static int failures;

static void check(int ok, const char *what, unsigned a, unsigned b)
{
    if (!ok) {
        printf("FAIL %s: %u (0x%04X) vs %u\n", what, a, a, b);
        failures++;
    }
}

int main(void)
{
    static const uint16_t golden[16] = {
        0x8798u, 0x155Bu, 0x4820u, 0x81B3u, 0x703Au, 0x29A8u, 0x89CAu, 0xC518u,
        0xD378u, 0x3AB1u, 0x69BFu, 0x4E0Fu, 0xFCBEu, 0x3565u, 0x8CC6u, 0xF6DAu
    };
    static const uint16_t seed0[4] = { 0xE124u, 0x8B9Au, 0x64E1u, 0x0017u };
    static const uint16_t ranges[] = { 1u, 2u, 3u, 7u, 100u, 65535u };
    unsigned i, k;

    ng_rand_seed(0x12345678u);
    for (i = 0; i < 16; i++) check(ng_rand() == golden[i], "golden", golden[i], i);

    /* the same seed again: the same sequence */
    ng_rand_seed(0x12345678u);
    for (i = 0; i < 16; i++) check(ng_rand() == golden[i], "replay", golden[i], i);

    /* seed 0 is remapped, never the all-zero state xorshift stays in */
    ng_rand_seed(0u);
    for (i = 0; i < 4; i++) check(ng_rand() == seed0[i], "seed 0", seed0[i], i);

    for (k = 0; k < sizeof ranges / sizeof ranges[0]; k++) {
        uint16_t n = ranges[k], max = 0;
        ng_rand_seed(0xC0FFEEu + k);
        for (i = 0; i < 200000u; i++) {
            uint16_t v = ng_rand_range(n);
            check(v < n, "range", v, n);
            if (v > max) max = v;
            if (failures > 8) break;
        }
        /* the draws reach the top of the range, not just its start */
        check(max == (uint16_t)(n - 1u) || n > 1000u, "range top", max, n);
    }
    check(ng_rand_range(0u) == 0u, "range 0", 0, 0);

    if (failures) {
        printf("ng_rand: %d failure(s)\n", failures);
        return 1;
    }
    printf("ng_rand: all tests passed\n");
    return 0;
}
