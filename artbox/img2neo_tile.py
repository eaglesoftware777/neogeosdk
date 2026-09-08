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
1. Linear-light Lanczos resize to a target divisible by 16.
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
from PIL import Image, ImageFilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# Reuse the NeoGeo 5-bit grid snap and Lab conversion from img2neo —
# keeps the module dependency-free of scikit-learn / scikit-image.
from img2neo import (rgb_to_lab, lab_to_rgb,
                     kmeans_palette as _img2neo_kmeans_palette,
                     alpha_bleed, resize_rgba_linear,
                     enhance_for_quantisation, enhance_enabled)


TILE_SIZE = 16
COLORS_PER_TILE = 15
PIXELS_PER_TILE = TILE_SIZE * TILE_SIZE   # 256
ALPHA_OPAQUE_THRESHOLD = 128
ALPHA_SOLID_THRESHOLD = 240

# How much local turbulence a pixel needs before it is treated as detail and
# error-diffused rather than snapped to its nearest palette entry.
#
# Sprite art on this hardware is drawn the way the arcade originals drew it:
# flat colour fields separated by deliberate shading bands and a hard
# outline.  Error diffusion across a field like that invents texture the
# artist never put there, and on an LCD or over HDMI - where there is no
# CRT spot bloom to average it away - that texture reads as dirt on the
# character's skin.  Photographic and painted backdrops are the opposite
# case: they have real continuous gradients that need the dither to avoid
# banding, so they cross into detail far sooner.
#
# The sprite cut used to be much higher than the backdrop one, to hold flat
# fields out of a diffusion pass that was all-or-nothing.  Now that the
# strength ramps in (see the dither loop), the wide band is no longer needed
# and it cost accuracy: measured against the source both per pixel and
# through a display blur, ramping from the lower cut beats the old hard
# switch at the higher one on every one of those measures at once.
SPRITE_SMOOTH_CUT = 15.0
BACKGROUND_SMOOTH_CUT = 15.0
# Ordered blue-noise mixing between the two nearest entries.  Backdrops want
# it (it is what keeps a sky from banding); flat sprite fields do not.
SPRITE_ORDERED_MIX = False


# ---------------------------------------------------------------------------
# Screen display geometry
# ---------------------------------------------------------------------------
# A screen asset is stored as a square 256x256 canvas, and that is not the
# shape anybody sees it in.  The strips are placed with a vertical shrink, so
# the 256 stored rows land on fewer scanlines than they occupy in the file,
# and the console's 320x224 output is itself shown on a 4:3 screen, so one
# pixel is not square either.  A 256x256 canvas drawn with the usual
# background shrink covers 256x176 pixels, which on a 4:3 screen is about
# 1.36:1 - so fitting artwork to the square canvas preserves its proportions
# only on a surface nobody looks at, and guarantees they are wrong on the one
# they do.
#
# These convert between the stored canvas and the screen so a fit can target
# the shape the player actually sees.

NG_ACTIVE_W = 320
NG_ACTIVE_H = 224
# Width of one pixel relative to its height, once the active area is shown
# on a 4:3 screen.
NG_PIXEL_ASPECT = (4.0 / 3.0) / (float(NG_ACTIVE_W) / float(NG_ACTIVE_H))
# SCB2 vertical shrink meaning "keep every line".
NG_SHRINK_Y_NONE = 255


def ng_vertical_scale(shrink_y: int = NG_SHRINK_Y_NONE) -> float:
    """Fraction of its stored height a strip keeps for an SCB2 shrink value."""
    return (float(int(shrink_y)) + 1.0) / 256.0


def screen_display_aspect(canvas_w: int, canvas_h: int,
                          shrink_y: int = NG_SHRINK_Y_NONE) -> float:
    """Width:height that a stored canvas actually occupies on the screen."""
    return ((float(canvas_w) * NG_PIXEL_ASPECT) /
            (float(canvas_h) * ng_vertical_scale(shrink_y)))


def fit_screen_for_display(img: Image.Image,
                           canvas_w: int,
                           canvas_h: int,
                           shrink_y: int = NG_SHRINK_Y_NONE,
                           fit: str = "crop",
                           anchor: str = "center"
                           ) -> tuple[Image.Image, int, int, int, int]:
    """
    Place `img` on a canvas_w x canvas_h canvas so it is undistorted ON THE
    SCREEN, pre-compensating for the vertical shrink and the pixel aspect.

    fit="crop"    fills the canvas edge to edge and trims whatever does not
                  fit, which is what a backdrop wants - a letterboxed
                  background is transparent bands over the backdrop colour.
    fit="contain" keeps the whole picture and pads instead.

    Returns (canvas RGBA, left, top, content_w, content_h).
    """
    img = img.convert("RGBA")
    src_w, src_h = img.size
    if src_w <= 0 or src_h <= 0:
        return (Image.new("RGBA", (canvas_w, canvas_h), (0, 0, 0, 0)),
                0, 0, 0, 0)

    src_ar = float(src_w) / float(src_h)
    vscale = ng_vertical_scale(shrink_y)

    if fit == "crop":
        # Crop the source to the shape the whole canvas will be shown in,
        # then let it fill the canvas.
        want = screen_display_aspect(canvas_w, canvas_h, shrink_y)
        if src_ar > want:
            keep_w = int(round(src_h * want))
            keep_h = src_h
        else:
            keep_w = src_w
            keep_h = int(round(src_w / want))
        keep_w = max(1, min(src_w, keep_w))
        keep_h = max(1, min(src_h, keep_h))
        left = (src_w - keep_w) // 2
        top = (src_h - keep_h) // 2
        if anchor.startswith("top"):
            top = 0
        elif anchor.startswith("bottom"):
            top = src_h - keep_h
        img = img.crop((left, top, left + keep_w, top + keep_h))
        img = resize_rgba_linear(img, canvas_w, canvas_h)
        return img, 0, 0, canvas_w, canvas_h

    # "contain": the content keeps its own proportions, so its width and
    # height in canvas space have to be pre-distorted by the same shrink the
    # hardware is about to apply.
    want_ratio = src_ar * vscale / NG_PIXEL_ASPECT
    if want_ratio >= (float(canvas_w) / float(canvas_h)):
        content_w = canvas_w
        content_h = int(round(canvas_w / want_ratio))
    else:
        content_h = canvas_h
        content_w = int(round(canvas_h * want_ratio))
    content_w = max(1, min(canvas_w, content_w))
    content_h = max(1, min(canvas_h, content_h))

    scaled = resize_rgba_linear(img, content_w, content_h)
    canvas = Image.new("RGBA", (canvas_w, canvas_h), (0, 0, 0, 0))
    cl = (canvas_w - content_w) // 2
    ct = (canvas_h - content_h) // 2
    if anchor.startswith("top"):
        ct = 0
    elif anchor.startswith("bottom"):
        ct = canvas_h - content_h
    if anchor.endswith("left"):
        cl = 0
    elif anchor.endswith("right"):
        cl = canvas_w - content_w
    canvas.alpha_composite(scaled, (cl, ct))
    return canvas, cl, ct, content_w, content_h


# ---------------------------------------------------------------------------
# The hardware colour lattice
# ---------------------------------------------------------------------------
# A palette word is  D | R0 G0 B0 | R4..R1 | G4..G1 | B4..B1.  Each channel
# carries SIX bits: the five held in the word, plus a sixth and least
# significant one that all three channels share and that D supplies
# inverted -
#
#     channel6 = (channel5 << 1) | (1 - D)
#
# which is why the word with D set and nothing else is true black, and the
# word with D clear and everything else set is full white.  Because that
# sixth bit is shared, the three channels always come out on the same
# parity, so the reachable set is two interleaved 32^3 sub-lattices - 65536
# colours - rather than a free 64^3.
#
# This module used to quantise to 32 evenly spaced levels per channel
# (step 8, 0..248) and leave D clear for every entry.  That threw away the
# whole second sub-lattice, and it put the darkest colour the palette could
# express at 4/255 instead of 0 - black line art and black backdrops shipped
# as a very dark grey, which reads as washed-out on an LCD or over HDMI
# where there is no CRT falloff to hide it.  It also meant every entry sat
# half a step away from the colour the quantiser believed it had chosen, so
# the error being diffused was measured against the wrong target.
#
# Working on the real lattice costs nothing: it is the same number of
# palette entries, packed into the same 16-bit word.

_NG_LUMA_WEIGHT = np.array([0.299, 0.587, 0.114], dtype=np.float32)


def ng_expand6(v6: np.ndarray) -> np.ndarray:
    """6-bit channel value -> the 8-bit value the hardware actually shows."""
    v = np.asarray(v6, dtype=np.int32)
    return ((v << 2) | (v >> 4)).astype(np.uint8)


