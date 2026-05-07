# Neo Geo SDK

Neo Geo development kit for SNK hardware.

- Repository: https://github.com/eaglesoftware777/neogeosdk
- Current release target: `v1.2.0`
- Changelog: [`CHANGELOG.md`](./CHANGELOG.md)

A hardware-centered SDK for Neo Geo arcade and home systems, with direct 68000-side control over VRAM, palettes, sprites, DMA, FIX tiles, and a custom YM2610 sound stack driven by a Z80 sound driver.

## Release Assets

The `v1.2.0` release is expected to publish these assets:

- `x-tools.tar`  
  m68k cross-toolchain bundle used by the Linux build flow
- `neogeosdkv1.2.0.tar.gz`  
  source snapshot for the SDK
- `roms-ssideki-v1.2.0.tar.gz`  
  generated demo ROM set for quick MAME testing

Toolchain download link:

- `x-tools.tar`  
  `https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.2.0/x-tools.tar`

## Requirements

Linux:

- Ubuntu or another recent Linux distribution
- `mame`
- `srecord`
- `cmake`
- `build-essential`
- `python3`
- `python3-pip`
- `python3-numpy`
- `python3-pil`
- `sqlite3`
- `sox`
- `wla-z80` and `wlalink`

Windows:

- SysGCC `m68k-elf`
- `py`
- `sox`
- `wla-z80` and `wlalink`
- MAME

Python packages:

- `pypng`

## Installation on Linux

Install the base packages:

```bash
sudo apt-get update
sudo apt-get install mame srecord cmake build-essential python3 python3-pip python3-numpy python3-pil sqlite3 sox
pip3 install pypng
git clone https://github.com/vhelin/wla-dx
cd wla-dx
cmake .
make
sudo cp binaries/wla-z80 binaries/wlalink /usr/local/bin/
```

Directory layout:

```text
~/neogeosdk   -> this repository
~/x-tools     -> m68k cross compiler bundle
```

Set `SDKHOME` to the parent directory of both:

```bash
export SDKHOME=~
```

## Installation on Windows

Expected layout:

```text
C:\neogeo\neogeosdk
C:\neogeo\x-tools
```

The Win32 makefile assumes:

- `C:\SysGCC\m68k-elf\bin\m68k-elf-gcc.exe`
- `C:\SysGCC\m68k-elf\bin\m68k-elf-ld.exe`
- `C:\SysGCC\m68k-elf\bin\m68k-elf-objcopy.exe`

Also make sure these are callable from `PATH`:

- `py`
- `sox`
- `wla-z80`
- `wlalink`
- `mame`

You can set:

```bat
set SDKHOME=C:\neogeo
```

Then run the Win32 build with:

```bat
make -f MakefileWin32.mak all
```

## Quick Start

Linux:

```bash
make all
make test
```

Common partial builds:

```bash
make p1
make sound
make art
make sfix
make m1rom
make m1rom-c
make compare-driver
```

Windows:

```bat
make -f MakefileWin32.mak all
make -f MakefileWin32.mak sound
make -f MakefileWin32.mak p1
```

## Build Targets

Main targets from `Makefile` and `MakefileWin32.mak`:

```text
make all             : art + sfix + sound + p1
make p1              : build only the 68000 game ROM
make sound           : samples + vrom + fmpatches + fm + mml + ssgconfig + ssg + m1rom
make sound-all       : alias for make sound
make art             : rebuild sprite C ROMs from artbox
make sfix            : rebuild S1 FIX ROM
make srom            : alias for make sfix
make vrom            : rebuild V ROM from ADPCM assets
make m1rom           : authoritative ASM Z80 driver build
make m1rom-asm       : snapshot ASM M1 for compare flow
make m1rom-c         : build experimental C-driver M1
make compare-driver  : build ASM and C M1 ROMs and compare them
make fm              : compile FM MML data
make fmpatches       : compile FM patch table
make mml             : compile music MML data
make ssg             : compile standalone SSG tracks
make ssgconfig       : compile SSG preset configuration
make samples         : convert WAV sources to ADPCM-A / ADPCM-B
make clean           : remove P1/game build outputs
make sound-clean     : remove sound build outputs
make art-clean       : remove artbox-generated outputs
make clean-all       : full clean
make test            : run the generated ROM set in MAME
make debug           : run MAME with debugger
```

