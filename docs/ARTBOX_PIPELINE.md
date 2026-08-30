# Artbox Graphics Pipeline

**Eagle Software · Neo Geo SDK v1.7.0**

Artbox converts PNG sources into Neo Geo graphics data and the generated
metadata your game code binds against. It is the stage that decides how
your art actually looks on the machine, so it is worth understanding rather
than treating as a black box.

---

## The colour target

A Neo Geo colour word is not RGB444. Bit 15 is the "dark" bit, bits 14-12
carry the least-significant bit of each channel, and bits 11-0 carry the
top four bits of R, G and B. The result is **5 bits per channel, 32 levels,
in steps of 8**. Every colour the pipeline emits is snapped onto that
lattice.

Each sprite or tile can use **15 colours plus transparent** — 4 bits of
index, where index 0 is transparent.

## The v1.7.0 quantiser

`artbox/img2neo_tile.py` is the default conversion path for every build;
both `make art` (Linux) and `make -f MakefileWin32.mak art` export
`ARTBOX_TILE=1` so this is the route every ROM takes.

The v1.7.0 pass rebuilt it around four ideas. Together they moved mean
colour error (CIE-Lab dE) from 9.10 to 8.33 and p95 error from 18.19 to
15.66 across the reference asset set, while cutting measured dither
speckle by a third.

**Match in Lab, not in RGB.** Palette selection, the k-means refinement,
and the final per-pixel remap all run in CIE-Lab through a precomputed
32³ lattice LUT (`lab_lattice_lut()`, `lab_palette()`). Matching in one
space and measuring in another is what makes a quantiser that scores well
look wrong; doing everything in Lab is what makes the numbers mean
something. An earlier attempt that matched in luma-weighted YCbCr while
measuring in Lab made sprites measurably *worse*.

**Refit the palette onto the hardware grid, then re-optimise.** k-means
picks colours in continuous space, and snapping them to the 32-level
lattice afterwards moves every centroid slightly — so the result is no
longer optimal for the colours it was chosen for.
`refine_palette_on_lattice()` runs Lloyd iterations with the snap *inside*
the update step, keeps a move only when total error actually drops, and
re-seeds duplicate or unowned slots from the worst-served pixels. Nothing
is wasted on a colour no pixel wants.

**Dither with blue noise, and only where dithering helps.**
`_void_and_cluster_mask()` builds a 32 × 32 void-and-cluster blue-noise
mask (0.04 % of its energy in the low frequencies), used to order the mix
between the two nearest palette entries along a serpentine scan. A dead
band skips the mix entirely when a pixel sits within 18 % of either
endpoint — those are the pixels where dithering adds visible speckle
without adding colour. This is what removed the "noise dots" from flat
regions.

**Push contrast and saturation before clustering, with a soft knee.**
Fifteen colours across a whole image pull everything toward the middle of
the gamut, because the average of a cluster is always less saturated than
its members. `_neo_palette_pop()` compensates before clustering — a
contrast blend and a saturation push, both stronger for sprites than for
backgrounds — and compresses the top and bottom eighth with a `tanh` knee
instead of clipping. Clipping folds every boosted highlight onto pure
white, which throws colour detail away before k-means ever sees it.

Sprite and background paths are tuned separately: sprites get a wider
smoothing cut and no ordered mixing (a dithered sprite edge reads as
noise against a moving background), backgrounds get the full treatment.

Results are cached; `_VIVID_CACHE_VERSION` invalidates the cache when the
algorithm changes.

## Alternative pipelines

| Route | Script | When |
|---|---|---|
| Default (tile-local) | `img2neo_tile.py` | Every build. Per-tile k-means++, per-tile dither, greedy MAE bank dedup, Lab-nearest remap. |
| CRT | `img2neo_crt.py` | `make art-crt` or `ARTBOX_CRT=1`. Lab k-means, horizontal-biased dither, gamma 1.20 / contrast 1.10 pre-boost. |
| HD | `img2neo_hd.py` | Opt-in. Bilateral filter, CLAHE on the Lab L channel, unsharp mask, blue-noise dither. Best for photographs. |
| Legacy | `ARTBOX_LEGACY=1` | The original nearest-neighbour path, kept for diffing. |
| FIX HD | `fixtiles_hd.py` | Per-tile palette FIX conversion; `--sharp-text` binarises glyph sources. |

