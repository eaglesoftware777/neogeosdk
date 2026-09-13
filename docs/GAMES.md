# The Shipped Games

**Eagle Software · Neo Geo SDK v1.7.0**

Six complete games live in `games/`. They are not samples in the "toy
fragment" sense — every one of them builds to a real ROM that runs in MAME
and on hardware, and between them they exercise every public subsystem in the
SDK. They are the reference implementations: when a document says "this is
how you do X", one of these games is doing X.

| Game | Id | Engine | What it is |
|---|---|---|---|
| `demo` | 777 | C | The 25-chapter engine reel — every subsystem, in order |
| `demo_plus` | 778 | C++ | The same engine through the C++ API, as a smoke test |
| `skylance` | 779 | C | Sky Lance — a complete vertical shooter |
| `helloworld` | 772 | — | FIX text and one sample. The tutorial target. |
| `tutorial` | 555 | C | The minimal engine loop, nothing else |
| `neogeogame` | 775 | C | An empty starting point to copy |

Every game carries its own `game.cfg`, so all six build the same way:

```sh
make GAME=<name> GAME_CFG_FILE=games/<name>/game.cfg all
make GAME=<name> GAME_CFG_FILE=games/<name>/game.cfg test
```

The repo-root `game.cfg` names whichever game you are actively working on;
passing `GAME_CFG_FILE=` lets you build any other without editing it.

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

Three sub-scenes — TITLE, PARTICLES, MARQUEE — driving the render queue,
palette effects, and the particle pool through the C++ API. It shares the
demo's artbox rather than duplicating the data, via the `GAME_EXTRA_INCLUDES`
hook in its `game.mk`:

```make
GAME_EXTRA_INCLUDES = -Igames/demo -Igames/demo/artbox
```

Its purpose is to prove the two engine builds stay ABI-compatible. If a
change to the C engine is not mirrored in the C++ one, this is the ROM that
fails to build.

## skylance — Sky Lance (id 779)

A complete vertical arcade shooter: three pilots, seven stages, one named
boss per stage, attract reel, pilot select, scoring, lives, energy, and a
game-over/continue flow.

### Pilots

| Call sign | Name | Fire rate | Speed | Bolt |
|---|---|---|---|---|
| BLUE LANCE | ROOK | 8 | 3 | single — the baseline |
| GREEN WING | KIRA | 11 | 3 | twin — wider spread, slower rate |
| RED STRIKER | BLAZE | 6 | 4 | single — fast and rapid, has to get close |

The three are differentiated the way the arcade originals do it: by feel, not
by damage numbers.

### Stages

| # | Backdrop | Boss |
|---|---|---|
| 1 | Mountain | CRIMSON KEEP |
| 2 | Coast | IRON TIDE |
| 3 | Mountain | SOL CORE |
| 4 | Coast | NIGHT RAZOR |
| 5 | Mountain | ROTOR NEST |
| 6 | Coast | EARTH HAMMER |
| 7 | Mountain | SPIRE GOD |

Each stage runs 5–7 squadrons of enemies from its own roster, then the boss.

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

**The backdrop had to be authored to tile.** A page is 16 × 16 tiles and the
sprite chip can only shrink, never stretch, so a vertically scrolling
backdrop has to be exactly one page tall and seamless at the join. Both
backdrops were resized to 256 × 256 and cross-faded across the wrap.

Sky Lance also ships without its own sample set, so its audio path is
`vrom m1rom` rather than the full `sound` target — see
`games/skylance/sound/README.md`.

A slice of Sky Lance also appears as chapter 24 of the demo reel, so you can
see the same code driven two ways.

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

An empty game — the BIOS hooks, the linker script, an artbox directory, and
nothing else. Rename it, set `GAME_ID` and `GAME_NAME` in its `game.mk`,
and start writing. See [`ADDING_A_GAME.md`](./ADDING_A_GAME.md).

---

## Choosing a starting point

| You want to… | Copy |
|---|---|
| Talk to the hardware directly | `helloworld` |
| Get an engine tick running immediately | `tutorial` |
| Start from a blank slate with the layout already right | `neogeogame` |
| See how a finished game is structured | `skylance` |
| Look up how one subsystem is actually driven | `demo` |
| Write the game in C++ | `demo_plus` |
