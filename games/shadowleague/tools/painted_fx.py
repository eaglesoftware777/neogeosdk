"""Hit, guard and dust effects, the projectile, and the special-move fire and
super flash, from painted sheets of four frames a row on magenta. Each frame
is centred on its cell, one row to a palette of 15 colours fitted to its four
frames. The hit, guard and dust bursts (assets/authored/effects/fx_sheet_v2.png)
and the super flash are 64x64 sprites (4x4 tiles); the projectile (the last
row of fx_sheet.png) and the fire are 48x48 (3x3).

    frames() -> {kind: (four index arrays, 16x3 palette)}
    kinds: 'hit', 'guard', 'dust', 'super' (64), 'projectile', 'fire' (48)
"""
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

from palette_banks import quantize

EFFECTS = Path(__file__).resolve().parents[1] / 'assets/authored/effects'
SHEET = EFFECTS / 'fx_sheet.png'
BURSTS = EFFECTS / 'fx_sheet_v2.png'
SIZE = 48
BIG = 64


def _key(rgb):
    f = rgb.astype(int)
    r, g, b = f[..., 0], f[..., 1], f[..., 2]
    return (r > 140) & (b > 140) & (g < 120) & (r - g > 70) & (b - g > 70)


def _sheet(path, kinds, side=SIZE, rows=None):
    rgb = np.asarray(Image.open(path).convert('RGB'))
    h, w = rgb.shape[:2]
    ch, cw = h // (rows or len(kinds)), w // 4
    out = {}
    for row, kind in enumerate(kinds):
        cells = []
        for col in range(4):
            cell = rgb[row * ch:(row + 1) * ch, col * cw:(col + 1) * cw]
            solid = ~ndi.binary_dilation(_key(cell), iterations=2)   # (the magenta fringe goes too)
            cells.append((cell, solid))
        extent = max(max(np.ptp(np.nonzero(s)[0]), np.ptp(np.nonzero(s)[1])) for _, s in cells if s.any())
        scale = (side - 4) / extent
        rgba = []
        for cell, solid in cells:
            ys, xs = np.nonzero(solid)
            cy, cx = (ys.min() + ys.max()) / 2, (xs.min() + xs.max()) / 2
            if kind == 'hit' or kind == 'guard':
                cy, cx = ch / 2, cw / 2               # bursts grow from one point
            img = np.dstack([cell, np.where(solid, 255, 0)]).astype(np.uint8)
            size = (max(1, round(cw * scale)), max(1, round(ch * scale)))
            small = np.asarray(Image.fromarray(img).resize(size, Image.Resampling.NEAREST))
            canvas = np.zeros((side, side, 4), np.uint8)
            ox, oy = round(side / 2 - cx * scale), round(side / 2 - cy * scale)
            y0, x0 = max(0, oy), max(0, ox)
            y1, x1 = min(side, oy + small.shape[0]), min(side, ox + small.shape[1])
            canvas[y0:y1, x0:x1] = small[y0 - oy:y1 - oy, x0 - ox:x1 - ox]
            canvas[..., 3] = np.where(canvas[..., 3] >= 128, 255, 0)
            canvas[0, :, 3] = canvas[-1, :, 3] = canvas[:, 0, 3] = canvas[:, -1, 3] = 0
            rgba.append(canvas)
        indices, banks, _ = quantize(np.vstack(rgba), bank_limit=1)
        out[kind] = ([indices[i * side:(i + 1) * side] for i in range(4)], banks[0])
    return out


def frames():
    out = _sheet(BURSTS, ('hit', 'guard', 'dust'), BIG)
    out['projectile'] = _sheet(SHEET, ('hit', 'guard', 'dust', 'projectile'))['projectile']
    special = EFFECTS.parent / 'sdk177/sources/special-effects.png'
    out['fire'] = _sheet(special, ('fire', 'super'))['fire']
    out['super'] = _sheet(special, ('fire', 'super'), BIG)['super']
    return out
