# NEO·GEO SDK — v1.7.0

**EAGLE SOFTWARE · MIT · A HARDWARE-CENTERED SDK**

> Direct 68000-side control over VRAM, palettes, sprites, DMA and FIX tiles,
> and a custom YM2610 sound stack driven by a Z80 sound driver.
>
> **68000 · Z80 · YM2610** — SNK Arcade (MVS) and Home (AES)
>
> **TECHNICAL OVERVIEW · 2026**

---

## 01 / OVERVIEW — The pitch

### Real Neo Geo development, made practical.

For self-made games, demos and homebrew that want to stay close to original
Neo Geo development practice — while still using modern build tooling,
scripting, and emulator-based test loops.

The goal is not to abstract the hardware away. The goal is to make working
with it possible on Linux, WSL, and Windows.

| | |
|---|---|
| **Target** | SNK Neo Geo — cartridge-era arcade hardware (MVS) and the home AES |
| **Release** | v1.7.0 · MIT licensed |
| **Host platforms** | Linux · WSL · Windows |
| **Build languages** | C99 · C++14 · 68000 asm · Z80 asm · Python |
| **Audio layers** | FM · SSG · ADPCM-A · ADPCM-B |
| **Shipped games** | 6, all building to real ROMs |
| **Engine builds** | 2 — plain C and C++14, same ABI |

---

## 02 / HARDWARE — The target machine

### Three chips. One bus. No engine.

| | Chip | Role |
|---|---|---|
| **Main CPU** | M68000 @ 12 MHz | Game logic, VRAM, palettes, sprites, DMA, FIX tiles — directly |
| **Sound CPU** | Z80A @ 4 MHz | Runs the dedicated driver; talks to the YM2610 over command ports |
| **Sound chip** | YM2610 | FM synthesis, SSG square waves, ADPCM-A and ADPCM-B sample playback |

### The development model

Graphics, palettes, FIX tiles, ROM layout and sound commands are part of the
day-to-day programming model rather than hidden behind a large engine layer.
The SDK preserves that — it gives you headers, build glue, asset pipelines
and a 2D engine you can opt into, not abstractions you cannot see past.

---

# PART ONE — For everyone

## 03 / NEO GEO DEV 101

Before the SDK matters, the hardware has to make sense. Six sections on what
every Neo Geo programmer needs to know: the FIX layer, sprites, palettes,
VRAM access, interrupts, and how the two CPUs talk.

---

## 04 / GRAPHICS — How a frame is drawn

### Three layers, fixed priority. A frame is stacked, not composited.

**FIX layer** — 40 × 32 grid of 8 × 8 tiles in a fixed location, **28 rows
visible**. Up to 15 colours per cell, 16 palettes, 4096 tiles. This is the
HUD / text plane, and it is always on top. *(128 KB ROM · S1)*

**Sprites** — up to **380 on screen, 96 per scanline**. Each sprite is a
vertical strip of up to 32 tiles of 16 × 16 pixels, with per-sprite vertical
and horizontal *reduction* and a chain bit that welds a strip to its left
neighbour. *(up to 128 MB ROM · C ROMs)*

**Backdrop** — a single solid colour behind everything else. One value from
65,536. That is the entire background layer. *(1 colour)*

### Why this matters

There is no tile-mapped background plane. Every moving and static element —
player, enemies, terrain, parallax — is built from sprites. Sprite count and
the horizontal-per-line limit drive your scene budget.

### Two rules that catch everyone

- **SCB2 can only shrink, never stretch.** Author art at its largest
  intended size.
- **HIGHER slot number is drawn IN FRONT.** Backgrounds belong at slots
  1–32, characters at 96–223, foreground effects at 288+.

---

## 05 / MEMORY — VRAM access via LSPC

### You don't poke VRAM directly. Talk to LSPC; it talks to VRAM.

| Address | R/W | Register |
|---|---|---|
| `$3C0000` | W | VRAM address (16-bit word index) |
| `$3C0000` | R | Read data (address register unchanged) |
| `$3C0002` | W | VRAM write data — writing pushes to VRAM |
| `$3C0004` | R/W | Auto-increment — added to the address after every write |
| `$3C0006` | W | Mode register — timer / IRQ / animation speed |
| `$3C0008` | W | Timer high |
| `$3C000A` | W | Timer low |
| `$3C000C` | W | Interrupt clear (bit 1 timer, bit 2 VBL) |

