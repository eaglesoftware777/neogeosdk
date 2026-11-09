# The Shipped Games

**Eagle Software · Neo Geo SDK v1.7.8 (pre-release)**

Thirteen projects are listed here: eight games (a platformer, a shooter, a
fighting game, a beat'em-up, an adventure RPG, a puzzle game, a mahjong table
and, from the v1.7.8 pre-release, a football game), the showcase and its C++
counterpart, a formation shooter and two minimal examples. Use their source as reference implementations for
the SDK features they demonstrate.

| Game | Id | Engine | What it is |
|---|---|---|---|
| `demo` | 777 | C | The 25-chapter engine reel |
| `demo_plus` | 778 | C++ | The same demo scenes, art, and sound on the C++ engine |
| `skylance` | 779 | C | Sky Lance — a complete vertical shooter |
| `maiya` | 780 | C | Maiya: Super Nature Girl, twelve stages, vine climbing, guardian arenas, vaults and bonus playfields |
| `shadowleague` | 781 | C, `ng_fight` | Shadow League Fighting, 44 fighters, team battles |
| `sevenwonders` | 782 | C, `ng_puzzle` | Seven Wonders Puzzles: match, bubble and picture-logic boards |
| `maiyaluna` | 783 | C, `ng_rpg` | Maiya & Luna: two solo journeys, seven chapters |
| `nosignal` | 784 | C, `ng_beatemup` | No Signal Z Alpha: eight streets for one player |
| `mahjong_demo` | 785 | C, `ng_mahjong` | Mahjong Demo: four-seat Hong Kong Old Style and solitaire |
| `centuryxi` | 787 | C, `ng_soccer` | Century XI: Road to 2030, parody international football, a 64-match championship |
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

Building her release on a second machine (Python 3.10 or later):

```sh
ASEPRITE=builtin MAIYA_ART_REFERENCE=v1.7.3 make neo GAME=maiya
python3 tools/make_release.py --game maiya --game-id 780 --prefix Maiya \
    --name "Maiya: Super Nature Girl" --version <MAJOR.MINOR.PATCH> \
    --readme <readme> --changelog games/maiya/CHANGELOG.md
python3 tools/audit_release.py --prefix Maiya --game maiya --game-id 780 --version <MAJOR.MINOR.PATCH>
python3 tools/rom_registry.py
```

A version that went out is never rebuilt in place: changed bytes get a new
number (`docs/releases/VERSIONING_POLICY.md`).

`ASEPRITE=builtin` stands in for Aseprite in her walk/run pipeline (the same
`.aseprite` file and exports). `MAIYA_ART_REFERENCE` keeps her painted art
exactly as that revision's build drew it, because resampling and palette
fitting round differently from one PC to another; her frames, the code and
the sound build the same everywhere. `audit_release.py` checks every release
file for the hardware fixes (alignment, boot logo bank, FIX isolation, MVS
sound protocol, no BIOS in the cartridge packages).

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
depth projection, two-layer parallax, NPC patrol behaviour, live joystick handling
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

## shadowleague — Shadow League Fighting (id 781)

An original fighting game on the `ng_fight` engine (`docs/fighting.md`):
44 painted fighters, 12 arenas, single matches and three-on-three team
battles with strikers, a referee's ceremony, voices, a score with bonus
counts after each round won, arcade, versus and practice (AES), MVS and AES.
Fighters are JSON definitions (`games/shadowleague/fighters/`) cooked by the
game's tools; arenas are painted panoramas fitted to 32 palettes. Build with
`make GAME=shadowleague all`; check with `python3 games/shadowleague/tools/qa.py`
(`--team` for a team battle). Details: `games/shadowleague/README.md`.

---

## The genre games (ids 782-785)

Each is built on one of the v1.7.7 genre engines (`docs/GENRE_ENGINES_V1.7.7.md`),
chosen with `GAME_GENRE_ENGINE` in its `game.mk`:

- `sevenwonders` (`puzzle`): match and bubble boards and picture-logic
  levels, an album and rewards, saves.
- `maiyaluna` (`rpg`): two solo journeys through seven chapters of rooms,
  people, items and environmental puzzles; saves.
- `nosignal` (`beatemup`): eight streets for one player, weighty blows,
  grabs and throws, a ridable scooter, living backgrounds.
- `mahjong_demo` (`mahjong`): four seats of Hong Kong Old Style against three
  CPU players, and solitaire.

Each has its own `README.md` and `CHANGELOG.md`; `make GAME=<name> all` builds it.

## centuryxi — Century XI: Road to 2030 (id 787)

Original arcade football on the v1.7.8 football engine (`GAME_GENRE_ENGINE =
soccer`, `docs/GENRE_ENGINES_V1.7.7.md` section 7): eleven a side in a
pseudo-3D stadium, a cute parody of 1994 with 48 made-up nations (32 reach
each championship's finals), chibi figurines whose kits, skin and hair come
from palette roles at run time, and the whole championship: qualifying, an
animated draw, eight groups, the knock-out bracket, line-ups, top scorers,
suspensions and a trophy ceremony. Quick Cup, exhibition, two-player versus
with join-in, penalty challenge and a training ground; Classic and Modern
controls; eight CPU levels (`ng_tune`); video review; a 3D penalty view;
speech for the announcer, the chants and every shirt name. The match, the
men and the CPU are in `games/centuryxi/scenes`; the ball, the lines,
offside and the championship are the engine's. Build with
`make GAME=centuryxi all`; host tests `python3 games/centuryxi/tests/run.py`;
play-test logging `games/centuryxi/tools/live_debug.py`. Details, controls and
measurements: `games/centuryxi/README.md` and `games/centuryxi/docs/STATUS.md`.

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

## Experimental Neo Geo CD target

Only `helloworld/main.c` currently has an opt-in native CD build with dedicated
startup and original diagnostic assets. Stock BIOS boot acceptance is blocked;
the secondary HLE video/input/effect diagnostic does not establish a finished
game port. Tutorial, demo and larger-game CD ports are not available yet.
See [CD build/status](NEOGEO_CD.md). Existing AES/MVS game targets are unchanged.
