"""Maiya's walk, run and stop, assembled from her own native pixels.

Every frame shares one upper body -- her head, rose, ponytail and tunic
from the stride drawing on her original heroine sheet, the sheet all her
other moves (throws, whip, jumps, landing, victory) are cut from -- so
walking and running look like the same girl as the rest of the game. It is
moved only by whole-pixel bob. Her arms swing with her legs between the
stride's reach (front fist forward, back hand behind) and the same sheet's
passing figure's arms hanging by her hips, the arm nearest the viewer
changing every step. Her legs are cut
from the native 80x64 WALK.png / RUN.png sheets (assets/source_art), below
each sheet's skirt hem, and placed under the skirt by whole pixels, both at
full brightness as on her original drawings. No scaling, resampling,
blending or mirroring.

The text maps written here are the reviewed pixels that build_walkrun.lua
turns into the layered Aseprite source.
"""
from pathlib import Path
import importlib.util
import json
import sys
import numpy as np
from PIL import Image

GAME = Path(__file__).resolve().parents[2]
ROOT = GAME / 'assets/authored'
SOURCE = GAME / 'assets/source_art'
SIZE = (80, 64)
# Bottom to top. Legs sit under the skirt; the near arm swings over the tunic.
LAYERS = ('far_arm', 'far_leg', 'near_leg', 'torso', 'head_hair', 'near_arm')
PALETTE = json.loads((ROOT / 'palette.json').read_text())
HERO = PALETTE['colors']
BASELINE = PALETTE['baseline']
sys.path.insert(0, str(GAME / 'tools'))
from hair_specks import smooth_hair  # noqa: E402

# Hero palette indices: 1 outline, 5/7/11/13 boot browns, 6/9 skin.
LEG_COLOURS = (1, 5, 6, 7, 9, 11, 13)


def sheet(name):
    """The eight 80x64 cells of a native sheet, in hero palette indices.

    The sheets were drawn on her own 15 colours (in another slot order);
    each slot maps to its hero twin. The one dark accent with no twin is
    read as outline, as the game's own run0 import does."""
    im = Image.open(SOURCE / f'{name}.png')
    if im.mode != 'P' or im.size != (8 * SIZE[0], SIZE[1]):
        raise ValueError(f'{name}.png: expected an indexed 640x64 sheet')
    pal = im.getpalette()[:48]
    lut = np.zeros(256, np.uint8)
    for i in range(1, 16):
        c = np.array(pal[i * 3:i * 3 + 3])
        d = [int(((c - np.array(h)) ** 2).sum()) for h in HERO[1:]]
        lut[i] = 1 + int(np.argmin(d)) if min(d) <= 300 else 1
    a = lut[np.asarray(im)]
    return [a[:, i * 80:(i + 1) * 80].copy() for i in range(8)]


def original_figures():
    """Her stride and passing figures (the heroine sheet's second and first
    jump-row drawings), cut, fitted and reduced to the hero palette exactly
    as build_commercial_assets.py cuts her other poses, at her sprint
    height, hair specks removed."""
    spec = importlib.util.spec_from_file_location('maiya_assets_builder', GAME / 'tools/build_commercial_assets.py')
    bca = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(bca)
    sheet_art = Image.open(bca.find_file('maiya_heroine*.jpg')).convert('RGB')
    sprint = {'run0': (354, 70, 488, 268), 'run1': (515, 70, 664, 268),
              'run2': (682, 70, 834, 268), 'run3': (852, 70, 997, 268)}
    fitted = bca.fit_group(sheet_art, sprint, bca.HERO_CANVAS, bca.HERO_HEIGHT)
    heights = []
    for frame in fitted.values():
        rows_ = np.nonzero(bca.clean_sprite(frame, white_area=10)[:, :, 3].any(axis=1))[0]
        heights.append(int(rows_[-1] - rows_[0] + 1))
    run_h = int(np.median(heights))
    master = np.array(HERO[1:], dtype=np.uint8)
    out = []
    for name, box in (('stride', (189, 310, 341, 520)), ('pass', (37, 310, 156, 520))):
        figure = bca.fit_group(sheet_art, {name: box}, bca.HERO_CANVAS, run_h)[name]
        figure = bca.clean_sprite(figure, white_area=10)
        indices, _, _ = bca.quantize(np.asarray(figure, np.uint8), 1, master, dither='none')
        out.append(smooth_hair(np.asarray(indices, np.uint8)))
    return out


