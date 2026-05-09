# Neo Geo SDK

Neo Geo development SDK for SNK hardware.

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

## 2D Game Engine Layer

The repository includes a reusable 2D game engine layer under `sdk/ng_*`.

It is a plain-C 2D engine built around:

- characters
- actions
- per-frame `game_engine_frame()`
- small `game_events`
- border constraints
- status flags
- timers
- progress counters
- a properties matrix

Important current state:

- the source files live in `sdk/ng_*.c` and `sdk/ng_*.h`
- the linker scripts reserve `game_engine_bss` for the engine state objects
- `Makefile` and `MakefileWin32.mak` compile and link the `sdk/ng_*` modules by default

Use these docs for the current integration path:

- [`docs/GAME_ENGINE_LAYER.md`](./docs/GAME_ENGINE_LAYER.md)
- [`docs/MAKEFILE_INTEGRATION.md`](./docs/MAKEFILE_INTEGRATION.md)


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

## Documentation

Primary repository docs:

- [`README.md`](./README.md)
- [`SDK_API_GUIDE.md`](./SDK_API_GUIDE.md)
- [`docs/GAME_ENGINE_LAYER.md`](./docs/GAME_ENGINE_LAYER.md)
- [`docs/MAKEFILE_INTEGRATION.md`](./docs/MAKEFILE_INTEGRATION.md)
- [`sound/SOUND_DRIVER_GUIDE.txt`](./sound/SOUND_DRIVER_GUIDE.txt)
- [`sound/driver/readme`](./sound/driver/readme)
- [`sound/mml/readme`](./sound/mml/readme)
- [`sound/ssg/readme`](./sound/ssg/readme)
- [`sound/samples/readme`](./sound/samples/readme)
- [`sound/samples/SOURCES.md`](./sound/samples/SOURCES.md)
- [`sound/tools/readme`](./sound/tools/readme)

Wiki pages:

- `Home`
- `Build-and-Installation`
- `SDK-Library-Reference`
- `Sound-System-Guide`
- `Game-Engine-Layer`

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
- `make samples` uses the bundled Python WAV converter by default on Linux and Windows
- set `SOX=/path/to/sox` only when you explicitly want the SoX conversion path
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

## Game Engine Build Integration

The 2D game engine layer in `sdk/ng_*` is already wired into the default
`game:` recipes in both `Makefile` and `MakefileWin32.mak`.

Those builds now:

- compile the `sdk/ng_*.c` modules into `out/ng_*0.o`
- link those objects into the main 68000 game binary
- place engine state in the linker-managed `game_engine_bss` region

See:

- [`docs/GAME_ENGINE_LAYER.md`](./docs/GAME_ENGINE_LAYER.md)
- [`docs/MAKEFILE_INTEGRATION.md`](./docs/MAKEFILE_INTEGRATION.md)

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

Current ADPCM-B theme mapping:

- `playSFXB(0)` / `SOUND_BED_TITLE_THEME`
  - title theme
- `playSFXB(1)` / `SOUND_BED_STAGE_ONE`
  - stage loop 1
- `playSFXB(2)` / `SOUND_BED_STAGE_TWO`
  - stage loop 2
- `playSFXB(3)` / `SOUND_BED_ENDING_THEME`
  - ending / results theme

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

Current live 68000-side flow:

- `COIN_SOUND()` uses an ADPCM-A coin chime
- `PLAYER_START()` uses an ADPCM-A start cue
- `soundPlayTitleMusic()` resets the scene, plays the title gong, then starts the ADPCM-B title theme
- `soundPlayGameLoop()` resets the scene and selects an ADPCM-B stage or ending bed directly
- `playMusic()` remains available for explicit MML/SSG playback, but the shipped title/game helpers now prefer direct ADPCM-B playback for cleaner live behavior

Detailed sound usage:

- [`sound/SOUND_DRIVER_GUIDE.txt`](./sound/SOUND_DRIVER_GUIDE.txt)
- [`sound/driver/readme`](./sound/driver/readme)
- [`sound/mml/readme`](./sound/mml/readme)

General SDK usage:

- [`SDK_API_GUIDE.md`](./SDK_API_GUIDE.md)
- GitHub wiki:
  - `Build-and-Installation`
  - `Home`
  - `Game-Engine-Layer`
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
- 2026-05-08
  2D game engine layer sources added under `sdk/ng_*`, with linker-space reservation for engine state
