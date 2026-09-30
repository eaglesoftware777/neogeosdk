# v1.7.1 NeoSD Test Images

These are separate prerelease hardware-test assets. They do not replace the
working `Maiya-WIP-NeoSD_AES_v1.neo` and `Maiya-WIP-NeoSD_MVS_v1.neo` images.
Hardware confirmation on NeoSD and NeoSD Pro is still required.

## MVS Sound Only

`dist/release/Maiya-WIP-NeoSD_MVS_SOUND_FIX_PRERELEASE.neo`

Based on the exact MVS v1 image. Its header, S1, C, V1 and V2 are unchanged.
P1 changes by exactly one byte: `soundInit()` sends `$09`, rather than using the
BIOS-reserved `$01`. M1 implements the BIOS slot-switch handshake:

- `$01` silences the chip and disables/acknowledges YM timer interrupts.
- The ready reply is written from a RAM-resident routine at `$FF80`.
- The Z80 waits in RAM with interrupts enabled while M1 can be replaced.
- `$03` received during that wait restarts the selected driver.
- Ordinary game `$03` remains the existing soft reset.
- `$02` retains the existing boot-music handler.
- Incoming parameter state is tracked before FIFO dispatch. Parameter `$01`
  must never be mistaken for a BIOS switch request.

The normal playback routines, patches, tracks, samples and timer settings are
not retuned. The experimental C driver accepts game-init `$09`, but does not
implement this slot-switch fix; use the authoritative ASM path for MVS hardware.
Old binaries that send game-init `$01` must be rebuilt with the updated SDK.
Do not mix a newly compiled P1 with an old M1.

Reproduction after assembling the updated ASM driver and padding M1 to 128 KiB:

```sh
python3 tools/patch_neosd_sound.py \
  dist/release/Maiya-WIP-NeoSD_MVS_v1.neo \
  --m1 out/780-m1.m1 \
  --output dist/release/Maiya-WIP-NeoSD_MVS_SOUND_FIX_PRERELEASE.neo
```

This patcher is intentionally release-specific. It rejects an unexpected P1
instruction signature, incorrect region sizes, and a base without 8 MiB V1.

## AES Boot Compatibility

`dist/release/Maiya-WIP-NeoSD_AES_UNIBIOS_FIX_PRERELEASE.neo`

Based on the exact AES v1 image. M1 and V1/V2 remain byte-identical to that image.
No gameplay C tile moves. A previously unused C bank `$49` holds an original
Maiya boot logo; its tile `$FF` is blank. The AES cartridge header selects the
standard BIOS animation and bank `$49`.

The FIX boot area provides the system animation's text, publisher mark and
copyright glyph. The original game FIX tiles `$000-$2FF`, including colored
HUD graphics and font pixels, are copied unchanged into private tiles
`$D00-$FFF`. Gameplay references use that private range; the BIOS blank tile
`$FF` remains transparent. Existing imported FIX art in other banks is retained.

`ng_fix_set_ascii_base()` selects a 256-tile character page and invalidates the
text cache. Default `$000` preserves other games' layout; Maiya uses `$D00`.
Full-width tile calls are used for relocated HUD symbols, avoiding char truncation.
The font selection persists across scene clears; a clear never changes fonts.

```sh
make GAME=maiya PLATFORM=aes p1
python3 tools/patch_neosd_aes_boot.py \
  dist/release/Maiya-WIP-NeoSD_AES_v1.neo \
  --p1 roms/maiya/780-p1.p1 \
  --output dist/release/Maiya-WIP-NeoSD_AES_UNIBIOS_FIX_PRERELEASE.neo
```

The AES test-image patcher deliberately restores game-init `$01` in the compiled
P1 to match the preserved old AES M1. Normal new builds use `$09` with the new M1.
This is not a general-purpose P-ROM patcher; do not apply it to unrelated games.

## Padding and Container Layout

The previous 8 MiB V1 padding script and aligned baseline images were committed
in `00c0e46`. Both Python and native C packagers now pad nonempty V sample regions
to the next power of two, with a 64 KiB minimum. Maiya's `$79AB00` sample payload
therefore occupies `$800000` bytes. Padding is `$FF`; already aligned regions
are not extended again. P/S/M use 64 KiB alignment; C uses 256 KiB alignment.

Header lengths describe the actual padded payloads. C1 occupies even C bytes,
C2 odd C bytes. Padding does not change sample addresses or C-ROM tile order.
The Python `--legacy-alignment` option is an explicit diagnostic opt-out, not
the recommended hardware release path. The native CLI and its GUI use the
hardware policy automatically; rebuild the native executable after updating.

## Verification

Every test asset has a JSON manifest containing region sizes, SHA-256 hashes,
and unchanged-region flags. The MVS patcher asserts only one P1 byte changes.
The AES patcher asserts sample and driver preservation and exact game-font copying.

```sh
python3 -m unittest discover -s tests -p 'test_neosd*.py' -v
python3 -m unittest discover -s tests -p test_maiya_boot_assets.py -v
python3 -m unittest discover -s tests -p test_mvs_sound_handshake.py -v
```

The last suite requires WLA-DX and Z80Ex (`libz80ex1` on Ubuntu). A non-system
library can be selected with `Z80EX_LIB=/path/to/libz80ex.so.1`. It executes the
assembled driver and checks RAM acknowledgement, restart, parameter races,
timer continuity, and YM write parity against the release baseline. It does not
emulate YM audio synthesis or flashcart hardware.

MAME smoke testing additionally checks boot, title/game progression and captured
MVS audio. Hardware acceptance must check cold/warm boots, MVS slot switching,
coin/start, ADPCM-A/B, FM/SSG, and AES UniBIOS splash and menu readability.

## Windows MAME installation

Install the exact test-image regions and checksum-matched software lists:

```bat
py tools\install_maiya_mame_tests.py C:\mame\neogeosdk
```

The destination can be any existing MAME SDK installation. System BIOS files
are retained, and replaced launchers/test sets are backed up. Run
`run_maiya_mvs.bat` for the sound fix, or `run_maiya_aes.bat` for the UniBIOS 4.0
logo fix. Use `run_maiya.bat aes asia` for a standard AES BIOS instead.
Each platform has separate ROM/hash paths, configuration and NVRAM, preventing
the AES baseline audio driver from being mixed with the new MVS program.