def save_reference(a, name):
    im = Image.fromarray(a, 'P')
    im.putpalette(sum(HERO, []) + [0] * 720)
    im.save(ROOT / f'{name}.png', transparency=0)


WALK, RUN = sheet('WALK'), sheet('RUN')
MASTER, PASSING = original_figures()
HEM = 43   # the stride's skirt bottom row

# Leg sources: sheet cell, the row of that cell's skirt hem, and the column
# dividing its back leg from its front leg (per row where the legs meet).
LEGS = {
    'walk_contact': (WALK[0], 42, lambda y: 39),
    'walk_passing': (WALK[1], 42, lambda y: 39 if y < 50 else 38),
    'run_contact': (RUN[0], 42, lambda y: 41),
    'run_pass': (RUN[1], 41, lambda y: 38 if y < 49 else 37),
    'run_push': (RUN[5], 41, lambda y: 38 if y <= 47 else 35),
    'run_flight': (RUN[3], 40, lambda y: 41),
    'run_tuck': (RUN[7], 42, lambda y: 42 if y <= 45 else (39 if y == 46 else (38 if y <= 48 else 36))),
}

# The stride's arms, row by row (inclusive columns): the back hand low
# behind her, the front fist forward at her chest.
BACK_ARM_ROWS = {25: (32, 35), 26: (31, 35), 27: (31, 35), 28: (30, 34), 29: (30, 34), 30: (29, 33),
                 31: (29, 33), 32: (28, 33), 33: (28, 33), 34: (28, 33), 35: (29, 34), 36: (30, 31)}
FRONT_ARM_ROWS = {27: (56, 59), 28: (55, 60), 29: (52, 60), 30: (50, 60), 31: (52, 60), 32: (52, 59),
                  33: (52, 57), 34: (51, 55), 35: (50, 54)}
# The passing figure's arms hanging by her hips (its own coordinates); it
# sits one pixel up and left of the stride (their heads match there).
PASS_BACK_ROWS = {26: (32, 35), 27: (32, 35), 28: (32, 35), 29: (32, 35), 30: (32, 36), 31: (32, 36),
                  32: (32, 36), 33: (32, 36), 34: (33, 36), 35: (33, 36), 36: (32, 38), 37: (32, 38),
                  38: (32, 38), 39: (32, 38), 40: (34, 38)}
PASS_FRONT_ROWS = {33: (47, 50), 34: (46, 54), 35: (46, 55), 36: (48, 55), 37: (48, 55), 38: (48, 55),
                   39: (52, 54)}
PASS_TO_STRIDE = (1, 1)


def shift(a, dx, dy):
    out = np.zeros_like(a)
    ys, xs = np.nonzero(a)
    ny, nx = ys + dy, xs + dx
    ok = (ny >= 0) & (ny < a.shape[0]) & (nx >= 0) & (nx < a.shape[1])
    out[ny[ok], nx[ok]] = a[ys[ok], xs[ok]]
    return out


def over(dst, src):
    dst[src > 0] = src[src > 0]


def rows(a, spans):
    out = np.zeros_like(a)
    for y, (lo, hi) in spans.items():
        out[y, lo:hi + 1] = a[y, lo:hi + 1]
    return out


def components(mask):
    seen = np.zeros(mask.shape, bool)
    found = []
    for y, x in zip(*np.nonzero(mask)):
        if seen[y, x]:
            continue
        todo, part = [(int(y), int(x))], []
        seen[y, x] = True
        while todo:
            u, v = todo.pop()
            part.append((u, v))
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    a, b = u + dy, v + dx
                    if 0 <= a < mask.shape[0] and 0 <= b < mask.shape[1] and mask[a, b] and not seen[a, b]:
                        seen[a, b] = True
                        todo.append((a, b))
        found.append(part)
    return found


