#!/usr/bin/env python3
"""
process_eyecatcher_ng.py — Pre-process eyecatcher PNGs for NeoGeo hardware colour accuracy.

NeoGeo palette: 12-bit colour, 4 bits per channel (0-15 levels each).
Running this BEFORE makeartbox.sh ensures the k-means quantiser in the artbox
pipeline has clean, hardware-aligned colour data to work from, avoiding the
per-tile banding that appears when photographic gradients are truncated.

Steps applied per image:
  1. Boost contrast and saturation (vivid arcade palette)
  2. Apply Bayer 4×4 ordered dithering to break up gradients at 4-bit boundary
  3. Quantise each channel to 16 levels (NeoGeo 4-bit colour space)
  4. Sharpen edges so tile boundaries are less visible at hardware resolution

Original files are backed up to *.orig before first run.
"""

import os
from PIL import Image, ImageEnhance, ImageFilter
import numpy as np

EYECATCHER_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                               "in", "eyecatcher")

# Bayer 4×4 matrix, normalised to 0..1
BAYER4 = np.array([
    [ 0,  8,  2, 10],
    [12,  4, 14,  6],
    [ 3, 11,  1,  9],
    [15,  7, 13,  5],
], dtype=np.float32) / 16.0


def bayer_dither_4bit(img: Image.Image) -> Image.Image:
    """Quantise each channel to 16 levels with Bayer ordered dithering."""
    arr = np.array(img.convert("RGB"), dtype=np.float32)
    h, w = arr.shape[:2]

    reps_y = h // 4 + 1
    reps_x = w // 4 + 1
    bayer  = np.tile(BAYER4, (reps_y, reps_x))[:h, :w]

    # Dither offset ±8 DN (half a 4-bit step = 255/16/2 ≈ 8)
    offset = (bayer - 0.5) * 16.0

    for ch in range(3):
        plane = arr[:, :, ch] + offset
        plane = np.clip(plane, 0, 255)
        plane = np.floor(plane / 16.0) * 16.0
        arr[:, :, ch] = plane

    return Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8))


def enhance(img: Image.Image) -> Image.Image:
    """Contrast, saturation and sharpness boost for arcade-vivid output."""
    img = ImageEnhance.Contrast(img).enhance(1.40)
    img = ImageEnhance.Color(img).enhance(1.40)
    img = ImageEnhance.Sharpness(img).enhance(2.0)
    return img


def main():
    pngs = sorted(f for f in os.listdir(EYECATCHER_DIR) if f.endswith(".png"))
    if not pngs:
        print("No PNGs found in", EYECATCHER_DIR)
        return

    for fname in pngs:
        path     = os.path.join(EYECATCHER_DIR, fname)
        orig     = path + ".orig"

        if not os.path.exists(orig):
            import shutil
            shutil.copy2(path, orig)
            print(f"  backed up → {fname}.orig")

        img = Image.open(orig).convert("RGB")
        img = enhance(img)
        img = bayer_dither_4bit(img)
        img.save(path)
        print(f"  processed  {fname}  {img.size[0]}×{img.size[1]}")

    print("\nDone. Run  make art && make  to rebuild C ROMs and P1 ROM.")


if __name__ == "__main__":
    main()
