# Sky Lance

SDK v1.7.3 pre-release: opt-in vertical camera/parallax support, widened
terrain, cleaned sprite pixels, 80x96 pilots and a metallic squadron title.
See [Vertical Shooters](../../docs/vertical_shooters.md) for the additive API.

A vertical arcade shooter for the Neo Geo, built on this SDK's 2D engine.
Three pilots, seven stages, one named boss per stage.

## Build

Sky Lance keeps its own `game.cfg` so the repo-root one can stay pointed at
whatever game you are working on:

```sh
make GAME=skylance GAME_CFG_FILE=games/skylance/game.cfg art     # direct C-ROMs + asset tables
make GAME=skylance GAME_CFG_FILE=games/skylance/game.cfg sfix    # S-ROM (HUD tiles)
make GAME=skylance GAME_CFG_FILE=games/skylance/game.cfg vrom m1rom
make GAME=skylance GAME_CFG_FILE=games/skylance/game.cfg p1      # program ROM
```

`make ... sound` is the full audio path and re-encodes from `sound/samples/in_wav_*`,
which this game does not ship - see `sound/README.md`.  Use `vrom m1rom` instead.

Run it with `-cart1 skylance` and the hash path `hash_eagle/skylance`.

## Layout

| File | What it holds |
|------|---------------|
| `scenes/sky_draw.c`   | asset binding, the scrolling backdrop, the FIX HUD, the frame pump |
| `scenes/sky_stage.c`  | enemy roster, stage table, wave director, boss AI |
| `scenes/sky_sortie.c` | attract reel, pilot select, the flight loop, collisions, scoring |
| `user.c`              | BIOS handshake, FIX palettes, MVS title |
| `main.c`              | engine lookup hooks for the generated asset tables |

## Art

`artbox/in/` is split into the SDK's category folders - `backgrounds/`,
`characters/`, `npcs/` - because the flat layout falls through to the
screen-mode fallback rule and sprites lose their transparency handling.

`tools/art.py` generates C1/C2 directly using the shared palette/tile codecs,
without an intermediate ZIP or generated `screens.c`. Each lane is 4 MiB.
Boot tiles at `0x4900..0x49FF` and utility tiles at `0xFFFD..0xFFFF` are reserved.
The cartridge packer checks that conversion preserves each lane byte for byte.

Seven native 352x512 terrain variants use two resident 22-strip pages, without
reflecting the map. The original art is retained; craft and opponents are
48x48, portraits 80x96 and bosses 112x112. Aircraft have banked poses and
explosions have six dedicated 32x32 pixel frames with a shared palette.
Preview PNGs and metadata are in `artbox/generated`.

## Playfield

The arena now covers the full 320x224 image, without black side pillars.
This is detailed native Neo Geo art, not an HD video mode.

## Colour and effects

The direct builder owns the palette budgets. Bosses use up to 16 banks,
ordinary opponents 3 and player craft 4. Scene-exclusive assets share banks
to fit the 104-bank vblank update budget, including the sparse cloud layer.
Pilots and aircraft no longer share an unrelated animation palette. Bindings
in `sky_draw.c` carry the per-tile map for characters and both background pages.

Enemy shots use a compact independent shrink preset and a 6x6 collision core.
Explosion frames stay anchored at the impact position, with no scale overflow.
Sprite/FIX writes are deferred to vblank and gameplay fonts use private tiles.

## Hardware And Installation

The shared Z80 driver retains MVS slot-switch and timer recovery fixes.
Sound starts after work-RAM clearing; coin sounds do not stop music.
AES Start and MVS credits use the detected BIOS machine mode. Cartridge ID
is `0x0779`; backup RAM is reserved outside engine BSS. AES/MVS boot headers,
ROM sizes, blank tiles and sample alignment are verified during packaging.

```sh
python3 tools/build_skylance_mame.py /mnt/c/mame/neogeosdk
python3 -m unittest discover -s tests -p 'test_skylance_assets.py'
python3 games/skylance/tools/qa.py --platform mvs --bios eagle
python3 games/skylance/tools/qa.py --platform aes --bios stock
```

The installer backs up the previous Sky Lance set and leaves Maiya alone.
Open `run_skylance_mvs.bat` or `run_skylance_aes.bat` in the installed folder.
See `TESTING.txt` for controls and BIOS options. Separate `.neo` cartridges
are under `tests/skylance/aes` and `tests/skylance/mvs`.

The capture runner needs Windows MAME at `/mnt/c/mame/mame.exe` and BIOS ROMs.
It checks visible gameplay, audio, the Z80 protocol and timer, saving PNG/WAV
captures under `tests/skylance/qa`. `--stage 0..6` selects an inspection sector
with temporary invulnerability; normal runs do not modify game RAM.
Emulator checks do not replace real-board testing.
