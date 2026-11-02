# Neo Geo CD Native Pipeline & Testing Guide

## Overview

The native Neo Geo CD pipeline for [neogeosdk](../) builds CD programs, authors
discs and packs them as one BIN/CUE per disc. Four games have CD ports:
**[Maiya: Super Nature Girl](../games/maiya)**, **[Sky Lance](../games/skylance)**,
**[Maiya & Luna](../games/maiyaluna)** and **[No Signal Z Alpha](../games/nosignal)**,
each with Red Book CD-DA music, ADPCM-A sound effects and native CD sprite,
FIX and sound uploads.

The support is **experimental**. Every disc is checked in emulation with two
system ROMs: the stock CDZ firmware (supplied by the user, never included) and
**EagleBIOS-CD**, the SDK's own CD firmware built from `bios/cd`. Burnt discs,
optical drive emulators and real Neo Geo CD consoles have not been tested.

---

## 1. Neo Geo CD Pipeline Architecture

```
Cartridge Assets (C1/C2, S1, ADPCM-A, ADPCM-B)
       │
       ├── C1 / C2 Sprites  ──► tools/neocd/formats.py ──► SPR0.SPR .. SPR3.SPR (4 MB DRAM)
       ├── S1 FIX Layer     ──► Native CD FIX Bank     ──► GAME.FIX (128 KiB)
       ├── ADPCM-A SFX/Vox  ──► CD Sample Table Builder ──► SFX.PCM (512 KiB DRAM) + SOUND.Z80
       ├── ADPCM-B Music    ──► FFmpeg CD-DA Encoder   ──► Track02.wav .. Track10.wav (Red Book)
       └── 68K Game Code    ──► sdk/platform/cd/       ──► GAME.PRG (Linked with neocd.ld)
                                       │
                                       ▼
                       tools/neocd/build_disc.py (ISO9660 + IPL.TXT)
                                       │
                                       ▼
                             game.iso + game.cue + audio/
```

### Key Technical Implementations

- **Sprite DRAM Streamer ([`cartridge_sprites_to_spr`](../tools/neocd/formats.py))**:
  Neo Geo CD stores sprite data in ordered byte planes (`1, 0, 3, 2`) with the right 8-pixel half stored before the left 8-pixel half. The pipeline automatically transcodes interleaved cartridge C-ROM pairs into native CD `.SPR` banks without needing graphic artists to re-export sprites.
- **CDDA Music Coordinator ([`games/maiya/cd/audio.c`](../games/maiya/cd/audio.c))**:
  Cartridge ADPCM-B streaming is mapped to CD-DA audio commands dispatched through the Neo Geo CD BIOS command vector (`ng_cd_bios_cdda()`). Track durations are tracked to ensure loop boundaries.
- **Z80 Sound RAM Driver ([`games/maiya/cd/sound.s`](../games/maiya/cd/sound.s))**:
  A 6-channel ADPCM-A driver running on the Z80 receives SFX and voice requests through port `0x320000`, indexing into `SFX.PCM`.
- **Interrupts & VRAM Synchronization ([`games/maiya/cd/platform.c`](../games/maiya/cd/platform.c))**:
  CD firmware owns the primary interrupt vector table. When game execution starts, `BIOS_SYSTEM_MODE` bit 7 is set to route vertical blanking into `maiya_video_vblank()`, providing deferred VRAM palette and sprite queue commits.

---

## 2. Test & Verification Matrix

Checked on 2026-11-01 with fresh builds of all four discs, in MAME 0.264 (Linux)
and MAME 0.283 (Windows, through the test-folder launchers): each disc loads and
reaches its title screen and attract mode.

| Target | Firmware | Maiya | Sky Lance | Maiya & Luna | No Signal |
| :--- | :--- | :--- | :--- | :--- | :--- |
| MAME `neocdz` | stock CDZ (`neocd.bin`, CRC32 `df9de490`, user-supplied) | boots | boots | boots | boots |
| MAME `neocdz` | EagleBIOS-CD CDZ (`bios/cd`) | boots | boots | boots | boots |
| NeoCD HLE core (`neocd_libretro`) | built-in HLE | ran 2026-10-22 | ran 2026-10-22 | – | – |
| Burnt CD-R, SD loaders, real consoles | any | not tested | not tested | not tested | not tested |

The HLE core runs (`tools/neocd/qa_libretro.py`, optional) are startup
diagnostics only and never count as firmware QA.

