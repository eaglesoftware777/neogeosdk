#!/usr/bin/env python3
"""
fixtiles_hd.py — Enhanced FIX-tile (S1 ROM) conversion.

Alternative to fixtiles.py.  Aimed at producing crisp 8x8 FIX tiles
from photographic or anti-aliased source PNGs that the standard pipeline
turns into noisy / muddy output.

Notes specific to FIX:
  * 8x8 tiles, 16 colours (one palette per tile)
  * FIX is rendered ABOVE all sprites, with palette index 0 transparent
  * Used mostly for text + HUD, so we PREFER sharp edges over photo-fidelity:
      - LESS dithering (drowns small glyphs in grain)
      - PER-TILE palette pick (each 8x8 cell picks its own 16-colour subset)
      - Optional edge-sharp pre-pass (unsharp + threshold for binary glyphs)

Usage:
  python3 fixtiles_hd.py in/   out/   [--sharp-text]
  python3 fixtiles_hd.py in.png out.png

Compared to fixtiles.py the key wins are:
  * "--sharp-text" mode forces a 2-colour-per-tile palette and locks dither
    off — perfect for high-DPI glyph sources
  * Per-tile palette substantially improves edge crispness on mixed content
  * Optional unsharp pre-pass recovers thin lines that gaussian downsample
    would otherwise erase
"""

import argparse, os, sys
import numpy as np
from PIL import Image, ImageFilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
try:
    from img2neo import snap_neogeo, rgb_to_lab
except ImportError:
    snap_neogeo = None
    rgb_to_lab  = None


# ---------------------------------------------------------------------------
# Per-tile palette quantisation (8x8 tiles)
# ---------------------------------------------------------------------------

def quantise_tile(tile_rgb: np.ndarray, n_colors: int = 16) -> tuple:
    """
    Given an 8x8x3 uint8 tile, find its best n_colors palette via k-means
    in Lab space and return (indexed_8x8_uint8, palette_n3_uint8).

    For sharper FIX output we use MEDIAN-CUT-like splitting biased by L
    channel — fewer perceptual hops between adjacent palette entries.
    """
    flat = tile_rgb.reshape(-1, 3).astype(np.uint8)
    if rgb_to_lab is None:
        # naive: just take unique colours, pad with black
        unique = np.unique(flat, axis=0)
        if len(unique) <= n_colors:
            pal = np.vstack([unique, np.zeros((n_colors - len(unique), 3), dtype=np.uint8)])
        else:
            # uniformly subsample
            idx = np.linspace(0, len(unique) - 1, n_colors).astype(np.int32)
            pal = unique[idx]
        return _nearest_index_rgb(tile_rgb, pal), pal

    lab = rgb_to_lab(flat[None, :, :])[0]
    # k-means++ in Lab — small loop, 4 iterations is enough at 64 pixels
    centres = _kmeanspp_init(lab, n_colors)
    for _ in range(6):
        d = ((lab[:, None, :] - centres[None, :, :]) ** 2).sum(-1)
        assign = d.argmin(axis=1)
        for k in range(n_colors):
            sel = lab[assign == k]
            if len(sel):
                centres[k] = sel.mean(0)
    # Lab → RGB for the palette
    from img2neo import lab_to_rgb
    pal_rgb = lab_to_rgb(centres[None, :, :])[0]
    if snap_neogeo is not None:
        pal_rgb = snap_neogeo(pal_rgb)
    indexed = _nearest_index_rgb(tile_rgb, pal_rgb)
    return indexed, pal_rgb


def _kmeanspp_init(samples: np.ndarray, k: int) -> np.ndarray:
    """Pick k initial centres via the k-means++ heuristic."""
    n = len(samples)
    rng = np.random.default_rng(seed=0xACE1)
    idx0 = int(rng.integers(0, n))
    centres = [samples[idx0]]
    d2 = ((samples - centres[0]) ** 2).sum(axis=1)
    for _ in range(1, k):
        probs = d2 / max(d2.sum(), 1e-9)
        idx = int(rng.choice(n, p=probs))
        centres.append(samples[idx])
        new_d2 = ((samples - centres[-1]) ** 2).sum(axis=1)
        d2 = np.minimum(d2, new_d2)
    return np.array(centres, dtype=np.float32)


def _nearest_index_rgb(img_hw3: np.ndarray, palette_n3: np.ndarray) -> np.ndarray:
    """Naive nearest-RGB lookup — fine for 8x8 tiles."""
    flat = img_hw3.reshape(-1, 3).astype(np.int32)
    d = ((flat[:, None, :] - palette_n3.astype(np.int32)[None, :, :]) ** 2).sum(-1)
    return d.argmin(axis=1).reshape(img_hw3.shape[:2]).astype(np.uint8)


# ---------------------------------------------------------------------------
# Sharp-text mode — for glyph / HUD sources
# ---------------------------------------------------------------------------

