"""Hit, guard and dust effects and the projectile, from the painted effects
sheet (assets/authored/effects/fx_sheet.png): four rows of four frames on
magenta. Each frame becomes a 48x48 sprite (3x3 tiles) centred on its cell,
one row to a palette of 15 colours fitted to its four frames.

    frames() -> {kind: (four 48x48 index arrays, 16x3 palette)}
    kinds: 'hit', 'guard', 'dust', 'projectile'
"""
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

from palette_banks import quantize

SHEET = Path(__file__).resolve().parents[1] / 'assets/authored/effects/fx_sheet.png'
KINDS = ('hit', 'guard', 'dust', 'projectile')
SIZE = 48


def _key(rgb):
    f = rgb.astype(int)
    r, g, b = f[..., 0], f[..., 1], f[..., 2]
    return (r > 140) & (b > 140) & (g < 120) & (r - g > 70) & (b - g > 70)


def frames():
    rgb = np.asarray(Image.open(SHEET).convert('RGB'))
    h, w = rgb.shape[:2]
    ch, cw = h // 4, w // 4
    out = {}
    for row, kind in enumerate(KINDS):
        cells = []
        for col in range(4):
            cell = rgb[row * ch:(row + 1) * ch, col * cw:(col + 1) * cw]
            solid = ~ndi.binary_dilation(_key(cell), iterations=2)   # (the magenta fringe goes too)
            cells.append((cell, solid))
        extent = max(max(np.ptp(np.nonzero(s)[0]), np.ptp(np.nonzero(s)[1])) for _, s in cells if s.any())
        scale = (SIZE - 4) / extent
        rgba = []
        for cell, solid in cells:
            ys, xs = np.nonzero(solid)
            cy, cx = (ys.min() + ys.max()) / 2, (xs.min() + xs.max()) / 2
            if kind == 'hit' or kind == 'guard':
                cy, cx = ch / 2, cw / 2               # bursts grow from one point
            img = np.dstack([cell, np.where(solid, 255, 0)]).astype(np.uint8)
            size = (max(1, round(cw * scale)), max(1, round(ch * scale)))
            small = np.asarray(Image.fromarray(img).resize(size, Image.Resampling.LANCZOS))
            canvas = np.zeros((SIZE, SIZE, 4), np.uint8)
            ox, oy = round(SIZE / 2 - cx * scale), round(SIZE / 2 - cy * scale)
            y0, x0 = max(0, oy), max(0, ox)
            y1, x1 = min(SIZE, oy + small.shape[0]), min(SIZE, ox + small.shape[1])
            canvas[y0:y1, x0:x1] = small[y0 - oy:y1 - oy, x0 - ox:x1 - ox]
            canvas[..., 3] = np.where(canvas[..., 3] >= 128, 255, 0)
            canvas[0, :, 3] = canvas[-1, :, 3] = canvas[:, 0, 3] = canvas[:, -1, 3] = 0
            rgba.append(canvas)
        indices, banks, _ = quantize(np.vstack(rgba), bank_limit=1)
        out[kind] = ([indices[i * SIZE:(i + 1) * SIZE] for i in range(4)], banks[0])
    return out