- 2026-05-09
  Python became the default sample-conversion path, and the live title/game flow was remapped around the current ADPCM-B theme set

See [`CHANGELOG.md`](./CHANGELOG.md) for release-level notes.

## Repository Overview

```text
CHANGELOG.md      — release notes and version history
docs/             — game-engine and build integration docs
main.c            — demo/game presentation flow on 68000 side
user.c            — Neo Geo BIOS hook handlers and startup flow
sdk/              — headers, linker scripts, support library
  sound_ids.h     — named sound IDs for music, SFX, beds, FM, and SSG tracks
  ng_*.h/.c       — 2D game engine layer modules
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

License

This project is released under the MIT License.

See LICENSE.

Eagle Software
https://eaglesoftware.biz/




 ## Eagle Software Engineering Note
 
## Reprogrammable Cartridge PCB Architecture for NeoGeoSDK Homebrew Development

Eagle Software is documenting a technical direction for a reprogrammable cartridge PCB intended for NeoGeoSDK homebrew development, hardware validation, diagnostics, and private engineering work.
The purpose of this board is to allow original NeoGeoSDK output to be programmed into cartridge ROM regions and tested on compatible hardware. It is not intended for unauthorized copying, reproduction, conversion, sale, or use of copyrighted commercial software.
This engineering note describes two possible hardware paths:
```text
1. Parallel NOR flash cartridge
2. SDRAM loader cartridge
```
The recommended first revision is the parallel NOR flash cartridge, because it behaves closest to a fixed ROM cartridge and is easier to validate.
---
Legal and Rights Notice
This project is not affiliated with, endorsed by, approved by, licensed by, or connected to SNK Corporation or any related rights holder.
All trademarks, names, historical references, hardware references, and platform references remain the property of their respective owners.
This work must be used only for:
```text
Original homebrew software
Private development
Technical research
Education
Lawful preservation-oriented study
Authorized consulting work
```
It must not be used for unauthorized copying, redistribution, reproduction, sale, conversion, or operation of copyrighted commercial software.
Any commercial, consulting, manufacturing, or distribution activity must be reviewed for legal compliance and, where required, performed only with proper authorization or acceptance from the relevant rights holders.
---
1. Cartridge ROM Model
The cartridge memory model must be respected as separate functional ROM regions:
```text
P1  - main program region
C1  - sprite graphics region
C2  - sprite graphics region
S1  - fix/text graphics region
M1  - sound program region
V1  - sample data region
```
The cartridge structure uses two boards:
```text
PROG board:
  P1  - main program region
  V1  - sample data region

CHA board:
  C1  - sprite graphics region
  C2  - sprite graphics region
  S1  - fix/text graphics region
  M1  - sound program region
```
For the home console cartridge format, the cartridge uses two boards with 50 pins per side, 100 pins per board, 200 pins per cartridge, 2.54 mm pitch, and 1.6 mm PCB thickness.
---
2. First Revision Target
The first Eagle Software engineering target should be:
```text
Platform:       home console cartridge format
Board style:    two-board cartridge set
Memory type:    parallel NOR flash
ROM layout:     fixed map
Banking:        none in revision 1
Programming:    external USB programmer or onboard programming controller
Purpose:        NeoGeoSDK homebrew validation
```
Recommended first capacity:
```text
P1  = 1 MB
C1  = 2 MB
C2  = 2 MB
S1  = 128 KB
M1  = 128 KB
V1  = 2 MB
```
This capacity is enough for early NeoGeoSDK demonstrations, engine tests, diagnostic ROMs, visual tests, sound tests, and small homebrew programs.
---
3. Architecture A — Parallel NOR Flash Cartridge
This is the preferred first design.
3.1 Principle
In PLAY mode, each flash device behaves like a normal ROM.
In PROGRAM mode, a programmer takes control of the flash address bus, data bus, and write strobes.
```text
                 +---------------------+
                 | USB programmer MCU  |
                 +----------+----------+
                            |
                            v
                 +---------------------+
                 | CPLD / bus control  |
                 +----------+----------+
                            |
     +----------------------+----------------------+
     |                      |                      |
     v                      v                      v
  P1 flash              C1/C2 flash           S1/M1/V1 flash
     |                      |                      |
     +----------------------+----------------------+
                            |
                            v
                  Cartridge edge connector
