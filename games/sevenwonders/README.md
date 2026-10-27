# Seven Wonders Puzzles

A native Neo Geo puzzle game built on the SDK's puzzle engine
(`GAME_GENRE_ENGINE = puzzle`, `sdk/2d_engine/ng_puzzle.c`). Game ID 782.

Seven wonders, five levels each: the Sky Garden, the Crystal Canal, the Moon
Lighthouse, the Clockwork Tower, the Coral Palace, the Sunstone Pyramid and
the Starlight Dome. Each has its cat, its mood and its goals (match colours,
break ice, make special pieces, clear crates, rescue kittens floating in
bubbles), in three kinds of level: matching pieces on a board, launched
bubbles, and a picture to finish from its number clues, which ends each
chapter and shows the wonder's picture in colour. 35 levels, with stars and
best scores kept in the save block.

The look is a cute Japanese one: a candy logo with a cat-headed O and a
kana ribbon, round puffball cats with big shining eyes, candy pieces with
little faces on a pink gingham board, soft berry frames, and a pastel title
scene of cherry blossom and the seven wonders on the horizon.

What keeps a player coming back:

- Cheering words pop up over the board in candy kana: いいね! (nice) for a
  first chain, a finished line or a big pop, すごい! for a three-chain, a
  special piece or a big drop, すてき! for a four-chain or a kitten freed,
  にゃんだふる! from five, and かんせい! when a picture is done.
- After every level won, the lucky paw: a beckoning cat's roulette of
  500, 1000 or 3000 points or a paw (one more hammer or hint next level).
  A button stops it; the more stars the level earned, the better the odds.
- The picture album: each chapter's finished picture, with its stars; the
  attract shows it, and a picture finished for the first time opens it.
- The chapter card shows the stars won there so far, of fifteen.
- The best five scores with three-letter names (BEST CATS), kept in the save
  block and shown in the attract; a game that makes the table asks for a
  name. The attract turns between the table, the album and the stars of
  each wonder, and between a match board and a picture as its demonstration.

## Controls

The joystick moves the paw (match, pictures) or aims (bubbles). A picks up a
piece, then the joystick pushes it into its neighbour, or A fires. B puts the
piece down, or swaps the loaded and the next bubble. C is the paw hammer,
once a level: it breaks the piece under the paw, or the bubble the aim points
at. D shows a hint (match) or switches the aim guide (bubbles). On a console
START pauses.

On a picture, A fills the square under the paw and B crosses it out (or
takes a cross away); held down, they go on along the paw's way. Each number
beside a row or above a column is a run of filled squares, in order. A
square filled outside the picture is a mistake: it is crossed out and costs
a heart, and the filling stops until A is let go. A finished line crosses
out the rest of itself and its clue turns grey. C shows the row and column
under the paw, once a picture. Five hearts; the last one lost, or the time
running out, ends the try.

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
  pictures, goals, moves) and the attract demonstrations; `tools/levels.py`
  writes `scenes/sw_levels_data.h` (the build's level builder). It rejects a
  picture that is not 5 x 5 or 10 x 10, has more than four runs in a line
  (the room the clues have on screen) or needs a guess: every picture must
  solve from its clues by line logic alone.
- `scenes/sw_game.c` is the flow of the game, `scenes/sw_draw.c` the screen.
- `tools/art.py` draws and packs the sprites; painted scenes are imported
  with `tools/paint_import.py`. `tools/logo.py --fonts DIR` renders the logo
  into `artbox/painted/logo.png` with the M PLUS Rounded 1c fonts (SIL Open
  Font License; only the rendered picture is kept). `tools/sound.py` renders
  the music and effects before `make`.

## Build and check

```sh
python3 games/sevenwonders/tools/sound.py      # when the music changes
make GAME=sevenwonders GAME_CFG_FILE=games/sevenwonders/game.cfg all
python3 games/sevenwonders/tools/check_levels.py --seeds 40
```

`check_levels.py` checks every level with `ng_puzzle_level_check()` and plays
it over many seeds with the engine's own hints (a picture: its squares in a
random order, with a few crosses and mistakes). It fails if a level is
rejected, stays busy after settling (a softlock) or is never won; the bot is
far weaker than a player, so its win rates are a floor.

`tools/levels.py --check` validates without rewriting generated data. The
game rejects boards beyond its 7x7 match / 10x12 bubble / 5x5 or 10x10
picture reservations before initializing or binding them. SDK-valid larger
boards need a renderer redesign.

The save block keeps the first release's records where they were (four
levels a chapter), adds the pictures after them and the best five scores
after those, so best scores carry over. Its last byte (past the checked
data) remembers which demonstration the attract shows next.

## Test build (never install it)

`-DSW_AUTOPLAY=1` added to `GAME_ENGINE_DEFINES` (keep the game's own
defines) lets the hint bot play every level, so `tools/genre_qa.py` can run
the whole game in MAME to the ending.
