# Neo Geo SDK

Neo Geo development SDK for SNK hardware.

- Repository: https://github.com/eaglesoftware777/neogeosdk
- Current release: `v1.7.0`
- Changelog: [`CHANGELOG.md`](./CHANGELOG.md)
- SDK API guide: [`SDK_API_GUIDE.md`](./SDK_API_GUIDE.md)

## What's new in v1.7.0 — the 2D engine release

v1.7.0 consolidates the whole `neo_universal_2d` line of work. It is the
largest release the SDK has had.

- **A complete 2D game engine**, in plain C (`sdk/2d_engine/`) and C++14
  (`sdk/2d_engine_plus/`) with an identical public ABI — 35 modules covering
  characters, actions, physics, NPCs, camera, level, particles, feedback,
  depth effects, palette effects, sprite groups, the render queue, the FIX
  layer, input, timers, events and fixed-point math. Select the C++ build
  with `USE_2D_PLUS=1`.
- **A multi-game build system** — one repository, any number of independent
  games, each with its own id, ROM folder, artbox, sound tree and MAME hash
  set. Six games ship with it, and all six build the same way.
- **A rebuilt art pipeline** — a perceptual quantiser working in CIE-Lab on
  the Neo Geo's actual 5-bit colour lattice, with void-and-cluster blue-noise
  dithering, per-tile palettes, master sprite palettes, sprite halo removal
  and a content-preserving screen fit. Mean colour error across the reference
  assets fell from dE 9.10 to 8.33, p95 from 18.19 to 15.66.
- **A rebuilt audio stack** — nine ADPCM-B beds, eight FM tracks, nine SSG
  tracks, a recorded voice bank with `speakText()`, live FM tempo, LFO, pan,
  noise and CSM control, and a fade engine that actually fades. The Z80
  polling race that deadlocked the 68000 is fixed.
- **Sky Lance** (`games/skylance`, id 779) — a complete vertical arcade
  shooter: three pilots, seven stages, a named boss per stage, attract reel,
  pilot select, scoring, lives, energy and a continue flow.
- **A 25-chapter demo reel** (`games/demo`, id 777) exercising every public
  subsystem in order, ending on a playable Sky Lance slice. Chapter number
  printed top-right; **A** advances, **C** restarts.
- **Native Windows builds**, one-shot installers for Linux / Ubuntu /
  Windows / WSL, and a documented WSL2 + PulseAudio audio path.
- **Two PyQt6 desktop tools** — Artbox Studio (pipeline runner, hex sprite
  inspector, movement designer, level designer, HD compare, ROM inventory,
  asset-rule editor) and Sound Studio (track / mix / ROM tabs, live waveform,
  MML designer).
- **A documentation set written for the end user** — introduction,
  programmer's manual, hello-world tutorial, sound driver reference, art
  pipeline, shipped-game guide, chapter guide, generated C and C++ API
  references, a single-file overview, and a printable PDF manual.

Corrections worth knowing about if you have code on an earlier revision:

- **Sprite priority: HIGHER slot number is drawn IN FRONT.** Backgrounds
  belong at slots 1–32, characters at 96–223, foreground effects at 288+.
  Earlier revisions claimed the opposite.
- **The backdrop register is `$401FFE`**, the last word of palette RAM.
  `$402000` is a mirror that silently does nothing.
- **The FIX layer has 28 visible rows**; visible row *y* is map row *y + 2*.
- **`REG_PALBANK0` / `REG_PALBANK1`** had their addresses swapped.
- **A minimal game can link again** — `ng_bg.c` / `ng_bg.cpp` now carry weak
  fallbacks for `ng_screen_table[]` / `ng_screen_count`.

Full detail, commit by commit: [`CHANGELOG.md`](./CHANGELOG.md).

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

The repository includes a reusable 2D game engine layer under `sdk/2d_engine/ng_*`.

It is a plain-C engine targeting **NeoGeo Deluxe 2D**: huge animated characters, large bosses, smooth camera movement, parallax depth, sprite-scaling depth effects, palette lighting, hit sparks, particles, cinematic transitions, and stable 60 FPS — no float, no malloc during gameplay, no division in the frame loop.

### Core engine modules

- characters and action scripts
- level state and camera scroll
- per-frame `game_engine_frame()`
- game events, border constraints
- NPC helpers, physics bodies and solids
- cached FIX-layer text output
- status flags, timers, progress counters, properties matrix

### Deluxe 2D engine modules

