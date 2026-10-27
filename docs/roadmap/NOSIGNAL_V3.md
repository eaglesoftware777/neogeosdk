# No Signal Z Alpha v3: audit, plan and status

v3 answers the feedback players posted on the Gamopat forum about the v2
release (NeoGeoSDK v1.7.7 pre-release, `NoSignal-*_v2` files). It keeps the
game as it is (its heroes, stages, story, modes and hardware support) and
works on five things the players named: animation, the weight of a blow,
hit boxes, colour, and the street crew's look. What can serve other
brawlers goes into the SDK's `ng_beatemup` engine, not into the game.

Status words: **done** (in the code, tested as stated), **partial**,
**planned** (not started), **deferred** (not in v3). Nothing here was run on
physical hardware: results are MAME and host tests only.

## 1. Audit of v2

### 1.1 Animation

- Heroes have 30 pictures each: stance, idle, four walk, two run, jump up
  and down, two guard poses, one picture per attack (jab, cross, uppercut,
  kick, jump kick, sweep, dash attack), two for the special, two hurt,
  knocked, down, getting up, grab, throw, victory, riding.
- An attack showed its one picture for its whole length: from the first
  wind-up frame to the last recovery frame. No anticipation, no follow-through,
  no return to guard.
- The street crew have 10 pictures each (stance, three walk, wind-up,
  attack, hurt, knocked, down, getting up). An attack's picture stayed up
  through the whole recovery (16 to 22 frames).
- Walk cycles changed picture every 8 frames whatever the speed, so feet
  slid: Zoe walks 2.4 pixels a frame, Kai 1.75, the biker 1.1.
- Hurt poses alternated between the two hurt pictures on a timer, which read
  as shaking, whatever the blow.