EagleBIOS-CD has no memory card service yet: Maiya & Luna keeps its
checkpoints in RAM under it (see `games/maiyaluna/cd/README.md`). Use the
stock firmware when saves must persist.

---

## 3. Emulation & Simulator Testing

### Option A: Testing with MAME

`neocdz` needs `neocd.bin` and `000-lo.lo` in a `neocdz` folder on the ROM path.
For the stock profile these are your own files. For EagleBIOS-CD, put the
firmware in MAME's word order (the release's `roms/neocdz/neocd.bin`) next to
the SDK's own `bios/000-lo.lo`; MAME reports their checksums as unexpected and
runs them.

```bash
# stock CDZ firmware in roms-stock/neocdz:
mame neocdz -bios official -rompath roms-stock -cdrom maiya.cue

# EagleBIOS-CD in roms-eagle/neocdz:
mame neocdz -bios official -rompath roms-eagle -cdrom maiya.cue
```

Loading takes about 30 seconds of emulated disc reads before the title.

**Standard Controls**:
- `1`: Player 1 Start
- `5`: Coin / Select
- `Arrow Keys`: Directional Joystick (Up / Down / Left / Right)
- `Left Ctrl`: Button A (Jump / Primary Action)
- `Left Alt`: Button B (Throw / Attack)
- `Space`: Button C (Special Art / Bomb)
- `Left Shift`: Button D

### Option B: The MAME test folder

`tools/mame_bats.py` installs discs into the Windows test folder
(`C:\mame\neogeosdk`) and writes two launchers per game, also listed in
`MENU.bat` under each game:

```bash
python3 tools/mame_bats.py --dest /mnt/c/mame/neogeosdk \
    --cd maiya build/neocd/maiya/bincue/maiya.cue \
    --cd-bios build/neocd/bios/eagle_cd_cdz.rom
```

- `run_<game>_cd_eagle.bat`: the disc with EagleBIOS-CD from
  `tests\cd-bios\eagle\neocdz` (installed by `--cd-bios`, with the SDK's LO data).
- `run_<game>_cd_stock.bat`: the same disc with your own stock firmware from
  MAME's `roms\neocdz` folder. No stock firmware is copied or supplied.

Each firmware keeps its own settings and saves in `tests\<game>\cd\eagle` and
`tests\<game>\cd\stock`.

### Option C: Testing with NeoCD / Libretro

NeoCD HLE emulation can be executed either standalone or through RetroArch's `neocd_libretro` core:

```bash
# Automated QA test run for Maiya CD:
python3 tools/neocd/qa_maiya.py --core /path/to/neocd_libretro.so --build build/neocd/maiya --out out/qa_maiya

# Automated QA test run for Sky Lance CD:
python3 tools/neocd/qa.py --build build/neocd/skylance --out out/qa_skylance
```

---

## 4. Building a Disc, BIN/CUE, and Burning a CD-R

```bash
python3 tools/neocd/build.py --game maiya --out build/neocd/maiya
python3 tools/neocd/bincue.py --cue build/neocd/maiya/disc/maiya.cue \
    --out build/neocd/maiya/bincue --name maiya
make bios-cd CD_BIOS_BUILD_DIR=build/neocd/bios     # EagleBIOS-CD
```

`build.py` writes the ISO with WAV audio tracks and a cue (`disc/`).
`bincue.py` turns it into one `.bin` (track 1 as MODE1/2352 with EDC/ECC, the
audio tracks with their pregaps) and a `.cue`: the format optical drive
emulators (SD loaders) and burning programs take; they do not read a CHD.

Each disc is a mixed-mode disc: track 1 data, then 44.1 kHz stereo CD-DA
tracks (9 for Maiya and Sky Lance, 18 for Maiya & Luna and No Signal).

To burn one (untested on real consoles so far):

1. Open the `.cue` (not the `.bin`) in a burner that writes mixed-mode discs
   in disc-at-once mode (ImgBurn, CDRWIN, cdrdao).
2. Write at a low speed on good CD-R media.
3. Keep the `.cue` and `.bin` together; the `.cue` names the `.bin`.

---

## 5. Release Packages

Neo Geo CD archives are built at release time and published with the release
(`dist/release/` is not tracked):

- `<Game>-NeoGeoCD-BinCue_v<version>.zip`: the `.cue` and `.bin`, a README,
  the licence and SHA-256 sums, for every CD port.
- `EagleBIOS-CD_v<sdk>.zip`: the firmware in both byte orders.

No archive contains a stock system ROM.