```
3.2 Advantages
```text
Closest behavior to fixed ROM hardware
No boot loader delay
No SDRAM refresh concern
No runtime image loading
Simpler failure analysis
Good for manufacturing prototype revision 1
```
3.3 Disadvantages
```text
More flash devices
More address/data routing
More level shifting
Less flexible than SDRAM
Large ROM expansion requires larger flash devices or banking
```
---
4. Architecture B — SDRAM Loader Cartridge
This is a more advanced design.
4.1 Principle
The cartridge contains SDRAM. On power-up or reset, a microcontroller loads ROM images from onboard flash, SD card, or USB into SDRAM. After loading, the SDRAM controller presents those memory regions to the cartridge bus as if they were ROM.
```text
              +-------------------+
              | SD / USB storage  |
              +---------+---------+
                        |
                        v
              +-------------------+
              | Loader MCU        |
              +---------+---------+
                        |
                        v
              +-------------------+
              | SDRAM controller  |
              +---------+---------+
                        |
     +------------------+------------------+
     |                  |                  |
     v                  v                  v
  P/V bus            C bus              S/M bus
     |                  |                  |
     +------------------+------------------+
                        |
                        v
             Cartridge edge connector
```
4.2 Advantages
```text
Flexible ROM sizes
Can support SD card loading
Can support multiple homebrew images
Can reduce number of large parallel flash chips
Easier to update content without erasing many NOR devices
```
4.3 Disadvantages
```text
Harder timing problem
Requires deterministic bus response
Requires SDRAM refresh management
Requires loader firmware
Requires robust reset behavior
More complex validation
Harder to manufacture correctly on first revision
```
For Eagle Software revision 1, SDRAM should be considered a later engineering path after the direct NOR flash cartridge is validated.
---
5. Board Division
5.1 PROG Board
The PROG board should implement:
```text
P1 flash region
V1 flash region
Bus transceivers
CPLD or control logic
Programming connector
3.3 V regulator
Mode select circuit
Test pads
Cartridge edge connector fingers
```
Functional role:
```text
P1 = main program read region
V1 = sample data read region
```
5.2 CHA Board
The CHA board should implement:
```text
C1 flash region
C2 flash region
S1 flash region
M1 flash region
Bus transceivers
CPLD or control logic
Programming connector
3.3 V regulator
Mode select circuit
Test pads
Cartridge edge connector fingers
```
Functional role:
```text
C1/C2 = sprite data regions
S1    = fix/text data region
M1    = sound program region
```
---
6. Electrical Requirements
The cartridge must be designed as a 5 V bus system connected to modern 3.3 V logic.
Minimum requirements:
```text
No 5 V signal shall be connected directly to a non-5 V-tolerant input.
All bidirectional data buses shall use controlled bus transceivers.
Flash write enable shall be disabled in PLAY mode.
All control inputs shall have defined pull-ups or pull-downs.
All unused flash control pins shall be tied to known states.
All voltage rails shall have local decoupling.
The programming interface shall never drive the console bus.
```
Recommended devices:
```text
74LVC245 / 74LVC16245 bus transceivers
74LVC573 / 74LVC16373 latches where needed
CPLD or small FPGA for mode and chip-select control
3.3 V parallel NOR flash
RP2040 / RP2350 / STM32-class programming MCU
```
---
7. PLAY / PROGRAM Mode
The board must have two electrically separate states.
PLAY Mode
```text
Console address bus -> flash address bus
Console read strobes -> flash /OE and /CE
Flash data bus -> console data bus
Flash /WE -> forced inactive
Programmer bus -> disconnected
```
In this mode, the board behaves as a read-only cartridge.
PROGRAM Mode
```text
Console cartridge bus -> disconnected
Programmer address bus -> flash address bus
Programmer data bus -> flash data bus
Programmer control -> flash /CE, /OE, /WE
Flash regions -> erase, write, read, verify
```
The safest first revision uses a physical switch or jumper:
```text
PLAY
PROGRAM
```
Default must be safe:
```text
/WE pulled high
/OE pulled high until valid
/CE pulled high until valid
programmer disabled unless PROGRAM mode is selected
```
---
8. NOR Flash Implementation Detail
8.1 Suggested Region Mapping
```text
P1 region:
  width: 16-bit preferred
  size: 1 MB minimum
  device: x16 parallel NOR flash

