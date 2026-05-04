#!/usr/bin/env python3
"""
img2neo.py — Convert any image to a NeoGeo-ready 15-color indexed PNG.

Pipeline:
  1. Load source image (JPG, PNG, BMP, etc.)
  2. Center-crop and resize to target dimensions (multiple of 16)
  3. Pre-quantise every channel to the NeoGeo 5-bit colour space (steps of 8)
  4. Optimise 15-colour palette with k-means++ clustering
  5. Apply Floyd-Steinberg dithering for maximum perceived quality
  6. Save indexed PNG in the format expected by romdbimgimport.py

Usage:
  python3 img2neo.py src.jpg dst.png [-W 256] [-H 256] [-c 15]
  python3 img2neo.py --batch            (converts all docs images to artbox/in/)
"""

import argparse, os, sys
import numpy as np
import png
from PIL import Image, ImageFilter

DOCS_IMGS = os.path.join(os.path.dirname(__file__), '..', '..', 'docs', 'img')
IN_DIR    = os.path.join(os.path.dirname(__file__), 'in')


# ---------------------------------------------------------------------------
# Colour utilities
# ---------------------------------------------------------------------------

def snap_neogeo(arr):
    """Round each 8-bit channel to the nearest NeoGeo 5-bit value (step=8)."""
    return np.clip(((arr.astype(np.int32) + 4) // 8) * 8, 0, 248).astype(np.uint8)


def kmeans_palette(pixels_hw3, n=15, iters=25, seed=0):
    """
    K-means++ clustering in the NeoGeo colour space.
    Returns an (n, 3) uint8 array of palette RGB values.
    """
    rng   = np.random.default_rng(seed)
    flat  = pixels_hw3.reshape(-1, 3).astype(np.float32)
    # Subsample to 8192 pixels for speed without losing colour diversity
    idx   = rng.choice(len(flat), min(len(flat), 8192), replace=False)
    samp  = flat[idx]

    # K-means++ initialisation
    c = [samp[int(rng.integers(len(samp)))]]
    for _ in range(n - 1):
        d2   = np.min([np.sum((samp - cc) ** 2, axis=1) for cc in c], axis=0)
        prob = d2 / d2.sum()
        c.append(samp[rng.choice(len(samp), p=prob)])
    centers = np.array(c, dtype=np.float32)

    for _ in range(iters):
        d2     = np.sum((samp[:, None] - centers[None]) ** 2, axis=2)
        labels = d2.argmin(axis=1)
        for k in range(n):
            mask = labels == k
            if mask.any():
                centers[k] = samp[mask].mean(0)

    return snap_neogeo(centers.astype(np.uint8))


def floyd_steinberg(img_hw3, palette_n3):
    """
    Floyd-Steinberg dithering.
    Returns (H, W) uint8 array of palette indices (0-based).
    """
    h, w = img_hw3.shape[:2]
    buf  = img_hw3.astype(np.float32).copy()
    out  = np.zeros((h, w), dtype=np.uint8)
    palf = palette_n3.astype(np.float32)

    for y in range(h):
        for x in range(w):
            old  = np.clip(buf[y, x], 0, 255)
            d2   = np.sum((palf - old) ** 2, axis=1)
            k    = int(d2.argmin())
            out[y, x] = k
            err  = old - palf[k]
            if x + 1 < w:
                buf[y,     x + 1] += err * (7 / 16)
            if y + 1 < h:
                if x > 0:
                    buf[y + 1, x - 1] += err * (3 / 16)
                buf[y + 1, x    ] += err * (5 / 16)
                if x + 1 < w:
                    buf[y + 1, x + 1] += err * (1 / 16)
    return out


# ---------------------------------------------------------------------------
# Image preparation
# ---------------------------------------------------------------------------

def crop_center(img, tw, th):
    """Center-crop then resize."""
    w, h = img.size
    if (w / h) > (tw / th):
        nw   = int(h * tw / th)
        left = (w - nw) // 2
        img  = img.crop((left, 0, left + nw, h))
    else:
        nh  = int(w * th / tw)
        top = (h - nh) // 2
        img = img.crop((0, top, w, top + nh))
    return img.resize((tw, th), Image.LANCZOS)


def open_as_rgb(path):
    img = Image.open(path)
    if img.mode == 'P':
        img = img.convert('RGB')
    elif img.mode in ('RGBA', 'LA'):
        bg = Image.new('RGB', img.size, (0, 0, 0))
        bg.paste(img, mask=img.split()[-1])
        img = bg
    else:
        img = img.convert('RGB')
    return img


# ---------------------------------------------------------------------------
# Main conversion
# ---------------------------------------------------------------------------

def convert(src, dst, W=256, H=256, n_colors=15):
    W = (W // 16) * 16
    H = (H // 16) * 16
    print(f"  {os.path.basename(src)}  →  {os.path.basename(dst)}  ({W}x{H}, {n_colors} colours)")

    img = open_as_rgb(src)
    img = crop_center(img, W, H)
    # Light sharpening to compensate for LANCZOS softening
    img = img.filter(ImageFilter.UnsharpMask(radius=1.2, percent=110, threshold=2))

    # Snap to NeoGeo 5-bit colour space BEFORE palette extraction
    arr = snap_neogeo(np.array(img, dtype=np.uint8))

    print(f"    k-means palette extraction…")
    palette = kmeans_palette(arr, n=n_colors)

    print(f"    Floyd-Steinberg dithering…")
    indexed = floyd_steinberg(arr, palette)     # values 0 .. n_colors-1

    # Build PyPNG palette list.  romdbimgimport.py reads len(palette)-1 entries
    # so we pad to n_colors+1 entries; the +1 tail entry is never read.
    pal_list = [(int(r), int(g), int(b)) for r, g, b in palette]
    pal_list.append((0, 0, 0))   # padding so sz = n_colors (not n_colors-1)

    rows = [indexed[y].tolist() for y in range(H)]
    with open(dst, 'wb') as f:
        writer = png.Writer(width=W, height=H, palette=pal_list, bitdepth=8)
        writer.write(f, rows)
    print(f"    Saved.")


# ---------------------------------------------------------------------------
# Batch mode: convert the best docs/img sources into artbox/in/0..9.png
# ---------------------------------------------------------------------------

BATCH_SOURCES = [
    # (source filename,                    crop/resize W, H)
    ('beastlands_0.png',                   256, 256),   # 0 - fantasy background
    ('forest_tiles.png',                   256, 256),   # 1 - forest tileset
    ('generic_platformer_mockup.png',      256, 256),   # 2 - platform level
    ('plastic_shamtastic_mockup.png',      256, 256),   # 3 - colourful scene
    ('gunnes_0.png',                       256, 256),   # 4 - gun/action
    ('beastlands_mockup.png',              256, 256),   # 5 - mockup view
    ('forest_tiles_preview.png',           256, 256),   # 6 - forest preview
    ('plastic_shamtastic_preview.png',     256, 256),   # 7 - shamtastic
    ('255.png',                            256, 256),   # 8 - 256-colour test
    ('plastic_shamtastic_alt_palette.png', 256, 256),   # 9 - alt palette
]


def run_batch():
    os.makedirs(IN_DIR, exist_ok=True)
    for i, (fname, W, H) in enumerate(BATCH_SOURCES):
        src = os.path.join(DOCS_IMGS, fname)
        dst = os.path.join(IN_DIR, f"{i}.png")
        if not os.path.exists(src):
            print(f"  [skip] {fname} not found")
            continue
        convert(src, dst, W, H)
    print("Batch done.")


# ---------------------------------------------------------------------------

if __name__ == '__main__':
    ap = argparse.ArgumentParser(description='Convert any image to NeoGeo indexed PNG')
    ap.add_argument('src',      nargs='?', help='Source image file')
    ap.add_argument('dst',      nargs='?', help='Destination indexed PNG')
    ap.add_argument('-W', '--width',  type=int, default=256)
    ap.add_argument('-H', '--height', type=int, default=256)
    ap.add_argument('-c', '--colors', type=int, default=15,
                    help='Number of palette colours (default 15)')
    ap.add_argument('--batch', action='store_true',
                    help='Convert the standard docs/img set to artbox/in/')
    args = ap.parse_args()

    if args.batch:
        run_batch()
    elif args.src and args.dst:
        convert(args.src, args.dst, args.width, args.height, args.colors)
    else:
        ap.print_help()
        sys.exit(1)
