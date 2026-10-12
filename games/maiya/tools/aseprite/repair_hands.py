"""Draw wrist-connected palms/fists on the reviewed native pixel maps.

The six patches are hand-authored indexed pixel rows, with no transforms.
Apply to each physical arm before its existing integer bob. The accepted
head, tunic, leg outlines, boot outlines and gait timing remain unchanged.
"""
import argparse
import json
from pathlib import Path

ART = Path(__file__).resolve().parents[2] / 'assets/authored'

# (first x, first y, exact pixels). Index 6 is skin, 9 its warm shadow,
# and 1 the existing outline. Each palm meets the existing forearm.
HANDS = {
    ('back', 0): (26, 33, (
        '.16661.', '1666691', '1669691', '.16991.', '..111..',
    )),
    ('back', 1): (29, 33, (
        '.16661.', '1666691', '1669691', '.16991.', '..111..',
    )),
    ('back', 2): (32, 34, (
        '16661.', '166691', '166691', '166991', '.1991.', '..11..',
    )),
    ('front', 0): (54, 26, (
        '..111..', '.16691.', '1666691', '1669691', '666691.', '66911..',
    )),
    ('front', 1): (53, 30, (
        '1661..', '166691', '166691', '.16991', '..111.',
    )),
    ('front', 2): (49, 34, (
        '1661..', '166691', '166691', '.16991', '..111.',
    )),
}


def read(path):
    rows = [list(row) for row in path.read_text().splitlines()]
    if len(rows) != 64 or any(len(row) != 80 for row in rows):
        raise ValueError(f'{path}: expected an 80x64 indexed map')
    return rows


def repair(root):
    changes = []
    hands = []
    for pose in json.loads((root / 'poses.json').read_text()):
        for layer in ('far_arm', 'near_arm'):
            path = root / 'maps' / f"{pose['name']}__{layer}.txt"
            rows = read(path)
            xs = [x for row in rows for x, c in enumerate(row) if c != '.']
            side = 'back' if min(xs) < 40 else 'front'
            x0, y0, pixels = HANDS[side, pose['swing']]
            y0 += pose['bob']
            for dy, row in enumerate(pixels):
                for dx, value in enumerate(row):
                    x, y = x0 + dx, y0 + dy
                    if rows[y][x] != value:
                        changes.append(dict(pose=pose['name'], layer=layer, x=x,
                                            y=y, before=rows[y][x], after=value))
                        rows[y][x] = value
            path.write_text('\n'.join(''.join(row) for row in rows) + '\n')
            hands.append(dict(pose=pose['name'], layer=layer, side=side,
                              box=[x0, y0, x0 + len(pixels[0]) - 1,
                                   y0 + len(pixels) - 1]))
        # Keep the two physical legs distinguishable when their positions
        # exchange in the second half. Only the far knee/thigh highlight
        # uses the existing warm skin shade; neither leg changes shape.
        path = root / 'maps' / f"{pose['name']}__far_leg.txt"
        rows = read(path)
        for y, row in enumerate(rows):
            for x, value in enumerate(row):
                if value == '6':
                    changes.append(dict(pose=pose['name'], layer='far_leg',
                                        x=x, y=y, before='6', after='9'))
                    row[x] = '9'
        path.write_text('\n'.join(''.join(row) for row in rows) + '\n')
    if changes:
        (root / 'hand-repair.json').write_text(json.dumps(
            {'method': 'exact authored indexed pixel rows', 'hands': hands,
             'edits': changes}, indent=2) + '\n')
    print(f'Hand repair: {len(changes)} exact pixel edits on reviewed maps')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ART)
    args = parser.parse_args()
    repair(args.root)
