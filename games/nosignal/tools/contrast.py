#!/usr/bin/env python3
"""
Do the fighters stand out from the streets they fight on?

    python3 games/nosignal/tools/contrast.py            the table
    python3 games/nosignal/tools/contrast.py --check    exit 1 if a fighter melts into a stage

Reads the built art (artbox/generated: the colours the console shows) and
each stage's crew in ns_data.c (S<n>_ENEMIES; the heroes are on every stage).
Each fighter's stance is stood on the street every 60 pixels at the middle
of the walking band (y 173), and its outline (opaque pixels next to clear
ones) is compared with the street just outside it in CIE Lab:

  melt   the share of the outline within dE 10 of the street behind it
         (a hard-to-see edge); --check fails above MELT_MAX
  L      the fighter's lightness, 10th to 90th percentile, and the street's
         behind the fighters (rows 40-206), to compare value ranges
"""
import re
import sys
from pathlib import Path

import numpy as np
from PIL import Image

GAME = Path(__file__).resolve().parents[1]
GEN = GAME / "artbox/generated"
HEROES = ("jay", "zoe", "kai")
FEET_Y = 173            # the middle of the walking band (ns_data.c: y 140-206)
FRAME_FEET = 4          # a frame's feet are 4 pixels above its bottom (tools/import_sheets.py)
MELT_MAX = 0.15


def lab(rgb):
    c = rgb / 255.0
    c = np.where(c > 0.04045, ((c + 0.055) / 1.055) ** 2.4, c / 12.92)
    m = np.array([[0.4124, 0.3576, 0.1805], [0.2126, 0.7152, 0.0722], [0.0193, 0.1192, 0.9505]])
    xyz = c @ m.T / np.array([0.95047, 1.0, 1.08883])
    f = np.where(xyz > 0.008856, np.cbrt(xyz), 7.787 * xyz + 16 / 116)
    return np.stack([116 * f[..., 1] - 16, 500 * (f[..., 0] - f[..., 1]), 200 * (f[..., 1] - f[..., 2])], -1)


def crews():
    src = (GAME / "scenes/ns_data.c").read_text()
    out = {}
    for n, body in re.findall(r"S(\d)_ENEMIES\[\d+\] = \{([^}]*)\}", src):
        out[int(n)] = re.findall(r"&ns_(\w+)", body)
    return out


def stance(name):
    sheet = np.asarray(Image.open(GEN / f"f_{name}.png").convert("RGBA"))
    size = sheet.shape[0]
    return sheet[:, :size]


def measure(stage_rgb, fig):
    """melt share, fighter L p10/p90 over the placements"""
    size = fig.shape[0]
    opaque = fig[..., 3] > 0
    inner = opaque.copy()
    inner[1:-1, 1:-1] = opaque[1:-1, 1:-1] & opaque[:-2, 1:-1] & opaque[2:, 1:-1] & opaque[1:-1, :-2] & opaque[1:-1, 2:]
    edge = opaque & ~inner
    ys, xs = np.nonzero(edge)
    # the street pixel just outside each edge pixel: the first clear neighbour
    out_y, out_x = ys.copy(), xs.copy()
    for dy, dx in ((0, -1), (0, 1), (-1, 0), (1, 0)):
        ny, nx = np.clip(ys + dy, 0, size - 1), np.clip(xs + dx, 0, size - 1)
        clear = ~opaque[ny, nx]
        take = clear & (out_y == ys) & (out_x == xs)
        out_y[take], out_x[take] = ny[take], nx[take]
    fig_lab = lab(fig[..., :3].astype(float))
    top = FEET_Y - (size - FRAME_FEET)
    melts = []
    for x0 in range(0, stage_rgb.shape[1] - size, 60):
        sy, sx = top + out_y, x0 + out_x
        ok = (sy >= 0) & (sy < stage_rgb.shape[0])
        st = lab(stage_rgb[sy[ok], sx[ok]].astype(float))
        d = np.linalg.norm(fig_lab[ys[ok], xs[ok]] - st, axis=1)
        melts.append(float((d < 10).mean()))
    l = fig_lab[..., 0][opaque]
    return float(np.mean(melts)), float(np.max(melts)), np.percentile(l, 10), np.percentile(l, 90)


def main():
    check = "--check" in sys.argv[1:]
    bad = 0
    print("stage fighter        melt mean  worst   fighter L p10-p90   street L p10-p90")
    for n, crew in sorted(crews().items()):
        stage_rgb = np.asarray(Image.open(GEN / f"stage{n}.png").convert("RGB"))
        sl = lab(stage_rgb[40:207].astype(float))[..., 0]
        street = f"{np.percentile(sl, 10):3.0f}-{np.percentile(sl, 90):3.0f}"
        for name in HEROES + tuple(crew):
            mean, worst, lo, hi = measure(stage_rgb, stance(name))
            flag = ""
            if mean > MELT_MAX:
                flag = "   melts into the street"
                bad += 1
            print(f"{n:5d} {name:12s} {mean:9.2f} {worst:6.2f}   {lo:7.0f}-{hi:3.0f}          {street}{flag}")
    print(f"contrast: {bad} fighter(s) melt into a stage" if bad else "contrast: every fighter stands out on its stages")
    return 1 if check and bad else 0


if __name__ == "__main__":
    sys.exit(main())
