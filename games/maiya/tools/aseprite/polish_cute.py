"""Small native palette patches on the player's accepted stride.

Round, warm palm clusters and quieter limb shading. Every silhouette,
joint, wrist, sole, pose and timing value stays at its accepted pixel.
The resulting 80x64 text maps are the source drawn in Aseprite, without
filters, transforms, intermediate frames or generated artwork.
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np

from refine_stride import read, write

ART = Path(__file__).resolve().parents[2] / 'assets/authored'
LAYERS = ('far_arm', 'far_leg', 'near_leg', 'torso', 'head_hair', 'near_arm')

# Palm patches, at the original unbobbed coordinates. The dark-brown
# contour replaces the harsh black mitten rim; the uninterrupted cream
# top and small lower warm rim describe a rounded palm. Wrists stay put.
PALMS = {
    ('back', 0): (
        (33, 27, 'B666B'), (34, 26, 'B66669B'),
        (35, 26, 'B66669B'), (36, 27, 'B699B'), (37, 28, 'BBB'),
    ),
    ('back', 1): (
        (33, 30, 'B666B'), (34, 29, 'B66669B'),
        (35, 29, 'B66669B'), (36, 30, 'B699B'), (37, 31, 'BBB'),
    ),
    ('back', 2): (
        (34, 32, 'B666B'), (35, 32, 'B66669'),
        (36, 32, 'B66669'), (37, 32, 'B66699'),
        (38, 33, 'B99B'), (39, 34, 'BB'),
    ),
    ('front', 0): (
        (26, 56, 'BBB'), (27, 55, 'B666B'),
        (28, 54, 'B66669B'), (29, 55, '66669B'),
        (30, 56, '66B'),
    ),
    ('front', 1): (
        (30, 53, 'B66B'), (31, 54, '6669B'),
        (32, 54, '6669B'), (33, 54, 'B699B'), (34, 55, 'BBB'),
    ),
    ('front', 2): (
        (34, 49, 'B66B'), (35, 49, 'B6669B'),
        (36, 49, 'B6669B'), (37, 50, 'B699B'), (38, 51, 'BBB'),
    ),
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def polish(root):
    poses = json.loads((root / 'poses.json').read_text())
    edits, evidence = [], []
    manifest = root / 'cute-polish.json'
    if manifest.exists():
        old = json.loads(manifest.read_text())
        if all(sha(read(root / 'maps' / item['file']).tobytes()) == item['after']
               for item in old['layers']):
            print('Cute polish: 0 changes (reviewed maps already patched)')
            return
        raise ValueError('Maps changed after polish; review them before repatching')
    for p in poses:
        for layer in LAYERS:
            filename = f"{p['name']}__{layer}.txt"
            path = root / 'maps' / filename
            before = read(path)
            after = before.copy()
            if layer.endswith('arm'):
                side = p['near_arm'] if layer == 'near_arm' else (
                    'back' if p['near_arm'] == 'front' else 'front')
                for y, x, row in PALMS[side, p['swing']]:
                    for dx, c in enumerate(row):
                        after[y + p['bob'], x + dx] = int(c, 16)
                # The exposed shoulder/elbow has one continuous light
                # surface, with a warm lower edge, instead of a moving
                # dark fleck inside otherwise light skin.
                for y in range(25 + p['bob'], 31 + p['bob']):
                    xs = np.nonzero(np.isin(before[y], (6, 9)))[0]
                    if len(xs) >= 2 and layer == 'near_arm':
                        after[y, xs[:-1]] = 6
                        after[y, xs[-1]] = 9
            elif layer.endswith('leg'):
                # A short, connected light surface inside a warm rim.
                # The far knee gets a small highlight so it reads as a
                # solid second leg, while remaining darker than the near.
                for y in range(64):
                    xs = np.nonzero(np.isin(before[y], (6, 9)))[0]
                    if not len(xs):
                        continue
                    after[y, xs] = 9
                    light = min(4, len(xs) - 2) if layer == 'near_leg' else min(2, len(xs) - 2)
                    if light > 0:
                        start = (len(xs) - light) // 2
                        after[y, xs[start:start + light]] = 6
                # Keep the exact boot mask and band. Remove the darkest
                # internal boot flecks, but retain the silhouette's black
                # edge and its continuous far-side shadow.
                for y in range(48, 61):
                    xs = np.nonzero(np.isin(before[y], (2, 5, 7, 11, 13)))[0]
                    if len(xs) >= 5:
                        x = int(xs[len(xs) // 2])
                        if before[y, x] in (5, 7, 11):
                            after[y, x] = 7 if layer == 'near_leg' else 5
            if not np.array_equal(before != 0, after != 0):
                raise ValueError(f'{filename}: patch would move an accepted outline')
            delta = []
            for y, x in zip(*np.nonzero(before != after)):
                delta.append([int(x), int(y), int(before[y, x]), int(after[y, x])])
            edits.append(dict(file=filename, pixels=delta))
            evidence.append(dict(file=filename, before=sha(before.tobytes()),
                                 after=sha(after.tobytes()),
                                 silhouette=sha((before != 0).tobytes())))
            write(path, after)
    result = dict(method='native indexed colour patches; accepted silhouettes and motion unchanged',
                  layers=evidence, edits=edits,
                  poses_sha256=sha((root / 'poses.json').read_bytes()),
                  palette_sha256=sha((root / 'palette.json').read_bytes()),
                  pixel_changes=sum(len(e['pixels']) for e in edits))
    manifest.write_text(json.dumps(result, indent=2) + '\n')
    print(f"Cute polish: {result['pixel_changes']} exact indexed pixel changes")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ART)
    polish(parser.parse_args().root)
