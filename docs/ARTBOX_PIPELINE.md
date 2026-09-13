# Artbox Graphics Pipeline

**Eagle Software · Neo Geo SDK v1.7.0**

Artbox converts PNG sources into Neo Geo graphics data and the generated
metadata your game code binds against. It is the stage that decides how
your art actually looks on the machine, so it is worth understanding rather
than treating as a black box.

---

## The colour target

A Neo Geo colour word is not RGB444. Bit 15 is the common "dark" bit,
bits 14-12 carry the low bit of each five-bit channel, and bits 11-0
carry their upper four bits. The common dark bit produces two interleaved
intensity grids; it is not an independent sixth bit for each channel.
The converter fits on this hardware lattice and verifies the packed words.

Each sprite or tile can use **15 colours plus transparent** — 4 bits of
index, where index 0 is transparent.

## Default quantiser

`artbox/romdbimgimport.py` routes normal builds through
`artbox/palette_banks.py`. Both makefiles use this path. Older experimental
converters remain available, but their contrast enhancement, CLAHE and
per-tile diffusion are not part of the default import.

1. Fit the first 15-colour palette to the whole image's visible content,
   weighted by source pixel frequency. Refine in CIE-Lab, snapping to the
   hardware grid during refinement rather than only after fitting.
2. Preserve that whole-image palette as a fallback. For each extra bank,
   fit the worst remaining tile, then consider refitting the whole region
   that benefits. Accept a bank only when it reduces nearest-colour error.
3. Select one bank per 16x16 tile. Keep the indices and that bank map
   together through C-ROM encoding, generated metadata and runtime binding.
4. Apply no dither to normal sprite art. Backgrounds may use low-amplitude,
   image-aligned ordered dither; no diffusion wave crosses sprite contours.

Solid core pixels train sprite palettes where sufficient core coverage
exists. Tiny antialiased craft fall back to visible pixels so their palette
is not fitted to a few highlights alone. Alpha below 128 becomes index zero.
Opaque black remains a nonzero index. The default path does not force extra
contrast or saturation onto the artist's colours.

`palette_banks = 1..16` is an upper limit, not a promise to consume every
bank. Static sprites now honour it as screens do. Animation families retain
one shared master palette; raising their budget alone does not enable
per-frame extra banks, which could introduce colour changes during motion.

## Resampling

Screen fitting uses `fit_screen_for_display()` and accounts for the chosen
vertical hardware shrink. Sprite fitting uses `fit_sprite_rgba()`: preserve
aspect ratio, reduce only when necessary, then pad transparently at the
requested anchor. The current sprite fitter rounds canvas dimensions down
to multiples of 16, so choose explicit multiples for predictable output.
It uses Pillow's RGBA Lanczos thumbnail path, not the optional linear-light
helper available in `img2neo.py`.

`prepare_source_sprite()` applies optional border-connected halo cleanup
and alpha-edge colour bleeding after fitting. Original PNG files are not
rewritten. Avoid global halo stripping on luminous effects: bright pixels
can be intentional art, not a matte defect.

## Palette banks

`allocate_extra_palettes()` assigns absolute hardware banks after fitting.
Byte-identical ordered colour words share one slot, including extra banks.
This step does not approximate or reorder colours: pixel indices are already
encoded, so two palettes containing the same colours in a different order
are not interchangeable. The verifier compares the actual words, not only
the manifest's `palette_key` hash.

The generated `NGArtAsset.tile_palettes` pointer starts at the artwork's
first active tile. It uses the same row stride as the tile data. Bind it
with `ng_sprite_group_set_palette_map()` or `ng_char_set_palette_map()`;
loading the banks without binding this map renders the right pixels in the
wrong colours. Neither setter loads palettes or writes VRAM immediately.

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

The pipeline fits and quantises against the pixels it actually produces. The
rule is: set each rule's `target_width`/`target_height` to the largest size
that asset is ever drawn, and draw it at full scale.

