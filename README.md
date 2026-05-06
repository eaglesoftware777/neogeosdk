# Neo Geo SDK

Neo Geo Development Kit for SNK Hardware <br/>

[https://github.com/eaglesoftware777](https://github.com/eaglesoftware777) <br/>
[https://github.com/eaglesoftware777/neogeosdk](https://github.com/eaglesoftware777/neogeosdk) <br/><br/>

A complete, self-contained development environment for creating software for the Neo Geo arcade and home systems. The toolchain gives direct control over VRAM, DMA, palette registers, sprites, and the YM2610 audio subsystem driven by a custom Z80 sound driver.

*The purpose of this SDK is to maintain authentic Neo-Geo development — hardware-centered and performance-critical — while making it accessible to modern developers.* <br/><br/>

---

## Requirements

<br/>

Ubuntu Linux  ⇒  [https://ubuntu.com/](https://ubuntu.com/) <br/>
m68k compiler ⇒  [https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.0/x-tools.tar](https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.0/x-tools.tar) <br/>
mame          ⇒  [https://www.mamedev.org/](https://www.mamedev.org/) <br/>
srecord       ⇒  [https://packages.ubuntu.com/search?keywords=srecord](https://packages.ubuntu.com/search?keywords=srecord) <br/>
wla-dx        ⇒  [https://github.com/vhelin/wla-dx](https://github.com/vhelin/wla-dx) (`wla-z80`, `wlalink`) <br/>
python 3      ⇒  [https://www.python.org/downloads/](https://www.python.org/downloads/) <br/>
numpy         ⇒  [https://numpy.org/](https://numpy.org/) <br/>
pypng         ⇒  [https://pypi.org/project/pypng/](https://pypi.org/project/pypng/) <br/>
sqlite3       ⇒  [https://www.sqlite.org/index.html](https://www.sqlite.org/index.html) <br/><br/>

---

## Installation on Linux

<br/>

```bash
sudo apt-get update
sudo apt-get install mame srecord cmake build-essential python3 python3-pip python3-numpy python3-pil
pip3 install pypng
git clone https://github.com/vhelin/wla-dx
cd wla-dx && cmake . && make && sudo cp binaries/wla-z80 binaries/wlalink /usr/local/bin/
```

Place the SDK in `~/neogeosdk` and the m68k toolchain in `~/x-tools`. <br/>
Set `SDKHOME` to the parent directory: <br/>

```bash
export SDKHOME=~
```

---

## Build Targets

<br/>

```
make            : compile game ROM (p1)
make all        : art + sfix + sound + p1 (full rebuild)
make sound      : rebuild all sound ROMs (samples + vrom + fmpatches + fm + mml + ssgconfig + ssg + m1rom)
make sound-all  : alias for make sound
make vrom       : V ROM only (pack ADPCM samples)
make m1rom      : M1 ROM only (Z80 driver; depends on fmpatches fm mml ssgconfig ssg)
make m1rom-asm  : build and snapshot the authoritative ASM M1 ROM
make m1rom-c    : build the experimental C-driver M1 ROM
make compare-driver : build ASM + C M1 ROMs and compare them
make mml        : compile MML music data only (sound/mml → music_data.inc)
make fm         : compile FM MML data only (sound/fm → fm_data.inc)
make fmpatches  : compile FM patch bank (sound/fm/patches.fm → fm_patch_table.inc)
make ssg        : compile SSG MML data only (sound/ssg → ssg_data.inc)
make ssgconfig  : compile SSG config (sound/ssg/config.ssg → ssg_config.inc)
make samples    : re-encode raw WAVs to ADPCM-A and ADPCM-B
make art        : rebuild C ROMs from artbox pipeline
make sfix       : rebuild S1 (FIX) ROM from infix images
make srom       : alias for make sfix
make test       : run in MAME
make debug      : run in MAME with debugger
make clean      : remove game ROM build artifacts
make sound-clean: remove sound ROM artifacts
make clean-all  : full clean
```

---

## Artbox Graphics Pipeline

The Artbox pipeline converts PNG images to Neo Geo formats (Sprites, FIX tiles).

*   **img2neo.py**: PNG → Neo Geo colour space; k-means clustering for palette reduction, Floyd-Steinberg dithering.
*   **romtiles.py**: Slices images into 16×16 sprite tiles or 8×8 FIX tiles; generates C ROMs.
*   **fixtiles.py**: Builds the 128 KB S1 (FIX) ROM. Preserves existing game tiles, seeds from `sfix.sfix` system fonts, then overlays custom tiles from `artbox/infix/`.
*   **createromdb.py**: Initialises the `neorom.db` SQLite asset database.

Place sprite source images in `artbox/in/` and fix-layer images in `artbox/infix/`. Run `make art` to rebuild sprite ROMs or `make sfix` for the fix ROM.

---

## Sound Subsystem (YM2610)

The SDK includes a custom Z80 sound driver for the YM2610 (OPNB) chip.

*   **ADPCM-A**: 6-channel sample playback, per-channel volume at maximum (TL=0, both pan bits set).
*   **ADPCM-B**: Delta-T streaming sample channel with linear volume control.
*   **SSG/MML**: Music Macro Language engine on Timer B IRQ for PSG melody tracks. Standalone SSG sequencer with preset control (`playSSGTrack`, `soundSetSSGPreset`).
*   **FM**: Compiled FM sequencer with table-driven patch bank (16 patches). `playFMTrack(n)` selects from up to 8 compiled FM tracks. `soundSetFMVolume` controls the FM output level. Patches defined in `sound/fm/patches.fm` and compiled to `fm_patch_table.inc`.
*   **Handshake**: Reliable 68000→Z80 command protocol via NMI and a 32-byte FIFO.
*   **Scene helpers**: `soundSceneReset()`, `soundPlayTitleMusic()`, `soundPlayGameLoop()`, `soundPlayDemoFM()` for clean scene transitions.
*   **Named sound IDs**: `sdk/sound_ids.h` provides stable track / cue / voice / bed identifiers for 68k-side code.
*   **Runtime sync**: `make sound` and `make m1rom` copy rebuilt `M1`, `V1`, and `sm1` outputs into `roms/ssideki/` for direct MAME testing.

Primary driver source: `sound/driver/driver.asm`. An experimental high-level C port also exists in `sound/driver/driver.c`, compiled through `z80c-special/` with the same generated music, FM, and SSG tables. Rebuild with `make m1rom`, or use `make m1rom-c` to assemble the C-driver runtime.

---

## Repository Overview

```
main.c          — Demo entry point (68k)
user.c          — Game startup / DEMO_GAME / START_GAME hooks
sdk/            — Hardware headers, linker scripts, neogeolib
  sound_ids.h   — Named sound track / sample / cue identifiers
artbox/         — Graphics conversion tools and asset pipeline
  in/           — Sprite source PNGs
  infix/        — FIX layer source PNGs
sound/          — Z80 driver, MML source, ADPCM samples
  driver/       — driver.asm, driver.c, generated YM2610 tables
  fm/           — FM MML source tracks (.mml) and FM patch bank (patches.fm)
  mml/          — SSG/MML music source tracks (.mml)
  ssg/          — Standalone SSG MML tracks (.mml) and config (config.ssg)
  samples/      — ADPCM-A / ADPCM-B raw and encoded audio
  tools/        — vrom.py, mml_compile.py, fm_compile.py, fm_patch_compile.py,
                  ssg_compile.py, ssg_config_compile.py, encoder scripts
roms/ssideki/   — Generated ROM output (mame rompath target)
out/            — Intermediate build objects
docs/           — YM2610 datasheet and reference materials
traces/         — Z80/68k execution traces for debugging
z80c-special/   — Experimental Z80 C compiler used by the C-driver path
```

---