def ng_snap(rgb_u8: np.ndarray) -> np.ndarray:
    """
    Round RGB onto the nearest colour the hardware can display.

    Both parities are tried and the closer one wins, judged by
    luminance-weighted squared error - the two candidates are never more
    than one lattice step apart, so a full Lab round trip would not change
    the outcome often enough to pay for itself here.

    The returned values are the 8-bit colours the hardware displays, so
    `>> 2` recovers the exact 6-bit channel the packer needs.
    """
    src = np.asarray(rgb_u8, dtype=np.float32)
    shape = src.shape
    flat = src.reshape(-1, 3)
    exact = flat * (63.0 / 255.0)

    best = None
    best_err = None
    for parity in (0, 1):
        # Clamp inside the parity, not to the raw 0..63 span: rounding a
        # near-white channel up to 64 and then clipping it to 63 would hand
        # back an odd value on the even pass, and a triple whose channels
        # disagree on parity is not a colour the hardware can hold.
        v6 = np.clip(np.round((exact - parity) * 0.5) * 2.0 + parity,
                     float(parity), float(62 + parity))
        cand = ng_expand6(v6.astype(np.int32)).astype(np.float32)
        err = (((cand - flat) ** 2) * _NG_LUMA_WEIGHT).sum(axis=1)
        if best is None:
            best, best_err = cand, err
        else:
            take = err < best_err
            best = np.where(take[:, None], cand, best)
            best_err = np.where(take, err, best_err)
    return best.astype(np.uint8).reshape(shape)


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
# CIE-Lab lookup over the Neo Geo colour lattice
# ---------------------------------------------------------------------------
# Palettes are clustered in Lab, so pixels should be assigned in Lab too -
# matching in a different space than the one the clusters were built in is
# what lets a pixel land on an entry its own cluster never claimed, and it
# shows up as colour noise inside flat regions.
#
# Doing that honestly means a Lab conversion per pixel per candidate, which
# is far too slow for a per-pixel Python loop.  Each channel only has 64
# distinguishable levels, so the whole space is 64^3 = 262144 entries:
# precompute Lab for all of them once (~3 MB) and every lookup afterwards is
# an array index.  The grid is indexed by `rgb >> 2`, which is exact for any
# colour the hardware can show and costs at most one level of rounding for
# the error-laden accumulator values that get looked up mid-dither - an
# order of magnitude below the spacing between palette entries.

_LAB_LUT = None


def lab_lattice_lut() -> np.ndarray:
    """(64, 64, 64, 3) float32 Lab, indexed by the 6-bit channel triple."""
    global _LAB_LUT
    if _LAB_LUT is None:
        levels = ng_expand6(np.arange(64))
        grid = np.stack(np.meshgrid(levels, levels, levels, indexing="ij"),
                        axis=-1).astype(np.uint8)
        _LAB_LUT = rgb_to_lab(grid.reshape(-1, 3)).reshape(64, 64, 64, 3)
    return _LAB_LUT


def lab_palette(palette_u8: np.ndarray) -> np.ndarray:
    """Palette (K, 3) uint8 -> (K, 3) float32 Lab, via the lattice LUT."""
    lut = lab_lattice_lut()
    p = ng_snap(np.asarray(palette_u8, dtype=np.uint8).reshape(-1, 3)) >> 2
    return lut[p[:, 0], p[:, 1], p[:, 2]]


# ---------------------------------------------------------------------------
# Blue-noise threshold mask (void-and-cluster)
# ---------------------------------------------------------------------------
# Ordered dithering needs a threshold mask.  Bayer is the usual choice and
# is wrong here: its 45-degree cross-hatch is a strong periodic signal, and
# on a 15-colour palette covering a whole sky it reads as visible weave.
#
# A blue-noise mask has its energy pushed to high spatial frequencies, so
# the same amount of dithering disappears into texture instead of forming
# a pattern.  Void-and-cluster (Ulichney) builds one deterministically:
# repeatedly find the largest "void" (sparsest spot) and the tightest
# "cluster" in a binary pattern, and rank every pixel by the order in which
# it is filled.  Ranking gives a uniform threshold field with no low
# frequencies.
#
# 32x32 is large enough that the tile grid (16px) does not alias with it,
# and small enough to build in a fraction of a second at import.

_BLUE_NOISE_SIZE = 32