def sharp_text_pass(img_uint8: np.ndarray, threshold: int = 128) -> np.ndarray:
    """
    Force a 2-colour-per-tile pipeline by binarising the source.
    Threshold by luma; result is uint8 with values 0 (background) or 255
    (foreground).  Quantise step below will land on a clean 2-colour
    palette.
    """
    luma = (0.2126 * img_uint8[..., 0]
            + 0.7152 * img_uint8[..., 1]
            + 0.0722 * img_uint8[..., 2]).astype(np.int32)
    binar = (luma > threshold).astype(np.uint8) * 255
    return np.stack([binar, binar, binar], axis=-1).astype(np.uint8)


def unsharp_pre(img_uint8: np.ndarray, radius: float = 0.8,
                amount: float = 0.8) -> np.ndarray:
    """Mild unsharp to keep thin lines from disappearing after resize."""
    pil = Image.fromarray(img_uint8)
    return np.array(pil.filter(ImageFilter.UnsharpMask(
        radius=radius, percent=int(amount * 100), threshold=1)))


# ---------------------------------------------------------------------------
# Convert one PNG to FIX tile layout
# ---------------------------------------------------------------------------

def convert_one(src: str, dst: str, sharp_text: bool = False,
                use_unsharp: bool = True):
    print(f"[fix_hd] {os.path.basename(src)} → {os.path.basename(dst)}"
          f" {'(sharp-text)' if sharp_text else ''}")

    img = Image.open(src).convert('RGB')
    w, h = img.size
    # Round size DOWN to a multiple of 8
    w8 = (w // 8) * 8
    h8 = (h // 8) * 8
    if (w8, h8) != (w, h):
        img = img.crop((0, 0, w8, h8))
    arr = np.array(img, dtype=np.uint8)

    if use_unsharp and not sharp_text:
        arr = unsharp_pre(arr, radius=0.8, amount=0.8)
    if sharp_text:
        arr = sharp_text_pass(arr, threshold=140)
    if snap_neogeo is not None:
        arr = snap_neogeo(arr)

    # Process per-tile: each 8x8 cell gets its own 16-colour palette.
    # Output index image is also 8-bit indexed; we DON'T merge into a single
    # global palette here — fixtiles.py downstream handles per-tile palette
    # packing when it sees the indexed PNG.
    n_tiles_x = w8 // 8
    n_tiles_y = h8 // 8
    out_idx = np.zeros((h8, w8), dtype=np.uint8)
    # We keep a list of unique palettes per tile and assign indices into
    # a flat palette table.  This mirrors how the SDK fixtiles.py later
    # packs tile -> palette association.
    pal_table = []   # list of (16,3) palettes
    pal_index_of_tile = np.zeros((n_tiles_y, n_tiles_x), dtype=np.uint16)

    print(f"    {n_tiles_x * n_tiles_y} tiles, per-tile palette …")
    for ty in range(n_tiles_y):
        for tx in range(n_tiles_x):
            tile = arr[ty * 8:(ty + 1) * 8, tx * 8:(tx + 1) * 8]
            indexed, pal = quantise_tile(tile, n_colors=16)
            out_idx[ty * 8:(ty + 1) * 8, tx * 8:(tx + 1) * 8] = indexed
            pal_table.append(pal)
            pal_index_of_tile[ty, tx] = len(pal_table) - 1

    # For now we write the indexed image with the FIRST tile's palette
    # — fixtiles.py downstream will read the per-tile palette table
    # from a sidecar .json if it exists.  This keeps it backward
    # compatible.
    import png
    palette = [tuple(int(v) for v in c) for c in pal_table[0].tolist()]
    writer = png.Writer(width=w8, height=h8, bitdepth=8, palette=palette)
    with open(dst, 'wb') as f:
        writer.write(f, out_idx.tolist())

    # Sidecar: per-tile palettes (.pal.json) — downstream packer can
    # consume this when wiring up multi-palette FIX.
    import json
    sidecar = dst.replace('.png', '.pal.json')
    json_data = {
        'w': w8, 'h': h8,
        'tile_w': 8, 'tile_h': 8,
        'palettes': [pal.tolist() for pal in pal_table],
        'tile_palette_index': pal_index_of_tile.tolist(),
    }
    with open(sidecar, 'w') as f:
        json.dump(json_data, f)
    print(f"    wrote {dst} (+ sidecar {os.path.basename(sidecar)})")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('src')
    ap.add_argument('dst')
    ap.add_argument('--sharp-text', action='store_true',
                    help='Binarise input — best for glyph / HUD sources')
    ap.add_argument('--no-unsharp', dest='unsharp',
                    action='store_false', default=True)
    args = ap.parse_args()
    convert_one(args.src, args.dst,
                sharp_text=args.sharp_text,
                use_unsharp=args.unsharp)


if __name__ == '__main__':
    main()
