# Changelog

## v1.3.0 - Bug Fixes, Per-Game ROM Folders, Z80 Sound Fix, and Demo Overhaul

Release date: 2026-05-17

### Highlights

- 15 SDK bug fixes across sprite, FIX, sound, and build subsystems
- Per-game ROM output folders (`roms/<game>/`) — all ROM files now isolated per game
- FIX layer hardware bug fix: `clearFix()` now restores BRDFIX so game S ROM is always selected after a clear
- Z80 communication race condition fixed: `soundCommand` no longer hangs the 68k in polling loop
- Sound driver restored to last known-good assembly version (1947-line working driver)
- Demo fully rewritten as a cinematic 13-scene SDK showcase
- Windows build path bugs corrected across all sound and FIX tools
- Multiple-definition linker errors from scene file additions resolved

### Bug Fixes

#### FIX Layer — BRDFIX Register Not Restored After `clearFix()`

`clearFix()` calls the BIOS routine `SYS_FIX_CLEAR` which resets the `BRDFIX`
register (`$3A0003` bit 0) to 0, switching hardware to the BIOS S ROM.  All text
drawn afterward used wrong tile indices and appeared blank or black.

Fix: `clearFix()` in `sdk/neogeolib.c` now executes `BSET.B #0,REG_BRDFIX`
immediately after the BIOS call to restore the game S ROM.  All callers —
including `ngfix_clear()` — benefit automatically.  The redundant `setsfix()`
that was in `ngfix_clear()` has been removed.

#### Z80 Sound — `soundCommand` Polling Race Condition

`soundCommand` called `isZ80Ready()` twice: once before writing the command and
once after.  The trailing call wrote 0 to `$300001` (REG_DIPSW), triggering a Z80
NMI.  The NMI handler clears `$320000` (REG_SOUND) to 0 while processing the
previous command.  The 68k polling loop then reads 0, re-triggers another NMI, and
the cycle deadlocks permanently — producing a frozen white screen with no sound.

Fix: the trailing `isZ80Ready()` has been removed from `soundCommand` in
`sdk/neogeolib.c`.

#### Z80 Driver — Restored Working Assembly Driver

The M1 ROM was being built from an updated 2301-line `driver.asm` that diverged
from the last verified-working version (1947 lines).  `sound/driver/driver.asm`
has been restored to the working version and the M1 ROM rebuilt via the pure
assembler path (`USE_Z80C=0`).

#### Particle System — Sprite Budget Reporting and Stale Slot Cleanup

- Particle sprite budget counter now correctly reports actual slots consumed.
- Stale particle sprite slots are cleared when particles expire, preventing
  ghost sprites from persisting across scene transitions.
- Demo sprite cleanup limited to valid slot range to avoid out-of-bounds writes.

#### Linker — Multiple-Definition Errors When Adding Scene Files

Adding `demo_screen.c` to the demo scene list caused multiple-definition linker
errors for symbols shared across scene files.  Fixed by correcting the `GAME_SCENES`
list to exclude files that are `#include`-d by other scenes.

### Per-Game ROM Folders

ROM output files are now written to `roms/<game>/` instead of a shared `roms/`
root.  The `make p1`, `make sound`, `make art`, and `make sfix` targets all sync
their outputs to the per-game folder.

```text
roms/demo/       777-p1.p1  777-m1.m1  777-s1.s1  777-v1.v1  777-c1.c1  777-c2.c2
roms/helloworld/ 772-p1.p1  772-m1.m1  ...
roms/tutorial/   555-p1.p1  555-m1.m1  ...
```

`hash_eagle/<game>/neogeo.xml` is regenerated automatically on every `make p1`.

### Windows Build Fixes

- `MakefileWin32.mak`: `SHELL=cmd.exe` forced to prevent sh-style path expansion.
- `romts.bat` / `romfx.bat`: `GAME_ID` filenames now quoted to prevent romtool from
  treating hyphens as flags.
- `GAME_SOUND` path backslash-corrected; trailing space in `sfix GAME_ID` removed.
- All game linker scripts gain catch-all `.data` / `.bss` patterns for Windows `ld`
  compatibility.
- Per-game `GAME_ID` and `GAME_SOUND` propagated to all artbox and sound tools.

### Demo Overhaul

The demo game (`games/demo`) has been rewritten as a cinematic 13-scene SDK
showcase covering: intro, sprites, camera, palette FX, particles, depth FX, FIX
layer, sound, combat, stress test, title, render queue, and a 3D starfield scene.
Each scene exercises a distinct engine subsystem with timed transitions.

### Documentation

- `BUGFIX_SESSION.md` added at repository root — full root-cause analysis and fix
  description for the frozen-screen regression.
- `docs/ADDING_A_GAME.md` updated with per-game ROM folder layout.