| Module | Header | What it does |
|--------|--------|-------------|
| Render queue | `ng_render_queue.h` | 128-slot VBlank-safe deferred VRAM/palette write queue |
| Fixed-point math | `ng_fixed.h` | 16.16 fixed-point, sin/cos/shrink lookup tables |
| Camera | `ng_camera.h` | Smooth follow, dead zone, look-ahead, shake, cinematic pan, border clamp |
| Palette FX | `ng_palette_fx.h` | Fade, flash (white/red/blue), pulse, color cycle — queue-safe |
| Particles | `ng_particles.h` | 32-slot fixed pool, 8 types, priority eviction |
| Feedback | `ng_feedback.h` | Hitstop + screen shake + palette flash + sound hook in one call |
| Depth FX | `ng_depthfx.h` | NGVec3 perspective projection, Z→shrink lookup, starfield |
| Sprite groups | `ng_sprite_group.h` | Dirty-flag sticky-bit sprite chains, write-only-what-changed |
| Debug HUD | `ng_debug.h` | Fix-layer perf overlay (`#define NG_DEBUG_PERF 1`) |

### Important build notes

- source files live in `sdk/2d_engine/ng_*.c` and `sdk/2d_engine/ng_*.h`
- linker scripts reserve `game_engine_bss` for the engine state objects
- `Makefile` and `MakefileWin32.mak` compile and link all `sdk/2d_engine/ng_*` modules automatically
- sprite drawing uses world-space character coordinates minus the current level camera scroll

Use these docs for the current integration path:

- [`docs/GAME_ENGINE_LAYER.md`](./docs/GAME_ENGINE_LAYER.md)
- [`docs/sprite_groups.md`](./docs/sprite_groups.md)
- [`docs/render_queue.md`](./docs/render_queue.md)
- [`docs/camera.md`](./docs/camera.md)
- [`docs/palette_fx.md`](./docs/palette_fx.md)
- [`docs/particles.md`](./docs/particles.md)
- [`docs/depthfx.md`](./docs/depthfx.md)
- [`docs/performance_rules.md`](./docs/performance_rules.md)
- [`docs/MAKEFILE_INTEGRATION.md`](./docs/MAKEFILE_INTEGRATION.md)
- [`docs/DEPENDENCIES.md`](./docs/DEPENDENCIES.md)
- [`docs/GDB_GUIDE.md`](./docs/GDB_GUIDE.md)
- [`docs/BANKSWITCH.md`](./docs/BANKSWITCH.md)

## Desktop Tools

Two PyQt6 graphical tools ship with the SDK for visual asset editing and sound composition.

### Artbox Studio

```bash
python3 artbox/artbox_studio.py
```

Four tabs: C-ROM tile grid viewer, sprite designer with C-snippet export, hitbox editor with draggable rects, and 16-color pixel paint editor that writes back to the ROM buffer.  See [`docs/ARTBOX_PIPELINE.md`](./docs/ARTBOX_PIPELINE.md).

### Sound Studio

```bash
python3 sound/sound_studio.py
```

Five tabs: FM patch editor (4-operator, all parameters), MML composer with piano roll and one-click compile, SSG preset editor, ADPCM sample manager with waveform preview, and a YM2610 simulator for mixed FM+SSG channel playback.  See [`docs/SOUND_STUDIO_GUIDE.md`](./docs/SOUND_STUDIO_GUIDE.md).

**Requirements:** Python 3, PyQt6, numpy, scipy.

```bash
pip install PyQt6 numpy scipy
```


## Quick Start

Build the MVS program ROM and launch in MAME (default BIOS: EUROPE MVS):

```bash
# Linux / WSL
make test

# Windows
make -f MakefileWin32.mak test
```

Test with a specific BIOS:

```bash
# Linux / WSL
make test BIOS=euro
make test BIOS=unibios40
make test BIOS=japan

# Windows
make -f MakefileWin32.mak test BIOS=euro
```

List all supported BIOS names:

```bash
make bios-list
# Windows: make -f MakefileWin32.mak bios-list
```

Build AES cartridge ROM instead:

```bash
make test-aes
# Windows: make -f MakefileWin32.mak test-aes
```


## Multi-Game Build System

The SDK supports multiple independent games under a single repository.  Each game
lives in its own subdirectory under `games/` and produces its own ROM when built
with `GAME=<name>`.

### Included games

