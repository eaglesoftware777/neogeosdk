# Hello World — your first Neo Geo ROM

**Eagle Software · Neo Geo SDK v1.7.0**

This tutorial takes you from a clean checkout to a running ROM that prints
text on screen and plays a sample when the player presses **A**. It uses the
`games/helloworld` project that ships with the SDK, then shows you how to
copy it into a game of your own.

Budget: about ten minutes, most of it the first art build.

---

## 0. Before you start

You need the toolchain installed and `SDKHOME` set. See
[`DEPENDENCIES.md`](./DEPENDENCIES.md) and the installation sections of
[`../README.md`](../README.md), or run the installer for your platform from
`install/`.

Quick check:

```sh
m68k-unknown-elf-gcc --version     # 68000 cross-compiler
wla-z80 --version                  # Z80 assembler for the sound driver
python3 --version                  # 3.10+, with Pillow and NumPy
make --version
```

## 1. Build and run it

```sh
# Linux / WSL
make GAME=helloworld GAME_CFG_FILE=games/helloworld/game.cfg all
make GAME=helloworld GAME_CFG_FILE=games/helloworld/game.cfg test

# Windows
make -f MakefileWin32.mak GAME=helloworld GAME_CFG_FILE=games/helloworld/game.cfg all
make -f MakefileWin32.mak GAME=helloworld GAME_CFG_FILE=games/helloworld/game.cfg test
```

`all` runs five stages in order — **art**, **sfix**, **sound**, **p1**, and
the hash-set generation MAME needs. `test` launches MAME on the result.

Every game in the tree carries its own `game.cfg` naming itself, and the
repo-root `game.cfg` names whatever game you are working on. The
`GAME_CFG_FILE=` argument is what lets you build a different game without
editing the root file. If the two disagree, the build stops and tells you.

You should see `Hello World!` and `Press A for sound` on a black screen.
Press **A**: sample 0 plays.

## 2. What actually ran

A Neo Geo cartridge boots into the BIOS, not into your `main()`. The BIOS
calls back into the cartridge for four things — power-on, eye-catcher,
game, and title — and every game in this SDK provides those hooks in
`user.c`. You almost never edit that file.

What you *do* write is two functions in `main.c`:

```c
void game_boot(void);    /* called once, after the BIOS hands over */
void game_frame(void);   /* called once per frame, forever */
```

That is the whole contract for a minimal game.

### `games/helloworld/main.c`

```c
#include <stdint.h>
#include "sdk/2d_engine/ng_fix.h"
#include "sdk/neogeo.h"
#include "sdk/macro.h"    /* button and joystick bit masks */

void game_boot(void) {
    ng_fix_init();                              /* set up the FIX text layer */
    ng_fix_clear();                             /* wipe whatever the BIOS left */
    ng_fix_puts(3, 15, "Hello World!", 0);      /* x, y in 8x8 cells; 0 = palette */
    ng_fix_puts(3, 17, "Press A for sound", 0);
    soundInit();                                /* boot the Z80 + YM2610 */
}

static uint16_t prev_joy = 0;                   /* file scope: survives frames */

void game_frame(void) {
    uint16_t joy = poll_joystick();             /* JOY_* / BUTTON_* bitfield */

    if (!(prev_joy & BUTTON_A) && (joy & BUTTON_A)) {
        playSFX(0);                             /* rising edge only */
    }
    prev_joy = joy;
}
```

Three things in that file are worth internalising, because they apply to
every Neo Geo program you will write:

**The FIX layer is cell-addressed, not pixel-addressed.** `ng_fix_puts(3,
15, ...)` means column 3, row 15 of a 40 × 32 grid of 8 × 8 tiles. Only 28
rows are visible, and the SDK's FIX helpers already account for the two-row
offset between map rows and screen rows.

**Input is polled, not queued.** `poll_joystick()` returns the *current*
state. If you act on the raw value you will fire once per frame for as long
as the button is held, which is almost never what you want — hence the
`prev_joy` edge test. The 2D engine's `ng_joystick` module does this
bookkeeping for you (`ng_joy_pressed()`), and that is what a real game
should use.

**Sound is a one-byte command to another CPU.** `playSFX(0)` writes a
sound code to `$320000`. The Z80 picks it up on its own port and dispatches
it. Nothing about that call is synchronous; do not expect a return value or
a completion signal.

### Why `prev_joy` is at file scope

A `static` inside `game_frame()` would work too. What must *not* happen is a
plain local — `game_frame()` returns every frame, so a stack local is gone
before the next call. This is the single most common first bug.

## 3. Change something

Edit the text, then rebuild only the 68000 side:

```sh
make GAME=helloworld GAME_CFG_FILE=games/helloworld/game.cfg p1
make GAME=helloworld GAME_CFG_FILE=games/helloworld/game.cfg test
```

`p1` skips art and sound and just recompiles and relinks the program ROM.
On a warm tree it takes a couple of seconds, and it is the target you will
use ninety percent of the time.

Some things to try:

| Change | What you learn |
|---|---|
| `ng_fix_puts(3, 15, "Hello World!", 1)` | Palette 1 instead of 0. FIX palettes are 4 bits — banks 0–15 only. |
| Add `setBACKDROP(0x8001);` in `game_boot()` | The backdrop register is the whole background layer. |
| Print `joy` with `ng_fix_put_u16(3, 19, joy, 0, 0)` | Watch the input bitfield live. |
| `playSFX(1)` on `BUTTON_B` | Sample ids come from `sound/samples/in_wav_a/`. |

## 4. Add a sprite

`ng_fix_*` draws text. Anything that moves is a sprite, and sprites come
from the artbox.

1. Drop a PNG into `games/helloworld/artbox/in/characters/`.
2. Re-run the art stage:
   ```sh
   make GAME=helloworld GAME_CFG_FILE=games/helloworld/game.cfg art
   ```
3. The pipeline writes `games/helloworld/artbox/sprite_meta.h` with one
   entry per asset, plus the C-ROM data the hardware reads tiles from.
4. Include `sprite_meta.h` and bind the asset to a character with
   `ng_char_bind_asset()`, or draw it directly with the sprite-group API.

The asset id is its 1-based index in the generated manifest, and the order
is decided by directory (`backgrounds`, `characters`, `effects`,
`eyecatcher`, `npc`, `screens`, `titles`, then any other directory
alphabetically) and then by filename. Adding a file in the middle of a
category renumbers everything after it, so the shipped games pad names to
control ordering. [`ARTBOX_PIPELINE.md`](./ARTBOX_PIPELINE.md) covers this
properly.

## 5. Turn it into your own game

`games/neogeogame` (id 775) is the same skeleton with nothing in it, ready
to be renamed. To start clean:

```sh
cp -r games/helloworld games/mygame
```

Then edit **four** things:

| File | Change |
|---|---|
| `games/mygame/game.mk` | `GAME_NAME`, and a `GAME_ID` no other game uses |
| `games/mygame/game.cfg` | `CURRENT_GAME=mygame` |
| `games/mygame/main.c` | your `game_boot()` / `game_frame()` |
| `games/mygame/artbox/in/` | your PNGs |

`GAME_ID` becomes the ROM filename prefix (`777-p1.p1`, `779-c1.c1`, …), so
it has to be unique across the tree or two games will overwrite each other's
output. Ids in use: 555 tutorial, 772 helloworld, 775 neogeogame, 777 demo,
778 demo_plus, 779 skylance.

If your game needs more than `main.c`, add scene files under
`games/mygame/scenes/` and list them in `GAME_SCENES` in `game.mk` — the
build compiles exactly that list, so helper files that are `#include`d
elsewhere do not get compiled twice.

Full instructions: [`ADDING_A_GAME.md`](./ADDING_A_GAME.md).

## 6. Stepping up to the engine

`helloworld` talks to the hardware directly. `games/tutorial` (id 555) is
the same size but drives the 2D engine instead:

```c
#include "sdk/2d_engine/ng_engine.h"

void game_boot(void)  { ng_game_engine_init(); soundInit(); }
void game_frame(void) { waitVbl(); ng_game_engine_frame(); }
```

`ng_game_engine_frame()` runs the whole engine tick in order — timers,
characters, actions, physics, NPCs, camera, particles, palette effects,
events — and flushes the render queue during VBlank. You hook into it at
five points with `ng_game_engine_set_hooks()`: before logic, collision,
after events, before draw, after draw.

From there:

- [`GAME_ENGINE_LAYER.md`](./GAME_ENGINE_LAYER.md) — how the engine tick fits together
- [`API_2D_ENGINE_C.md`](./API_2D_ENGINE_C.md) — every C call, by module
- [`API_2D_ENGINE_CPP.md`](./API_2D_ENGINE_CPP.md) — the C++ build
- [`GAMES.md`](./GAMES.md) — what each shipped game demonstrates, and
  `games/skylance` as a complete worked example

## 7. When it does not work

| Symptom | Cause |
|---|---|
| `ERROR: GAME mismatch` | `GAME=` and `CURRENT_GAME` in the cfg disagree. Pass the right `GAME_CFG_FILE=`. |
| Green screen with test text on boot | The BIOS self-test, not your ROM. It is normal on some BIOS revisions; `make test BIOS=unibios40` skips it. |
| Blank white or black screen | Usually a backdrop or palette that was never loaded. Check `setBACKDROP()` and that your palettes are uploaded before you draw. |
| Text draws as solid blocks | The BIOS SFIX font's space glyph is *opaque* colour index 2. To leave a cell truly empty use `ng_fix_blank_cell()`, not `" "`. |
| Sprite is invisible | Lower slot = in front. A background parked at a low slot covers everything. Also check the sprite was not left with SCB3 = `0x8000`. |
| Sprite is the wrong size | SCB2 only shrinks. Author art at full size and scale down. |
| Link error about `ng_screen_table` | Only if you removed the weak fallback in `ng_bg.c`; a game with no generated screens links against it. |

The demo ROM prints a chapter number in the top-right corner of every scene
precisely so problems can be reported by number — see
[`DEMO_CHAPTERS.md`](./DEMO_CHAPTERS.md).
