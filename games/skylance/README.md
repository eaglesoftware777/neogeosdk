# Sky Lance

A vertical arcade shooter for the Neo Geo, built on this SDK's 2D engine.
Three pilots, seven stages, one named boss per stage.

## Build

Sky Lance keeps its own `game.cfg` so the repo-root one can stay pointed at
whatever game you are working on:

```sh
make GAME=skylance GAME_CFG_FILE=games/skylance/game.cfg art     # C-ROMs + screens.c
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
| `main.c`              | generated `showScreenN()` art entry points (rewritten by `make art`) |

## Art

`artbox/in/` is split into the SDK's category folders - `backgrounds/`,
`characters/`, `npcs/` - because the flat layout falls through to the
screen-mode fallback rule and sprites lose their transparency handling.

The two backdrops were resized to 256x256 and cross-faded across the wrap so
they tile top-to-bottom: a page is 16x16 tiles and the sprite chip can only
shrink, never stretch, so a scrolling backdrop has to be exactly one page tall
and seamless at the join.

## Playfield

The arena is the 256-px-wide column the backdrop covers, centred on the 320-px
screen, with the 32 px either side left as backdrop.  That is a hardware
consequence, not a style choice - see `scenes/sky.h`.