**VRAM size** 64 KB + 4 KB. All access is word-aligned; long-word and byte
access are not allowed.

**Timing rule** ~1.4 µs. Wait at least 16 CPU clocks after touching the
address register before reading. Cross-bank writes cannot use
auto-increment.

**SDK shortcut** The headers in `sdk/` wrap these registers in C macros, so
you write to named symbols rather than raw addresses.

---

## 06 / TIMING — Interrupts

### Two interrupts. That's the whole frame loop.

**IRQ 1 · V-BLANK** — fires once per frame, vector `$64`. This is your main
game tick: read inputs, run logic, push the frame's sprite list into VRAM.
The window is ~3 ms.

```asm
MOVE.W #4, $3C000C   ; ack VBL
MOVE.B D0, $300001   ; kick watchdog
```

**IRQ 2 · TIMER** — fires when the LSPC timer counter hits 0, vector `$68`.
Decrements every 167 ns (one pixel). Used for raster effects: split scrolls,
mid-frame palette swaps, status bars.

```asm
MOVE.W #2, $3C000C   ; ack timer
```

**Tip** For "every N pixels", set mode-register bits 4–7 to `%1001` and
Timer = N−1. For multiple arbitrary scanlines, use mode `%1101` and reload
the timer inside the handler.

---

## 07 / IPC — 68000 ↔ Z80 communication

### One byte at a time.

| Direction | Port | What happens |
|---|---|---|
| 68000 → Z80 | `$320000` (write byte) | The game writes a 1-byte sound code. The Z80 reads it on its own port and dispatches to the right music / SFX / FM / SSG handler. |
| Z80 → 68000 | `$320000` (read byte) | The Z80 can post a status byte back; the game reads from the same address. |

**SDK abstraction** You don't write that byte by hand. You call
`playMusic()` / `playSFX()` with a named id from `sdk/sound_ids.h`. The SDK
encodes it; the Z80 driver decodes it.

**One caveat** A prefix byte and its parameter are two separate NMIs on the
Z80. Space multi-byte commands with a `waitVbl()` or the second one can be
dropped.

---

## 08 / COLOUR — Palettes

### Colour is paletted, not direct. 4,096 on screen, of 65,536.

15 colours per character — 4 bits of palette index, colour 0 is transparent.
256 palettes total: 16 reserved for FIX, 239 for sprites, 1 for the
backdrop.

**Palette RAM** `$400000–$401FFF`. Two banks, toggled via `$3A000F` /
`$3A001F` for snappy palette swaps. The shadow bit at `$3A0011` dims the
entire display — pause screens, hit flashes.

**The backdrop register is the last word of palette RAM, `$401FFE`.**
`$402000` is a mirror that silently does nothing.

**The colour word is not RGB444.** Bit 15 is the dark bit, bits 14–12 carry
the least-significant bit of each channel, and bits 11–0 carry the top four
bits of R, G and B — **5 bits per channel, 32 levels, in steps of 8**. The
artbox quantiser snaps to exactly that lattice.

---

## 09 / PHILOSOPHY — Design intent

### The machine is close to the metal by design. The SDK doesn't try to change that.

| Preserve | Modernise | Resist |
|---|---|---|
| Original Neo Geo development practice — direct hardware register access | Build tooling, asset scripting, and emulator-based test loops on current operating systems | Heavy abstraction layers that hide what the hardware is actually doing |

---

## 10 / STRUCTURE — Repository map

```
neogeosdk/
├─ sdk/                  68000 headers, runtime, and both 2D engines
│  ├─ 2d_engine/         the C engine   (ng_*.c / ng_*.h)
│  ├─ 2d_engine_plus/    the C++ engine (ng_*.cpp / ng_*.hpp), same ABI
│  ├─ ng_fix/ ng_audio/ ng_video/ ng_scene/ ng_show/ bsp/
│  ├─ neogeo.h macro.h   hardware registers and inline helpers
│  └─ sound_ids.h        named music / SFX / FM / SSG ids
├─ games/                one directory per game, each self-contained
│  ├─ demo/              the 25-chapter engine reel        (id 777)
│  ├─ demo_plus/         C++-engine smoke test             (id 778)
│  ├─ skylance/          Sky Lance, a complete shooter     (id 779)
│  ├─ helloworld/        minimal FIX text + one sample     (id 772)
│  ├─ tutorial/          minimal engine loop               (id 555)
│  └─ neogeogame/        empty starting point              (id 775)
├─ artbox/               PNG → sprite / FIX / C-ROM pipeline + Artbox Studio
├─ sound/                YM2610 stack: Z80 driver, MML, FM, SSG, samples
├─ docs/                 the documentation set
├─ tools/                validators, the API generator, the manual builder
├─ dist/ roms/ hash_eagle/   build output and MAME hash sets
├─ Makefile              Linux + WSL build
└─ MakefileWin32.mak     native Windows build
```