| Folder | Game ID | ROM prefix | Engine | Description |
|--------|---------|------------|--------|-------------|
| `games/demo` | 777 | `777-*` | C | The 25-chapter engine reel — every subsystem, in order |
| `games/demo_plus` | 778 | `778-*` | C++ | The same engine through the C++ API (`USE_2D_PLUS=1`) |
| `games/skylance` | 779 | `779-*` | C | Sky Lance — a complete vertical shooter |
| `games/helloworld` | 772 | `772-*` | — | Minimal FIX-text and one sample; the tutorial target |
| `games/tutorial` | 555 | `555-*` | C | The minimal engine loop, nothing else |
| `games/neogeogame` | 775 | `775-*` | C | Blank template for new projects |

Every game carries its own `game.cfg`, so all six build the same way:

```bash
make GAME=<name> GAME_CFG_FILE=games/<name>/game.cfg all
make GAME=<name> GAME_CFG_FILE=games/<name>/game.cfg test
```

What each one demonstrates: [`docs/GAMES.md`](./docs/GAMES.md).

### Building a specific game

`demo` is the default when `GAME` is not specified.

```bash
# Linux / WSL
make game                          # build demo (default)
make GAME=helloworld game          # build helloworld
make GAME=tutorial p1              # build + package tutorial ROM
make GAME=neogeogame p1            # build + package neogeogame ROM

# Windows
make -f MakefileWin32.mak game
make -f MakefileWin32.mak GAME=helloworld game
make -f MakefileWin32.mak GAME=tutorial p1
make -f MakefileWin32.mak GAME=neogeogame p1
```

Full build (art + sound + program ROM):

```bash
# Linux / WSL
make GAME=helloworld all

# Windows
make -f MakefileWin32.mak GAME=helloworld all
```

### Game folder layout

Each game under `games/<name>/` contains:

```text
games/<name>/
  game.mk          — GAME_NAME, GAME_ID, GAME_SCENES (required)
  main.c           — ng_screen_table, ng_screen_count stubs
  user.c           — all BIOS dispatch and interrupt handlers
  eyecatcher.c     — eye-catcher animation
  neogeo_mvs.c     — MVS ROM header with game ID
  neogeo_aes.c     — AES ROM header with game ID
  neogeo.ld        — linker script listing this game's object files
  artbox/
    in/            — sprite and background source images
    infix/         — FIX tile source images
  sound/
    fm/            — FM patch and music MML files
    mml/           — SSG/MML music files
    ssg/           — SSG configuration and data
    samples/
      in_wav_a/    — ADPCM-A source WAV files
      in_wav_b/    — ADPCM-B source WAV files
  scenes/          — scene source files (listed in GAME_SCENES)
```

### `game.mk` format

Every game must provide a `game.mk`:

```makefile
GAME_NAME  = My Game Title
GAME_ID    = 775
GAME_SCENES = scene_main scene_title scene_game
```

`GAME_SCENES` is an explicit space-separated list of scene base names (no `.c`
extension, no path prefix). Files that are `#include`-d from other scene files
should be omitted to avoid multiple-definition errors.

If the game has no scene files (e.g. `helloworld`), omit `GAME_SCENES` entirely.

### Art and sound pipelines

The art and sound pipelines read per-game asset folders automatically:

```bash
# Linux / WSL
make GAME=helloworld art          # runs artbox on games/helloworld/artbox/
make GAME=tutorial sound          # compiles games/tutorial/sound/{fm,mml,ssg,samples}

# Windows
make -f MakefileWin32.mak GAME=helloworld art
make -f MakefileWin32.mak GAME=tutorial sound
```

On Linux, `artbox/makeartbox.sh` uses temporary symlinks (`ln -sfn`) to redirect
the pipeline to the per-game asset folder.  On Windows, `artbox/makeartbox.bat`
uses directory junctions (`mklink /J`), which do not require administrator
privileges on Windows Vista and later.

### Interactive launcher

Both launcher scripts include a game-selection step:

```bash
# Linux / WSL
./neogeosdk.sh          # press g to change the active game

# Windows
neogeosdk.bat           # press g to change the active game
```

### Adding a new game

1. Copy `games/neogeogame/` to `games/<yourname>/`.
2. Edit `games/<yourname>/game.mk` — set `GAME_NAME`, `GAME_ID`, and `GAME_SCENES`.
3. Edit `games/<yourname>/neogeo_mvs.c` and `neogeo_aes.c` — update the `id` field to your unique game ID (hex).
4. Edit `games/<yourname>/neogeo.ld` — add any new scene object file patterns.
5. Implement your scenes in `games/<yourname>/scenes/`.
6. Build: `make GAME=<yourname> p1`

## Running the ROM in MAME

The cartridge is loaded through the Neo Geo software list as `neogeosdk`.

