"""Check the stride and the idle outfit details on reopened Aseprite art.

The planted foot must travel with the road: from one pose to the next it
moves back by the road that pose is held for (the run within 1 px, the walk,
whose clock runs 5/4 of the road, within 3 px). The swing foot must lift and
fold through. Both legs wear the same boots and colours. This supplements the
anatomy, visible-fist and C-ROM gates.
"""
import argparse
import json
from pathlib import Path

import numpy as np
from PIL import Image

ART = Path(__file__).resolve().parents[1] / 'assets/authored'
LAYERS = ('far_arm', 'far_leg', 'near_leg', 'torso', 'head_hair', 'near_arm')
BOOTS = (2, 5, 7, 11, 13)


def check(root, before=None):
    poses = json.loads((root / 'poses.json').read_text())
    images, metrics, errors = {}, [], []

    def need(ok, message):
        if not ok:
            errors.append(message)

    idle = np.array(Image.open(root / 'reference_idle0.png'))
    need(all(np.any(idle == i) for i in (2, 3, 4, 5, 6, 7, 9, 11, 12)),
         'idle: required outfit/skin ramps missing from reference')
    for p in poses:
        name = p['name']
        a = {l: np.array(Image.open(root / 'layers' / f'{name}__{l}.png'))
             for l in LAYERS}
        images[name] = a
        feet = {}
        for l in ('near_leg', 'far_leg'):
            y, x = np.nonzero(np.isin(a[l], BOOTS))
            feet[l] = dict(x=float(x[y >= y.max() - 6].mean()),
                           sole=int(np.nonzero(a[l])[0].max()))
            need(int(np.count_nonzero(a[l] == 2)) >= 2,
                 f'{name}/{l}: idle boot-band glint missing')
            need(np.any(a[l] == 9), f'{name}/{l}: warm knee shadow missing')
            if l == 'near_leg':
                light = int(np.count_nonzero(a[l] == 6))
                warm = int(np.count_nonzero(a[l] == 9))
                need(light >= 3 and warm >= light / 2,
                     f'{name}/{l}: knee lost its highlight or warm idle shadow')
        for l in ('near_arm', 'far_arm'):
            green = int(np.count_nonzero(np.isin(a[l], (4, 10, 15))))
            trim = int(np.count_nonzero(np.isin(a[l], (3, 12))))
            wrist = int(np.count_nonzero(np.isin(a[l], (5, 7, 11))))
            need(green >= 4, f'{name}/{l}: green sleeve missing')
            need(trim >= 2, f'{name}/{l}: pink idle sleeve trim missing')
            need(wrist >= 4, f'{name}/{l}: brown idle wrist band missing')
        if before:
            for l in ('head_hair', 'torso'):
                old = np.array(Image.open(before / 'layers' / f'{name}__{l}.png'))
                need(np.array_equal(old, a[l]), f'{name}/{l}: accepted pixels changed')
            if p['mode'] != 'run' or p['slot'] % 4 in (0, 3):
                for l in ('near_leg', 'far_leg'):
                    old_leg = np.array(Image.open(before / 'layers' / f'{name}__{l}.png'))
                    need(np.array_equal(old_leg != 0, a[l] != 0),
                         f'{name}/{l}: accepted contact/walk/flight outline changed')
        metrics.append(dict(pose=name, feet=feet))

    run = [p for p in poses if p['mode'] == 'run']
    foot = {m['pose']: m['feet'] for m in metrics}
    recovery = []
    for mode, road, slack in (('walk', 0.8, 3.0), ('run', 1.0, 1.0)):
        cycle = [p for p in poses if p['mode'] == mode]
        for half in (0, 4):
            # A half: the far leg leads the contact and is planted through it
            stance = 'far_leg' if half == 0 else 'near_leg'
            swing = 'near_leg' if half == 0 else 'far_leg'
            names = [cycle[i]['name'] for i in range(half, half + 4)]
            tx = [foot[n][stance]['x'] for n in names]
            planted = 3 if mode == 'walk' else 2
            for i in range(planted):
                hold = cycle[half + i]['hold_distance'] * road
                need(abs(tx[i] - tx[i + 1] - hold) <= slack,
                     f'{names[i]}: planted foot moves {tx[i] - tx[i + 1]:.1f}px over {hold:.1f}px of road')
            sole = [foot[n][swing]['sole'] for n in names]
            if mode == 'run':
                need(sole[1] <= 59 and sole[2] <= 58, f'run half {half // 4}: swing boot fails to lift: {sole}')
                knee = images[names[2]][swing]
                y, x = np.nonzero(np.isin(knee, BOOTS))
                cuff = float(x[y <= y.min() + 2].mean())
                heel = float(x[y >= y.max() - 2].mean())
                need(cuff - heel >= 3, f'{names[2]}/{swing}: driving knee without a folded shin ({cuff - heel:.1f}px)')
                recovery.append(dict(half=half // 4, swing_layer=swing, swing_soles=sole,
                                     stance_foot_x=tx, folded_shin_px=cuff - heel))
            else:
                need(min(sole[2:]) <= 58, f'walk half {half // 4}: swing boot never lifts: {sole}')
                recovery.append(dict(half=half // 4, swing_layer=swing, swing_soles=sole, stance_foot_x=tx))
    for slot in range(4):
        a, b = images[run[slot]['name']], images[run[slot + 4]['name']]
        need(np.array_equal(a['near_leg'] != 0, b['far_leg'] != 0) and
             np.array_equal(a['far_leg'] != 0, b['near_leg'] != 0),
             f'run {slot}/{slot + 4}: opposite legs do not follow the same complete path')
    if before:
        old_poses = json.loads((before / 'poses.json').read_text())
        for p, old in zip(poses, old_poses):
            need(all(p[k] == old[k] for k in ('name', 'mode', 'slot', 'bob',
                                            'swing', 'near_arm', 'hold_distance', 'grounded')),
                 f"{p['name']}: accepted clock/bob/arm order changed")
        for name in ('palette.json', 'reference_idle0.png', 'reference_stride.png'):
            need((root / name).read_bytes() == (before / name).read_bytes(),
                 f'{name}: accepted palette/reference changed')
    result = dict(passed=not errors, errors=errors, poses=metrics, recovery=recovery,
                  accepted_upper_body_preserved=(not errors) if before else None)
    (root / 'stride-check-result.json').write_text(json.dumps(result, indent=2) + '\n')
    for error in errors:
        print('FAIL:', error)
    print('Continuous leg recovery and idle outfit details:', 'PASS' if not errors else 'FAIL')
    return not errors


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ART)
    parser.add_argument('--before', type=Path)
    args = parser.parse_args()
    raise SystemExit(0 if check(args.root, args.before) else 1)