**Top-level entry** `games/<name>/main.c` and `user.c` — the 68000-side game
flow and the BIOS-hook startup handlers. `main.c` is the file you edit;
`user.c` you almost never touch.

**Asset pipelines** `artbox/` and `sound/` — two parallel pipelines: PNG →
sprite C-ROMs and FIX, plus WAV and MML → ADPCM and driver tables.

**Output target** `roms/<game>/` — generated ROM sets kept in-tree so MAME
test runs are instant.

---

## 11 / BUILD — End-to-end pipeline

### Five stages. One `make all`.

| Stage | Target | Produces |
|---|---|---|
| 01 | `art` | Sprite C-ROMs, `screens.c`, `sprite_meta.h`, palettes |
| 02 | `sfix` | The S1 FIX ROM, 128 KB |
| 03 | `sound` | ADPCM samples, V-ROM, and the M1 Z80 driver ROM |
| 04 | `p1` | The 68000 program ROM, 512 KB |
| 05 | *(in `p1`)* | `hash_eagle/<game>/neogeo.xml` — the MAME hash set |

```sh
make GAME=<name> GAME_CFG_FILE=games/<name>/game.cfg all
make GAME=<name> GAME_CFG_FILE=games/<name>/game.cfg test
```

Every game carries its own `game.cfg`, and the build cross-checks `GAME=`
against it — which is what stops you silently building one game's art into
another game's ROM.

In day-to-day work you run `p1` almost exclusively. It takes seconds.

---

## 12 / TOOLCHAINS

| Component | Tool |
|---|---|
| 68000 C / C++ | `m68k-unknown-elf-gcc`, freestanding, `-march=68000` |
| 68000 link | `m68k-unknown-elf-ld` with a per-game `neogeo.ld` |
| ROM shaping | `objcopy` → `srec_cat` crop / byte-swap / pad |
| Z80 sound driver | `wla-z80` + `wlalink` |
| Asset conversion | Python 3.10+, Pillow, NumPy |
| Desktop tools | PyQt6 (Artbox Studio, Sound Studio) |
| Test loop | MAME with a generated hash set |
| Debug | GDB, MAME debugger, MAME Lua scripting |

C++ builds use `-std=c++14 -fno-exceptions -fno-rtti
-fno-threadsafe-statics`. No standard library, no `malloc`, no floating
point, no order-dependent global constructors.

---

## 13 / TARGETS — What you can build

```text
make all             art + sfix + sound + p1
make p1              68000 program ROM only
make aes / mvs       force the AES or MVS platform variant
make art             rebuild sprite C-ROMs from artbox
make art-crt         same, through the CRT-preprocessing pipeline
make sfix / srom     rebuild the S1 FIX ROM
make sound           samples + vrom + fmpatches + fm + mml + ssgconfig + ssg + m1rom
make m1rom           the shipping ASM Z80 driver build
make m1rom-c         the experimental C driver build
make compare-driver  build both and diff the ROM images
make dist            package dist/roms/<game>.zip
make test            run in MAME (MVS, sp-s2.sp1 BIOS)
make test-aes        run in MAME (AES, unibios22)
make debug           run MAME with the debugger attached
make debug-build     P1 with debug symbols and a linker map
make gdb / gdb-trace / gdb-remote
make bios-list / games-list / menu / help
make clean / sound-clean / art-clean / clean-all
```

---

# PART TWO — For the programmer

## 14 / THE GAME CONTRACT

A Neo Geo cartridge boots into the BIOS, not into `main()`. The BIOS calls
back for power-on, eye-catcher, game and title; `user.c` provides those
hooks. What you write is two functions:

```c
void game_boot(void);    /* once, after the BIOS hands over */
void game_frame(void);   /* once per frame, forever */
```