- Besides the hero, only one other fighter could change picture in a frame
  (`NEW_PICTURES`, a limit of the vertical blank's time), taken in drawing
  order: in a crowd, a struck enemy could show its hurt picture several
  frames after the blow.

### 1.2 The weight of a blow

- Hitstop: 2 + damage / 4 frames, at most 6, the same rule for every move.
  A jab held 2 frames, the heaviest kick 4.
- The spark appeared halfway between the two fighters, always 52 pixels
  above the target's feet: a sweep sparked at head height.
- The struck fighter didn't move during hitstop; nothing marked the last
  blow of a fight.
- The punch, kick and heavy sounds were a band of noise over a low thump
  (`tools/sound.py` `hit()`).

### 1.3 Hit boxes

Measured with `games/nosignal/tools/boxes.py`, which finds the picture shown
while a move can hit and, in it, the frontmost drawn pixel at the heights
the box covers.

| | v2 box ends | the picture reaches | |
|---|---|---|---|
| Jay jab, cross | 40, 44 | 49, 50 | a blow that visibly lands misses |
| thief jab | 34 | 49 | 15 short |
| runner slap | 34 | 63 | 29 short |
| Kai headbutt | 30 | 51 | 21 short |
| bloater slam | 50 | 38 | 12 past: lands from out of reach |
| android blade sweep | 50 | 26 at its height | the blade is higher than the box |
| Kai jump kick | box 0-50 high | the leg is 60-78 high | |

- 33 of the 71 measured moves ended more than 4 pixels short of or past their pictures.
- Across the belt a blow reached its box's half depth plus the target's: 16
  to 21 pixels for most moves, so fighters on visibly different lines hit
  each other.
- The A chain (jab, cross, uppercut, kick) started at the jab's full reach
  lost the uppercut every time, for every hero against every street enemy:
  the first two blows push the target back, and the uppercut's reach (36)
  is far shorter than the jab's. With boxes fitted to the pictures this got
  worse (the uppercut's fist reaches 22 to 24 pixels).

### 1.4 Colour and backgrounds

- Every stage painting is three 480-pixel scenes side by side. They meet
  with a soft 40-pixel blend, but the light changes there: stage 2 goes from
  day to sunset, stage 3 from a night street to a bright interior and back,
  stage 5 from a green day to an orange sunset to a blue evening. The colour
  step across a seam (CIE Lab, mean of 16 columns either side) is 13 to 56.
- A statistical grade of each scene towards the stage's average (tried for
  v3, half strength) changed the steps by 5 to 25% and the pictures hardly
  at all: the jumps are in what is painted, not in a colour cast. It was
  not kept; see 3.2.
- The street (the lower 45%) is flat grey on stages 3 to 8.
- The backgrounds are soft paintings; the fighters are hard-edged sprites
  with their own light.

### 1.5 The street crew

- DIESEL, the biker: red mohawk, studded leather vest, chains.
- SNATCH, the thief: striped tracksuit and beanie.
- The story is about a stolen smartphone; these two read as a 1980s film's
  street gang rather than today's city.

### 1.5b Still pictures (a later comment)

A further comment on V2: "the backgrounds lack life, too static, no
animation". The street paintings never changed: no light, no movement in
the air. Only the hero, the crew and their effects moved.

### 1.6 Performance (to be measured on the v3 build, same method as v2)

The v1.7.7 test report measured v2 with `make ... PERF=2` and an autopilot
run: 1695 of 14204 frames (12%) with a sprite update left waiting for the
next blank, longest run 32 frames (Zoe, 240 s). v3's figures go in section 4.

### 1.7 The AES+ report

A player reported the picture's size on an AES+. The AES+ outputs HDMI and
shows the whole 320-pixel width, the border included
(`docs/platforms/CLASSIC_BASELINE.md` section 6). v2 kept HUD text inside the
title-safe columns (the v1.7.7 safe-area update) and fills the border with
the stage painting. No AES+ was available to test on: the report is
recorded, not resolved.

## 2. Plan, by priority

| # | Work | Area | Status |
|---|---|---|---|
| 1 | Boxes fitted to the pictures; half depths 7 (heroes) and 9 (crew), so a blow reaches 13 to 16 pixels across the belt; `boxes.py --check` in the tests | hit boxes | done |
| 2 | `ng_beatemup`: a move may step in while winding up, only as far as the nearest foe in its lane needs (`NGBMove.step`); the heroes' cross, uppercut and kick use it, so the chain lands from the jab's reach | hit boxes, animation | done |
| 3 | Host test of the moves on the engine: the chain lands four blows against six kinds of enemy, a jab misses 4 pixels past the fist and 17 pixels across, a brawler's blows reach where it stops to strike | hit boxes | done |
| 4 | `ng_beatemup`: per-move hitstop (`NGBMove.hitstop`), a longer hold on the blow that beats an enemy or a boss (`NGBStage.ko_hitstop`, `boss_ko_hitstop`); No Signal: jab 3 frames up to the special 8, the last blow 10, a boss's 18 | impact | done |
| 5 | `ng_beatemup`: the hit event says where the blow landed (`value`: height above the feet; `NGBFighter.hit_z`); the spark at the body's edge on the striker's side at that height | impact | done |
| 6 | The struck fighter shudders 2 pixels during hitstop | impact | done |
| 7 | Attacks in three pictures from the existing art: a wind-up (chambered guard, arm drawn back, crouch), the blow, the blow held for half the recovery, then the guard; the crew's attack picture gives way to its stance halfway through the recovery | animation | done |
| 8 | `ng_beatemup`: `ng_beatemup_move_phase()` (wind-up, active, recovery and the frame within it) for item 7 | animation | done |
| 9 | `ng_beatemup`: the distance each fighter walks (`NGBFighter.stride`); walk and run pictures every 16 pixels, so feet keep to the ground | animation | done |
| 10 | Hurt pictures by where the blow landed: high, the head snaps back; at the body, doubled over | animation | done |
| 11 | Picture changes ranked: the hero always, then fighters striking or struck, then walkers; two others when the hero's picture stays | animation, VRAM | done |
| 12 | Punch, kick, heavy and knockout sounds layered: a click, a crack, a falling thump, a sub drop and a short rumble for the heavy ones | impact | done |
| 13 | Debug view of the volumes in MAME (`tools/beatemup_boxes.py`): hurt, wind-up and strike boxes, depth, hitstop, drawn live and painted onto snapshots | hit boxes | done |
| 14 | New in-between pictures: two-step anticipation for the heavy blows, a recovery pose per hero, run and jump pictures for the crew | animation | partial: ten new pictures a hero (wind-ups, returns to guard, a landing, a run stride); the crews' run and jump pictures not drawn |
| 15 | DIESEL and SNATCH redrawn in today's clothes (an e-bike courier crew, a hoodie and sling bag), same silhouettes, moves and colours slots | crew | done |
| 16 | Each stage's three scenes repainted to one time of day and one key light; the seams hidden by a pillar, a fence or a corner; the street given texture and wear | colour | done for the light (7 of 24 sections repainted; stages 1 and 2 already kept one light); the seams are cross-faded, not hidden behind a pillar; repainted floors have wear |
| 17 | The fighters' palettes checked against each stage's light (same value range, rim light from the same side) | colour | partial: outline contrast and value range checked on every stage (`tools/contrast.py`); the fighters' shading not redrawn per stage |
| 18 | Performance and compatibility measured on the v3 build: frames with a waiting sprite update, sprites on a line, MVS and AES in MAME with EagleBIOS, the NeoSD, Darksoft and BackBit files, the CD build | all | done (section 4) |
| 19 | Frame time: each fighter's look found once instead of searched for several times a frame; the status line writes one of score, enemy bar and combo a frame in play | performance | done |
| 20 | The CD port converts the cartridge version named in `cd/reference.json` (V3); the scenery slot moved up to tile 0x7A00 for the larger fighter set | CD | done |
| 21 | Living streets: each painting's lights (small bright colours picked by `tools/art.py`) pulse, flicker or blink through palette reloads; six drifting specks a street (leaves, dust, drizzle, bubbles, pollen, embers, bits, sparkles) | colour | done |
| 22 | Depth: a front plane of near objects (one a street) passing in front of everyone at 1.25 times the street's speed; the heroes' walk in six steps and run in four (ten new pictures each); on CD the played hero's ten strides load into their own slot with each street | animation, colour, CD | done |

Art (items 14 to 17) is drawn with the image tools, then reviewed frame by
frame for anatomy, matching proportions, palette and originality before it
is imported; nothing generated goes in unchecked.

## 3. Notes on the choices

### 3.1 The step in

A blow that only reaches as far as its picture is honest, but a chain whose
blows have different reaches then falls apart. Real fighters step into the
next blow; `NGBMove.step` does that during the wind-up, towards the nearest
foe in front and in the lane, and stops as soon as the blow will reach it 4
pixels deep, so a fighter standing close is never walked through. With
nobody in reach the move whiffs on the spot.

### 3.2 Colour

The colour jumps are in the paintings (different times of day, a lit
interior between two night streets), so the fix is art: repainting the
scenes of each stage to one light. Tinting the fighters to each stage at run
time was considered and left out: at night the brightest lights are warm
lamps, the average is dark blue, and either would make the heroes muddy.

## 4. Verification

MAME 0.283 (MVS and AES drivers, EagleBIOS) and host tests, 2026-10-27.
Nothing was run on hardware.

### 4.1 Frame time

Method of the v1.7.7 report: a `PERF=2` autoplay build, 240 seconds from
power-on, the SDK's counters (`ng_perf`) logged every frame; sprites on a
line read back from video memory every second frame. "Before" is the
committed source this work started from, built and run the same way.

| Build | Hero | Frames with a sprite update waiting | Longest run | Over time (all / in play) | Work median / 99% (lines of 264) | Most sprites a line |
|---|---|---|---|---|---|---|
| before | Zoe | 832 (5.9%) | 10 | 10 / 6 | 71 / 166 | 77 |
| V3, first build | Jay, Zoe, Kai | 1118, 1210, 839 | 12, 14, 18 | 17, 18, 11 / 13, 14, 7 | 77, 80, 71 / 175, 182, 162 | 81, 79, 79 |
| V3, frame-time fixes (item 19) | Jay, Zoe, Kai | 1290, 1366, 833 | 15, 19, 18 | 9, 8, 6 / 5, 4, 2 | 78, 77, 68 / 162, 165, 146 | 81, 79, 79 |
| V3 with living streets, first try | Jay, Zoe, Kai | 2580, 2491, 2178 | 25, 30, 21 | 18, 13, 9 / 14, 9, 5 | 98, 95, 93 / 179, 173, 174 | 82, 81, 79 |
| V3, living streets (first release) | Jay, Zoe, Kai | 1257, 1160, 1013 | 22, 16, 16 | 8, 7, 8 / 4, 3, 4 | 84, 83, 80 / 160, 164, 158 | 81, 79, 80 |
| V3, final: front plane and new walk and run (item 22) | Jay, Zoe, Kai | 1291, 1193, 1039 | 22, 16, 16 | 8, 7, 8 / 4, 3, 4 | 79, 78, 76 / 158, 163, 158 | 85, 83, 84 |

The first V3 build ran long on frames where a blow landed in a crowd: an
instruction trace of one such frame (492 lines) showed the status line
rewriting score, enemy bar and combo at once (about 25,000 cycles) on top of
a frame already at 150,000, and a quarter of every frame spent searching the
looks table (`look_of`). Item 19 fixed both. The living streets' first try
moved six specks through the sprite-group commit and reloaded six palette
banks every other frame; the final one posts the specks' positions only,
reloads one changed bank a frame, and skips the frame a blow lands.

### 4.2 Pictures and boxes

- `boxes.py --check`: every strike box agrees with its picture (after the
  new crew art, the thief's two moves and the courier's charge were refitted).
- Sprite check in MAME (`scan.lua`: every visible sprite's tiles in a real
  asset frame, in that asset's palettes, every fourth frame): the three
  240-second campaigns, 3,433 frames each, and the released MVS and AES sets
  played 120 seconds with stick and buttons, 1,657 frames each: no fault.
- Palette RAM read in MAME on stage 6: the three neon colours watched step
  through all eight brightness levels, each on its own phase; an ordinary
  colour of the same painting never changed.
- `contrast.py --check`: every fighter keeps its outline on its streets
  (worst: 11% of the guard's outline within dE 10 of stage 7's street).

### 4.3 Builds

- Host: 22 No Signal, CD conversion and engine tests pass.
- Release files: `make neo` (the AES and MVS sets, NeoSD images) and
  `tools/make_release.py --version v3` (NeoSD, Darksoft, BackBit, MAME with
  EagleBIOS). The MAME sets were played as in 4.2; the flash-cart files were
  not run on carts.
- Neo Geo CD: `tools/neocd/build.py --game nosignal` from the V3 sources
  (`cd/reference.json` version 3), `qa_nosignal.py` in MAME's CDZ driver:
  Jay, Zoe and Kai on stock and EagleBIOS-CD firmware, the 13-scene transfer
  diagnostic on both, the CHD on both: all passed.
