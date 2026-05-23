# Artbox Graphics Pipeline

> **v1.3.1 — Artbox Studio Pipeline tab + Asset Rules editor**
>
> `artbox/artbox_studio.py` gained EIGHT new tabs in v1.3.1:
>
> - **Hex Sprite Inspector** — pick any tile index, see its decoded
>   16×16 pixel grid, the raw C1/C2 ROM bytes (4 rows × 16 hex
>   bytes each), and a live palette-swap dropdown.  Useful when
>   debugging palette assignments or verifying tile order in C-ROM.
> - **Movement Designer** — build a frame sequence (tile + duration
>   per frame), loop-preview it at the chosen FPS, and the EXPORT
>   pane writes a ready-to-paste pair of C arrays (`anim_frame_tile[]`
>   and `anim_frame_dur[]` in vblank units).
> - **Level Designer** — paint a 20×14 tilemap.  Left-click paints
>   the current brush (tile + palette); right-click erases.  Tiles
>   are rendered as scaled thumbnails so you see exactly what's at
>   each cell.  Export gives a `level_tile[LEVEL_H][LEVEL_W]` C array.
>
> Plus the five tabs from the previous v1.3.1 commits:
>
> - **Pipeline** — run every step (img2neo / genscreens /
>   gen_sprite_meta / fixtiles / romdbimgimport / romtiles /
>   romdbfiximport / createromdb) with a status pill per step and a
>   live log.  Includes opt-in HD variants (`img2neo HD`,
>   `fixtiles HD`).  "Run Full Pipeline" chains the standard set
>   sequentially.
> - **Asset Browser** — tree of every PNG under `artbox/in/*` and
>   `artbox/infix/`, with dimensions / file size and a side-panel
>   preview at native scale.
> - **HD Compare** — pick any PNG, run it through both the standard
>   and HD pipelines, see the indexed outputs side-by-side.  Output
>   files land in `artbox/out/_hd_cmp/` (originals untouched).
> - **ROM Inventory** — table of every ROM kind (p1 / m1 / s1 / v1 /
>   c1 / c2) for every game folder, with size + mtime + present/absent
>   colour-coded.
> - **Asset Rules** — editor for `artbox/assets.cfg` with save/reload
>   buttons.
>
> **HD alt scripts** (introduced in v1.3.1):
>
> Two new scripts sit alongside the existing pipeline.  They do NOT
> replace `img2neo.py` / `fixtiles.py` — they're opt-in alternatives:
>
> - `artbox/img2neo_hd.py` — high-quality photo-to-NeoGeo conversion:
>   bilateral filter (edge-preserving smoothing), CLAHE (local contrast
>   equalisation on the L channel of Lab), unsharp mask, embedded 32×32
>   blue-noise dither.  Best for photographs and detailed art.  CLI:
>   `--no-bilateral`, `--no-clahe`, `--no-unsharp`,
>   `--dither blue|fs|ordered|none`.
>
> - `artbox/fixtiles_hd.py` — per-tile palette FIX conversion: each 8×8
>   FIX cell picks its own 16-colour palette.  `--sharp-text` binarises
>   the input for glyph/HUD sources.  Writes a sidecar `*.pal.json` with
>   the per-tile palette table for a future packer.
>
> The existing scripts continue to be the default for the demo's
> `make GAME=demo art` pipeline.

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

If `makeartbox.bat` fails with `PIL/img2neo not available`, install packages
through the same interpreter used by `py`:

```bat
py -0p
py -m pip --version
py -m pip install --upgrade pip
py -m pip install numpy pillow pypng
cd artbox
py -c "import sys; print(sys.executable); import PIL, numpy, png, img2neo; print('OK')"
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

## Artbox Studio UI

`artbox/artbox_studio.py` is a PyQt6 desktop tool for inspecting and editing
the graphics ROM without running the full pipeline.

```
python3 artbox/artbox_studio.py
```

### Tabs

| Tab | Function |
|-----|---------|
| **Tile Grid** | Scrollable viewer of all tiles from the combined C-ROM. Zoom 1–6×, per-asset palette, click to select, jump to any asset. |
| **Sprite Designer** | Configure a multi-strip sprite: tile base, strips, active rows, stride, palette bank. Exports a ready-to-paste C `char_set_sprite` call. |
| **Hitbox Editor** | Place draggable body and hit rectangles over the sprite preview. Exports a C hitbox struct definition. |
| **Pixel Paint** | 16-color pixel art editor for patching individual tiles directly in the C-ROM buffer. Saves back to `777-c1.c1` / `777-c2.c2`. |

Tile decoding follows the exact layout written by `romtiles.py`:

- Each C-ROM stores **64 bytes per 16×16 tile** (4 blocks × 8 rows × 2 bytes).
- Block order: top-right, bottom-right, top-left, bottom-left.
- `777-c1.c1` is byte-flipped (`romts.sh /f`): row bytes are `[plane_a, plane_b]`.
- `777-c2.c2` similarly: `[plane_c, plane_d]`.
- Color index per pixel: `d<<3 | c<<2 | b<<1 | a` (4-bit, maps into palette[0..15]).
