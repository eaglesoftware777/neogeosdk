#!/usr/bin/env python3
"""
Painted pose sheets -> fighter frames for tools/art.py.

    python3 games/nosignal/tools/import_sheets.py SOURCE_DIR [NAME ...]

SOURCE_DIR holds ns_<name>_s1.png, ns_<name>_s2.png ... : figures on flat
magenta, two rows of five, the first figure of every sheet the standing
pose. Each figure is cut out, scaled so that standing pose is the fighter's
height in pixels, and set on a square frame (128 pixels, bigger for the
giant bosses: FRAME_PX) with the feet four pixels above its bottom and the
body's middle on its centre line (an airborne figure keeps its height above
the row's floor). Frames go to artbox/painted/fighters/<name>/NN.png,
written fresh (no metadata from the source), numbered as ns.h's HF_ / EF_ /
BF_ frames: sheet k's figure i is frame 10 * (k - 1) + i.
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

GAME = Path(__file__).resolve().parents[1]
OUT = GAME / "artbox/painted/fighters"
HEIGHT = {"jay": 96, "zoe": 92, "kai": 98, "thief": 92, "biker": 104, "skater": 92, "rico": 100,
          "guard": 98, "drone": 44, "runner": 90, "bloater": 108, "android": 100, "robodog": 52, "walker": 104,
          "alien": 106, "bigflex": 120, "zerox": 106, "m08": 146, "behemoth": 142, "proxy": 106, "overmind": 176,
          "archivist": 114}
SHEETS = {"jay": 3, "zoe": 3, "kai": 3, "thief": 1, "biker": 1, "skater": 1, "rico": 2,
          "guard": 1, "drone": 1, "runner": 1, "bloater": 1, "android": 1, "robodog": 1, "walker": 1, "alien": 1,
          "bigflex": 2, "zerox": 2, "m08": 2, "behemoth": 2, "proxy": 2, "overmind": 1, "archivist": 3}
FRAME_PX = {"bigflex": 160, "m08": 160, "behemoth": 160, "overmind": 192, "archivist": 160}   # all fit the screen


def magenta(rgb):
    r, g, b = (rgb[..., i].astype(int) for i in range(3))
    return (r > 110) & (b > 110) & (g < 140) & (r - g > 45) & (b - g > 45)


def trim(sl, mask):
    """The slice and mask cut down to the mask's own bounding box."""
    ys, xs = np.nonzero(mask)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    return (slice(sl[0].start + y0, sl[0].start + y1), slice(sl[1].start + x0, sl[1].start + x1)), mask[y0:y1, x0:x1]


def cut_in_two(sl, mask):
    """Two figures from one blob, cut where the fewest pixels are in its middle half."""
    w = mask.shape[1]
    if w < 120:
        return None
    counts = mask.sum(axis=0)
    lo, hi = int(w * 0.3), int(w * 0.7)
    cut = lo + int(np.argmin(counts[lo:hi]))
    left, right = mask.copy(), mask.copy()
    left[:, cut:] = False
    right[:, :cut] = False
    if left.sum() < 1500 or right.sum() < 1500:
        return None
    return [trim(sl, left), trim(sl, right)]


def figures(rgb):
    fg = ~magenta(rgb)
    fg = ndi.binary_opening(fg, iterations=1)
    lab, n = ndi.label(ndi.binary_dilation(fg, iterations=9))
    boxes = []
    for i, sl in enumerate(ndi.find_objects(lab), 1):
        if sl is None:
            continue
        mask = (lab[sl] == i) & fg[sl]
        if mask.sum() < 1500:
            continue
        boxes.append((sl, mask))
    if not boxes:
        return []
    ys = np.array([(sl[0].start + sl[0].stop) / 2 for sl, _ in boxes])
    split = (ys.min() + ys.max()) / 2 if ys.max() - ys.min() > 80 else ys.max() + 1
    rows = [[b for b, y in zip(boxes, ys) if y < split], [b for b, y in zip(boxes, ys) if y >= split]]
    out = []
    for row in rows:
        if not row:
            continue
        while len(row) < 5:                  # two figures touching: cut the widest at its thinnest column
            k = max(range(len(row)), key=lambda j: row[j][1].shape[1])
            parts = cut_in_two(*row[k])
            if not parts:
                break
            row[k:k + 1] = parts
        row.sort(key=lambda b: b[0][1].start)
        floor = int(np.median([b[0][0].stop for b in row]))
        for sl, mask in row:
            out.append((sl, mask, floor))
    return out


def reduce(rgb, mask, s):
    h, w = mask.shape
    W, H = max(1, round(w * s)), max(1, round(h * s))
    pre = rgb * mask[..., None]
    col = np.asarray(Image.fromarray(pre.astype(np.uint8)).resize((W, H), Image.BOX)).astype(float)
    al = np.asarray(Image.fromarray((mask * 255).astype(np.uint8)).resize((W, H), Image.BOX)).astype(float)
    col = np.where(al[..., None] > 0, col / np.maximum(al[..., None] / 255, 1e-3), 0)
    a = al >= 120
    out = np.zeros((H, W, 4), np.uint8)
    out[..., :3] = np.clip(col, 0, 255)
    out[..., 3] = np.where(a, 255, 0)
    out[~a] = 0
    return out


def import_one(src, name):
    FRAME = FRAME_PX.get(name, 128)
    FEET, MIDDLE = FRAME - 4, FRAME // 2
    target = HEIGHT[name]
    dest = OUT / name
    dest.mkdir(parents=True, exist_ok=True)
    made = 0
    for k in range(1, SHEETS[name] + 1):
        f = src / f"ns_{name}_s{k}.png"
        if not f.is_file():
            print(f"import: no {f.name}")
            continue
        rgb = np.asarray(Image.open(f).convert("RGB")).astype(float)
        figs = figures(rgb)
        if len(figs) != 10:
            print(f"import: {f.name}: {len(figs)} figures (10 expected): using what is there")
        if not figs:
            continue
        key_sl, key_mask, key_floor = figs[0]
        s = target / (key_sl[0].stop - key_sl[0].start)
        for i, (sl, mask, floor) in enumerate(figs[:10]):
            crop = rgb[sl]
            small = reduce(crop, mask, s)
            # the body's middle: mean x of the figure between 35% and 60% of its height
            hh = mask.shape[0]
            band = mask[int(hh * 0.35):int(hh * 0.6)]
            cx = (np.nonzero(band)[1].mean() if band.any() else mask.shape[1] / 2) * s
            lift = max(0, floor - sl[0].stop) * s
            frame = np.zeros((FRAME, FRAME, 4), np.uint8)
            x0 = int(round(MIDDLE - cx))
            y0 = int(round(FEET - small.shape[0] - lift))
            for yy in range(small.shape[0]):
                ty = y0 + yy
                if not 0 <= ty < FRAME:
                    continue
                for xx in range(small.shape[1]):
                    tx = x0 + xx
                    if 0 <= tx < FRAME and small[yy, xx, 3]:
                        frame[ty, tx] = small[yy, xx]
            Image.fromarray(frame, "RGBA").save(dest / f"{10 * (k - 1) + i:02d}.png")
            made += 1
    print(f"import: {name}: {made} frames")


def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    src = Path(sys.argv[1])
    names = sys.argv[2:] or list(HEIGHT)
    for n in names:
        import_one(src, n)


if __name__ == "__main__":
    main()
