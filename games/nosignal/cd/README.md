# No Signal Z Alpha V2 on Neo Geo CD

This native CD port preserves the V2 game, three heroes, eight stages, console
modes, controls, character frames and palettes. It compiles the existing game
and beat-'em-up engine for CD; it does not rename a cartridge program or rebuild
the artwork. Cartridge builds exclude the CD hooks.

All fighters, portraits and interface graphics remain resident. The 13 scenery
assets share one upload area and load while the scene is hidden. Every scenery
transfer is checked by reading back sprite RAM and comparing CRC32. The total
layout stays within the CD's 4 MiB sprite RAM. The original 47 ADPCM-A effects
and voices use sound RAM, and the 18 original music files become CD audio tracks
in the same order.

## Build

```sh
python3 tools/neocd/build.py --game nosignal --out out/neocd/nosignal-v2
python3 -m unittest discover -s tests -p 'test_nosignal*.py'
python3 -m unittest discover -s tests -p 'test_cd_*.py'
```

The build requires the 68000 toolchain, WLA-DX, FFmpeg and the existing V2 sound
inputs. `v2_reference.json` checks the game, graphics and sound inputs before
and after conversion. The generated asset header relocates tiles without
changing palettes, frame geometry or pixels.

## Test

`tools/neocd/qa_nosignal.py` uses MAME with explicitly supplied stock CDZ or
EagleBIOS-CDZ firmware. It checks actual controller input, all three heroes,
movement, jumping, sound, music, and exact program/firmware readback. Use
`--media` to repeat against a CHD and `--startup-script` to test its launcher.

The separate `--media-audit --no-cdda` build exercises all 13 scenery transfers
and checks that the complete resident fighter/interface graphics survive.
It is a diagnostic, and release packaging rejects it.

The source build supplies ISO/CUE/WAV media. The compact release contains the
complete CHD plus a helper to extract a burnable mixed-mode CUE/BIN image. Stock and EagleBIOS launchers
use the same game image with separate configuration and save directories.
The stock drive-menu helper releases all forced controls before play.
Supply your own legitimate firmware and LO data; neither is included.

Both BIOS profiles have passed native MAME CDZ tests. Physical hardware,
front/top-loading models, full campaigns and power-cycle save persistence
remain unverified. Progress and unlocks are available during the running
session. These limits are not hardware compatibility certification.