`game_frame()` returns every frame, so anything that must persist is
`static` or file-scope. A plain local is gone before the next call — the
most common first bug on this platform.

---

## 15 / THE 2D ENGINE

The SDK ships the same engine twice: plain C in `sdk/2d_engine/`, C++14 in
`sdk/2d_engine_plus/`. Same design, same public `extern "C"` ABI, so a C
game can link the C++ build and vice versa. One engine per ROM, selected
with `USE_2D_PLUS=1`.

| Module | Header | What it does |
|---|---|---|
| Frame tick | `ng_game_interupt.h` | The whole engine tick, with five hook points |
| Characters | `ng_chars.h` | 64-object pool: movement, animation, hitboxes, damage |
| Actions | `ng_actions.h` | Table-driven action scripts with sound and FX hooks |
| NPCs | `ng_npcs.h` | Patrol bounds, home position, think callbacks |
| Physics | `ng_physics.h` | Gravity, drag, solid rectangles, grounded tests |
| Level | `ng_level.h` | World bounds, scroll, camera helpers |
| Camera | `ng_camera.h` | Follow, dead zone, look-ahead, shake, cinematic pan |
| Render queue | `ng_render_queue.h` | 128-slot VBlank-safe deferred VRAM/palette writes |
| Sprite groups | `ng_sprite_group.h` | Dirty-flag sticky-bit chains — write only what changed |
| Sprite pool | `ng_sprite_pool.h` | The hardware slot map |
| FIX | `ng_fix.h` | Text layer with a dirty-cell cache |
| Backgrounds | `ng_bg.h` | Two scrolling sprite layers with parallax factors |
| Palette FX | `ng_palette_fx.h` | Fade, flash, pulse, colour cycle — all queue-safe |
| Particles | `ng_particles.h` | 32-slot fixed pool, typed spawns, priority eviction |
| Feedback | `ng_feedback.h` | Hitstop + shake + flash + sound hook in one call |
| Depth FX | `ng_depthfx.h` | Perspective projection, Z → shrink, fog, starfields |
| Joystick | `ng_joystick.h` | Edge detection, repeat, motion specials |
| Fixed point | `ng_fixed.h` | 16.16 arithmetic, sin/cos and shrink tables |
| Debug | `ng_debug.h` | FIX-layer performance overlay |

Full references: `docs/API_2D_ENGINE_C.md`, `docs/API_2D_ENGINE_CPP.md`.

---

## 16 / GRAPHICS PIPELINE — Artbox

Drop PNGs into `games/<game>/artbox/in/<category>/`, run `make art`. The
pipeline quantises to the Neo Geo colour lattice, derives palettes, packs
tiles into C-ROMs, and generates `screens.c` (one `showScreenN()` per asset)
and `sprite_meta.h` (geometry per asset).

### The v1.7.0 quantiser

Four ideas, and together they took mean colour error from dE 9.10 to 8.33
and p95 from 18.19 to 15.66 across the reference assets, with measured
dither speckle down a third.

**Match in Lab, not RGB.** Palette selection, k-means refinement, and the
final per-pixel remap all run in CIE-Lab through a precomputed 32³ lattice
LUT. Matching in one space and measuring in another is what makes a
quantiser that scores well look wrong.

**Refit the palette onto the hardware grid, then re-optimise.** k-means
picks colours in continuous space; snapping them to the 32-level lattice
moves every centroid. Lloyd iterations with the snap *inside* the update
step, keeping a move only when total error drops, and re-seeding duplicate
or unowned slots from the worst-served pixels.

**Blue-noise dither, and only where it helps.** A 32 × 32 void-and-cluster
mask orders the mix between the two nearest palette entries along a
serpentine scan. A dead band skips the mix entirely within 18 % of either
endpoint — those are the pixels where dithering adds speckle without adding
colour.

**Push contrast and saturation before clustering, with a soft knee.**
Fifteen colours pull everything toward the middle of the gamut, because the
average of a cluster is always less saturated than its members. Compensate
before clustering, and compress the extremes with `tanh` rather than
clipping — clipping folds every boosted highlight onto pure white, throwing
colour detail away before k-means ever sees it.

### Ordering

Categories are processed as `backgrounds`, `characters`, `effects`,
`eyecatcher`, `npc`, `screens`, `titles`, then any other directory
alphabetically. **An asset id is its 1-based position in that sequence** —
inserting a file in the middle renumbers everything after it.

