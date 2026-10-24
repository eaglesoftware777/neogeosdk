# Maiya & Luna V2 on Neo Geo CD

This native CD port uses the released V2 game, renderer, world tables and
artwork unchanged. It preserves both journeys, the 46 rooms, puzzles,
notebook, character scale and controller behavior. `v2_reference.json` checks
the source and graphics against that baseline before and after conversion.
The cartridge build does not link this directory.

Build from the SDK root:

```sh
python3 tools/neocd/build.py --game maiyaluna --out out/neocd/maiyaluna-v2
python3 -m unittest discover -s tests -p 'test_maiyaluna_cd.py'
python3 games/maiyaluna/tools/check_world.py
```

A normal 68000 toolchain, WLA-DX and FFmpeg are required. Existing V2 sound
inputs must be present. No artwork authoring or palette fitting runs. Every
used sprite byte and the utility tiles are converted losslessly to four
1 MiB SPR banks. All 26 effects retain ADPCM-A playback; the 18 existing
music tracks use CD audio in their original order. CD data uses game ID 783.

The CD program retains the V2 save format and the console notebook. With a
stock CDZ system ROM it uses the system's internal virtual memory card.
Standalone EagleBIOS CD currently has no SYS_CARD service: the port avoids
calling a missing vector and retains session checkpoints in RAM. Persistent
saving under that firmware remains unavailable; use the stock profile when
it is required. Neither package includes proprietary firmware.

The disc is supplied as ISO/CUE with WAV audio and as CHD. Keep every CUE
file and its audio directory together. Use the stock package with stock CDZ
firmware and the EagleBIOS package with the separate EagleBIOS CD firmware.
Emulator results do not certify physical hardware compatibility.
