"""A painted logo on magenta -> assets/authored/rebirth/identity/logo.png.

    python3 games/shadowleague/tools/import_logo.py PAINTED.png

The logo is cut from its magenta key (and the key's fringe), fitted inside
the 288x96 title sprite with its proportions kept and 8 pixels clear at
each side (inside the CRT safe area once the sprite sits at x 16), and reduced to one
palette of 15 colours plus transparency: it is drawn as one sprite group
over the title card, with its own palette.
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

GAME = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(GAME.parents[1] / 'artbox'))
from palette_banks import quantize  # noqa: E402

OUT = GAME / 'assets/authored/rebirth/identity/logo.png'
W, H = 288, 96


def main():
    rgb = np.asarray(Image.open(sys.argv[1]).convert('RGB')).astype(int)
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    key = (r > 140) & (b > 140) & (g < 120) & (r - g > 70) & (b - g > 70)
    solid = ~ndi.binary_dilation(key, iterations=2)
    ys, xs = np.nonzero(solid)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    crop = np.dstack([rgb[y0:y1, x0:x1], np.where(solid[y0:y1, x0:x1], 255, 0)]).astype(np.uint8)
    scale = min((W - 16) / crop.shape[1], (H - 4) / crop.shape[0])
    size = (max(1, round(crop.shape[1] * scale)), max(1, round(crop.shape[0] * scale)))
    small = np.asarray(Image.fromarray(crop).resize(size, Image.Resampling.LANCZOS)).copy()
    small[..., 3] = np.where(small[..., 3] >= 160, 255, 0)
    canvas = np.zeros((H, W, 4), np.uint8)
    oy, ox = (H - size[1]) // 2, (W - size[0]) // 2
    canvas[oy:oy + size[1], ox:ox + size[0]] = small
    indices, banks, _ = quantize(canvas, bank_limit=1)
    im = Image.fromarray(indices.astype(np.uint8), 'P')
    flat = [int(v) for c in banks[0] for v in c]
    im.putpalette(flat + [0] * (768 - len(flat)))
    im.save(OUT, transparency=0)
    print(OUT, im.size, 'colours', int(indices.max()))


if __name__ == '__main__':
    main()
