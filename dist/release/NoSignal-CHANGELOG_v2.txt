# No Signal Z Alpha — changelog

The ROM sets ship with the NeoGeoSDK pre-releases and are numbered on their
own: `_v1` and `_v2` with v1.7.7.

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
