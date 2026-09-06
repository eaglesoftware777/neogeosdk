# Programmer's Manual

**Eagle Software · Neo Geo SDK v1.7.0**

The working manual for building Neo Geo software with this SDK: the build
system, the project layout, the game contract, the engine tick, the asset
pipelines, and the rules that keep a frame inside 16.7 ms.

Read [`INTRODUCTION.md`](./INTRODUCTION.md) first if you have not — it
covers the hardware this manual assumes you know. For a first ROM, follow
[`HELLO_WORLD.md`](./HELLO_WORLD.md). For call-by-call detail, use
[`API_2D_ENGINE_C.md`](./API_2D_ENGINE_C.md),
[`API_2D_ENGINE_CPP.md`](./API_2D_ENGINE_CPP.md) and
[`../SDK_API_GUIDE.md`](../SDK_API_GUIDE.md).

---

# Part I — The build

## 1. Two makefiles, one build

| Host | Command |
|---|---|
| Linux, WSL | `make …` |
| Windows (native) | `make -f MakefileWin32.mak …` |

They implement the same targets and produce byte-identical ROMs. Everything
in this manual applies to both; only the invocation differs.

## 2. Selecting a game

Every game directory carries a `game.cfg` naming itself, and the repo root
carries one naming whichever game you are working on. The build cross-checks
`GAME=` against the cfg and stops if they disagree — which is what stops you
from silently building the demo's art into another game's ROM.

```sh
make GAME=skylance GAME_CFG_FILE=games/skylance/game.cfg all
```

Setting `CURRENT_GAME` in the root `game.cfg` lets you drop both arguments
for the game you work on most.

## 3. The five stages

`make all` runs them in order. Each is also a target on its own.

| Stage | Target | Produces |
|---|---|---|
| 1 | `art` | Sprite C-ROMs (`<id>-c1.c1`, `<id>-c2.c2`), `screens.c`, `sprite_meta.h`, palettes |
| 2 | `sfix` | The S1 FIX ROM (`<id>-s1.s1`), 128 KB |
| 3 | `sound` | ADPCM samples, V-ROM, and the M1 Z80 driver ROM |
| 4 | `p1` | The 68000 program ROM (`<id>-p1.p1`), 512 KB |
| 5 | *(in `p1`)* | `hash_eagle/<game>/neogeo.xml` — the MAME hash set |

In day-to-day work you will run `p1` almost exclusively; it takes seconds on
a warm tree. Re-run `art` when you change a PNG, `sound` when you change
audio, and `all` before you ship.

### Full target list

```text
make all             art + sfix + sound + p1
make p1              68000 program ROM only
make aes / mvs       force the AES or MVS platform variant
make art             rebuild sprite C-ROMs from artbox
make art-crt         same, through the CRT-preprocessing pipeline
make sfix / srom     rebuild the S1 FIX ROM
make sound           samples + vrom + fmpatches + fm + mml + ssgconfig + ssg + m1rom
make vrom            rebuild the V-ROM from ADPCM assets
make m1rom           the shipping ASM Z80 driver build
make m1rom-c         the experimental C driver build
make compare-driver  build both and diff the ROM images
make fm / fmpatches / mml / ssg / ssgconfig / samples
make dist            package dist/roms/<game>.zip
make test            run in MAME (MVS, sp-s2.sp1 BIOS)
make test-aes        run in MAME (AES, unibios22)
make test BIOS=unibios40
make unit-tests      host-side sprite renderer tests (no emulator)
make check           unit tests plus a complete ROM set
make debug           run MAME with the debugger attached
make debug-build     P1 with debug symbols and a linker map
make debug-artifacts size, symbols, readelf, map and disassembly dumps
make gdb / gdb-trace / gdb-remote
make bios-list       list the BIOS images MAME can see
make games-list      list the games in the tree
make menu / help     the interactive launcher
make clean / sound-clean / art-clean / clean-all
```

## 4. Build behaviour worth knowing

- ROMs are written per game: `roms/demo/777-p1.p1`, `roms/skylance/779-p1.p1`.
- `hash_eagle/<game>/neogeo.xml` is regenerated on every `p1` with correct
  CRC/SHA1 and `loadflag="load16_word_swap"` on the P-ROM. MAME will not
  load the cart without it.
- P1 generation crops to the full 512 KB window (`0x080000`) *before* the
  byte swap and padding. This is the required format for both MAME and
  hardware — a ROM built without the crop boots to garbage.
- `make samples` uses the bundled Python WAV converter by default. Set
  `SOX=/path/to/sox` only if you specifically want the SoX path.
- `make dist` puts the ROM files at the archive root, no subfolder.
- `make m1rom-asm` is the authoritative M1 path; the C driver is for
  comparison.

## 5. Debugging