Smaller assets need fewer hardware sprite strips. This ROM layout still
reserves 256 tiles per asset, so smaller imports currently leave transparent
C-ROM padding rather than reducing the reserved allocation.

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

## Tile stride

Every asset carries a `tile_stride` in its metadata: the width of the
canvas it was imported onto, in tiles. Row *n* of the artwork begins that
many tiles after row *n-1*.

Anything binding an asset has to use it — `ng_char_set_tile_stride()`, or
`ng_sprite_group_set_tile_stride()`. **Do not assume 16.** Sixteen is the
stride of a 256 px canvas and nothing more; an asset imported onto a
narrower one has a narrower stride, and binding it with 16 reads every
row after the first from further along the C ROM than the artwork is. The
symptom is the right palette over the wrong art — a recognisable shape in
plausible colours that is not the asset you asked for, which is easy to
mistake for a quantiser problem.

Both engines validate `ng_char_bind_asset()` against the incoming asset's
stride and bounds before modifying the character. It returns zero on
failure and leaves the previous frame intact; check its return value.
The lower-level `ng_char_set_sprite()` returns void and retains its existing
bounds checks. Bind the map after setting sprite geometry and stride:

```c
ng_char_set_sprite(character, first_slot, asset->strips, asset->active_rows,
                   asset->tile_base, asset->palette_bank);
ng_char_set_tile_stride(character, asset->tile_stride);
ng_char_set_palette_map(character, asset->tile_palettes);
```

For an atomic bind, zero-initialize `NGSpriteAssetView`, fill its geometry,
range, offsets and `tile_palettes`, then call `ng_char_bind_asset()`.
The map contains absolute bank numbers and must remain alive while bound;
generated ROM tables satisfy that lifetime. NULL selects the single bank in
`character->palette`. `ng_char_set_sprite()` resets the map, so set it again
after changing the asset. The C++ method is `NGCharacter::setPaletteMap()`;
the same C entry point is available in both engines. Rebuild all game objects
when updating the SDK because NGCharacter and NGSpriteAssetView gained a field.

## Palette RAM budget

The SDK allocates 239 asset banks: 16..254. It reserves 0..15 for FIX and
255 for backdrop/HUD use. Each asset bank has 15 visible entries plus
transparent index zero, so the asset budget is 3585 visible colour entries.
That is an allocation ceiling, not a guarantee of that many distinct colours.

That budget is global to a game — an asset owns its banks for the life of
the ROM, so every asset's banks have to coexist. Two things keep it from
being wasted:

**Assets holding the same palette share a bank.** Base banks used to be
positional, one per asset, so an animation whose frames all render
against one shared palette occupied a bank per frame holding the same
colours. Base and extra banks now share only when all sixteen ordered words
match. `palette_key` is a stable diagnostic fingerprint; the verifier reads
the actual base words from neopal.bin and extra words from the manifest.
Changing a shared bank for a palette effect changes every object using it.
Copy to a separately reserved effect bank before an independent colour effect.

**Extra banks go to what covers the screen.** A background occupies the
whole frame and a sprite occupies a fraction of it, so the budget is
weighted accordingly. Budgets live in each game's `artbox/assets.cfg`, not
only in the shared generator defaults. Existing configs are preserved.
Sky Lance has enough capacity for 16-bank bosses, 4-bank opponents and
3-bank player craft. The demo retains conservative single-bank animation
families. Do not copy one game's budgets into another without rebuilding
and checking total occupancy. Additional banks are used only when beneficial.

### What extra banks cannot fix

A tile reads one palette. Extra banks help where the variety is *between*
tiles and not at all where it is *within* one, which is why they transform
a background more than a small character. An earlier isolated fitting study
measured these mean dE values; they are examples, not current build totals:

| | 1 bank | 8 banks | 16 banks | one per tile |
|---|---|---|---|---|
| background | 3.93 | 3.18 | 2.99 | 2.29 |
| boss sprite | 11.78 | 9.30 | 8.23 | 5.85 |
| character | 11.39 | 10.16 | 9.77 | 8.06 |

A bank per tile is the floor this hardware allows, and it is out of reach
for a game with 86 character frames — that alone would want more palette
RAM than exists.

### The second hardware palette bank

The hardware has two complete palette sets, selected by a write to
`REG_PALBANK0` / `REG_PALBANK1`, so 8192 entries can be resident and 4096
displayed. The allocator currently uses one set. The second set may be
useful for prepared whole-scene palette switches, but it does not remove
the 15-visible-colours-per-tile limit.

## Palette anchors

Index zero is always transparent, independently of its RGB value. The
default quantiser preserves all distinct hardware colours when they fit
within fifteen entries. More complex sources use frequency-weighted fitting;
there is no automatic UI detector or unconditional black/white reservation.
The optional older converters have separate anchor heuristics.

## HD conditioning

The older `img2neo_tile.py` and optional HD tools expose bilateral filtering,
CLAHE and sharpening. Those controls do not run in the default source-faithful
path. Compare any optional converter against the source and actual C-ROMs
before adopting it; stronger contrast or more dither is not automatically
more accurate. No converter can recover detail removed by resizing or make
the hardware display unrestricted RGB per pixel.

## Alternative pipelines

| Route | Script | When |
|---|---|---|
| Default | `palette_banks.py` via `romdbimgimport.py` | Whole-image fallback, error-driven extra banks, stable animation masters. |
| CRT | `img2neo_crt.py` | `make art-crt` or `ARTBOX_CRT=1`. Lab k-means, horizontal-biased dither, gamma 1.20 / contrast 1.10 pre-boost. |
| HD | `img2neo_hd.py` | Opt-in conditioning; measure against the source before adopting it. |
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

Host regressions and captured runtime checks:

```sh
make unit-tests
python3 -m unittest discover -s tests -p 'test_artbox*.py' -v
python3 tools/game_capture.py --game skylance --output /tmp/sky-capture --seconds 180
python3 tools/artbox_runtime_report.py --game skylance --capture /tmp/sky-capture --strict
```

The capture stores screenshots, SCB VRAM and palette RAM. The runtime report
compares populated tile entries with generated bank assignments and colour
words. Palette-effect scenes intentionally change colours and are not suitable
for an unconditional strict comparison. Captures mute audio and cannot verify
sound quality. Rebuild P1 after rebuilding art so runtime tables match C-ROMs.

For source fidelity, retain `assets_manifest.json` and `neorom.db` from the
previous build in a separate folder, then run:

```sh
python3 tools/artbox_quality_report.py --game skylance --before /tmp/sky-before --output /tmp/sky-quality
```

This decodes both C-ROMs, checks them against the importer indices and writes
source/before/after comparisons plus per-asset Lab error. Matching asset lists
are required. It accepts manifests produced on either Windows or WSL.

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
- `fit`: `crop`/`contain` for screens, `pad` for sprites, or `native` for pre-tiled screens
- `anchor`: placement anchor such as `center` or `bottom-center`
- `target_width` / `target_height`: output canvas size
- `palette_banks`: maximum 1..16 banks for static art; shared animation masters remain single-bank
- `dither`: use `none` for sprites or `ordered` for gentle background dithering
- `halo_strip` / `halo_luma_threshold`: optional sprite border cleanup
- `display_shrink_y`: the screen fitting ratio expected at runtime
- older image-processing settings such as `contrast` and `kmeans_iters` affect optional converters, not the default fitter

`fit = native` bypasses screen aspect compensation and requires the source
dimensions to equal the target canvas. Use it for pixel art that already has
a tile-aligned repeat, such as the demo's 144-pixel Sky Lance sky loop.
Applying `contain` to that art adds transparent bands and changes the repeat
distance. Other screens retain their existing fitting and colour algorithms.

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