C1 region:
  width: according to CHA bus wiring
  size: 2 MB minimum
  device: parallel NOR flash

C2 region:
  width: according to CHA bus wiring
  size: 2 MB minimum
  device: parallel NOR flash

S1 region:
  width: 8-bit
  size: 128 KB
  device: small NOR flash or shared flash window

M1 region:
  width: 8-bit
  size: 128 KB
  device: small NOR flash or shared flash window

V1 region:
  width: 8-bit or 16-bit according to board design
  size: 2 MB minimum
  device: parallel NOR flash
```
8.2 Programming Process
```text
1. Put cartridge in PROGRAM mode.
2. Programmer reads board ID.
3. Programmer reads flash manufacturer/device ID.
4. Programmer erases target region.
5. Programmer writes data in flash program units.
6. Programmer reads back region.
7. Programmer verifies CRC32/SHA-1.
8. Programmer reports pass/fail.
9. Cartridge returns to PLAY mode.
```
8.3 File Padding
The SDK output must be padded to the exact hardware region size using `0xFF`.
```text
P1  -> 0x100000
C1  -> 0x200000
C2  -> 0x200000
S1  -> 0x020000
M1  -> 0x020000
V1  -> 0x200000
```
Example region table:
```text
REGION  FILE EXTENSION  SIZE       FUNCTION
P1      .p1             1 MB       main program
C1      .c1             2 MB       sprite data
C2      .c2             2 MB       sprite data
S1      .s1             128 KB     fix/text data
M1      .m1             128 KB     sound program
V1      .v1             2 MB       sample data
```
---
9. SDRAM Implementation Detail
The SDRAM design should be treated as a second-generation cartridge design.
9.1 Required Blocks
```text
Storage:
  SD card, onboard QSPI flash, or USB mass-storage interface

Loader:
  MCU loads ROM files into SDRAM

Memory:
  SDRAM or PSRAM large enough for all ROM regions

Controller:
  FPGA or fast MCU logic serving cartridge bus reads

Bus interface:
  Level shifting between cartridge bus and 3.3 V logic

Protection:
  State machine preventing invalid bus ownership
```
9.2 SDRAM Load Process
```text
1. Power on.
2. Hold cartridge output disabled until loader is ready.
3. Loader reads ROM manifest.
4. Loader copies P1, C1, C2, S1, M1, V1 into SDRAM regions.
5. Loader verifies CRC32/SHA-1.
6. Controller enables console read access.
7. Cartridge presents memory as ROM.
```
9.3 SDRAM Runtime Requirements
The SDRAM design must meet read timing for every cartridge bus access.
Required behavior:
```text
The bus controller must decode the requested region.
It must return stable data within the required access window.
It must handle refresh cycles without corrupting cartridge reads.
It must provide deterministic response.
It must recover cleanly from reset.
It must never expose half-loaded memory to the console.
```
9.4 SDRAM Risks
```text
Late data causes boot failure or corrupted graphics.
Refresh collision causes intermittent crash.
Incorrect bus isolation can damage logic.
Loader delay must be handled safely.
Reset sequencing must be deterministic.
```
For this reason, SDRAM should not be the first production path unless the engineering team has already validated bus timing with a prototype.
---
10. Programming Tool Requirements
The PC-side tool should handle ROM regions explicitly.
Required commands:
```text
identify-board
identify-flash
erase-region P1
write-region P1 file.p1
verify-region P1 file.p1
erase-region C1
write-region C1 file.c1
verify-region C1 file.c1
erase-region C2
write-region C2 file.c2
verify-region C2 file.c2
erase-region S1
write-region S1 file.s1
verify-region S1 file.s1
erase-region M1
write-region M1 file.m1
verify-region M1 file.m1
erase-region V1
write-region V1 file.v1
verify-region V1 file.v1
```
The tool must reject:
```text
Files larger than region
Unknown board revision
Unknown flash ID
Missing required region
CRC mismatch after readback
Programming while board is not in PROGRAM mode
```
---
11. Manufacturing Requirements
The PCB set should use:
```text
Two-board cartridge PCB set
1.6 mm PCB thickness
4-layer stackup
Hard-gold edge fingers
Beveled edge connector
2.54 mm pitch
Solid ground plane
Local decoupling for every IC
Clear silkscreen region labels
Test pads for major buses
Revision ID on both boards
```
Recommended stackup:
```text
Layer 1: components and high-priority signals
Layer 2: ground plane
Layer 3: power and secondary routing
Layer 4: signals
```
First production order:
```text
5 bare PCB sets
Assemble 1 set only
Bench-test before full assembly
Revise before larger run
```
---
12. Bring-Up Procedure
12.1 Bench Test
```text
1. Inspect soldering under microscope.
2. Check 5 V to GND resistance.
3. Check 3.3 V to GND resistance.
4. Power from bench supply with current limit.
5. Verify 3.3 V regulator output.
6. Verify CPLD/MCU clock.
7. Verify mode-select signal.
8. Verify flash ID read.
9. Verify erase/write/read/verify on each flash.
```
12.2 Cartridge Slot Test
```text
1. Insert into unpowered console slot.
2. Confirm mechanical fit.
3. Confirm no edge connector misalignment.
4. Confirm no shell interference.
5. Confirm no short on power rails after insertion.
```
12.3 Powered Console Test
```text
1. Use a known-good console unit.
2. Use current-monitored power.
3. Insert cartridge in PLAY mode only.
4. Power on.
5. Observe current behavior.
6. If current is abnormal, power off immediately.
7. Test P1 boot.
8. Test S1 text.
9. Test C1/C2 sprites.
10. Test M1 sound.
11. Test V1 sample playback.
```
---
13. Diagnostic ROM Validation Order
Do not validate the board with a full game first.
Use a staged NeoGeoSDK diagnostic sequence:
```text
Test 1:
  P1 only boot and watchdog stability

