"""Painted sheets -> Shadow League fighter folder (indexed frames + manifest).

    python3 import_fighter.py ID [--out WORKTREE] [--preview DIR]

Requires all seven supplied pose sheets, with their exact figure counts. Builds
four tile palette banks, places whole figures on a 192x192 canvas with per-pose
roots, and rejects opaque cropping. No missing-pose fallback is used.
"""
import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

TOOLS = Path(__file__).resolve().parent
SOURCE = TOOLS.parents[1] / 'assets/authored/sdk177/sources'
sys.path.insert(0, str(TOOLS))
import sprites as sp  # noqa: E402
from roster import R  # noqa: E402

KEY = '_key'
# per sheet: (rows, pose names in reading order)
SHEETS = {
    's1': (2, ['stance', 'idle2', 'idle3', 'walk1', 'walk2', 'walk3', 'walk4', 'walk5', 'walk6', 'crouch']),
    's2': (2, [KEY, 'jump_up', 'jump_apex', 'jump_down', 'dash', 'backdash', 'guard', 'guard_low', 'hit', 'hit_low']),
    's3': (2, [KEY, 'fall', 'down', 'getup', 'dizzy', 'dizzy_b', 'intro', 'win', 'win_b', 'lose']),
    's4': (2, [KEY, 'lp1', 'lp2', 'lk2', 'hp1', 'hp2', 'hk1', 'hk2']),
    's5': (2, [KEY, 'cr_lp', 'cr_lk', 'cr_hp1', 'cr_hp2', 'cr_hk', 'j_p', 'j_k']),
    's6': (2, [KEY, 'sp1_1', 'sp1_2', 'sp2_1', 'sp2_2', 'sp3_1', 'sp3_2', 'gattack']),
    's7': (2, [KEY, 'super_1', 'super_2', 'super_3', 'throw_1', 'throw_2', 'roll', 'hit_b']),
}
AIR = {'jump_up', 'jump_apex', 'jump_down', 'backdash', 'fall', 'j_p', 'j_k', 'hit_b_air'}
# where an active pose's striking limb points
STRIKE = {'cr_hp2': 'up', 'sp2_2': 'up', 'cr_lk': 'low', 'cr_hk': 'low', 'j_k': 'down', 'j_p': 'down'}
LIMBS = ('far_fist', 'near_fist', 'far_ankle', 'near_ankle')


def stance_px(c):
    return int(round(c['cm'] * 0.62))


def strike_box(idx, kind, root):
    """Hit rectangle around the extreme limb in engine coordinates."""
    ys, xs = np.nonzero(idx)
    if kind == 'up':
        y0 = ys.min()
        sel = ys < y0 + 18
        bx, by = xs[sel], ys[sel]
    else:
        if kind == 'low':
            band = ys > ys.max() - 40
        elif kind == 'down':
            band = ys > ys.min() + (ys.max() - ys.min()) * 0.35
        else:
            band = ys < ys.max() - 25
        if not band.any():
            band = ys >= 0
        xr = xs[band].max()
        sel = band & (xs > xr - 16)
        bx, by = xs[sel], ys[sel]
    x0, x1, y0, y1 = bx.min(), bx.max() + 1, by.min(), by.max() + 1
    if y1 - y0 > 24:                          # keep it to the fist/foot itself
        mid = int(np.median(by)); y0, y1 = max(y0, mid - 12), min(y1, mid + 12)
    ax, ay = root
    bottom=int(ay + 1 - y1);height=int(y1-y0)
    if bottom>127:height+=bottom-127;bottom=127
    x=int(x0-ax);w=int(x1-x0)
    if x < -128:w-=(-128-x);x=-128
    if x>127:w+=x-127;x=127
    return [x,bottom,w,height]