**Requirements:**
- MAME installed and on `PATH`
- Legal `neogeo.zip` BIOS placed in the `roms/` folder

**Do not** run `mame neogeosdk` directly — that looks for a driver, not a cartridge.  
**Correct command:**

```
mame neogeo -cart1 demo -rompath roms/demo;roms -hashpath hash_eagle/demo;hash_eagle;hash -bios unibios22
```

**Windows quick-start:**
```
run_neogeosdk.bat
```

**Linux quick-start:**
```
./run.sh
```

**Insert coin / start (MVS mode):** press `5` then `1` on the keyboard.

### Building the release package

```
make dist                          # Linux
make -f MakefileWin32.mak dist    # Windows
```

This builds the ROM set, regenerates `hash_eagle/<game>/neogeo.xml`, creates `dist/roms/<game>.zip`
(ROM files at archive root), copies `hash_eagle/<game>/neogeo.xml` to `dist/hash_eagle/<game>/`, and writes
`dist/run_neogeosdk.bat` and `dist/run_neogeosdk_debug.bat`.

Distribute `dist/` as-is. End users place their `neogeo.zip` BIOS inside `dist/roms/` and
run `dist/run_neogeosdk.bat`.

## Release Assets

The `v1.7.0` release publishes these attached assets:

- `neogeosdkv1.7.0.tar.gz`  
  source snapshot for the SDK
- `neogeosdk.zip`  
  generated demo ROM set for MAME (`777-p1.p1`, `777-m1.m1`, `777-s1.s1`, `777-v1.v1`, `777-c1.c1`, `777-c2.c2`)

The release page also carries `x-tools.tar` for the Linux toolchain layout used by
the default `Makefile`. That asset is kept as-is when documentation-only or ROM-only
release updates are published.

Current release page:

- `https://github.com/eaglesoftware777/neogeosdk/releases/tag/v1.7.0`

## Documentation

**Start here**

| Document | For |
|---|---|
| [`docs/INTRODUCTION.md`](./docs/INTRODUCTION.md) | What the SDK is, what the machine is, and the reading order |
| [`docs/HELLO_WORLD.md`](./docs/HELLO_WORLD.md) | Your first ROM, in ten minutes |
| [`docs/PROGRAMMERS_MANUAL.md`](./docs/PROGRAMMERS_MANUAL.md) | The working manual — build, layout, engine tick, assets, frame budget |
| [`NEOGEOSDK_v1.7.0.md`](./NEOGEOSDK_v1.7.0.md) | The single-file release overview |
| [`docs/neogeosdk_v1.7.0_manual.pdf`](./docs/neogeosdk_v1.7.0_manual.pdf) | The printable manual and API reference |

**Reference**

| Document | Covers |
|---|---|
| [`SDK_API_GUIDE.md`](./SDK_API_GUIDE.md) | The bare-metal 68000 helpers in `sdk/neogeo.h` |
| [`docs/API_2D_ENGINE_C.md`](./docs/API_2D_ENGINE_C.md) | Every call in the C 2D engine |
| [`docs/API_2D_ENGINE_CPP.md`](./docs/API_2D_ENGINE_CPP.md) | The C++14 build of the same engine |
| [`docs/SOUND_DRIVER.md`](./docs/SOUND_DRIVER.md) | The Z80 driver, the YM2610, and the audio pipelines |
| [`docs/ARTBOX_PIPELINE.md`](./docs/ARTBOX_PIPELINE.md) | PNG to C-ROM, the quantiser, and Artbox Studio |
| [`docs/GAMES.md`](./docs/GAMES.md) | What each of the six shipped games demonstrates |
| [`docs/DEMO_CHAPTERS.md`](./docs/DEMO_CHAPTERS.md) | The 25 demo chapters |
| [`docs/TOOLS.md`](./docs/TOOLS.md) | Every helper script — art, sound, verification, launchers, GIMP plug-ins |

Primary repository docs:

