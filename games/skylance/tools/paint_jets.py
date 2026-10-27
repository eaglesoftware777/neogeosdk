"""Player jets: every in-game frame from three paintings of each aircraft.

assets/painted/sources/<model>_top.png, _side.png and _bottom.png are
full-colour paintings on transparency, nose up, all at one scale. From them
this writes each pilot's 24 frames (seven bank poses, four roll/loop poses,
seven damaged bank poses, two boost, four destruction) to
assets/painted/jets/<model>/, their ground shadows, and the families in
assets/painted/painted.json.

Banking is a roll about the jet's long axis: seen from above the wings
shorten by cos(angle), the wing dipping toward the light brightens and the
rising wing darkens. At 90 degrees the camera sees the side painting; past
that, the underside.
"""
import json
import math
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1] / 'assets'
PAINTED = ROOT / 'painted'
SOURCES = PAINTED / 'sources'
MODELS = ('f16', 'f35', 'f15')            # pilot order, as aircraft_models.json
CANVAS = 64
BODY = 50                                 # nose to flame tip, native pixels
TOP = 3                                   # first row of the nose
SHADOW = 48                               # the ground shadow's canvas (three strips)
BANKS = (-45, -30, -15, 0, 15, 30, 45)
SHADOW_GREYS = [0, 0, 0] + [v for g in range(20, 231, 15) for v in (g, g, g)]


def painting(model, view):
    with Image.open(SOURCES / f'{model}_{view}.png') as im:
        rgba = np.array(im.convert('RGBA')).astype(np.float32)
    rgba[rgba[:, :, 3] < 128] = 0
    rgba[:, :, 3] = np.where(rgba[:, :, 3] >= 128, 255, 0)
    matte = (rgba[:, :, 0] - rgba[:, :, 1] > 60) & (rgba[:, :, 2] - rgba[:, :, 1] > 60)
    rgba[matte] = 0                                          # background matte left in a gap
    return rgba


def length(rgba):
    """Nose to flame tip, in the painting's pixels."""
    ys = np.nonzero(rgba[:, :, 3].any(1))[0]
    return ys[-1] - ys[0] + 1


def roll(rgba, angle, light=0.4):
    """Squash about the fuselage centreline by cos(angle); shade wing by wing."""
    h, w = rgba.shape[:2]
    alpha = rgba[:, :, 3] > 0
    xs = np.nonzero(alpha.any(0))[0]
    cx = float((np.nonzero(alpha)[1]).mean())
    half = max(cx - xs[0], xs[-1] - cx)
    c = max(math.cos(math.radians(angle)), 0.05)
    s = math.sin(math.radians(angle))
    out = np.zeros_like(rgba)
    for x in range(w):
        src = cx + (x - cx) / c
        sx = int(round(src))
        if 0 <= sx < w:
            out[:, x] = rgba[:, sx]
    # positive angle: right wing down, away from the upper-left light
    side = np.tanh((np.arange(w) - cx) / (half * 0.35))
    factor = 1.0 - light * s * side
    out[:, :, :3] = np.clip(out[:, :, :3] * factor[None, :, None], 0, 255)
    return out


def scale_to(rgba, factor):
    h, w = rgba.shape[:2]
    size = (max(1, round(w * factor)), max(1, round(h * factor)))
    im = Image.fromarray(rgba.astype(np.uint8), 'RGBA')
    # premultiply so transparent pixels never bleed colour into the edges
    rgb = Image.fromarray((rgba[:, :, :3] * (rgba[:, :, 3:] / 255)).astype(np.uint8), 'RGB')
    a = im.getchannel('A').resize(size, Image.Resampling.BOX)
    rgb = rgb.resize(size, Image.Resampling.BOX)
    out = np.dstack([np.array(rgb), np.array(a)]).astype(np.float32)
    cover = out[:, :, 3:] / 255
    out[:, :, :3] = np.where(cover > 0, out[:, :, :3] / np.maximum(cover, 1e-3), 0)
    out[:, :, 3] = np.where(out[:, :, 3] >= 110, 255, 0)
    out[out[:, :, 3] == 0] = 0
    return out


