#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
img2neo_crt.py — CRT-optimized PNG -> NeoGeo C-ROM converter.

Produces three artefacts from a single PNG:

  1. A swizzled C-ROM byte stream (16x16 tiles, Z-pattern sub-blocks,
     reversed-row 4bpp planar) ready for direct ROM packing.
  2. A 15-entry hardware-native RGB palette (index 0 = transparent in the
     packed indexed buffer; the returned palette covers indices 1..15).
  3. A metadata dictionary describing original-content clipping borders so
     downstream asset packers can re-center the image on screen.

Visual pipeline (applied to opaque pixels before quantisation):

  - Gamma-Corrected Scaling     : compensates for CRT EOTF (default 1.20)
  - Perceptual Contrast Boost   : 1.10x linear gain after gamma
  - Spatial CIE-Lab Quantisation: k-means++ palette in Lab space
  - Horizontal-Biased Dither    : modified Floyd-Steinberg that pushes
                                  most of the error rightward (8/2 on the
                                  current row, 2/4 down) to leverage the
                                  CRT scanline's natural horizontal blur.

The module is self-contained on numpy + Pillow + pypng (the same footprint
used by the rest of artbox/).  CIE-Lab + k-means++ helpers are reused from
img2neo.py to avoid pulling in scikit-image / scikit-learn.

Two entry points:

    convert_png_to_neogeo_arcade(path, ...)
        Returns (c_rom_bytes, palette_rgb_uint8, metadata_dict).
        Matches the production reference contract.

    convert_screen_to_indexed(path, ...)
        Returns (indexed_uint16, palette16_uint16, metadata_dict) using the
        same 16-entry layout (index 0 = transparent) used by load_screen_asset
        in romdbimgimport.py, so the new pipeline can be slotted into the
        existing artbox driver chain unchanged.

CLI:

    python3 img2neo_crt.py src.png dst.png [-W 256 -H 224 -c 15]
        Writes an indexed PNG identical in layout to img2neo_hd.py output
        (palette index 0 transparent) AND, when --emit-crom is given, also
        writes <dst>.crom + <dst>.pal.json + <dst>.meta.json sidecars for
        direct ROM packing.
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

from img2neo import snap_neogeo, rgb_to_lab, lab_to_rgb, kmeans_palette, alpha_bleed


# ---------------------------------------------------------------------------
# CRT pre-quantisation tone curve
# ---------------------------------------------------------------------------

def apply_crt_tone(rgb_float01: np.ndarray,
                   gamma: float = 1.20,
                   contrast: float = 1.10) -> np.ndarray:
    """
    Pre-bake the CRT EOTF compensation + a small linear contrast boost.

    On a real arcade CRT the displayed luminance is roughly L = V^2.2, so
    quantising in linear sRGB makes shadow detail clump.  Raising the input
    to a power slightly above 1 ('gamma') counteracts that bias before
    palette selection.  A subsequent 1.10x multiply restores punch lost
    when the CRT compresses bright highlights.

    Operates on float arrays in 0..1.  Output is clipped to 0..1.
    """
    out = np.power(np.clip(rgb_float01, 0.0, 1.0), gamma)
    out = np.clip(out * contrast, 0.0, 1.0)
    return out


# ---------------------------------------------------------------------------
# Horizontal-biased Floyd-Steinberg in CIE-Lab space
# ---------------------------------------------------------------------------

# Diffusion weights chosen for CRT scanlines:
#   - the current row gets 10/16 of the error (8 right, 2 two-right) so the
#     beam's natural horizontal smear blurs the dither pattern;
#   - the next row gets only 6/16 (2 down-left, 4 down) so vertical noise is
#     suppressed (interleaving makes vertical artefacts very visible on CRTs).
_DIFF_X_OFFSETS = ((0,  1, 8 / 16.0),
                   (0,  2, 2 / 16.0),
                   (1, -1, 2 / 16.0),
                   (1,  0, 4 / 16.0))