- [`README.md`](./README.md)
- [`docs/ADDING_A_GAME.md`](./docs/ADDING_A_GAME.md)
- [`docs/GAME_ENGINE_LAYER.md`](./docs/GAME_ENGINE_LAYER.md)
- [`docs/sprite_groups.md`](./docs/sprite_groups.md)
- [`docs/render_queue.md`](./docs/render_queue.md)
- [`docs/camera.md`](./docs/camera.md)
- [`docs/palette_fx.md`](./docs/palette_fx.md)
- [`docs/particles.md`](./docs/particles.md)
- [`docs/depthfx.md`](./docs/depthfx.md)
- [`docs/performance_rules.md`](./docs/performance_rules.md)
- [`docs/ARTBOX_PIPELINE.md`](./docs/ARTBOX_PIPELINE.md)
- [`docs/MAKEFILE_INTEGRATION.md`](./docs/MAKEFILE_INTEGRATION.md)
- [`docs/DEPENDENCIES.md`](./docs/DEPENDENCIES.md)
- [`docs/GDB_GUIDE.md`](./docs/GDB_GUIDE.md)
- [`docs/BANKSWITCH.md`](./docs/BANKSWITCH.md)
- [`docs/SOUND_STUDIO_GUIDE.md`](./docs/SOUND_STUDIO_GUIDE.md)
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

- default search order in `Makefile`:
  1. `$(SDKHOME)/x-tools-v2`
  2. `$(SDKHOME)/x-tools` (legacy fallback)
- after extraction, compiler path is:
  - `$(XTOOLS_ROOT)/m68k-unknown-elf/bin/m68k-unknown-elf-gcc`

Windows 68000 compiler:

- default search order in `MakefileWin32.mak`:
  1. `$(SDKHOME)\x-tools-v2-win\m68k-unknown-elf\bin`
  2. `$(SDKHOME)\x-tools-v2-win\m68k-elf\bin`
  3. `M68K_ELF_ROOT` fallback (default `C:\SysGCC\m68k-elf`)
- if your fallback toolchain is installed elsewhere, set `M68K_ELF_ROOT` when invoking `make`
- you do not need to clone the SDK into a fixed drive or fixed folder name beyond
  keeping `neogeosdk/` under the chosen `SDKHOME` parent

## SDKHOME Layout

Both makefiles expect `SDKHOME` to point to the directory that contains `neogeosdk/`
and toolchain folders.

Expected layout:

```text
SDKHOME/
  neogeosdk/
  x-tools-v2/         # Linux default
  x-tools/            # Linux legacy fallback
  x-tools-v2-win/     # Windows default
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
$HOME/neogeo/x-tools-v2  -> m68k Linux cross compiler bundle (default)
$HOME/neogeo/x-tools     -> legacy fallback bundle
```

Install the Linux 68000 toolchain from the release asset so that `x-tools-v2/` lands
next to the repository:

```bash
cd $HOME/neogeo
curl -L -o x-tools-v2.tar https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.0/x-tools-v2.tar
tar -xf x-tools-v2.tar
```

After extraction, verify:

```bash
$HOME/neogeo/x-tools-v2/m68k-unknown-elf/bin/m68k-unknown-elf-gcc --version
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

- `$(SDKHOME)/x-tools-v2/...` (or legacy `x-tools/...`) for `m68k-unknown-elf` binaries
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
curl -L -o x-tools-v2.tar https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.0/x-tools-v2.tar
tar -xf x-tools-v2.tar

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

### WSL2 + Ubuntu 24.04 + PulseAudio - Working Audio Guide

#### The Problem

WSLg ships PulseAudio `17.0`, but Ubuntu 24.04 only has `16.1` in its
repositories. The version mismatch can cause connections to hang or fail with
protocol errors. This can affect both WSL terminal sessions and SSH sessions.

#### Part 1 - Fix WSL Terminal Audio Through WSLg

Disable the conflicting systemd PulseAudio units:

```bash
systemctl --user disable --now pulseaudio.service pulseaudio.socket
systemctl --user mask pulseaudio.service pulseaudio.socket
```

Clean broken symlinks:

```bash
rm -rf /run/user/1000/pulse
mkdir -p /run/user/1000/pulse
```

Test WSLg directly:

```bash
PULSE_SERVER=unix:/mnt/wslg/PulseServer pactl info
```

The output should report `Server Version: 17.0` and `Default Sink: RDPSink`.

#### Part 2 - Build PulseAudio 17.0 From Source

Ubuntu repositories only have PulseAudio `16.1`; build `17.0` to match WSLg.

Install build dependencies:

```bash
sudo apt install -y build-essential meson ninja-build git \
    libsndfile1-dev libspeexdsp-dev libtdb-dev \
    libdbus-1-dev libcap-dev libasyncns-dev \
    libglib2.0-dev libavahi-client-dev \
    libssl-dev check libsoxr-dev \
    libfftw3-dev libwebrtc-audio-processing-dev
```

Clone and build:

```bash
mkdir -p ~/pulseaudio && cd ~/pulseaudio
git clone --depth=1 --branch v17.0 \
    https://gitlab.freedesktop.org/pulseaudio/pulseaudio.git .

