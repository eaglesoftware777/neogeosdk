#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
img2neo_tile.py — Spatially Localized Adaptive Clustering with
Perceptual Luminance Weighted Error Diffusion.

Why this exists
---------------
Global-palette converters (img2neo, img2neo_hd, img2neo_crt) all
quantize a whole image against a single 15-colour LUT.  For
gradient-heavy artwork — sunsets, skin tones layered against HUD
text, screen-sized title cards — the global palette has to compromise
between "smooth gradient" and "sharp text", and ends up doing neither
well.  HUD pixels get washed into the background; sky bands posterise
into 3-4 visible stripes.

The tile-local approach quantizes each 16x16 macroblock against its
OWN dynamically computed 15-colour LUT.  A tile that contains nothing
but white-on-black HUD text gets a palette with crisp white + a few
black/grey gradients; a tile that's pure gradient sky gets 15 cleanly
spaced sky shades.  Neither tile's palette pollutes the other.

The hardware catch
------------------
Neo Geo has only 256 palette banks of 16 colours in palette RAM.  A
320x224 image is 20*14 = 280 tiles, already over the bank limit.
This module deliberately does NOT solve the bank-fit problem — it
produces the highest-fidelity indexed-tile-plus-local-palette pair,
then leaves palette deduplication / merging to a downstream pass.
See `cluster_palette_banks()` for a starter implementation that
collapses near-identical palettes into shared banks.

Algorithm summary
-----------------
1. LANCZOS resize the input to a target divisible by 16.
2. For each 16x16 tile (raster scan):
   a. Convert tile RGB -> luminance-weighted YCbCr (Y * 4, Cb, Cr).
      The 4x weight on Y means k-means and the error-diffusion
      nearest-palette lookup both prioritise brightness over hue,
      which matches how the CRT (and the human eye) perceives
      detail.
   b. Run k-means++ in this weighted space to derive a 15-colour
      local LUT.  Transparent pixels are masked out so they don't
      occupy a cluster slot.
   c. Floyd-Steinberg dither the tile against its local LUT, using
      the same luminance-weighted distance for nearest-colour lookup.
      Diffuse the unweighted RGB error to neighbours (standard
      7/3/5/1 kernel) so the dither remains colour-faithful.
   d. Snap palette colours to the NeoGeo 5-bit-per-channel grid
      (step=8) so the LUT is exact ROM-storable.
   e. Shift assigned indices up by +1 so palette slot 0 stays free
      (NeoGeo hardware: slot 0 == transparency).
3. Emit per-tile artefacts ready for downstream packing.

Public API
----------
convert_png_to_neogeo_tile_palettes(image_path, target_w=None,
                                    target_h=None, fit="contain")

  Returns
  -------
  indices_flat : ndarray (n_tiles, 256) uint8 in 1..15 for opaque,
                 0 for transparent — one 256-byte buffer per tile,
                 raster-scan order (top-to-bottom rows of tiles,
                 left-to-right within each row).
  palettes     : ndarray (n_tiles, 15, 3) uint8 RGB on the 5-bit
                 NeoGeo grid (step=8).  palette[i, k] gives RGB for
                 index (k+1) of tile i.  Flatten to bytes for the
                 45-byte-per-tile contract: palettes.reshape(-1, 45).
  metadata     : dict {tile_cols, tile_rows, canvas_w, canvas_h,
                       content_left, content_top, content_width,
                       content_height, n_tiles, target_w, target_h}.

cluster_palette_banks(palettes, max_banks=256, lab_eps=8.0)

  Optional downstream pass.  Groups per-tile palettes that fall
  within `lab_eps` mean-Lab distance into shared banks and returns:
  (bank_palettes, tile_to_bank).  `bank_palettes` is bounded by
  `max_banks` so the result fits in NeoGeo palette RAM.

CLI
---
python3 img2neo_tile.py src.png dst_dir [-W 320 -H 224] [-c 15]
                                        [--merge-banks N]

  Writes:
    dst_dir/indices.bin     — n_tiles * 256 bytes, raw flat
    dst_dir/palettes.bin    — n_tiles * 45 bytes (15 RGB triples)
    dst_dir/preview.png     — indexed PNG using a flattened megapalette
                              for visual QA only (not ROM-packable)
    dst_dir/meta.json       — metadata dict
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path

import numpy as np
import png
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# Reuse the NeoGeo 5-bit grid snap and Lab conversion from img2neo —
# keeps the module dependency-free of scikit-learn / scikit-image.
from img2neo import snap_neogeo, rgb_to_lab, kmeans_palette as _img2neo_kmeans_palette


TILE_SIZE = 16
COLORS_PER_TILE = 15
PIXELS_PER_TILE = TILE_SIZE * TILE_SIZE   # 256
ALPHA_OPAQUE_THRESHOLD = 128


# ---------------------------------------------------------------------------
# Perceptually-weighted colour space — luminance-emphasised YCbCr
# ---------------------------------------------------------------------------

# BT.601 RGB -> YCbCr coefficients.  Y is full-swing 0..255.
_M_RGB_YCBCR = np.array([
    [ 0.299,     0.587,     0.114   ],
    [-0.168736, -0.331264,  0.5     ],
    [ 0.5,      -0.418688, -0.081312],
], dtype=np.float32)

# Weight applied to the Y axis before clustering / nearest-palette
# lookup.  4.0 means a 1-unit luminance error counts 16x more than a
# 1-unit chroma error in squared distance — strong perceptual bias.
LUMA_WEIGHT = 4.0


def rgb_to_weighted_ycbcr(rgb_uint8: np.ndarray) -> np.ndarray:
    """
    Convert (..., 3) uint8 RGB into a perceptually-weighted YCbCr
    feature space where the Y channel is scaled by LUMA_WEIGHT.

    Cb/Cr are kept in their native 0-centred ±0.5*255 range.  All
    distance comparisons happen in this space so k-means clusters
    along luminance bands first, hue distinctions second.
    """
    rgb = rgb_uint8.astype(np.float32)
    ycc = rgb @ _M_RGB_YCBCR.T
    ycc[..., 0] *= LUMA_WEIGHT
    return ycc


# ---------------------------------------------------------------------------
# k-means++ in the weighted YCbCr space (small-k, small-n specialisation)
# ---------------------------------------------------------------------------

def _kmeans_pp_weighted(samples: np.ndarray,
                        n_clusters: int,
                        max_iters: int = 20,
                        rng_seed: int = 0) -> np.ndarray:
    """
    K-means++ in weighted-YCbCr space.  Input `samples` is (N, 3)
    float32 already in weighted-YCbCr coordinates (i.e. Y pre-scaled).
    Returns (n_clusters, 3) cluster centres in the same space.

    Specialised for the tile case (n_clusters ~= 15, N <= 256) — no
    mini-batching, no sample-limit subsampling.
    """
    n = samples.shape[0]
    if n == 0:
        return np.zeros((n_clusters, 3), dtype=np.float32)
    if n <= n_clusters:
        # Pad with the last sample so the LUT slot count is fixed.
        out = np.zeros((n_clusters, 3), dtype=np.float32)
        out[:n] = samples
        out[n:] = samples[-1]
        return out

    rng = np.random.default_rng(rng_seed)

    # k-means++ seed: first centre is random, subsequent centres are
    # chosen with probability proportional to squared distance from
    # nearest existing centre.
    centres = np.empty((n_clusters, 3), dtype=np.float32)
    centres[0] = samples[rng.integers(n)]
    closest_d2 = np.sum((samples - centres[0]) ** 2, axis=1)
    for i in range(1, n_clusters):
        total = closest_d2.sum()
        if total <= 1e-12:
            # All samples already on a centre — pick any remaining.
            centres[i] = samples[rng.integers(n)]
        else:
            probs = closest_d2 / total
            centres[i] = samples[rng.choice(n, p=probs)]
            new_d2 = np.sum((samples - centres[i]) ** 2, axis=1)
            closest_d2 = np.minimum(closest_d2, new_d2)

    # Lloyd's iterations.
    for _ in range(max_iters):
        # Assignment step: nearest centre per sample.
        d2 = np.sum((samples[:, None, :] - centres[None, :, :]) ** 2, axis=2)
        labels = d2.argmin(axis=1)
        # Update step: centre = mean of assigned samples.
        new_centres = centres.copy()
        moved = False
        for k in range(n_clusters):
            mask = labels == k
            if mask.any():
                mean = samples[mask].mean(axis=0)
                if np.any(np.abs(mean - centres[k]) > 1e-3):
                    moved = True
                new_centres[k] = mean
        centres = new_centres
        if not moved:
            break

    return centres


# ---------------------------------------------------------------------------
# Floyd-Steinberg with perceptual nearest-colour lookup
# ---------------------------------------------------------------------------

# Standard Floyd-Steinberg diffusion kernel — pushes 7/16 right,
# 3/16 down-left, 5/16 down, 1/16 down-right.
_FS_KERNEL = (
    (0,  1, 7.0 / 16.0),
    (1, -1, 3.0 / 16.0),
    (1,  0, 5.0 / 16.0),
    (1,  1, 1.0 / 16.0),
)


