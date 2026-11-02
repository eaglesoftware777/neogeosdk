"""Add frames from a new painted sheet to an approved fighter, in its own colours.

    python3 import_extra.py ID SHEET NAME,NAME,... [--preview DIR] [--replace]

SHEET is a magenta sheet whose first figure is the fighter's guard stance (the
scale key, as in import_fighter.py); NAME... name the figures after it, in
reading order ("-" skips one). The approved frames and the fighter's four
tile palettes are left exactly as they are: every new figure is reduced to
native pixels at the same scale as the approved ones (the manifest's
uniform factor and the roster height), placed on the same canvas, and each
16x16 tile of it takes the one of the four palettes that draws it best.
The manifest gets each new frame with its root, hurt and hit boxes and foot
contacts, the way import_fighter.py writes them.
"""
import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
import sprites as sp  # noqa: E402
import import_fighter as imp  # noqa: E402
from roster import R  # noqa: E402

GAME = TOOLS.parents[1]
AIRBORNE = ('jump', 'air', 'flip', 'leap', 'j_')


def bank_palette(folder):
    """The fighter's 60 colours (4 banks x 15), as the approved frames carry them."""
    im = Image.open(folder / 'stance.png')
    raw = np.array(im.getpalette()[:3 * 61], np.uint8).reshape(-1, 3)
    return raw[1:61]


def fit_banks(canvas, rgb_of, gpal):
    """canvas: label image (0 empty); rgb_of[k]: the colour of label k. Each 16x16 tile
    takes the bank whose 15 colours draw it best; returns global indices 1..60."""
    lab_bank = [sp.to_lab(gpal[b * 15:(b + 1) * 15].reshape(1, -1, 3))[0] for b in range(4)]
    out = np.zeros(canvas.shape, np.uint8)
    H, W = canvas.shape
    for ty in range(0, H, 16):
        for tx in range(0, W, 16):
            tile = canvas[ty:ty + 16, tx:tx + 16]
            on = tile > 0
            if not on.any():
                continue
            cols = sp.to_lab(rgb_of[tile[on]].reshape(1, -1, 3))[0]
            best, best_err, best_idx = 0, None, None
            for b in range(4):
                d = ((cols[:, None, :] - lab_bank[b][None, :, :]) ** 2).sum(-1)
                idx = d.argmin(1)
                err = float(d[np.arange(len(idx)), idx].sum())
                if best_err is None or err < best_err:
                    best, best_err, best_idx = b, err, idx
            sub = np.zeros(tile.shape, np.uint8)
            sub[on] = (best * 15 + best_idx + 1).astype(np.uint8)
            out[ty:ty + 16, tx:tx + 16] = sub
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('id')
    ap.add_argument('sheet', type=Path)
    ap.add_argument('names')
    ap.add_argument('--rows', type=int, default=2)
    ap.add_argument('--folder', type=Path, default=None)
    ap.add_argument('--preview', default=None)
    ap.add_argument('--replace', action='store_true', help='overwrite frames of these names that already exist')
    ap.add_argument('--scale', type=float, default=1.0,
                    help="correct the key's scale (when the sheet's key stance is drawn taller or shorter than the approved one)")
    a = ap.parse_args()
    folder = (a.folder or GAME / 'assets/authored/fighters' / a.id).resolve()
    manifest = json.loads((folder / 'manifest.json').read_text())
    names = a.names.split(',')
    for n in names:
        if n != '-' and n in manifest['frames'] and not a.replace:
            raise SystemExit(f'{a.id}: frame {n} exists (use --replace)')
    gpal = bank_palette(folder)
    # the reduction's palette: the 60 colours, the darkest first (sp.native reads index 1 as ink)
    order = np.argsort(sp.to_lab(gpal.reshape(1, -1, 3))[0][:, 0])
    pal = gpal[order]
    rgb = sp.load(a.sheet)
    figs, _ = sp.figures(rgb, a.rows)
    if len(figs) != len(names) + 1:
        raise SystemExit(f'{a.sheet.name}: expected {len(names) + 1} figures (the key first), found {len(figs)}')
    factor = float(manifest.get('source_fit', {}).get('uniform_factor', 1.0))
    key = figs[0][0]
    scale = factor * imp.stance_px(R[a.id]) / (key[3] - key[1]) * a.scale
    added, frames = [], {}
    for (box, frgb, mask), name in zip(figs[1:], names):
        if name == '-':
            continue
        idx = sp.native(frgb, mask, scale, pal)
        idx = sp.outline(idx, pal)
        canvas, root = sp.place(idx)
        lab, n = ndi.label(canvas > 0, np.ones((3, 3), bool))
        if n > 1:
            sizes = ndi.sum(canvas > 0, lab, range(1, n + 1))
            for k, size in enumerate(sizes, 1):
                if size < 3:
                    canvas[lab == k] = 0
        air = any(t in name for t in AIRBORNE)
        if not air:
            drop = sp.ROOT[1] - np.nonzero(canvas)[0].max()
            if drop > 0:
                canvas = np.roll(canvas, drop, axis=0)
        if any(e.any() for e in (canvas[0], canvas[-1], canvas[:, 0], canvas[:, -1])):
            raise SystemExit(f'{a.id}/{name}: the figure reaches the canvas edge')
        rgb_of = np.vstack([[0, 0, 0], pal]).astype(np.uint8)
        final = fit_banks(canvas, rgb_of, gpal)
        sp.save_indexed(final, gpal, folder / f'{name}.png')
        kind = imp.STRIKE.get(name, 'fwd')
        hb = imp.strike_box(final, kind, root)
        manifest['frames'][name] = {
            'file': f'{name}.png', 'root': root, 'hurt': imp.hurt_boxes(final, root),
            'hit': {limb: [hb] for limb in imp.LIMBS},
            'contacts': [] if air else ['far_leg', 'near_leg'],
        }
        frames[name] = final
        added.append(name)
    (folder / 'manifest.json').write_text(json.dumps(manifest, indent=1) + '\n')
    if a.preview:
        sp.preview([frames[n] for n in added], gpal, Path(a.preview) / f'{a.id}-extra.png', scale=2, cols=8)
    print(a.id, 'added', ','.join(added))


if __name__ == '__main__':
    main()
