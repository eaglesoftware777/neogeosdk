# No Signal Z Alpha — changelog

The ROM sets ship with the NeoGeoSDK pre-releases and are numbered on their
own: `_v1` and `_v2` with v1.7.7.

## v3 (in progress, SDK 1.7.7)

- Walk over food, a gold sneaker or a weapon to pick it up (a weapon only with
  empty hands; C still picks up too). The engine gains the stage flag
  `NGB_SF_AUTO_PICKUP`; every No Signal stage sets it.
- Crates, bins and weapons now lie in the middle of the street (depth
  168-180, the hero starts on 173), away from hazards. The last crate of
  stage 1 sits in reach before the boss.
- Fix a hero drawn as a black or scrambled figure while swinging a picked-up
  weapon, and a wrong picture for a moment after every throw (heroes and
  grappling enemies): the drawing code read the move table past its end for
  the engine's weapon swing (move 0xFE) and a throw's recovery (no move).
  Poses are also clamped to the sheet. A swing now shows the hero raising
  the weapon, then swinging it through.
- A held bat, pipe or wreck is drawn in the hero's front fist (measured per
  hero and pose) instead of floating at the waist.
- Things to pick up bob gently on the ground; picking one up sparkles and
  says what it was.
- Status line: the hero's portrait (the dialogue portrait, shrunk to 32 x 32
  by the hardware, in palette banks of its own), name, battery, enemy bar
  and combo beside it.
- All text is ringed with a dark outline (`ArtPack(text_edge=2)`), readable
  over the streets; other games' fonts are unchanged.
- The hero select's names and lines sit on the dialogue plate instead of the
  lit windows.
- The red "!" over an enemy about to strike is rubbed out after half a
  second (it used to stay on screen).
- Dust when landing from a jump and when breaking into a run; a short
  sideways screen shake on heavy blows, boss phase changes, a boss going
  down and scooter crashes.
- Existing art is unchanged: the C ROM only gains the three 32 x 32 status
  portraits (48 tiles appended after the logo). M1 and V1 are byte-identical.
- Verification (MAME, 2026-10-24): an in-emulator check that every visible
  sprite's tiles belong to a real asset frame and use that asset's own
  palettes ran every fourth frame through 1800-second autoplay campaigns
  for Jay, Zoe and Kai (stages 1 to 8 between them): 79,545 frames, no
  fault (the check itself flags a planted bad palette). An AES run of the
  real (non-autoplay) build: 1,154 frames, no fault. Host engine tests
  (walking over pick-ups included) and the No Signal tests pass. Physical
  hardware is untested.

## v2 (2026-10-24, SDK 1.7.7)

- Restore missing clothing and armour pixels from the original pose sheets for
  all 22 fighters (360 poses), including Kai's hoodie, Zoe's outfit and the
  Archivist's cape. Background removal now starts at the bright magenta key
  and stops before purple fabric; enclosed spaces between limbs stay clear.
- Refit fighter and portrait colours within their existing palette allocations
  for cleaner skin, clothing and material shading. Canvas sizes, frame order,
  anchors, gameplay, sound and cartridge alignment are retained.
- Add regression tests for fabric versus key colour, enclosed gaps, recovered
  clothing samples, existing canvas limits and exact source-to-C-ROM alpha.
- Includes the hidden-bank depth-ordering improvements described below.
- Verification: all 360 source masks match decoded C-ROM pixels. All three
  heroes passed 110-second MAME crowd replays on both AES and MVS with no
  missing visible hero rows, black hero palettes or layout faults; peak load
  was 81 strips. Beat-'em-up, deferred-renderer, content-budget and API checks
  pass. Physical cartridge testing and full campaigns remain unverified.
- The broader SDK host suite still stops at the existing sprite-position
  write-count assertion in `tests/sprite_render_test.c`; it is unchanged here.

## v2 candidate (2026-10-20, SDK 1.7.7)

- Prepare depth-order changes in a hidden sprite bank, then publish the complete
  layout together. Retain the visible layout while uploads wait for VBlank.
- Keep death and respawn blink maps resident instead of reloading every flash.
- Give hero animation uploads priority and restore selected hero palettes at
  stage entry. Consume published video before BIOS input service.
- Validate both 80-strip banks against all stages and expert crowds.
- Local 110-second MVS crowd replays for Jay, Zoe and Kai reported zero missing
  visible sprite rows, black hero palettes, capacity faults or cutover deadline
  misses. Peak load was 79/75/77 strips respectively. Kai's AES replay also passed.
  Physical hardware, full campaigns and the remaining AES hero checks are pending.
- Character artwork, C/S graphics, M/V audio and cartridge mapper are unchanged.

## v1 (2026-10-18, NeoGeoSDK v1.7.7 pre-release) — the first public set

- A beat-'em-up for one player: choose Jay, Zoe or Kai and get the stolen
  phone back across eight stages, each with its own crew and a boss in
  three phases.
- A chain of four attacks, a sweep, jump kicks, grabs and throws, pick-ups,
  a scooter to ride, and each hero's special (it costs battery).
- Enemy name and health bar when you hit one; health is the phone's
  battery, lives its signal bars.
- Modes: Story Arcade; Boss Rush and Stage Select after clearing the story;
  a two-minute Challenge; the Gallery.
- Arcade settings (MVS): LIVES, DIFFICULTY (EASY to EXPERT), HOW TO PLAY,
  BARKS.
- Still open: flicker when many fighters stand close together, the
  repainting of stages 2 to 8, and the feel of the fights.
- Built on the SDK's beat-'em-up engine (`ng_beatemup`).