Test 2:
  P1 + S1 text output

Test 3:
  P1 + C1/C2 sprite pattern

Test 4:
  P1 + M1 sound command test

Test 5:
  P1 + M1 + V1 sample playback

Test 6:
  Full NeoGeoSDK demo
```
Failure isolation:
```text
No boot:
  P1 mapping, address bus, data bus, read strobes, reset, checksum

Wrong text:
  S1 mapping, S data lines, address lines

Bad sprites:
  C1/C2 swapped, C bus wiring, C address lines, data plane issue

No sound driver:
  M1 mapping, Z80 region issue, sound reset issue

No samples:
  V1 mapping, sample address issue, sample data bus issue

Random crash:
  Timing margin, bus contention, weak level shifting, power noise
```
---
14. Eagle Software Engineering Position
Eagle Software’s position is that NeoGeoSDK hardware support must remain technically faithful to the cartridge memory model while staying strictly focused on lawful homebrew development.
The first cartridge revision should not try to be universal. It should be a stable, conservative, fixed-layout development board.
Primary goals:
```text
Correct mechanical format
Correct two-board organization
Correct ROM region behavior
Safe PLAY / PROGRAM mode separation
Reliable flash programming
Reliable readback verification
Proper voltage translation
Stable operation on original compatible hardware
```
Secondary goals, only after validation:
```text
Larger ROM regions
Bank switching
SD loading
Multiple homebrew images
Shared arcade/home cartridge design
Advanced diagnostics
Manufacturing test fixture
```
---
15. Short Public Notice
```text
Eagle Software is evaluating a reprogrammable cartridge PCB direction for NeoGeoSDK homebrew development.

The proposed board is intended to support original homebrew software produced with NeoGeoSDK by allowing developers to program and test standard cartridge ROM regions on compatible hardware. The first engineering direction is a conservative flash-based development cartridge focused on reliable region mapping, safe bus control, proper voltage translation, and hardware validation.

Two technical paths are under study: a direct parallel NOR flash cartridge and a later SDRAM loader cartridge. The first revision is expected to use parallel NOR flash with a fixed ROM map, because this provides the most direct and reliable path to hardware validation.

This work is not affiliated with, endorsed by, approved by, licensed by, or connected to SNK Corporation or any related rights holder.

The project is intended only for lawful homebrew development, technical research, education, preservation-oriented study, or authorized consulting work. It must not be used for unauthorized copying, distribution, reproduction, sale, conversion, or operation of copyrighted commercial software.

All copyrights, trademarks, and intellectual property rights must be respected. Any commercial or consulting use should be performed only in accordance with applicable law and with proper authorization or acceptance from the relevant rights holders.
```
