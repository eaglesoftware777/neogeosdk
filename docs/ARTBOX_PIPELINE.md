# Artbox Graphics Pipeline

Artbox converts PNG sources into Neo Geo graphics data and generated metadata.
The current pipeline is rule-driven through `artbox/assets.cfg`.

## Inputs

- `artbox/in/` contains PNG artwork for sprite pages and screen/background pages.
- `artbox/infix/` contains FIX-layer source artwork.

## Rule Fields

Each `[rule:name]` section in `artbox/assets.cfg` can define:

- `pattern`: filename glob, such as `sprite_*.png`
- `mode`: `screen` or `sprite`
- `category`: gameplay category written into generated metadata
- `fit`: `crop` or `pad`
- `anchor`: placement anchor such as `center` or `bottom-center`
- `target_width` / `target_height`: output canvas size
- palette and image-processing settings such as `dither`, `contrast`, and `kmeans_iters`

## Categories

Generated metadata uses these category constants:

```c
#define NG_ASSET_CATEGORY_BACKGROUND 0
#define NG_ASSET_CATEGORY_MAIN_CHARACTER 1
#define NG_ASSET_CATEGORY_OPPONENT 2
#define NG_ASSET_CATEGORY_NPC 3
```

Current naming rules:

| PNG pattern | Mode | Category |
| --- | --- | --- |
| `[0-9].png` | `screen` | `background` |
| `background_*.png` | `screen` | `background` |
| `zz_npc_forest_alley.png` | `screen` | `background` |
| `sprite_*.png` | `sprite` | `main_character` |
| `opponent_*.png` | `sprite` | `opponent` |
| `z_npc_*.png` | `sprite` | `npc` |

## Generated Files

The pipeline writes:

- `artbox/assets_manifest.json`
- `artbox/out.srt`
- `artbox/sprite_meta.h`
- `artbox/screens.c`
- Neo Geo graphics ROM data consumed by the main build

`sprite_meta.h` exposes `NGSpriteAssetMeta` records with tile base, palette
bank, sprite strips, active rows, content bounds, mode, and category.

## Build

Run the full artbox pipeline:

```bash
cd artbox
./makeartbox.sh
```

Windows:

```bat
cd artbox
makeartbox.bat
```

Then rebuild the ROM:

```bash
make
```

## Runtime Use

The 68000 demo includes `artbox/sprite_meta.h` and uses the generated metadata
to place sprite frames without hardcoding each frame's tile offsets, palette
bank, active rows, and visible strips.

Gameplay code can use `meta->category` to distinguish backgrounds, main
character frames, opponents, and NPC frames while keeping the graphics ROM
layout stable.
