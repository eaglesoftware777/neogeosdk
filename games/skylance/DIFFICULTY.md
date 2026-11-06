# Sky Lance difficulty model

Players reported finishing every sector on the first credit. The difficulty
now rises by sector through a table of measurable values, `k_difficulty` in
`scenes/sky_stage.c`, instead of more hit points:

| Sector | Fire cycle | Pattern tier | Live enemy shots | Aimed speed-3 shot |
|---|---|---|---|---|
| 1 | 100% | 0 | 8 | 3.00 px/update |
| 2 | 92% | 0 | 8 | 3.12 |
| 3 | 85% | 1 | 10 | 3.25 |
| 4 | 78% | 1 | 10 | 3.38 |
| 5 | 72% | 2 | 12 | 3.50 |
| 6 | 66% | 2 | 12 | 3.62 |
| 7 | 60% | 3 | 12 | 3.75 |

Sector 1 keeps the released rates, so the opening stays accessible.

- **Fire cycle**: every roster unit fires on a fixed cycle (drone 60
  updates, fighter 72, interceptor 80, heli 96, missile boat 100, tank 112,
  bomber 100). The sector scales the cycle; each unit's firing phase stays
  inside even the shortest cycle.
- **Pattern tier** adds shots that keep a lane open:
  - tier 1: fighters fire a second aimed tracer 10 updates after the first,
    re-aimed, so a sidestep after the first one still works;
  - tier 2: drones aim their dart; interceptors add a wider outer pair, the
    centre gap stays; helis follow the rocket pair with one aimed rocket;
  - tier 3: missile boats add a slow aimed shell; bombers open the sides
    with two angled bombs around the ring.
- **Live enemy shots**: the shot pool grew from 8 to 12 pinned sprites; the
  sector sets how many may fly at once.
- **Aimed speed**: the speed-3 aimed shot in 8.8 fixed point; speed-2 and
  speed-4 callers scale with it, never above 4.25 px/update.

Bosses keep their own escalation (fire interval 44 - 3 x sector updates,
two thirds of that once below half health).

## Expected shots per minute, per unit

At about 29 game updates a second (the QA probe measured logic on 49% of
video frames):

| Sector | Drone | Fighter | Interceptor | Heli | Missile boat | Tank | Bomber |
|---|---|---|---|---|---|---|---|
| 1 | 29 | 24 | 44 | 37 | 18 | 16 | 18 |
| 2 | 32 | 27 | 48 | 40 | 19 | 17 | 19 |
| 3 | 35 | 58 | 52 | 44 | 21 | 19 | 21 |
| 4 | 38 | 63 | 57 | 48 | 23 | 20 | 23 |
| 5 | 41 | 69 | 124 | 77 | 24 | 22 | 24 |
| 6 | 45 | 75 | 136 | 84 | 27 | 24 | 27 |
| 7 | 49 | 82 | 147 | 93 | 59 | 26 | 88 |

The live cap bounds what is actually on screen. A capture can check the
table by counting `SKY_KIND_ESHOT` spawns per unit type.

## Fairness rules kept

- No aimed shot is faster than 4.25 px/update.
- Interceptor fans keep their centre gap; bomber side bombs leave the lane
  under the ring.
- Fire is held while the busiest scanline is within two sprites of the
  LSPC's 96 (`SKY_FIRE_LINE_LIMIT`): on real hardware a shot on an
  overfull line would not be drawn, and an invisible bullet is never fair.
- A shot never starts or stays still: aimed velocity is computed in 8.8
  fixed point, and a non-seeker shot that stops, or lives longer than any
  crossing of the field (600 updates), is removed and counted in
  `s_stale_bolts` for the QA probe.

## Related fixes in the same pass

- Enemy shots no longer hang on wings: whole-pixel aiming truncated a shot
  fired at a player within 16 pixels to velocity (0,0), and it hung at the
  muzzle (the Crimson Keep's wing guns at +-28, a hovering heli) while it
  took a place in the 8-shot pool, which also quietly lowered the fire
  rate. Automated test: `tools/shot_probe.py --point-blank` holds the plane
  under each enemy for 75 s and fails when a shot keeps one position for 30
  frames. v2 release: 1 hung shot, standing still for 2999 frames; v2.1.0
  candidate: none (`docs/qa/skylance-probe/`).
- Flicker: the fixed sprite slots put enemy shots last (216..223), and the
  LSPC drops the highest sprite numbers first on a line with more than 96.
  The new order (`scenes/sky_draw.h`) makes the pilot portrait and the
  player's shots the first to go; enemy shots still draw over every
  aircraft.
- Clouds: the overhead cloud pans with the field (1.5 times the ground, the
  depth module's foreground weight) instead of keeping a screen x, and the
  low cloud bank scrolls and pans slightly faster than the ground instead of
  drifting along with the plane at a quarter of its speed.