def place(native, nose_row=TOP, size=CANVAS):
    """Centre on the canvas, nose on its row; the pivot is the canvas centre."""
    CANVAS = size
    canvas = np.zeros((CANVAS, CANVAS, 4), np.float32)
    ys, xs = np.nonzero(native[:, :, 3])
    if not len(ys):
        return canvas
    native = native[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    h, w = native.shape[:2]
    x0 = (CANVAS - w) // 2
    y0 = min(max(1, nose_row), CANVAS - 1 - h)
    if x0 < 1 or h > CANVAS - 2:
        raise ValueError(f'frame {w}x{h} does not fit the {CANVAS}-pixel canvas')
    canvas[y0:y0 + h, x0:x0 + w] = native
    return canvas


def outline(frame, colour=(14, 16, 22)):
    """A one-pixel dark contour where the airframe meets transparency."""
    a = frame[:, :, 3] > 0
    pad = np.pad(a, 1)
    edge = a & ~(pad[:-2, 1:-1] & pad[2:, 1:-1] & pad[1:-1, :-2] & pad[1:-1, 2:])
    out = frame.copy()
    out[edge, :3] = out[edge, :3] * 0.35 + np.array(colour) * 0.65
    return out


FIRE = np.array([[255, 248, 200], [255, 200, 70], [250, 120, 30], [190, 40, 20]], np.float32)


def boosted(rgba, stretch):
    """Afterburner: the painted flame below the nozzles drawn `stretch` times longer."""
    # orange and yellow flame, and its white-hot core
    warm = (rgba[:, :, 0] > 230) & (rgba[:, :, 1] > 90) & ((rgba[:, :, 2] < 150) | (rgba[:, :, 1] > 230)) & (rgba[:, :, 3] > 0)
    on = rgba[:, :, 3] > 0
    h = rgba.shape[0]
    start = h
    for y in range(h - 1, int(h * 0.6), -1):
        if on[y].any() and (warm[y] | ~on[y]).all():
            start = y
        elif start < h:
            break
    if start >= h:
        return rgba
    band = Image.fromarray(rgba[start:].astype(np.uint8), 'RGBA')
    band = np.array(band.resize((band.width, round(band.height * stretch)), Image.Resampling.NEAREST)).astype(np.float32)
    return np.concatenate([rgba[:start], band], axis=0)


def scorch(frame, cx, cy, r, amount=0.55):
    out = frame.copy()
    yy, xx = np.mgrid[:CANVAS, :CANVAS]
    d = np.hypot(xx - cx, yy - cy)
    near = (d <= r) & (out[:, :, 3] > 0)
    out[near, :3] *= (1 - amount * (1 - d[near] / (r + 1)))[:, None]
    return out


def fire(frame, cx, cy, r):
    out = frame.copy()
    yy, xx = np.mgrid[:CANVAS, :CANVAS]
    d = np.hypot(xx - cx, yy - cy)
    for k, limit in enumerate((0.35, 0.6, 0.85, 1.0)):
        ring = (d <= r * limit) & (out[:, :, 3] > 0) & (out[:, :, 0] >= 0)
        sel = ring & ~np.isin(out[:, :, 0], FIRE[:k, 0]) if k else ring
        out[sel, :3] = FIRE[k]
    return out


def damaged(frame):
    a = frame[:, :, 3] > 0
    ys, xs = np.nonzero(a)
    cy, cx = ys.mean(), xs.mean()
    w = xs.max() - xs.min()
    f = scorch(frame, cx + w * 0.22, cy + 2, max(2.5, w * 0.16))
    f = scorch(f, cx - w * 0.18, cy + 8, max(2, w * 0.1), 0.4)
    return fire(f, cx + w * 0.22, cy + 2, max(1.5, w * 0.07))


def destruction(level):
    a = level[:, :, 3] > 0
    ys, xs = np.nonzero(a)
    cy, cx = ys.mean(), xs.mean()
    frames = []
    flash = level.copy()
    flash[a, :3] = np.clip(flash[a, :3] * 0.6 + 110, 0, 255)
    frames.append(fire(flash, cx, cy, 7))
    yy, xx = np.mgrid[:CANVAS, :CANVAS]
    along = np.round((xx - cx) * math.cos(0.5) + (yy - cy) * math.sin(0.5)).astype(int)
    tear = np.array([0, 1, 2, 1, 0, -1, 1, 0, -2, -1])[along % 10]
    side = (xx - cx) * math.sin(0.5) - (yy - cy) * math.cos(0.5) + tear > 0
    for (dx, dy), dark, embers in (((2, 2), 0.75, 0.9), ((5, 4), 0.5, 0.6), ((8, 7), 0.28, 0.3)):
        out = np.zeros_like(level)
        for piece, sx, sy in ((side, -dx, -dy + 1), (~side, dx, dy)):
            part = np.where((piece & a)[:, :, None], level, 0)
            part[:, :, :3] *= dark
            moved = np.zeros_like(part)
            py, px = np.nonzero(part[:, :, 3])
            ny, nx = py + sy, px + sx
            ok = (ny > 0) & (ny < CANVAS - 1) & (nx > 0) & (nx < CANVAS - 1)
            moved[ny[ok], nx[ok]] = part[py[ok], px[ok]]
            edge = moved[:, :, 3] > 0
            pad = np.pad(edge, 1)
            rim = edge & ~(pad[:-2, 1:-1] & pad[2:, 1:-1] & pad[1:-1, :-2] & pad[1:-1, 2:])
            rng = np.random.default_rng(int(dx * 7 + sy))
            hot = rim & (rng.random(rim.shape) < embers)
            moved[hot, :3] = FIRE[rng.integers(1, 4, hot.sum())]
            out = np.where(moved[:, :, 3:] > 0, moved, out)
        frames.append(out)
    return frames


def shadow(frame):
    return np.where(frame[:, :, 3] > 0, 2, 0).astype(np.uint8)


def save_rgba(frame, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    Image.fromarray(np.clip(frame, 0, 255).astype(np.uint8), 'RGBA').save(path)


def build(model, pilot):
    top, side, bottom = (painting(model, v) for v in ('top', 'side', 'bottom'))

    level = BODY / length(top)

    def native(rgba, factor=level):
        return place(scale_to(rgba, factor))
    frames = {}
    poses = [outline(native(roll(top, a))) for a in BANKS]
    prefix = f'p{pilot + 1}_plane'
    for name, k in ((prefix, 3), (prefix + '_left', 0), (prefix + '_left_mid', 1), (prefix + '_left_soft', 2),
                    (prefix + '_right_soft', 4), (prefix + '_right_mid', 5), (prefix + '_right', 6)):
        frames[name] = poses[k]
    # barrel roll: 60 degrees, knife edge (side painting), 120 (underside); loop: pitched up
    # the side and underside paintings come at their own scale: match their length
    knife = outline(native(side, BODY / length(side)))
    under = outline(native(roll(bottom[:, ::-1], 60, light=0.15), BODY / length(bottom)))
    pitched = Image.fromarray(top.astype(np.uint8), 'RGBA')
    pitched = np.array(pitched.resize((pitched.width, round(pitched.height * 0.8)), Image.Resampling.BOX)).astype(np.float32)
    pitched[:, :, 3] = np.where(pitched[:, :, 3] >= 128, 255, 0)
    pitched[pitched[:, :, 3] == 0] = 0
    for i, f in enumerate((outline(native(roll(top, 60))), knife, under, outline(native(pitched)))):
        frames[f'maneuver_{pilot}_{i}'] = f
    for i, pose in enumerate(poses):
        frames[f'native_damage_{pilot}_{i}'] = damaged(pose)
    frames[f'native_boost_{pilot}_0'] = outline(native(boosted(top, 2.2)))
    frames[f'native_boost_{pilot}_1'] = outline(native(boosted(top, 3.2)))
    for i, f in enumerate(destruction(poses[3])):
        frames[f'native_death_{pilot}_{i}'] = f
    members = {}
    for name, f in frames.items():
        rel = Path('jets') / model / f'{name}.png'
        save_rgba(f, PAINTED / rel)
        members[name] = str(rel)
    for i, a in enumerate(BANKS):
        small = place(scale_to(roll(top, a), level * 0.75), TOP, SHADOW)
        im = Image.fromarray(shadow(small), 'P')
        im.putpalette(SHADOW_GREYS + [0] * 720)
        path = PAINTED / 'jets' / model / f'shadow_{i}.png'
        im.save(path, transparency=0)
    return {'name': f'pilot_{pilot}', 'bank': 44 + pilot * 4, 'banks': 4,
            'pivot': [CANVAS // 2, CANVAS // 2], 'members': members}


def families():
    manifest = PAINTED / 'painted.json'
    return json.loads(manifest.read_text()) if manifest.exists() else []


def run(models=MODELS):
    found = [m for m in models if all((SOURCES / f'{m}_{v}.png').exists() for v in ('top', 'side', 'bottom'))]
    keep = [f for f in families() if not f['name'].startswith('pilot_')]
    jets = [build(m, MODELS.index(m)) for m in found]
    (PAINTED / 'painted.json').write_text(json.dumps(keep + jets, indent=1) + '\n')
    print(f'Painted jets: {", ".join(found) or "none"}')


if __name__ == '__main__':
    run()
