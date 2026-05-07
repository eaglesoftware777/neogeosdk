# Neo Geo SDK

Neo Geo development kit for SNK hardware.

- Repository: https://github.com/eaglesoftware777/neogeosdk
- Current release target: `v1.2.0`
- Changelog: [`CHANGELOG.md`](./CHANGELOG.md)
- SDK API guide: [`SDK_API_GUIDE.md`](./SDK_API_GUIDE.md)

A hardware-centered SDK for Neo Geo arcade and home systems, with direct 68000-side control over VRAM, palettes, sprites, DMA, FIX tiles, and a custom YM2610 sound stack driven by a Z80 sound driver.

SNK's Neo Geo sits in a very specific development world: cartridge-era arcade
hardware built around a Motorola 68000 main CPU, a Z80 sound subsystem, and the
YM2610 for FM, SSG, and sampled audio. The machine is close to the metal by
design. Graphics, palettes, FIX tiles, ROM layout, and sound commands are all
part of the day-to-day programming model rather than hidden behind a large engine
layer.

This SDK is intended for self-made Neo Geo games, demos, experiments, and
homebrew projects that want to stay close to original Neo Geo development
practice while still using modern build tooling, scripting, and emulator-based
test loops. The goal is not to abstract the hardware away. The goal is to make
real Neo Geo development practical on current Linux, WSL, and Windows setups.

## Release Assets

The `v1.2.0` release publishes these attached assets:

- `neogeosdkv1.2.0.tar.gz`  
  source snapshot for the SDK
- `roms-ssideki-v1.2.0.tar.gz`  
  generated demo ROM set for quick MAME testing

The release page also carries `x-tools.tar` for the Linux toolchain layout used by
the default `Makefile`. That asset is kept as-is when documentation-only or ROM-only
release updates are published.

Current release page:

- `https://github.com/eaglesoftware777/neogeosdk/releases/tag/v1.2.0`

## Requirements

Linux:

- Ubuntu or another recent Linux distribution
- `git`
- `mame`
- `srecord`
- `cmake`
- `build-essential`
- `python3`
- `python3-pip`
- `python3-numpy`
- `python3-pil`
- `sqlite3`
- `wla-z80` and `wlalink`

Optional on Linux:

- `sox`

Windows:

- a Windows `m68k-elf` GCC toolchain
- GNU Make
- `py`
- `wla-z80` and `wlalink`
- MAME

Optional on Windows:

- `sox`

Python packages:

- `numpy`
- `Pillow`
- `pypng`

`sox` is optional on both platforms. If it is not installed, `make samples` falls
back to the bundled `sound/tools/wav_to_raw_pcm.py` converter. No separate Python
`sox` module is required.

## Toolchain Notes

Linux 68000 compiler:

- the default Linux `Makefile` expects an `x-tools/` directory under `SDKHOME`
- the current release page includes `x-tools.tar`
- after extraction, the compiler should exist at:
  - `$(SDKHOME)/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-gcc`

Windows 68000 compiler:

- the Win32 makefile expects a Windows `m68k-elf` GCC toolchain
- the tested default is a SysGCC-style layout under:
  - `C:\SysGCC\m68k-elf`
- if your toolchain is installed elsewhere, set `M68K_ELF_ROOT` when invoking `make`
- you do not need to clone the SDK into a fixed drive or fixed folder name beyond
  keeping `neogeosdk/` under the chosen `SDKHOME` parent

## SDKHOME Layout

Both makefiles expect `SDKHOME` to point to the directory that contains both
`neogeosdk/` and `x-tools/`.

Expected layout:

```text
SDKHOME/
  neogeosdk/
  x-tools/
```

If `SDKHOME` is not set, both makefiles default to the parent directory of the
repository checkout. That works only when the repository is already laid out as
`<sdk root>/neogeosdk`.

Examples:

```bash
export SDKHOME=$HOME/neogeo
export SDKHOME=/opt/neogeo
export SDKHOME=/mnt/c/neogeo
```

```bat
set SDKHOME=D:\projects\arcade
set SDKHOME=E:\homebrew\snk
```

## Installation on Linux

Install the base packages:

```bash
sudo apt-get update
sudo apt-get install git mame srecord cmake build-essential python3 python3-pip python3-numpy python3-pil sqlite3 sox
python3 -m pip install --user pypng
git clone https://github.com/vhelin/wla-dx
cd wla-dx
cmake -S . -B build
cmake --build build -j
sudo cp build/binaries/wla-z80 build/binaries/wlalink /usr/local/bin/
```

Recommended layout:

```text
$HOME/neogeo/neogeosdk   -> this repository
$HOME/neogeo/x-tools     -> m68k Linux cross compiler bundle
```

Install the Linux 68000 toolchain from the release asset so that `x-tools/` lands
next to the repository:

```bash
cd $HOME/neogeo
curl -L -o x-tools.tar https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.2.0/x-tools.tar
tar -xf x-tools.tar
```

After extraction, verify:

```bash
$HOME/neogeo/x-tools/m68k-unknown-elf/bin/m68k-unknown-elf-gcc --version
```

