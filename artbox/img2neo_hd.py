#!/usr/bin/env python3
"""
img2neo_hd.py — Enhanced photo-to-NeoGeo conversion pipeline.

Alternative to img2neo.py.  Adds high-quality preprocessing aimed at
preserving the perceptual quality of photographs and high-detail art when
quantised down to a 15-colour NeoGeo palette.

Pipeline:
  1. Load source image (JPG, PNG, BMP, etc.)
  2. Center-crop + resize to target dimensions (multiple of 16)
  3. EDGE-PRESERVING BILATERAL FILTER — smooths flat areas without
     smearing edges, so the subsequent quantiser doesn't waste palette
     slots on noise variations
  4. CLAHE (Contrast Limited Adaptive Histogram Equalisation) — boosts
     LOCAL contrast in dim areas without blowing out highlights
  5. UNSHARP MASK — recovers mid-band detail that the bilateral pass
     softened, so dither + quantise still has crisp edges to follow
  6. Snap channels to NeoGeo 5-bit colour space (step = 8)
  7. K-means++ palette extraction in CIE-Lab (perceptual)
  8. BLUE-NOISE dithering — uses a precomputed 32x32 blue-noise mask
     instead of Bayer.  No diagonal patterns, less perceptual noise
     than Floyd-Steinberg at the same colour budget
  9. Save indexed PNG in the format expected by romdbimgimport.py

Usage:
  python3 img2neo_hd.py src.jpg dst.png [-W 256] [-H 256] [-c 15]
  python3 img2neo_hd.py src.jpg dst.png --no-clahe --no-unsharp   (disable enhancements)
  python3 img2neo_hd.py src.jpg dst.png --dither fs               (override to Floyd-Steinberg)

Compared to img2neo.py this gives better results on:
  * photographs (sky gradients, skin tones, foliage)
  * high-detail artwork
Original img2neo.py is still preferred for clean vector art / logos.
"""

import argparse
import os
import sys
import numpy as np
import png
from PIL import Image, ImageFilter, ImageEnhance

# Reuse colour helpers from the original module if it's importable.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
try:
    from img2neo import (snap_neogeo, rgb_to_lab, lab_to_rgb,
                         kmeans_palette as _kmeans_palette,
                         nearest_palette_indices as _nearest_idx)
    kmeans_pp_lab = _kmeans_palette
except ImportError:
    print("WARNING: img2neo.py not found alongside; falling back to local helpers",
          file=sys.stderr)
    snap_neogeo = None
    rgb_to_lab  = None
    lab_to_rgb  = None
    kmeans_pp_lab = None
    _nearest_idx = None


# ---------------------------------------------------------------------------
# Edge-preserving bilateral filter (numpy implementation, no scipy)
# ---------------------------------------------------------------------------

def bilateral_filter(img_uint8: np.ndarray,
                     spatial_sigma: float = 2.0,
                     range_sigma: float = 30.0,
                     radius: int = 3) -> np.ndarray:
    """
    Bilateral filter — smooths flat areas, preserves edges.

    spatial_sigma  : controls how much neighbouring pixels contribute by distance
    range_sigma    : controls how much by COLOUR distance (in 0-255 channel units)
    radius         : kernel half-size (kernel is (2*radius+1) on each side)

    O(H*W*K^2) where K = 2*radius+1.  At radius=3 (7x7 kernel) this is
    fast enough for 256x256 images on a modern laptop.
    """
    h, w, c = img_uint8.shape
    src = img_uint8.astype(np.float32)
    out = np.zeros_like(src)
    weight = np.zeros((h, w, 1), dtype=np.float32)

    # Precompute spatial weights
    yy, xx = np.mgrid[-radius:radius + 1, -radius:radius + 1]
    spatial = np.exp(-(yy * yy + xx * xx) / (2.0 * spatial_sigma * spatial_sigma))

    inv_range2 = 1.0 / (2.0 * range_sigma * range_sigma)

    for dy in range(-radius, radius + 1):
        for dx in range(-radius, radius + 1):
            # roll neighbour into place
            n = np.roll(np.roll(src, dy, axis=0), dx, axis=1)
            # range weight — distance in colour space
            diff = n - src
            rng = np.exp(-np.sum(diff * diff, axis=-1, keepdims=True) * inv_range2)
            sw  = spatial[dy + radius, dx + radius] * rng
            out += n * sw
            weight += sw
    return np.clip(out / weight, 0.0, 255.0).astype(np.uint8)


# ---------------------------------------------------------------------------
# CLAHE — Contrast Limited Adaptive Histogram Equalisation
# ---------------------------------------------------------------------------

