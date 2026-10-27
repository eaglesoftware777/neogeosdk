# No Signal Z Alpha

A native Neo Geo single-player beat-'em-up, built on the SDK's beat-'em-up
engine (`GAME_GENRE_ENGINE = beatemup`, `sdk/2d_engine/ng_beatemup.c`).
Game ID 784.

Current build: **V3**, the answer to the players' feedback on V2: blows wind up,
land and recover in their own pictures, hold and spark where they hit, reach
exactly as far as they are drawn, and sound heavier; the street crew dress
for today; each street keeps one light from end to end, its lights and air
move, and near objects pass in front of the camera for depth; the heroes
walk in six steps and run in four. See the changelog for
what was done, what is partial and how it was tested. V1 and V2 releases
remain separate.

A native Neo Geo CD version is also available, built from the same V3 game,
with resident fighters, scene loading and CD audio. See
[`cd/README.md`](cd/README.md) for building, testing and firmware requirements.

Someone stole the phone. Then the whole city lost its signal. Choose one hero
(exactly one person plays) and get it back across eight stages: The Hood, The
City Underpass, The Viral Outbreak, The Clone Lab, Concrete Jungle, The
Machine City, The Rogue AI and The Signal Beyond Earth, each with its own
crew and a boss in three phases.

- **Jay** ("No Cap"): fast hands, big heart; special Ratio Rush.
- **Zoe** ("Main Character"): flips, counters, sass; special Main Character
  Energy.
- **Kai** ("Aura"): slow, strong, unbothered; special Aura Breaker.

On an arcade board (MVS) a credit goes straight into the story: eight
stages, one phone. A console (AES) also offers a menu of modes: Story
Arcade, Stage Select (opened by clearing the story), Challenge (two
minutes, high score) and the Gallery of everyone met so far. Health is the
phone's battery, lives its signal bars; out of lives, RECONNECT? (a credit on
an arcade board, free on a console).

## Controls

The stick walks along and across the street; a double tap runs. A attacks (a
chain of four; down + A sweeps), B jumps (A in the air kicks), C grabs and
throws, D is the hero's special (it costs a little battery). Walking over
food, a gold sneaker or a weapon picks it up (a weapon only with empty hands;
C still picks up too). Walking onto the electric scooter rides it; B (or C)
jumps off. On a console START pauses.

## How it is made

- `scenes/ns_data.c`: the heroes, enemies, bosses and their moves, the eight
  stages' waves, props and weapons, the scenes' lines.
- `scenes/ns_game.c`: the flow of a game; `scenes/ns_draw.c`: the screen.
- `tools/import_sheets.py SOURCE_DIR [NAME ...]` cuts painted pose sheets into
  fighter frames (`artbox/painted/fighters/<name>/NN.png`): 128-pixel frames,
  160 for the larger bosses, 192 for the Overmind, so every fighter fits the
  screen. `tools/paint_import.py` imports the street paintings (three
  sections each), the title, the endings and the portraits.
- `tools/art.py` packs them; `tools/sound.py` renders the music, effects and
  voices before `make`.
- `tools/boxes.py --check` compares every move's strike box with the picture
  shown while it can hit (`--suggest` proposes a fitted box);
  `tools/contrast.py --check` stands every fighter on the streets it fights on
  and checks its outline stays visible. Both run in the tests.
- `tools/beatemup_boxes.py` (the SDK's) draws the volumes live in MAME: hurt,
  wind-up and strike boxes, depth and hitstop.

The screen is planned around two hardware limits. The Neo Geo shows at most
96 sprites on a scanline, counting every sprite whose height covers it,
on the screen or not, so the street's strips are placed one by one and only
the 21 on the screen are shown, and five enemies at most are out at once.
Sprite tables are written in the vertical blank only (`NG_VRAM_DEFER`), about
four whole fighter pictures' worth a frame, so each fighter keeps its own
block of sprites while the depth order allows, animation clocks are offset
per fighter, and at most two fighters besides the hero change picture in one
frame (the others a frame later).

## Build

```sh
python3 games/nosignal/tools/sound.py        # when the music or voices change
make GAME=nosignal GAME_CFG_FILE=games/nosignal/game.cfg all
make unit-tests                               # includes tests/beatemup_test.c
python3 -m unittest discover -s tests -p 'test_nosignal*.py'
```

## Test builds (never install these)

Add to `GAME_ENGINE_DEFINES` (keep the game's own defines):

- `-DNS_AUTOPLAY=1`: a plain bot plays the story (it lines up with the
  nearest enemy, punches, and spends the special when crowded; a slow mend
  lets it reach the ending). `-DNS_AUTOPLAY_HERO=0|1|2` picks Jay, Zoe or Kai,
  `-DNS_AUTOPLAY_STAGE=0..7` the street the story starts on.
- `make ... PERF=2`: the SDK's frame and video-memory counters (`ng_perf`,
  `sdk/ng_perf.h`): how many sprite updates waited for a later frame.

`tools/genre_qa.py` drives MAME from a timeline and logs RAM by symbol, for
example (START every ten seconds moves the scenes on, coins keep credits for
the continue screen):

```sh
python3 tools/genre_qa.py --game nosignal --seconds 1500 --every 10 \
  --step 3:coin --step 6:start --step 20:start --step 30:start \
  --watch s_hero:1 --watch s_total:4 --out /tmp/ns-qa
```