def horizontal_biased_dither_lab(lab_image: np.ndarray,
                                 palette_lab: np.ndarray,
                                 alpha_mask: np.ndarray) -> np.ndarray:
    """
    Floyd-Steinberg dithering in CIE-Lab space with a horizontal bias kernel.

    Parameters
    ----------
    lab_image    : (H, W, 3) float32 Lab pixels (gamma-boosted source)
    palette_lab  : (N, 3) float32 Lab palette entries (N == n_colors)
    alpha_mask   : (H, W) bool, True where the pixel is opaque

    Returns
    -------
    (H, W) uint8 — palette index in 1..N for opaque pixels, 0 for transparent.
    Caller must keep palette[0] reserved as transparent black.
    """
    h, w = alpha_mask.shape
    buf = lab_image.astype(np.float32).copy()
    out = np.zeros((h, w), dtype=np.uint8)

    for y in range(h):
        row_alpha = alpha_mask[y]
        for x in range(w):
            if not row_alpha[x]:
                out[y, x] = 0
                continue
            current = buf[y, x]
            dist = np.sum((palette_lab - current) ** 2, axis=1)
            best = int(np.argmin(dist))
            out[y, x] = best + 1
            err = current - palette_lab[best]
            for dy, dx, weight in _DIFF_X_OFFSETS:
                ny, nx = y + dy, x + dx
                if 0 <= ny < h and 0 <= nx < w and alpha_mask[ny, nx]:
                    buf[ny, nx] += err * weight
    return out


# ---------------------------------------------------------------------------
# Neo Geo C-ROM swizzler
# ---------------------------------------------------------------------------

def pack_indexed_to_crom(indexed: np.ndarray) -> bytes:
    """
    Pack an indexed (H, W) uint8 image into Neo Geo C-ROM byte order.

    Layout (per 16x16 tile):
      - four 8x8 sub-blocks read in Z-order: TL, BL, TR, BR
      - within each sub-block, 8 rows top-to-bottom
      - each row is emitted as 4 bytes (bit-planes 0..3)
      - within a bit-plane byte, pixel column 7 is the MSB (reversed read)

    Input H and W must be multiples of 16.
    """
    h, w = indexed.shape
    if (h % 16) or (w % 16):
        raise ValueError(f"indexed shape ({h}x{w}) must be multiples of 16")
    if indexed.dtype != np.uint8:
        indexed = indexed.astype(np.uint8)

    stream = bytearray()
    for tile_y in range(0, h, 16):
        for tile_x in range(0, w, 16):
            sub_origins = (
                (tile_y,     tile_x),       # TL
                (tile_y + 8, tile_x),       # BL
                (tile_y,     tile_x + 8),   # TR
                (tile_y + 8, tile_x + 8),   # BR
            )
            for y0, x0 in sub_origins:
                block = indexed[y0:y0 + 8, x0:x0 + 8] & 0x0F
                for row in range(8):
                    bp0 = bp1 = bp2 = bp3 = 0
                    for col in range(7, -1, -1):
                        v = int(block[row, col])
                        bp0 = (bp0 << 1) | (v        & 1)
                        bp1 = (bp1 << 1) | ((v >> 1) & 1)
                        bp2 = (bp2 << 1) | ((v >> 2) & 1)
                        bp3 = (bp3 << 1) | ((v >> 3) & 1)
                    stream.append(bp0)
                    stream.append(bp1)
                    stream.append(bp2)
                    stream.append(bp3)
    return bytes(stream)


# ---------------------------------------------------------------------------
# Top-level conversion
# ---------------------------------------------------------------------------

