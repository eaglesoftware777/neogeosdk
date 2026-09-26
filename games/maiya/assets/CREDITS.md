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

Guardian sheets contain both large drawings and thumbnails; guardian frames
normalize their longest silhouette dimension to 86 pixels on a 96x96 canvas.
The hero keeps a shared scale on an 80x64 canvas. Both paths preserve a
two-pixel foot margin and one palette for the entire animation.

Every valley takes its far layer and road strip from its own painted panel:

| Mission | Valley | Panel |
| --- | --- | --- |
| 1 | Emerald Forest | forest canopy |
| 2 | Valley of Sacred Falls | bamboo pass and waterfall |
| 3 | Azure Coral Coast | harbour at sunset |
| 4 | Golden Autumn Grove | blossom village |
| 5 | Crystal Grotto | snow temple |
| 6 | Sacred World Tree | old-growth swamp |
| 7 | Rio Negro Works | the old plant over the swamp river |
| 8 | Sunken Reef | the reef painting (`arena_eel.png`), a band off its foot |
| 9 | Silver Cave | the mine painting (`arena_wyrm_0.jpg`) |
| 10 | Golden Savanna | the hunters' camp painting (`arena_hyena.jpg`) |

The poison dart frog is the small-animal sheet's green frog
(`npc_small_animals_0.jpg`), recoloured blue by the builder.

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

Maiya has her own YM2610 bank, not the demo's.  `game.mk` points the sound
build at `games/maiya/sound`, so nothing in the ROM comes from another
game's bank.

The sixteen ADPCM-A effects in `sound/samples/in_wav_a/` (whip crack, thorn
toss, squish, clank, spit, pulse, saw, flame, shatter, blast, the pick-up
chimes, the art surge, the guardian roar, dash and hurt) are public-domain
(CC0) recordings from free sound libraries, converted to 16-bit mono
22 kHz and trimmed for the driver.  The tracked files are the set the ROM
ships with.

The eight FM/SSG MML tracks in `sound/mml/` -- the valley theme the
eyecatcher plays, one per world, and the guardian battle -- are written by
`tools/build_sound_bank.py`.  The same script can also synthesise a
placeholder set of the sixteen effects when the recordings are not wanted.

## Provenance

This is an original game.  It contains no commercial game data, no
disassembled code and no assets taken from other games.
