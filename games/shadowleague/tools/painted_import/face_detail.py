"""Eyes, brows and mouths for the painted fighters' faces.

The painted import shrinks each sheet figure to about a third of its size
by averaging every block of source pixels, then drops lone pixels as
specks. A face is ten pixels wide in the game, so its eyes, brows and mouth
-- a few dark source pixels in a block of skin -- averaged into skin and the
faces read empty. This finds those features again from the source sheets
and paints them into the approved frames, changing nothing else.

For each fighter the importer runs twice on its sheets into a scratch
folder, without the tile-bank refit: once as it was and once where a block
that is mostly skin keeps a dark feature (much darker than the skin, at
least FEATURE_SHARE of the block) and the speck pass leaves it alone. The
two exports differ only at those features. Each such pixel is copied into
the approved frame where the approved pixel is skin, in the nearest colour
its 16x16 tile already uses, so the tile palettes stay as they are.

    python3 games/shadowleague/tools/painted_import/face_detail.py ID [ID ...] [--write] [--preview DIR]

Without --write it only reports. Fighters whose sheets do not reproduce
the approved silhouettes (overlap under MIN_IOU) are skipped.
"""
import argparse
import colorsys
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

TOOLS = Path(__file__).resolve().parent
GAME = TOOLS.parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(TOOLS))
sys.path.insert(0, str(TOOLS.parent))
import sprites as sp  # noqa: E402
import import_fighter as imp  # noqa: E402

FIGHTERS = GAME / 'assets/authored/fighters'
SOURCES = GAME / 'assets/authored/sdk177/sources'
FEATURE_SHARE = 0.18     # of a block's opaque source pixels
FEATURE_DARKER = 26.0    # L* below the block's skin
HEAD_BAND = 0.2          # the top of the silhouette that holds the face
LYING = ('down', 'fall', 'getup', 'roll')   # no face up top
MIN_IOU = 0.95


def is_skin(rgb):
    r, g, b = (np.asarray(rgb, float) / 255).tolist()
    h, l, s = colorsys.rgb_to_hls(r, g, b)
    return 0.0 <= h <= 0.12 and s > 0.18 and 0.22 < l < 0.92 and r > b


def source_dir(fid):
    """The folder holding ID_s1..s7 (the approved import's sheets)."""
    for d in (SOURCES / f'{fid}-corrected', SOURCES / fid, SOURCES, Path.home() / '.cache/slf-art/gen',
              SOURCES / 'earlier-art'):
        if all((d / f'{fid}_s{k}.png').exists() for k in range(1, 8)):
            return d
    return None


def native_features(rgb, mask, scale, pal):
    """sprites.native, keeping dark features inside skin blocks."""
    mask = sp.clean_fringe(rgb, mask)
    h, w = mask.shape
    H, W = max(1, round(h * scale)), max(1, round(w * scale))
    ys = np.linspace(0, h, H + 1).astype(int)
    xs = np.linspace(0, w, W + 1).astype(int)
    pl = sp.to_lab(pal)
    skin_label = np.array([False] + [is_skin(c) for c in pal])
    idx_full = sp.label(rgb, mask, pal)
    lum = sp.to_lab(rgb)[..., 0]
    base = sp.native(rgb, mask, scale, pal)
    keep = np.zeros((H, W), bool)
    out = base.copy()
    for j in range(H):
        a, b = ys[j], max(ys[j + 1], ys[j] + 1)
        for i in range(W):
            if not out[j, i] or not skin_label[out[j, i]]:
                continue
            c, e = xs[i], max(xs[i + 1], xs[i] + 1)
            sub_m = mask[a:b, c:e]
            sub_idx = idx_full[a:b, c:e][sub_m]
            if not len(sub_idx):
                continue
            skin = skin_label[sub_idx]
            if skin.sum() * 2 < len(sub_idx):
                continue
            sub_l = lum[a:b, c:e][sub_m]
            dark = sub_l < np.median(sub_l[skin]) - FEATURE_DARKER
            if dark.sum() < FEATURE_SHARE * len(sub_idx):
                continue
            mean_rgb = rgb[a:b, c:e][sub_m][dark].mean(0).reshape(1, 1, 3)
            out[j, i] = int(((pl - sp.to_lab(mean_rgb)[0, 0]) ** 2).sum(-1).argmin()) + 1
            keep[j, i] = True
    return out, base