BACK_ARM = rows(MASTER, BACK_ARM_ROWS)
FRONT_ARM = rows(MASTER, FRONT_ARM_ROWS)
UPPER = MASTER.copy()
UPPER[HEM + 1:] = 0
UPPER[HEM, :35] = 0          # on the hem row, only the skirt's own bottom
UPPER[HEM, 45:] = 0
UPPER[BACK_ARM > 0] = 0
UPPER[FRONT_ARM > 0] = 0
# Her head and hair above the collar; everything else is the tunic.
HEAD = np.zeros_like(UPPER)
HEAD[:23] = UPPER[:23]
HEAD[23:28, :33] = UPPER[23:28, :33]
TORSO = UPPER.copy()
TORSO[HEAD > 0] = 0


def move_block(arm, r0, r1, columns, dx, dy):
    """Move the part of an arm in rows r0..r1 and the given columns
    (slice) by whole pixels: a wrist and fist swinging at the elbow."""
    part = np.zeros_like(arm)
    part[r0:r1 + 1, columns] = arm[r0:r1 + 1, columns]
    out = arm.copy()
    out[part > 0] = 0
    over(out, shift(part, dx, dy))
    return out


# Swing 0 is the stride's own reach; 1 halfway (the front fist lowered at
# the wrist, the back arm drawn in toward her side); 2 the passing figure's
# arms hanging by her hips.
BACK_SWING = (BACK_ARM,
              shift(BACK_ARM, 2, 2),
              shift(rows(PASSING, PASS_BACK_ROWS), *PASS_TO_STRIDE))
FRONT_SWING = (FRONT_ARM,
               move_block(FRONT_ARM, 27, 32, slice(54, 80), -1, 4),
               shift(rows(PASSING, PASS_FRONT_ROWS), *PASS_TO_STRIDE))


def leg(source, which, dx=0, dy=0, extend=0):
    """One leg below a sheet cell's skirt, moved by whole pixels. `extend`
    repeats its top row upwards to reach a skirt that bobbed higher."""
    cell, hem, split = LEGS[source]
    out = np.zeros_like(cell)
    for y in range(hem + 1, SIZE[1]):
        for x in range(SIZE[0]):
            v = cell[y, x]
            if v in LEG_COLOURS and (x < split(y)) == (which == 'back'):
                out[y, x] = v
    out = shift(out, dx, dy)
    if extend:
        top = int(np.nonzero(out.any(1))[0][0])
        for k in range(1, extend + 1):
            out[top - k][out[top] > 0] = out[top][out[top] > 0]
    # flecks of the sheet's own skirt outline left on the cut
    for part in components(out != 0):
        if len(part) < 4:
            for y, x in part:
                out[y, x] = 0
    return out


# One step each; in the 'a' half her near leg starts behind her, so her near
# arm (the stride's front arm) starts forward and swings back through her
# side; in the 'b' half the near arm is the one behind and swings forward.
# (name, far leg, near leg, bob, arm swing, near arm, hold, grounded)
# Bob is in pixels (+ lower). Holds are gait clock units; a cycle is 80 (a
# walk covers 64 px of road in it, a run 80).
WALK_STEP = (
    ('contact', ('walk_contact', 'front'), ('walk_contact', 'back'), 0, 0, 'front', 12, True),
    ('down', ('walk_contact', 'front', -3), ('walk_contact', 'back', 0, -1), 1, 1, 'front', 8, True),
    ('passing', ('walk_passing', 'front', -1), ('walk_passing', 'back', 4, -1), 0, 2, 'front', 10, True),
    ('up', ('walk_contact', 'back', 6, 0), ('walk_contact', 'front', -6, -2), -1, 1, 'back', 10, True),
)
RUN_STEP = (
    ('contact', ('run_contact', 'front'), ('run_contact', 'back'), 0, 0, 'front', 8, True),
    ('down', ('run_pass', 'front', 4, 1), ('run_tuck', 'back', 3, 0, 1), 2, 1, 'front', 10, True),
    ('push', ('run_pass', 'back', 0, 3, 2), ('run_push', 'front', 1, 1), 0, 2, 'front', 8, True),
    ('flight', ('run_flight', 'back'), ('run_flight', 'front'), -2, 1, 'back', 14, False),
)
STOP = ('stop', ('walk_passing', 'back', 4, 1), ('walk_passing', 'front', -1), 0, 2, 'front', 0, True)