```sh
make debug-build        # P1 with symbols + linker map
make debug-artifacts    # size / nm / readelf / map / disassembly into dump/
make gdb                # GDB on out/game
make gdb-remote GDB_REMOTE=localhost:23946
make gdb-trace          # scripted batch trace into dump/gdb_trace.txt
make debug              # MAME with the debugger attached
```

Symbol addresses come from `m68k-unknown-elf-nm -n out/game`, which is how
you find a variable to watch from a MAME Lua script. MAME's
`-autoboot_script` plus `install_write_tap` is the practical way to trace a
value across frames — keep the tap handle in a global or it is collected and
silently stops firing.

Full workflow: [`GDB_GUIDE.md`](./GDB_GUIDE.md).

---

# Part II — Writing the game

## 6. The contract

A Neo Geo cartridge boots into the BIOS, not into `main()`. The BIOS calls
back into the cart for power-on, eye-catcher, game and title; `user.c`
provides those hooks and you rarely touch it.

What you write is:

```c
void game_boot(void);    /* once, after the BIOS hands over */
void game_frame(void);   /* once per frame, forever */
```

`game_frame()` returns every frame, so anything that must persist across
frames is `static` or file-scope. A plain local is gone before the next
call — the most common first bug on this platform.

## 7. Project layout

```
games/mygame/
├─ game.mk           GAME_NAME, GAME_ID, GAME_SCENES, GAME_EXTRA_INCLUDES
├─ game.cfg          CURRENT_GAME=mygame
├─ main.c            game_boot() / game_frame(), and the generated screen table
├─ user.c            BIOS hooks — interrupt vectors, USER dispatch
├─ eyecatcher.c      generated by the artbox
├─ neogeo.ld         linker script
├─ neogeo_mvs.c      MVS platform stub
├─ neogeo_aes.c      AES platform stub
├─ scenes/           your code, listed in GAME_SCENES
├─ artbox/in/        source PNGs
└─ sound/            MML, FM, SSG and WAV sources
```

`game.mk`:

```make
GAME_NAME  = My Game
GAME_ID    = 780
GAME_SCENES = title stage boss
GAME_EXTRA_INCLUDES = -Igames/other/artbox   # optional
```

`GAME_ID` becomes the ROM filename prefix and must be unique in the tree.
In use: 555 tutorial, 772 helloworld, 775 neogeogame, 777 demo, 778
demo_plus, 779 skylance.

`GAME_SCENES` is an explicit list, not a wildcard, so helper files that are
`#include`d elsewhere do not get compiled twice.

Details: [`ADDING_A_GAME.md`](./ADDING_A_GAME.md).

## 8. Driving the engine

The minimum:

```c
#include "sdk/2d_engine/ng_engine.h"

void game_boot(void)  { ng_game_engine_init(); soundInit(); }
void game_frame(void) { waitVbl(); ng_game_engine_frame(); }
```

`ng_game_engine_frame()` runs a fixed order — timers, characters, actions,
physics, NPCs, camera, particles, palette effects, events — then flushes the
render queue inside VBlank. You attach your game at five points:

```c
ng_game_engine_set_hooks(before_logic,
                         collision_logic,
                         after_events,
                         before_draw,
                         after_draw);
```

Use the hooks rather than reimplementing the tick. Ordering inside the tick
exists for reasons that are not obvious from outside: physics resolves after
character movement but before the camera reads positions, and the render
queue flush must be the last thing that happens.

## 9. Characters

`ng_chars` is a fixed pool of 64. Add, bind art, set a body, and the engine
moves, animates, culls and draws them.

```c
NGCharacter *c = chars_add(KIND_PLAYER, x, y);
ng_char_bind_asset(c, &asset_view);
ng_char_set_body(c, 0, 0, 24, 40);
ng_char_set_speed_fp(c, vx_fp, vy_fp);
ng_char_set_priority(c, NG_RENDER_BAND_PLAYER, 0);
```

Two traps that have cost real debugging time:

**`ng_chars_count()` is not an array bound.** It returns the number of
*active* objects. Once a slot frees, the active count is lower than the
highest occupied index, and a loop bounded by it silently skips the tail.
Iterate `NG_MAX_CHARS` and skip inactive slots:

```c
for (i = 0; i < NG_MAX_CHARS; i++) {
    NGCharacter *c = chars_at(i);
    if (!c || !c->active) continue;
    ...
}
```

**Remove with `ng_chars_remove()`.** Setting a "destroy pending" life state
is not acted on by the engine — the object stays active, the pool fills, and
new spawns start failing. This presents as "the player can only fire six
shots, then never again".

For collision over two pools, gather index lists first and then test list
against list. A naive double loop over `NG_MAX_CHARS` is 64 × 64 rectangle
tests per frame.