def _void_and_cluster_mask(size: int = _BLUE_NOISE_SIZE,
                            seed: int = 0) -> np.ndarray:
    """
    Deterministic void-and-cluster blue-noise mask, values in [0, 1).

    Returns (size, size) float32.  The filter is a wrapped Gaussian, so
    the mask tiles seamlessly - important because it is indexed modulo
    its size across the whole image.
    """
    n = size * size
    rng = np.random.default_rng(seed)

    # Wrapped Gaussian energy kernel in the frequency domain: filtering
    # by multiplication in FFT space keeps the toroidal wrap exact.
    ax = np.minimum(np.arange(size), size - np.arange(size)).astype(np.float32)
    d2 = ax[:, None] ** 2 + ax[None, :] ** 2
    kernel = np.exp(-d2 / (2.0 * (1.5 ** 2))).astype(np.float32)
    kernel_f = np.fft.rfft2(kernel)

    def energy(binary: np.ndarray) -> np.ndarray:
        return np.fft.irfft2(np.fft.rfft2(binary.astype(np.float32)) * kernel_f,
                              s=(size, size))

    # Initial binary pattern: 10% ones, then relaxed by repeatedly moving
    # the tightest cluster into the largest void until it is stable.
    initial = np.zeros((size, size), dtype=bool)
    flat = rng.permutation(n)[: max(1, n // 10)]
    initial.flat[flat] = True

    for _ in range(n):
        e = energy(initial)
        cluster = np.argmax(np.where(initial, e, -np.inf))
        initial.flat[cluster] = False
        e = energy(initial)
        void = np.argmin(np.where(initial, np.inf, e))
        if void == cluster:
            initial.flat[cluster] = True
            break
        initial.flat[void] = True

    rank = np.zeros((size, size), dtype=np.int32)
    prototype = initial.copy()
    ones = int(prototype.sum())

    # Phase 1: remove the tightest cluster repeatedly, ranking downward.
    work = prototype.copy()
    for r in range(ones - 1, -1, -1):
        e = energy(work)
        cluster = np.argmax(np.where(work, e, -np.inf))
        work.flat[cluster] = False
        rank.flat[cluster] = r

    # Phase 2 and 3: fill the largest void repeatedly, ranking upward.
    work = prototype.copy()
    for r in range(ones, n):
        e = energy(work)
        void = np.argmin(np.where(work, np.inf, e))
        work.flat[void] = True
        rank.flat[void] = r

    return (rank.astype(np.float32) + 0.5) / float(n)


_BLUE_NOISE = None


def blue_noise_mask() -> np.ndarray:
    """Lazily-built module-level blue-noise threshold mask."""
    global _BLUE_NOISE
    if _BLUE_NOISE is None:
        _BLUE_NOISE = _void_and_cluster_mask()
    return _BLUE_NOISE


# ---------------------------------------------------------------------------
# Lattice-constrained palette refinement
# ---------------------------------------------------------------------------

def refine_palette_on_lattice(palette_u8: np.ndarray,
                              pixels_u8: np.ndarray,
                              iters: int = 6) -> np.ndarray:
    """
    Re-fit a palette to its pixels *on the Neo Geo 5-bit colour lattice*.

    Lab k-means picks centroids in continuous space and the result is then
    snapped to the hardware grid, which moves every entry by up to half a
    step in each channel and leaves it sitting somewhere that is no longer
    the mean of anything.  Nobody re-fits afterwards, so the palette that
    ships is a rounded version of the right answer rather than the right
    answer for the colours the hardware can actually show.

    This runs Lloyd iterations where the update step is followed by a snap
    back onto the lattice, and keeps a move only when it lowers total
    error - so it can never make the palette worse than what came in.

    It also reclaims dead slots.  Snapping collapses near-identical
    centroids onto the same grid point, and _dedupe_and_pad_palette()
    pads the gap by repeating the last colour, so an image can ship with
    several of its fifteen slots doing no work at all.  Any duplicate or
    unused entry is re-seeded on the pixel that is currently worst served,
    which is the standard split-the-worst-cluster move.
    """
    pal = ng_snap(np.asarray(palette_u8, dtype=np.uint8).reshape(-1, 3))
    px = np.asarray(pixels_u8, dtype=np.uint8).reshape(-1, 3)
    if px.size == 0 or pal.size == 0:
        return pal

    # Cap the working set: the refit is O(N*K) per iteration and a full
    # 256x256 background is 65k pixels against 15 entries.
    if px.shape[0] > 24576:
        rng = np.random.default_rng(0)
        px = px[rng.choice(px.shape[0], 24576, replace=False)]

    k = pal.shape[0]
    px_f = lab_palette(px)

    def total_error(p_u8):
        d = np.sum((px_f[:, None, :] - lab_palette(p_u8)[None, :, :]) ** 2,
                   axis=2)
        return float(d.min(axis=1).sum()), d.argmin(axis=1), d.min(axis=1)

    best_err, labels, per_px = total_error(pal)

    for _ in range(iters):
        cand = pal.copy()

        # --- update step, snapped back onto the lattice ---
        for j in range(k):
            m = labels == j
            if m.any():
                cand[j] = ng_snap(px[m].mean(axis=0).round().astype(np.uint8))

        # --- reclaim slots that ended up duplicated or unowned ---
        seen = {}
        dead = []
        for j in range(k):
            key = tuple(int(v) for v in cand[j])
            if key in seen or not (labels == j).any():
                dead.append(j)
            else:
                seen[key] = j
        if dead:
            order = np.argsort(-per_px)
            taken = set(seen.keys())
            pick = 0
            for j in dead:
                while pick < order.shape[0]:
                    c = ng_snap(px[order[pick]])
                    pick += 1
                    key = tuple(int(v) for v in c)
                    if key not in taken:
                        cand[j] = c
                        taken.add(key)
                        break
                else:
                    break

        err, new_labels, new_per_px = total_error(cand)
        if err >= best_err:
            break
        pal, best_err, labels, per_px = cand, err, new_labels, new_per_px

    return pal


def _luma(rgb: np.ndarray) -> np.ndarray:
    rgb_f = rgb.astype(np.float32)
    return (rgb_f[..., 0] * 0.299 +
            rgb_f[..., 1] * 0.587 +
            rgb_f[..., 2] * 0.114)


def bilateral_filter(img_uint8: np.ndarray,
                     spatial_sigma: float = 2.0,
                     range_sigma: float = 28.0,
                     radius: int = 2) -> np.ndarray:
    """
    Bilateral filter — smooths flat areas, preserves sharp edges.
    """
    h, w, c = img_uint8.shape
    src = img_uint8.astype(np.float32)
    out = np.zeros_like(src)
    weight = np.zeros((h, w, 1), dtype=np.float32)

    yy, xx = np.mgrid[-radius:radius + 1, -radius:radius + 1]
    spatial = np.exp(-(yy * yy + xx * xx) / (2.0 * spatial_sigma * spatial_sigma))
    inv_range2 = 1.0 / (2.0 * range_sigma * range_sigma)

    for dy in range(-radius, radius + 1):
        for dx in range(-radius, radius + 1):
            n = np.roll(np.roll(src, dy, axis=0), dx, axis=1)
            diff = n - src
            rng = np.exp(-np.sum(diff * diff, axis=-1, keepdims=True) * inv_range2)
            sw  = spatial[dy + radius, dx + radius] * rng
            out += n * sw
            weight += sw
    return np.clip(out / (weight + 1e-6), 0.0, 255.0).astype(np.uint8)


def clahe_luminance(img_uint8: np.ndarray,
                    tile_grid: int = 4,
                    clip_limit: float = 1.8) -> np.ndarray:
    """
    Apply CLAHE to the luminance channel in Lab space to preserve and enhance
    local contrast across shadows and highlights.
    """
    h, w, _ = img_uint8.shape
    lab = rgb_to_lab(img_uint8)
    L = lab[..., 0]   # 0..100

    th = h // tile_grid
    tw = w // tile_grid
    if th == 0 or tw == 0:
        return img_uint8

    luts = np.zeros((tile_grid, tile_grid, 256), dtype=np.float32)
    L8   = np.clip(L * 2.55, 0, 255).astype(np.uint8)

    for ty in range(tile_grid):
        for tx in range(tile_grid):
            y0 = ty * th; y1 = (ty + 1) * th if ty < tile_grid - 1 else h
            x0 = tx * tw; x1 = (tx + 1) * tw if tx < tile_grid - 1 else w
            tile = L8[y0:y1, x0:x1]
            hist, _ = np.histogram(tile, bins=256, range=(0, 256))
            clip_val = max(1, int(clip_limit * (tile.size / 256.0)))
            excess = np.maximum(0, hist - clip_val).sum()
            hist = np.minimum(hist, clip_val) + (excess // 256)
            cdf = hist.cumsum().astype(np.float32)
            if cdf[-1] > 0:
                cdf = (cdf - cdf[0]) * 255.0 / (cdf[-1] - cdf[0] + 1e-6)
            luts[ty, tx] = cdf

    out_L = np.zeros_like(L, dtype=np.float32)
    for y in range(h):
        ty = min(tile_grid - 1, max(0, int((y - th / 2) / th)))
        fy = (y - (ty + 0.5) * th) / th
        fy = min(1.0, max(0.0, fy))
        ty1 = min(tile_grid - 1, ty + 1)
        for x in range(w):
            tx = min(tile_grid - 1, max(0, int((x - tw / 2) / tw)))
            fx = (x - (tx + 0.5) * tw) / tw
            fx = min(1.0, max(0.0, fx))
            tx1 = min(tile_grid - 1, tx + 1)
            v = L8[y, x]
            val = ((1 - fy) * (1 - fx) * luts[ty, tx, v] +
                   (1 - fy) * fx * luts[ty, tx1, v] +
                   fy * (1 - fx) * luts[ty1, tx, v] +
                   fy * fx * luts[ty1, tx1, v])
            out_L[y, x] = val / 2.55

    new_lab = lab.copy()
    new_lab[..., 0] = np.clip(out_L, 0.0, 100.0)
    return lab_to_rgb(new_lab)


def _neo_palette_pop(rgb: np.ndarray,
                     mask: np.ndarray | None,
                     asset_type: str = "background") -> np.ndarray:
    """
    Enhanced pre-quantisation tone and contrast pass for palette selection.

    Uses bilateral edge-preserving filtering + CLAHE local contrast on
    backgrounds, and perceptual shadow/highlight separation with saturation
    pop to ensure 15-colour palettes preserve the vibrant feeling of HD original art.
    """
    out = rgb.astype(np.float32).copy()

    if asset_type == "background" and out.ndim == 3 and out.shape[0] >= 16 and out.shape[1] >= 16:
        # Edge-preserving bilateral filter + CLAHE luminance expansion
        u8 = bilateral_filter(np.clip(out, 0.0, 255.0).astype(np.uint8), radius=2)
        u8 = clahe_luminance(u8, tile_grid=4, clip_limit=1.8)
        out = u8.astype(np.float32)

    if mask is None:
        sample = out.reshape(-1, 3)
    else:
        sample = out[mask]

    if sample.size:
        lum = _luma(sample)
        lo, hi = np.percentile(lum, (3.0, 97.0))
        if hi - lo > 18.0:
            cur_lum = _luma(out)
            target_lum = np.clip((cur_lum - lo) * (255.0 / (hi - lo)), 0.0, 255.0)
            blend = 0.30 if asset_type == "sprite" else 0.24
            scale = (cur_lum * (1.0 - blend) + target_lum * blend + 1.0) / (cur_lum + 1.0)
            out *= scale[..., None]

    # Saturation.  Fifteen colours across a whole image pulls everything
    # toward the middle of the gamut - the average of a cluster is always
    # less saturated than its members - so the source is pushed out before
    # clustering to land back at roughly the original vividness.  Arcade
    # art is also authored to read at a distance on a CRT, which wants
    # more separation than a fidelity-optimal match gives.
    sat = 1.16 if asset_type == "sprite" else 1.13
    gray = _luma(out)[..., None]
    out = gray + (out - gray) * sat

    # Soft knee instead of a hard clip.  Clipping folds every value above
    # 255 onto pure white and every negative onto pure black, so a boosted
    # highlight loses the shading that distinguished it - which is colour
    # detail thrown away before k-means ever sees it.  Compressing the top
    # and bottom eighth keeps those tones distinct and orderable.
    knee = 32.0
    hi_mask = out > (255.0 - knee)
    out[hi_mask] = (255.0 - knee) + knee * np.tanh(
        (out[hi_mask] - (255.0 - knee)) / knee)
    lo_mask = out < knee
    out[lo_mask] = knee * np.tanh(out[lo_mask] / knee)
    return np.clip(out, 0.0, 255.0).astype(np.uint8)


def _reserved_anchor_colors(rgb: np.ndarray,
                            max_anchors: int = 2,
                            min_fraction: float = 0.004) -> list:
    """
    Reserve hard contrast colours when the asset actually uses them.

    Pure black and pure white are what line art and speculars are drawn
    with, and k-means will not choose either: they sit at the end of a
    distribution, so a centroid always lands short of them and the
    outline comes back grey.  Reserving the two ends protects that.

    But a reserved slot is one of fifteen, and it has to earn it.  An
    existence test does not: a single dark pixel anywhere in the image -
    and an anti-aliased contour bled inward almost always leaves one -
    was enough to spend a slot on a colour nothing else wanted, which is
    two of fifteen gone on most assets for nothing.  Requiring a real
    population at the end means the slot goes to a colour the picture is
    actually made of, and an asset that has no true black keeps all
    fifteen for the colours it does have.
    """
    if rgb.size == 0 or max_anchors <= 0:
        return []
    lum = _luma(rgb)
    if lum.size == 0:
        return []
    anchors: list[np.ndarray] = []
    if float(np.mean(lum < 18.0)) >= min_fraction:
        anchors.append(np.array([0, 0, 0], dtype=np.uint8))
    if len(anchors) < max_anchors and float(np.mean(lum > 236.0)) >= min_fraction:
        # Both ends are exact hardware colours: black is the word with only
        # the dark bit set, white the word with every other bit set.
        anchors.append(np.array([255, 255, 255], dtype=np.uint8))
    return anchors


def _lab_kmeans_palette_with_anchors(rgb: np.ndarray,
                                     n_colors: int = 15,
                                     asset_type: str = "background",
                                     pre_toned: bool = False
                                     ) -> np.ndarray:
    """
    CIE-Lab k-means palette with optional black/white slot reservation.

    Returns exactly n_colors RGB entries on the hardware colour lattice.

    `pre_toned` says the caller has already run _neo_palette_pop over these
    pixels.  It matters: the quantiser dithers against the toned canvas, so
    a palette derived from a second helping of tone lands somewhere the
    pixels it has to serve never go, and every pixel is then matched to an
    entry pulled away from it.  Callers that hand over raw source pixels
    leave this False and get the tone pass applied once, here.
    """
    rgb = np.asarray(rgb, dtype=np.uint8).reshape(-1, 3)
    if rgb.size == 0:
        return np.zeros((n_colors, 3), dtype=np.uint8)

    anchors = _reserved_anchor_colors(rgb, max_anchors=2)
    k = max(1, n_colors - len(anchors))

    if rgb.shape[0] > 16384:
        rng = np.random.default_rng(0)
        rgb = rgb[rng.choice(rgb.shape[0], 16384, replace=False)]

    if pre_toned:
        source = rgb
    else:
        source = _neo_palette_pop(rgb.reshape(-1, 1, 3),
                                  np.ones((rgb.shape[0], 1), dtype=bool),
                                  asset_type=asset_type).reshape(-1, 3)
    lab = rgb_to_lab(source).astype(np.float32)
    centres = _kmeans_pp_weighted_vivid(lab, k)
    palette = ng_snap(lab_to_rgb(centres))

    if anchors:
        palette = np.vstack([palette, np.stack(anchors, axis=0)])

    palette = _dedupe_and_pad_palette(palette, source, n_colors)
    return ng_snap(palette)


def _dedupe_and_pad_palette(palette: np.ndarray,
                            fallback_rgb: np.ndarray,
                            n_colors: int = 15) -> np.ndarray:
    palette = ng_snap(np.asarray(palette, dtype=np.uint8).reshape(-1, 3))
    unique = []
    seen = set()
    for color in palette:
        key = tuple(int(v) for v in color)
        if key not in seen:
            unique.append(color)
            seen.add(key)
        if len(unique) == n_colors:
            break

    if not unique:
        unique.append(np.array([0, 0, 0], dtype=np.uint8))

    if len(unique) < n_colors and fallback_rgb.size:
        fallback = ng_snap(np.asarray(fallback_rgb, dtype=np.uint8).reshape(-1, 3))
        vals, counts = np.unique(fallback, axis=0, return_counts=True)
        order = np.argsort(-counts)
        for idx in order:
            key = tuple(int(v) for v in vals[idx])
            if key not in seen:
                unique.append(vals[idx])
                seen.add(key)
            if len(unique) == n_colors:
                break

    while len(unique) < n_colors:
        unique.append(unique[-1])
    return np.stack(unique[:n_colors], axis=0).astype(np.uint8)


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
    palette_rgb = ng_snap(palette_rgb)

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
    Linear-light resize / letterbox `img` onto a canvas whose dimensions
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
            img = resize_rgba_linear(img, tw, th)
            canvas = Image.new("RGBA", (tw, th), (0, 0, 0, 0))
            canvas.alpha_composite(img, (0, 0))
            return canvas, 0, 0, tw, th
        # "contain"
        if img.width > tw or img.height > th:
            k = min(tw / img.width, th / img.height)
            img = resize_rgba_linear(img,
                                     max(1, int(img.width * k)),
                                     max(1, int(img.height * k)))
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
    if enhance_enabled():
        arr = enhance_for_quantisation(arr)
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

def _palette_set_distance(bank_lab: np.ndarray,
                          tile_lab: np.ndarray) -> np.ndarray:
    """
    Perceptual distance between a tile palette and each candidate bank,
    treating both as SETS of colours rather than ordered lists.

    Comparing slot i of one palette against slot i of another is the
    obvious thing to do and it is wrong here: these palettes come out of
    k-means, so their slot order is whatever the seeding happened to
    produce.  Two tiles holding the same fifteen colours in a different
    order score as maximally different under a slot-wise metric, so they
    each take a bank of their own, the bank budget runs out, the
    tolerance is widened to compensate, and palettes that really are
    different get merged instead.  Order sensitivity spends the budget
    on duplicates and pays for it in fidelity.

    The metric here is symmetric mean-nearest-neighbour in Lab: how far
    each colour of one set sits from its closest counterpart in the
    other, averaged both ways.  Reordering a palette cannot change it,
    and it stays in units of dE, so a threshold means something.  Earth
    mover's distance would also work and would cost an assignment solve
    per comparison; at fifteen points against fifteen the two agree
    closely enough that the solve buys nothing.

    bank_lab : (B, 15, 3) float32
    tile_lab : (15, 3) float32
    returns  : (B,) float32
    """
    d2 = np.sum((bank_lab[:, None, :, :] - tile_lab[None, :, None, :]) ** 2,
                axis=3)                       # (B, tile_i, bank_j)
    d = np.sqrt(np.maximum(d2, 0.0))
    return 0.5 * (d.min(axis=2).mean(axis=1) + d.min(axis=1).mean(axis=1))


def _refit_bank(member_lab: np.ndarray,
                seed_lab: np.ndarray,
                iters: int = 8) -> np.ndarray:
    """
    Re-derive a bank's fifteen colours from every colour its members
    actually contain, instead of keeping whichever member arrived first.

    A greedy pass has to name a bank before it knows who will join it,
    so without this step a bank is one tile's palette and every other
    member is quantised to it.  A few Lloyd iterations in Lab, seeded
    from that first palette, move each entry to the centre of what it is
    being asked to represent.  Cheap, and it strictly lowers the error
    of the remap that follows.

    member_lab : (M, 3) float32 - every colour from every member palette
    seed_lab   : (15, 3) float32
    """
    centres = seed_lab.astype(np.float32).copy()
    if member_lab.shape[0] == 0:
        return centres
    for _ in range(iters):
        d2 = np.sum((member_lab[:, None, :] - centres[None, :, :]) ** 2,
                    axis=2)
        owner = d2.argmin(axis=1)
        moved = False
        for k in range(centres.shape[0]):
            hit = member_lab[owner == k]
            if hit.shape[0] == 0:
                continue
            new_c = hit.mean(axis=0)
            if not np.allclose(new_c, centres[k], atol=1e-3):
                moved = True
            centres[k] = new_c
        if not moved:
            break
    return centres


def cluster_and_remap_tile_palettes(
        indices_per_tile: np.ndarray,
        palettes_per_tile: np.ndarray,
        max_banks: int = 32,
        epsilon: float = 6.0,
        ) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """
    Greedy perceptual clustering of N per-tile palettes into <= max_banks
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
    epsilon           : dE threshold for "close enough to an existing
                        bank", measured with _palette_set_distance().
                        Lower = more banks, higher fidelity; higher =
                        fewer banks, more colour compromise.

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

    # Everything below compares colours in Lab, so convert once.
    pal_lab = rgb_to_lab(
        palettes_per_tile.reshape(-1, 3)).reshape(n_tiles, -1, 3)

    # --- 1. Greedy perceptual clustering with auto-scaling epsilon ---
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
    bank_lab_arr = None
    tile_to_bank = None
    for _attempt in range(8):
        bank_seed_lab: list[np.ndarray] = []
        candidate_assignments = np.full(n_tiles, -1, dtype=np.int32)
        overflowed = False
        for ti in range(n_tiles):
            matched = -1
            if bank_seed_lab:
                d = _palette_set_distance(np.stack(bank_seed_lab, axis=0),
                                          pal_lab[ti])
                best = int(d.argmin())
                if d[best] < cur_eps:
                    matched = best
            if matched == -1:
                if len(bank_seed_lab) < max_banks:
                    bank_seed_lab.append(pal_lab[ti].copy())
                    matched = len(bank_seed_lab) - 1
                else:
                    overflowed = True
                    break
            candidate_assignments[ti] = matched
        if not overflowed:
            bank_lab_arr = np.stack(bank_seed_lab, axis=0)
            tile_to_bank = candidate_assignments.astype(np.uint16)
            break
        # Overflow: widen tolerance and retry the entire pass.
        cur_eps *= 1.5
    if bank_lab_arr is None:
        # 8 attempts and still overflowing.  Fall back to the brute-
        # force nearest-bank assignment with the last attempted eps;
        # at least colours stay close to their original cluster.
        bank_lab_arr = np.stack(bank_seed_lab, axis=0)
        tile_to_bank = np.zeros(n_tiles, dtype=np.uint16)
        for ti in range(n_tiles):
            if candidate_assignments[ti] != -1:
                tile_to_bank[ti] = candidate_assignments[ti]
                continue
            d = _palette_set_distance(bank_lab_arr, pal_lab[ti])
            tile_to_bank[ti] = int(d.argmin())

    # --- 1b. Re-fit every bank to the members it ended up with -------
    # The greedy pass names a bank after the first palette that reaches
    # it and never revisits the choice, so an unrefined bank is one
    # tile's colours with everyone else rounded onto them.
    for bi in range(bank_lab_arr.shape[0]):
        members = np.flatnonzero(tile_to_bank == bi)
        if members.size <= 1:
            continue
        bank_lab_arr[bi] = _refit_bank(
            pal_lab[members].reshape(-1, 3), bank_lab_arr[bi])

    bank_palettes_arr = ng_snap(lab_to_rgb(bank_lab_arr))

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
        assigned_bank = int(tile_to_bank[ti])

        local_lab = pal_lab[ti]                            # (15, 3) f32
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
        return ng_snap(out)

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
    palette_rgb = ng_snap(lab_to_rgb(centres))
    return palette_rgb


def _floyd_steinberg_global(rgb: np.ndarray,
                            palette: np.ndarray,
                            alpha_mask: np.ndarray) -> np.ndarray:
    """
    Variance-gated dither — the "Photocopy" approach.  At every
    pixel we compute local turbulence (sum of abs RGB diffs with
    left + top neighbour).  Smooth surfaces (variance < 15) and
    luma extremes (>240 / <15) snap straight to the nearest palette
    entry from the PRISTINE source value: no upstream error
    contamination, no new error propagated.  Detail regions get
    the classic Floyd-Steinberg 7/3/5/1 distribution with a 0.75
    leak-dampener (75% retention = same total energy as Atkinson,
    tighter 4-neighbour diamond preserves high-frequency texture).

    The palette was already DERIVED in CIE-Lab via the context-
    window k-means, so RGB distance against a Lab-derived palette
    gives perceptually-correct nearest-colour matches at a
    fraction of the cost of per-pixel Lab conversion in the inner
    loop.

    Function name retained for source-stability; the algorithm is
    the variance-gated FS hybrid.  Returns (H, W) uint8 of palette
    indices 0..K-1; caller masks transparent pixels separately.
    """
    h, w = rgb.shape[:2]
    buf = rgb.astype(np.float32).copy()
    src = rgb.astype(np.float32)
    out = np.zeros((h, w), dtype=np.uint8)
    palette_f = palette.astype(np.float32)

    for y in range(h):
        for x in range(w):
            if not alpha_mask[y, x]:
                continue
            current_err = np.clip(buf[y, x], 0.0, 255.0)
            orig = src[y, x]
            orig_luma = (orig[0] * 0.299 +
                          orig[1] * 0.587 +
                          orig[2] * 0.114)

            variance = 0.0
            if x > 0 and y > 0:
                left = src[y, x - 1]
                top = src[y - 1, x]
                variance = (abs(orig[0] - left[0]) +
                             abs(orig[1] - left[1]) +
                             abs(orig[2] - left[2]) +
                             abs(orig[0] - top[0]) +
                             abs(orig[1] - top[1]) +
                             abs(orig[2] - top[2]))

            smooth_mode = (variance < 15.0
                            or orig_luma > 240.0
                            or orig_luma < 15.0)
            match_px = orig if smooth_mode else current_err

            d2 = np.sum((palette_f - match_px) ** 2, axis=1)
            best = int(d2.argmin())
            out[y, x] = best

            if not smooth_mode:
                err = (current_err - palette_f[best]) * 0.75
                for dy, dx, wgt in ((0, 1, 7.0 / 16.0),
                                      (1, -1, 3.0 / 16.0),
                                      (1, 0, 5.0 / 16.0),
                                      (1, 1, 1.0 / 16.0)):
                    ny, nx = y + dy, x + dx
                    if 0 <= ny < h and 0 <= nx < w and alpha_mask[ny, nx]:
                        buf[ny, nx] += err * wgt
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
    src = resize_rgba_linear(src, target_w, target_h)
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


def derive_master_sprite_palette(image_paths,
                                   n_colors: int = 15
                                   ) -> np.ndarray:
    """
    Compute ONE 15-colour CIE-Lab k-means++ palette covering the
    opaque pixels of an entire sprite sheet — or a list of separate
    animation-frame PNGs that should look identical from frame to
    frame.

    Pass this palette to `execute_final_vivid_pipeline(... master_palette=...)`
    for every frame so the animation can't flicker between frames:
    each frame is dithered against the SAME 15 colours.

    Transparent pixels (alpha < 128) are excluded from the cluster
    input so the master palette isn't biased toward whatever colour
    PNG editors happen to leave hidden under fully-transparent areas.

    Parameters
    ----------
    image_paths : str | os.PathLike | iterable of paths
                  Single sheet, or any iterable of per-frame paths.
    n_colors    : int, default 15.

    Returns
    -------
    (n_colors, 3) uint8 NeoGeo-snapped RGB palette.
    """
    if isinstance(image_paths, (str, os.PathLike)):
        image_paths = [image_paths]

    opaque_chunks: list[np.ndarray] = []
    for p in image_paths:
        img = Image.open(p).convert("RGBA")
        arr = alpha_bleed(np.array(img, dtype=np.uint8),
                          opaque_alpha=ALPHA_OPAQUE_THRESHOLD)
        alpha = arr[:, :, 3]
        mask = alpha >= ALPHA_SOLID_THRESHOLD
        if not mask.any():
            mask = alpha >= ALPHA_OPAQUE_THRESHOLD
        if mask.any():
            opaque_chunks.append(arr[mask][:, :3])

    if not opaque_chunks:
        return np.zeros((n_colors, 3), dtype=np.uint8)

    opaque_rgb = np.concatenate(opaque_chunks, axis=0)
    # Sub-sample very large sheets so k-means stays fast — Lab
    # clustering converges well on a few thousand representative
    # pixels.
    if opaque_rgb.shape[0] > 16384:
        rng = np.random.default_rng(0)
        idx = rng.choice(opaque_rgb.shape[0], 16384, replace=False)
        opaque_rgb = opaque_rgb[idx]

    return _lab_kmeans_palette_with_anchors(
        opaque_rgb, n_colors=n_colors, asset_type="sprite")


def execute_final_vivid_pipeline(image_path,
                                   target_w: int = 320,
                                   target_h: int = 224,
                                   asset_type: str = "background",
                                   master_palette: np.ndarray | None = None,
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
    src = resize_rgba_linear(src, target_w, target_h)
    return _vivid_pipeline_from_rgba(
        np.array(src, dtype=np.float32),
        asset_type=asset_type,
        master_palette=master_palette,
    )


def _vivid_pipeline_from_rgba(rgba: np.ndarray,
                                asset_type: str = "background",
                                master_palette: np.ndarray | None = None
                                ) -> tuple[list, list]:
    """
    Core 3-pass quantizer working from a pre-prepared (H, W, 4)
    float32 RGBA array.  Both execute_final_vivid_pipeline (the
    path-based entry) and the sprite import path call into this —
    the latter needs to run anchor-aware fit_sprite_rgba +
    alpha_bleed BEFORE quantisation, which would be skipped if it
    went through the path-based wrapper's flat .resize().
    """
    if rgba.ndim != 3 or rgba.shape[2] != 4:
        raise ValueError(f"_vivid_pipeline_from_rgba needs (H, W, 4); "
                          f"got shape {rgba.shape}")
    if asset_type == "sprite":
        rgba = alpha_bleed(rgba.astype(np.uint8),
                           opaque_alpha=ALPHA_OPAQUE_THRESHOLD)
    if enhance_enabled():
        rgba = enhance_for_quantisation(np.asarray(rgba, dtype=np.uint8))
    src_np = np.asarray(rgba).astype(np.float32, copy=False)
    target_h, target_w = src_np.shape[:2]
    if (target_h % 16) or (target_w % 16):
        raise ValueError(f"input dims must be multiples of 16; got "
                          f"{target_w}x{target_h}")
    rgb_raw = src_np[:, :, :3]
    alpha_np = src_np[:, :, 3]
    opaque_mask = alpha_np >= 128.0
    solid_mask = alpha_np >= float(ALPHA_SOLID_THRESHOLD)
    rgb_np = _neo_palette_pop(
        rgb_raw.astype(np.uint8),
        solid_mask if asset_type == "sprite" and solid_mask.any() else opaque_mask,
        asset_type=asset_type,
    ).astype(np.float32)

    # Painted/rendered source art (as opposed to art authored pixel-by-
    # pixel) carries fine per-pixel grain that has nothing to do with
    # genuine shading edges - every adjacent pair of pixels differs by a
    # little even across what reads, to the eye, as a single flat tone.
    # The turbulence test below decides "detail" vs "smooth" from raw
    # adjacent-pixel deltas, so that grain alone was enough to push
    # almost an entire flat face or sky into the Floyd-Steinberg branch,
    # sprinkling speckle nothing in the source actually called for.
    # A light blur, sampled only for the turbulence test (never for the
    # colour that gets quantised), tells flat-but-grainy apart from
    # genuinely shaded: real shading bands and outlines survive a small
    # blur with most of their contrast intact, single-pixel grain does
    # not.
    if asset_type == "sprite":
        rgb_for_turbulence = np.asarray(
            Image.fromarray(np.clip(rgb_np, 0, 255).astype(np.uint8))
                 .filter(ImageFilter.GaussianBlur(radius=2.0)),
            dtype=np.float32)
    else:
        rgb_for_turbulence = rgb_np

    tile_rows = target_h // 16
    tile_cols = target_w // 16

    # ---------------------------------------------------------
    # PASS 1: PALETTE DERIVATION (routed by asset_type)
    # ---------------------------------------------------------
    # Three modes:
    #   master_palette != None  -> caller supplies the 15-colour
    #     palette directly.  Every tile uses it.  This is the path
    #     for animation frames that must share a palette to avoid
    #     per-frame flicker — see derive_master_sprite_palette().
    #   asset_type == "sprite"  -> derive ONE 15-colour palette from
    #     all opaque pixels in this image and assign it to every
    #     tile.  Single-frame sprites + UI elements use this path
    #     so the result has zero per-tile palette drift.
    #   asset_type == "background"  -> derive a fresh 15-colour
    #     palette per 16x16 tile (the original high-fidelity HD
    #     path).  Requires palette dedup downstream — see
    #     cluster_and_remap_tile_palettes().
    # All modes feed a Lab-space k-means++ via PIL's LAB conversion;
    # transparent pixels (alpha < 128) are excluded from clustering
    # so the palette isn't biased by under-the-mask hidden colours.
    local_luts = {}

    if master_palette is not None:
        shared = np.asarray(master_palette, dtype=np.float32)
        if shared.shape != (15, 3):
            raise ValueError(
                f"master_palette must be shape (15, 3); got {shared.shape}")
        for ty_idx in range(tile_rows):
            for tx_idx in range(tile_cols):
                local_luts[(ty_idx, tx_idx)] = shared

    elif asset_type == "sprite":
        # Single global palette over all opaque pixels of this image.
        if opaque_mask.any():
            palette_mask = solid_mask if solid_mask.any() else opaque_mask
            opaque_rgb = rgb_np[palette_mask].astype(np.uint8)
            master_rgb = _lab_kmeans_palette_with_anchors(
                opaque_rgb, n_colors=15, asset_type="sprite", pre_toned=True
            ).astype(np.float32)
        else:
            master_rgb = np.zeros((15, 3), dtype=np.float32)
        for ty_idx in range(tile_rows):
            for tx_idx in range(tile_cols):
                local_luts[(ty_idx, tx_idx)] = master_rgb

    else:  # asset_type == "background"
        # Context padding (4px) + global dominant sample so adjacent 16x16 tiles
        # produce aligned palette candidates and smooth gradients across tile seams.
        pad = 4
        padded_rgb = np.pad(rgb_np, ((pad, pad), (pad, pad), (0, 0)), mode="edge")
        padded_mask = np.pad(opaque_mask, ((pad, pad), (pad, pad)), mode="edge")

        # Global dominant colors across the entire background image
        global_dominant = None
        if opaque_mask.any():
            all_opq = rgb_np[opaque_mask].astype(np.uint8)
            if all_opq.shape[0] > 8000:
                rng = np.random.RandomState(42)
                all_opq = all_opq[rng.choice(all_opq.shape[0], 8000, replace=False)]
            global_dominant = _lab_kmeans_palette_with_anchors(
                all_opq, n_colors=15, asset_type="background", pre_toned=True
            ).astype(np.float32)

        for ty_idx in range(tile_rows):
            for tx_idx in range(tile_cols):
                y, x = ty_idx * 16, tx_idx * 16
                tile_alpha = opaque_mask[y:y + 16, x:x + 16]

                if not tile_alpha.any():
                    # Fully transparent tile — no opaque pixels to cluster.
                    local_luts[(ty_idx, tx_idx)] = np.zeros((15, 3),
                                                              dtype=np.float32)
                    continue

                center_rgb = rgb_np[y:y + 16, x:x + 16][tile_alpha].astype(np.uint8)

                # Sample context window (24x24)
                py, px = y + pad, x + pad
                ctx_rgb = padded_rgb[py - pad:py + 16 + pad, px - pad:px + 16 + pad]
                ctx_mask = padded_mask[py - pad:py + 16 + pad, px - pad:px + 16 + pad]
                ctx_opq = ctx_rgb[ctx_mask].astype(np.uint8)

                # Center tile pixels get double weight so local details are sharp,
                # while context pixels ensure smooth palette transitions at edges.
                if ctx_opq.shape[0] > 0:
                    combined = np.vstack([center_rgb, center_rgb, ctx_opq])
                else:
                    combined = center_rgb

                local_pal = _lab_kmeans_palette_with_anchors(
                    combined, n_colors=15, asset_type="background",
                    pre_toned=True
                ).astype(np.float32)

                if local_pal.shape[0] < 15 and global_dominant is not None:
                    needed = 15 - local_pal.shape[0]
                    local_pal = np.vstack([local_pal, global_dominant[:needed]])

                local_luts[(ty_idx, tx_idx)] = local_pal[:15]

    # ---------------------------------------------------------
    # PASS 1b: RE-FIT EVERY PALETTE ONTO THE HARDWARE COLOUR GRID
    # ---------------------------------------------------------
    # The k-means above works in continuous Lab and the result is snapped
    # to the Neo Geo 5-bit grid on the way out, which leaves each entry up
    # to half a step away from the mean of the pixels it represents - and
    # nothing re-fits it afterwards.  refine_palette_on_lattice() runs the
    # update step again with the snap inside the loop, so the palette that
    # ships is optimal for the colours the hardware can actually show, and
    # it re-seeds slots that snapping collapsed into duplicates.
    #
    # Skipped when the caller supplied a master palette: those frames must
    # keep a byte-identical palette to avoid flicker between them, so the
    # refit belongs to whoever derived the master, not to each frame.
    if master_palette is None:
        refit_cache: dict = {}
        for key, pal in local_luts.items():
            pal_u8 = pal.astype(np.uint8)
            ck = pal_u8.tobytes()
            if asset_type == "sprite":
                # One palette shared by every tile - refit once against
                # the whole image rather than once per tile.
                if ck not in refit_cache:
                    px = rgb_np[solid_mask if solid_mask.any() else opaque_mask]
                    refit_cache[ck] = refine_palette_on_lattice(
                        pal_u8, px.astype(np.uint8))
                local_luts[key] = refit_cache[ck].astype(np.float32)
            else:
                ty_idx, tx_idx = key
                y, x = ty_idx * 16, tx_idx * 16
                m = opaque_mask[y:y + 16, x:x + 16]
                if not m.any():
                    continue
                px = rgb_np[y:y + 16, x:x + 16][m].astype(np.uint8)
                local_luts[key] = refine_palette_on_lattice(
                    pal_u8, px).astype(np.float32)

    # Matching happens in the same perceptually-weighted space the palette
    # was clustered in.  It used to be plain RGB Euclidean while the
    # clustering was Lab, so pixels were routinely assigned to a different
    # entry than the one their cluster had been built around - which shows
    # up as colour noise inside otherwise flat regions.
    local_ycc = {key: lab_palette(pal.astype(np.uint8))
                 for key, pal in local_luts.items()}
    lab_lut = lab_lattice_lut()

    # Largest gap between two palette entries that is still worth mixing.
    #
    # Ordered dithering between the two nearest entries only reads as an
    # intermediate colour when those entries are close together.  Mix two
    # entries that are far apart and the eye resolves them separately, so
    # a gradient turns into visible two-colour speckle - which is exactly
    # how the first version of this pass regressed sprite quality.  The
    # limit is derived per palette from its own spacing (twice the median
    # nearest-neighbour distance) rather than being a fixed constant, so
    # a tightly-packed palette mixes freely and a sparse one mostly snaps.
    local_mix_limit = {}
    for key, ycc in local_ycc.items():
        dd = np.sum((ycc[:, None, :] - ycc[None, :, :]) ** 2, axis=2)
        np.fill_diagonal(dd, np.inf)
        nn = dd.min(axis=1)
        nn = nn[np.isfinite(nn)]
        local_mix_limit[key] = (4.0 * float(np.median(nn))) if nn.size else 0.0

    # ---------------------------------------------------------
    # PASS 2: SERPENTINE DITHER (alpha-aware)
    # ---------------------------------------------------------
    # Two regimes, chosen per pixel by local turbulence:
    #
    #   Detail  - Floyd-Steinberg error diffusion, scanned serpentine.
    #             Alternating the scan direction each row cancels the
    #             directional bias that makes single-direction diffusion
    #             grow diagonal "worms" across large areas.
    #
    #   Smooth  - ordered dithering between the two nearest palette
    #             entries, thresholded by a blue-noise mask.  The old
    #             pipeline turned dithering OFF here, which is what put
    #             hard bands across every sky and every gradient: with
    #             fifteen colours over a whole image there is simply not
    #             an entry close enough to snap to.  Mixing the two
    #             nearest entries at the ratio the true colour sits
    #             between them reproduces the gradient, and doing it
    #             against blue noise rather than a Bayer matrix keeps the
    #             mix from reading as a pattern.
    #
    # Error is still accumulated in RGB - the canvas is RGB - while the
    # nearest-entry search runs in weighted YCbCr.
    error_canvas = np.pad(rgb_np, ((0, 2), (0, 2), (0, 0)), mode='edge')
    global_indices = np.full((target_h, target_w), 255, dtype=np.uint8)
    # Sentinel 255 = transparent.  Opaque pixels get 0..14, then the
    # +1 shift in Pass 3 maps them to NeoGeo slots 1..15.

    noise = blue_noise_mask()
    noise_n = noise.shape[0]
    mix_ok = (SPRITE_ORDERED_MIX if asset_type == "sprite" else True)
    smooth_cut = (SPRITE_SMOOTH_CUT if asset_type == "sprite"
                  else BACKGROUND_SMOOTH_CUT)

    for gy in range(target_h):
        left_to_right = (gy & 1) == 0
        xs = range(target_w) if left_to_right else range(target_w - 1, -1, -1)
        for gx in xs:
            # Alpha skip: transparent source pixels short-circuit straight
            # to the transparency sentinel.  Error diffused INTO a
            # transparent pixel just sits in the canvas unread.
            if not opaque_mask[gy, gx]:
                continue

            ty_idx = gy // 16
            tx_idx = gx // 16
            pal_rgb = local_luts[(ty_idx, tx_idx)]
            pal_ycc = local_ycc[(ty_idx, tx_idx)]
            mix_limit = local_mix_limit[(ty_idx, tx_idx)]

            px = np.clip(error_canvas[gy, gx, :], 0.0, 255.0)
            orig_px = rgb_np[gy, gx]

            # Turbulence from the (lightly denoised) ORIGINAL source, not
            # the error-laden canvas, so inherited error cannot promote a
            # flat area into the detail branch.
            variance = 0.0
            if gx > 0 and gy > 0:
                left = rgb_for_turbulence[gy, gx - 1]
                top = rgb_for_turbulence[gy - 1, gx]
                orig_t = rgb_for_turbulence[gy, gx]
                variance = (abs(orig_t[0] - left[0]) +
                             abs(orig_t[1] - left[1]) +
                             abs(orig_t[2] - left[2]) +
                             abs(orig_t[0] - top[0]) +
                             abs(orig_t[1] - top[1]) +
                             abs(orig_t[2] - top[2]))

            # A sprite's semi-transparent contour pixels keep the clean
            # source colour: they are anti-aliasing, and diffusing error
            # along an outline smears it.
            edge_mode = (asset_type == "sprite" and alpha_np[gy, gx] < 250.0)
            smooth_mode = edge_mode or variance < smooth_cut

            if smooth_mode:
                q = np.clip(orig_px, 0.0, 255.0).astype(np.uint8) >> 2
                target = lab_lut[q[0], q[1], q[2]]
                d = np.sum((pal_ycc - target) ** 2, axis=1)
                order = np.argsort(d)
                a = int(order[0])
                best_idx = a
                # Only photographic / painted material gets the ordered
                # mix.  Sprite art is cel-shaded: its flat fields are flat
                # on purpose, and dithering them adds noise the artist did
                # not put there.
                if (mix_ok and not edge_mode and order.shape[0] > 1):
                    b = int(order[1])
                    seg = pal_ycc[b] - pal_ycc[a]
                    seg2 = float(seg @ seg)
                    if 1e-6 < seg2 <= mix_limit:
                        # How far along a->b the true colour actually sits.
                        t = float((target - pal_ycc[a]) @ seg) / seg2
                        # Only mix when the colour genuinely sits BETWEEN
                        # the two entries.  Without the dead band, a pixel
                        # that the nearest entry already reproduces almost
                        # exactly still flips to the neighbour a fifth of
                        # the time, and those isolated flips are what read
                        # as speckle across an otherwise clean surface.
                        if 0.18 < t < 0.82:
                            if t > noise[gy % noise_n, gx % noise_n]:
                                best_idx = b
                global_indices[gy, gx] = best_idx
                continue

            q = px.astype(np.uint8) >> 2
            target = lab_lut[q[0], q[1], q[2]]
            d = np.sum((pal_ycc - target) ** 2, axis=1)
            best_idx = int(np.argmin(d))
            global_indices[gy, gx] = best_idx

            # Floyd-Steinberg 7/3/5/1 with a 0.75 leak dampener, mirrored
            # when the row runs right-to-left.
            #
            # The dampener is ramped in over the turbulence just above the
            # cut rather than applied at full strength the moment a pixel
            # crosses it.  With a hard switch, two regimes end up side by
            # side inside one surface - a pixel a hair over the line pushes
            # three quarters of its error into its neighbours while the one
            # next to it pushes none - and that interleaving is itself the
            # speckle the split was meant to prevent.  Ramping means a
            # barely-textured area diffuses barely anything, so it stays as
            # flat as the artist drew it, while real detail still gets the
            # full treatment.
            ramp = (variance - smooth_cut) / (smooth_cut * 1.5)
            if ramp > 1.0:
                ramp = 1.0
            quant_error = (px - pal_rgb[best_idx]) * (0.75 * ramp)
            step = 1 if left_to_right else -1
            nx = gx + step
            if 0 <= nx < target_w:
                error_canvas[gy, nx, :] += quant_error * (7.0 / 16.0)
            bx = gx - step
            if 0 <= bx < target_w:
                error_canvas[gy + 1, bx, :] += quant_error * (3.0 / 16.0)
            error_canvas[gy + 1, gx, :] += quant_error * (5.0 / 16.0)
            if 0 <= nx < target_w:
                error_canvas[gy + 1, nx, :] += quant_error * (1.0 / 16.0)

    # ---------------------------------------------------------
    # PASS 3: SLICE BACK TO NEO GEO HARDWARE BLOCKS
    # Transparent sentinel 255 -> NeoGeo slot 0; opaque 0..14 -> 1..15.
    # ---------------------------------------------------------
    # Pre-allocate output lists by exact tile count to avoid
    # repeated list.append() reallocations inside the inner loop —
    # tile_rows / tile_cols computed once at the top of the function.
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
_VIVID_CACHE_VERSION = "v18-hd-bilateral-clahe-context-global"


def _vivid_cache_key(image_path: str, target_w: int, target_h: int,
                      fit: str, anchor: str, n_colors: int,
                      asset_type: str = "background",
                      master_palette: np.ndarray | None = None,
                      shrink_y: int | None = None) -> str:
    """
    SHA256 of source bytes + params + pipeline version tag.

    asset_type and master_palette participate so background and
    sprite conversions of the same source PNG live in separate
    cache entries; a per-frame call with a shared master palette
    keys on the palette's bytes so reusing the same master across
    frames produces deterministic cache hits.
    """
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
              f"{asset_type}|{shrink_y}|{_VIVID_CACHE_VERSION}".encode())
    if master_palette is not None:
        h.update(b"|master:")
        h.update(np.ascontiguousarray(
            master_palette, dtype=np.uint8).tobytes())
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


def convert_sprite_via_vivid_pipeline(
        image_path,
        target_w: int = 64,
        target_h: int = 64,
        fit: str = "contain",
        anchor: str = "center",
        n_colors: int = COLORS_PER_TILE,
        master_palette: np.ndarray | None = None,
        **kwargs,
        ) -> tuple[np.ndarray, np.ndarray, dict]:
    """
    Sprite-mode wrapper: routes through execute_final_vivid_pipeline
    with asset_type="sprite" so the whole image locks to a single
    15-colour palette.  Use master_palette=<derived> to share one
    palette across every frame of an animation sequence so there
    is zero per-frame palette drift (no shimmer).

    Returns the same (indexed_uint16, palette16_uint16, metadata)
    contract as convert_screen_via_vivid_pipeline so callers can
    treat sprites and screens identically downstream.

    For sprites the single-bank collapse is trivial — the master
    palette IS the only palette — so the global-palette derivation
    step is skipped.  The dither pattern uses the per-pixel scanline
    FS with the same Teflon Routing for high-contrast UI elements.
    """
    target_w = max(16, (target_w // 16) * 16)
    target_h = max(16, (target_h // 16) * 16)

    cache_key = _vivid_cache_key(str(image_path), target_w, target_h,
                                   fit, anchor, n_colors,
                                   asset_type="sprite",
                                   master_palette=master_palette)
    cached = _vivid_cache_load(cache_key)
    if cached is not None:
        return cached

    ready_tiles, final_palettes = execute_final_vivid_pipeline(
        image_path, target_w, target_h,
        asset_type="sprite",
        master_palette=master_palette,
    )

    tile_cols = target_w // 16
    tile_rows = target_h // 16
    n_tiles = len(ready_tiles)

    # Single palette across the whole sprite — every tile shares
    # final_palettes[0].  Pack into the (16, 3) NeoGeo bank shape.
    sprite_palette = np.array(final_palettes[0],
                                dtype=np.uint8).reshape(15, 3)
    palette16 = np.zeros((16, 3), dtype=np.uint16)
    palette16[1:1 + n_colors] = sprite_palette.astype(np.uint16)

    # Assemble the (H, W) indexed buffer directly — no second
    # dither pass needed since every tile already references the
    # same palette.
    indexed = np.zeros((target_h, target_w), dtype=np.uint16)
    for ti in range(n_tiles):
        ty, tx = divmod(ti, tile_cols)
        y0, x0 = ty * 16, tx * 16
        tile_indices = np.array(ready_tiles[ti],
                                  dtype=np.uint8).reshape(16, 16)
        indexed[y0:y0 + 16, x0:x0 + 16] = tile_indices.astype(np.uint16)

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
        "pipeline":       "vivid_sprite_single_palette",
        "cache_version":  _VIVID_CACHE_VERSION,
        "master_palette_supplied": master_palette is not None,
    }
    _vivid_cache_save(cache_key, indexed, palette16, metadata)
    return indexed, palette16, metadata


def convert_screen_via_vivid_pipeline(
        image_path,
        target_w: int = 320,
        target_h: int = 224,
        fit: str = "contain",
        anchor: str = "center",
        n_colors: int = COLORS_PER_TILE,
        display_shrink_y: int | None = None,
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
                                   fit, anchor, n_colors,
                                   asset_type="background",
                                   shrink_y=display_shrink_y)
    cached = _vivid_cache_load(cache_key)
    if cached is not None:
        return cached

    # A screen gets ONE palette bank on the hardware, so it gets one
    # palette here, derived from the pixels and applied in a single
    # quantisation.
    #
    # This used to run the per-tile quantiser first, rebuild an RGB canvas
    # out of its output, cluster a global palette from the per-tile
    # PALETTES, and then dither that rebuilt canvas a second time.  Three
    # things went wrong with it.  Quantising twice meant the second pass
    # was dithering a picture that was already dithered, so its own error
    # diffusion piled noise on top of noise instead of on top of the
    # photograph.  Clustering over palette entries threw away how many
    # pixels each colour actually covered - a shade held by one pixel in
    # one tile counted for exactly as much as the sky - which is the one
    # weight a palette must respect.  And the second dither matched in
    # plain RGB while the palette had been clustered in Lab, the same
    # mismatch that puts colour noise into flat regions.
    tile_cols = target_w // 16
    tile_rows = target_h // 16
    n_tiles = tile_cols * tile_rows

    # When the caller knows how the game draws this screen, fit to the shape
    # it will be seen in rather than to the square canvas it is stored in.
    # Without it the source is simply stretched to 256x256 and then squashed
    # again by the hardware on the way to the screen, and a backdrop drawn
    # wider than it is tall arrives noticeably too tall and too narrow.
    src = Image.open(image_path)
    if display_shrink_y is None:
        src = resize_rgba_linear(src.convert("RGBA"), target_w, target_h)
        content_left, content_top = 0, 0
        content_w, content_h = target_w, target_h
    else:
        src, content_left, content_top, content_w, content_h = (
            fit_screen_for_display(src, target_w, target_h,
                                   shrink_y=display_shrink_y,
                                   fit=fit, anchor=anchor))
    rgba = np.array(src, dtype=np.uint8)

    # Cluster on the toned pixels, because the toned canvas is what the
    # dither below has to reproduce.
    opaque = rgba[:, :, 3] >= ALPHA_OPAQUE_THRESHOLD
    toned = _neo_palette_pop(rgba[:, :, :3], opaque, asset_type="background")
    pixels = toned[opaque] if opaque.any() else toned.reshape(-1, 3)

    global_palette = _lab_kmeans_palette_with_anchors(
        pixels, n_colors=n_colors, asset_type="background", pre_toned=True)
    global_palette = refine_palette_on_lattice(global_palette, pixels)

    master = np.zeros((COLORS_PER_TILE, 3), dtype=np.float32)
    fill = min(COLORS_PER_TILE, global_palette.shape[0])
    master[:fill] = global_palette[:fill]
    if fill < COLORS_PER_TILE:
        master[fill:] = global_palette[fill - 1]

    ready_tiles, _ = _vivid_pipeline_from_rgba(
        rgba.astype(np.float32),
        asset_type="background",
        master_palette=master,
    )

    indexed = np.zeros((target_h, target_w), dtype=np.uint16)
    for ti, tile_row in enumerate(ready_tiles):
        ty, tx = divmod(ti, tile_cols)
        indexed[ty * 16:(ty + 1) * 16, tx * 16:(tx + 1) * 16] = np.array(
            tile_row, dtype=np.uint8).reshape(16, 16)
    global_palette = master.astype(np.uint8)

    palette16 = np.zeros((16, 3), dtype=np.uint16)
    palette16[1:1 + COLORS_PER_TILE] = global_palette.astype(np.uint16)

    metadata = {
        "source_width":   int(target_w),
        "source_height":  int(target_h),
        "canvas_width":   int(target_w),
        "canvas_height":  int(target_h),
        "content_left":   int(content_left),
        "content_top":    int(content_top),
        "content_width":  int(content_w),
        "content_height": int(content_h),
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
    return ng_snap(lab_to_rgb(centres_lab))


def convert_screen_via_tile_palette(
        image_path: str | os.PathLike,
        target_w: int | None = 320,
        target_h: int | None = 224,
        fit: str = "contain",
        anchor: str = "center",
        n_colors: int = COLORS_PER_TILE,
        alpha_threshold: int = ALPHA_OPAQUE_THRESHOLD,
        max_banks: int = 32,
        epsilon: float = 6.0,
        context_pad: int = 4,
        gamma: float = 1.20,
        contrast: float = 1.10,
        ) -> tuple[np.ndarray, np.ndarray, dict]:
    """
    Drop-in replacement for `romdbimgimport.load_screen_asset`'s
    full-image quantizer.

    Pipeline (eliminates tile-boundary seams by combining context-
    window palette derivation with a single image-wide dither pass):

      1. Linear-light resize / pad source to a 16-aligned canvas, then
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
    rgb_corrected = ng_snap((rgb_f * 255.0).astype(np.uint8))
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
    rep_palette = ng_snap(lab_to_rgb(centres))         # (15, 3) u8
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
        "epsilon_de":            float(epsilon),
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

    Strategy: greedy-cluster tiles whose palettes fall within `lab_eps`
    of an existing bank under _palette_set_distance(), then re-fit each
    bank to every colour its members hold.

    An earlier version compared the MEAN Lab of a palette's 15 colours.
    That is not a comparison of palettes: a tile carrying a red and a
    blue gradient averages to purple, a tile of flat purple averages to
    the same purple, and the two get merged - taking the gradient with
    them.  Averaging discards exactly the structure the bank has to
    reproduce, so the distance looks at where the colours actually sit.

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

    eps = float(lab_eps)
    for _attempt in range(8):
        bank_lab: list[np.ndarray] = []
        bank_members: list[list[int]] = []
        tile_to_bank = np.full(n_tiles, -1, dtype=np.int32)

        for ti in range(n_tiles):
            if not bank_lab:
                bank_lab.append(palette_lab[ti].copy())
                bank_members.append([ti])
                tile_to_bank[ti] = 0
                continue
            d = _palette_set_distance(np.stack(bank_lab, axis=0),
                                      palette_lab[ti])
            best = int(d.argmin())
            if d[best] <= eps:
                bank_members[best].append(ti)
                tile_to_bank[ti] = best
            else:
                bank_lab.append(palette_lab[ti].copy())
                bank_members.append([ti])
                tile_to_bank[ti] = len(bank_lab) - 1

        if len(bank_lab) <= max_banks:
            break
        eps *= 1.5
    else:
        raise RuntimeError(f"Could not fit palette banks under "
                           f"{max_banks} even at eps={eps:.1f}")

    # Re-fit each bank to every colour its members hold, rather than
    # keeping the palette of whichever tile reached it first.
    bank_lab_arr = np.stack(bank_lab, axis=0)
    for bi, members in enumerate(bank_members):
        if len(members) <= 1:
            continue
        bank_lab_arr[bi] = _refit_bank(
            palette_lab[members].reshape(-1, 3), bank_lab_arr[bi])

    bank_palettes = ng_snap(lab_to_rgb(bank_lab_arr))
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
    ap.add_argument("--no-enhance", action="store_true",
                    help="Skip the HD conditioning before quantisation "
                         "(bilateral smooth then lightness CLAHE).  It is on "
                         "by default because the source art here is HD; turn "
                         "it off for art drawn at the target size that does "
                         "not want its tone touched.  Same as "
                         "ARTBOX_ENHANCE=0.")
    args = ap.parse_args()

    if args.no_enhance:
        os.environ["ARTBOX_ENHANCE"] = "0"

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