def _floyd_steinberg_perceptual(tile_rgb: np.ndarray,
                                 palette_rgb: np.ndarray,
                                 alpha_mask: np.ndarray) -> np.ndarray:
    """
    Floyd-Steinberg dither a single 16x16 tile against its local
    palette.  Nearest-colour lookup uses perceptually-weighted YCbCr
    distance (luminance dominates); error diffusion uses plain RGB
    so the propagated error stays colour-true.

    Parameters
    ----------
    tile_rgb     : (H, W, 3) uint8 source RGB for the tile
    palette_rgb  : (K, 3) uint8 local palette (K colours, K <= 15)
    alpha_mask   : (H, W) bool — True for opaque pixels

    Returns
    -------
    (H, W) uint8 of palette indices in 0..K-1 for opaque pixels,
    255 for transparent (caller masks 255 -> 0 after the +1 shift).
    """
    h, w = tile_rgb.shape[:2]
    buf = tile_rgb.astype(np.float32).copy()
    out = np.full((h, w), 255, dtype=np.uint8)

    palette_ycc = rgb_to_weighted_ycbcr(palette_rgb)

    for y in range(h):
        for x in range(w):
            if not alpha_mask[y, x]:
                continue
            current_rgb = np.clip(buf[y, x], 0.0, 255.0)
            current_ycc = rgb_to_weighted_ycbcr(current_rgb[None, :])[0]
            # Perceptually-weighted nearest palette index.
            d2 = np.sum((palette_ycc - current_ycc) ** 2, axis=1)
            best = int(d2.argmin())
            out[y, x] = best
            # Diffuse the RGB error (NOT the weighted error) — keeps
            # propagated tint accurate to source rather than luma-skewed.
            err = current_rgb - palette_rgb[best].astype(np.float32)
            for dy, dx, weight in _FS_KERNEL:
                ny, nx = y + dy, x + dx
                if 0 <= ny < h and 0 <= nx < w and alpha_mask[ny, nx]:
                    buf[ny, nx] += err * weight

    return out


# ---------------------------------------------------------------------------
# Per-tile quantisation
# ---------------------------------------------------------------------------

def _quantize_tile(tile_rgba: np.ndarray,
                    n_colors: int = COLORS_PER_TILE
                    ) -> tuple[np.ndarray, np.ndarray]:
    """
    Quantize a single TILE_SIZE x TILE_SIZE x 4 RGBA tile to a
    `n_colors`-entry local palette using luminance-weighted k-means
    + Floyd-Steinberg.  Returns (indices_uint8_256, palette_15x3_uint8).

    The returned indices array uses 0 for transparent pixels and
    1..n_colors for opaque pixels (the +1 shift is applied here so
    NeoGeo slot 0 = transparency is preserved).  Always returns a
    palette of shape (n_colors, 3) padded with the last-used colour
    if the tile has fewer than n_colors unique opaque samples.
    """
    rgb = tile_rgba[..., :3]
    alpha_mask = tile_rgba[..., 3] >= ALPHA_OPAQUE_THRESHOLD

    # Fully transparent tile: all-zero indices, all-zero palette.
    if not alpha_mask.any():
        return (np.zeros(PIXELS_PER_TILE, dtype=np.uint8),
                np.zeros((n_colors, 3), dtype=np.uint8))

    opaque_rgb = rgb[alpha_mask]
    opaque_ycc = rgb_to_weighted_ycbcr(opaque_rgb)

    centres_ycc = _kmeans_pp_weighted(opaque_ycc, n_clusters=n_colors)

    # Convert YCC centres back to RGB.  Since we only weighted Y,
    # unwinding is just divide-Y-by-LUMA_WEIGHT then inverse matrix.
    centres_ycc_unweighted = centres_ycc.copy()
    centres_ycc_unweighted[..., 0] /= LUMA_WEIGHT
    inv = np.linalg.inv(_M_RGB_YCBCR.astype(np.float64))
    palette_rgb_float = centres_ycc_unweighted @ inv.T.astype(np.float32)
    palette_rgb = np.clip(palette_rgb_float, 0.0, 255.0).astype(np.uint8)

    # Snap to the NeoGeo 5-bit grid so the palette is ROM-storable
    # without further rounding loss.
    palette_rgb = snap_neogeo(palette_rgb)

    # Dither the tile against this snapped local palette.
    raw_indices = _floyd_steinberg_perceptual(rgb, palette_rgb, alpha_mask)

    # +1 shift: NeoGeo hardware reserves slot 0 for transparency.
    # Opaque palette indices are stored as 1..n_colors; transparent
    # samples (marked 255 by the dither helper) become 0.
    shifted = np.where(raw_indices == 255,
                       np.uint8(0),
                       (raw_indices + np.uint8(1)))
    return shifted.reshape(PIXELS_PER_TILE), palette_rgb


# ---------------------------------------------------------------------------
# Top-level conversion
# ---------------------------------------------------------------------------