### Validation

- `make GAME=demo all` — zero errors, zero warnings
- `make GAME=helloworld all`
- `make GAME=tutorial game`
- `make GAME=neogeogame game`
- `make m1rom-asm` — assembler path, working driver
- `make -f MakefileWin32.mak sound`
- `make -f MakefileWin32.mak sfix`

---

## v1.2.0 - NeoGeo Deluxe 2D Engine, Multi-Game Build System, and Sound Pipeline

Release date: 2026-05-06

### Highlights

- Complete NeoGeo Deluxe 2D engine layer targeting huge animated characters, smooth camera, parallax depth, sprite-scaling, palette FX, particles, and stable 60 FPS — no float, no malloc during gameplay
- Multi-game build system: single repository, multiple independent games under `games/<name>/`
- Full per-game art, sound, and ROM pipeline on both Linux and Windows
- Refined YM2610 sound driver workflow on both the authoritative ASM path and the experimental C path
- Stable sound IDs for 68k-side code and improved demo/game loop mixing

### Multi-Game Build System

| Game | ID | Description |
|------|----|-------------|
| `games/demo` | 777 | Full SDK showcase (13 scenes, all engine features) |
| `games/helloworld` | 772 | Minimal FIX-text hello world |
| `games/tutorial` | 555 | Tutorial template with BIOS hooks |
| `games/neogeogame` | 775 | Blank template for new projects |

Build any game with `make GAME=<name>` (Linux) or `make -f MakefileWin32.mak GAME=<name>` (Windows).

- `Makefile` and `MakefileWin32.mak` accept `GAME=<name>` (default `demo`)
- `-include games/$(GAME)/game.mk` loads per-game `GAME_ID` and `GAME_SCENES`
- All ROM filenames derived from `$(GAME_ID)` — no hardcoded `777`
- Per-game source paths: `games/$(GAME)/user.c`, `main.c`, `eyecatcher.c`, `neogeo.ld`
- Per-game sound: `GAME_SOUND = games/$(GAME)/sound` — fm, mml, ssg, samples all per-game
- Per-game art: `artbox/makeartbox.sh GAME` / `artbox/makeartbox.bat GAME`
- `GAME_ID` and `GAME_SOUND` propagated to all artbox and sound tools (romts, romfx, fixtiles, vrom, m1rom)
- Games with no art receive a stub `showEyeCatcherMVS()` to avoid linker errors
- Empty sound/art folders handled gracefully — each target skips with a message
- `aes`, `mvs`, `test-aes`, `test-mvs`, `debug-aes`, `debug-build` targets forward `GAME=`

### Deluxe 2D Engine Layer

New engine modules under `sdk/2d_engine/`:

| Module | Files | What it does |
|--------|-------|-------------|
| Render queue | `ng_render_queue.h/.c` | 128-slot VBlank-safe deferred VRAM and palette write queue |
| Fixed-point math | `ng_fixed.h/.c` | 16.16 fixed-point, pre-baked sin/cos/shrink lookup tables |
| Camera | `ng_camera.h/.c` | Smooth follow, dead zone, look-ahead, shake, cinematic pan, border clamp |
| Palette FX | `ng_palette_fx.h/.c` | Fade, flash, pulse, color cycle — queue-safe |
| Particles | `ng_particles.h/.c` | 32-slot fixed pool, 8 particle types, priority-based eviction |
| Feedback | `ng_feedback.h/.c` | Hitstop + screen shake + palette flash + sound hook in one call |
| Depth FX | `ng_depthfx.h/.c` | NGVec3 perspective projection, Z→shrink lookup table, starfield |
| Debug HUD | `ng_debug.h/.c` | Fix-layer perf overlay (compile with `NG_DEBUG_PERF=1`) |

Updated engine modules:

- `ng_sprite_group`: dirty flags and `ng_sprite_group_flush()` — only changed SCB regions written per frame
- `ng_depthfx`: extended with `NGVec3`, full perspective projection, Z→fog palette, starfield advance
- `ng_engine.h`: aggregate include covers all subsystem headers

### Sound System

- Refined YM2610 sound driver workflow on ASM and experimental C-driver paths
- `sdk/sound_ids.h` — stable track / cue / bed / voice identifiers for 68k-side code
- `sound/driver/driver_defs.h` — named C-driver command and runtime constants
- Experimental C-driver: `driver.c`, `driver_prelude.asm`, `combine_split_driver.py`, `compare_m1.py`
- Compare flow: `make m1rom-asm`, `make m1rom-c`, `make compare-driver`
- Higher-level sound helpers and mix setup in `sdk/neogeolib.c`
- Improved demo/game loop mix layering: SSG, FM, ADPCM-B, and ADPCM-A accents
- `GAME_SOUND` env var passed through all sound tools for per-game audio isolation
- `enc_wave16le_a/b.sh` and `.bat` read WAV input from `$GAME_SOUND/samples/` instead of shared dir
- Linux and Windows makefiles support `SDKHOME`, WSL usage, SoX-optional sample conversion

