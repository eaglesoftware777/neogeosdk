# Native drawing sources

The active playable proofs are `assets/authored/fighters/kenji`, `jack` and
`amara`. Each contains a complete manifest for its referenced poses, six indexed
PNG layers per pose, flat PNGs, exact initial `.pixels` maps, contact sheets and
GIFs. The PNG layers are the editable source. The initial maps and `showcase_*`
recipes record construction; normal builds never rerun those drawing recipes.

After editing a fighter's layers, export one pose or the full fighter:

    python3 games/shadowleague/tools/native_art/recompose_fighter.py games/shadowleague/assets/authored/fighters/kenji --pose stance
    python3 games/shadowleague/tools/native_art/recompose_fighter.py games/shadowleague/assets/authored/fighters/kenji

The exporter stages and validates all referenced frames and palette banks before
publishing flat PNGs. It preserves collision rectangles and animation timing:
edit manifest.json contacts/boxes and fighters/<id>.json timings intentionally
when a drawing changes reach or support. GIFs/contact sheets in the fighter
folder are snapshots from the initial export; a layer-only edit does not refresh
them. Rebuild the cartridge and inspect actual motion after editing.

There are 203 referenced pose exports and 185 distinct drawings across the three
fighters. These are playable art proofs, not professional quality approval or a
completed 44-character redraw. The older studies described below remain inactive.

The editable source is indexed PNG, not a painting resized for the cartridge.
Each fighter study has six separate native 160x160 layer PNGs for each drawing,
a project.json with draw order, frame durations and a fixed [80,141] anchor,
flat PNG exports, contact sheets and GIF previews.

Edit the layer PNGs with any pixel editor that preserves indexed colors and
transparent index zero. Export one fighter without touching another:

    python3 games/shadowleague/tools/native_art/export_study.py games/shadowleague/assets/authored/rebirth/kenji

The export only composites opaque pixels at their original integer coordinates.
It does not draw, resize, rotate, interpolate or change the palette. The author_*
scripts describe the initial study contours and must not overwrite edited art.

The new body studies are not approved production animation and are deliberately
outside the playable fighter import directory. They are not a finished walk or
a complete special/super/throw set. Do not activate them as finished fighters.

A production fighter is activated by assets/authored/fighters/<id>/manifest.json:

    {
      "canvas": [160,160], "anchor": [80,141],
      "head": "head.png", "portrait_small": "portrait-small.png",
      "alt_palette": [[0,0,0], "...15 RGB entries..."],
      "frames": {
        "stance": {
          "file": "stance.png",
          "layers": {"far_arm":"...", "far_leg":"...", "torso":"...", "head_hair":"...", "near_leg":"...", "near_arm":"..."},
          "hurt": [[-12,0,24,40],[-15,40,30,38],[-10,78,20,22]],
          "hit": {"near_fist": [[20,65,12,10]]},
          "contacts": ["near_leg"],
          "lead_leg": "far", "lead_arm": "near"
        }
      }
    }

Every pose referenced by fighter JSON must have an export. Missing artwork fails
the build; the geometry renderer cannot replace it. Boxes use engine coordinates:
x relative to the root, y measured upward from feet. An empty hit map is a whiff.
Authored fighters support up to four 16-color banks, with transparent index zero
in each. Every 16x16 tile must fit fifteen opaque colors. The strip packer retains
separate top/bottom palette assignments even when two strips share pixel bytes.
The alternate palette must supply every global source index. Normal and mirror
palettes are loaded into separate runtime ranges, including each team's striker.
Uniform one-bank groups retain the original fast renderer. Source-to-C-ROM color
reconstruction and palette-map host tests must pass before an import is accepted.

The current identity is identity/logo.png (288x96), identity/subtitle.png (128x16),
cinema/title.png (352x224, cropped by the game to the visible 320x224), and three
native 80x96 portraits. The logo enters over the separately illustrated title
composition. Six subsequent story cards pan at whole-pixel offsets and use timed
captions, music, and immediate START/credit handling. These are an illustrated
attract sequence, not a prerendered movie or additional gameplay mode.

cinema/project.json defines card order, duration, and captions. Each native PNG
is directly editable. The current bust/portrait/title recipe is
showcase_presentation.py. Story scenes remain from author_cinema.py and the mark
from author_mark.py; author_identity.py and author_portraits.py record the earlier
superseded study. Never run an initial recipe over a manually edited export.

models/ contains a newer pair of guard/contact drawings per showcase fighter,
with independent six-layer PNG sources and stable [80,141] anchors. These prove
direction only. They still lack full movement, anticipation, recovery, specials,
supers and reactions, and are deliberately not activated as playable fighters.

footwork-20261014/ contains eight distinct drawings per showcase fighter. Kenji
alternates support in a karate step, Jack advances and gathers without crossing
his boxing stance, and Amara follows separate triangular ginga contours. Each
drawing has six indexed layer PNGs and exact initial pixel maps as text, plus
contact and foot-centre metadata in project.json. The PNG layers are the editable
source; export_study.py composites edited pixels without running the drawing
recipe. Per-fighter contact sheets and footwork.gif are previews only. These
are motion studies, not finished production movement or full combat sets.

announcements/ contains sixteen native FIX word artworks with indexed PNGs,
initial pixel maps and project.json. A normal build reads those PNGs directly.
test_shadowleague_announcements.py decodes the actual S-ROM and checks every
source index. A FIX-only asset rebuild also refreshes header dimensions and
palette tables; it does not regenerate C-ROM fighter bodies.

The native presentation occupies C-ROM tile addresses above 0xFFFF. The renderer
uses the hardware's address bits and the cartridge reserves two 8-MiB C lanes.
P-ROM code stays fixed below 1 MiB; presentation descriptors are copied from two
aligned 1-MiB data banks. No bank-window pointers enter the deferred draw queue.

The four active arena sources remain assets/authored/<arena>-master.png,
byte-identical to the base revision. The optional indexed replacement path is
assets/authored/arenas/<arena>.png at 512x224; no replacements are active.
That optional path preserves colors and rejects an overfull tile or more than
eight banks. The separate rebirth/arena dojo study is not activated.
