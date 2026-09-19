# Maiya: Super Nature Girl — artwork and audio

## Source art

`assets/source_art/` holds the original painted sheets the game is built
from: Maiya's moveset, the six blight creatures, the guardians, the allies
and villagers, the pick-up icons and the title key visual.  `environments.png`
holds six painted environment panels.

`tools/build_commercial_assets.py` cuts those sheets into Neo Geo sprites:
one shared scale per animation, a foot-centre anchor so a walk and a whip
stay the same girl, a fitted 15-colour palette per group, and the C-ROM pair
(`780-c1.c1`, `780-c2.c2`) plus `maiya_assets.h`.

Six valleys take their far layer and road strip from the painted panels:

| Mission | Valley | Panel |
| --- | --- | --- |
| 1 | Emerald Forest | forest canopy |
| 2 | Valley of Sacred Falls | bamboo pass and waterfall |
| 3 | Azure Coral Coast | harbour at sunset |
| 4 | Golden Autumn Grove | blossom village |
| 5 | Crystal Grotto | snow temple |
| 6 | Sacred World Tree | old-growth swamp |

Each valley carries a second, blighted palette: the same painting with its
hues pulled part way toward the blight and some life drained out, so
cleansing it reads as the valley waking up rather than a different picture.

## Drawn in the repository

`tools/nature_art.py` draws every piece of scenery and every trinket on one
15-colour nature palette: grass, blossoms, bushes, mushrooms, saplings,
rocks, stone lanterns, signposts, lily pads, waterfalls, climbing vines,
doors, the Ancient Nature Gate, the six biome ledge sets, the coins, the
cut flower, the forest friend, the extra life, and the three power-ups.

`tools/build_fix_assets.py` draws the FIX layer sheets and writes the HUD
glyphs (heart, rose, key, coin, leaf, spark) into the low FIX codes of
`artbox/780-s1.s1`, which is what the lives counter prints.

## Audio

The YM2610 bank is the SDK's own sound data, used without modifying the
driver or its samples.  Its credits are in `games/demo/sound/SOURCES.md`.

## Provenance

This is an original game.  It contains no commercial game data, no
disassembled code and no assets taken from other games.