Important recent build behavior:

- `make sound`, `make vrom`, and `make m1rom` sync generated outputs into `roms/ssideki/`
- both Linux and Windows makefiles keep `052-m1.m1`, `052-v1.v1`, `052-p1.p1`, and `sm1.sm1` aligned with the current build
- the Win32 flow now matches the Linux sound pipeline much more closely

## Artbox Graphics Pipeline

The Artbox pipeline converts PNG graphics into Neo Geo sprite and FIX formats.

Core tools:

- `artbox/img2neo.py`
- `artbox/romtiles.py`
- `artbox/fixtiles.py`
- `artbox/romdbfiximport.py`
- `artbox/createromdb.py`

Inputs:

- `artbox/in/` for sprite source PNGs
- `artbox/infix/` for FIX-layer source PNGs

Outputs:

- generated C ROMs
- generated S1 FIX ROM

## Sound Subsystem (YM2610)

The SDK includes a custom Z80 sound system with these layers:

- ADPCM-A for short sound effects and voice cues
- ADPCM-B for long beds, ambience, and backing layers
- SSG/MML for square-wave melody sequencing
- FM sequencing with compiled patch and track data

Key sound-side paths:

- authoritative Z80 driver: `sound/driver/driver.asm`
- experimental C-driver port: `sound/driver/driver.c`
- experimental Z80 C compiler: `z80c-special/`
- generated sound tables:
  - `sound/driver/music_data.inc`
  - `sound/driver/fm_data.inc`
  - `sound/driver/fm_patch_table.inc`
  - `sound/driver/ssg_data.inc`
  - `sound/driver/ssg_config.inc`
  - `sound/driver/sample_table.inc`

68k-side sound API highlights:

- `soundSceneReset()`
- `soundPlayTitleMusic(track)`
- `soundPlayGameLoop(track)`
- `soundPlayDemoFM(track)`
- `playMusic(track)`
- `playSFX(sample)`
- `playSFXB(sample)`
- `playFMTrack(track)`
- `playSSGTrack(track)`

Named sound resources now live in:

- `sdk/sound_ids.h`

Detailed sound usage:

- [`sound/SOUND_DRIVER_GUIDE.txt`](./sound/SOUND_DRIVER_GUIDE.txt)
- [`sound/driver/readme`](./sound/driver/readme)
- [`sound/mml/readme`](./sound/mml/readme)

## Recent Changes Since 2025

Highlights from the recent commit line:

- 2025-11-29  
  README refresh and project structure cleanup
- 2026-04-26  
  artbox pipeline migrated to Python 3 and installation docs updated
- 2026-05-04  
  custom Neo Geo sound system added:
  - Z80 YM2610 driver
  - MML / FM / SSG build chain
  - cross-platform Python ADPCM encoder
  - Windows sound build parity
- 2026-05-05  
  experimental C migration of the Z80 sound driver and `z80c-special` compiler work
- 2026-05-06  
  higher-level sound workflow, named sound IDs, improved multi-layer demo mix, and compare flow for ASM vs C M1 builds

See [`CHANGELOG.md`](./CHANGELOG.md) for release-level notes.

## Repository Overview

```text
CHANGELOG.md      — release notes and version history
main.c            — demo/game presentation flow on 68000 side
user.c            — Neo Geo BIOS hook handlers and startup flow
sdk/              — headers, linker scripts, support library
  sound_ids.h     — named sound IDs for music, SFX, beds, FM, and SSG tracks
artbox/           — graphics conversion pipeline
sound/            — sound driver, tracks, samples, tools
  driver/         — ASM driver, experimental C driver, generated tables
  fm/             — FM tracks and patch bank
  mml/            — main music tracks
  ssg/            — standalone SSG tracks and presets
  samples/        — raw and converted sample assets
  tools/          — sound build utilities
roms/ssideki/     — synced ROM outputs for MAME
out/              — intermediate and generated build artifacts
win/              — Windows-side helper binaries used by the build
z80c-special/     — experimental Z80 C compiler used by the C-driver path
```

## Notes

- the authoritative playable sound driver remains `sound/driver/driver.asm`
- the experimental C-driver path is built for comparison and incremental migration work
- release assets include generated ROM data because this repository tracks and tests them directly
