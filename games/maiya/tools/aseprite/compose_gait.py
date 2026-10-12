"""Maiya's walk and run, composed from her original drawings.

Head and tunic: her stride figure (unchanged; the same drawing as her throws,
jumps and landing), moved only by the whole-pixel bob.

Legs: one leg pair per phase, cut from her original WALK.png / RUN.png cells
in assets/source_art, so every boot, knee and thigh is hand-drawn:

    walk  contact WALK 0, down WALK 2, passing WALK 1, up WALK 5
    run   contact RUN 0, down RUN 1, push-off and knee drive RUN 5,
          flight RUN 3 (both feet off the ground)

Each leg goes through every phase in turn -- planted ahead, under the hip,
pushing off behind, lifted and swung through -- and the other leg is half a
cycle behind it, so one is always up while the other is down. Both legs keep
the sheet's own colours (no darker far leg). The pair is split into the far
and near layers along the gap between the boots; the B half of each cycle
uses the same drawing with the layers exchanged.

Arms: ten pixel drawings below, five for the walk's relaxed swing (fist ahead
at the waist, beside the hip, behind) and five for the run's pump (elbow near
90 degrees, fist from the chin back past the hip): a short green sleeve with
its pink trim, the bare upper arm, the brown bracer lit along one edge and a
small closed fist joined at the wrist. The arm nearest the viewer swings
against the near leg; the far arm is half a cycle behind it.

Writes assets/authored/maps/<pose>__<layer>.txt for the 16 gait poses and the
stop's arms, and the leg sources and bounds in poses.json. Then rebuild with
pipeline.py --author.
"""
import json
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
AUTH = ROOT / 'assets/authored'
MAPS = AUTH / 'maps'
SOURCE = ROOT / 'assets/source_art'
LAYERS = ('far_arm', 'far_leg', 'near_leg', 'torso', 'head_hair', 'near_arm')
TUNIC = (3, 4, 10, 12, 15)
BOOT = (5, 7, 11)
GROUND = 61                      # the sole's outline row on the ground line
FLIGHT_SOLE = 57                 # the run's flight: both soles 4 px clear of the ground
HEM = 42                         # the tunic's lowest full row at bob 0
HIP_X = 40.5                     # the hips' centre under the tunic
NEAR_SHOULDER, FAR_SHOULDER = (47, 26), (36, 26)

# per half: contact, down, passing / push-off, up / flight
PHASES = {'walk': [('WALK', 0), ('WALK', 2), ('WALK', 1), ('WALK', 5)],
          'run': [('RUN', 0), ('RUN', 1), ('RUN', 5), ('RUN', 3)]}
BOB = {'walk': [0, 1, 0, -1], 'run': [0, 2, 0, -2]}
# Holds (gait clock units per pose; 40 per step) and whole-pixel leg shifts
# under the tunic, chosen together so the planted foot travels with the road:
# at most 3 px of slip a pose at a walk (clock 5/4 of the road), 1 px at a run.
HOLDS = {'walk': [8, 10, 14, 8], 'run': [10, 14, 8, 8]}
LEG_DX = {'walk': [0, -3, -2, 3], 'run': [0, 0, 4, 0]}
GROUNDED = {'walk': [True, True, True, True], 'run': [True, True, True, False]}
# the near leg (A half) is the back one until it swings through: at the
# walk's up, and from the run's knee drive on
NEAR_IS_FRONT = {'walk': [False, False, False, True], 'run': [False, False, True, True]}
ARM_CYCLE = {'walk': ['W0', 'W1', 'W2', 'W3', 'W4', 'W3', 'W2', 'W1'],
             'run': ['R0', 'R1', 'R2', 'R3', 'R4', 'R3', 'R2', 'R1']}