---

## 17 / AUDIO — The stack

```
   68000  ──write one byte──▶  $320000  ──NMI $0066──▶   Z80
                                                          │
                                       Timer B IRQ $0038 ──┤ music engine tick
                                                          ▼
                                                      YM2610
                                          FM · SSG · ADPCM-A · ADPCM-B
```

| Area | Channels | Use |
|---|---|---|
| ADPCM-A | 6, one-shot | Short sound effects, voice samples |
| ADPCM-B | 1 stream | Backing tracks, ambience, long cues |
| SSG | 3 square waves | MML melody, plus a standalone sequencer |
| FM | Compiled sequencer, 16-patch bank | Music, stings, formant effects |

### Content pipelines

| Source | Compiler | Target |
|---|---|---|
| `sound/mml/*.mml` | `mml_compile.py` | `mml` |
| `sound/fm/*.fm` | `fm_compile.py` | `fm` |
| `sound/fm/patches.fm` | `fm_patch_compile.py` | `fmpatches` |
| `sound/ssg/*` | `ssg_compile.py` | `ssg` |
| `sound/samples/in_wav_a/*.wav` | `adpcm_enc.py` | `samples` |
| `sound/samples/in_wav_b/*.wav` | `adpcm_enc.py` | `samples` |
| voice samples | `vrom.py` | `vrom` |
| all of the above | `wla-z80` + `wlalink` | `m1rom` |

Full reference: `docs/SOUND_DRIVER.md`.

---

## 18 / THE SOUND DRIVER

`soundInit()` once at boot, then named calls with ids from
`sdk/sound_ids.h`:

```c
playMusic(SOUND_MUSIC_E);     playSFX(SOUND_SFX_7);
playSFXB(track);              playFMTrack(SOUND_FM_A);
playSSGTrack(SOUND_SSG_B);    soundSetSSGPreset(3);
soundFadeOut();               soundSceneReset();
speakText("READY!");
```

Beyond playback the driver exposes chip-level control — ADPCM-B pan
(reg `$11`), FM LFO (reg `$22`), SSG noise period (reg `$06`) — and CSM,
Composite Sine Mode, which auto-keys FM channel 2 from Timer A for formant
and vowel-slide effects.

**Practices that keep audio stable:** initialise once; use
`soundSceneReset()` on scene change rather than stacking stops; space
multi-byte commands with `waitVbl()`; fade before you stop; and budget
ADPCM-A effects the way you budget sprites — there are six channels and a
seventh trigger steals one.

---

## 19 / THE FRAME BUDGET

| Resource | Limit |
|---|---|
| CPU | ~12 MHz 68000, ~200,000 cycles per frame |
| VBlank | ~3 ms — all VRAM writes must fit |
| Sprites | 380 total, **96 per scanline** |
| Palettes | 16 entries per bank, 256 banks |
| FIX | 40 × 32 cells, 28 rows visible, 16 palettes |

**No `malloc` during gameplay.** Every engine subsystem uses a fixed pool:
characters 64, NPCs 32, particles 32, palette FX 8, render queue 128.

**No `float` or `double`.** There is no FPU; every float costs hundreds of
cycles in software emulation. Use 16.16 fixed-point.

**No division in the frame loop.** `DIVS`/`DIVU` cost 76–158 cycles each.
Use `ng_sin_tab[]`, `ng_cos_tab[]`, `ng_shrink_tab[]`, and right shifts.

**Write only what changed.** Sprite groups carry dirty flags; the FIX layer
has a dirty-cell cache.

**Queue every VRAM write.** Post to the render queue from game logic; let
`ng_render_queue_flush()` drain it in VBlank.

---

## 20 / THE SHIPPED GAMES

| Game | Id | Engine | What it is |
|---|---|---|---|
| `demo` | 777 | C | The 25-chapter engine reel |
| `demo_plus` | 778 | C++ | The same engine through the C++ API |
| `skylance` | 779 | C | Sky Lance — a complete vertical shooter |
| `helloworld` | 772 | — | FIX text and one sample; the tutorial target |
| `tutorial` | 555 | C | The minimal engine loop |
| `neogeogame` | 775 | C | An empty starting point |

### Sky Lance

