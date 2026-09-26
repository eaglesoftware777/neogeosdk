# Level Data

## Stages Outside C

A game can author its stages in files of its own and have them turned into
the C its scenes include when it is built. Its `game.mk` names the script:

```makefile
GAME_LEVEL_BUILDER = games/maiya/tools/levels.py
```

`make` runs it before compiling the game (both makefiles), and
`make level-check` runs it before checking, so the check always sees what
the build compiles. The script owns the format; the SDK only runs it.

## Maiya's Stage Files

Each of Maiya's stages is one JSON file in `games/maiya/levels/`, taken in
file-name order: `01_emerald_forest.json` is mission 1. Adding a file adds a
stage (`MG_LEVEL_COUNT` follows the file count). `levels.py` checks every
file and writes `games/maiya/scenes/maiya_levels_data.h` (generated, not in
git), whose initialisers `maiya_levels.h` builds the stage tables from.

```json
{
  "name": "EMERALD FOREST",
  "width": 3840,
  "background": 0,
  "music": 1,
  "mechanic": "none",
  "gate_x": 3200,
  "key": {"x": 2012, "y": 42},
  "guardian": {
    "name": "CHAINSAW BEETLE", "style": "beetle", "hp": 16,
    "hint": "...", "taunt": "...", "reply": "..."
  },
  "art": {"name": "ROSE WIND STRIKE", "words": "MAIYA: ROSE WIND, CARRY ME!"},
  "secret_hint": "...",
  "sunboy": ["...", "..."],
  "platforms":  [{"x": 320, "y": 140, "w": 96}],
  "encounters": [{"x": 180, "enemy": "slime"}],
  "archers":    [{"x": 510, "y": 104}],
  "hazards":    [{"x": 760, "w": 48, "type": "spikes"}],
  "rescues":    [{"x": 650, "who": "elder"}],
  "secrets":    [{"x": 510, "y": 74, "type": "chest"}],
  "pickups":    [{"x": 260, "y": 168, "kind": "silver"}],
  "decor":      [{"x": 150, "y": 160, "kind": "grass"}],
  "vines":      [{"x": 692, "top": 68, "bottom": 192}],
  "npcs":       [{"x": 444, "who": "elder", "line": "THE GATE OPENS TO THE SUN KEY"}]
}
```

Coordinates are world pixels: x from the stage's left edge, y from the top
of the screen (the road is at y 192). A platform's y is the surface she
stands on; pickups and secrets are placed by their top-left corner.

Names stand for the game's constants, lower case without the prefix:

| Field | Constants | Examples |
|---|---|---|
| `enemy` | `MG_E_*` in `maiya_levels.h` | slime, beetle, crow, jellyfish, pair |
| `hazards[].type` | `MG_H_*` | fire, spikes, sludge, toxic, pit |
| `guardian.style` | `MG_B_*` | beetle, toad, leviathan, eel |
| `mechanic` | `MG_M_*` in `maiya_game.c` | none, crumble, ice, water |
| `pickups[].kind` | `MG_K_*` from the art build | silver, gold, flower, life |
| `decor[].kind` | `MG_D_*` from the art build | grass, lantern, rock |
| `who` | the ally art, in order | elder, maiden, spirit, sunboy |
| `secrets[].type` | | rose, gem, chest |

Each list holds as many entries as its table (`MG_PLATFORM_COUNT` and the
rest in `maiya_levels.h`); encounters are met in x order. Screen text is
plain upper-case ASCII, at most 38 characters (a guardian's taunt and reply
40, names 30).

## Checks

`levels.py` stops at the first mistake and names the file, the field and the
entry, before anything is written:

```
levels: 01_emerald_forest.json: encounters[1].enemy: unknown enemy "betle" (known: acidmoth, beetle, ...)
```

It checks the fields are there and of the right kind, numbers in range
(inside the stage's width, on the 224-line screen), names known, lists no
longer than their tables, and text that fits the screen. `make level-check`
then checks placement on what was built (`tools/level_check.py`), and names
the stage file in each finding:

```
ERROR: EMERALD FOREST (level 0, 01_emerald_forest.json), pickup 0 at (330,130): inside -- ...
```

Converting the tables that were hand-written in C to these files left the
P-ROM byte-for-byte the same.