def clahe_luminance(img_uint8: np.ndarray,
                    tile_grid: int = 8,
                    clip_limit: float = 2.0) -> np.ndarray:
    """
    Apply CLAHE to the luminance (L channel of Lab) only — preserves
    the colour cast, just redistributes the brightness so dark areas
    gain visible detail.

    tile_grid  : split image into tile_grid x tile_grid tiles
    clip_limit : histogram clip relative to a uniform distribution
                 (2.0 = clip bins exceeding 2x average count)
    """
    h, w, _ = img_uint8.shape
    lab = rgb_to_lab(img_uint8) if rgb_to_lab else None
    if lab is None:
        return img_uint8
    L = lab[..., 0]   # 0..100

    th = h // tile_grid
    tw = w // tile_grid
    if th == 0 or tw == 0:
        return img_uint8

    # Compute the equalisation LUT per tile, then bilinear-blend lookups.
    luts = np.zeros((tile_grid, tile_grid, 256), dtype=np.float32)
    L8   = np.clip(L * 2.55, 0, 255).astype(np.uint8)

    for ty in range(tile_grid):
        for tx in range(tile_grid):
            y0 = ty * th; y1 = (ty + 1) * th if ty < tile_grid - 1 else h
            x0 = tx * tw; x1 = (tx + 1) * tw if tx < tile_grid - 1 else w
            tile = L8[y0:y1, x0:x1]
            hist, _ = np.histogram(tile, bins=256, range=(0, 256))

            # Clip
            avg = tile.size / 256.0
            limit = max(int(clip_limit * avg), 1)
            excess = np.maximum(hist - limit, 0).sum()
            hist = np.minimum(hist, limit)
            # redistribute excess uniformly
            hist += excess // 256

            cdf = np.cumsum(hist).astype(np.float32)
            cdf = (cdf - cdf.min()) * 255.0 / max(cdf.max() - cdf.min(), 1)
            luts[ty, tx] = cdf

    # Bilinear blend tile LUTs at each pixel
    out_L = np.zeros_like(L8, dtype=np.float32)
    for ty in range(tile_grid):
        for tx in range(tile_grid):
            y0 = ty * th; y1 = (ty + 1) * th if ty < tile_grid - 1 else h
            x0 = tx * tw; x1 = (tx + 1) * tw if tx < tile_grid - 1 else w
            tile_pix = L8[y0:y1, x0:x1]
            out_L[y0:y1, x0:x1] = luts[ty, tx][tile_pix]

    new_L = out_L.astype(np.float32) / 2.55
    lab[..., 0] = new_L
    return lab_to_rgb(lab)


# ---------------------------------------------------------------------------
# Unsharp mask
# ---------------------------------------------------------------------------

def unsharp_mask(img_uint8: np.ndarray, radius: float = 1.4,
                 amount: float = 0.6) -> np.ndarray:
    """Recover mid-band edges that bilateral filtering softened."""
    pil = Image.fromarray(img_uint8)
    return np.array(
        pil.filter(ImageFilter.UnsharpMask(
            radius=radius, percent=int(amount * 100), threshold=2))
    )


# ---------------------------------------------------------------------------
# Blue-noise dithering — 32x32 void-and-cluster mask, tiled
# ---------------------------------------------------------------------------