def _fit_to_tile_grid(img: Image.Image,
                      target_w: int | None,
                      target_h: int | None,
                      fit: str = "contain"
                      ) -> tuple[Image.Image, int, int, int, int]:
    """
    LANCZOS-resize / letterbox `img` onto a canvas whose dimensions
    are multiples of TILE_SIZE.  Returns (canvas, content_left,
    content_top, content_width, content_height).

    target_w / target_h:
      - If both supplied, the canvas is exactly that size (rounded
        down to multiple of 16); image is centred with "contain"
        letterboxing or stretched-cropped with "crop".
      - If either is None, the canvas is the smallest tile-aligned
        size that contains the source untouched.
    """
    img = img.convert("RGBA")
    src_w, src_h = img.size

    if target_w and target_h:
        tw = (target_w // TILE_SIZE) * TILE_SIZE
        th = (target_h // TILE_SIZE) * TILE_SIZE
        if fit.lower() == "crop":
            sw, sh = img.size
            if sh > 0 and sw > 0:
                if (sw / sh) > (tw / th):
                    nw = int(sh * tw / th)
                    left = (sw - nw) // 2
                    img = img.crop((left, 0, left + nw, sh))
                else:
                    nh = int(sw * th / tw)
                    top = (sh - nh) // 2
                    img = img.crop((0, top, sw, top + nh))
            img = img.resize((tw, th), Image.LANCZOS)
            canvas = Image.new("RGBA", (tw, th), (0, 0, 0, 0))
            canvas.alpha_composite(img, (0, 0))
            return canvas, 0, 0, tw, th
        # "contain"
        if img.width > tw or img.height > th:
            img.thumbnail((tw, th), Image.LANCZOS)
        canvas = Image.new("RGBA", (tw, th), (0, 0, 0, 0))
        cl = (tw - img.width) // 2
        ct = (th - img.height) // 2
        canvas.alpha_composite(img, (cl, ct))
        return canvas, cl, ct, img.width, img.height

    # Auto-fit: pad up to next multiple of 16.
    tw = ((src_w + TILE_SIZE - 1) // TILE_SIZE) * TILE_SIZE
    th = ((src_h + TILE_SIZE - 1) // TILE_SIZE) * TILE_SIZE
    canvas = Image.new("RGBA", (tw, th), (0, 0, 0, 0))
    cl = (tw - src_w) // 2
    ct = (th - src_h) // 2
    canvas.alpha_composite(img, (cl, ct))
    return canvas, cl, ct, src_w, src_h


def convert_png_to_neogeo_tile_palettes(
        image_path: str | os.PathLike,
        target_w: int | None = 320,
        target_h: int | None = 224,
        fit: str = "contain",
        n_colors_per_tile: int = COLORS_PER_TILE,
        ) -> tuple[np.ndarray, np.ndarray, dict]:
    """
    Run the full spatially-localised conversion on a PNG.

    Returns
    -------
    indices_flat : ndarray (n_tiles, 256) uint8, values in {0..15}
                    (0 = transparent, 1..15 = palette entry).
    palettes     : ndarray (n_tiles, n_colors_per_tile, 3) uint8 —
                   RGB triples already snapped to the NeoGeo 5-bit grid.
                   For the 45-byte-per-tile flat contract, use
                   `palettes.reshape(n_tiles, -1)`.
    metadata     : dict — canvas geometry + content rect + tile grid.
    """
    img = Image.open(image_path).convert("RGBA")
    canvas, cl, ct, cw, ch = _fit_to_tile_grid(img, target_w, target_h, fit)
    arr = np.array(canvas, dtype=np.uint8)
    canvas_h, canvas_w = arr.shape[:2]

    tile_cols = canvas_w // TILE_SIZE
    tile_rows = canvas_h // TILE_SIZE
    n_tiles = tile_cols * tile_rows

    indices_flat = np.zeros((n_tiles, PIXELS_PER_TILE), dtype=np.uint8)
    palettes = np.zeros((n_tiles, n_colors_per_tile, 3), dtype=np.uint8)

    # Raster scan: row-major (top-to-bottom rows, left-to-right within).
    for ty in range(tile_rows):
        for tx in range(tile_cols):
            y0 = ty * TILE_SIZE
            x0 = tx * TILE_SIZE
            tile_rgba = arr[y0:y0 + TILE_SIZE, x0:x0 + TILE_SIZE]
            t_idx = ty * tile_cols + tx
            tile_indices, tile_palette = _quantize_tile(tile_rgba,
                                                        n_colors_per_tile)
            indices_flat[t_idx] = tile_indices
            palettes[t_idx] = tile_palette

    metadata = {
        "tile_cols":      int(tile_cols),
        "tile_rows":      int(tile_rows),
        "n_tiles":        int(n_tiles),
        "canvas_w":       int(canvas_w),
        "canvas_h":       int(canvas_h),
        "content_left":   int(cl),
        "content_top":    int(ct),
        "content_width":  int(cw),
        "content_height": int(ch),
        "target_w":       int(canvas_w),
        "target_h":       int(canvas_h),
        "tile_size":      int(TILE_SIZE),
        "colors_per_tile": int(n_colors_per_tile),
    }
    return indices_flat, palettes, metadata


# ---------------------------------------------------------------------------
# Dedup-friendly entry point — structured per-tile records
# ---------------------------------------------------------------------------

def extract_tile_data_for_dedup(
        image_path: str | os.PathLike,
        target_w: int | None = 320,
        target_h: int | None = 224,
        fit: str = "contain",
        n_colors_per_tile: int = COLORS_PER_TILE,
        ) -> tuple[list[dict], dict[int, bytes], dict]:
    """
    Run the localized quantizer and return its output in the shape a
    palette-deduplication pass actually wants to consume.

    Designed as the "unzipped" entry point for a global palette-merging
    algorithm — every tile is a self-contained record carrying its
    spatial coordinates AND its uncompressed RGB LUT, so the dedup
    pass can score (tile_a.palette, tile_b.palette) pairwise without
    having to map back through array indices.

    Returns
    -------
    tiles_data : list of dicts, one entry per tile in raster-scan
                 order (top-to-bottom rows, left-to-right within).
                 Each entry has:

                   {
                     "tile_index": int,           # 0..n_tiles-1
                     "tile_col":   int,           # x in tile units
                     "tile_row":   int,           # y in tile units
                     "pixel_x":    int,           # x in canvas pixels
                     "pixel_y":    int,           # y in canvas pixels
                     "indices":    np.ndarray,    # (256,) uint8, 0..15
                                                  #   0 = transparent
                                                  #   1..15 = palette slot
                     "indices_bytes": bytes,      # same as above, 256 bytes
                   }

    raw_local_palettes : dict[int, bytes]
                 Map of tile_index -> 45-byte raw RGB palette chunk
                 (15 colours * 3 bytes RGB, big-endian R/G/B per entry).
                 Bytes ordering matches NeoGeo palette-RAM layout
                 (palette entry k's RGB at bytes [k*3 : k*3+3]).
                 Caller can either iterate `tiles_data` and look up
                 via `tile_index`, or zip the two structures.

    metadata : dict — canvas geometry, tile grid, content rect.
               Use this to drive the dedup pass's bank budget.

    The returned ndarrays inside tiles_data share memory with the
    backing batch array — read-only consumers are safe; downstream
    code that mutates indices should `.copy()` first.
    """
    indices_flat, palettes, meta = convert_png_to_neogeo_tile_palettes(
        image_path,
        target_w=target_w,
        target_h=target_h,
        fit=fit,
        n_colors_per_tile=n_colors_per_tile,
    )

    n_tiles = meta["n_tiles"]
    tile_cols = meta["tile_cols"]

    tiles_data: list[dict] = []
    raw_local_palettes: dict[int, bytes] = {}

    for ti in range(n_tiles):
        ty, tx = divmod(ti, tile_cols)
        indices = indices_flat[ti]                       # (256,) uint8
        tiles_data.append({
            "tile_index":    int(ti),
            "tile_col":      int(tx),
            "tile_row":      int(ty),
            "pixel_x":       int(tx * TILE_SIZE),
            "pixel_y":       int(ty * TILE_SIZE),
            "indices":       indices,
            "indices_bytes": indices.tobytes(),
        })
        # 45-byte RGB chunk: 15 colours * 3 bytes each, R/G/B/R/G/B/...
        raw_local_palettes[int(ti)] = palettes[ti].tobytes()

    return tiles_data, raw_local_palettes, meta


# ---------------------------------------------------------------------------
# Greedy palette-bank packer with smart pixel remap
# ---------------------------------------------------------------------------

def cluster_and_remap_tile_palettes(
        indices_per_tile: np.ndarray,
        palettes_per_tile: np.ndarray,
        max_banks: int = 32,
        epsilon: float = 15.0,
        ) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """
    Greedy MAE clustering of N per-tile palettes into <= max_banks
    shared hardware banks, followed by a perceptually-correct LOCAL
    -> BANK pixel-index remap.

    Why the remap matters: the per-tile dither produces indices
    0..15 that reference each tile's OWN local palette.  Just
    assigning the tile to a shared bank without remapping leaves
    those indices pointing at the wrong slot inside the bank — the
    Floyd-Steinberg pattern is preserved but the colours are
    nonsense.  This pass builds a local -> bank slot table per tile
    (Lab-nearest match for each of the 15 local colours against the
    15 bank colours) and translates the dithered indices through it,
    so the dither texture is preserved while colours land at the
    bank's nearest perceptual neighbour.

    Index-0 transparency is preserved unchanged.

    Parameters
    ----------
    indices_per_tile : (N, 256) uint8, values in 0..15
                       (0 = transparent, 1..15 = local palette slot)
    palettes_per_tile : (N, 15, 3) uint8 NeoGeo-grid RGB
    max_banks         : hard ceiling on emitted shared banks
    epsilon           : MAE threshold for "close enough to existing
                        bank".  Lower = more banks, higher fidelity;
                        higher = fewer banks, more colour compromise.

    Returns
    -------
    remapped_indices : (N, 256) uint8 — same layout as input, but
                       indices 1..15 now reference the assigned
                       bank's palette (not the local palette).
    bank_palettes    : (n_banks, 15, 3) uint8 — shared bank LUTs.
                       n_banks is <= max_banks.
    tile_to_bank     : (N,) uint16 — bank index per tile.
    """
    if indices_per_tile.shape[0] != palettes_per_tile.shape[0]:
        raise ValueError("indices/palettes tile-count mismatch")

    n_tiles = palettes_per_tile.shape[0]
    pal_f32 = palettes_per_tile.astype(np.float32)

    # --- 1. Greedy MAE clustering with auto-scaling epsilon ----------
    # The eps tolerance for "close enough to existing bank" is the
    # tuning knob: too low and we run out of bank budget before all
    # tiles are clustered (forcing a brute-force "nearest existing
    # bank" fallback that shatters colours on the unlucky overflow
    # tiles); too high and dissimilar palettes get merged together.
    # Rather than relying on a hand-picked constant, retry the whole
    # clustering with progressively wider eps until the result fits
    # under max_banks WITHOUT triggering the fallback for any tile.
    # Geometric growth (eps *= 1.5) converges in <8 attempts even on
    # extremely colourful sources.
    cur_eps = float(epsilon)
    bank_palettes_arr = None
    tile_to_bank = None
    eps_history = [cur_eps]
    for _attempt in range(8):
        bank_palettes: list[np.ndarray] = []
        candidate_assignments = np.full(n_tiles, -1, dtype=np.int32)
        overflowed = False
        for ti in range(n_tiles):
            pal = pal_f32[ti]
            matched = -1
            for bi, bank in enumerate(bank_palettes):
                mae = float(np.mean(np.abs(pal - bank)))
                if mae < cur_eps:
                    matched = bi
                    break
            if matched == -1:
                if len(bank_palettes) < max_banks:
                    bank_palettes.append(pal.copy())
                    matched = len(bank_palettes) - 1
                else:
                    overflowed = True
                    break
            candidate_assignments[ti] = matched
        if not overflowed:
            # Fits in budget — accept this clustering.
            bank_palettes_arr = np.stack(bank_palettes,
                                          axis=0).astype(np.uint8)
            bank_palettes_arr = snap_neogeo(bank_palettes_arr)
            tile_to_bank = candidate_assignments.astype(np.uint16)
            break
        # Overflow: widen tolerance and retry the entire pass.
        cur_eps *= 1.5
        eps_history.append(cur_eps)
    if bank_palettes_arr is None:
        # 8 attempts and still overflowing.  Fall back to the brute-
        # force nearest-bank assignment with the last attempted eps;
        # at least colours stay close to their original cluster.
        bank_palettes_arr = np.stack(bank_palettes,
                                      axis=0).astype(np.uint8)
        bank_palettes_arr = snap_neogeo(bank_palettes_arr)
        tile_to_bank = np.zeros(n_tiles, dtype=np.uint16)
        for ti in range(n_tiles):
            if candidate_assignments[ti] != -1:
                tile_to_bank[ti] = candidate_assignments[ti]
                continue
            pal = pal_f32[ti]
            dists = np.array([
                float(np.mean(np.abs(pal - b.astype(np.float32))))
                for b in bank_palettes_arr
            ])
            tile_to_bank[ti] = int(dists.argmin())

    # --- 2. Smart LOCAL -> BANK pixel remap --------------------------
    # For each tile, pre-compute a 15-entry lookup that maps each
    # local palette slot (0..14) to its nearest match in the assigned
    # bank (also 0..14).  Lab distance ensures perceptual fidelity.
    remapped = np.zeros_like(indices_per_tile)

    # Cache the bank palettes' Lab once — Lab conversion is the
    # expensive step.
    bank_lab_cache = {
        bi: rgb_to_lab(bank_palettes_arr[bi])
        for bi in range(bank_palettes_arr.shape[0])
    }

    for ti in range(n_tiles):
        local_pal = palettes_per_tile[ti]                  # (15, 3) u8
        assigned_bank = int(tile_to_bank[ti])

        local_lab = rgb_to_lab(local_pal)                  # (15, 3) f32
        bank_lab = bank_lab_cache[assigned_bank]           # (15, 3) f32

        # (15, 15) squared distance matrix — local i to bank j.
        d2 = np.sum((local_lab[:, None, :] - bank_lab[None, :, :]) ** 2,
                    axis=2)
        # local_to_bank[k] = bank slot best matching local slot k.
        # Both are in 0..14 (i.e. before the +1 transparency shift).
        local_to_bank = d2.argmin(axis=1).astype(np.uint8)

        tile_indices = indices_per_tile[ti]                # (256,) u8
        opaque = tile_indices != 0
        out_tile = np.zeros_like(tile_indices)
        # For opaque pixels: index = local_to_bank[(idx - 1)] + 1.
        # The +1 shift preserves slot 0 = transparency on both sides.
        out_tile[opaque] = local_to_bank[tile_indices[opaque] - 1] + 1
        remapped[ti] = out_tile

    return remapped, bank_palettes_arr, tile_to_bank


# ---------------------------------------------------------------------------
# Single-palette derivation for single-bank downstream pipelines
# ---------------------------------------------------------------------------

def derive_global_palette_from_tile_palettes(
        palettes: np.ndarray,
        k: int = COLORS_PER_TILE,
        ) -> np.ndarray:
    """
    Collapse N tile-local palettes (shape (N, 15, 3)) into a single
    high-quality global k-colour palette by Lab-space k-means++ on
    the WEIGHTED union of every tile palette's entries.

    Why this beats naive whole-image k-means:
      - The tile-local pass already de-noised the input by clustering
        within each 16x16 macroblock, so colours that only appear in
        one tile aren't drowned out by the dominant background.
      - Each tile contributes 15 candidate colours.  Identical colours
        across many tiles accumulate "weight" (i.e. they count more
        in the cluster updates), so frequently-used hues are
        well-represented in the final palette.

    Returns (k, 3) uint8 snapped to the NeoGeo 5-bit grid.
    """
    # Flatten (N, 15, 3) -> (N*15, 3), drop fully-transparent (all-zero)
    # palette slots — they're padding from empty tiles.
    flat = palettes.reshape(-1, 3)
    nonblack_mask = flat.sum(axis=1) > 0
    if not nonblack_mask.any():
        return np.zeros((k, 3), dtype=np.uint8)
    flat = flat[nonblack_mask]

    # Weight every candidate by how often it appears across tiles.
    # np.unique with return_counts gives us this for free.
    unique_colors, counts = np.unique(flat, axis=0, return_counts=True)

    if len(unique_colors) <= k:
        # Already fits in the budget — pad to k by repeating the
        # most-frequent colour so the LUT is fixed-size.
        out = np.zeros((k, 3), dtype=np.uint8)
        out[:len(unique_colors)] = unique_colors
        if len(unique_colors) < k:
            most_common = unique_colors[counts.argmax()]
            out[len(unique_colors):] = most_common
        return snap_neogeo(out)

    # k-means++ in Lab space, sample-weighted by `counts`.
    unique_lab = rgb_to_lab(unique_colors)
    rng = np.random.default_rng(0)

    centres = np.empty((k, 3), dtype=np.float32)
    weights = counts.astype(np.float32)
    probs = weights / weights.sum()
    centres[0] = unique_lab[rng.choice(len(unique_lab), p=probs)]
    closest_d2 = np.sum((unique_lab - centres[0]) ** 2, axis=1)
    for i in range(1, k):
        biased = closest_d2 * weights
        total = biased.sum()
        if total <= 1e-12:
            centres[i] = unique_lab[rng.integers(len(unique_lab))]
        else:
            centres[i] = unique_lab[rng.choice(len(unique_lab),
                                                 p=biased / total)]
        new_d2 = np.sum((unique_lab - centres[i]) ** 2, axis=1)
        closest_d2 = np.minimum(closest_d2, new_d2)

    # Lloyd iterations, weighted by counts.
    for _ in range(25):
        d2 = np.sum((unique_lab[:, None, :] - centres[None, :, :]) ** 2,
                    axis=2)
        labels = d2.argmin(axis=1)
        moved = False
        new_centres = centres.copy()
        for j in range(k):
            mask = labels == j
            if mask.any():
                w = weights[mask][:, None]
                wsum = w.sum()
                if wsum > 0:
                    new_mean = (unique_lab[mask] * w).sum(axis=0) / wsum
                    if np.any(np.abs(new_mean - centres[j]) > 1e-3):
                        moved = True
                    new_centres[j] = new_mean
        centres = new_centres
        if not moved:
            break

    # Lab -> RGB -> snap.  Reuse img2neo.lab_to_rgb to avoid pulling
    # skimage; import locally so the rest of the module is unaffected
    # if img2neo isn't available.
    from img2neo import lab_to_rgb
    palette_rgb = snap_neogeo(lab_to_rgb(centres))
    return palette_rgb


def _floyd_steinberg_global(rgb: np.ndarray,
                            palette: np.ndarray,
                            alpha_mask: np.ndarray) -> np.ndarray:
    """
    Floyd-Steinberg dither a whole image against a single global
    palette using RGB Euclidean nearest-colour lookup.

    The palette was DERIVED in CIE-Lab via the context-window
    k-means in step 2/3 of convert_screen_via_tile_palette, so it
    already spans hues perceptually — RGB distance against a
    Lab-derived palette is the standard approach (see
    img2neo.floyd_steinberg) and avoids the per-pixel rgb_to_lab
    call that pushed the inner loop from 0.6s to 4s per 320x224
    screen.  Earlier luma-weighted YCbCr lookup was the buggy
    middle path that produced the sepia collapse.

    Returns (H, W) uint8 of palette indices 0..K-1; caller masks
    transparent pixels separately.
    """
    h, w = rgb.shape[:2]
    buf = rgb.astype(np.float32).copy()
    out = np.zeros((h, w), dtype=np.uint8)
    palette_f = palette.astype(np.float32)

    for y in range(h):
        for x in range(w):
            if not alpha_mask[y, x]:
                continue
            current = np.clip(buf[y, x], 0.0, 255.0)
            d2 = np.sum((palette_f - current) ** 2, axis=1)
            best = int(d2.argmin())
            out[y, x] = best
            err = current - palette_f[best]
            for dy, dx, weight in _FS_KERNEL:
                ny, nx = y + dy, x + dx
                if 0 <= ny < h and 0 <= nx < w and alpha_mask[ny, nx]:
                    buf[ny, nx] += err * weight
    return out


# ---------------------------------------------------------------------------
# Consolidated "vivid" pipeline — per-tile CIE-Lab K-Means clustering
# + per-tile RGB-Euclidean Floyd-Steinberg
# ---------------------------------------------------------------------------
#
# Per the user's specified architecture.  Two-step solution:
#   STEP 1: Convert each 16x16 tile to CIE-Lab and run k-means++ with
#           uniform-input guards.  Perceptually-balanced Lab clustering
#           avoids the luma-weighted YCbCr "sepia collapse" bug.
#   STEP 2: Convert the resulting 15-colour palette back to RGB and dither
#           the tile with a fast RGB-Euclidean Floyd-Steinberg loop.
#
# The downstream adapter `convert_screen_via_vivid_pipeline` collapses
# the per-tile output into the single-bank (indexed, palette16, meta)
# shape that `romdbimgimport.load_screen_asset` consumes.

def _kmeans_pp_weighted_local(data: np.ndarray,
                               k: int,
                               max_iters: int = 20,
                               rng_seed: int = 0) -> np.ndarray:
    """
    K-Means++ initialisation and Lloyd's clustering with a uniform-input
    guard.  Operates on (N, 3) float features in any colour space
    (caller decides — Lab for the vivid pipeline).  Returns (k, 3)
    cluster centres in the same feature space.

    Uniform-input guard prevents the numerical-probability collapse
    that crashes the standard implementation on flat colour zones
    (clear-sky tiles, blank fills): if the input's max - min spread
    is below 1e-5 across all axes, every centre is set to the input's
    single colour.
    """
    n = data.shape[0]
    if n == 0:
        return np.zeros((k, 3), dtype=np.float32)
    if float(np.max(data) - np.min(data)) < 1e-5:
        return np.repeat(data[0:1], k, axis=0).astype(np.float32)
    if n <= k:
        out = np.zeros((k, 3), dtype=np.float32)
        out[:n] = data
        out[n:] = data[-1]
        return out

    rng = np.random.default_rng(rng_seed)
    centres = np.empty((k, 3), dtype=np.float32)
    centres[0] = data[rng.integers(n)]
    closest_d2 = np.sum((data - centres[0]) ** 2, axis=1)
    for i in range(1, k):
        total = closest_d2.sum()
        if total <= 1e-12:
            centres[i] = data[rng.integers(n)]
        else:
            probs = closest_d2 / total
            centres[i] = data[rng.choice(n, p=probs)]
        new_d2 = np.sum((data - centres[i]) ** 2, axis=1)
        closest_d2 = np.minimum(closest_d2, new_d2)

    for _ in range(max_iters):
        d2 = np.sum((data[:, None, :] - centres[None, :, :]) ** 2, axis=2)
        labels = d2.argmin(axis=1)
        moved = False
        new_centres = centres.copy()
        for j in range(k):
            mask = labels == j
            if mask.any():
                mean = data[mask].mean(axis=0)
                if np.any(np.abs(mean - centres[j]) > 1e-3):
                    moved = True
                new_centres[j] = mean
        centres = new_centres
        if not moved:
            break
    return centres


def process_vivid_artbox_pipeline(image_path,
                                   target_w: int = 320,
                                   target_h: int = 224
                                   ) -> tuple[list, list]:
    """
    Per the user's specified architecture: per-tile CIE-Lab K-Means
    palette generation + per-tile RGB-Euclidean Floyd-Steinberg loop.

    Returns
    -------
    ready_tiles : list of 256-int lists, one per tile, values in 1..15
                  (Neo Geo +1 transparency-bypass shift already applied).
    final_palettes : list of 45-int lists, one per tile, holding the
                     15 RGB palette colours flat (R,G,B,R,G,B,...).
    """
    src = Image.open(image_path).convert("RGB")
    src = src.resize((target_w, target_h), Image.Resampling.LANCZOS)
    src_np = np.array(src, dtype=np.float32)

    ready_tiles = []
    final_palettes = []

    for y in range(0, target_h, 16):
        for x in range(0, target_w, 16):
            tile_rgb = src_np[y:y + 16, x:x + 16]

            # STEP 1: per-tile CIE-Lab k-means++ palette generation.
            tile_pil = Image.fromarray(tile_rgb.astype(np.uint8))
            tile_lab = np.array(
                tile_pil.convert("LAB"), dtype=np.float32
            ).reshape(-1, 3)
            lab_centres = _kmeans_pp_weighted_local(tile_lab, 15)

            # Lab -> RGB via PIL ICC so the dither loop sees displayable
            # RGB values for nearest-colour lookup.
            palette_img = Image.fromarray(
                lab_centres.reshape(1, 15, 3).astype(np.uint8), mode="LAB"
            )
            rgb_palette = np.array(
                palette_img.convert("RGB")
            ).reshape(15, 3).astype(np.float32)

            # STEP 2: per-tile RGB-Euclidean Floyd-Steinberg dither.
            output_indices = np.zeros((16, 16), dtype=np.uint8)
            error_buffer = np.zeros((18, 18, 3), dtype=np.float32)
            error_buffer[1:17, 1:17, :] = tile_rgb.copy()
            for ty in range(16):
                for tx in range(16):
                    px = error_buffer[ty + 1, tx + 1, :]
                    distances = np.sum((rgb_palette - px) ** 2, axis=1)
                    best_idx = int(np.argmin(distances))
                    output_indices[ty, tx] = best_idx
                    quant_error = px - rgb_palette[best_idx]
                    error_buffer[ty + 1, tx + 2, :] += quant_error * (7.0 / 16.0)
                    error_buffer[ty + 2, tx,     :] += quant_error * (3.0 / 16.0)
                    error_buffer[ty + 2, tx + 1, :] += quant_error * (5.0 / 16.0)
                    error_buffer[ty + 2, tx + 2, :] += quant_error * (1.0 / 16.0)

            hardware_tile = output_indices + 1
            ready_tiles.append(hardware_tile.flatten().tolist())
            final_palettes.append(rgb_palette.astype(np.uint8).flatten().tolist())

    return ready_tiles, final_palettes


def _kmeans_pp_weighted_vivid(data: np.ndarray, k: int) -> np.ndarray:
    """
    K-Means++ initialization with an absolute variance guard to prevent
    numerical collapse on uniform flat-colour zones.

    Per the brief, the implementation prefers sklearn's MiniBatchKMeans
    (k-means++ init, 3 restarts, deterministic seed) when sklearn is
    available.  If sklearn isn't installed in the build environment,
    we fall back to the local numpy k-means++ already in this module
    so the artbox pipeline stays buildable on minimal toolchains.
    """
    data = np.asarray(data, dtype=np.float32)
    if float(np.max(data) - np.min(data)) < 1e-4:
        return np.repeat(data[0:1], k, axis=0).astype(np.float32)
    try:
        from sklearn.cluster import MiniBatchKMeans
        kmeans = MiniBatchKMeans(n_clusters=k, init='k-means++',
                                  n_init=3, random_state=42)
        return kmeans.fit(data).cluster_centers_.astype(np.float32)
    except ImportError:
        # No sklearn — use the equivalent local k-means++ implementation
        # already present in this module.  Same return shape (k, 3).
        return _kmeans_pp_weighted_local(data, k)


def execute_final_vivid_pipeline(image_path,
                                   target_w: int = 320,
                                   target_h: int = 224
                                   ) -> tuple[list, list]:
    """
    Decoupled Spatial Processing pipeline — corrects the Z-Order
    Traversal Bug that produced deep-fried high-frequency noise in
    earlier per-tile-loop iterations, with full alpha-channel
    preservation so sprites with transparent regions don't render
    as solid coloured blocks.

      Pass 1 — Pre-calculate all per-tile CIE-Lab palettes.  Each
               16x16 tile is converted to PIL "LAB"; only OPAQUE
               pixels feed the k-means clusterer (alpha >= 128
               threshold).  Fully-transparent tiles emit a zero
               palette.  Uniform-input guard at spread < 1e-4;
               sklearn MiniBatchKMeans where available, local
               numpy k-means++ fallback otherwise.

      Pass 2 — GLOBAL SCANLINE Floyd-Steinberg with alpha skip.
               A single continuous (gy, gx) sweep with per-pixel
               palette hot-swap via (gy // 16, gx // 16).
               Transparent pixels short-circuit straight to the
               transparency sentinel (255) without consuming a
               palette slot or diffusing error.  Opaque pixels
               get the textbook FS treatment + 0.85 leak-dampener.

      Pass 3 — Slice into per-tile blocks.  Opaque pixels apply the
               NeoGeo +1 transparency shift (so they end up in
               slots 1..15).  Transparent pixels become 0.

    Hardware contract caveat
    ------------------------
    The returned `final_palettes` list contains ONE 15-colour palette
    per tile — i.e. up to (target_w/16) * (target_h/16) entries
    (280 for a 320x224 screen).  Neo Geo has 256 palette banks of
    16 colours TOTAL in palette RAM.  This raw output is NOT
    hardware-safe by itself.  Run it through
    `cluster_and_remap_tile_palettes()` (greedy MAE clustering) or
    the convert_screen_via_vivid_pipeline adapter (single-palette
    collapse) BEFORE feeding to romtiles / genscreens.

    Returns
    -------
    ready_tiles    : list of flat 256-int lists, values 0..15
                     (0 = transparent, 1..15 = palette slot).
    final_palettes : list of flat 45-int lists, 15 RGB triples each.
    """
    src = Image.open(image_path).convert("RGBA")
    src = src.resize((target_w, target_h), Image.Resampling.LANCZOS)
    src_np = np.array(src, dtype=np.float32)
    rgb_np = src_np[:, :, :3]
    alpha_np = src_np[:, :, 3]
    opaque_mask = alpha_np >= 128.0

    # ---------------------------------------------------------
    # PASS 1: PRE-CALCULATE ALL LOCAL PALETTES
    # Alpha-aware: only opaque pixels feed the clusterer.  Fully-
    # transparent tiles emit a zero palette (irrelevant — those
    # pixels short-circuit in Pass 2 anyway).
    # ---------------------------------------------------------
    local_luts = {}
    for ty_idx in range(target_h // 16):
        for tx_idx in range(target_w // 16):
            y, x = ty_idx * 16, tx_idx * 16
            tile_rgb = rgb_np[y:y + 16, x:x + 16]
            tile_alpha = opaque_mask[y:y + 16, x:x + 16]

            if not tile_alpha.any():
                # Fully transparent tile — no opaque pixels to cluster.
                local_luts[(ty_idx, tx_idx)] = np.zeros((15, 3),
                                                          dtype=np.float32)
                continue

            opaque_rgb = tile_rgb[tile_alpha].astype(np.uint8)
            # PIL LAB conversion needs a 2D image, not a flat list.
            # Pack opaque pixels into an Nx1 strip for conversion.
            opaque_strip = opaque_rgb.reshape(-1, 1, 3)
            opaque_pil = Image.fromarray(opaque_strip, mode="RGB")
            opaque_lab = np.array(
                opaque_pil.convert("LAB"), dtype=np.float32
            ).reshape(-1, 3)

            lab_centres = _kmeans_pp_weighted_vivid(opaque_lab, 15)
            palette_img = Image.fromarray(
                lab_centres.reshape(1, 15, 3).astype(np.uint8), mode="LAB"
            )
            local_luts[(ty_idx, tx_idx)] = np.array(
                palette_img.convert("RGB")
            ).reshape(15, 3).astype(np.float32)

    # ---------------------------------------------------------
    # PASS 2: GLOBAL SCANLINE DITHER (alpha-aware)
    # ---------------------------------------------------------
    # Pad right + bottom by 1 so the FS kernel never indexes past the
    # screen edge for the final row / column.
    global_error_canvas = np.pad(rgb_np, ((0, 1), (0, 1), (0, 0)),
                                   mode='edge')
    global_indices = np.full((target_h, target_w), 255, dtype=np.uint8)
    # Sentinel 255 = transparent.  Opaque pixels get 0..14, then the
    # +1 shift in Pass 3 maps them to NeoGeo slots 1..15.

    for gy in range(target_h):
        for gx in range(target_w):
            # Alpha skip: transparent source pixels short-circuit
            # straight to the transparency sentinel.  No palette
            # lookup, no error diffusion — error from neighbours
            # propagating INTO a transparent pixel just sits in the
            # canvas, which is fine since we never read it.
            if not opaque_mask[gy, gx]:
                continue

            ty_idx = gy // 16
            tx_idx = gx // 16
            active_palette = local_luts[(ty_idx, tx_idx)]

            # Teflon Routing for UI + line art.
            # Separate the colour-MATCHING pixel from the error-CARRYING
            # pixel.  Earlier "zero the outgoing error" trap broke the
            # Floyd-Steinberg wavefront everywhere line art was drawn:
            # the wave couldn't cross 16x16 tile boundaries, so per-tile
            # palette seams reappeared.
            #
            # Routing:
            #   - When the ORIGINAL source pixel is near-pure-white
            #     (luma > 240) or near-pure-black (luma < 15) — bubble
            #     fills, text glyphs, panel outlines — the nearest-
            #     palette lookup uses the pristine source RGB so the
            #     pixel snaps to the actual white/black palette entry
            #     instead of absorbing the upstream FS bleed.
            #   - The quant_error is ALWAYS computed against the
            #     error-laden `px` (the global-canvas value).  This
            #     keeps the FS wave's mathematical energy intact: the
            #     bubble/line acts as Teflon — the wave flows through
            #     uninterrupted, bouncing the accumulated error forward
            #     to the next pixel where it belongs.
            px = np.clip(global_error_canvas[gy, gx, :], 0.0, 255.0)
            orig_px = rgb_np[gy, gx]
            orig_luma = (orig_px[0] * 0.299
                          + orig_px[1] * 0.587
                          + orig_px[2] * 0.114)
            if orig_luma > 240.0 or orig_luma < 15.0:
                match_px = np.clip(orig_px, 0.0, 255.0)
            else:
                match_px = px

            distances = np.sum((active_palette - match_px) ** 2, axis=1)
            best_idx = int(np.argmin(distances))
            global_indices[gy, gx] = best_idx

            # 0.85 leak-dampener prevents runaway speckle compounding.
            # Error is the difference between the ERROR-LADEN px and
            # the chosen palette entry — never between match_px (which
            # would silently delete energy at trap pixels).
            quant_error = (px - active_palette[best_idx]) * 0.85

            global_error_canvas[gy, gx + 1, :] += quant_error * (7.0 / 16.0)
            if gx > 0:
                global_error_canvas[gy + 1, gx - 1, :] += quant_error * (3.0 / 16.0)
            global_error_canvas[gy + 1, gx,     :] += quant_error * (5.0 / 16.0)
            global_error_canvas[gy + 1, gx + 1, :] += quant_error * (1.0 / 16.0)

    # ---------------------------------------------------------
    # PASS 3: SLICE BACK TO NEO GEO HARDWARE BLOCKS
    # Transparent sentinel 255 -> NeoGeo slot 0; opaque 0..14 -> 1..15.
    # ---------------------------------------------------------
    # Pre-allocate output lists by exact tile count to avoid
    # repeated list.append() reallocations inside the inner loop —
    # tile count is known upfront from the target dimensions.
    tile_rows = target_h // 16
    tile_cols = target_w // 16
    n_tiles = tile_rows * tile_cols
    ready_tiles: list = [None] * n_tiles
    final_palettes: list = [None] * n_tiles
    for ty_idx in range(tile_rows):
        for tx_idx in range(tile_cols):
            tile_block = global_indices[ty_idx * 16:(ty_idx + 1) * 16,
                                          tx_idx * 16:(tx_idx + 1) * 16]
            transparent = tile_block == 255
            shifted = np.where(transparent,
                               np.uint8(0),
                               (tile_block + 1).astype(np.uint8))
            slot = ty_idx * tile_cols + tx_idx
            ready_tiles[slot] = shifted.flatten().tolist()
            final_palettes[slot] = (
                local_luts[(ty_idx, tx_idx)].astype(np.uint8).flatten().tolist()
            )

    return ready_tiles, final_palettes


# ---------------------------------------------------------------------------
# Hash-keyed cache for the vivid screen converter
# ---------------------------------------------------------------------------
# Per-tile FS over 121+ images during `make all` is the dominant cost of a
# full rebuild.  Most images don't change between builds, so SHA256 the
# source + params, persist the (indexed, palette16, meta) tuple, and skip
# the recompute on cache hit.  Cache lives under artbox/.cache/vivid/ so
# it stays inside the repo dir tree (gitignored).

_VIVID_CACHE_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), ".cache", "vivid")
_VIVID_CACHE_VERSION = "v6-scanline-alpha-teflon"


def _vivid_cache_key(image_path: str, target_w: int, target_h: int,
                      fit: str, anchor: str, n_colors: int) -> str:
    """SHA256 of source bytes + params + pipeline version tag."""
    import hashlib
    h = hashlib.sha256()
    try:
        with open(image_path, "rb") as f:
            while True:
                chunk = f.read(64 * 1024)
                if not chunk:
                    break
                h.update(chunk)
    except OSError:
        return ""
    h.update(f"|{target_w}|{target_h}|{fit}|{anchor}|{n_colors}|"
              f"{_VIVID_CACHE_VERSION}".encode())
    return h.hexdigest()


def _vivid_cache_load(key: str):
    if not key:
        return None
    path = os.path.join(_VIVID_CACHE_DIR, f"{key}.npz")
    if not os.path.exists(path):
        return None
    try:
        d = np.load(path, allow_pickle=True)
        indexed = d["indexed"]
        palette16 = d["palette16"]
        meta = d["meta"].item()
        return indexed, palette16, meta
    except (OSError, KeyError, ValueError):
        return None


def _vivid_cache_save(key: str, indexed: np.ndarray,
                       palette16: np.ndarray, meta: dict):
    if not key:
        return
    try:
        os.makedirs(_VIVID_CACHE_DIR, exist_ok=True)
        path = os.path.join(_VIVID_CACHE_DIR, f"{key}.npz")
        np.savez(path, indexed=indexed, palette16=palette16,
                 meta=np.array(meta, dtype=object))
    except OSError:
        pass


def convert_screen_via_vivid_pipeline(
        image_path,
        target_w: int = 320,
        target_h: int = 224,
        fit: str = "contain",
        anchor: str = "center",
        n_colors: int = COLORS_PER_TILE,
        **kwargs,
        ) -> tuple[np.ndarray, np.ndarray, dict]:
    """
    Adapter that runs `process_vivid_artbox_pipeline` and shapes its
    per-tile output into the (indexed_uint16, palette16_uint16,
    metadata) contract required by `romdbimgimport.load_screen_asset`.

    Pipeline:
      1. process_vivid_artbox_pipeline -> per-tile indices + palettes.
      2. Reconstruct a per-pixel RGB canvas from those per-tile
         outputs (so colour information from each tile's local
         palette survives into the global pass).
      3. Derive ONE 15-colour global palette via Lab-space k-means++
         over the union of every per-tile palette's colours.
      4. Re-quantise the reconstructed canvas against the global
         palette with the same RGB-Euclidean Floyd-Steinberg used in
         the per-tile loop (consistent dither character).
    """
    target_w = (target_w // 16) * 16
    target_h = (target_h // 16) * 16

    # Cache hot-path: SHA256 of source bytes + params + version tag.
    cache_key = _vivid_cache_key(str(image_path), target_w, target_h,
                                   fit, anchor, n_colors)
    cached = _vivid_cache_load(cache_key)
    if cached is not None:
        return cached

    # 3-Pass Context-Insulated Quantizer: Lab kmeans palettes per tile
    # + unified global error matrix (no per-tile FS reset = no seams)
    # + scanline FS with alpha skip.
    ready_tiles, final_palettes = execute_final_vivid_pipeline(
        image_path, target_w, target_h)

    tile_cols = target_w // 16
    tile_rows = target_h // 16
    n_tiles = len(ready_tiles)

    # 2. Reconstruct the per-pixel RGB canvas + alpha mask from the
    # per-tile output.  Transparent pixels (index 0) propagate the
    # transparency through the final global FS pass.
    canvas_rgb = np.zeros((target_h, target_w, 3), dtype=np.uint8)
    alpha_mask = np.zeros((target_h, target_w), dtype=bool)
    for ti in range(n_tiles):
        ty, tx = divmod(ti, tile_cols)
        y0, x0 = ty * 16, tx * 16
        indices_u8 = np.array(ready_tiles[ti], dtype=np.uint8).reshape(16, 16)
        palette_u8 = np.array(final_palettes[ti], dtype=np.uint8).reshape(15, 3)
        opaque_tile = indices_u8 != 0
        idx_local = (indices_u8.astype(np.int16) - 1).clip(0, 14).astype(np.uint8)
        tile_rgb = palette_u8[idx_local]
        canvas_rgb[y0:y0 + 16, x0:x0 + 16] = tile_rgb
        alpha_mask[y0:y0 + 16, x0:x0 + 16] = opaque_tile

    # 3. Single global palette from the union of per-tile palettes.
    all_pal_rgb = np.concatenate([
        np.array(p, dtype=np.uint8).reshape(15, 3) for p in final_palettes
    ], axis=0)
    nonzero = all_pal_rgb.sum(axis=1) > 0
    candidates = all_pal_rgb[nonzero] if nonzero.any() else all_pal_rgb
    candidates = snap_neogeo(candidates)
    cand_lab = rgb_to_lab(candidates).astype(np.float32)
    global_centres_lab = _kmeans_pp_weighted_local(cand_lab, n_colors)
    from img2neo import lab_to_rgb
    global_palette = snap_neogeo(lab_to_rgb(global_centres_lab))

    # 4. Global RGB-Euclidean Floyd-Steinberg dither.  Transparent
    # pixels skip the dither entirely so the alpha mask survives the
    # final pass and the output's slot-0 transparency is preserved.
    raw_idx = _floyd_steinberg_global(canvas_rgb, global_palette, alpha_mask)
    indexed = np.where(alpha_mask,
                        (raw_idx + 1).astype(np.uint16),
                        np.uint16(0))

    palette16 = np.zeros((16, 3), dtype=np.uint16)
    palette16[1:1 + n_colors] = global_palette.astype(np.uint16)

    metadata = {
        "source_width":   int(target_w),
        "source_height":  int(target_h),
        "canvas_width":   int(target_w),
        "canvas_height":  int(target_h),
        "content_left":   0,
        "content_top":    0,
        "content_width":  int(target_w),
        "content_height": int(target_h),
        "n_colors":       int(n_colors),
        "tile_cols":      int(tile_cols),
        "tile_rows":      int(tile_rows),
        "n_tiles":        int(n_tiles),
        "pipeline":       "vivid_pertile_lab_kmeans",
        "cache_version":  _VIVID_CACHE_VERSION,
    }
    _vivid_cache_save(cache_key, indexed, palette16, metadata)
    return indexed, palette16, metadata


def _kmeans_palette_from_context(context_rgb: np.ndarray,
                                  alpha_mask: np.ndarray | None,
                                  n_colors: int) -> np.ndarray:
    """
    Run CIE-Lab k-means++ over the OPAQUE pixels of a context window
    and return the (n_colors, 3) uint8 NeoGeo-snapped palette.  No
    dithering — palette derivation only.

    Uses Lab distance (L*a*b* perceptually balanced) instead of the
    earlier luma-weighted YCbCr.  The 4x Y weight in YCbCr collapsed
    chroma distinctions: blue + grey pixels of the same luma landed
    in the same cluster, so palettes ended up sepia/grey-heavy.
    Lab keeps brightness AND hue/chroma weighted naturally
    (1-unit-L ≈ 1-unit-a* ≈ 1-unit-b* perceptually).

    Used by convert_screen_via_tile_palette to sample palette
    candidates from a 24x24 window so adjacent tiles see shared
    content and produce palette hints that line up across the
    16-pixel tile boundary.
    """
    if alpha_mask is None:
        opaque_rgb = context_rgb.reshape(-1, 3)
    else:
        opaque_rgb = context_rgb[alpha_mask]
    if opaque_rgb.size == 0:
        return np.zeros((n_colors, 3), dtype=np.uint8)
    # CIE-Lab feature space — perceptually balanced L*a*b* so cluster
    # centres respect both brightness AND chroma.  _kmeans_pp_weighted
    # has explicit uniform-input guards that img2neo.kmeans_palette
    # lacks (the latter trips a numerical probability-collapse on
    # small uniform-colour windows like a clear-sky tile).
    opaque_lab = rgb_to_lab(opaque_rgb)
    centres_lab = _kmeans_pp_weighted(opaque_lab.astype(np.float32),
                                        n_clusters=n_colors)
    from img2neo import lab_to_rgb
    return snap_neogeo(lab_to_rgb(centres_lab))


def convert_screen_via_tile_palette(
        image_path: str | os.PathLike,
        target_w: int | None = 320,
        target_h: int | None = 224,
        fit: str = "contain",
        anchor: str = "center",
        n_colors: int = COLORS_PER_TILE,
        alpha_threshold: int = ALPHA_OPAQUE_THRESHOLD,
        max_banks: int = 32,
        epsilon: float = 15.0,
        context_pad: int = 4,
        gamma: float = 1.20,
        contrast: float = 1.10,
        ) -> tuple[np.ndarray, np.ndarray, dict]:
    """
    Drop-in replacement for `romdbimgimport.load_screen_asset`'s
    full-image quantizer.

    Pipeline (eliminates tile-boundary seams by combining context-
    window palette derivation with a single image-wide dither pass):

      1. LANCZOS-resize / pad source to a 16-aligned canvas, then
         apply the CRT pre-boost (gamma 1.20 + 1.10x contrast) so
         the rest of the pipeline operates in arcade-luminance space.

      2. For each 16x16 tile slot, sample a (16 + 2*context_pad)
         square centred on the tile and run luma-weighted k-means++
         over its opaque pixels.  Default context_pad=4 gives a
         24x24 window per tile with an 8-pixel-strip overlap with
         every neighbour, so adjacent tiles see shared content and
         produce palette candidates that align across boundaries
         (the single biggest cause of visible 16-pixel grid seams
         in the previous version).

      3. Greedy-MAE-cluster the per-tile palettes into <= max_banks
         shared banks.  Multi-bank data is kept available via
         `extract_tile_data_for_dedup()` / `cluster_and_remap_tile_palettes()`
         for future per-tile-bank packers; the single-bank
         downstream consumes it via step 4.

      4. Derive ONE representative 15-colour palette by weighted
         Lab-space k-means++ over the union of per-tile palettes,
         each bank weighted by its tile count.  Better-informed
         than naive global k-means because per-tile clustering
         already de-noised the input set.

      5. Floyd-Steinberg dither the WHOLE image (single pass) against
         the representative palette using luma-weighted nearest-
         colour lookup.  A single global pass means error diffuses
         seamlessly across tile boundaries — no more grid lines
         from independent per-tile FS runs.

    Multi-bank artefacts are still emitted in `metadata` so a future
    `genscreens` that supports per-tile palette attributes can use
    them without re-running the whole pipeline.

    Returns
    -------
    indexed   : (H, W) uint16 — 0 = transparent, 1..15 opaque
    palette16 : (16, 3) uint16 — palette[0] sentinel, palette[1..15]
                are the derived representative colours
    metadata  : dict — geometry + dedup statistics + CRT params
    """
    from img2neo_crt import apply_crt_tone

    img = Image.open(image_path).convert("RGBA")
    canvas, cl, ct, cw, ch = _fit_to_tile_grid(img, target_w, target_h, fit)
    arr = np.array(canvas, dtype=np.uint8)
    canvas_h, canvas_w = arr.shape[:2]

    # --- Step 1: CRT pre-boost (gamma + contrast) ---------------------
    rgb_f = arr[:, :, :3].astype(np.float32) / 255.0
    rgb_f = apply_crt_tone(rgb_f, gamma=gamma, contrast=contrast)
    rgb_corrected = snap_neogeo((rgb_f * 255.0).astype(np.uint8))
    alpha_full = arr[:, :, 3]
    opaque_full = alpha_full >= alpha_threshold

    tile_cols = canvas_w // TILE_SIZE
    tile_rows = canvas_h // TILE_SIZE
    n_tiles = tile_cols * tile_rows

    # Pad by context_pad pixels on every side so edge tiles can sample
    # a full context window without index gymnastics.  Pad mode "edge"
    # extends the boundary colours into the pad — matches what neighbour
    # tiles see for interior windows.
    pad = int(context_pad)
    padded_rgb = np.pad(rgb_corrected,
                        ((pad, pad), (pad, pad), (0, 0)), mode="edge")
    padded_opaque = np.pad(opaque_full,
                           ((pad, pad), (pad, pad)), mode="edge")

    # --- Step 2: per-tile palette from context windows ----------------
    # Sample (TILE_SIZE + 2*pad) windows; cluster k=n_colors per window.
    # No per-tile dither here — the global pass below handles dither.
    tile_palettes = np.zeros((n_tiles, n_colors, 3), dtype=np.uint8)
    win_size = TILE_SIZE + 2 * pad
    for ty in range(tile_rows):
        for tx in range(tile_cols):
            py = ty * TILE_SIZE + pad
            px = tx * TILE_SIZE + pad
            ctx_rgb = padded_rgb[py - pad:py - pad + win_size,
                                  px - pad:px - pad + win_size]
            ctx_opq = padded_opaque[py - pad:py - pad + win_size,
                                     px - pad:px - pad + win_size]
            ti = ty * tile_cols + tx
            tile_palettes[ti] = _kmeans_palette_from_context(
                ctx_rgb, ctx_opq, n_colors)

    # --- Step 3: greedy MAE bank dedup (kept for future per-bank work)
    # We don't use the remapped indices here (no per-tile dither to
    # remap), but the bank palettes + tile-to-bank mapping are
    # information consumers of extract_tile_data_for_dedup expect.
    # Reuse `cluster_and_remap_tile_palettes` for the bank palettes
    # only; throw away the remapped indices (we'll dither globally).
    dummy_local_indices = np.zeros((n_tiles, PIXELS_PER_TILE),
                                    dtype=np.uint8)
    _, bank_palettes, tile_to_bank = cluster_and_remap_tile_palettes(
        dummy_local_indices, tile_palettes,
        max_banks=max_banks, epsilon=epsilon,
    )

    # --- Step 5: collapse to single representative bank ---------------
    # Pick the bank used by the most tiles WHOSE PALETTE IS NON-
    # DEGENERATE.  Naive "most-tile-count" picks the all-zero palette
    # produced by fully-transparent border tiles in letterboxed
    # images — that bank dominates the count but contributes no
    # colour, so the second remap pass collapses every content
    # colour onto bank-slot-0 and the whole image goes monochrome.
    # Filter to banks whose palette has at least one non-zero RGB
    # channel before voting.
    n_banks_pre = bank_palettes.shape[0]
    bincount = np.bincount(tile_to_bank, minlength=n_banks_pre)
    nonzero_bank_mask = bank_palettes.reshape(n_banks_pre, -1).any(axis=1)
    if not nonzero_bank_mask.any():
        # Whole image is transparent — emit zeros.
        final_indices_2d = np.zeros((canvas_h, canvas_w), dtype=np.uint16)
        palette16 = np.zeros((16, 3), dtype=np.uint16)
        return final_indices_2d, palette16, {
            "source_width":   int(img.size[0]),
            "source_height":  int(img.size[1]),
            "canvas_width":   int(canvas_w),
            "canvas_height":  int(canvas_h),
            "content_left":   int(cl),
            "content_top":    int(ct),
            "content_width":  int(cw),
            "content_height": int(ch),
            "n_colors":       int(n_colors),
            "tile_cols":      int(tile_cols),
            "tile_rows":      int(tile_rows),
            "n_tiles":        int(n_tiles),
            "pipeline":       "tile_local_dedup_remap_empty",
        }
    # Derive the representative palette from ALL non-degenerate banks
    # weighted by their tile count.  Picking the single most-used bank
    # (argmax) biases toward whatever region dominates the canvas
    # (e.g. a sky / floor) and the rest of the image collapses to its
    # nearest neighbours in that region's 15 colours.  Weighted
    # Lab-k-means over the multi-bank candidate union finds the 15
    # colours that best span the actual content across the whole
    # image.  Still ONE palette (single-bank downstream constraint)
    # but informed by the entire multi-bank dedup.
    masked_counts = np.where(nonzero_bank_mask, bincount, 0).astype(np.float32)
    # Build a weighted candidate-colour set: each bank's 15 colours
    # repeated `tile_count` times.  np.repeat keeps this simple and
    # the union stays small (<= 32 banks * 15 colours = 480 candidates
    # before weighting, which weighted k-means handles trivially).
    weighted_candidates = []
    weighted_weights = []
    for bi in range(n_banks_pre):
        if not nonzero_bank_mask[bi] or masked_counts[bi] <= 0:
            continue
        weighted_candidates.append(bank_palettes[bi])
        weighted_weights.append(np.full(n_colors, masked_counts[bi],
                                          dtype=np.float32))
    flat_candidates = np.concatenate(weighted_candidates, axis=0)
    flat_weights = np.concatenate(weighted_weights, axis=0)
    # Drop pure-black padding entries — every bank pads short palettes
    # with the last-used colour, but legitimate dark colours rarely
    # land at exactly (0,0,0).  Keep (0,0,0) only when it carries
    # weight from a truly-black tile.
    nonzero_color_mask = flat_candidates.sum(axis=1) > 0
    if nonzero_color_mask.any():
        flat_candidates = flat_candidates[nonzero_color_mask]
        flat_weights = flat_weights[nonzero_color_mask]

    # Lab-space weighted k-means++.
    cand_lab = rgb_to_lab(flat_candidates)
    rng = np.random.default_rng(0)
    centres = np.empty((n_colors, 3), dtype=np.float32)
    probs = flat_weights / flat_weights.sum()
    centres[0] = cand_lab[rng.choice(len(cand_lab), p=probs)]
    closest_d2 = np.sum((cand_lab - centres[0]) ** 2, axis=1)
    for i in range(1, n_colors):
        biased = closest_d2 * flat_weights
        total = biased.sum()
        if total <= 1e-12:
            centres[i] = cand_lab[rng.integers(len(cand_lab))]
        else:
            centres[i] = cand_lab[rng.choice(len(cand_lab),
                                              p=biased / total)]
        new_d2 = np.sum((cand_lab - centres[i]) ** 2, axis=1)
        closest_d2 = np.minimum(closest_d2, new_d2)
    for _ in range(20):
        d2 = np.sum((cand_lab[:, None, :] - centres[None, :, :]) ** 2,
                    axis=2)
        labels = d2.argmin(axis=1)
        moved = False
        new_centres = centres.copy()
        for j in range(n_colors):
            mask = labels == j
            if mask.any():
                w = flat_weights[mask][:, None]
                wsum = w.sum()
                if wsum > 0:
                    mean = (cand_lab[mask] * w).sum(axis=0) / wsum
                    if np.any(np.abs(mean - centres[j]) > 1e-3):
                        moved = True
                    new_centres[j] = mean
        centres = new_centres
        if not moved:
            break

    from img2neo import lab_to_rgb
    rep_palette = snap_neogeo(lab_to_rgb(centres))         # (15, 3) u8
    # No single source bank now corresponds to rep_palette — fix the
    # rep_bank value to a sentinel so the equality check in the tile
    # loop below never short-circuits the second remap pass (every
    # tile gets remapped through rep_palette via Lab-nearest).
    rep_bank = -1

    # --- Step 5: single image-wide Floyd-Steinberg pass ---------------
    # One global FS pass means dither error diffuses smoothly across
    # tile boundaries.  This is the second half of the seam fix — the
    # context window aligned the palette choices in step 2, and now
    # the global dither aligns the per-pixel patterns too.
    if opaque_full.any():
        raw_idx = _floyd_steinberg_global(rgb_corrected, rep_palette,
                                            opaque_full)
        final_indices_2d = np.where(opaque_full,
                                     (raw_idx + 1).astype(np.uint16),
                                     np.uint16(0))
    else:
        final_indices_2d = np.zeros((canvas_h, canvas_w), dtype=np.uint16)

    palette16 = np.zeros((16, 3), dtype=np.uint16)
    palette16[1:1 + n_colors] = rep_palette.astype(np.uint16)

    metadata = {
        "source_width":   int(img.size[0]),
        "source_height":  int(img.size[1]),
        "canvas_width":   int(canvas_w),
        "canvas_height":  int(canvas_h),
        "content_left":   int(cl),
        "content_top":    int(ct),
        "content_width":  int(cw),
        "content_height": int(ch),
        "n_colors":       int(n_colors),
        "tile_cols":      int(tile_cols),
        "tile_rows":      int(tile_rows),
        "n_tiles":        int(n_tiles),
        "pipeline":       "tile_context_global_fs",
        "n_banks_pre_collapse":  int(n_banks_pre),
        "n_banks_used":          int(nonzero_bank_mask.sum()),
        "max_banks_budget":      int(max_banks),
        "epsilon_mae":           float(epsilon),
        "context_pad":           int(pad),
        "context_window":        int(win_size),
        "gamma":                 float(gamma),
        "contrast":              float(contrast),
    }
    return final_indices_2d, palette16, metadata


# ---------------------------------------------------------------------------
# Optional downstream — palette-bank deduplication
# ---------------------------------------------------------------------------

def cluster_palette_banks(palettes: np.ndarray,
                          max_banks: int = 256,
                          lab_eps: float = 8.0
                          ) -> tuple[np.ndarray, np.ndarray]:
    """
    Collapse near-identical per-tile palettes into shared banks so the
    final set fits in NeoGeo palette RAM (256 banks of 16 colours).

    Strategy: convert each palette's 15 colours to Lab, take the mean
    Lab vector as the palette's "centroid", then greedy-cluster tiles
    whose centroids fall within `lab_eps` of an existing bank's
    centroid.  Each bank's palette is the elementwise median of its
    member palettes (more robust than mean against single-tile
    outliers).  If the greedy pass exceeds `max_banks`, the eps is
    doubled and retried until the bank budget is met.

    Parameters
    ----------
    palettes : (n_tiles, 15, 3) uint8 — output of the tile converter.
    max_banks : hardware bank budget (defaults to the YM6502/Neo Geo
                256-bank ceiling — set lower if your game reserves
                banks for HUD/fonts).
    lab_eps   : initial mean-Lab distance threshold for grouping.

    Returns
    -------
    bank_palettes : (n_banks, 15, 3) uint8 — the deduped palette set.
    tile_to_bank  : (n_tiles,) int32 — index into bank_palettes for
                    each input tile.
    """
    n_tiles = palettes.shape[0]

    # Per-palette Lab centroid (mean of the 15 colours' Lab).  Skip
    # all-zero palettes (fully transparent tiles) — they get assigned
    # to whichever bank ends up at index 0, contents irrelevant since
    # their indices are all 0 (transparent).
    palette_lab = rgb_to_lab(palettes.reshape(-1, 3)).reshape(n_tiles, -1, 3)
    centroids = palette_lab.mean(axis=1).astype(np.float32)

    eps = float(lab_eps)
    for _attempt in range(8):
        bank_centroids: list[np.ndarray] = []
        bank_members: list[list[int]] = []
        tile_to_bank = np.full(n_tiles, -1, dtype=np.int32)

        for ti in range(n_tiles):
            if not bank_centroids:
                bank_centroids.append(centroids[ti])
                bank_members.append([ti])
                tile_to_bank[ti] = 0
                continue
            arr = np.stack(bank_centroids, axis=0)
            d = np.sqrt(np.sum((arr - centroids[ti]) ** 2, axis=1))
            best = int(d.argmin())
            if d[best] <= eps:
                bank_members[best].append(ti)
                tile_to_bank[ti] = best
            else:
                bank_centroids.append(centroids[ti])
                bank_members.append([ti])
                tile_to_bank[ti] = len(bank_centroids) - 1

        if len(bank_centroids) <= max_banks:
            break
        eps *= 1.5
    else:
        raise RuntimeError(f"Could not fit palette banks under "
                           f"{max_banks} even at eps={eps:.1f}")

    # Median-merge each bank's member palettes.  Median is robust to
    # single outliers and preserves the most-common colour in each slot.
    n_banks = len(bank_centroids)
    bank_palettes = np.zeros((n_banks, palettes.shape[1], 3), dtype=np.uint8)
    for bi, members in enumerate(bank_members):
        member_palettes = palettes[members]
        bank_palettes[bi] = np.median(member_palettes, axis=0).astype(np.uint8)
    bank_palettes = snap_neogeo(bank_palettes)
    return bank_palettes, tile_to_bank


# ---------------------------------------------------------------------------
# CLI / file plumbing
# ---------------------------------------------------------------------------

def _save_preview_png(out_path: str,
                      indices_flat: np.ndarray,
                      palettes: np.ndarray,
                      tile_cols: int,
                      tile_rows: int):
    """
    Write a quick visual-QA indexed PNG using a flat megapalette that
    concatenates every tile's palette.  Each tile's local indices are
    rewritten as (tile_idx * 16 + local_index) so the result is one
    big indexed image; useful for eyeballing the conversion but NOT
    a valid Neo Geo asset (the megapalette exceeds the bank limit).
    """
    n_tiles = indices_flat.shape[0]
    canvas_w = tile_cols * TILE_SIZE
    canvas_h = tile_rows * TILE_SIZE

    mega_palette = [(0, 0, 0)] * (n_tiles * 16)
    for ti in range(n_tiles):
        base = ti * 16
        for ci in range(palettes.shape[1]):
            r, g, b = palettes[ti, ci].tolist()
            mega_palette[base + 1 + ci] = (int(r), int(g), int(b))

    canvas = np.zeros((canvas_h, canvas_w), dtype=np.uint16)
    for ti in range(n_tiles):
        ty, tx = divmod(ti, tile_cols)
        tile = indices_flat[ti].reshape(TILE_SIZE, TILE_SIZE)
        nonzero = tile != 0
        out_slice = np.zeros_like(tile, dtype=np.uint16)
        out_slice[nonzero] = ti * 16 + tile[nonzero]
        canvas[ty * TILE_SIZE:(ty + 1) * TILE_SIZE,
               tx * TILE_SIZE:(tx + 1) * TILE_SIZE] = out_slice

    bitdepth = 16 if n_tiles * 16 > 256 else 8
    writer = png.Writer(width=canvas_w, height=canvas_h,
                        bitdepth=bitdepth, palette=mega_palette)
    rows = canvas.tolist()
    with open(out_path, "wb") as f:
        writer.write(f, rows)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n", 1)[0])
    ap.add_argument("src")
    ap.add_argument("dst_dir")
    ap.add_argument("-W", "--width", type=int, default=320)
    ap.add_argument("-H", "--height", type=int, default=224)
    ap.add_argument("-c", "--colors", type=int, default=COLORS_PER_TILE,
                    help="Colours per tile (1..15). Default 15.")
    ap.add_argument("--fit", choices=("contain", "crop"), default="contain")
    ap.add_argument("--merge-banks", type=int, default=0, metavar="N",
                    help="Run cluster_palette_banks() with max_banks=N "
                         "and write bank_palettes.bin + tile_to_bank.bin")
    args = ap.parse_args()

    if not 1 <= args.colors <= 15:
        ap.error("--colors must be 1..15")

    indices, palettes, meta = convert_png_to_neogeo_tile_palettes(
        args.src,
        target_w=args.width,
        target_h=args.height,
        fit=args.fit,
        n_colors_per_tile=args.colors,
    )

    out = Path(args.dst_dir)
    out.mkdir(parents=True, exist_ok=True)

    (out / "indices.bin").write_bytes(indices.tobytes())
    (out / "palettes.bin").write_bytes(palettes.reshape(-1, 45).tobytes())
    with open(out / "meta.json", "w") as f:
        json.dump(meta, f, indent=2)
    _save_preview_png(str(out / "preview.png"),
                      indices, palettes,
                      meta["tile_cols"], meta["tile_rows"])

    print(f"[tile] {os.path.basename(args.src)} -> {out}/")
    print(f"       canvas {meta['canvas_w']}x{meta['canvas_h']} "
          f"({meta['tile_cols']}x{meta['tile_rows']} tiles, "
          f"{meta['n_tiles']} total)")
    print(f"       indices.bin   = {meta['n_tiles']} * 256 = "
          f"{meta['n_tiles'] * 256} bytes")
    print(f"       palettes.bin  = {meta['n_tiles']} * 45  = "
          f"{meta['n_tiles'] * 45} bytes")

    if args.merge_banks:
        bank_palettes, tile_to_bank = cluster_palette_banks(
            palettes, max_banks=args.merge_banks)
        (out / "bank_palettes.bin").write_bytes(
            bank_palettes.reshape(-1, 45).tobytes())
        (out / "tile_to_bank.bin").write_bytes(
            tile_to_bank.astype(np.uint16).tobytes())
        print(f"       merged into {bank_palettes.shape[0]} palette banks "
              f"(budget {args.merge_banks})")


if __name__ == "__main__":
    main()