Three pilots — ROOK (BLUE LANCE), KIRA (GREEN WING), BLAZE (RED STRIKER) —
differentiated by fire rate, speed and single vs. twin bolt rather than by
damage numbers. Seven stages, each running 5–7 squadrons before its boss:
CRIMSON KEEP, IRON TIDE, SOL CORE, NIGHT RAZOR, ROTOR NEST, EARTH HAMMER,
SPIRE GOD.

Two hardware lessons are visible in its source: the playfield is the 256 px
column the backdrop covers, not the full 320 px screen, because a page is
16 × 16 tiles; and the backdrops had to be authored to tile seamlessly,
because the sprite chip can shrink but never stretch.

### The demo reel

25 chapters, each isolating one subsystem, with the chapter number printed
top-right so a problem can be reported by number. **A** advances, **C**
restarts.

`BOOT · TITLE · FIX LAYER · FIX FX · SPRITE SCREENS · CHARACTERS · CHAR
SELECT · PHYSICS · CAMERA LAB · PALETTE FX · PARTICLES · PARTICLE LOAD ·
FEEDBACK · DEPTH FX · DEPTH PARALLAX · NPCS · MINI-GAME · JOYSTICK · SCROLL
LEVEL · CHAR 2D · TARGET RANGE · DEPTH RIDE · SOUND · SKY LANCE · CREDITS`

---

## 21 / SETUP

```sh
# 1. install the toolchain
install/install-linux.sh          # or install-ubuntu.sh / install-wsl.bat
                                  # or install-windows.bat

# 2. check it
m68k-unknown-elf-gcc --version
wla-z80 --version
python3 --version                 # 3.10+, with Pillow and NumPy

# 3. build and run
make GAME=helloworld GAME_CFG_FILE=games/helloworld/game.cfg all
make GAME=helloworld GAME_CFG_FILE=games/helloworld/game.cfg test
```

Set `SDKHOME` to the checkout. See `docs/DEPENDENCIES.md` and the
installation sections of `README.md` — including the WSL2 + Ubuntu 24.04 +
PulseAudio walkthrough for working audio under WSL.

---

## 22 / COMMON SYMPTOMS

| Symptom | Cause |
|---|---|
| Sprite invisible behind the background | Slot priority — HIGHER is in front |
| Sprite too large, cannot shrink further | SCB2 only reduces; re-author the art |
| Backdrop colour ignored | Wrote `$402000` instead of `$401FFE` |
| Text draws as solid blocks | The BIOS space glyph is opaque; use `ng_fix_blank_cell()` |
| FIX palette wraps onto text colours | Only 4 palette bits — fold duplicates below 16 banks |
| Pool fills and spawns stop | Objects marked destroyed but never `ng_chars_remove()`d |
| Loop skips the last objects | `ng_chars_count()` used as an array bound |
| Second byte of a sound command lost | Prefix and parameter not spaced by `waitVbl()` |
| Sprites dropped along one row | The 96-per-scanline limit |
| Green self-test screen on boot | The BIOS self-test, not your ROM — try `BIOS=unibios40` |
| Tearing or corrupt tiles | VRAM written outside VBlank instead of queued |

---

## 23 / DOCUMENTATION

| Document | For |
|---|---|
| `docs/INTRODUCTION.md` | What the SDK is, what the machine is, the reading order |
| `docs/HELLO_WORLD.md` | Your first ROM, in ten minutes |
| `docs/PROGRAMMERS_MANUAL.md` | Build, layout, engine tick, assets, frame budget |
| `docs/API_2D_ENGINE_C.md` | Every call in the C 2D engine |
| `docs/API_2D_ENGINE_CPP.md` | The C++14 build of the same engine |
| `SDK_API_GUIDE.md` | The bare-metal 68000 helpers |
| `docs/SOUND_DRIVER.md` | The Z80 driver, the YM2610, the audio pipelines |
| `docs/ARTBOX_PIPELINE.md` | PNG to C-ROM, the quantiser, Artbox Studio |
| `docs/GAMES.md` | What each shipped game demonstrates |
| `docs/DEMO_CHAPTERS.md` | The 25 demo chapters |
| `docs/TOOLS.md` | Every helper script — art, sound, verification, launchers, GIMP plug-ins |
| `docs/ADDING_A_GAME.md` | Starting your own project |
| `docs/GAME_ENGINE_LAYER.md` | How the engine tick fits together |
| `docs/performance_rules.md` | Staying inside the frame |
| `docs/GDB_GUIDE.md` | Debugging on real timing |
| `docs/neogeosdk_v1.7.0_manual.pdf` | The printable manual and API reference |

