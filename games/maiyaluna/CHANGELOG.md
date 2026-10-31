# Maiya & Luna — changelog

The ROM sets ship with the NeoGeoSDK pre-releases and are numbered on their
own: `_v1` and `_v2` with v1.7.7.

## v2.0.1 (2026-10-31, NeoGeoSDK v1.7.7 pre-release)

- The 2026-10-26 v2 update under a number of its own. The name v2 went out
  as two different builds; v2.0.1 names this one only
  (docs/releases/VERSIONING_POLICY.md). No change to the game.

## v2 update (2026-10-26, NeoGeoSDK v1.7.7 pre-release, same file names)

- Everything on screen stays inside columns 3-36 of the 40 (x 24-295), the
  part of the picture every CRT shows: the status line, the portrait and the
  dialogue (25 letters a line beside a portrait), the notes and the credits.
  Messages too long for one row go on to a second.
- Chapter VII: Director Valen's question lists its three answers one a row
  under it, and they show as soon as the question has been read (they used
  to overlap and wait for a direction press). The answers are shorter.
- The world tool holds dialogue to the box and tool names to the status line.
- Rooms with water, light and gear puzzles no longer slow down: the mill,
  the headland, the lamp room, the orchard, the terraces, the crystal hall,
  the old mine, the sun terrace and the sluice gallery ran at about half
  speed and now keep 60 frames a second (the puzzles work out water and
  light again only when something in them changes).

## v2 (2026-10-24, NeoGeoSDK v1.7.7 pre-release)

- Proportional character sprite scaling: Maiya and Luna adjusted to ~76.5% height (~42px tall) to naturally match the 43-44px NPC townsfolk.
- Precision feet anchoring at (-29, -47) on the exploration maps.
- Engine hardware sprite SCB2 scaling fix ensuring shrink registers persist across VRAM commit passes and slot parking.
- Full AES console support with memory card saving and pristine video playback pipeline.

## v1 (2026-10-18, NeoGeoSDK v1.7.7 pre-release) — the first public set

- The sequel to Maiya: Super Nature Girl. Choose who sets out: Maiya climbs
  ivy, pushes stones and shoos creatures with her whip; Luna slips through
  gaps, works fine mechanisms and looks closely. Each sister has her own
  routes, puzzles and conversations through the same places.
- Seven chapters, 46 rooms and 175 finds for the notebook in each journey.
  The sisters meet only in Chapter VI, at the World Tree, and solve it
  together (D switches between them).
- Every room settles and no place can strand a sister (checked on both
  journeys by `tools/check_world.py`).
- The notebook (START): bag, finds, notes, journey, saving.
- Arcade settings (MVS): HOW TO PLAY, DANGER (GENTLE or NORMAL), HINTS.
- Built on the SDK's RPG engine (`ng_rpg`).