def compose(far, near, bob, swing, near_arm):
    back, front = BACK_SWING[swing], FRONT_SWING[swing]
    far_arm, near_arm_px = (back, front) if near_arm == 'front' else (front, back)
    layers = {
        'far_arm': shift(far_arm, 0, bob),
        'far_leg': leg(*far),
        'near_leg': leg(*near),
        'torso': shift(TORSO, 0, bob),
        'head_hair': shift(HEAD, 0, bob),
        'near_arm': shift(near_arm_px, 0, bob),
    }
    flat = np.zeros((SIZE[1], SIZE[0]), np.uint8)
    for name in LAYERS:
        over(flat, layers[name])
    return layers, flat


def write_map(name, layer, a):
    rows_ = (''.join('.' if v == 0 else format(int(v), 'X') for v in row) for row in a)
    (ROOT / 'maps' / f'{name}__{layer}.txt').write_text('\n'.join(rows_) + '\n')


def poses():
    """Every pose in ROM order: walk 0-7, run 0-7, stop."""
    out = []
    for mode, step in (('walk', WALK_STEP), ('run', RUN_STEP)):
        slot = 0
        for half in ('a', 'b'):
            for name, far, near, bob, swing, near_arm, hold, grounded in step:
                if half == 'b':
                    # The other foot: the same drawings, near and far exchanged.
                    far, near = near, far
                    near_arm = 'back' if near_arm == 'front' else 'front'
                out.append(dict(name=f'{mode}_{name}_{half}', mode=mode, slot=slot, far=far, near=near,
                                bob=bob, swing=swing, near_arm=near_arm, hold_distance=hold,
                                grounded=grounded, half=half))
                slot += 1
    name, far, near, bob, swing, near_arm, hold, grounded = STOP
    out.append(dict(name=name, mode='stop', slot=0, far=far, near=near, bob=bob, swing=swing,
                    near_arm=near_arm, hold_distance=hold, grounded=grounded, half=None))
    return out


def main():
    (ROOT / 'maps').mkdir(exist_ok=True)
    for old in (ROOT / 'maps').glob('*.txt'):
        old.unlink()
    save_reference(MASTER, 'reference_stride')
    save_reference(PASSING, 'reference_pass')
    metadata = []
    for p in poses():
        layers, flat = compose(p['far'], p['near'], p['bob'], p['swing'], p['near_arm'])
        for layer, a in layers.items():
            write_map(p['name'], layer, a)
        ys, xs = np.nonzero(flat)
        meta = {k: p[k] for k in ('name', 'mode', 'slot', 'half', 'bob', 'swing', 'near_arm',
                                  'hold_distance', 'grounded')}
        meta.update(far_leg=list(p['far']), near_leg=list(p['near']),
                    bounds=[int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())])
        metadata.append(meta)
    (ROOT / 'poses.json').write_text(json.dumps(metadata, indent=2) + '\n')
    for name in ('reference_stride', 'reference_idle0'):
        a = np.array(Image.open(ROOT / f'{name}.png'))
        rows_ = (''.join('.' if v == 0 else format(int(v), 'X') for v in row) for row in a)
        (ROOT / 'maps' / f'{name}.txt').write_text('\n'.join(rows_) + '\n')
    print(f'Authored {len(metadata)} poses from her original stride and passing figures and the native WALK/RUN legs')


if __name__ == '__main__':
    main()
