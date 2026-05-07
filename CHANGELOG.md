# Changelog

## v1.2.0 - C Driver Workflow, Layered Demo Mix, and Release Refresh

Release date: 2026-05-06

### Highlights

- refined the YM2610 sound-driver workflow on both the authoritative ASM path and the experimental C-driver path
- added stable named sound IDs for 68k-side code
- improved the demo/game loop mix to layer SSG, FM, ADPCM-B, and short ADPCM-A accents more coherently
- expanded build and release documentation for Linux and Windows
- prepared release assets for source, ROMs, and toolchain distribution

### Included work

- higher-level sound helpers and mix setup in `sdk/neogeolib.c`
- `sdk/sound_ids.h` for stable track / cue / bed / voice identifiers
- `sound/driver/driver_defs.h` for named C-driver command and runtime constants
- experimental C-driver support files:
  - `sound/driver/driver.c`
  - `sound/driver/driver_prelude.asm`
  - `sound/driver/driver_prelude.inc`
  - `sound/tools/combine_split_driver.py`
  - `sound/tools/compare_m1.py`
- new compare flow:
  - `make m1rom-asm`
  - `make m1rom-c`
  - `make compare-driver`
- updated Linux and Windows makefiles so generated sound outputs are copied back into `roms/ssideki/`
- updated Linux and Windows makefiles to support `SDKHOME`, WSL usage, SoX-optional
  sample conversion, and Win32 wildcard expansion for FM/MML/SSG builds
- corrected the Win32 `sfix` path so `052-s1.s1` remains the proper 128 KB FIX ROM
- revised sound content and game-loop layering in:
  - `sound/mml/0_samurai_game_loop.mml`
  - `sound/fm/4_bass_motif.mml`
- refreshed top-level SDK, install, and sound-driver documentation

### Release asset notes

- refreshed release-facing source and ROM bundle documentation
- `x-tools.tar` remains on the release page but is intentionally not refreshed by
  this update path

### Validation

- `make p1`
- `make m1rom-c`
- `make m1rom USE_Z80C=0`
- `make -n -f MakefileWin32.mak sound`
- `make -n -f MakefileWin32.mak sfix`

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
