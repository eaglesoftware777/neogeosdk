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

## Resampling

Every resize in the pipeline goes through `resize_rgba_linear()`
(`img2neo.py`), not `PIL.Image.resize()` directly.

PIL resizes gamma-encoded sRGB: it takes the arithmetic mean of the stored
numbers, and those numbers are roughly the 1/2.2 power of the light they
stand for. Averaging in that space is not averaging light. A checkerboard
of black and white, which should resolve to a mid grey of 188, resolves to
128 instead — and the same loss, smaller but in the same direction, applies
to every downscale. Measured across the demo's character art, gamma-space
downscaling was throwing away 8–22% of each sprite's luminance before the
quantiser ever saw it, which is most of what "the scaled sprite looks
washed out next to the source" means.

`resize_rgba_linear()` converts to linear light, resizes there, and converts
back. Alpha gets the same care: RGB is premultiplied before the filter so a
transparent pixel's colour cannot leak into its opaque neighbours, and
unpremultiplied afterwards.

It also runs `alpha_bleed()` at **source** resolution first. That order
matters. A sprite cut out against white has white sitting under its
transparent pixels; a filter run before the bleed mixes that white into the
contour, baking in the halo `alpha_bleed()` exists to remove — by the time
the bleed runs, the halo is opaque pixels and it can no longer see it.

## Palette banks

Per-tile palettes have to collapse into shared hardware banks, and how two
palettes are compared decides how much colour survives that.

`cluster_and_remap_tile_palettes()` compares them as **sets**, with a
symmetric mean-nearest-neighbour distance in Lab: how far each colour of one
palette sits from its closest counterpart in the other, averaged both ways.

The reason is that these palettes come out of k-means, so their slot order
is whatever the seeding happened to produce. Comparing slot *i* against slot
*i* — the obvious thing to do — means two tiles holding the same fifteen
colours in a different order score as maximally different. They take a bank
each, the bank budget runs out on duplicates, the tolerance widens to
compensate, and palettes that really *are* different get merged. Order
sensitivity spends the budget on duplicates and pays for it in fidelity.

Each bank is then re-fitted to every colour its members hold, with a few
Lloyd iterations in Lab. A greedy pass has to name a bank before it knows
who will join it, so without this a bank is one tile's palette and every
other member is quantised onto it.

Mean dE against the pre-clustering (per-tile optimal) reference, measured on
three of the demo's sources:

| Source | Before | After |
|---|---|---|
| `zz_bg/0.png` | 11.39 | 1.88 |
| `backgrounds/0.png` | 11.71 | 2.25 |
| `npcs/…_sky_boss.png` | 28.18 | 5.37 |

`epsilon` is in dE, so a threshold means something; the default is 6.

## Import size

**Import an asset at the size it is drawn.** This matters more than any
single setting in the quantiser.

The sprite chip can shrink, and it is tempting to import large and let the
hardware fit the art to the scene. But the hardware does not resample when
it shrinks — it drops whole rows and columns, and what it drops is a dither
pattern the quantiser chose for pixels it expected to survive. Halve a
sprite in hardware and half of that pattern is gone; quarter it and the
shape goes with it. The source can be flawless and the result still arrives
as a smear of colour.

The pipeline's own resampling is linear-light Lanczos followed by a
quantiser that dithers against the pixels it is actually producing. So the
rule is: set each rule's `target_width`/`target_height` to the largest size
that asset is ever drawn, and draw it at full scale.

Two further benefits fall out of it: the asset costs proportionally less
C ROM, and it needs fewer hardware sprite strips, which is the scarcer
budget in a busy scene.

`cat_sky_planes`, `cat_sky_opponents`, `cat_sky_bosses` and `cat_big_enemy`
exist for exactly this — per-asset ceilings for sprites whose scene size is
known.

One caveat before lowering a ceiling on art that is already in use: changing
it moves that asset's strip count and tile stride, and
`ng_char_set_sprite()` silently drops a bind whose asset window fails
validation. A character bound through that path then keeps whatever it had
before, which shows up as stale or missing art rather than as an error. The
demo's own `cat_characters` ceiling is still 256 for that reason. Check what
a ceiling change does to `NG_ASSET_STRIPS_*` and to any slot map built
around it before assuming it is free.

## Synthesised backdrops

`artbox/gen_starfield.py` generates a tiling deep-space page — a multi-hue
nebula from wrapping fractal noise, dust lanes, and three layers of stars.
The noise lattice wraps, so the page joins itself exactly rather than
approximately, and a backdrop scrolled as two copies shows no line at the
wrap.

```
python3 artbox/gen_starfield.py left.png right.png --seed 7
```

Given several outputs it generates one page at the combined width and cuts
it into columns, so pages laid side by side join without a seam down the
middle. Everything is synthesised, so the result carries no licence.

## HD conditioning

`ARTBOX_ENHANCE=0`, or `img2neo_tile.py --no-enhance`, turns off two passes
that otherwise run before quantisation:

- an edge-preserving **bilateral** smooth, which calms the compression noise
  and gradient banding that fifteen colours turn into blotches without
  softening outlines — a plain blur would take the outlines with it;
- **CLAHE** on the Lab lightness channel only, so local contrast survives the
  quantisation while hue and chroma stay where the artist put them.

They are **on by default**, because the source art in this tree is HD —
photographs, renders and upscaled scans, where noise and a wide tonal range
both survive the resize and then fight the 15-colour budget. Turn them off
for art authored at the target size, which has neither problem: running them
over it rewrites the artist's tone choices for nothing.

## Alternative pipelines

| Route | Script | When |
|---|---|---|
| Default (tile-local) | `img2neo_tile.py` | Every build. Per-tile k-means++, per-tile dither, Lab set-distance bank dedup, Lab-nearest remap. |
| CRT | `img2neo_crt.py` | `make art-crt` or `ARTBOX_CRT=1`. Lab k-means, horizontal-biased dither, gamma 1.20 / contrast 1.10 pre-boost. |
| HD | `img2neo_hd.py` | Opt-in. Bilateral filter, CLAHE on the Lab L channel, unsharp mask, blue-noise dither. Best for photographs. |
| Legacy | `ARTBOX_LEGACY=1` | The original nearest-neighbour path, kept for diffing. |
| FIX HD | `fixtiles_hd.py` | Per-tile palette FIX conversion; `--sharp-text` binarises glyph sources. |

`img2neo.py` also exposes `alpha_bleed()`, which stops anti-aliased contours
baking the source PNG's hidden transparent-pixel RGB (usually near-white)
into the indexed sprite. That is the fix for the pale halo around sprite
edges, and `resize_rgba_linear()` runs it before any resize so the halo
never gets a chance to become opaque.

A sprite whose background is opaque instead of transparent is a different
problem and no filter can fix it: index 0 is transparency on this hardware,
so an opaque matte is not a colour that happens to be wrong, it is a solid
block the sprite carries around with it. Repair the source with
`artbox/fix_sprite_alpha.py PATH`, which flood-fills only background
connected to the image border and leaves interior colour alone.

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

Animated sprites share a derived 15-colour master palette per animation
family (`build_master_sprite_palettes()`).  The demo keeps the warrior,
selection portraits, eagle, and each remaining character row in separate
families.  NPC filenames are grouped by their stable name prefix.  This keeps
colors stable within an animation without forcing unrelated artwork into one
15-color budget.  A sprite that looks wrong after being renamed or moved
between categories may have entered the wrong palette family.

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
