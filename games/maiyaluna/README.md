# Maiya & Luna: Wonderful Adventures in Nature Fantasy

A native Neo Geo exploration and puzzle adventure, the sequel to Maiya: Super
Nature Girl, built on the SDK's RPG engine (`GAME_GENRE_ENGINE = rpg`,
`sdk/2d_engine/ng_rpg.c`). Game ID 783.

After the storm, the valley's seven wonders, the old machines that shared its
water fairly between the villages, have gone quiet. The player chooses who
sets out: **Maiya**, who climbs ivy, pushes heavy stones and shoos creatures
with her whip, or **Luna**, who slips through narrow gaps, works fine
mechanisms and reads what she looks at closely. The other sister stays to
help Willowbrook mend its storm damage. Both journeys pass through the same
places, but each sister has her own routes, puzzles and conversations. The
sisters meet in Chapter VI, at the World Tree, and solve it together (D
switches between them); in the finale the other one keeps watch at the mill
while the traveller goes down alone.

Seven chapters: the Forgotten Watermill, the Emerald Labyrinth, the
Lighthouse of Seven Mirrors, the Clockwork Orchard, the Crystal Grotto, the
World Tree Observatory and the Great Valley Restoration. 46 rooms, 175 finds
for the notebook (70 flowers, 35 history, 28 diagrams, 21 photographs, 14
specimens, 7 wonders) in each journey.

## Controls

The stick walks. A talks, reads, takes, turns, rings and fits. B is Maiya's
whip or Luna's close look. C uses the tool chosen in the notebook. D, while
both sisters travel, switches between them; held, it makes the other wait or
follow. START opens the notebook (bag, finds, notes, journey; saving).

## How it is made

- `world/chapter1.py` ... `chapter7.py`, `world/finds.py`: the world, written
  with `tools/worldlib.py`: rooms as 20 x 14 character maps, people,
  puzzles, items, flags and dialogue. `maiya(...)` / `luna(...)` mark objects,
  exits and triggers of one journey; `J(maiya=..., luna=...)` is a value that
  differs between them; `say(...)` is a line the travelling sister says.
- `tools/world.py` turns them into `scenes/ml_world_data.h` and
  `scenes/ml_world_ids.h` (the build's level builder): one world per journey,
  `ml_worlds[ML_J_MAIYA]` and `ml_worlds[ML_J_LUNA]`, sharing every table
  that is the same in both.
- `tools/art.py` paints each room from its map (`tools/ml_tiles.py`), draws
  the objects and people, and takes the sisters from the first game's own
  frames (Luna in her palette). Painted pictures (title, chapter cards,
  portraits) are imported with `tools/paint_import.py`, townsfolk sheets with
  `tools/folk_import.py`.
- `tools/sound.py` renders the effects and the 18 tunes before `make`.
- `scenes/ml_game.c` is the flow of the game, `scenes/ml_draw.c` the screen.

## Build and check

```sh
python3 games/maiyaluna/tools/sound.py          # when a tune or effect changes
make GAME=maiyaluna GAME_CFG_FILE=games/maiyaluna/game.cfg all
python3 games/maiyaluna/tools/check_world.py    # both journeys, on the host
```

`tools/check_world.py` builds `tools/check_world.c` with the real engine and
plays both journeys from the first morning to the credits: every step only to
a tile the sister can walk to, every puzzle by its intended solution, the
cards' room changes as the game does them. It checks every find can be
collected, every room settles (no gate, water or light that keeps switching)
and, after the story, that a lone sister can get home from every place an
exit leads to. `--upto N` stops after chapter N, `--journey maiya|luna` plays
one.

It also writes the autopilot steps (`scenes/ml_autoplay_maiya.h`,
`scenes/ml_autoplay_luna.h`) for the test builds below.

## Test builds (never install these)

Add to `GAME_ENGINE_DEFINES` (keep the game's own defines):

- `-DML_AUTOPLAY=1`: the ROM plays Maiya's journey by itself, the host
  check's steps through the real game loop, so an emulator script can watch
  every room, conversation, card and the credits. With
  `-DML_AUTOPLAY_LUNA=1`, Luna's journey.
- `-DML_TOUR=1`: visits every room in turn, a few seconds each.

`tools/genre_qa.py` drives MAME from a timeline and logs RAM by symbol, for
example:

```sh
make GAME=maiyaluna GAME_CFG_FILE=games/maiyaluna/game.cfg \
  GAME_ENGINE_DEFINES="-DNG_PALFX_SCREEN=1 -DNG_VRAM_DEFER=1 -DNG_SOUND_QUEUE=1 -DML_AUTOPLAY=1" p1
python3 tools/genre_qa.py --game maiyaluna --seconds 1100 --every 4 \
  --step 3:coin --step 5:start --step 8:press:A --watch s_ap:2 --watch s_g+8:1 --out /tmp/ml-qa
```

`s_ap` is the autopilot's step and `s_g+8` the room.

## Neo Geo CD V2

A native CD version preserves the released V2 artwork and game rules. See
[the CD build and firmware notes](cd/README.md) for media, audio and saving.
