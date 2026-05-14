#!/usr/bin/env python3
"""
img2neo.py — Convert any image to a NeoGeo-ready 15-colour indexed PNG.

Pipeline:
  1. Load source image (JPG, PNG, BMP, etc.)
  2. Center-crop and resize to target dimensions (multiple of 16)
  3. Pre-quantise every channel to the NeoGeo 5-bit colour space (step=8)
  4. Extract 15-colour palette via k-means++ in CIE-Lab space (perceptual)
  5. Apply serpentine Floyd-Steinberg dithering (alternating scan, sharper grain)
  6. Optional second-pass quality refinement (re-anchors worst tiles)
  7. Save indexed PNG in the format expected by romdbimgimport.py

Usage:
  python3 img2neo.py src.jpg dst.png [-W 256] [-H 256] [-c 15]
  python3 img2neo.py --batch            (converts all docs images to artbox/in/)
"""

import argparse, os, sys
import numpy as np
import png
from PIL import Image, ImageFilter, ImageEnhance

DOCS_IMGS = os.path.join(os.path.dirname(__file__), '..', '..', 'docs', 'img')
IN_DIR    = os.path.join(os.path.dirname(__file__), 'in')


# ---------------------------------------------------------------------------
# NeoGeo colour-space utilities
# ---------------------------------------------------------------------------