## 10. Sprites, slots, and priority

**HIGHER slot number is drawn IN FRONT.** This is the observed hardware
direction. Earlier revisions of the engine headers claimed the opposite, and
the result was backgrounds parked at "behind" slots drawing over characters.

| Slots | Use |
|---|---|
| 1–32 | Background layers, parallax — the true back |
| 33–95 | Title and one-off generated screens |
| 96–223 | Characters and NPCs, assigned by `ng_chars` |
| 224–255 | Effects |
| 256–287 | Particles |
| 288–379 | Foreground / temporary effects |

`NG_SPR_BG0_FIRST` and `NG_SPR_BG1_FIRST` predate the correction; the names
read as if they were about layer order and they are not. Prefer explicit
slot numbers in new code.

Other hardware facts that shape your code:

- **SCB2 only shrinks.** There is no stretch. Author art at its largest
  intended size and scale down. A "30 %" sprite is `0x4C`-ish in the shrink
  field, not a multiplier you can exceed.
- **SCB3's Y field is `496 - y`, nine bits.** So `y` and `y + 512` are the
  same position, and negative Y wraps correctly — you do not need to clamp
  a sprite moving off the top.
- **381 sprites, 96 per scanline.** The per-line limit is what actually
  bites: a wide row of enemies at the same Y will drop sprites long before
  the total budget is reached.
- **A page is 16 × 16 tiles.** A scrolling background must be exactly one
  page in the scroll axis and seamless at the join, because you cannot
  stretch it to fit.

## 11. The FIX layer

40 × 32 cells of 8 × 8 pixels, of which **28 rows are visible**. Visible row
*y* is map row *y + 2*; the `ng_fix_*` helpers apply that for you.

The FIX map word is `(palette << 12) | tile` — **four bits of palette**, so
banks 0–15 only. A game with more than 16 FIX palettes has to fold
duplicates; `sky_fix_palettes_init()` in Sky Lance is a worked example of
doing that at boot.

**The BIOS SFIX font's space glyph is opaque.** Printing `" "` punches a
solid colour-index-2 plate through whatever is behind it. To leave a cell
genuinely empty use `ng_fix_blank_cell()`; tile `0xFF` is the transparent
one.

The layer is always on top of every sprite. That is why it is the right
place for a HUD and the wrong place for anything that should pass behind a
character.

## 12. Palettes and the backdrop

Palette RAM is `$400000–$401FFF`. A colour word is not RGB444: bit 15 is the
dark bit, bits 14–12 carry the least-significant bit of each channel, and
bits 11–0 carry the top four bits of R, G and B — **5 bits per channel, 32
levels, in steps of 8**.

**The backdrop register is the last word of palette RAM, `$401FFE`.**
`$402000` is a mirror that silently does nothing; writing there is a
frequent cause of "my backdrop colour is ignored".

`ng_palette_fx` runs fades, flashes, pulses and colour cycles on a bank, all
routed through the render queue so an effect started in game logic still
lands inside VBlank.

## 13. Input

```c
ng_joystick_update();                 /* once per frame, first */
if (ng_joy_pressed() & BUTTON_A) { ... }   /* edge */
if (ng_joystick_state()->raw & JOY_LEFT) { ... }
```

`poll_joystick()` gives you the raw current state; acting on it directly
fires once per frame for as long as the button is held. `ng_joy_repeat()`
gives menu-style auto-repeat, `ng_joy_held_frames()` gives charge timing,
and `ng_joy_special_qcf()` / `ng_joy_special_dp()` recognise motion inputs.

---

# Part III — Assets

## 14. Art

Drop PNGs into `games/<game>/artbox/in/<category>/` and run `make art`. The
pipeline quantises to the Neo Geo colour lattice, builds palettes, packs
tiles into C-ROMs, and generates `screens.c` (one `showScreenN()` per asset)
and `sprite_meta.h` (geometry per asset).

Categories are processed in a fixed order — `backgrounds`, `characters`,
`effects`, `eyecatcher`, `npc`, `screens`, `titles` — and any other
directory is appended alphabetically. **Asset ids are the 1-based position in
that order**, so inserting a file in the middle renumbers everything after
it. The shipped games pad filenames to control ordering; do the same.

`characters` and `npcs` share one derived 15-colour master palette per
group. Other categories get per-asset palettes. That is why a sprite that
looks wrong after being moved between categories is usually a palette
problem, not a conversion problem.

Full detail, including the quantiser and the Artbox Studio GUI:
[`ARTBOX_PIPELINE.md`](./ARTBOX_PIPELINE.md).

## 15. Sound

`soundInit()` once at boot, then `playMusic()`, `playSFX()`, `playSFXB()`,
`playFMTrack()`, `playSSGTrack()` with ids from `sdk/sound_ids.h`.