def hurt_boxes(idx, root):
    ys, xs = np.nonzero(idx)
    top, bot = ys.min(), ys.max()
    h = bot - top + 1
    cuts = [top, top + int(h * 0.26), top + int(h * 0.60), bot + 1]
    boxes = []
    ax, ay = root
    for a, b in zip(cuts[:-1], cuts[1:]):
        band = (ys >= a) & (ys < b)
        if not band.any():
            continue
        bx = xs[band]
        x0, x1 = np.percentile(bx, 4), np.percentile(bx, 96) + 1
        boxes.append([int(x0 - ax), int(ay + 1 - b), max(1, int(x1 - x0)), int(b - a)])
    boxes.reverse()                           # feet first, like the original exports
    for bx in boxes:
        bx[0] = max(-128, min(127, bx[0])); bx[2] = max(1, min(255, bx[2])); bx[3] = max(1, min(255, bx[3]))
        bx[1] = max(-128, min(127, bx[1]))
    return boxes


def load_sheets(id):
    found = {}
    for k, (rows, names) in SHEETS.items():
        p = SOURCE / f'{id}_{k}.png'
        if not p.exists():
            continue
        rgb = sp.load(p)
        figs, nrows = sp.figures(rgb, rows)
        if len(figs) != len(names):
            print(f'{id}_{k}: expected {len(names)} figures, found {len(figs)}; skipped')
            continue
        found[k] = (figs, names)
    return found


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('id')
    ap.add_argument('--out', default=str(TOOLS.parents[3]))
    ap.add_argument('--sources', type=Path, required=True)
    ap.add_argument('--folder', type=Path, required=True)
    ap.add_argument('--preview', default=None)
    ap.add_argument('--colours', type=int, default=48)
    ap.add_argument('--banks', type=int, default=4, help='hardware palettes, one chosen per 16x16 tile')
    a = ap.parse_args()
    global SOURCE
    SOURCE = a.sources.resolve()
    if a.folder.exists():raise SystemExit("Refusing to replace an existing export folder")
    c = R[a.id]
    sheets = load_sheets(a.id)
    if len(sheets) != 7:raise SystemExit(a.id + ': all seven complete source sheets are required')
    crops = [(fig[1], fig[2]) for figs, _ in sheets.values() for fig in figs]
    pal = sp.palette(crops, a.colours)
    target = stance_px(c)
    frames, meta, order, roots = {}, {}, [], {}
    factor = 1.0
    prevented = []
    for k, (figs, names) in sheets.items():
        scale = target / (figs[0][0][3] - figs[0][0][1])
        for (box, rgb, mask), name in zip(figs, names):
            if name == KEY:continue
            native = sp.outline(sp.native(rgb, mask, scale, pal), pal)
            yy,xx=np.nonzero(native);native=native[yy.min():yy.max()+1,xx.min():xx.max()+1]
            center=sp.body_centre(native)
            if native.shape[0]>140 or center>78 or native.shape[1]-center>78:prevented.append(name)
            factor=min(factor,170/max(1,native.shape[0]),188/max(1,native.shape[1]))
    if factor<1.0:factor*=.98

    for k, (figs, names) in sheets.items():
        key = figs[0]
        scale = factor * target / (key[0][3] - key[0][1])
        for (box, rgb, mask), name in zip(figs, names):
            if name == KEY:
                continue
            idx = sp.native(rgb, mask, scale, pal)
            idx = sp.outline(idx, pal)
            canvas, root = sp.place(idx)
            roots[name] = root
            lab, n = ndi.label(canvas > 0, np.ones((3, 3), bool))
            if n > 1:
                # specks the reduction or the canvas edge cut off from the figure
                sizes = ndi.sum(canvas > 0, lab, range(1, n + 1))
                for k, size in enumerate(sizes, 1):
                    if size < 3:
                        canvas[lab == k] = 0
            if name not in AIR:
                # a limb cropped at the canvas edge may have held the lowest pixel
                drop = sp.ROOT[1] - np.nonzero(canvas)[0].max()
                if drop > 0:
                    canvas = np.roll(canvas, drop, axis=0)
            frames[name] = canvas
            order.append(name)
    if a.banks:
        frames, pal = tile_banks(frames, pal, a.banks, a.out)
    folder = a.folder.resolve()
    folder.mkdir(parents=True, exist_ok=False)
    manifest = {'canvas': [sp.CANVAS, sp.CANVAS], 'anchor': list(sp.ROOT), 'head': 'head.png', 'portrait_small': 'portrait-small.png',
                'source': 'Native indexed PNG frames; four 15-colour palettes, one chosen per 16x16 tile', 'frames': {}}
    for name in order:
        img = frames[name]
        sp.save_indexed(img, pal, folder / f'{name}.png')
        kind = STRIKE.get(name, 'fwd')
        box = strike_box(img, kind, roots[name])
        grounded = name not in AIR
        manifest['frames'][name] = {
            'file': f'{name}.png', 'root': roots[name], 'hurt': hurt_boxes(img, roots[name]),
            'hit': {limb: [box] for limb in LIMBS},
            'contacts': ['far_leg', 'near_leg'] if grounded else [],
        }
    # 32x32 head crops from the guard stance (select icons until the painted portraits arrive)
    st = frames['stance']
    ys, xs = np.nonzero(st)
    top = ys.min()
    head_x = int(np.median(xs[ys < top + 14]))
    y0 = max(1, top - 2); x0 = max(1, min(sp.CANVAS - 33, head_x - 16))
    crop = st[y0:y0 + 32, x0:x0 + 32].copy()
    if crop.max() > 15:
        # the select screen loads the first palette only: its 15 colours
        full = np.vstack([[0, 0, 0], pal]).astype(int)
        first = sp.to_lab(full[1:16].astype(np.uint8))
        on = crop > 0
        lab = sp.to_lab(full[crop[on]].astype(np.uint8))
        crop[on] = ((lab[:, None] - first[None]) ** 2).sum(-1).argmin(1) + 1
    for fname in ('head.png', 'portrait-small.png'):
        sp.save_indexed(crop, pal, folder / fname)
    # player 2 colours: same ramps, clothes hue-rotated (skin and ink kept)
    manifest['alt_palette'] = [[0, 0, 0]] + alternate(pal).tolist()
    manifest['source_fit']={'uniform_factor':factor,'poses_previously_exceeding_160_canvas':prevented,'opaque_cropping':False}
    (folder / 'manifest.json').write_text(json.dumps(manifest, indent=1) + '\n')
    sys.path.insert(0,str(Path(a.out)/'games/shadowleague/tools'))
    from native_art_io import fighter_source
    fighter_source(folder,order)
    if a.preview:
        names = order
        sp.preview([frames[n] for n in names], pal, Path(a.preview) / f'{a.id}-frames.png', scale=2, cols=10)
    print(a.id, len(order), 'frames', 'palette', pal.tolist())