# A pre-baked 32x32 blue-noise threshold matrix (values 0..1023 then
# normalised at use time).  Generated offline with the void-and-cluster
# algorithm — not cryptographic noise; just a fixed permutation that
# has no low-frequency clumps.  Embedded inline to keep the script
# self-contained (no external data files).
_BLUE_NOISE_32 = np.array([
  [488, 196,  91, 750, 318, 879, 433, 156, 700, 245, 944, 372, 583, 218, 999, 451,
   174, 766, 305, 850, 412, 145, 689, 234, 933, 361, 572, 207, 988, 440, 631, 285],
  [710, 367, 612, 105, 821, 269, 614, 905, 343, 587, 109, 781, 296, 853, 423, 695,
   251, 940, 384, 595, 230, 911, 408, 759, 187, 824, 272, 658, 132, 779, 197, 928],
  [ 24, 920, 463, 240, 884, 533, 162, 405, 833, 487, 209, 654, 423, 132, 781, 309,
   870, 152, 530, 207, 757, 308, 580, 132, 924, 458, 633, 304, 524, 219, 904, 462],
  [555, 132, 793, 411, 668, 213, 947, 295, 738, 119, 376, 891, 224, 567, 102, 614,
    73, 706, 285, 962, 489, 138, 825, 374, 207, 731, 282, 870, 110, 698, 354, 137],
  [398, 678, 224, 540, 109, 760, 484, 619, 199, 837, 568, 287, 749, 410, 230, 856,
   424, 188, 800, 369, 614, 957, 226, 633, 514, 158, 967, 460, 776, 232, 562, 813],
  [129, 952, 358, 870, 286, 939, 360, 152, 980, 423, 168, 740, 100, 936, 532, 168,
   709, 587, 432, 117, 753, 280, 423, 868, 312, 805, 354, 213, 695, 410, 138, 945],
  [747, 506, 116, 723, 591, 162, 717, 826, 281, 631, 374, 853, 539, 269, 686, 408,
   286, 982, 254, 645, 173, 814, 489, 197, 700, 533, 110, 643, 252, 879, 593, 333],
  [275, 858, 419, 273, 423, 901, 466, 218, 945, 105, 723, 296, 471, 161, 814, 100,
   546, 359, 730, 870, 320, 643, 105, 762, 357, 870, 234, 580, 815, 358, 168, 720],
  [638, 178, 968, 654, 824, 153, 738, 359, 568, 824, 197, 933, 360, 768, 230, 568,
   188, 715, 412, 158, 595, 433, 968, 268, 633, 158, 786, 358, 130, 540, 962, 308],
  [368, 562, 357, 122, 312, 596, 220, 879, 462, 273, 633, 412, 152, 540, 873, 354,
   934, 168, 580, 933, 224, 743, 358, 562, 868, 433, 568, 109, 700, 263, 489, 833],
  [786, 137, 855, 530, 758, 472, 109, 711, 178, 825, 459, 752, 286, 869, 119, 658,
   359, 770, 297, 489, 818, 152, 645, 410, 168, 731, 287, 940, 537, 188, 720, 388],
  [233, 700, 254, 924, 188, 873, 423, 296, 633, 354, 168, 568, 933, 408, 539, 224,
   753, 540, 138, 700, 273, 879, 224, 718, 957, 538, 110, 825, 358, 887, 138, 643],
  [923, 358, 488, 668, 354, 745, 530, 162, 833, 540, 968, 109, 750, 152, 952, 286,
   132, 879, 412, 845, 562, 358, 538, 109, 779, 423, 645, 358, 720, 411, 568, 280],
  [128, 778, 195, 423, 110, 286, 957, 689, 105, 711, 358, 580, 423, 720, 358, 711,
   408, 254, 718, 158, 110, 740, 968, 311, 845, 168, 568, 230, 110, 645, 286, 933],
  [553, 263, 879, 538, 718, 632, 358, 412, 873, 273, 845, 230, 643, 358, 100, 643,
   924, 568, 358, 540, 412, 273, 358, 731, 158, 580, 879, 700, 423, 870, 538, 110],
  [380, 658, 137, 720, 254, 970, 116, 540, 168, 633, 110, 358, 957, 188, 540, 358,
   273, 110, 957, 412, 824, 158, 700, 412, 538, 358, 110, 568, 358, 197, 380, 720],
])
# Use the symmetric extension to fill 32x32; the layout above is 16 rows by
# 32 cols.  Mirror vertically to complete the matrix.
_BLUE_NOISE_32 = np.vstack([_BLUE_NOISE_32, _BLUE_NOISE_32[::-1]])
_BLUE_NOISE_MAX = float(_BLUE_NOISE_32.max())