`img2neo.py` also exposes `alpha_bleed()`, applied after `fit_sprite_rgba`
so anti-aliased contours stop baking the source PNG's hidden
transparent-pixel RGB (usually near-white) into the indexed sprite. That is
the fix for the pale halo around sprite edges.

## Asset ordering and ids

Categories are processed in a fixed order:

```
backgrounds, characters, effects, eyecatcher, npc, screens, titles
```

then any other directory alphabetically. Within a category, files are
processed in filename order.

**An asset id is its 1-based position in that sequence.** Inserting a file
in the middle of a category renumbers everything after it, and your game
code refers to assets by id. The shipped games pad filenames to control
placement, and you should too.

`characters` and `npcs` share **one derived 15-colour master palette per
group** (`build_master_sprite_palettes()`); every other category gets
per-asset palettes. A sprite that looks wrong after being moved between
categories is almost always hitting this, not a conversion bug.

Rule matching in `assets.cfg` has three priorities; the lowest is a rule
with an empty `match_category` plus a filename pattern, which is how the
`legacy_screens` rule (`[0-9].png`, `mode = screen`) catches numbered files
in any directory.

## Generated outputs

| File | Contents |
|---|---|
| `<id>-c1.c1`, `<id>-c2.c2` | Sprite tile data, the C-ROM pair |
| `<id>-s1.s1` | FIX tile ROM |
| `screens.c` | One `showScreenN()` entry point per asset |
| `sprite_meta.h` | Per-asset geometry — tile base, strips, rows, padding |
| `assets_manifest.json` | The manifest the rest of the pipeline reads |
| `neo.pal`, `std.pal`, `neopal.bin` | Palette data |
| `infix_palettes.h` | FIX palettes for INFIX photo pages |

`main.c` in each game holds the generated `ng_screen_table[]` and
`ng_screen_count`, which `ng_bg_set_by_id()` resolves screen ids through. A
game with no screen art links against a weak fallback in `ng_bg.c` instead.

## Verifying a build

```sh
python3 tools/verify_artbox_palettes.py --root . --game demo
python3 tools/verify_sfix_output.py
python3 artbox/validate_assets.py
```

`make art` runs the palette check itself and fails the build on a mismatch.

## Artbox Studio

`artbox/artbox_studio.py` is the PyQt6 desktop front end for everything
above. It does not replace the make targets — it drives the same scripts —
but it makes the feedback loop visual.

| Tab | What it does |
|---|---|
| **Pipeline** | Runs every step (img2neo, genscreens, gen_sprite_meta, fixtiles, romdbimgimport, romtiles, romdbfiximport, createromdb) with a status pill per step and a live log. "Run Full Pipeline" chains the standard set. |
| **Asset Browser** | Tree of every PNG under `artbox/in/*` and `artbox/infix/`, with dimensions, file size, and a native-scale preview. |
| **Hex Sprite Inspector** | Pick a tile index and see its decoded 16 × 16 grid, the raw C1/C2 ROM bytes, and a live palette-swap dropdown. The tool for debugging palette assignment and tile order. |
| **Movement Designer** | Build a frame sequence (tile + duration), loop-preview it at a chosen FPS, and export `anim_frame_tile[]` / `anim_frame_dur[]` C arrays in vblank units. |
| **Level Designer** | Paint a 20 × 14 tilemap with tile thumbnails; export a `level_tile[LEVEL_H][LEVEL_W]` C array. |
| **HD Compare** | Run one PNG through both the standard and HD pipelines and see the indexed results side by side. Output lands in `artbox/out/_hd_cmp/`; originals are untouched. |
| **ROM Inventory** | Every ROM kind (p1 / m1 / s1 / v1 / c1 / c2) for every game folder, with size, mtime, and present/absent colour coding. |
| **Asset Rules** | Editor for `artbox/assets.cfg`, with save and reload. |

---

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