def import_pair(fid, src, factor=None):
    """Plain and feature-keeping exports (48-colour, no bank refit): {name: (plain, featured)}, palette.
    factor: the approved import's uniform scale (its manifest's source_fit), else worked out again."""
    imp.SOURCE = src
    sheets = imp.load_sheets(fid)
    if len(sheets) != 7:
        raise ValueError(f'{fid}: sheets incomplete in {src}')
    c = imp.R[fid]
    crops = [(fig[1], fig[2]) for figs, _ in sheets.values() for fig in figs]
    pal = sp.palette(crops, 48)
    target = imp.stance_px(c)
    fixed = factor is not None
    for figs, names in ([] if fixed else sheets.values()):
        scale = target / (figs[0][0][3] - figs[0][0][1])
        for (box, rgb, mask), name in zip(figs, names):
            if name == imp.KEY:
                continue
            n = sp.outline(sp.native(rgb, mask, scale, pal), pal)
            yy, xx = np.nonzero(n)
            n = n[yy.min():yy.max() + 1, xx.min():xx.max() + 1]
            factor = min(factor or 1.0, 170 / max(1, n.shape[0]), 188 / max(1, n.shape[1]))
    if factor is None:
        factor = 1.0
    elif factor < 1.0 and not fixed:
        factor *= .98
    out = {}
    for figs, names in sheets.values():
        scale = factor * target / (figs[0][0][3] - figs[0][0][1])
        for (box, rgb, mask), name in zip(figs, names):
            if name == imp.KEY:
                continue
            featured, plain = native_features(rgb, mask, scale, pal)
            # the featured figure takes the plain one's ink ring and centre,
            # so the two differ only at the features
            outlined = sp.outline(plain, pal)
            ring = (outlined == 1) & (plain == 0)
            featured = featured.copy()
            featured[ring] = 1
            ys, xs = np.nonzero(outlined)
            crop = outlined[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
            centre = sp.body_centre(crop) + xs.min()
            out[name] = (sp.place(outlined, centre=centre)[0], sp.place(featured, centre=centre)[0])
    return out, pal


def best_shift(a, b):
    best = (0.0, 0, 0)
    for dy in range(-3, 4):
        for dx in range(-3, 4):
            bb = np.roll(np.roll(b, dy, 0), dx, 1)
            iou = (a & bb).sum() / max(1, (a | bb).sum())
            if iou > best[0]:
                best = (iou, dy, dx)
    return best


def detail(fid, write=False, preview=None):
    src = source_dir(fid)
    if src is None:
        return f'{fid}: no source sheets'
    manifest = json.loads((FIGHTERS / fid / 'manifest.json').read_text())
    pairs, pal = import_pair(fid, src, manifest.get('source_fit', {}).get('uniform_factor'))
    full = np.vstack([[0, 0, 0], pal]).astype(int)
    changed, painted, ious, shots = 0, 0, [], []
    new_frames = {}
    for name, fr in manifest['frames'].items():
        if name not in pairs or name in LYING:
            continue
        im = Image.open(FIGHTERS / fid / fr['file'])
        a = np.array(im)
        apal = np.array(im.getpalette()).reshape(-1, 3).astype(int)
        plain, featured = pairs[name]
        iou, dy, dx = best_shift(a > 0, plain > 0)
        ious.append(iou)
        if iou < MIN_IOU:
            continue
        plain = np.roll(np.roll(plain, dy, 0), dx, 1)
        featured = np.roll(np.roll(featured, dy, 0), dx, 1)
        feat = (featured != plain) & (featured > 0) & (a > 0)
        ys = np.nonzero(a)[0]
        feat[int(ys.min() + (ys.max() - ys.min()) * HEAD_BAND):] = False
        out = a.copy()
        for y, x in zip(*np.nonzero(feat)):
            if not is_skin(apal[a[y, x]]):
                continue
            ty, tx = y // 16 * 16, x // 16 * 16
            used = [int(v) for v in np.unique(a[ty:ty + 16, tx:tx + 16]) if v]
            want = full[featured[y, x]]
            darker = [v for v in used if sp.to_lab(apal[v].reshape(1, 1, 3).astype(np.uint8))[0, 0, 0]
                      < sp.to_lab(apal[a[y, x]].reshape(1, 1, 3).astype(np.uint8))[0, 0, 0] - 15]
            if not darker:
                continue
            out[y, x] = min(darker, key=lambda v: ((apal[v] - want) ** 2).sum())
            painted += 1
        if (out != a).any():
            changed += 1
            new_frames[fr['file']] = (out, im)
            if name in ('stance', 'hp1', 'walk3', 'win'):
                shots.append((name, a, out, apal))
    if write:
        for file, (out, im) in new_frames.items():
            img = Image.fromarray(out.astype(np.uint8), 'P')
            img.putpalette(im.getpalette())
            img.save(FIGHTERS / fid / file, transparency=0)
    if preview and shots:
        tiles = []
        for name, a, out, apal in shots:
            for arr in (a, out):
                rgb = apal[arr].astype(np.uint8)
                rgb[arr == 0] = (60, 75, 95)
                ys, xs = np.nonzero(arr)
                top = ys.min()
                band = arr[top:top + 30] > 0
                cols = np.nonzero(band.any(0))[0]
                crop = rgb[max(0, top - 2):top + 30, max(0, cols.min() - 2):cols.max() + 3]
                tiles.append(Image.fromarray(crop).resize((crop.shape[1] * 5, crop.shape[0] * 5), Image.NEAREST))
        w = sum(t.width for t in tiles) + 6 * len(tiles)
        sheet = Image.new('RGB', (w, max(t.height for t in tiles)), (0, 0, 0))
        x = 0
        for t in tiles:
            sheet.paste(t, (x, 0))
            x += t.width + 6
        Path(preview).mkdir(parents=True, exist_ok=True)
        sheet.save(Path(preview) / f'{fid}.png')
    return (f'{fid}: sheets {src.relative_to(GAME) if src.is_relative_to(GAME) else src}, '
            f'silhouette overlap {min(ious):.3f}-{max(ious):.3f}, {painted} pixels in {changed} frames'
            + (' written' if write and changed else ''))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('ids', nargs='+')
    ap.add_argument('--write', action='store_true')
    ap.add_argument('--preview', type=Path)
    a = ap.parse_args()
    for fid in a.ids:
        print(detail(fid, a.write, a.preview), flush=True)


if __name__ == '__main__':
    main()