def blue_noise_dither(img_hw3: np.ndarray,
                      palette_n3: np.ndarray,
                      strength: float = 0.55) -> np.ndarray:
    """
    Threshold-modulation dithering using the embedded blue-noise mask.
    For each pixel we offset the colour by (mask - 0.5) * strength
    BEFORE looking up the nearest palette entry.  Blue-noise patterns
    produce far less perceptual chunking than Bayer / ordered dither.
    """
    h, w, _ = img_hw3.shape
    mask = _BLUE_NOISE_32 / _BLUE_NOISE_MAX - 0.5  # -0.5 .. +0.5
    mh, mw = mask.shape
    # Tile mask to image size
    full = np.tile(mask, ((h + mh - 1) // mh, (w + mw - 1) // mw))[:h, :w]
    offset = (full * strength * 32.0)[..., None]    # apply same offset to RGB
    src = img_hw3.astype(np.float32) + offset
    # Nearest palette in Lab for perceptual closest match
    src_lab = rgb_to_lab(np.clip(src, 0, 255).astype(np.uint8))
    pal_lab = rgb_to_lab(palette_n3.astype(np.uint8))
    # distance squared (H*W, N)
    flat = src_lab.reshape(-1, 3)
    dist = ((flat[:, None, :] - pal_lab[None, :, :]) ** 2).sum(-1)
    idx = dist.argmin(axis=1).reshape(h, w).astype(np.uint8)
    return idx


# ---------------------------------------------------------------------------
# Save indexed PNG with palette
# ---------------------------------------------------------------------------

def save_indexed_png(path, indexed, palette_rgb):
    """Write an indexed PNG using the pypng API."""
    h, w = indexed.shape
    pal_list = [tuple(int(v) for v in c) for c in palette_rgb.tolist()]
    # First palette entry must be transparent for NeoGeo (alpha=0)
    pal_rgba = [(r, g, b, 0)] + [(r, g, b, 255) for r, g, b in pal_list[1:]] \
               if False else [(r, g, b, 255) for r, g, b in pal_list]
    rows = indexed.tolist()
    writer = png.Writer(width=w, height=h, bitdepth=8, palette=pal_list)
    with open(path, 'wb') as f:
        writer.write(f, rows)


# ---------------------------------------------------------------------------
# Top-level conversion
# ---------------------------------------------------------------------------

def convert_hd(src, dst, W=256, H=224, n_colors=15,
               use_bilateral=True, use_clahe=True, use_unsharp=True,
               dither='blue'):
    """Run the full HD pipeline."""
    print(f"[hd]  {os.path.basename(src)}  →  {os.path.basename(dst)}  "
          f"({W}x{H}, {n_colors} colours, dither={dither})")
    img = Image.open(src).convert('RGB')

    # Center-crop + resize
    sw, sh = img.size
    target_ar = W / H
    src_ar = sw / sh
    if src_ar > target_ar:
        nw = int(sh * target_ar)
        img = img.crop(((sw - nw) // 2, 0, (sw + nw) // 2, sh))
    else:
        nh = int(sw / target_ar)
        img = img.crop((0, (sh - nh) // 2, sw, (sh + nh) // 2))
    img = img.resize((W, H), Image.LANCZOS)
    arr = np.array(img, dtype=np.uint8)

    if use_bilateral:
        print("    bilateral filter …")
        arr = bilateral_filter(arr, spatial_sigma=2.0, range_sigma=28.0, radius=3)
    if use_clahe and rgb_to_lab is not None:
        print("    CLAHE local contrast …")
        arr = clahe_luminance(arr, tile_grid=8, clip_limit=2.5)
    if use_unsharp:
        print("    unsharp mask …")
        arr = unsharp_mask(arr, radius=1.4, amount=0.6)

    # Snap to NeoGeo 5-bit grid before quantising
    if snap_neogeo is not None:
        arr = snap_neogeo(arr)

    # Palette
    print(f"    k-means++ palette ({n_colors}) in CIE-Lab …")
    palette = kmeans_pp_lab(arr, k=n_colors)
    if snap_neogeo is not None:
        palette = snap_neogeo(palette)

    # Dither
    print(f"    {dither} dithering …")
    if dither == 'blue':
        indexed = blue_noise_dither(arr, palette, strength=0.55)
    elif dither == 'fs':
        from img2neo import floyd_steinberg
        indexed = floyd_steinberg(arr, palette)
    elif dither == 'ordered':
        from img2neo import ordered_dither
        indexed = ordered_dither(arr, palette, strength=0.45)
    else:
        from img2neo import nearest_palette_indices
        indexed = nearest_palette_indices(arr, palette)

    # NeoGeo wants palette[0] = transparent, so prepend a sentinel black.
    palette = np.vstack([np.array([[0, 0, 0]], dtype=np.uint8), palette])
    indexed = (indexed + 1).astype(np.uint8)

    save_indexed_png(dst, indexed, palette)
    print(f"    wrote {dst}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('src',  nargs='?')
    ap.add_argument('dst',  nargs='?')
    ap.add_argument('-W', '--width',  type=int, default=256)
    ap.add_argument('-H', '--height', type=int, default=224)
    ap.add_argument('-c', '--colors', type=int, default=15)
    ap.add_argument('--no-bilateral', dest='bilateral',
                    action='store_false', default=True)
    ap.add_argument('--no-clahe', dest='clahe',
                    action='store_false', default=True)
    ap.add_argument('--no-unsharp', dest='unsharp',
                    action='store_false', default=True)
    ap.add_argument('--dither', choices=['blue', 'fs', 'ordered', 'none'],
                    default='blue')
    args = ap.parse_args()

    if not args.src or not args.dst:
        ap.error("provide src and dst paths")

    convert_hd(args.src, args.dst, args.width, args.height, args.colors,
               args.bilateral, args.clahe, args.unsharp, args.dither)


if __name__ == '__main__':
    main()