def _pad_to_tile_grid(img_rgba: Image.Image,
                      fit_strategy: str = "contain"
                      ) -> tuple[Image.Image, int, int, int, int]:
    """
    Center-pad an RGBA image up to the next 16-pixel multiple in each axis.
    Returns (padded_img, content_left, content_top, content_w, content_h).
    fit_strategy == "topleft" anchors at (0,0); anything else centers.
    """
    orig_w, orig_h = img_rgba.size
    target_w = ((orig_w + 15) // 16) * 16
    target_h = ((orig_h + 15) // 16) * 16
    padded = Image.new("RGBA", (target_w, target_h), (0, 0, 0, 0))
    if fit_strategy == "contain":
        ox = (target_w - orig_w) // 2
        oy = (target_h - orig_h) // 2
    else:
        ox = 0
        oy = 0
    padded.paste(img_rgba, (ox, oy))
    return padded, ox, oy, orig_w, orig_h


def convert_png_to_neogeo_arcade(image_path: str | os.PathLike,
                                 fit_strategy: str = "contain",
                                 n_colors: int = 15,
                                 gamma: float = 1.20,
                                 contrast: float = 1.10,
                                 alpha_threshold: int = 128
                                 ) -> tuple[bytes, np.ndarray, dict]:
    """
    Convert a PNG into ready-to-pack Neo Geo arcade data.

    Returns
    -------
    c_rom_bytes : bytes
        Swizzled C-ROM stream (length = tiles * 128 bytes).
    palette_rgb : np.ndarray, shape (n_colors, 3), dtype uint8
        Hardware-native RGB palette (already snapped to the NeoGeo 5-bit
        grid).  Caller is responsible for prepending a transparent entry
        when wrapping this into a 16-entry NeoGeo palette.
    metadata : dict
        {"content_left", "content_top", "content_width", "content_height",
         "canvas_width", "canvas_height", "n_colors", "tile_cols",
         "tile_rows", "gamma", "contrast"}.
    """
    img = Image.open(image_path).convert("RGBA")
    padded, ox, oy, ow, oh = _pad_to_tile_grid(img, fit_strategy)
    img_np = alpha_bleed(np.array(padded, dtype=np.uint8),
                         opaque_alpha=max(alpha_threshold, 128))
    target_h, target_w = img_np.shape[:2]

    alpha = img_np[:, :, 3]
    transparent_mask = alpha < alpha_threshold
    opaque_mask = ~transparent_mask

    rgb_f = img_np[:, :, :3].astype(np.float32) / 255.0
    rgb_f = apply_crt_tone(rgb_f, gamma=gamma, contrast=contrast)

    # Snap to the NeoGeo 5-bit grid in uint8 space so palette candidates
    # and dithered pixels share the exact same colour lattice.
    rgb_u8 = snap_neogeo((rgb_f * 255.0).astype(np.uint8))

    if opaque_mask.any():
        palette_rgb = kmeans_palette(rgb_u8[opaque_mask], n=n_colors)
    else:
        palette_rgb = np.zeros((n_colors, 3), dtype=np.uint8)

    lab_image = rgb_to_lab(rgb_u8)
    palette_lab = rgb_to_lab(palette_rgb)

    indexed = horizontal_biased_dither_lab(lab_image, palette_lab, opaque_mask)

    c_rom_bytes = pack_indexed_to_crom(indexed)

    metadata = {
        "content_left":   int(ox),
        "content_top":    int(oy),
        "content_width":  int(ow),
        "content_height": int(oh),
        "canvas_width":   int(target_w),
        "canvas_height":  int(target_h),
        "n_colors":       int(n_colors),
        "tile_cols":      int(target_w // 16),
        "tile_rows":      int(target_h // 16),
        "gamma":          float(gamma),
        "contrast":       float(contrast),
    }
    return c_rom_bytes, palette_rgb, metadata


def convert_screen_to_indexed(image_path: str | os.PathLike,
                              target_w: int | None = None,
                              target_h: int | None = None,
                              fit: str = "contain",
                              anchor: str = "center",
                              n_colors: int = 15,
                              gamma: float = 1.20,
                              contrast: float = 1.10,
                              alpha_threshold: int = 16
                              ) -> tuple[np.ndarray, np.ndarray, dict]:
    """
    Drop-in replacement for the per-screen branch of romdbimgimport's
    load_screen_asset.  Returns a 16-entry palette where palette[0] is the
    sentinel transparent black, mirroring the contract of make_sprite_palette
    so the resulting (indexed, palette) tuple is interchangeable with the
    output of the existing pipeline.
    """
    img = Image.open(image_path).convert("RGBA")
    src_w, src_h = img.size

    # Resize-to-screen when target dimensions are supplied; otherwise leave
    # the original size and pad up to the next 16-pixel multiple.
    if target_w and target_h:
        tw = (target_w // 16) * 16
        th = (target_h // 16) * 16
        if (fit or "contain").lower() == "crop":
            w, h = img.size
            if h > 0 and w > 0:
                if (w / h) > (tw / th):
                    nw = int(h * tw / th)
                    left = (w - nw) // 2
                    img = img.crop((left, 0, left + nw, h))
                else:
                    nh = int(w * th / tw)
                    top = (h - nh) // 2
                    img = img.crop((0, top, w, top + nh))
                img = img.resize((tw, th), Image.LANCZOS)
        else:
            if img.width > tw or img.height > th:
                img.thumbnail((tw, th), Image.LANCZOS)
        canvas = Image.new("RGBA", (tw, th), (0, 0, 0, 0))
        cw, ch = img.size
        left = (tw - cw) // 2
        top = (th - ch) // 2
        if anchor == "top-center":
            top = 0
        elif anchor == "bottom-center":
            top = th - ch
        elif anchor == "left-center":
            left = 0
        elif anchor == "right-center":
            left = tw - cw
        if left < 0:
            left = 0
        if top < 0:
            top = 0
        canvas.alpha_composite(img, (left, top))
        content_left, content_top = left, top
        content_w, content_h = cw, ch
    else:
        canvas, content_left, content_top, content_w, content_h = _pad_to_tile_grid(img)
        tw, th = canvas.size

    img_np = alpha_bleed(np.array(canvas, dtype=np.uint8),
                         opaque_alpha=max(alpha_threshold, 128))
    alpha = img_np[:, :, 3]
    opaque_mask = alpha >= alpha_threshold

    rgb_f = img_np[:, :, :3].astype(np.float32) / 255.0
    rgb_f = apply_crt_tone(rgb_f, gamma=gamma, contrast=contrast)
    rgb_u8 = snap_neogeo((rgb_f * 255.0).astype(np.uint8))

    if opaque_mask.any():
        palette_rgb = kmeans_palette(rgb_u8[opaque_mask], n=n_colors)
    else:
        palette_rgb = np.zeros((n_colors, 3), dtype=np.uint8)

    lab_image = rgb_to_lab(rgb_u8)
    palette_lab = rgb_to_lab(palette_rgb)

    indexed8 = horizontal_biased_dither_lab(lab_image, palette_lab, opaque_mask)
    indexed = indexed8.astype(np.uint16)

    palette16 = np.zeros((16, 3), dtype=np.uint16)
    palette16[1:1 + n_colors] = palette_rgb.astype(np.uint16)

    metadata = {
        "source_width":   int(src_w),
        "source_height":  int(src_h),
        "canvas_width":   int(tw),
        "canvas_height":  int(th),
        "content_left":   int(content_left),
        "content_top":    int(content_top),
        "content_width":  int(content_w),
        "content_height": int(content_h),
    }
    return indexed, palette16, metadata


# ---------------------------------------------------------------------------
# CLI — produce an indexed PNG (+ optional raw C-ROM sidecars)
# ---------------------------------------------------------------------------

def _save_indexed_png(path: str, indexed: np.ndarray, palette_rgb_15: np.ndarray):
    """Write an indexed PNG with palette[0] = transparent black."""
    pal = [(0, 0, 0)] + [tuple(int(v) for v in c) for c in palette_rgb_15.tolist()]
    h, w = indexed.shape
    writer = png.Writer(width=w, height=h, bitdepth=8, palette=pal)
    rows = indexed.astype(np.uint8).tolist()
    with open(path, "wb") as f:
        writer.write(f, rows)


def _convert_cli(src: str, dst: str,
                 W: int | None, H: int | None,
                 n_colors: int, gamma: float, contrast: float,
                 emit_crom: bool):
    print(f"[crt] {os.path.basename(src)} -> {os.path.basename(dst)}  "
          f"(colors={n_colors}, gamma={gamma:.2f}, contrast={contrast:.2f})")
    indexed, palette16, meta = convert_screen_to_indexed(
        src, target_w=W, target_h=H, n_colors=n_colors,
        gamma=gamma, contrast=contrast,
    )
    palette15 = palette16[1:1 + n_colors].astype(np.uint8)
    _save_indexed_png(dst, indexed, palette15)
    print(f"      wrote {dst}  ({meta['canvas_width']}x{meta['canvas_height']})")

    if emit_crom:
        crom_path = dst + ".crom"
        pal_path = dst + ".pal.json"
        meta_path = dst + ".meta.json"
        with open(crom_path, "wb") as f:
            f.write(pack_indexed_to_crom(indexed.astype(np.uint8)))
        with open(pal_path, "w") as f:
            json.dump([list(map(int, c)) for c in palette15.tolist()], f, indent=2)
        with open(meta_path, "w") as f:
            json.dump(meta, f, indent=2)
        print(f"      wrote {crom_path} + {os.path.basename(pal_path)} + {os.path.basename(meta_path)}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n", 1)[0])
    ap.add_argument("src")
    ap.add_argument("dst")
    ap.add_argument("-W", "--width", type=int, default=None,
                    help="Target canvas width (multiple of 16). If omitted, "
                         "pad to next-multiple-of-16 of the source size.")
    ap.add_argument("-H", "--height", type=int, default=None,
                    help="Target canvas height (multiple of 16). If omitted, "
                         "pad to next-multiple-of-16 of the source size.")
    ap.add_argument("-c", "--colors", type=int, default=15,
                    help="Palette size (1..15). Default 15.")
    ap.add_argument("--gamma", type=float, default=1.20,
                    help="Pre-quantisation gamma (CRT EOTF compensation).")
    ap.add_argument("--contrast", type=float, default=1.10,
                    help="Linear contrast multiplier after gamma.")
    ap.add_argument("--emit-crom", action="store_true",
                    help="Also write .crom / .pal.json / .meta.json sidecars "
                         "for direct ROM packing.")
    args = ap.parse_args()

    if not 1 <= args.colors <= 15:
        ap.error("--colors must be between 1 and 15 inclusive")

    _convert_cli(args.src, args.dst,
                 args.width, args.height,
                 args.colors, args.gamma, args.contrast,
                 args.emit_crom)


if __name__ == "__main__":
    main()