def snap_neogeo(arr):
    """Round each 8-bit channel to the nearest NeoGeo 5-bit value (step=8)."""
    return np.clip(((arr.astype(np.int32) + 4) // 8) * 8, 0, 248).astype(np.uint8)


# ---------------------------------------------------------------------------
# CIE-Lab colour conversion (D65, no scipy dependency)
# ---------------------------------------------------------------------------

def _srgb_to_linear(c: np.ndarray) -> np.ndarray:
    """sRGB gamma-expand, input float 0-1."""
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def _linear_to_srgb(c: np.ndarray) -> np.ndarray:
    """sRGB gamma-compress, input float (may be negative/clipped)."""
    c = np.maximum(c, 0.0)
    return np.where(c <= 0.0031308, 12.92 * c, 1.055 * c ** (1.0 / 2.4) - 0.055)


_M_RGB_XYZ = np.array([
    [0.4124564, 0.3575761, 0.1804375],
    [0.2126729, 0.7151522, 0.0721750],
    [0.0193339, 0.1191920, 0.9503041],
], dtype=np.float32)

_M_XYZ_RGB = np.array([
    [ 3.2404542, -1.5371385, -0.4985314],
    [-0.9692660,  1.8760108,  0.0415560],
    [ 0.0556434, -0.2040259,  1.0572252],
], dtype=np.float32)

_D65_WHITE = np.array([0.95047, 1.00000, 1.08883], dtype=np.float32)


def rgb_to_lab(arr_uint8: np.ndarray) -> np.ndarray:
    """Convert (..., 3) uint8 RGB → (..., 3) float32 CIE-Lab (D65)."""
    c = arr_uint8.astype(np.float32) / 255.0
    linear = _srgb_to_linear(c)
    xyz = linear @ _M_RGB_XYZ.T
    xyz /= _D65_WHITE
    # f(t) cube-root with linear tail
    f = np.where(xyz > 0.008856, np.cbrt(np.maximum(xyz, 0.0)),
                 7.787 * xyz + 16.0 / 116.0)
    L = 116.0 * f[..., 1] - 16.0
    a = 500.0 * (f[..., 0] - f[..., 1])
    b = 200.0 * (f[..., 1] - f[..., 2])
    return np.stack([L, a, b], axis=-1).astype(np.float32)


def lab_to_rgb(lab: np.ndarray) -> np.ndarray:
    """Convert (..., 3) float32 CIE-Lab → (..., 3) uint8 RGB (clipped)."""
    L, a, b = lab[..., 0], lab[..., 1], lab[..., 2]
    fy = (L + 16.0) / 116.0
    fx = a / 500.0 + fy
    fz = fy - b / 200.0
    eps = 0.20689655  # cbrt(0.008856)
    x = np.where(fx > eps, fx ** 3, (fx - 16.0 / 116.0) / 7.787)
    y = np.where(fy > eps, fy ** 3, (fy - 16.0 / 116.0) / 7.787)
    z = np.where(fz > eps, fz ** 3, (fz - 16.0 / 116.0) / 7.787)
    xyz = np.stack([x * _D65_WHITE[0], y * _D65_WHITE[1], z * _D65_WHITE[2]], axis=-1)
    linear = xyz @ _M_XYZ_RGB.T
    srgb = _linear_to_srgb(linear)
    return np.clip(srgb * 255.0, 0, 255).astype(np.uint8)


# ---------------------------------------------------------------------------
# Palette extraction — k-means++ in CIE-Lab space
# ---------------------------------------------------------------------------

def kmeans_palette(pixels_hw3, n=15, iters=25, seed=0, sample_limit=8192):
    """
    K-means++ in CIE-Lab space for a perceptually optimal NeoGeo palette.
    Accepts (..., 3) uint8 RGB, returns (n, 3) uint8 snapped NeoGeo RGB.
    """
    rng  = np.random.default_rng(seed)
    snapped = snap_neogeo(pixels_hw3)
    flat_lab = rgb_to_lab(snapped.reshape(-1, 3))

    idx  = rng.choice(len(flat_lab), min(len(flat_lab), sample_limit), replace=False)
    samp = flat_lab[idx]

    # K-means++ initialisation in Lab space
    c = [samp[int(rng.integers(len(samp)))]]
    for _ in range(n - 1):
        d2   = np.min([np.sum((samp - cc) ** 2, axis=1) for cc in c], axis=0)
        prob = d2 / (d2.sum() + 1e-10)
        c.append(samp[rng.choice(len(samp), p=prob)])
    centers = np.array(c, dtype=np.float32)

    for _ in range(iters):
        d2     = np.sum((samp[:, None] - centers[None]) ** 2, axis=2)
        labels = d2.argmin(axis=1)
        for k in range(n):
            mask = labels == k
            if mask.any():
                centers[k] = samp[mask].mean(0)

    # Convert Lab centres back to RGB and snap to NeoGeo grid
    return snap_neogeo(lab_to_rgb(centers))


# ---------------------------------------------------------------------------
# Dithering — serpentine Floyd-Steinberg (perceptually uniform grain)
# ---------------------------------------------------------------------------

def floyd_steinberg(img_hw3, palette_n3):
    """
    Serpentine Floyd-Steinberg dithering (alternating scan direction per row).
    Returns (H, W) uint8 array of palette indices (0-based).
    """
    h, w = img_hw3.shape[:2]
    buf  = img_hw3.astype(np.float32).copy()
    out  = np.zeros((h, w), dtype=np.uint8)
    palf = palette_n3.astype(np.float32)

    for y in range(h):
        if y % 2 == 0:
            xs = range(w)
            nbr = lambda x: [(0, x+1, 7/16), (1, x-1, 3/16), (1, x, 5/16), (1, x+1, 1/16)]
        else:
            xs = range(w - 1, -1, -1)
            nbr = lambda x: [(0, x-1, 7/16), (1, x+1, 3/16), (1, x, 5/16), (1, x-1, 1/16)]

        for x in xs:
            old  = np.clip(buf[y, x], 0, 255)
            d2   = np.sum((palf - old) ** 2, axis=1)
            k    = int(d2.argmin())
            out[y, x] = k
            err  = old - palf[k]
            for dy, nx, weight in nbr(x):
                ny = y + dy
                if 0 <= ny < h and 0 <= nx < w:
                    buf[ny, nx] += err * weight
    return out


def nearest_palette_indices(img_hw3, palette_n3):
    """Fast nearest-colour lookup without diffusion dithering."""
    pixels = img_hw3.reshape(-1, 3).astype(np.int16)
    pal    = palette_n3.astype(np.int16)
    diff   = pixels[:, None, :] - pal[None, :, :]
    dist   = np.sum(diff * diff, axis=2)
    return np.argmin(dist, axis=1).astype(np.uint8).reshape(img_hw3.shape[:2])


def ordered_dither(img_hw3, palette_n3, strength=0.65):
    """
    Ordered (Bayer 4×4) dithering — fast, good for large screens.
    Keeps the image calmer than full error diffusion.
    """
    bayer4 = np.array(
        [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]],
        dtype=np.float32,
    )
    threshold = ((bayer4 / 15.0) - 0.5) * (8.0 * strength)
    tiled = np.tile(
        threshold,
        ((img_hw3.shape[0] + 3) // 4, (img_hw3.shape[1] + 3) // 4),
    )[: img_hw3.shape[0], : img_hw3.shape[1]]
    adjusted = np.clip(img_hw3.astype(np.float32) + tiled[:, :, None], 0, 255)
    adjusted = snap_neogeo(adjusted.astype(np.uint8))
    return nearest_palette_indices(adjusted, palette_n3)


# ---------------------------------------------------------------------------
# Second-pass quality refinement
# ---------------------------------------------------------------------------

def quality_second_pass(indexed_hw, palette_n3, img_hw3, mse_threshold=200.0,
                         n_worst=6, iters=20, sample_limit=16384):
    """
    Re-extract palette with extra weight on the worst 16×16 tiles.
    Returns a (potentially improved) (n, 3) palette and re-dithered indexed image.
    """
    h, w = indexed_hw.shape
    n = len(palette_n3)

    # Compute per-tile MSE
    tile_mse = []
    for ty in range(0, h, 16):
        for tx in range(0, w, 16):
            orig  = img_hw3[ty:ty+16, tx:tx+16].astype(np.float32)
            recon = palette_n3[indexed_hw[ty:ty+16, tx:tx+16]].astype(np.float32)
            mse   = float(np.mean((orig - recon) ** 2))
            tile_mse.append((mse, ty, tx))

    worst = sorted(tile_mse, reverse=True)[:n_worst]
    if not worst or worst[0][0] <= mse_threshold:
        return palette_n3, indexed_hw  # already good enough

    # Collect anchor colours from the worst tiles
    anchor_rgb = []
    for _, ty, tx in worst:
        tile = snap_neogeo(img_hw3[ty:ty+16, tx:tx+16])
        unique = np.unique(tile.reshape(-1, 3), axis=0)
        # Pick the 2 most visually distinct colours from this tile
        if len(unique) >= 2:
            lab = rgb_to_lab(unique)
            spread = np.max(np.sum((lab[:, None] - lab[None]) ** 2, axis=2), axis=1)
            tops = unique[np.argsort(spread)[-2:]]
            anchor_rgb.extend(tops.tolist())

    anchor_rgb = snap_neogeo(np.array(anchor_rgb, dtype=np.uint8))
    # Deduplicate anchors
    anchor_rgb = np.unique(anchor_rgb, axis=0)
    n_anch = min(len(anchor_rgb), n // 3)
    anchor_rgb = anchor_rgb[:n_anch]

    # Rebuild augmented pixel pool: normal pixels + 3× duplicated worst-tile pixels
    worst_pixels = np.vstack([
        img_hw3[ty:ty+16, tx:tx+16].reshape(-1, 3) for _, ty, tx in worst
    ])
    augmented = np.vstack([img_hw3.reshape(-1, 3),
                            worst_pixels, worst_pixels, worst_pixels])

    # New palette: anchors + free k-means slots
    n_free = n - n_anch
    free_pal = kmeans_palette(augmented.reshape(1, -1, 3),
                               n=n_free, iters=iters, seed=42,
                               sample_limit=sample_limit)
    new_palette = np.vstack([anchor_rgb, free_pal])

    # Re-dither full image with the improved palette
    arr = snap_neogeo(img_hw3)
    new_indexed = floyd_steinberg(arr, new_palette)

    # Keep the version with lower overall MSE
    old_mse = np.mean((palette_n3[indexed_hw].astype(np.float32)
                       - img_hw3.astype(np.float32)) ** 2)
    new_mse = np.mean((new_palette[new_indexed].astype(np.float32)
                       - img_hw3.astype(np.float32)) ** 2)
    if new_mse < old_mse:
        return new_palette, new_indexed
    return palette_n3, indexed_hw


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

def convert(src, dst, W=256, H=256, n_colors=15, dither='ordered'):
    """
    dither: 'ordered' (Bayer 4×4, calmer grain — recommended for logos/sprites),
            'fs' (serpentine Floyd-Steinberg, maximum diffusion),
            'none' (nearest colour only, cleanest solid areas).
    """
    W = (W // 16) * 16
    H = (H // 16) * 16
    print(f"  {os.path.basename(src)}  →  {os.path.basename(dst)}  ({W}x{H}, {n_colors} colours, dither={dither})")

    img = open_as_rgb(src)
    img = crop_center(img, W, H)
    img = ImageEnhance.Contrast(img).enhance(1.15)
    img = ImageEnhance.Color(img).enhance(1.25)
    img = img.filter(ImageFilter.UnsharpMask(radius=1.2, percent=120, threshold=3))

    arr = snap_neogeo(np.array(img, dtype=np.uint8))

    print(f"    k-means palette extraction (CIE-Lab)…")
    palette = kmeans_palette(arr, n=n_colors, iters=40, sample_limit=20480)

    print(f"    {dither} dithering…")
    if dither == 'none':
        indexed = nearest_palette_indices(arr, palette)
    elif dither == 'fs':
        indexed = floyd_steinberg(arr, palette)
    else:  # 'ordered' — default
        indexed = ordered_dither(arr, palette, strength=0.45)

    print(f"    quality refinement pass…")
    palette, indexed = quality_second_pass(indexed, palette, arr)

    pal_list = [(int(r), int(g), int(b)) for r, g, b in palette]
    rows = [indexed[y].tolist() for y in range(H)]
    with open(dst, 'wb') as f:
        writer = png.Writer(width=W, height=H, palette=pal_list, bitdepth=8)
        writer.write(f, rows)
    print(f"    Saved.")


# ---------------------------------------------------------------------------
# Batch mode
# ---------------------------------------------------------------------------

BATCH_SOURCES = [
    ('beastlands_0.png',                   256, 256),
    ('forest_tiles.png',                   256, 256),
    ('generic_platformer_mockup.png',      256, 256),
    ('plastic_shamtastic_mockup.png',      256, 256),
    ('gunnes_0.png',                       256, 256),
    ('beastlands_mockup.png',              256, 256),
    ('forest_tiles_preview.png',           256, 256),
    ('plastic_shamtastic_preview.png',     256, 256),
    ('255.png',                            256, 256),
    ('plastic_shamtastic_alt_palette.png', 256, 256),
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
    ap.add_argument('--dither', choices=['ordered', 'fs', 'none'], default='ordered',
                    help='Dithering method: ordered (default, Bayer 4x4), fs (Floyd-Steinberg), none')
    ap.add_argument('--batch', action='store_true',
                    help='Convert the standard docs/img set to artbox/in/')
    args = ap.parse_args()

    if args.batch:
        run_batch()
    elif args.src and args.dst:
        convert(args.src, args.dst, args.width, args.height, args.colors, args.dither)
    else:
        ap.print_help()
        sys.exit(1)