meson setup build --prefix=/usr/local \
    -Ddaemon=true \
    -Dclient=true \
    -Dtests=false \
    -Ddoxygen=false \
    -Dbluez5=disabled \
    -Djack=disabled \
    -Dlirc=disabled \
    -Dgcov=false

cd build
ninja
sudo ninja install
```

Fix library linking:

```bash
echo '/usr/local/lib' | sudo tee /etc/ld.so.conf.d/pulseaudio-local.conf
sudo ldconfig /usr/local/lib
```

Verify:

```bash
/usr/local/bin/pactl --version
# Should show: pactl 17.0

ldd /usr/local/bin/pactl | grep pulse
# Should show: /usr/local/lib/... instead of /usr/lib/...

PULSE_SERVER=unix:/mnt/wslg/PulseServer /usr/local/bin/pactl info
# Should connect and show Server Version: 17.0
```

#### Part 3 - Expose Audio Over TCP for SSH Sessions

SSH sessions cannot access WSLg's Unix socket directly, so expose it over TCP.
Use this only on a trusted local WSL instance.

Load the TCP module into WSLg:

```bash
PULSE_SERVER=unix:/mnt/wslg/PulseServer /usr/local/bin/pactl \
    load-module module-native-protocol-tcp auth-anonymous=1 port=4713
```

Test from an SSH session:

```bash
PULSE_SERVER=tcp:127.0.0.1:4713 /usr/local/bin/pactl info
# Should connect and show RDPSink
```

Fix the ALSA-to-PulseAudio bridge:

```bash
cat > ~/.asoundrc << 'EOF'
pcm.!default {
    type pulse
    server "tcp:127.0.0.1:4713"
}
ctl.!default {
    type pulse
    server "tcp:127.0.0.1:4713"
}
EOF
```

Make the environment permanent:

```bash
cat >> ~/.bashrc << 'EOF'
export PULSE_SERVER=tcp:127.0.0.1:4713
export SDL_AUDIODRIVER=pulse
export PATH=/usr/local/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
EOF

cat >> ~/.profile << 'EOF'
export PULSE_SERVER=tcp:127.0.0.1:4713
export SDL_AUDIODRIVER=pulse
export PATH=/usr/local/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
EOF

source ~/.bashrc
```

#### Part 4 - Auto-load the TCP Module on WSL Start

First get your username:

```bash
whoami
```

Then edit `/etc/wsl.conf`. Replace `YOUR_USERNAME` with the output from
`whoami`:

```bash
sudo tee /etc/wsl.conf << 'EOF'
[boot]
systemd=true
command = su - YOUR_USERNAME -c "PULSE_SERVER=unix:/mnt/wslg/PulseServer /usr/local/bin/pactl load-module module-native-protocol-tcp auth-anonymous=1 port=4713"

[user]
default=YOUR_USERNAME

[interop]
enabled=true
appendWindowsPath=true
EOF
```

Or fill it in automatically:

```bash
MYUSER=$(whoami)
sudo tee /etc/wsl.conf << EOF
[boot]
systemd=true
command = su - $MYUSER -c "PULSE_SERVER=unix:/mnt/wslg/PulseServer /usr/local/bin/pactl load-module module-native-protocol-tcp auth-anonymous=1 port=4713"

[user]
default=$MYUSER

[interop]
enabled=true
appendWindowsPath=true
EOF
```

Restart WSL from Windows PowerShell:

```powershell
wsl --shutdown
```

Reopen WSL and SSH. Audio should work automatically.

#### Quick Verification Checklist

```bash
# 1. WSLg socket exists
ls /mnt/wslg/PulseServer

# 2. TCP port is listening
ss -tlnp | grep 4713

# 3. pactl works over TCP
PULSE_SERVER=tcp:127.0.0.1:4713 /usr/local/bin/pactl info

# 4. Play a test sound
paplay /usr/share/sounds/freedesktop/stereo/bell.oga
```

Key facts:

- WSLg runs PulseAudio `17.0`, while Ubuntu 24.04 ships `16.1`.
- The WSLg Unix socket at `/mnt/wslg/PulseServer` works from the WSL terminal,
  but not from SSH sessions.
- The TCP module bridges SSH sessions to WSLg audio.
- Building PulseAudio `17.0` from source keeps the client and WSLg server
  protocol versions aligned on Ubuntu 24.04.

## Installation on Windows

Recommended layout:

```text
<sdk root>\neogeosdk
<sdk root>\x-tools-v2-win
```

The Win32 makefile auto-detects `x-tools-v2-win` first. If not present, it accepts
a configurable `M68K_ELF_ROOT` fallback. Example fallback layout:

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
py -0p
py -m pip --version
py -m pip install --upgrade pip
py -m pip install numpy pillow pypng
```