Set `SDKHOME` to the parent of both:

```bash
export SDKHOME=$HOME/neogeo
```

Build:

```bash
make all
make test
```

## Installation on WSL

Use the Linux `Makefile`, not `MakefileWin32.mak`.

WSL should use the Linux toolchain layout:

- `$(SDKHOME)/x-tools/...` for the `m68k-unknown-elf` binaries
- Linux `python3`
- Linux `wla-z80` / `wlalink`

Do not point the WSL build at the Windows SysGCC tree. `MakefileWin32.mak` is for
native Windows `cmd.exe` builds.

Typical WSL setup:

```bash
sudo apt-get update
sudo apt-get install git mame srecord cmake build-essential python3 python3-pip python3-numpy python3-pil sqlite3
python3 -m pip install --user pypng

mkdir -p $HOME/neogeo
cd $HOME/neogeo
git clone https://github.com/eaglesoftware777/neogeosdk.git
curl -L -o x-tools.tar https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.2.0/x-tools.tar
tar -xf x-tools.tar

export SDKHOME=$HOME/neogeo
cd $SDKHOME/neogeosdk
make all
```

If the checkout lives on `/mnt/c/...`, the same `SDKHOME` rule still applies:

```bash
export SDKHOME=/mnt/c/neogeo
cd /mnt/c/neogeo/neogeosdk
make sound
```

## Installation on Windows

Recommended layout:

```text
<sdk root>\neogeosdk
<sdk root>\x-tools
```

The Win32 makefile accepts a configurable `M68K_ELF_ROOT`. The tested default is:

- `C:\SysGCC\m68k-elf\bin\m68k-elf-gcc.exe`
- `C:\SysGCC\m68k-elf\bin\m68k-elf-ld.exe`
- `C:\SysGCC\m68k-elf\bin\m68k-elf-objcopy.exe`

If your Windows `m68k-elf` toolchain is installed somewhere else, pass its root:

```bat
make -f MakefileWin32.mak M68K_ELF_ROOT=D:\toolchains\m68k-elf all
```

Also make sure these are callable from `PATH`:

- `py`
- `make`
- `wla-z80`
- `wlalink`
- `mame`

Install the Python packages once:

```bat
py -m pip install numpy pillow pypng
```

Install Python itself from python.org or the Microsoft Store so that the `py`
launcher is available. Install a Windows `m68k-elf` GCC toolchain such as a
SysGCC-style package, then confirm:

```bat
C:\path\to\m68k-elf\bin\m68k-elf-gcc.exe --version
```

You also need a GNU Make binary on `PATH`. If your Windows toolchain bundle ships
`make.exe`, that is fine. Otherwise install any compatible GNU Make and verify:

```bat
make --version
```

Install or build WLA-DX for Windows and make sure both tools are on `PATH`:

```bat
wla-z80 --version
wlalink --version
```

Install MAME and make sure `mame.exe` is on `PATH`:

```bat
mame -help
```

`sox` is optional. If it is present in `PATH`, the sound pipeline uses it. If it is
missing, the bundled Python fallback is used automatically.

Set `SDKHOME` to the parent directory that contains your cloned `neogeosdk`
directory and the sibling `x-tools` directory:

```bat
set SDKHOME=D:\projects\arcade
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
make -f MakefileWin32.mak M68K_ELF_ROOT=D:\toolchains\m68k-elf all
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
- Windows `make samples` works with either SoX or the bundled Python WAV converter
- Windows `make fm`, `make mml`, and `make ssg` expand source file lists correctly
- Windows `make sfix` now keeps `052-s1.s1` in the correct 128 KB FIX-ROM format

## Sound Build Modes

There are three distinct Z80 sound-driver build flows:

1. Default playable driver (`driver.asm`)

```bash
make m1rom
make sound
```

```bat
make -f MakefileWin32.mak m1rom
make -f MakefileWin32.mak sound
```

This is the authoritative runtime path used for the generated playable `M1`.

2. Experimental C compilation with the final playable `M1` still linked from `driver.asm`

```bash
make m1rom USE_Z80C=1
```

```bat
make -f MakefileWin32.mak m1rom USE_Z80C=1
```

This compiles the experimental `sound/driver/driver.c` flow for comparison work but
still ships the assembler runtime in the final output.

3. Experimental C-linked runtime

```bash
make m1rom-c
make compare-driver
```

```bat
make -f MakefileWin32.mak m1rom-c
make -f MakefileWin32.mak compare-driver
```

This path links the experimental C runtime into the output `M1`. Use it for
comparison and migration work, not as the default release path.

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

General SDK usage:

- [`SDK_API_GUIDE.md`](./SDK_API_GUIDE.md)
- GitHub wiki:
  - `Build-and-Installation`
  - `Home`
  - `SDK-Library-Reference`
  - `Sound-System-Guide`

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
- 2026-05-07
  softfloat removal, SDK API docs refresh, Windows sound-build parity fixes, and corrected Win32 FIX-ROM generation

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
- `x-tools.tar` remains on the release page for the Linux toolchain layout, but it
  is not refreshed by every documentation or ROM update
