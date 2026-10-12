"""Native pixel corrections: knee recovery, sleeves and idle outfit details.

The accepted maps are the input. Six arms and two recovery legs below are
hand-specified indexed rows, drawn by build_walkrun.lua in Aseprite. Never
run the historical full-figure assembler over the reviewed source.
"""
import argparse
import json
from pathlib import Path

import numpy as np

ART = Path(__file__).resolve().parents[2] / 'assets/authored'
ARMS = {
    ('back', 0): (
        (25, 32, '1F41'), (26, 31, '1F4A1'),
        (27, 31, '13C61'), (28, 30, '19661'),
        (29, 30, '1B751'), (30, 29, '1B751'),
        (31, 29, '1B771'), (32, 28, '1B751'),
        (33, 27, '16661'), (34, 26, '1666691'),
        (35, 26, '1669691'), (36, 27, '16991'), (37, 28, '111'),
    ),
    ('back', 1): (
        (25, 33, '1F1'), (26, 32, '1F41'), (27, 32, '1AC61'),
        (28, 32, '13661'), (29, 31, '19661'),
        (30, 31, '1B751'), (31, 30, '1B771'), (32, 30, '1B751'),
        (33, 30, '16661'), (34, 29, '1666691'),
        (35, 29, '1669691'), (36, 30, '16991'), (37, 31, '111'),
    ),
    ('back', 2): (
        (25, 33, '1F1'), (26, 33, '1F41'), (27, 33, '1AC1'),
        (28, 33, '1361'), (29, 33, '19661'),
        (30, 33, '1B751'), (31, 33, '1B751'),
        (32, 33, '1B771'), (33, 33, '1B751'),
        (34, 32, '16661'), (35, 32, '166691'),
        (36, 32, '166691'), (37, 32, '166991'),
        (38, 33, '1991'), (39, 34, '11'),
    ),
    ('front', 0): (
        (26, 56, '111'), (27, 55, '16691'),
        (28, 49, '1F4..1666691'), (29, 49, '1F431B669691'),
        (30, 50, '1C975B6691'), (31, 51, '1B775911'), (32, 52, '111'),
    ),
    ('front', 1): (
        (27, 49, '1F1'), (28, 49, '1F41'), (29, 49, '1AC91'),
        (30, 50, '1371661'), (31, 51, '1B766691'),
        (32, 52, '1B66691'), (33, 54, '16991'), (34, 55, '111'),
    ),
    ('front', 2): (
        (27, 48, '1F1'), (28, 48, '1F41'), (29, 48, '1AC1'),
        (30, 48, '13751'), (31, 48, '1B751'),
        (32, 48, '1B771'), (33, 48, '1B751'),
        (34, 48, '11661'), (35, 49, '166691'),
        (36, 49, '166691'), (37, 50, '16991'), (38, 51, '111'),
    ),
}

# Whole limbs, including their soles. The down pose tucks the heel behind
# a recovering knee; the passing/push pose drives that knee ahead, with
# the lower leg folded back. Contact/extension drawings remain intact.
RECOVERY = {
    'down': (
        (42, 36, '196691'), (43, 35, '1966691'), (44, 35, '1966691'),
        (45, 34, '19666691'), (46, 33, '19666991'), (47, 32, '1B79991'),
        (48, 29, '1B777775B1'), (49, 27, '1B777775B11'),
        (50, 26, '1B777775B11'), (51, 26, '1B77775B11'),
        (52, 27, '1B7775B11'), (53, 28, '1B775B11'),
        (54, 29, '1B75B11'), (55, 30, '1111'),
    ),
    'push': (
        (43, 39, '196691'), (44, 39, '19666691'),
        (45, 40, '1966666691'), (46, 41, '1966666691'),
        (47, 42, '19699991'), (48, 43, '1B77591'),
        (49, 42, '1B7775B1'), (50, 41, '1B7775B1'),
        (51, 40, '1B7775B1'), (52, 39, '1B7775B1'),
        (53, 38, '1B7775B1'), (54, 38, '1B7775B1'),
        (55, 39, '1B775B1'), (56, 40, '1B75B1'), (57, 41, '1111'),
    ),
}


def read(path):
    rows = path.read_text().splitlines()
    if len(rows) != 64 or any(len(row) != 80 for row in rows):
        raise ValueError(f'{path}: expected 80x64 map')
    return np.array([[0 if c == '.' else int(c, 16) for c in row]
                     for row in rows], np.uint8)


