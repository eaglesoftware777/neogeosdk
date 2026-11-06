#!/usr/bin/env python3
"""
Painted townsfolk sheets -> artbox/painted/folk_<name>.png, which tools/art.py
draws instead of the stand-in figures.

    python3 games/maiyaluna/tools/folk_import.py SOURCE_DIR [NAME ...]

SOURCE_DIR holds ml_f_<name>.png: three poses of one person (standing,
swaying, talking) side by side on flat magenta, drawn as enlarged pixel art in
the first game's villager style. Each pose is cut out, its colours fitted to
fifteen shared by all three, reduced to HEIGHT pixels (an animal to the
frame's width) by majority of the source pixels, and set on a 32 x 48 frame
with the feet two pixels above its bottom, centred. Written fresh: nothing
from the source file is kept.
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

GAME = Path(__file__).resolve().parents[1]
OUT = GAME / "artbox/painted"
FRAME_W, FRAME_H = 32, 48
HEIGHT = 44
ANIMALS = {"piko"}
NAMES = ["nari", "tovan", "mina", "orin", "valen", "piko", "marra", "pell", "dara", "sefa", "brack", "worker"]


def magenta(rgb):
    r, g, b = (rgb[..., i].astype(int) for i in range(3))
    return (r > 110) & (b > 110) & (g < 130) & (r - g > 50) & (b - g > 50)


def figures(rgb, n=3):
    """The n poses, left to right: boxes split by empty columns."""
    fg = ndi.binary_opening(~magenta(rgb), iterations=2)
    cols = fg.sum(0) > 2
    runs, x = [], 0
    while x < len(cols):
        if not cols[x]:
            x += 1
            continue
        s, gap = x, 0
        while x < len(cols) and (cols[x] or gap < 8):
            gap = 0 if cols[x] else gap + 1
            x += 1
        if x - gap - s > 20:
            runs.append((s, x - gap))
    if len(runs) != n:
        raise SystemExit(f"folk_import: found {len(runs)} figures, wanted {n}")
    boxes = []
    for x0, x1 in runs:
        ys = np.nonzero(fg[:, x0:x1].any(1))[0]
        boxes.append((x0, ys[0], x1, ys[-1] + 1))
    return boxes


def palette(rgb, boxes, k=15):
    px = []
    for x0, y0, x1, y1 in boxes:
        sub = rgb[y0:y1, x0:x1]
        inner = ndi.binary_erosion(~magenta(sub), iterations=3)
        px.append(sub[inner].reshape(-1, 3))
    px = np.concatenate(px).astype(float)
    rng = np.random.default_rng(0)
    px = px[rng.choice(len(px), min(60000, len(px)), replace=False)]
    c = px[rng.choice(len(px), k, replace=False)]
    for _ in range(30):
        lab = ((px[:, None, :] - c[None]) ** 2).sum(2).argmin(1)
        for j in range(k):
            sel = px[lab == j]
            if len(sel):
                c[j] = sel.mean(0)
    return c[np.argsort(c.sum(1))]


def labels(sub, pal):
    flat = sub.reshape(-1, 3).astype(float)
    lab = ((flat[:, None, :] - pal[None]) ** 2).sum(2).argmin(1)
    dist = ((flat - pal[lab]) ** 2).sum(1)
    dmag = ((flat - np.array([255, 0, 255])) ** 2).sum(1)
    lab = lab + 1
    lab[magenta(sub).ravel() | (dmag < dist)] = 0
    return lab.reshape(sub.shape[:2])


def reduce(lab, h, w):
    """Majority of the source pixels in each target pixel (background wins at half)."""
    H, W = lab.shape
    out = np.zeros((h, w), np.uint8)
    k = int(lab.max()) + 1
    for j in range(h):
        a, b = j * H // h, max((j + 1) * H // h, j * H // h + 1)
        for i in range(w):
            c, e = i * W // w, max((i + 1) * W // w, i * W // w + 1)
            cnt = np.bincount(lab[a:b, c:e].ravel(), minlength=k)
            if cnt[0] > cnt.sum() * 0.5:
                continue
            cnt[0] = 0
            out[j, i] = cnt.argmax()
    return out


def clean(idx):
    """Drop specks of fewer than three pixels apart from the figure."""
    lab, n = ndi.label(idx > 0, structure=np.ones((3, 3)))
    if n > 1:
        sizes = ndi.sum(idx > 0, lab, range(1, n + 1))
        for i, s in enumerate(sizes, 1):
            if s < 3:
                idx[lab == i] = 0
    return idx


def frame_of(idx, pal):
    h, w = idx.shape
    out = np.zeros((FRAME_H, FRAME_W, 4), np.uint8)
    x0 = (FRAME_W - w) // 2
    y0 = FRAME_H - 2 - h
    rgba = np.zeros((h, w, 4), np.uint8)
    on = idx > 0
    rgba[on, :3] = np.clip(np.rint(pal[idx[on] - 1]), 0, 255).astype(np.uint8)
    rgba[on, 3] = 255
    out[y0:y0 + h, x0:x0 + w] = rgba
    return out


def import_one(src, name):
    rgb = np.asarray(Image.open(src).convert("RGB"))
    boxes = figures(rgb)
    pal = palette(rgb, boxes)
    frames = []
    # one scale for all three poses: the tallest (or, for an animal, the widest) fits
    tallest = max(y1 - y0 for x0, y0, x1, y1 in boxes)
    widest = max(x1 - x0 for x0, y0, x1, y1 in boxes)
    scale = min(HEIGHT / tallest, (FRAME_W - 2) / widest) if name not in ANIMALS else (FRAME_W - 2) / widest
    for x0, y0, x1, y1 in boxes:
        lab = labels(rgb[y0:y1, x0:x1], pal)
        h, w = max(1, round((y1 - y0) * scale)), max(1, round((x1 - x0) * scale))
        frames.append(frame_of(clean(reduce(lab, h, w)), pal))
    sheet = np.concatenate(frames, axis=1)
    OUT.mkdir(parents=True, exist_ok=True)
    Image.fromarray(sheet, "RGBA").save(OUT / f"folk_{name}.png")
    print(f"folk_import: {name} <- {src.name} ({len(pal)} colours)")


def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    src = Path(sys.argv[1])
    for name in sys.argv[2:] or NAMES:
        p = src / f"ml_f_{name}.png"
        if p.is_file():
            import_one(p, name)
        else:
            print(f"folk_import: no {p.name}")


if __name__ == "__main__":
    main()
