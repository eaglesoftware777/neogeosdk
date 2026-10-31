"""The colour grade applied to palettes as they are emitted: livelier colour and
firmer light and shade than the painted sources give after reduction.

    grade(colors, strength) -> colors   (N x 3, 0-255; 1.0 for the fighters)

Saturation rises most where a colour is greyest (vibrance), less on skin,
and mid-tones are pushed toward an S-curve for contrast. Near-black outlines
and near-white highlights stay where they are.
"""
import colorsys

import numpy as np

VIBRANCE = 0.55      # added saturation at grey, falling to none at full
SKIN = (0.015, 0.115)  # hue band (red-orange to amber) kept gentler
SKIN_VIBRANCE = 0.18
CONTRAST = 0.40      # blend toward smoothstep(lightness)


def grade(colors, strength=1.0):
    src = np.asarray(colors, dtype=np.float64)
    out = src.copy()
    for i, (r, g, b) in enumerate(src / 255.0):
        h, l, s = colorsys.rgb_to_hls(r, g, b)
        if l < 0.06 or l > 0.96:
            continue
        vib = SKIN_VIBRANCE if SKIN[0] <= h <= SKIN[1] and s < 0.75 else VIBRANCE
        s = min(1.0, s * (1.0 + vib * strength * (1.0 - s)))
        curve = l * l * (3.0 - 2.0 * l)
        l = l + (curve - l) * CONTRAST * strength
        out[i] = np.asarray(colorsys.hls_to_rgb(h, l, s)) * 255.0
    return np.clip(np.rint(out), 0, 255).astype(np.uint8)