# (shoulder pixel in the drawing), rows; '.' is transparent, hex digits are palette slots
ARMS = {
    'W0': ((2, 2), [
        '.111............',
        '1FFA1...........',
        '1F44A1..........',
        '1F44A1..........',
        '1F4331..........',
        '.136991.........',
        '..16991.........',
        '..166991........',
        '...1699711......',
        '....1975771.....',
        '.....1555571....',
        '......1BB559111.',
        '.......11B996691',
        '.........1166691',
        '..........199971',
        '...........1111.',
    ]),
    'W1': ((2, 2), [
        '.111........',
        '1FFA1.......',
        '1F4A1.......',
        '1F44A1......',
        '1F4431......',
        '133991......',
        '.16991......',
        '.16691......',
        '.16699......',
        '..19971.....',
        '...975B1....',
        '...1755B1...',
        '....1755B1..',
        '.....179911.',
        '.....1996691',
        '......166691',
        '......199971',
        '.......1111.',
    ]),
    'W2': ((2, 2), [
        '.111....',
        '1FFA1...',
        '1F4A1...',
        '1F4A1...',
        '1F4A1...',
        '13331...',
        '16991...',
        '16991...',
        '16991...',
        '16991...',
        '.9971...',
        '1755B1..',
        '.175B1..',
        '.175B1..',
        '.179911.',
        '..996691',
        '..166691',
        '..199971',
        '...1111.',
    ]),
    'W3': ((6, 2), [
        '.....111.',
        '....1FFA1',
        '...1F44A1',
        '...1F44A1',
        '...1344A1',
        '...166331',
        '...16971.',
        '...1699..',
        '..16691..',
        '..16971..',
        '..175B1..',
        '..175B1..',
        '.175B1...',
        '.175B1...',
        '.19991...',
        '196691...',
        '166691...',
        '199971...',
        '.1111....',
    ]),
    'W4': ((8, 2), [
        '.......111.',
        '......1FFA1',
        '.....1F44A1',
        '.....1F44A1',
        '.....1344A1',
        '....166331.',
        '....16971..',
        '...16699...',
        '...96971...',
        '..175B1....',
        '..175B1....',
        '.1755B1....',
        '.175B1.....',
        '.175B1.....',
        '.1999......',
        '196691.....',
        '166691.....',
        '199971.....',
        '.1111......',
    ]),
    'R0': ((2, 2), [
        '.111..........',
        '1FFA1....1111.',
        '1F44A1..196691',
        '1F44A1.1166691',
        '1F433117999971',
        '.136997579111.',
        '..166755591...',
        '...1695B1.....',
        '...169B1......',
        '....111.......',
    ]),
    'R1': ((2, 2), [
        '.111..........',
        '1FFA1.........',
        '1F44A1........',
        '1F44A1........',
        '1F4431........',
        '133991..11111.',
        '.1669991196691',
        '..169777966691',
        '..169555999971',
        '..169BBB91111.',
        '...11111......',
    ]),
    'R2': ((3, 2), [
        '..111........',
        '.1FFA1.......',
        '.1F4A1.......',
        '.1F4A1.......',
        '1F44A1.......',
        '133331.......',
        '.16971.......',
        '.16951111111.',
        '.169577996691',
        '.169555966691',
        '.197BBB999971',
        '..11111.1111.',
    ]),
    'R3': ((5, 2), [
        '....111....',
        '...1FFA1...',
        '..1F44A1...',
        '..1F44A1...',
        '..1334A1...',
        '.166931....',
        '.16991.....',
        '166971.....',
        '169751.....',
        '.1975B1....',
        '..175B9111.',
        '...17996691',
        '....1166691',
        '.....199971',
        '......1111.',
    ]),
    'R4': ((7, 2), [
        '......111.',
        '....11FFA1',
        '...13F44A1',
        '..113444A1',
        '.166634A1.',
        '16699731..',
        '1697991...',
        '1975B1....',
        '.975B1....',
        '.1755B1...',
        '..1759111.',
        '...1996691',
        '....166691',
        '....199971',
        '.....1111.',
    ]),
}


def read(path):
    return np.array([[0 if c == '.' else int(c, 16) for c in row]
                     for row in Path(path).read_text().splitlines()], np.uint8)