### Artbox Pipeline

- Default fit mode changed from `crop` to `contain` — non-destructive asset scaling
- `gen_eyecatcher.py` updated with fast animation pacing (lead hold, 2× animation pass, final hold)
- Eyecatcher sprites positioned at hardware-accurate size and position
- `romdbimgimport.py` hardened with absolute paths and fail-fast error handling
- `makeartbox.sh` / `makeartbox.bat` use absolute-path resolution to avoid symlink nesting bugs
- Windows: `mklink /J` directory junctions — no administrator privileges required (Vista+)
- Windows path bug fixed: `REPO_DIR` now resolved with `for %%i ... %%~fi` instead of raw `..`

### Build System

- Makefile toolchain detection defaults to `x-tools-v2` with `x-tools` legacy fallback
- Linker flags corrected (`-nostdlib`, `-nostartfiles` removed)
- Win32 Makefile engine module list updated to match Linux (added `ng_render_queue`, `ng_fixed`, `ng_camera`, `ng_palette_fx`, `ng_particles`, `ng_feedback`, `ng_debug`, `ng_demo_advanced`)
- `MakefileWin32.mak`: `vrom` and `m1rom` targets now export `GAME_ID` and `GAME_SOUND` to environment
- FIX ROM path corrected — `sfix` target creates its own temporary `artbox/infix` symlink/junction
- Win32 sound/FIX path parity corrected

### Launcher Scripts

Both `neogeosdk.sh` (Linux) and `neogeosdk.bat` (Windows) gained a game-selection step (`g` key). The selected game is passed to every build command for the duration of the session.

### New Documentation

- `docs/sprite_groups.md`
- `docs/render_queue.md`
- `docs/camera.md`
- `docs/palette_fx.md`
- `docs/particles.md`
- `docs/depthfx.md`
- `docs/performance_rules.md`
- `docs/ARTBOX_PIPELINE.md`

### Validation

- `make all` (demo, default)
- `make GAME=helloworld all`
- `make GAME=tutorial game`
- `make GAME=neogeogame game`
- `make m1rom-c`
- `make -f MakefileWin32.mak sound`
- `make -f MakefileWin32.mak sfix`
- zero errors, zero warnings on all four games

---

## v1.1.0 - Sound System, Python 3 Migration & Build Tools

Release date: 2026-05-04

### Highlights

- added the custom Neo Geo YM2610 sound system
- migrated the artbox pipeline to Python 3
- replaced the legacy ADPCM encoder with a cross-platform Python implementation
- improved Windows parity across the build chain

### Included work

- Z80 YM2610 driver with FM, ADPCM-A, ADPCM-B, and SSG
- MML compilation pipeline for sound content
- Win32 sound build scripts
- updated installation and project documentation

## v1.0 - Initial Public Release

Release date: 2021-11-07

### Highlights

- initial Neo Geo SDK release
- toolchain asset publishing
- demo ROM and example project layout

## Recent Commit History Since 2025

- `bbf84b0` 2025-11-29 `Updated README.md to enhance clarity and structure`
- `d36fe14` 2026-04-26 `Migrate artbox pipeline from Python 2 to Python 3`
- `774171e` 2026-04-26 `Update README: install numpy via apt, pypng via pip3`
- `6c87ed2` 2026-04-26 `upgrade artbox to python3`
- `0ec1cd4` 2026-05-04 `Add complete NeoGeo sound system: Z80 driver, YM2610 FM/ADPCM/SSG, MML pipeline and ROM build chain`
- `79436bc` 2026-05-04 `Replace binary ADPCM encoder with cross-platform Python implementation and fix Win32 build scripts`
- `197b75b` 2026-05-04 `Update sound driver, SSG configuration, and project build artifacts`
- `e362f11` 2026-05-04 `Fine-tune sound initialization and track playback in main and demo loops`
- `fa8ca15` 2026-05-05 `feat: migrate Z80 sound driver to C and enhance z80cc compiler`
- `3b5c4d4` 2026-05-05 `feat: implement execute_command and fix compiler distance errors`
- `bfcaa20` 2026-05-06 `Refine sound driver workflow and demo mix`
- `7747dcd` 2026-05-07 `Remove softfloat runtime and refresh generated assets`
- `abaefbe` 2026-05-07 `Document SDK API and clean up source formatting`
- `HEAD` 2026-05-07 `Refresh install/build docs and fix Win32 sound/FIX parity`
