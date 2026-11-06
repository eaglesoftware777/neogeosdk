"""The colour grade applied to palettes as they are emitted: livelier colour and
firmer light and shade than the painted sources give after reduction.

    grade(colors, strength) -> colors   (N x 3, 0-255; 1.0 for the fighters)

Saturation rises most where a colour is greyest (vibrance), while skin tones
preserve their hand-crafted antialiasing without harsh contrast crushing.
Near-black outlines and near-white highlights stay where they are.
"""
import colorsys

import numpy as np

VIBRANCE = 0.35      # added saturation on garments/metal, falling to none at full
CONTRAST = 0.22      # blend toward smoothstep(lightness) for clothing and scenery


def grade(colors, strength=1.0):
    src = np.asarray(colors, dtype=np.float64)
    out = src.copy()
    for i, (r, g, b) in enumerate(src / 255.0):
        h, l, s = colorsys.rgb_to_hls(r, g, b)
        if l < 0.05 or l > 0.96:
            continue
        # Comprehensive skin protection: covers warm peach, golden amber, olive, and deep bronze
        is_skin = (h <= 0.14 or h >= 0.95) and 0.10 <= l <= 0.90 and 0.10 <= s <= 0.80
        if is_skin:
            # Subtle natural warmth, minimal contrast warping to keep delicate facial antialiasing intact
            vib = 0.08
            s = min(1.0, s * (1.0 + vib * strength * (1.0 - s)))
            contrast = 0.04 * strength
        else:
            vib = VIBRANCE
            s = min(1.0, s * (1.0 + vib * strength * (1.0 - s)))
            contrast = CONTRAST * strength
        curve = l * l * (3.0 - 2.0 * l)
        l = l + (curve - l) * contrast
        out[i] = np.asarray(colorsys.hls_to_rgb(h, l, s)) * 255.0
    return np.clip(np.rint(out), 0, 255).astype(np.uint8)

