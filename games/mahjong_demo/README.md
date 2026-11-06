# Mahjong Demo

A small technical demonstration of the SDK's Mahjong engine
(`GAME_GENRE_ENGINE = mahjong`, `sdk/2d_engine/ng_mahjong.c`). Game ID 785.

- **Four-seat Mahjong, Hong Kong Old Style**: the player at East against
  three CPU seats; the engine deals, draws, decides for the CPU seats and
  resolves claims (chow, pung, kong, a win), and scores the hand.
- **Solitaire** on the classic turtle layout of 144 tiles: the engine deals
  so the layout can always be cleared, and keeps hints, undo and shuffles.
- **Rules**: what this demo implements, on screen.

In attract, four CPU seats play a hand against each other.

## Controls

At the table: LEFT / RIGHT choose a tile, A discards it, C declares a win or
a kong when one is possible, D asks the CPU for a hint. In a claim window
LEFT / RIGHT choose, A takes it and B passes. Holding B speeds the CPU turns
up.

Solitaire: the stick moves between free tiles, A picks (a second matching
tile removes the pair), B undoes, C shows a pair, D shuffles what is left,
START pauses (continue, deal again, back to the menu).

## How it is made

`scenes/mj_table.c` is the four-seat table, `scenes/mj_sol.c` the solitaire,
`scenes/mj_game.c` the title, attract, menu and rules, `scenes/mj_draw.c`
the screen. `tools/art.py` draws the tiles (sprite tiles for the hands and
the solitaire, two-cell FIX mini tiles for the discards); `tools/sound.py`
renders the music and effects before `make`.

## Build

```sh
python3 games/mahjong_demo/tools/sound.py
make GAME=mahjong_demo GAME_CFG_FILE=games/mahjong_demo/game.cfg all
make unit-tests                                # includes tests/mahjong_test.c
```
