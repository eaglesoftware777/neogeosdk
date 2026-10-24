# V5 environment sources

These indexed PNGs are editable native-resolution game sources. Their embedded
colours are rounded to the hardware's five-bit channels. Index zero is
transparent; other colours are arranged into 16-entry banks. A 16x16 tile
must use only one non-transparent bank. No resizing is performed on import.

The twelve named panoramas hold a 192px far view and a 32px road. Their
matching `_mid` files are transparent middle-distance islands. `block_*`
sources retain the existing terrain silhouettes, while `landmark_*` contains
the gate surrounds. `v5_arena_*` supplies each stage's guardian backdrop.
The cart, three contact flashes and four water phases have separate sources.

`tools/arcade_art.py` imports these PNGs and verifies native tile encoding.
An ordinary build keeps source edits. Its explicit `--author` option redraws
the sources from the checked-in whole-pixel construction code; do not use it
to import an edited PNG. The importer rejects reservation-size changes,
mixed banks within one tile, cast-tile changes and CD sprite-memory overflow.

Existing character, enemy, boss, portrait and NPC reservations are protected.
The importer leaves frame numbers and the established cast palettes intact.
New scenery follows existing reservations and stops before CD utility tiles.