Every call posts one byte to a second CPU. Nothing is synchronous, and
multi-byte commands (a prefix plus a parameter) must be spaced by a
`waitVbl()` or the second byte can be dropped.

Full detail: [`SOUND_DRIVER.md`](./SOUND_DRIVER.md). Composing tools:
[`SOUND_STUDIO_GUIDE.md`](./SOUND_STUDIO_GUIDE.md).

---

# Part IV — Staying inside the frame

## 16. The budget

| Resource | Limit |
|---|---|
| CPU | ~12 MHz 68000, ~200,000 cycles per frame |
| VBlank | ~16 scanlines, ~1.1 ms — all VRAM writes must fit |
| Sprites | 380 total, **96 per scanline** |
| Palettes | 16 entries per bank, 256 banks |
| FIX | 40 × 32 cells, 28 rows visible, 16 palettes |

## 17. The rules

**No `malloc` during gameplay.** Every engine subsystem uses a fixed pool.
Dynamic allocation fragments RAM and produces non-deterministic pauses.

| Pool | Size | Constant |
|---|---|---|
| Characters | 64 | `NG_MAX_CHARS` |
| NPCs | 32 | `NG_MAX_NPCS` |
| Particles | 32 | `NG_PART_MAX_PARTICLES` |
| Palette FX | 8 | `NG_PALFX_MAX_SLOTS` |
| Render queue | 128 | `NG_RQ_MAX_CMDS` |

**No `float` or `double`.** There is no FPU; every float costs hundreds of
cycles in software emulation. Use 16.16 fixed-point from `ng_fixed.h`.

**No division in the frame loop.** `DIVS`/`DIVU` cost 76–158 cycles each.
Use `ng_sin_tab[]`, `ng_cos_tab[]`, `ng_shrink_tab[]`, right shifts, and
fixed-point multiply.

**Write only what changed.** Sprite groups carry dirty flags; the FIX layer
has a dirty-cell cache. Rewriting an unchanged sprite chain every frame is
the fastest way to blow the VBlank budget.

**Queue every VRAM write.** Post to the render queue from game logic and let
`ng_render_queue_flush()` drain it in VBlank. Writing VRAM outside VBlank
produces tearing and corrupt tiles.

**Budget sprites per line, not per screen.** 96 is the number that matters.

More: [`performance_rules.md`](./performance_rules.md).

---

# Appendix A — Register quick reference

| Address | R/W | Register |
|---|---|---|
| `$3C0000` | W | VRAM address (word index) |
| `$3C0000` | R | VRAM read data |
| `$3C0002` | W | VRAM write data |
| `$3C0004` | R/W | Auto-increment |
| `$3C0006` | W | Mode — timer / IRQ / auto-animation |
| `$3C0008` / `$3C000A` | W | Timer high / low |
| `$3C000C` | W | IRQ acknowledge (bit 1 timer, bit 2 VBL) |
| `$320000` | R/W | Sound command port to / from the Z80 |
| `$400000`–`$401FFF` | R/W | Palette RAM |
| `$401FFE` | W | **Backdrop colour** |
| `$3A000F` / `$3A001F` | W | Palette bank select |
| `$3A0011` | W | Shadow / display dim |
| `$300001` | W | Watchdog kick |

Word-aligned access only. Wait ≥ 1.4 µs after touching the VRAM address
register before reading.

# Appendix B — Interrupts

| IRQ | Source | Vector | Acknowledge |
|---|---|---|---|
| 1 | V-Blank | `$64` | `MOVE.W #4, $3C000C` |
| 2 | LSPC timer | `$68` | `MOVE.W #2, $3C000C` |

Kick the watchdog (`MOVE.B D0, $300001`) once per frame in the VBlank
handler or the board resets.

# Appendix C — Common symptoms

| Symptom | Cause |
|---|---|
| Sprite invisible behind the background | Slot priority — higher is in front |
| Sprite bigger than intended, cannot shrink further | SCB2 only reduces; re-author the art |
| Backdrop colour ignored | Wrote `$402000` instead of `$401FFE` |
| Text draws solid blocks | BIOS space glyph is opaque; use `ng_fix_blank_cell()` |
| FIX palette wraps onto text colours | Only 4 palette bits — fold duplicates below 16 banks |
| Pool fills and spawns stop | Objects marked destroyed but never `ng_chars_remove()`d |
| Loop skips the last objects | `ng_chars_count()` used as an array bound |
| Second byte of a sound command lost | Prefix and parameter not spaced by `waitVbl()` |
| Sprites dropped along one row | 96-per-scanline limit |
| Green self-test screen on boot | BIOS self-test, not your ROM — try `BIOS=unibios40` |
| Tearing or corrupt tiles | VRAM written outside VBlank instead of queued |