def tile_banks(frames, pal, banks, worktree):
    """Refit the drawings to BANKS palettes of 15, one chosen per 16x16 tile
    (the way the painted stages and title are packed), instead of one
    15-colour palette for the whole fighter. Returns frames indexed into the
    banks laid end to end: colour b*15+i is entry i of palette b."""
    sys.path.insert(0, str(Path(worktree) / 'artbox'))
    from palette_banks import quantize
    names = list(dict.fromkeys(frames))
    full = np.vstack([[0, 0, 0], pal]).astype(np.uint8)
    stack = np.vstack([frames[n] for n in names])
    rgba = np.zeros(stack.shape + (4,), np.uint8)
    rgba[..., :3] = full[stack]
    rgba[..., 3] = np.where(stack > 0, 255, 0)
    idx, fitted, assign = quantize(rgba, bank_limit=banks)
    bank_of = assign.repeat(16, 0).repeat(16, 1).astype(np.int32)
    glob = np.where(idx > 0, bank_of * 15 + idx, 0).astype(np.uint8)
    gpal = sp.snap(np.vstack([b[1:] for b in fitted]))
    out = {n: glob[i * sp.CANVAS:(i + 1) * sp.CANVAS] for i, n in enumerate(names)}
    return {n: out[n] for n in frames}, gpal


def alternate(pal):
    import colorsys
    out = []
    for i, c in enumerate(pal):
        r, g, b = (c / 255.0).tolist()
        h, l, s = colorsys.rgb_to_hls(r, g, b)
        skin = 0.02 < h < 0.12 and s > 0.25 and l > 0.25
        if i == 0 or s < 0.12 or skin:
            out.append(c)
        else:
            r, g, b = colorsys.hls_to_rgb((h + 0.45) % 1.0, l, min(1.0, s * 1.05))
            out.append(np.array([r, g, b]) * 255)
    return sp.snap(out)


if __name__ == '__main__':
    main()