---

## 24 / RELEASE — What v1.7.0 adds

v1.7.0 consolidates the whole `neo_universal_2d` line of work — 164 commits.
It is the largest release the SDK has had.

### The big pieces

| Area | What arrived |
|---|---|
| **2D engine** | 35 modules, built from nothing in thirteen staged passes: characters, actions, physics, NPCs, camera, level, particles, feedback, depth FX, palette FX, sprite groups, render queue, FIX layer, input, timers, events, fixed-point math |
| **C++14 engine** | The same engine with member methods and singletons, identical `extern "C"` ABI, selected with `USE_2D_PLUS=1` |
| **Supporting layers** | `sdk/ng_fix` (a standalone FIX SDK), plus `ng_audio`, `ng_scene`, `ng_show`, `ng_video`, `bsp` |
| **Multi-game builds** | Per-game id, ROM folder, artbox, sound tree, linker script and MAME hash set, with a `game.cfg` cross-check that stops one game's art building into another's ROM |
| **Windows** | Native `MakefileWin32.mak` builds, the shipped ROMs built from the Windows toolchain, CRLF churn stopped |
| **Installers** | One-shot setup for Linux, Ubuntu, native Windows and WSL |
| **Art pipeline** | Lab clustering, per-tile palettes, master sprite palettes, blue-noise dithering, halo removal, non-destructive screen fitting, CRT and HD alternative routes |
| **Audio** | Nine ADPCM-B beds, eight FM tracks, nine SSG tracks, a recorded voice bank, pan / LFO / noise / tempo / CSM control, and a working fade engine |
| **Games** | Sky Lance, the 25-chapter reel, `demo_plus`, and a working template set |
| **Desktop tools** | Artbox Studio and Sound Studio, both PyQt6 |
| **Documentation** | The set listed in section 23, including generated API references and this document |

### Engine correctness — found by observation

A long arc of hardware-behaviour fixes, each of which changed how the SDK
behaves rather than only how it reads.

- **Sprite teardown rewritten** — a full 64-word SCB1 wipe with blank tile
  `0xFFFF` (not tile 0, which may carry art), sprites parked off-screen, a
  two-tier teardown with a previous-strips tail clear, and per-window VRAM
  footprint tracking. Ghost sprites persisting across scene transitions were
  the visible symptom.
- **Sprite slot priority reversed** — HIGHER is in front. The earlier claim
  was the opposite, and backgrounds parked at "behind" slots were drawing
  over characters.
- **The backdrop register corrected** to `$401FFE`. `$402000` is a mirror
  that silently does nothing.
- **The FIX layer's visible row origin corrected** — 28 visible rows, visible
  row *y* is map row *y + 2*.
- **`REG_PALBANK0` / `REG_PALBANK1` address swap fixed.**
- **Per-axis camera dead zone fixed.**
- **`START_GAME` syncs to VBlank before clearing sprites**, removing the
  black-box flash on entry.
- **Weak fallbacks for `ng_screen_table[]` / `ng_screen_count`** so a game
  shipping no screen art can link at all.
- **The Z80 polling race fixed** — `soundCommand` called `isZ80Ready()`
  twice; the trailing call triggered an NMI whose handler cleared the sound
  port, so the 68000 polling loop read 0, re-triggered, and deadlocked
  permanently. A frozen white screen with no sound.

### One thing that was tried and removed

An SSG envelope and three-formant phoneme speech engine was built and
evaluated on hardware, then removed: a pure YM2610 SSG path cannot
synthesise intelligible speech regardless of driver tricks. Recorded ADPCM-A
samples are the shipping answer, and `speakText()` is built on them.

Full detail, subsystem by subsystem: `CHANGELOG.md`.

---

## 25 / END

**Neo Geo SDK v1.7.0 · Eagle Software · MIT**

Repository: https://github.com/eaglesoftware777/neogeosdk

The project is intended only for lawful homebrew development, technical
research, education, preservation-oriented study, or authorised consulting
work. It must not be used for unauthorised copying, distribution,
reproduction, sale, conversion, or operation of copyrighted commercial
software.
