# Seven Wonders Puzzles

A native Neo Geo puzzle game built on the SDK's puzzle engine
(`GAME_GENRE_ENGINE = puzzle`, `sdk/2d_engine/ng_puzzle.c`). Game ID 782.

Seven wonders, four levels each: the Sky Garden, the Crystal Canal, the Moon
Lighthouse, the Clockwork Tower, the Coral Palace, the Sunstone Pyramid and
the Starlight Dome. Each has its cat, its mood and its goals (match colours,
break ice, make special pieces, clear crates, rescue kittens floating in
bubbles), in two kinds of level: matching pieces on a board, and launched
bubbles. 28 levels, with stars and best scores kept in the save block.

## Controls

The joystick moves the paw (match) or aims (bubbles). A picks up a piece,
then the joystick pushes it into its neighbour, or A fires. B puts the piece
down, or swaps the loaded and the next bubble. C is the paw hammer, once a
level: it breaks the piece under the paw, or the bubble the aim points at. D
shows a hint (match) or switches the aim guide (bubbles). On a console START
pauses.

Aim can move while a bubble is flying to prepare the next shot; the flying
bubble keeps its original trajectory. Fire, loaded/next swap and the hammer
require a ready board. The hammer hits the first bubble along the aim path;
an anchor or a clear path to the ceiling leaves the hammer available.
Match cursor repeat uses a fresh delay after a direction reversal, with
independent horizontal and vertical timing.
The aim guide extends from a short direction pointer to its predicted landing
as collision checks finish. Board changes discard the old prediction; steady
aim reuses the completed path.

## How it is made

- `levels/levels.json`: the chapters, the levels (board layouts, bubble rows,
  goals, moves) and the attract demonstration; `tools/levels.py` writes
  `scenes/sw_levels_data.h` (the build's level builder).
- `scenes/sw_game.c` is the flow of the game, `scenes/sw_draw.c` the screen.
- `tools/art.py` packs the pictures; painted ones are imported with
  `tools/paint_import.py`. `tools/sound.py` renders the music and effects
  before `make`.

## Build and check

```sh
python3 games/sevenwonders/tools/sound.py      # when the music changes
make GAME=sevenwonders GAME_CFG_FILE=games/sevenwonders/game.cfg all
python3 games/sevenwonders/tools/check_levels.py --seeds 40
```

`check_levels.py` checks every level with `ng_puzzle_level_check()` and plays
it over many seeds with the engine's own hints. It fails if a level is
rejected, stays busy after settling (a softlock) or is never won; the bot is
far weaker than a player, so its win rates are a floor.

`tools/levels.py --check` validates without rewriting generated data. The
game rejects boards beyond its 7x7 match / 10x12 bubble reservations before
initializing or binding them. SDK-valid larger boards need a renderer redesign.

## Test build (never install it)

`-DSW_AUTOPLAY=1` added to `GAME_ENGINE_DEFINES` (keep the game's own
defines) lets the hint bot play every level, so `tools/genre_qa.py` can run
the whole game in MAME to the ending.
