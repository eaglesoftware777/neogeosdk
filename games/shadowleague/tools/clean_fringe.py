"""Paint out the magenta key left on the painted fighters' edges.

The fighter sheets were painted on magenta and keyed out. Where a stroke's
antialiasing mixed with the key, a pink colour survived the import: it sits
almost only along the silhouette and in the gaps of the hair (FRINGE_RIM of
its pixels or more within two pixels of transparency), so in a fight it
reads as a pink contour, and on a small hand it filled the whole hand.

Each such colour is found per fighter. Every pixel of it takes the most
common other colour around it, working inwards from the fringe's edge, so a
pink hand fills with the skin beside it. Only colours the pixel's 16x16 tile
already uses are taken, so the tile palettes still pack into four banks.
The frames are rewritten in place.

    python3 games/shadowleague/tools/clean_fringe.py [--write] [ID ...]
"""
import argparse
import colorsys
import sys
from collections import Counter
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

GAME = Path(__file__).resolve().parents[1]
FIGHTERS = GAME / 'assets/authored/fighters'
FRINGE_RIM = 0.8
NOT_FRAMES = ('head.png', 'portrait-small.png')


def frames(fid):
    return [p for p in sorted((FIGHTERS / fid).glob('*.png')) if p.name not in NOT_FRAMES]


def fringe_colours(fid):
    """Palette indices of pink colours that sit on the outline."""
    total, rim, pal = Counter(), Counter(), None
    for p in frames(fid):
        im = Image.open(p)
        a = np.array(im)
        pal = np.array(im.getpalette()).reshape(-1, 3).astype(int)
        on = a > 0
        edge = on & ~ndi.binary_erosion(on, iterations=2)
        total.update(a[on].tolist())
        rim.update(a[edge].tolist())
    out = []
    for i, n in total.items():
        r, g, b = pal[i]
        h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
        pink = (h > 0.78 or h < 0.02) and s > 0.35 and v > 0.3 and b > g + 25 and r > g + 25
        if pink and n >= 20 and rim[i] / n >= FRINGE_RIM:
            out.append(i)
    return sorted(out), pal


def repaint(a, fringe, pal):
    """The frame with every fringe pixel taken by its neighbours' colour."""
    out = a.copy()
    pending = np.isin(a, fringe)
    allowed = {}
    for ty in range(0, a.shape[0], 16):
        for tx in range(0, a.shape[1], 16):
            used = set(np.unique(a[ty:ty + 16, tx:tx + 16]).tolist()) - set(fringe) - {0}
            allowed[ty // 16, tx // 16] = used
    while pending.any():
        updates = []
        for y, x in zip(*np.nonzero(pending)):
            win = (slice(max(0, y - 1), y + 2), slice(max(0, x - 1), x + 2))
            nb = out[win][(out[win] > 0) & ~pending[win]]
            if not len(nb):
                continue
            used = allowed[y // 16, x // 16]
            want = Counter(nb.tolist()).most_common(1)[0][0]
            if want not in used:
                if not used:
                    continue
                want = min(used, key=lambda v: ((pal[v] - pal[want]) ** 2).sum())
            updates.append((y, x, want))
        if not updates:
            break
        for y, x, v in updates:
            out[y, x] = v
            pending[y, x] = False
    # fringe with no other colour near it (a speck on its own): the darkest
    # colour its tile uses
    for y, x in zip(*np.nonzero(pending)):
        used = allowed[y // 16, x // 16]
        if used:
            out[y, x] = min(used, key=lambda v: pal[v].sum())
    return out


def clean(fid, write=False):
    fringe, pal = fringe_colours(fid)
    if not fringe:
        return f'{fid}: clean'
    painted = changed = 0
    for p in frames(fid):
        im = Image.open(p)
        a = np.array(im)
        if not np.isin(a, fringe).any():
            continue
        out = repaint(a, fringe, pal)
        painted += int((out != a).sum())
        changed += 1
        if write:
            img = Image.fromarray(out.astype(np.uint8), 'P')
            img.putpalette(im.getpalette())
            img.save(p, transparency=0)
    colours = ', '.join('#%02x%02x%02x' % tuple(pal[i]) for i in fringe)
    return f'{fid}: {colours}: {painted} pixels in {changed} frames' + (' repainted' if write else '')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('ids', nargs='*')
    ap.add_argument('--write', action='store_true')
    a = ap.parse_args()
    ids = a.ids or sorted(p.name for p in FIGHTERS.iterdir() if (p / 'manifest.json').exists())
    for fid in ids:
        print(clean(fid, a.write), flush=True)
    if a.write:
        sys.path.insert(0, str(GAME / 'tools'))
        from native_art_io import fighter_source
        import json
        for fid in ids:
            names = list(json.loads((FIGHTERS / fid / 'manifest.json').read_text())['frames'])
            fighter_source(FIGHTERS / fid, names)      # raises if a tile or the banks no longer fit
        print('tile palettes still pack into four banks')


if __name__ == '__main__':
    main()