def write(path, a):
    path.write_text('\n'.join(''.join('.' if v == 0 else format(int(v), 'X')
                                     for v in row) for row in a) + '\n')


def pixels(rows, bob=0):
    a = np.zeros((64, 80), np.uint8)
    for y, x, row in rows:
        for dx, c in enumerate(row):
            a[y + bob, x + dx] = 0 if c == '.' else int(c, 16)
    return a


def leg_material(a, far):
    """Whole-pixel existing palette ramps, matching idle's warm knees.

    A short central highlight, a warm knee rim, and a two-pixel boot-band
    glint. The far limb uses the darker shades of the same ramps. No mask
    deformation, filtering, resize, outline generation or new colours.
    """
    a = a.copy()
    skin = np.isin(a, (6, 9))
    a[skin] = 9
    if not far:
        for y in range(64):
            xs = np.nonzero(skin[y])[0]
            if len(xs) >= 3:
                a[y, xs[1:1 + min(3, len(xs) - 2)]] = 6
    if far:
        a[a == 7] = 5
    boots = np.isin(a, (2, 5, 7, 11, 13))
    top = next(y for y in range(48, 62) if int(boots[y].sum()) >= 4)
    band_y = top + 1
    xs = np.nonzero(boots[band_y])[0]
    if len(xs) < 3:
        raise ValueError('Boot cuff has no complete band row')
    a[band_y, xs[-3:-1]] = 13 if far else 2
    return a, [(int(x), band_y) for x in xs[-3:-1]]


def refine(root):
    poses = json.loads((root / 'poses.json').read_text())
    edits, materials = [], []
    for p in poses:
        name = p['name']
        for layer in ('far_arm', 'near_arm', 'far_leg', 'near_leg'):
            path = root / 'maps' / f'{name}__{layer}.txt'
            old = read(path)
            if layer.endswith('arm'):
                side = 'back' if int(np.nonzero(old)[1].min()) < 40 else 'front'
                new = pixels(ARMS[side, p['swing']], p['bob'])
                if layer == 'far_arm':
                    new[new == 7] = 5
                    # Keep the visible fingers lit, shadow the upper arm.
                    limit = 30 + p['bob']
                    new[:limit][new[:limit] == 6] = 9
                materials.append(dict(pose=name, layer=layer, side=side,
                                      sleeve=(4, 10, 15), wrist=(5, 7, 11)))
            else:
                new = old
                state = name.split('_')[1] if p['mode'] == 'run' else None
                recovering = layer == ('near_leg' if p['half'] == 'a' else 'far_leg')
                if state in RECOVERY and recovering:
                    new = pixels(RECOVERY[state])
                    p[layer] = ['authored_recovery_' + state, 'recovery']
                elif state in RECOVERY:
                    # Match the stance stroke to the 8/10 px holds: the
                    # original support boot moved only 7, then 15 pixels.
                    marker = 'authored_stance_' + state
                    if p[layer][0] != marker:
                        dx = -1 if state == 'down' else 4
                        moved = np.zeros_like(new)
                        y, x = np.nonzero(new)
                        if np.any((x + dx <= 0) | (x + dx >= 79)):
                            raise ValueError('Stance move would clip a limb')
                        moved[y, x + dx] = new[y, x]
                        new = moved
                    p[layer] = [marker, 'support']
                new, band = leg_material(new, layer == 'far_leg')
                materials.append(dict(pose=name, layer=layer, boot_band=band,
                                      band_index=13 if layer == 'far_leg' else 2))
            for y, x in zip(*np.nonzero(old != new)):
                edits.append(dict(pose=name, layer=layer, x=int(x), y=int(y),
                                  before=int(old[y, x]), after=int(new[y, x])))
            write(path, new)
        flat = np.zeros((64, 80), np.uint8)
        for layer in ('far_arm', 'far_leg', 'near_leg', 'torso', 'head_hair', 'near_arm'):
            a = read(root / 'maps' / f'{name}__{layer}.txt')
            flat[a != 0] = a[a != 0]
        y, x = np.nonzero(flat)
        p['bounds'] = [int(x.min()), int(y.min()), int(x.max()), int(y.max())]
    (root / 'poses.json').write_text(json.dumps(poses, indent=2) + '\n')
    if edits:
        (root / 'stride-details.json').write_text(json.dumps(dict(
            method='hand-specified native indexed pixel rows; exact palette writes',
            materials=materials, edits=edits), indent=2) + '\n')
    print(f'Stride details: {len(edits)} exact indexed pixel changes')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ART)
    refine(parser.parse_args().root)