def write(path, m):
    Path(path).write_text('\n'.join(''.join('.' if v == 0 else format(v, 'X') for v in row)
                                    for row in m) + '\n')


def shift(m, dx, dy):
    out = np.zeros((64, 80), np.uint8)
    ys, xs = np.nonzero(m)
    ys2, xs2 = ys + dy, xs + dx
    ok = (ys2 >= 0) & (ys2 < 64) & (xs2 >= 0) & (xs2 < 80)
    out[ys2[ok], xs2[ok]] = m[ys[ok], xs[ok]]
    return out


def sheet(name):
    """The 80x64 cells of a source sheet, in Maiya's palette slots."""
    colors = json.loads((AUTH / 'palette.json').read_text())['colors']
    with Image.open(SOURCE / f'{name}.png') as im:
        pal, a = im.getpalette(), np.array(im)
    target = np.array(colors[1:], float)
    lut = np.zeros(256, np.uint8)
    for i in range(1, 16):
        lut[i] = 1 + int(((target - np.array(pal[i * 3:i * 3 + 3], float)) ** 2).sum(1).argmin())
    m = lut[a]
    return [m[:, k * 80:(k + 1) * 80] for k in range(a.shape[1] // 80)]


def legs(cell):
    """The leg pair below the cell's tunic, split into (back, front)."""
    tunic = np.isin(cell, TUNIC)
    hem = max(y for y in range(64) if tunic[y].sum() >= 4)
    top = hem + 2
    pair = np.zeros_like(cell)
    pair[top:] = cell[top:]
    pair[np.isin(pair, TUNIC)] = 0
    from scipy import ndimage as ndi
    lab, _ = ndi.label(pair > 0, structure=np.ones((3, 3)))
    keep = set(np.unique(lab[np.isin(pair, BOOT) & (np.arange(64)[:, None] > top + 4)])) - {0}
    pair[~np.isin(lab, list(keep))] = 0
    # the thighs run on up under the tunic, so no bob opens a gap at the hem
    row = pair[top].copy()
    for y in range(top - 5, top):
        pair[y][row > 0] = row[row > 0]
    # split: grow the two lowest boot clusters through the pair, nearest wins
    boots = np.isin(pair, BOOT) & (np.arange(64)[:, None] >= top + 6)
    blab, n = ndi.label(boots, structure=np.ones((3, 3)))
    sizes = sorted(((int((blab == i).sum()), i) for i in range(1, n + 1)), reverse=True)[:2]
    seeds = [np.nonzero(blab == i) for _, i in sizes]
    if len(sizes) < 2 or sizes[1][0] < 0.25 * sizes[0][0]:
        # the boots touch: cut the cluster at its thinnest column between them
        ys, xs = np.nonzero(blab == sizes[0][1])
        cols = range(xs.min() + 3, xs.max() - 2)
        cut = min(cols, key=lambda x: ((xs == x).sum(), abs(x - (xs.min() + xs.max()) / 2)))
        seeds = [(ys[xs < cut], xs[xs < cut]), (ys[xs > cut], xs[xs > cut])]
    owner = np.full(pair.shape, -1, int)
    frontier = []
    for k, (ys, xs) in enumerate(seeds):
        for y, x in zip(ys, xs):
            owner[y, x] = k
            frontier.append((y, x))
    while frontier:
        nxt = []
        for y, x in frontier:
            for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (-1, 1), (1, -1), (1, 1)):
                yy, xx = y + dy, x + dx
                if 0 <= yy < 64 and 0 <= xx < 80 and pair[yy, xx] and owner[yy, xx] < 0:
                    owner[yy, xx] = owner[y, x]
                    nxt.append((yy, xx))
        frontier = nxt
    a = glint(np.where(owner == 0, pair, 0))
    b = glint(np.where(owner == 1, pair, 0))
    if len(seeds) < 2 or np.nonzero(a)[1].mean() > np.nonzero(b)[1].mean():
        a, b = b, a
    ys, xs = np.nonzero(pair)
    thigh = xs[ys == top]
    hip_x = (thigh.min() + thigh.max()) / 2
    sole = ys.max()                  # the sole's outline, the leg's lowest pixel
    return a, b, hip_x, sole


def glint(leg):
    """The idle outfit's gold glint: two pixels across the boot band, just below the knee.

    The band is the first row, going down the leg, where the boot's browns
    outnumber the thigh's skin (the knee's own shading uses the same browns)."""
    out = leg.copy()
    boot = np.isin(leg, BOOT)
    skin = np.isin(leg, (6, 9))
    rows = [y for y in range(64) if boot[y].sum() >= 3 and boot[y].sum() > skin[y].sum()]
    if not rows:
        return out
    for y in (rows[0] + 1, rows[0], rows[0] + 2):
        xs = [x for x in range(1, 79) if boot[y, x] and leg[y, x - 1] not in (0, 1) and leg[y, x + 1] not in (0, 1)]
        if len(xs) >= 2:
            mid = xs[len(xs) // 2]
            for x in (mid - 1, mid):
                if boot[y, x]:
                    out[y, x] = 2
            return out
    # a slanted shin: two diagonal neighbours across the band instead
    inner = boot & (np.pad(leg, 1)[1:-1, :-2] > 1) & (np.pad(leg, 1)[1:-1, 2:] > 1)
    for y in range(rows[0], rows[0] + 4):
        for x in np.nonzero(inner[y])[0]:
            for dx in (1, -1, 0):
                if inner[y + 1, x + dx]:
                    out[y, x] = out[y + 1, x + dx] = 2
                    return out
    return out


def arm(name, shoulder, bob):
    (ax, ay), rows = ARMS[name]
    out = np.zeros((64, 80), np.uint8)
    for j, r in enumerate(rows):
        for i, c in enumerate(r):
            if c != '.':
                out[shoulder[1] + bob - ay + j, shoulder[0] - ax + i] = int(c, 16)
    return out


def compose():
    poses = json.loads((AUTH / 'poses.json').read_text())
    body = {l: read(MAPS / f'base__{l}.txt') for l in ('torso', 'head_hair')}
    cells = {n: sheet(n) for n in ('WALK', 'RUN')}
    out = {}
    for mode in ('walk', 'run'):
        for p in [p for p in poses if p['mode'] == mode]:
            k = p['slot']
            ph = k % 4
            name, cell = PHASES[mode][ph]
            back, front, hip_x, sole = legs(cells[name][cell])
            dx = int(round(HIP_X - hip_x)) + LEG_DX[mode][ph]
            dy = (GROUND if GROUNDED[mode][ph] else FLIGHT_SOLE) - sole
            back, front = shift(back, dx, dy), shift(front, dx, dy)
            near_front = NEAR_IS_FRONT[mode][ph] != (k >= 4)
            bob = BOB[mode][ph]
            # thighs exist above the hem only under the tunic: nothing pokes
            # out behind or ahead of the skirt
            torso = shift(body['torso'], 0, bob)
            hem = HEM + bob
            for x in range(80):
                col = np.nonzero(torso[:, x])[0]
                bottom = col.max() if len(col) else hem
                for leg in (back, front):
                    rows = np.arange(64)
                    stray = (rows <= max(bottom, hem - 1)) & (torso[:, x] == 0) & (leg[:, x] != 0)
                    leg[stray, x] = 0
            layers = {'far_leg': back if near_front else front,
                      'near_leg': front if near_front else back,
                      'torso': shift(body['torso'], 0, bob),
                      'head_hair': shift(body['head_hair'], 0, bob),
                      'near_arm': arm(ARM_CYCLE[mode][k], NEAR_SHOULDER, bob),
                      'far_arm': arm(ARM_CYCLE[mode][(k + 4) % 8], FAR_SHOULDER, bob)}
            p['bob'] = bob
            p['hold_distance'] = HOLDS[mode][ph]
            p['grounded'] = GROUNDED[mode][ph]
            side = ('front', 'back')
            p['near_leg'] = [f'{name}.png', cell, side[0] if near_front else side[1]]
            p['far_leg'] = [f'{name}.png', cell, side[1] if near_front else side[0]]
            out[p['name']] = layers
    stop = next(p for p in poses if p['mode'] == 'stop')
    layers = {l: read(MAPS / f"stop__{l}.txt") for l in LAYERS}
    for l in ('near_leg', 'far_leg'):       # both boot bands in the same gold
        layers[l][layers[l] == 13] = 2
    layers['near_arm'] = arm('W2', NEAR_SHOULDER, stop['bob'])
    layers['far_arm'] = arm('W2', FAR_SHOULDER, stop['bob'])
    out['stop'] = layers
    for p in poses:
        flat = np.zeros((64, 80), np.uint8)
        for l in LAYERS:
            m = out[p['name']][l]
            flat[m > 0] = m[m > 0]
        ys, xs = np.nonzero(flat)
        p['bounds'] = [int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())]
    return poses, out


def hands(poses, out):
    """Each fist's box: the skin cluster at the end of the arm, furthest from its shoulder."""
    from scipy import ndimage as ndi
    boxes = []
    for p in poses:
        for layer, shoulder in (('far_arm', FAR_SHOULDER), ('near_arm', NEAR_SHOULDER)):
            arm_map = out[p['name']][layer]
            lab, n = ndi.label(np.isin(arm_map, (6, 9)))
            sx, sy = shoulder[0], shoulder[1] + p['bob']
            far = max(range(1, n + 1), key=lambda i: max((x - sx) ** 2 + (y - sy) ** 2
                                                         for y, x in zip(*np.nonzero(lab == i))))
            ys, xs = np.nonzero(lab == far)
            fist_x = (xs.min() + xs.max()) / 2
            boxes.append(dict(pose=p['name'], layer=layer,
                              side='front' if fist_x > shoulder[0] else 'back',
                              box=[int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())]))
    return boxes


