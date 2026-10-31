"""Painted referee sheet -> the referee's authored folder (indexed frames + manifest).

    python3 import_referee.py SHEET.png --folder games/shadowleague/assets/authored/referee
        [--replace] [--preview DIR]

The sheet holds ten front-view figures in two rows of five, in POSES order
(referee_art.py).
They share one scale (the standing figure STAND_PX tall), are reduced to
native pixels with four 15-colour palettes chosen per 16x16 tile, and stand
on the fighters' 192x192 canvas and baseline so the game draws them with the
fighters' sprite code. The referee never fights: one token hurt box, no hit
boxes.
"""
import argparse
import json
import sys
from pathlib import Path

import numpy as np
from scipy import ndimage as ndi

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
sys.path.append(str(TOOLS.parent))           # referee_art.py
import sprites as sp  # noqa: E402
from import_fighter import tile_banks  # noqa: E402
from referee_art import POSES  # noqa: E402

AIR = {'leap'}
STAND_PX = 128
LEAP_LIFT = 24          # the airborne figure's feet above the baseline


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('sheet', type=Path)
    ap.add_argument('--folder', type=Path, required=True)
    ap.add_argument('--out', default=str(TOOLS.parents[3]))
    ap.add_argument('--replace', action='store_true')
    ap.add_argument('--preview', default=None)
    ap.add_argument('--colours', type=int, default=48)
    a = ap.parse_args()
    if a.folder.exists() and not a.replace:
        raise SystemExit('Refusing to replace an existing export folder')
    figs, _ = sp.figures(sp.load(a.sheet), 2)
    if len(figs) != len(POSES):
        raise SystemExit(f'{a.sheet}: expected {len(POSES)} figures, found {len(figs)}')
    pal = sp.palette([(f[1], f[2]) for f in figs], a.colours)
    scale = STAND_PX / (figs[0][0][3] - figs[0][0][1])
    frames, roots = {}, {}
    for (box, rgb, mask), name in zip(figs, POSES):
        native = sp.outline(sp.native(rgb, mask, scale, pal), pal)
        canvas, root = sp.place(native, bottom=sp.ROOT[1] - (LEAP_LIFT if name in AIR else 0))
        lab, n = ndi.label(canvas > 0, np.ones((3, 3), bool))
        if n > 1:
            sizes = ndi.sum(canvas > 0, lab, range(1, n + 1))
            for k, size in enumerate(sizes, 1):
                if size < 3:
                    canvas[lab == k] = 0
        if name not in AIR:
            drop = sp.ROOT[1] - np.nonzero(canvas)[0].max()
            if drop > 0:
                canvas = np.roll(canvas, drop, axis=0)
        frames[name] = canvas
        roots[name] = [root[0], sp.ROOT[1]]
    frames, pal = tile_banks(frames, pal, 4, a.out)
    folder = a.folder.resolve()
    folder.mkdir(parents=True, exist_ok=a.replace)
    manifest = {'canvas': [sp.CANVAS, sp.CANVAS], 'anchor': list(sp.ROOT),
                'source': 'Native indexed PNG frames; four 15-colour palettes, one chosen per 16x16 tile',
                'frames': {}}
    for name in POSES:
        sp.save_indexed(frames[name], pal, folder / f'{name}.png')
        manifest['frames'][name] = {'file': f'{name}.png', 'root': roots[name], 'hurt': [[-16, 0, 32, 100]],
                                    'hit': {}, 'contacts': [] if name in AIR else ['far_leg', 'near_leg']}
    manifest['alt_palette'] = [[0, 0, 0]] + pal.tolist()
    (folder / 'manifest.json').write_text(json.dumps(manifest, indent=1) + '\n')
    sys.path.insert(0, str(Path(a.out) / 'games/shadowleague/tools'))
    from native_art_io import fighter_source
    fighter_source(folder, POSES)
    if a.preview:
        sp.preview([frames[n] for n in POSES], pal, Path(a.preview) / 'referee-frames.png', scale=2, cols=5)
    print('referee', len(POSES), 'frames, palette', len(pal), 'colours')


if __name__ == '__main__':
    main()
