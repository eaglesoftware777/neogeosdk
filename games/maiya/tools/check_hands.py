"""Require visible wrist-connected hands, arms swinging against the legs, and
legs that alternate: both feet down at contact, the swing foot lifted during
each step, both off the ground in the run's flight."""
import argparse
import json
from pathlib import Path

import numpy as np
from PIL import Image

ART = Path(__file__).resolve().parents[1] / 'assets/authored'
LAYERS = ('far_arm', 'far_leg', 'near_leg', 'torso', 'head_hair', 'near_arm')
NEAR_SHOULDER_X = 47          # compose_gait.py's near shoulder
GROUND_SOLE = 61              # the sole's outline row on the ground


def check(root, before=None):
    poses = json.loads((root / 'poses.json').read_text())
    authored = json.loads((root / 'hand-repair.json').read_text())
    specs = {(h['pose'], h['layer']): h for h in authored['hands']}
    errors, metrics, images = [], [], {}

    def need(ok, message):
        if not ok:
            errors.append(message)

    for p in poses:
        name = p['name']
        layers = {l: np.array(Image.open(root / 'layers' / f'{name}__{l}.png'))
                  for l in LAYERS}
        images[name] = layers
        owner = np.full((64, 80), -1, np.int8)
        for i, l in enumerate(LAYERS):
            owner[layers[l] != 0] = i
        for l in ('far_arm', 'near_arm'):
            key = name, l
            need(key in specs, f'{name}/{l}: missing authored hand box')
            if key not in specs:
                continue
            x0, y0, x1, y1 = specs[key]['box']
            a = layers[l][y0:y1 + 1, x0:x1 + 1]
            visible = owner[y0:y1 + 1, x0:x1 + 1] == LAYERS.index(l)
            skin = np.isin(a, (6, 9))
            palm = skin & visible
            count = int(palm.sum())
            # The near fist shows whole. The far fist is behind her body when
            # it swings forward; whatever of it shows is at least a 3-pixel
            # edge, never a lone speck.
            if l == 'near_arm':
                need(count >= 12, f'{name}/{l}: {count} visible fist pixels; need 12')
            else:
                need(count == 0 or count >= 3, f'{name}/{l}: a {count}-pixel speck of the far fist shows')
            ys, xs = np.nonzero(palm)
            if l == 'near_arm' and len(xs):
                width, height = int(np.ptp(xs)) + 1, int(np.ptp(ys)) + 1
                need(width >= 3 and height >= 3, f'{name}/{l}: fist is only {width}x{height}')
            # the fist joins the bracer at the wrist: no outline ring between them
            fist = np.zeros((64, 80), bool)
            fist[y0:y1 + 1, x0:x1 + 1] = skin
            grown = np.zeros_like(fist)
            grown[1:] |= fist[:-1]; grown[:-1] |= fist[1:]; grown[:, 1:] |= fist[:, :-1]; grown[:, :-1] |= fist[:, 1:]
            need(np.any(grown & np.isin(layers[l], (5, 7, 11))), f'{name}/{l}: fist not joined to the wrist')
            metrics.append(dict(pose=name, layer=l, visible_palm_pixels=count))
        if before:
            for l in ('head_hair', 'torso', 'near_leg', 'far_leg'):
                old = np.array(Image.open(before / 'layers' / f'{name}__{l}.png'))
                new = layers[l]
                if l == 'far_leg':
                    changed = old != new
                    need(np.all((old[changed] == 6) & (new[changed] == 9)),
                         f'{name}/{l}: change beyond the far-leg skin shadow')
                else:
                    need(np.array_equal(old, new), f'{name}/{l}: accepted drawing changed')

    for mode in ('walk', 'run'):
        a, b = images[f'{mode}_contact_a'], images[f'{mode}_contact_b']
        def leading(layers):
            # Use the actual boot pixels, rather than the pose labels.
            return max(('near_leg', 'far_leg'), key=lambda l:
                       float(np.nonzero(np.isin(layers[l], (5, 7, 11, 13)))[1].mean()))
        need(leading(a) != leading(b), f'{mode}: contacts lead with the same physical leg')
        for pair in ('a', 'b'):
            pose = images[f'{mode}_contact_{pair}']
            near_front = leading(pose) == 'near_leg'
            box = specs[(f'{mode}_contact_{pair}', 'near_arm')]['box']
            arm_front = (box[0] + box[2]) / 2 > NEAR_SHOULDER_X
            need(near_front != arm_front,
                 f'{mode}_contact_{pair}: leading arm and leg on the same side')
        # Alternation: at contact both soles are down; within each step the
        # swing foot rises clear of the planted one; the run's flight leaves
        # the ground with both.
        def soles(p):
            return [int(np.nonzero(images[p['name']][l])[0].max()) for l in ('near_leg', 'far_leg')]
        for half in (0, 4):
            steps = [next(p for p in poses if p['mode'] == mode and p['slot'] == half + i) for i in range(4)]
            need(min(soles(steps[0])) >= GROUND_SOLE - 1, f"{steps[0]['name']}: a contact foot is off the ground")
            need(max(abs(a - b) for a, b in (soles(p) for p in steps[1:])) >= 3,
                 f'{mode} step {half // 4}: the swing foot never lifts clear of the planted one')
            if mode == 'run':
                need(max(soles(steps[3])) <= GROUND_SOLE - 3, f"{steps[3]['name']}: flight touches the ground")
    if before:
        for name in ('poses.json', 'palette.json'):
            need((before / name).read_bytes() == (root / name).read_bytes(),
                 f'{name}: accepted pose order, holds or palette changed')
    result = {'passed': not errors, 'errors': errors, 'hands': metrics,
              'accepted_head_torso_leg_shapes_preserved': (not errors) if before else None}
    (root / 'hand-check-result.json').write_text(json.dumps(result, indent=2) + '\n')
    for error in errors:
        print('FAIL:', error)
    print(f'Visible hands and opposite-leg contacts: {"PASS" if not errors else "FAIL"}')
    return not errors


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ART)
    parser.add_argument('--before', type=Path)
    args = parser.parse_args()
    raise SystemExit(0 if check(args.root, args.before) else 1)
