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
`USE_EAGLE_BIOS=1` builds and installs the optional firmware into its isolated
test directory. `bios-package` builds a cartridge-plus-firmware ZIP; the
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

A complete vertical arcade shooter: three pilots, seven stages over three
terrains, one named boss per stage with its own attack, gunboats and tanks
that fire from the ground, pick-ups and a super missile, attract reel,
pilot select, scoring, lives, energy, game-over handling, and the
credits with a victory flight after the seventh boss.

### Pilots

| Call sign | Name | Fire rate | Speed | Bolt |
|---|---|---|---|---|
| BLUE LANCE | ROOK | 8 | 3 | single — the baseline |
| GREEN WING | KIRA | 11 | 3 | twin — wider spread, slower rate |
| RED STRIKER | BLAZE | 6 | 4 | single — fast and rapid, has to get close |

The three are differentiated the way the arcade originals do it: by feel, not
by damage numbers.

### Stages

| # | Subtitle | Terrain | Roster | Boss |
|---|---|---|---|---|
| 1 | MOUNTAIN DAWN | Valley | drones, fighters, interceptors | CRIMSON KEEP |
| 2 | HARBOR ASSAULT | Harbour | gunboats, fighters, bombers | IRON TIDE |
| 3 | VALLEY CONVOY | Valley | tanks, interceptors, drones, bombers | SOL CORE |
| 4 | REEF INTERCEPT | Open sea | interceptors, gunboats, fighters, bombers | NIGHT RAZOR |
| 5 | FOREST OUTPOST | Valley | gunships, tanks, bombers | ROTOR NEST |
| 6 | ARMORED COLUMN | Valley | tanks, interceptors, bombers | EARTH HAMMER |
| 7 | FINAL APPROACH | Harbour | gunboats, interceptors, gunships, bombers | CRIMSON CITADEL |

Each stage runs 5–7 squadrons, every roster entry in turn, then the boss.
Gunboats and tanks are surface units: they ride the scroll along the sea
lanes or the valley road and fire from where they are, never chasing.
Gunships come down to a firing line and hold it; bombers dive.  Each boss
has its own volley - a fan with escape lanes, converging naval guns, a core
that alternates the safe side, a carrier fan that leaves the centre open -
and the player's ceiling keeps the plane under the boss station so a boss
is always fought from below.

Every fourth kill drops a pick-up: a speed step, two missiles for the rack,
or a spare plane (energy, once the rack of planes is full).  **D** fires a
super missile worth six hits.  The route line under the playfield names the
stage and the missile count; the impact ring cycles through its palette
bank every few frames.

After the seventh boss the credits name the pilot and the score, and the
plane flies a victory lap - a climb, a loop with the burst ring trailing,
a dive off the top - before the attract loop returns.

### Layout

| File | What it holds |
|---|---|
| `scenes/sky_draw.c` | Asset binding, the scrolling backdrop, the FIX HUD, the frame pump |
| `scenes/sky_stage.c` | Enemy roster, stage table, wave director, boss AI |
| `scenes/sky_sortie.c` | Attract reel, pilot select, the flight loop, collisions, scoring |
| `user.c` | BIOS handshake, FIX palettes, MVS title |
| `main.c` | Generated `showScreenN()` art entry points — rewritten by `make art` |

### Two hardware lessons worth reading the source for

**The playfield is 256 px wide, not 320.** The arena is the column the
backdrop covers, centred on the screen, with 32 px of backdrop either side.
That is a consequence of the backdrop being exactly one 16 × 16-tile page
wide, not a style choice.

**The backdrop does not have to be authored to tile.** A page is 16 × 16
tiles and the sprite chip can only shrink, never stretch, so a vertically
scrolling backdrop is exactly one page tall.  `scenes/sky_terrain.h` draws
each terrain as two copies of the page with the second flipped vertically,
so the join always meets the same source row from either side and any
256 × 256 picture scrolls seamlessly.  The three terrains - the valley,
the harbour and the open sea - are imported from
`artbox/in/backgrounds/` and `artbox/in/zzzz_terrain/` at 256 × 256 with
sixteen palette banks each.

Sky Lance also ships without its own sample set, so its audio path is
`vrom m1rom` rather than the full `sound` target — see
`games/skylance/sound/README.md`.

During a sortie the pilot chosen at the roster rides along in the margin
beside the playfield, in the back render band so a crowded frame can never
drop it.

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
