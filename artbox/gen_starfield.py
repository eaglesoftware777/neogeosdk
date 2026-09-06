#!/usr/bin/env python3
"""
Generate a tiling deep-space backdrop for a vertical shooter.

Everything here is synthesised, so the result carries no licence and the
seam is exact rather than hopeful: the noise lattice wraps, so the page
joins itself top to bottom and left to right and a scrolling background
built from two copies of it shows no line at the wrap.

    python3 artbox/gen_starfield.py OUT.png [OUT2.png ...]
                                    [--size 256] [--seed 7]
                                    [--nebula 1.0] [--stars 1.0]

Give more than one output and the page is generated at the full combined
width and cut into columns, so backdrops laid side by side join without a
line down the middle.

The palette is deliberately dark and blue-biased.  A shooter's backdrop
has to lose every contrast fight it picks with the sprites in front of
it, so the nebula tops out well below the brightness of the ships and
the bright stars are few enough to read as landmarks rather than noise.
"""

import argparse
import numpy as np
from PIL import Image


def _axis(period, size):
    """Interpolation indices and weights for one tiling axis."""
    t = np.linspace(0.0, period, size, endpoint=False, dtype=np.float32)
    i0 = np.floor(t).astype(int) % period
    i1 = (i0 + 1) % period
    f = t - np.floor(t)
    return i0, i1, f * f * (3.0 - 2.0 * f)           # smoothstep


def _lattice(rng, py, px, h, w):
    """Random values on a periodic lattice, bilinearly interpolated.

    The two axes carry their own period, so a page can tile at one length
    vertically and another horizontally - which is what a backdrop that
    scrolls in one axis and is laid side by side in the other needs.
    Indexing wraps with `% period`, and that is what makes it tile: the
    cell east of the last one is the first one.
    """
    grid = rng.random((py, px)).astype(np.float32)
    y0, y1, fy = _axis(py, h)
    x0, x1, fx = _axis(px, w)

    row0 = grid[y0][:, x0] * (1 - fx)[None, :] + grid[y0][:, x1] * fx[None, :]
    row1 = grid[y1][:, x0] * (1 - fx)[None, :] + grid[y1][:, x1] * fx[None, :]
    return row0 * (1 - fy)[:, None] + row1 * fy[:, None]


def _fbm(rng, h, w, octaves=5, base=2, gain=0.5):
    """Fractal sum of tiling lattice noise."""
    out = np.zeros((h, w), dtype=np.float32)
    amp, total, py, px = 1.0, 0.0, base, max(1, int(base * w / h))
    for _ in range(octaves):
        if py > h or px > w:
            break
        out += amp * _lattice(rng, py, px, h, w)
        total += amp
        amp *= gain
        py *= 2
        px *= 2
    return out / max(total, 1e-6)


def build(height=256, width=256, seed=7, nebula=1.0, stars=1.0):
    rng = np.random.default_rng(seed)
    img = np.zeros((height, width, 3), dtype=np.float32)
    area = (height * width) / (256.0 * 256.0)

    # Base: not black.  A flat black page reads as a hole rather than as
    # distance, and it gives the dithering nothing to work with.
    img[..., 2] += 0.055
    img[..., 0] += 0.012

    # Nebula: two clouds in different hues, each masked by its own noise
    # so they overlap in places instead of sharing one silhouette.
    if nebula > 0.0:
        warp = _fbm(rng, height, width, octaves=4) - 0.5
        for tint, power, weight in (((0.24, 0.10, 0.62), 2.6, 0.85),
                                    ((0.06, 0.34, 0.55), 3.2, 0.55),
                                    ((0.55, 0.14, 0.30), 3.8, 0.35)):
            f = _fbm(rng, height, width, octaves=5)
            f = np.clip(f + 0.35 * warp, 0.0, 1.0) ** power
            img += np.asarray(tint, dtype=np.float32) * (f * weight * nebula)[..., None]

    # Dust lanes: subtract a thin high-frequency band so the clouds get
    # structure instead of reading as one soft blob.
    lanes = _fbm(rng, height, width, octaves=6, base=4) ** 4.0
    img -= lanes[..., None] * 0.16 * nebula
    np.clip(img, 0.0, 1.0, out=img)

    # Stars, faintest layer first so the bright ones land on top.
    def scatter(count, lo, hi, tint):
        ys = rng.integers(0, height, count)
        xs = rng.integers(0, width, count)
        mag = rng.random(count).astype(np.float32) * (hi - lo) + lo
        for y, x, m in zip(ys, xs, mag):
            img[y, x] = np.maximum(img[y, x], np.asarray(tint) * m)

    n = lambda k: max(1, int(k * stars * area))
    scatter(n(900), 0.10, 0.32, (0.62, 0.68, 0.85))   # dust
    scatter(n(260), 0.34, 0.62, (0.80, 0.84, 0.95))   # mid
    scatter(n(70),  0.66, 0.92, (1.00, 0.98, 0.92))   # near

    # A handful of anchors with a one-pixel glint.  Wrapping the offsets
    # keeps them intact across the seam.
    for _ in range(n(14)):
        y = int(rng.integers(0, height)); x = int(rng.integers(0, width))
        hue = np.asarray((1.0, 0.96, 0.88) if rng.random() < 0.7
                         else (0.72, 0.84, 1.00), dtype=np.float32)
        img[y, x] = hue
        for dy, dx, f in ((0, 1, .45), (0, -1, .45), (1, 0, .45), (-1, 0, .45),
                          (0, 2, .18), (0, -2, .18), (2, 0, .18), (-2, 0, .18)):
            yy, xx = (y + dy) % height, (x + dx) % width
            img[yy, xx] = np.maximum(img[yy, xx], hue * f)

    np.clip(img, 0.0, 1.0, out=img)
    return Image.fromarray((img * 255.0 + 0.5).astype(np.uint8), mode="RGB")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("out", nargs="+",
                    help="Output PNG(s).  With more than one, the page is "
                         "generated at the full combined width and cut into "
                         "equal columns, so the joins between them are "
                         "continuous rather than merely similar.")
    ap.add_argument("--size", type=int, default=256,
                    help="Height, and the width of each output column.")
    ap.add_argument("--seed", type=int, default=7)
    ap.add_argument("--nebula", type=float, default=1.0)
    ap.add_argument("--stars", type=float, default=1.0)
    args = ap.parse_args()

    n = len(args.out)
    page = build(args.size, args.size * n, args.seed, args.nebula, args.stars)
    for i, path in enumerate(args.out):
        page.crop((i * args.size, 0, (i + 1) * args.size, args.size)).save(path)
        print(f"wrote {path} ({args.size}x{args.size}, seed {args.seed}, "
              f"column {i + 1}/{n})")


if __name__ == "__main__":
    main()
