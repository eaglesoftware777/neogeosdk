# Seven Wonders Puzzles — changelog

The ROM sets ship with the NeoGeoSDK pre-releases and are numbered on their
own: `_v1` with v1.7.7.

## v1.0.1 (2026-10-31, NeoGeoSDK v1.7.7 pre-release)

- The 2026-10-26 v1 update under a number of its own. The name v1 went out
  as three different builds; v1.0.1 names this one only
  (docs/releases/VERSIONING_POLICY.md). No change to the game.

## v1 update (2026-10-26, NeoGeoSDK v1.7.7 pre-release, same file names)

- Every HUD, text line and board now stays inside columns 3-36 of the 40
  (x 24-295), the part of the picture every CRT shows: the side panel, the
  picture plate and its corner box, the cats beside the boards, the title
  and the ending.
- Softer music: a music-box voice and a gentle mix for the quiet songs.
- A new kind of level, the picture: number clues give the runs of squares
  in each row and column, and the finished picture shows in colour. One
  ends each chapter (a tulip, a fish, the moon and a star, an alarm clock,
  an octopus, the pyramid and, last, Mimi), five hearts and a paw hint each;
  every picture solves without a guess. 35 levels now.
- A cute Japanese look: a new logo with a cat-headed O and a kana ribbon,
  readable on any background; round puffball cats with big shining eyes and
  blushing cheeks in all six moods, and big cats bobbing on the title; candy
  pieces with little faces (the clover has three leaves now), bubbles with
  kittens peeking out, a pink gingham board, ribbon crates, brick walls, a
  heart padlock, a cat-eared launcher, sparkle bursts and soft berry frames.
- A new title painting: cherry blossom, a pastel sky and the seven wonders
  on the horizon. The copyright line no longer sits on the screen's edge.
- To keep players coming: cheering kana words on chains, specials, big
  drops, freed kittens, finished lines and pictures; the lucky paw roulette
  after every level won (points, or one more paw for the next level, with
  better odds for more stars); the picture album; the stars of each wonder
  on its chapter card; a best-five table with name entry (BEST CATS).
- The attract shows a match board and a picture in turn, and turns between
  the best five, the album and the stars of each wonder.
- The ending: the title with both cats cheering, then the credits on a card.
- Best scores and stars from v1 carry over.

## v1 (2026-10-18, NeoGeoSDK v1.7.7 pre-release) — the first public set

- Seven wonders, four levels each (28): the Sky Garden, the Crystal Canal,
  the Moon Lighthouse, the Clockwork Tower, the Coral Palace, the Sunstone
  Pyramid and the Starlight Dome, each with its cat, mood and goals.
- Two kinds of level: matching pieces on a board (match colours, break ice,
  make special pieces, clear crates) and launched bubbles (rescue the
  kittens floating in them).
- The paw hammer once a level, hints, an aim guide; stars and best scores
  kept in the save block.
- Arcade settings (MVS): TRIES, HOW TO PLAY, DIFFICULTY (EASY to EXPERT),
  AIM GUIDE.
- Built on the SDK's puzzle engine (`ng_puzzle`).
