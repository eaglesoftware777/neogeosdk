# Adding a New Game to the NeoGeo SDK

This guide walks through every step needed to add a new game called `mygame`
with ROM ID prefix `888`. Substitute your own names throughout.

---

## 1. Create the game directory tree

```
games/
└── mygame/
    ├── game.mk
    ├── main.c
    ├── user.c
    ├── eyecatcher.c
    ├── neogeo_mvs.c         (copy from games/demo/ and customise)
    ├── neogeo_aes.c         (copy from games/demo/ and customise)
    ├── artbox/
    │   └── infix/           (FIX-layer tile imports, may be empty)
    ├── scenes/
    │   └── mygame.c / .h    (game-specific scene files)
    └── sound/               (optional — per-game sound assets)
        ├── fm/
        ├── mml/
        ├── ssg/
        └── samples/
```

The easiest starting point is to copy `games/demo/` and rename files.

---

## 2. Write `games/mygame/game.mk`

```makefile
GAME_NAME  = My Game
GAME_ID    = 888
GAME_SCENES = mygame
```

`GAME_SCENES` is a space-separated list of `.c` filenames (without the `.c`)
inside `games/mygame/scenes/`.  Every file listed here is compiled and linked.

---

## 3. Implement the required entry points

`main.c` — called at boot after BIOS init:
```c
#include "sdk/neogeo.h"
void game_main(void) { /* your attract loop / title / gameplay */ }
```

`user.c` — BIOS USER hook (called every VBlank):
```c
#include "sdk/neogeo.h"
void user_frame(void) { /* optional per-VBlank logic */ }
```

`eyecatcher.c` — MVS insert-coin screen:
```c
#include "sdk/neogeo.h"
void eyecatcher(void) { /* eyecatcher attract loop */ }
```

See `games/demo/` for working examples of each.

---

## 4. Build the game

```bash
# Linux
make GAME=mygame all

# Windows
make -f MakefileWin32.mak GAME=mygame all
```

`GAME_ID` is read from `games/mygame/game.mk` automatically.

Individual steps are available if you only changed one subsystem:

| Target     | What it rebuilds                  |
|------------|-----------------------------------|
| `p1`       | 68k code → P1 ROM                 |
| `art`      | Sprites → C1/C2 ROMs              |
| `sfix`     | Fix-layer font → S1 ROM           |
| `sound`    | Z80 driver + samples → M1/V1 ROMs |
| `all`      | art + sfix + sound + p1           |

---

## 5. Per-game ROM folder

All six ROMs land in `roms/mygame/`:

```
roms/
└── mygame/
    ├── 888-p1.p1
    ├── 888-m1.m1
    ├── 888-s1.s1
    ├── 888-v1.v1
    ├── 888-c1.c1
    └── 888-c2.c2
```

MAME is invoked with `-cart1 mygame` so it looks in `roms/mygame/` automatically.

---

## 6. Per-game hash XML

`gen_hash.py` generates `hash_eagle/mygame/neogeo.xml` with an entry for
`mygame` and ROM prefix `888`.  The HASHPATH passed to MAME is:

```
hash_eagle/mygame : hash_eagle : hash
```

MAME finds `hash_eagle/mygame/neogeo.xml` first (highest priority), so
different games never stomp each other's checksums.

Run it manually if needed:

```bash
GAME=mygame GAME_ID=888 python3 hash_eagle/gen_hash.py
```

---

## 7. Test in MAME

```bash
# Linux
make GAME=mygame test

# Windows
make -f MakefileWin32.mak GAME=mygame test
```

Override the BIOS if needed:

```bash
make GAME=mygame test BIOS=unibios40
```

---

## 8. Using the 2D engine

To use `ng_game_engine_init` / `ng_game_engine_frame` from `ng_game_interupt.h`:

```c
#include "sdk/2d_engine/ng_game_interupt.h"

void game_main(void)
{
    ng_engine_init_hardware(0);      /* clear VRAM, set transparent tile */
    ng_game_engine_init();           /* init all subsystems */

    ng_game_engine_set_hooks(
        my_before_logic,
        my_collision_logic,
        my_after_events,
        my_before_draw,
        my_after_draw
    );

    while (1) {
        waitVbl();
        ng_game_engine_frame();      /* runs logic, draw, palette FX, flush */
    }
}
```

`ng_game_engine_init()` now wires up every subsystem automatically:
render queue, particles, palette FX, feedback, and depth FX.

---

## 9. Linker script

Copy `games/demo/neogeo.ld` to `games/mygame/neogeo.ld`.  Adjust section
sizes if your game has more/fewer scenes or a larger BSS footprint.

The catch-all patterns at the end of `game_engine_bss` handle `.data` and
`.bss` sections from engine objects regardless of path separator, which is
required for Windows GNU ld compatibility.

---

## 10. Sound assets (optional)

If your game has its own music/samples, place them in `games/mygame/sound/`:

```
games/mygame/sound/
├── fm/patches.fm        (FM synth patches)
├── fm/*.mml             (FM music tracks)
├── mml/*.mml            (general MML)
├── ssg/config.ssg       (SSG channel config)
├── ssg/*.mml            (SSG tracks)
└── samples/
    ├── in_wav_a/        (ADPCM-A source WAVs, 8kHz mono 16-bit)
    └── in_wav_b/        (ADPCM-B source WAVs, 8kHz mono 16-bit)
```

Then build with:

```bash
make GAME=mygame sound
```

If a directory is missing the build silently skips it, so you only need the
subdirectories for the sound features you actually use.

---

## 11. Distribution package

```bash
make GAME=mygame dist
```

This produces:

```
dist/
├── roms/mygame.zip             (all six ROMs)
├── hash_eagle/mygame/neogeo.xml
├── run_mygame.sh / .bat
└── run_mygame_debug.sh / .bat
```

Distribute `roms/mygame.zip` + `hash_eagle/mygame/neogeo.xml` + the launcher
scripts. Players place `neogeo.zip` (BIOS) in the `roms/` folder and run the
launcher.
