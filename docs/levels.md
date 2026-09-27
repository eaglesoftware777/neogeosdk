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
  "pit": "water",
  "blocks": "grass",
  "climb": "vine",
  "upper": 0,
  "posted": "none",
  "gate_x": 3200,
  "key": {"x": 2012, "y": 42},
  "hideout": {"x": 2628, "y": 68, "hint": "KNEEL ON THE HIGH LEDGE PAST THE KEY"},
  "guardian": {
    "name": "CHAINSAW BEETLE", "style": "beetle", "hp": 16,
    "hint": "...", "taunt": "...", "reply": "..."
  },
  "art": {"kind": "blossom", "name": "ROSE BLOSSOM STORM", "words": "MAIYA: ROSE WIND, CARRY ME!"},
  "secret_hint": "...",
  "sunboy": ["...", "..."],
  "healed": ["THE OLD TREES BREATHE FREELY AGAIN", "..."],
  "briefing": ["A CHAINSAW BEETLE TEARS THE FOREST", "..."],
  "platforms":  [{"x": 320, "y": 140, "w": 96}],
  "encounters": [{"x": 180, "enemy": "slime"}],
  "archers":    [{"x": 510, "y": 104}],
  "hazards":    [{"x": 760, "w": 48, "type": "spikes"}],
  "rescues":    [{"x": 650, "who": "elder"}],
  "secrets":    [{"x": 510, "y": 74, "type": "chest"}],
  "pickups":    [{"x": 260, "y": 168, "kind": "silver"}],
  "decor":      [{"x": 150, "y": 160, "kind": "grass"}],
  "vines":      [{"x": 692, "top": 68, "bottom": 192}],
  "npcs":       [{"x": 444, "who": "elder", "line": "THE GATE OPENS TO THE SUN KEY"}],
  "waves":      [{"x": 900, "enemy": "gnat", "form": "swarm", "count": 5, "y": 100}]
}
```

Coordinates are world pixels: x from the stage's left edge, y from the top
of the screen (the road is at y 192). A platform's y is the surface she
stands on; pickups and secrets are placed by their top-left corner.
The hideout is where kneeling takes her to the valley's hidden vault; its
hint is what the elder's charm tells her. It must be on a ledge (or the
road, y 192), the key just above a ledge, and each climb's top on a ledge.

A platform marked `"rotten": true` is drawn greyed and gives way under her:
it trembles, drops out of sight and grows back a while later.

`healed` is what her work gave back, told by the elder in the healed
valley after its guardian falls; `briefing` is the elder's word before the
stage, on its pollution and its foe (two lines each).

`waves` (optional) is the spawn script: a flight of `count` (1 to 6)
creatures sent when the view's right edge reaches `x`, flying formation
`form` about height `y` -- `line` (in a row), `sine` (rising and falling),
`vee` (a V, point first), `dive` (drops in from above, hangs, dives at
her), `circle` (loops round a point), `swarm` (closes in on her), `charge`
(squares up, shakes, rams across), `hover` (keeps pace ahead of her,
firing). Waves come in x order.

`rush` (optional) is a boss rush: guardian styles met again, in order,
before the stage's own guardian (three stomps each, each in its own lair,
with the words of the valley it first guarded). The Smog Citadel sends
five before Lord Smoggar, who fights in three phases there.

A stage with `"mechanic": "flight"` is flown on the sun eagle: the view
scrolls on its own and its guardian meets her in the open sky at the end.
It has no ledges, captives, gate, key, hideout, climbs, hazards or posted
creatures (`gate_x` 0, `rescues` empty); its creatures come in waves.

`upper` (optional, 0 when left out) gives a valley an upper tier: how many
pixels above the screen it reaches. Its ledges, pickups, secrets, key and
hideout may then have y below 0 (a ledge down to 64 - upper, so she stands
fully in view on the top one), and a vine may climb up to 256 px from the
road to reach it. The view rises with her once she is above y 64 and eases
back down as she comes down; the painted scenery behind stays put.

Names stand for the game's constants, lower case without the prefix:

| Field | Constants | Examples |
|---|---|---|
| `enemy` | `MG_E_*` in `maiya_levels.h` | slime, beetle, crow, jellyfish, pair; the polluters: bagocto, binocto, sawbot, drillbot, torchbot, smogstack, sludgebarrel (the valleys' own foes: tough, a point of hp more every three valleys, and placed on the open road -- under a ledge a creature stands on the ledge) |
| `hazards[].type` | `MG_H_*` | fire, spikes, sludge, toxic, pit |
| `guardian.style` | `MG_B_*` | beetle, toad, leviathan, eel, airship (the Sky Road's, fought part by part) |
| `mechanic` | `MG_M_*` in `maiya_game.c` | none, crumble, ice, water, flight |
| `waves[].form` | `MG_FORM_*` in `maiya_levels.h` | line, sine, vee, dive, circle, swarm, charge, hover |
| `pit` | `MG_PIT_*`: what lies at the bottom of its pits | water, fire, toxic, void |
| `blocks` | `MG_BLOCKS_*`: the ledge set (and front-plane stone) | grass, moss, sand, autumn, snow, bark, rust, coral, stone, savanna |
| `posted` | `MG_E_*`: who stands on the `archers` ledges, or none | none, goblin, drone, poachdrone |
| `climb` | `MG_D_*`: what its vines are made of | vine, rope, ladder, chain, kelp, icevine |
| `pickups[].kind` | `MG_K_*` from the art build | silver, gold, flower, life, lily (the sky lily: a second jump for a while) |
| `landmark` | `MG_LM_*` from the art build (`tools/landmark_art.py`) | forest, falls, coast, autumn, ice, worldtree, works, reef, mountain, baobab, citadel: the great tree, cliff or mountain the gate stands in (192 x 176, from gate_x - 80). The gate is the valley's last thing: the view stops with its landmark, nothing may reach past gate_x - 80, and the last 96 px of road before the gate are clear of hazards |
| `art.kind` | `ART_KINDS` in `tools/levels.py` | blossom (known from the start), rain, sun, frost, gale: the valley's Secret Art. The other four are learned from the spirit orb, the first treasure in the hidden vault of a valley of that kind; until then D gives the blossom storm. `name` is the art's, `words` what she calls out in this valley |
| `decor[].kind` | `MG_D_*` from the art build | grass, lantern, rock |
| `who` | the ally art, in order | elder, maiden, spirit, sunboy |
| `secrets[].type` | | rose, gem, chest |

Each list holds as many entries as its table (`MG_PLATFORM_COUNT` and the
rest in `maiya_levels.h`); encounters are met in x order. Screen text is
plain upper-case ASCII, at most 38 characters (a guardian's taunt and reply
40, names 30).

## The Player's Guide to Hidden Things

`docs/maiya_secrets.md` lists, valley by valley, where the key, the hidden
vault and its spirit orb, the secrets, the sky lilies, the lives and the
other special items are, and what it takes to reach each. It is written
from these files by `python3 games/maiya/tools/secrets_doc.py`; run that
again after changing a stage.

## Checks

`levels.py` stops at the first mistake and names the file, the field and the
entry, before anything is written:

```
levels: 01_emerald_forest.json: encounters[1].enemy: unknown enemy "betle" (known: acidmoth, beetle, ...)
```

It checks the fields are there and of the right kind, numbers in range
(inside the stage's width, on the 224-line screen), names known, lists no
longer than their tables, and text that fits the screen; and what the game
needs of a stage: ledges between y 64 and 144 in whole blocks of 16,
encounters in x order, posted creatures standing on a ledge, hazards at
most 64 wide and clear of the arena, four captives in order before the
gate, the gate leaving room for the arena, vines from the road up at most
128 px (256 in a valley with an upper tier). `make level-check` then checks placement on what was built
(`tools/level_check.py`: pickups inside, under or floating off ledges,
ledges overlapping, two pickups drawn over each other), and names the stage
file in each finding:

```
ERROR: EMERALD FOREST (level 0, 01_emerald_forest.json), pickup 0 at (330,130): inside -- ...
```

Converting the tables that were hand-written in C to these files left the
P-ROM byte-for-byte the same.
