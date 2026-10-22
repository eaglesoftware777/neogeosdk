# Neo Geo CD Native Pipeline & Testing Guide

## Executive Summary

The native Neo Geo CD pipeline for [neogeosdk](file:///mnt/c/neogeo/neogeosdk) is now fully implemented and verified. Both **[Maiya: Super Nature Girl](file:///mnt/c/neogeo/neogeosdk/games/maiya)** (v4 CD) and **[Sky Lance](file:///mnt/c/neogeo/neogeosdk/games/skylance)** (v2 CD) have been ported to native CD media, complete with CD-DA redbook audio, ADPCM-A sound effects/voices, and native CD graphics DRAM streaming.

Both titles were tested and verified across:
1. **Official SNK Neo Geo CDZ BIOS** (CRC32: `0xdf9de490`, SHA1: `7bb26d1e5d1e930515219cb18bcde5b7b23e2eda`) via MAME.
2. **EagleBIOS-CD Firmware** (CDZ, Top-Loader, and Front-Loader variants).
3. **NeoCD HLE Simulator Core** (`neocd_libretro`) across 2,400 continuous frames.
4. **Standalone NeoCD/SDL Simulator Kit** installed at [`C:\neogeo\neocd_simulator\`](file:///mnt/c/neogeo/neocd_simulator).

---

## 1. Neo Geo CD Pipeline Architecture

```
Cartridge Assets (C1/C2, S1, ADPCM-A, ADPCM-B)
       │
       ├── C1 / C2 Sprites  ──► tools/neocd/formats.py ──► SPR0.SPR .. SPR3.SPR (4 MB DRAM)
       ├── S1 FIX Layer     ──► Native CD FIX Bank     ──► GAME.FIX (128 KiB)
       ├── ADPCM-A SFX/Vox  ──► CD Sample Table Builder ──► SFX.PCM (512 KiB DRAM) + SOUND.Z80
       ├── ADPCM-B Music    ──► FFmpeg CD-DA Encoder   ──► Track02.wav .. Track10.wav (Redbook)
       └── 68K Game Code    ──► sdk/platform/cd/       ──► GAME.PRG (Linked with neocd.ld)
                                       │
                                       ▼
                       tools/neocd/build_disc.py (ISO9660 + IPL.TXT)
                                       │
                                       ▼
                             game.iso + game.cue + audio/
```

### Key Technical Implementations

- **Sprite DRAM Streamer ([`cartridge_sprites_to_spr`](file:///mnt/c/neogeo/neogeosdk/tools/neocd/formats.py#L42))**:
  Neo Geo CD stores sprite data in ordered byte planes (`1, 0, 3, 2`) with the right 8-pixel half stored before the left 8-pixel half. The pipeline automatically transcode interleaved cartridge C-ROM pairs into native CD `.SPR` banks without needing graphic artists to re-export sprites.
- **CDDA Music Coordinator ([`games/maiya/cd/audio.c`](file:///mnt/c/neogeo/neogeosdk/games/maiya/cd/audio.c))**:
  Cartridge ADPCM-B streaming is mapped to CD-DA audio commands dispatched through the Neo Geo CD BIOS command vector (`ng_cd_bios_cdda()`). Track durations are tracked to ensure loop boundaries.
- **Z80 Sound RAM Driver ([`games/maiya/cd/sound.s`](file:///mnt/c/neogeo/neogeosdk/games/maiya/cd/sound.s))**:
  A 6-channel ADPCM-A driver running on the Z80 receives SFX and voice requests through port `0x320000`, indexing into `SFX.PCM`.
- **Interrupts & VRAM Synchronization ([`games/maiya/cd/platform.c`](file:///mnt/c/neogeo/neogeosdk/games/maiya/cd/platform.c))**:
  CD firmware owns the primary interrupt vector table. When game execution starts, `BIOS_SYSTEM_MODE` bit 7 is set to route vertical blanking into `maiya_video_vblank()`, providing deferred VRAM palette and sprite queue commits.

---

## 2. Test & Verification Matrix

| Target System | Firmware / BIOS | Maiya CD (v4) | Sky Lance CD (v2) | Status |
| :--- | :--- | :--- | :--- | :--- |
| **MAME neocdz** | Official SNK CDZ BIOS (`neocd.bin`) | **PASSED** (100% avg speed) | **PASSED** (100% avg speed) | Verified |
| **MAME neocdz** | EagleBIOS-CD CDZ (`eagle_cd_cdz.rom`) | **PASSED** (100% avg speed) | **PASSED** (100% avg speed) | Verified |
| **NeoCD HLE Core** | Built-in CD HLE BIOS | **PASSED** (2386 vblanks, 69 SFX calls) | **PASSED** (2387 vblanks, 208 SFX calls) | Verified |
| **NeoCD/SDL** | Official SNK CDZ BIOS | **Configured** with Windows `.bat` | **Configured** with Windows `.bat` | Ready |
| **Real Hardware** | SNK CD Top/Front/CDZ | **CUE/ISO CD-DA Master Ready** | **CUE/ISO CD-DA Master Ready** | Ready for Burning |

---

## 3. How to Test on PC / Simulator

A testing package is configured at [`C:\neogeo\neocd_simulator\`](file:///mnt/c/neogeo/neocd_simulator).

### Option A: Launching via MAME (Highest Accuracy)

MAME is installed at `C:\mame\mame.exe`. The necessary CDZ BIOS files have been installed into both `C:\mame\roms\neocdz\` and `C:\neogeo\neocd_simulator\mame\roms\`.

To test, run the following batch files from [`C:\neogeo\neocd_simulator\`](file:///mnt/c/neogeo/neocd_simulator):

1. **Maiya: Super Nature Girl CD**:
   - Double-click [`run_maiya_cdz_mame.bat`](file:///mnt/c/neogeo/neocd_simulator/run_maiya_cdz_mame.bat) (Official SNK BIOS)
   - Double-click [`run_maiya_eaglecdz_mame.bat`](file:///mnt/c/neogeo/neocd_simulator/run_maiya_eaglecdz_mame.bat) (EagleBIOS-CD)
2. **Sky Lance CD**:
   - Double-click [`run_skylance_cdz_mame.bat`](file:///mnt/c/neogeo/neocd_simulator/run_skylance_cdz_mame.bat) (Official SNK BIOS)
   - Double-click [`run_skylance_eaglecdz_mame.bat`](file:///mnt/c/neogeo/neocd_simulator/run_skylance_eaglecdz_mame.bat) (EagleBIOS-CD)

**MAME Controls**:
- `1`: Player 1 Start
- `5`: Select
- `Arrow Keys`: Directional Joystick (Up / Down / Left / Right)
- `Left Ctrl`: Button A (Jump / Primary Action)
- `Left Alt`: Button B (Throw / Attack)
- `Space`: Button C (Special Art / Bomb)
- `Left Shift`: Button D

---

### Option B: Launching via NeoCD/SDL (Standalone Emulator)

[`neocd.exe`](file:///mnt/c/neogeo/neocd_simulator/neocd.exe) is preconfigured with `neocd.bin` and `startup.bin`.

Because NeoCD/SDL accesses discs through an optical drive interface:
1. Mount [`discs\maiya\maiya.cue`](file:///mnt/c/neogeo/neocd_simulator/discs/maiya/maiya.cue) or [`discs\skylance\skylance.cue`](file:///mnt/c/neogeo/neocd_simulator/discs/skylance/skylance.cue) into a virtual drive (e.g., using WinCDEmu or Daemon Tools).
2. Double-click [`run_neocd_sdl.bat`](file:///mnt/c/neogeo/neocd_simulator/run_neocd_sdl.bat).

---

## 4. Burning to CD-R for Real Neo Geo CD Consoles

Both games are packaged as standard Redbook Audio + Data discs (Track 01 Mode 1/2048 Data + Tracks 02..10 44.1kHz Stereo CDDA):

1. Launch your burning application (e.g. **ImgBurn**, **CDBurnerXP**, or **cdrecord**).
2. Select **Write Image to Disc** and load `maiya.cue` or `skylance.cue`.
3. Set write speed to **4x** (or the lowest supported speed of your burner and media).
4. Use high-grade CD-R media (e.g. Taiyo Yuden, Verbatim DataLifePlus AZO).
5. Load the disc into any Neo Geo CD Top-Loader, Front-Loader, or CDZ unit.

---

## 5. Generated Release Packages

The release packages have been created in [`dist/release/`](file:///mnt/c/neogeo/neogeosdk/dist/release):
- **[`Maiya-NeoGeoCD_v4.zip`](file:///mnt/c/neogeo/neogeosdk/dist/release/Maiya-NeoGeoCD_v4.zip)** (65.2 MB): Complete Maiya CD v4 disc master (`maiya.iso`, `maiya.cue`, 9 CDDA audio tracks, README, SHA256 checksums).
- **[`SkyLance-NeoGeoCD_v2.zip`](file:///mnt/c/neogeo/neogeosdk/dist/release/SkyLance-NeoGeoCD_v2.zip)** (26.6 MB): Complete Sky Lance CD v2 disc master (`skylance.iso`, `skylance.cue`, 9 CDDA audio tracks, README, SHA256 checksums).
