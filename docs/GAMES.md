# The Shipped Games

**Eagle Software · Neo Geo SDK v1.7.0 (Maiya arrives in v1.7.1)**

Seven projects are listed here: the showcase, its C++ counterpart,
two shooters, a platform adventure and two minimal examples. Use their source as reference
implementations for the SDK features they demonstrate.

| Game | Id | Engine | What it is |
|---|---|---|---|
| `demo` | 777 | C | The 25-chapter engine reel |
| `demo_plus` | 778 | C++ | The same demo scenes, art, and sound on the C++ engine |
| `skylance` | 779 | C | Sky Lance — a complete vertical shooter |
| `maiya` | 780 | C | Maiya: Super Nature Girl, six valleys, vine climbing, guardian arenas and bonus playfields |
| `helloworld` | 772 | — | FIX text and one sample. The tutorial target. |
| `tutorial` | 555 | C | The minimal engine loop, nothing else |
| `neogeogame` | 775 | C | A sprite-based formation shooter |

Every game carries its own `game.cfg`; explicit game selection automatically
uses that file without changing the default `demo` configuration:

```sh
make GAME=<name> all
make GAME=<name> test
```

On Windows use `make -f MakefileWin32.mak` with the same arguments.
An explicit `GAME_CFG_FILE=` remains available for custom configurations.
EagleBIOS (`USE_EAGLE_BIOS=1`, the default) is built and installed into its
isolated test directory, and every test runs on it. `bios-package` builds a cartridge-plus-firmware ZIP; the
normal default never bundles replacement firmware.

## Maiya (id 780)

See [the game manual](../games/maiya/README.md) for controls, the fixed-camera
guardian arenas, bonus progression, source art, palette allocation and MAME
regression tests. Its per-game art builder preserves animation frame layout
instead of treating each pose as a generated screen.

---

## demo — the engine reel (id 777)

A single linear flow of **25 chapters**, each demonstrating one subsystem,
with the chapter number printed in the top-right corner of every scene so a
problem can be reported by number. **A** advances, **C** restarts the current
chapter.

Full chapter-by-chapter breakdown: [`DEMO_CHAPTERS.md`](./DEMO_CHAPTERS.md).

The demo is the largest consumer of the SDK and therefore the best place to
read working usage of: the FIX layer and INFIX photo pages, sprite screens,
character movesets, gravity and solids, camera follow / dead-zone / shake,
palette effects, the particle pool under stress, hitstop and shake feedback,
depth projection, two-layer parallax, NPC patrol AI, live joystick handling
with two-button specials, a scrolling world, sprite-scaling pseudo-3D, and a
playable shooter.

`games/demo/scenes/demo_unified.c` is the reel itself; the other files under
`scenes/` are older standalone scenes that the reel draws on.

## demo_plus — the C++ engine (id 778)

The same engine, linked from `sdk/2d_engine_plus/` instead of
`sdk/2d_engine/`:

```sh
make GAME=demo_plus GAME_CFG_FILE=games/demo_plus/game.cfg USE_2D_PLUS=1 all
```

The default build uses the same 25-chapter reel as `demo`. Its `game.mk`
shares scene sources, graphics inputs, and sound inputs while retaining
separate 778 ROM outputs:

```make
GAME_SCENES_FROM = demo
GAME_ART_FROM = demo
GAME_SOUND_FROM = demo
```

The old three-scene smoke test remains as an alternative source file, but
is not part of the default build. Run both demo builds when checking engine
parity; successful compilation alone does not prove identical behavior.

## skylance — Sky Lance (id 779)

A complete vertical arcade shooter: three pilots (Rook's F-16, Kira's F-35,
Blaze's F-15), seven long painted routes, three guarded command targets per
route, a mini-boss at each and one named boss per route, fought in an open
arena on the route's last page. See [the game's README](../games/skylance/README.md)
for controls, the build and packages, and its
[changelog](../games/skylance/CHANGELOG.md).

| # | Subtitle | Terrain | Boss | Arena |
|---|---|---|---|---|
| 1 | MOUNTAIN DAWN | Mountains | CRIMSON KEEP | alpine plateau |
| 2 | HARBOR ASSAULT | Coast | IRON TIDE | deep sea |
| 3 | VALLEY CONVOY | Valley | SOL CORE | open valley floor |
| 4 | REEF INTERCEPT | Open sea | NIGHT RAZOR | open ocean |
| 5 | FOREST OUTPOST | Forest | ROTOR NEST | forest clearing |
| 6 | ARMORED COLUMN | Desert | EARTH HAMMER | empty desert |
| 7 | FINAL APPROACH | Citadel | CRIMSON CITADEL | lava plain |

### Layout

| File | What it holds |
|---|---|
| `scenes/sky_draw.c` | Asset binding, the streamed routes, cockpit instruments, the FIX HUD, the frame pump |
| `scenes/sky_stage.c` | Enemy roster, stage table, wave director, encounters, chaff, boss behaviour |
| `scenes/sky_sortie.c` | Attract reel, pilot select, the flight loop, collisions, scoring |
| `user.c` | BIOS handshake, FIX palettes, MVS title |
| `tools/` | Painted art pipeline (`paint_terrain.py`, `paint_sprites.py`, `paint_jets.py`), the art build and checks |

The playfield is 288 pixels wide; the 32-pixel column to its right holds
the pilot's portrait and the cockpit instruments. Routes are 352 pixels wide
(the camera follows the jet sideways) and stream one 256-row page at a time.
Sky Lance builds with the opt-in shooter module (`-DNG_SHOOTER=1`).

A two-stage slice of Sky Lance also appears as chapter 24 of the demo reel,
built from the same art, so you can see the same game driven two ways.

## helloworld (id 772)

The smallest useful ROM: `ng_fix_init()`, some text, `soundInit()`, and an
edge-triggered `playSFX(0)` on button A. No engine, no scenes, no sprites.

This is the target of [`HELLO_WORLD.md`](./HELLO_WORLD.md), and the right
thing to copy when you want to start from bare metal.

## tutorial (id 555)

The same size as helloworld, but driving the engine rather than the hardware:

```c
void game_boot(void)  { ng_game_engine_init(); soundInit(); }
void game_frame(void) { waitVbl(); ng_game_engine_frame(); }
```

That is the whole program. It exists to show the shortest path to a running
engine tick, and as the skeleton to grow an engine-based game from.

## neogeogame (id 775)

A formation shooter with player and enemy sprites, a scrolling starfield,
bullets, collision handling, score, waves, and lives. The game loop is in
`games/neogeogame/main.c`. For a minimal new project, start from `helloworld`
or `tutorial` instead; see [`ADDING_A_GAME.md`](./ADDING_A_GAME.md).

---

## Choosing a starting point

| You want to… | Copy |
|---|---|
| Talk to the hardware directly | `helloworld` |
| Get an engine tick running immediately | `tutorial` |
| Start from a minimal project with the layout already right | `helloworld` or `tutorial` |
| See how a finished game is structured | `skylance` |
| Look up how one subsystem is actually driven | `demo` |
| Write the game in C++ | `demo_plus` |
