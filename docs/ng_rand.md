# Random Numbers

## What It Does

`ng_rand` (`sdk/2d_engine/ng_rand.h`) is the engine's one random number
generator: a 32-bit xorshift (shifts 13, 17, 5) with a single global state.
The game and the engine draw from the same sequence — `ng_depthfx`'s
starfield included — so one seed replays every random choice in the same
order.

```c
void     ng_rand_seed(uint32_t seed);  /* start the sequence              */
uint16_t ng_rand(void);                /* next value, 0..65535            */
uint16_t ng_rand_range(uint16_t n);    /* next value scaled to 0..n-1     */
```

## Seeding

- The same seed always gives the same sequence, on every build.
- Seed 0 is remapped to a fixed non-zero seed: xorshift never leaves the
  all-zero state.
- A state never seeded, or cleared with the rest of work RAM (work RAM is
  not loaded with initial values), starts as seed 0.
- Seed from something that varies (a frame count at Start) for play that
  differs each time; seed from a constant for attract demos and tests.

## Ranges Without Division

`ng_rand_range(n)` is `(ng_rand() * n) >> 16`: one `MULU` (16 × 16 → 32)
and a `SWAP`, never `DIVU` and never the 32-bit multiply routine. It is
always below `n`; `n = 0` gives 0. For a power of two, a mask
(`ng_rand() & 31`) is cheaper still. Keep `ng_rand_range` out of inner
loops.

`ng_rand()` itself is shifts and XORs only; `ng_rand()` returns the upper
half of the state, the better-mixed bits.

## Cost

About 70 bytes of code and 4 bytes of RAM. The module is built for size
(`-Os`) whatever the engine's optimisation level, so every C-engine game
carries it for no more ROM than the starfield's former private generator.
C engine only: a `USE_2D_PLUS=1` game does not link it.

## Tests

`tests/rand_test.c` (`make unit-tests`): a fixed seed gives a golden list
of the first 16 values, seed 0 is remapped, and `ng_rand_range(n)` stays
below `n` for n in {1, 2, 3, 7, 100, 65535}.
