# Fixed-Point Trigonometry

## What It Does

`ng_trig` (`sdk/2d_engine/ng_trig.h`) gives sine, cosine and atan2 with no
floating point and no division:

```c
int16_t ng_sin(uint8_t a);                 /* 1.14: 16384 = 1.0          */
int16_t ng_cos(uint8_t a);                 /* = ng_sin(a + 64)           */
uint8_t ng_atan2(int16_t dy, int16_t dx);  /* the angle (dx, dy) points at */
int16_t ng_trig_mul(int16_t v, int16_t t); /* v * t >> 14, one MULS       */
```

Angles are one byte, 256 steps to the turn, so they wrap for free: 64 is a
quarter turn. Angle 0 points along +x and 64 along +y, which on screen
(y grows downwards) is straight down.

```c
/* a point on a circle of radius r around (cx, cy) */
x = cx + ng_trig_mul(r, ng_cos(a));
y = cy + ng_trig_mul(r, ng_sin(a));

/* aim at the player */
a = ng_atan2(player_y - y, player_x - x);
```

## Why 1.14

1.0 has to be exact (16384) and still fit an `int16_t`, which 1.15 cannot
do. 8.8 would resolve only 1/256; 1.14 resolves 1/16384. A 1.14 value times
a 16-bit length is one `MULS` and a shift by 14 (`ng_trig_mul`).

## How

- Sine is one 256-entry table (512 bytes of ROM); cosine reads it 64
  steps on. Every value is within half an LSB of the true sine.
- `ng_atan2` folds the vector into the first octant, scales it up so the
  shifts keep their precision, and turns it onto the x axis in eight
  rounds of CORDIC: sign tests, shifts and additions only. It is within
  one step of the true angle; `ng_atan2(0, 0)` is 0.

## The Table Is Generated

`sdk/2d_engine/ng_trig_table.c` is written by `tools/gen_trig.py`: never
edit it by hand. Every game build regenerates it; the values are worked
out in 50-digit decimal arithmetic, not with the host's floating point,
and written with LF line ends, so the bytes are identical on Linux and
Windows (`make unit-tests` checks the committed file against a fresh one).

## Cost

About 460 bytes of code and 528 bytes of tables, linked only into a game
that calls it (it is in the on-demand library, `out/libng_sdk.a`).
`ng_fixed.h` keeps its own older sine table (`NGFX_SIN`, 16.16 steps).

## Tests

`tests/trig_test.c` (`make unit-tests`): sine and cosine within 1 LSB of
the host's maths library at all 256 angles, atan2 within one step over a
grid of vectors (every sign, both axes, lengths from 1 to 32768) and
random ones.