def main():
    poses, out = compose()
    for name, layers in out.items():
        for l in LAYERS:
            write(MAPS / f'{name}__{l}.txt', layers[l])
    (AUTH / 'hand-repair.json').write_text(json.dumps(dict(
        method='compose_gait.py: each closed fist is part of its drawn arm, joined to the '
               'bracer at the wrist; the box is the skin cluster at the end of the arm.',
        hands=hands(poses, out), edits=[]), indent=2) + '\n')
    (AUTH / 'poses.json').write_text(json.dumps(poses, indent=2) + '\n')
    print(f'{len(out)} poses written')


def accept():
    """Record the composed layers as the accepted art (cute-polish.json): any
    later pixel edit to a map, or a change of poses or palette, fails
    check_cute_polish until it is reviewed and accepted again."""
    import hashlib
    sha = lambda b: hashlib.sha256(b).hexdigest()
    poses = json.loads((AUTH / 'poses.json').read_text())
    layers, edits = [], []
    for p in poses:
        for l in LAYERS:
            name = f"{p['name']}__{l}.txt"
            m = read(MAPS / name)
            layers.append(dict(file=name, before=sha(m.tobytes()), after=sha(m.tobytes()),
                               silhouette=sha((m != 0).tobytes())))
            edits.append(dict(file=name, pixels=[]))
    (AUTH / 'cute-polish.json').write_text(json.dumps(dict(
        method='compose_gait.py: the redrawn walk and run accepted as drawn; no colour patches since',
        layers=layers, edits=edits,
        poses_sha256=sha((AUTH / 'poses.json').read_bytes()),
        palette_sha256=sha((AUTH / 'palette.json').read_bytes()),
        pixel_changes=0), indent=2) + '\n')


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--accept', action='store_true',
                        help='also record the composed layers as the accepted art')
    args = parser.parse_args()
    main()
    if args.accept:
        accept()
        print('accepted: cute-polish.json records the composed layers')