Use `py -m pip` (not `pip`/`pip3`) so packages are installed into the same
interpreter used by `makeartbox.bat`.

Verify with:

```bat
cd artbox
py -c "import sys; print(sys.executable); import PIL, numpy, png, img2neo; print('OK')"
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
make art-crt         : same as 'art' but routes screens through the
                       CRT-optimised pipeline (img2neo_crt.py)
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
make debug-build     : build P1 with debug symbols and linker map
make debug-artifacts : write size, symbols, readelf, map, and disassembly files
make gdb-trace       : generate dump/gdb_trace.txt from a batch GDB script
make gdb             : open GDB on out/game
make gdb-remote      : open GDB and connect to GDB_REMOTE=host:port
make dist            : build p1 + package dist/roms/<game>.zip release layout
make test            : run in MAME (MVS, sp-s2.sp1 BIOS by default)
make test-aes        : run in MAME (AES, unibios22 BIOS)
make test BIOS=unibios22 : run with specific BIOS
```

Important recent build behavior:

- `make sound`, `make vrom`, and `make m1rom` sync generated outputs into `roms/<game>/`
- ROM files are written to `roms/<game>/` — e.g. `roms/demo/777-p1.p1`, `roms/helloworld/772-p1.p1`
- `make samples` uses the bundled Python WAV converter by default on Linux and Windows
- set `SOX=/path/to/sox` only when you explicitly want the SoX conversion path
- Windows `make fm`, `make mml`, and `make ssg` expand source file lists correctly
- Windows `make sfix` produces `<GAME_ID>-s1.s1` in the correct 128 KB FIX-ROM format
- P1 generation crops to the full 512 KB program ROM window (0x080000) before byte swap and padding; this is the required ROM format for MAME and hardware
- `hash_eagle/<game>/neogeo.xml` is auto-regenerated on every `make p1` build with correct CRC/SHA1 and `loadflag="load16_word_swap"` for the P-ROM
- `make dist` packages everything into `dist/roms/<game>.zip` with ROM files at archive root (no subfolder)
- `make m1rom-asm` is the authoritative M1 build path (`USE_Z80C=0`); the experimental C path (`make m1rom-c`) is for comparison only

## Debug and Trace Builds

The Linux and Windows makefiles support debug builds for the 68000 program.

Linux:

```bash
make debug-build
make debug-artifacts
make gdb-trace
make gdb
make gdb-server                         # terminal 1
make gdb-remote GDB_REMOTE=127.0.0.1:23946  # terminal 2
```

Windows:

```bat
make -f MakefileWin32.mak debug-build
make -f MakefileWin32.mak debug-artifacts
make -f MakefileWin32.mak gdb-trace
```

Generated debug files:

- `dump/game.size.txt`
- `dump/game.sym`
- `dump/game.readelf`
- `dump/game.debug.dump`
- `dump/game.map`
- `dump/gdb_trace.gdb`
- `dump/gdb_trace.txt`

If the bundled cross-GDB cannot start because of host library dependencies,
override it:

```bash
make gdb-trace GDB=/path/to/m68k-gdb
```

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

The 2D game engine layer in `sdk/2d_engine/ng_*` is already wired into the default
`game:` recipes in both `Makefile` and `MakefileWin32.mak`.

Those builds now:

- compile the `sdk/2d_engine/ng_*.c` modules into `out/ng_*0.o`
- link those objects into the main 68000 game binary
- place engine state in the linker-managed `game_engine_bss` region

See:

- [`docs/GAME_ENGINE_LAYER.md`](./docs/GAME_ENGINE_LAYER.md)
- [`docs/MAKEFILE_INTEGRATION.md`](./docs/MAKEFILE_INTEGRATION.md)
- [`docs/ARTBOX_PIPELINE.md`](./docs/ARTBOX_PIPELINE.md)

## Artbox Graphics Pipeline

The Artbox pipeline converts PNG graphics into Neo Geo sprite and FIX formats.

Detailed guide:

- [`docs/ARTBOX_PIPELINE.md`](./docs/ARTBOX_PIPELINE.md)

The pipeline is rule-driven through `artbox/assets.cfg`. Each PNG can now carry
both a conversion mode and a gameplay category. The generated manifest and
`artbox/sprite_meta.h` expose those categories to 68000-side code.

Current built-in categories:

- `background`
- `main_character`
- `opponent`
- `npc`

Current naming rules include:

- `sprite_*.png` as `main_character` sprite pages
- `opponent_*.png` as `opponent` sprite pages
- `z_npc_*.png` as `npc` sprite pages
- `background_*.png` and `zz_npc_forest_alley.png` as `background` screen pages

Core tools:

- `artbox/img2neo.py`
- `artbox/assets.cfg`
- `artbox/asset_rules.py`
- `artbox/gen_sprite_meta.py`
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
- `artbox/assets_manifest.json`
- `artbox/sprite_meta.h`
- `artbox/out.srt`

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

- `playSFXB(0)` / `SOUND_TRACK_A`
  - title theme
- `playSFXB(1)` / `SOUND_TRACK_B`
  - stage loop 1
- `playSFXB(2)` / `SOUND_TRACK_C`
  - stage loop 2
- `playSFXB(3)` / `SOUND_TRACK_D`
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
- `soundPlayGameLoop()` resets the scene and selects an ADPCM-B stage or ending TRACK directly
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
  custom Neo Geo sound system added: Z80 YM2610 driver, MML/FM/SSG build chain, cross-platform Python ADPCM encoder, Windows sound build parity
- 2026-05-05  
  experimental C migration of the Z80 sound driver and `z80c-special` compiler work
- 2026-05-06  
  higher-level sound workflow, named sound IDs, improved multi-layer demo mix, and compare flow for ASM vs C M1 builds
- 2026-05-07  
  softfloat removal, SDK API docs refresh, Windows sound-build parity fixes, and corrected Win32 FIX-ROM generation
- 2026-05-08  
  2D game engine layer sources added under `sdk/2d_engine/ng_*`, with linker-space reservation for engine state
- 2026-05-09  
  Python became the default sample-conversion path; live title/game flow remapped around the ADPCM-B theme set
- 2026-05-10  
  2D engine camera scroll, level/NPC/physics/fix modules, artbox asset categories, joystick-driven main-character actions; GDB guide, dependency docs, and bankswitch stub API added
- 2026-05-13 – 2026-05-15  
  per-game ROM folders (`roms/<game>/`), full `GAME_ID` propagation, multi-game build path refactor, Windows build path bugs fixed, 15 SDK bugs fixed
- 2026-05-16  
  FIX layer BRDFIX bug fixed in `clearFix()` — game S ROM now always restored after clear; demo rewritten as 13-scene cinematic showcase; particle slot and budget reporting corrected
- 2026-05-17  
  Z80 communication race condition in `soundCommand` fixed — trailing `isZ80Ready()` removed to prevent 68k deadlock; sound driver (`driver.asm`) restored to last known-good working version

See [`CHANGELOG.md`](./CHANGELOG.md) for full release-level notes.

## Repository Overview

```text
CHANGELOG.md      — release notes and version history
docs/             — game-engine and build integration docs
main.c            — demo/game presentation flow on 68000 side
user.c            — Neo Geo BIOS hook handlers and startup flow
sdk/              — headers, linker scripts, support library
  sound_ids.h         — named sound IDs for music, SFX, TRACKs, FM, and SSG tracks
  ng_*.h/.c           — 2D game engine layer modules
  ng_bankswitch.h/.c  — P-ROM bank switching stub (not used in demo)
artbox/           — graphics conversion pipeline
  assets.cfg      — art conversion rules and gameplay categories
  sprite_meta.h   — generated asset metadata for 68000-side sprite setup
sound/            — sound driver, tracks, samples, tools
  driver/         — ASM driver, experimental C driver, generated tables
  fm/             — FM tracks and patch bank
  mml/            — main music tracks
  ssg/            — standalone SSG tracks and presets
  samples/        — raw and converted sample assets
  tools/          — sound build utilities
roms/<game>/      — per-game ROM outputs for MAME (e.g. roms/demo/777-p1.p1 … 777-c2.c2)
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
C1  = 8 MB
C2  = 8 MB
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
  size: up to 8 MB in current SDK build flow
  device: parallel NOR flash

C2 region:
  width: according to CHA bus wiring
  size: up to 8 MB in current SDK build flow
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
C1  -> 0x800000
C2  -> 0x800000
S1  -> 0x020000
M1  -> 0x020000
V1  -> 0x200000
```
Example region table:
```text
REGION  FILE EXTENSION  SIZE       FUNCTION
P1      .p1             1 MB       main program
C1      .c1             8 MB       sprite data
C2      .c2             8 MB       sprite data
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
