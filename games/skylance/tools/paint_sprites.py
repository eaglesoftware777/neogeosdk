"""Bosses, boats, submarines and explosions from their paintings.

Each source in assets/painted/sources is a full-colour cut-out on
transparency, facing down the screen. This reduces it to its native canvas,
derives the frames the game shows (idle glow, damage, destruction) and
records the families in assets/painted/painted.json; tools/painted.py fits
each family to its palette banks when the art is built.
"""
import json
import math
from pathlib import Path

import numpy as np
from PIL import Image

import instruments
import pilot_cards

ROOT = Path(__file__).resolve().parents[1] / 'assets/painted'
SOURCES = ROOT / 'sources'
FIRE = np.array([[255, 248, 200], [255, 200, 70], [250, 120, 30], [190, 40, 20]], np.float32)


def source(name):
    with Image.open(SOURCES / f'{name}.png') as im:
        rgba = np.array(im.convert('RGBA')).astype(np.float32)
    rgba[:, :, 3] = np.where(rgba[:, :, 3] >= 128, 255, 0)
    # background matte the cut-out kept (gaps enclosed by chains, struts or smoke)
    matte = (rgba[:, :, 0] - rgba[:, :, 1] > 60) & (rgba[:, :, 2] - rgba[:, :, 1] > 60)
    rgba[matte, 3] = 0
    rgba[rgba[:, :, 3] == 0] = 0
    return rgba


def reduce(rgba, size):
    """Box-filter to `size` (w, h) with premultiplied alpha and a hard edge."""
    a = Image.fromarray(rgba[:, :, 3].astype(np.uint8)).resize(size, Image.Resampling.BOX)
    pre = Image.fromarray((rgba[:, :, :3] * rgba[:, :, 3:] / 255).astype(np.uint8)).resize(size, Image.Resampling.BOX)
    al = np.asarray(a).astype(np.float32)
    rgb = np.asarray(pre).astype(np.float32) / np.maximum(al[:, :, None] / 255, 1e-3)
    out = np.dstack([np.clip(rgb, 0, 255), np.where(al >= 110, 255, 0)]).astype(np.float32)
    out[out[:, :, 3] == 0] = 0
    return out


def fit(rgba, canvas, content):
    """Scale the painting so its longer side is `content` pixels, centred on the canvas."""
    h, w = rgba.shape[:2]
    cw, ch = canvas
    f = min(content / max(h, w), (cw - 2) / w, (ch - 2) / h)
    small = reduce(rgba, (max(1, round(w * f)), max(1, round(h * f))))
    out = np.zeros((ch, cw, 4), np.float32)
    sh, sw = small.shape[:2]
    x0, y0 = (cw - sw) // 2, (ch - sh) // 2
    if x0 < 1 or y0 < 1:
        raise ValueError(f'{sw}x{sh} does not fit {canvas} inside a one-pixel margin')
    out[y0:y0 + sh, x0:x0 + sw] = small
    return out


def outline(frame, colour=(12, 12, 18), mix=0.65):
    a = frame[:, :, 3] > 0
    pad = np.pad(a, 1)
    edge = a & ~(pad[:-2, 1:-1] & pad[2:, 1:-1] & pad[1:-1, :-2] & pad[1:-1, 2:])
    out = frame.copy()
    out[edge, :3] = out[edge, :3] * (1 - mix) + np.array(colour) * mix
    return out


def glow(frame, gain):
    """Brighten the warm lights (engines, reactor, lamps) by `gain`."""
    out = frame.copy()
    warm = (out[:, :, 0] > 180) & (out[:, :, 1] > 90) & (out[:, :, 2] < 130) & (out[:, :, 3] > 0)
    out[warm, :3] = np.clip(out[warm, :3] * gain, 0, 255)
    return out


def scorch(frame, cx, cy, r, amount=0.55):
    out = frame.copy()
    h, w = out.shape[:2]
    yy, xx = np.mgrid[:h, :w]
    d = np.hypot(xx - cx, yy - cy)
    near = (d <= r) & (out[:, :, 3] > 0)
    out[near, :3] *= (1 - amount * (1 - d[near] / (r + 1)))[:, None]
    return out


def fire(frame, cx, cy, r):
    out = frame.copy()
    h, w = out.shape[:2]
    yy, xx = np.mgrid[:h, :w]
    d = np.hypot(xx - cx, yy - cy)
    for k, limit in ((3, 1.0), (2, 0.8), (1, 0.55), (0, 0.3)):
        sel = (d <= r * limit) & (out[:, :, 3] > 0)
        out[sel, :3] = FIRE[k]
    return out


def damaged(frame, spots, fires=True):
    """Scorched plating with fires at the given fractional (x, y) spots. A jet
    gets charred cores instead (fires=False): on a hull that small a fire is
    an enemy orb's size and colour, and it read as a shot stuck to the wing."""
    a = frame[:, :, 3] > 0
    ys, xs = np.nonzero(a)
    x0, y0, w, h = xs.min(), ys.min(), xs.max() - xs.min(), ys.max() - ys.min()
    out = frame
    for fx, fy, fr in spots:
        cx, cy, r = x0 + fx * w, y0 + fy * h, max(2.0, fr * w)
        out = scorch(out, cx, cy, r * 1.8)
        out = fire(out, cx, cy, r * 0.7) if fires else scorch(out, cx, cy, r * 0.7, 0.8)
    return out


def destruction(frame, seed=1):
    """Flash, then the hull tears in two along a ragged line and burns away."""
    h, w = frame.shape[:2]
    a = frame[:, :, 3] > 0
    ys, xs = np.nonzero(a)
    cy, cx = ys.mean(), xs.mean()
    span = min(xs.max() - xs.min(), ys.max() - ys.min())
    flash = frame.copy()
    flash[a, :3] = np.clip(flash[a, :3] * 0.55 + 115, 0, 255)
    frames = [fire(flash, cx, cy, span * 0.22)]
    yy, xx = np.mgrid[:h, :w]
    t = 0.55
    along = np.round((xx - cx) * math.cos(t) + (yy - cy) * math.sin(t)).astype(int)
    tear = np.array([0, 1, 2, 1, 0, -1, 1, 0, -2, -1])[along % 10]
    side = (xx - cx) * math.sin(t) - (yy - cy) * math.cos(t) + tear > 0
    rng = np.random.default_rng(seed)
    step = max(2, round(span / 14))
    for k, (dark, embers) in enumerate(((0.75, 0.9), (0.5, 0.6), (0.28, 0.3))):
        d = step * (k + 1)
        out = np.zeros_like(frame)
        for piece, sx, sy in ((side, -d, -d + 1), (~side, d, d)):
            part = np.where((piece & a)[:, :, None], frame, 0)
            part[:, :, :3] *= dark
            moved = np.zeros_like(part)
            py, px = np.nonzero(part[:, :, 3])
            ny, nx = py + sy, px + sx
            ok = (ny > 0) & (ny < h - 1) & (nx > 0) & (nx < w - 1)
            moved[ny[ok], nx[ok]] = part[py[ok], px[ok]]
            on = moved[:, :, 3] > 0
            pad = np.pad(on, 1)
            rim = on & ~(pad[:-2, 1:-1] & pad[2:, 1:-1] & pad[1:-1, :-2] & pad[1:-1, 2:])
            hot = rim & (rng.random(rim.shape) < embers)
            moved[hot, :3] = FIRE[rng.integers(1, 4, hot.sum())]
            out = np.where(moved[:, :, 3:] > 0, moved, out)
        frames.append(out)
    return frames


def strip(base, spots, seed, fires=True):
    """Eight frames: three idle (lights pulsing), damaged, four destruction."""
    return [base, glow(base, 1.12), glow(base, 0.86), damaged(base, spots, fires)] + destruction(base, seed)


SHADOW_GREYS = [0, 0, 0] + [v for g in range(20, 231, 15) for v in (g, g, g)]


def shadow(frame, name):
    """The unit's silhouette in slot 2 of the shared grey shadow bank."""
    im = Image.fromarray(np.where(frame[:, :, 3] > 0, 2, 0).astype(np.uint8), 'P')
    im.putpalette(SHADOW_GREYS + [0] * 720)
    (ROOT / 'sprites').mkdir(parents=True, exist_ok=True)
    im.save(ROOT / 'sprites' / f'shadow_{name}.png', transparency=0)


def save(frame, rel):
    path = ROOT / rel
    path.parent.mkdir(parents=True, exist_ok=True)
    Image.fromarray(np.clip(frame, 0, 255).astype(np.uint8), 'RGBA').save(path)
    return str(rel)


def family(name, bank, banks, frames, pivot=None):
    members = {n: save(f, Path('sprites') / f'{n}.png') for n, f in frames.items()}
    fam = {'name': name, 'bank': bank, 'banks': banks, 'members': members}
    if pivot:
        fam['pivot'] = pivot
    return fam


def keep():
    hull = outline(fit(source('keep'), (112, 112), 110))
    turret = outline(fit(source('keep_turret'), (32, 32), 26))
    frames = {}
    for k, f in enumerate(strip(hull, ((0.3, 0.35, 0.06), (0.68, 0.55, 0.05), (0.45, 0.8, 0.04)), 5)):
        frames['red_fortress' if k == 0 else f'red_fortress_{k}'] = f
    for k, f in enumerate(strip(turret, ((0.55, 0.45, 0.1),), 6)):
        frames['native_keep_turret' if k == 0 else f'native_keep_turret_{k}'] = f
    shadow(hull, 'red_fortress')
    return family('crimson_keep', 56, 16, frames)


def warden():
    """The Ridge Warden miniboss: banks 76-77 (the drone keeps 75 alone)."""
    hull = outline(fit(source('warden'), (80, 64), 78))
    frames = {('miniboss_0' if k == 0 else f'miniboss_0_{k}'): f
              for k, f in enumerate(strip(hull, ((0.3, 0.45, 0.05), (0.7, 0.4, 0.05)), 7))}
    shadow(hull, 'miniboss_0')
    return family('ridge_warden', 76, 2, frames)


def interceptor():
    """The radar-directed interceptor: frames 1-7 follow the legacy drawing's ID."""
    jet = outline(fit(source('interceptor'), (48, 48), 44))
    frames = {('interceptor' if k == 0 else f'interceptor_{k}'): f
              for k, f in enumerate(strip(jet, ((0.7, 0.4, 0.08), (0.3, 0.55, 0.06)), 8, fires=False))}
    return family('interceptor', 84, 3, frames)


def clouds():
    """The distant cloud and the cloud bank passing over the player: banks 100-101."""
    return family('clouds', 100, 2, {'native_far_cloud': fit(source('cloud_far'), (96, 48), 94),
                                     'native_overhead_cloud': fit(source('cloud_over'), (96, 48), 94)})


def warships():
    """The sea stages' command targets: an AA cruiser and a radar picket, each a full
    strip on the bank of a legacy drawing the game never shows (119, 121)."""
    out = []
    for name, bank in (('naval_aa', 119), ('naval_radar', 121)):
        ship = outline(fit(source(name), (48, 80), 78))
        frames = {(name if k == 0 else f'{name}_{k}'): f
                  for k, f in enumerate(strip(ship, ((0.5, 0.35, 0.12), (0.45, 0.7, 0.1)), 9))}
        out.append(family(name, bank, 1, frames))
    return out


def title():
    """The title screen: the painting scaled to the 320-pixel width and cut to 224
    rows under its logo, on the sixteen banks the title owns (16-31)."""
    with Image.open(SOURCES / 'title_screen.png') as im:
        im = im.convert('RGB')
        im = im.resize((320, round(im.height * 320 / im.width)), Image.Resampling.LANCZOS)
    top = min(6, im.height - 224)
    picture = np.dstack([np.asarray(im)[top:top + 224], np.full((224, 320), 255, np.uint8)])
    fam = family('title', 16, 16, {'title': picture.astype(np.float32)})
    fam['full'] = True
    return fam


def minibosses():
    """The other two minibosses, on the banks the cockpit console used to hold
    (124-126; the HUD no longer draws it)."""
    out = []
    for name, bank, banks in (('miniboss_1', 124, 2), ('miniboss_2', 126, 1)):
        if not (SOURCES / f'{name}.png').exists():
            continue
        hull = outline(fit(source(name), (80, 64), 78))
        frames = {(name if k == 0 else f'{name}_{k}'): f
                  for k, f in enumerate(strip(hull, ((0.3, 0.45, 0.05), (0.7, 0.4, 0.05)), 11))}
        out.append(family(name, bank, banks, frames))
    return out


def explosions():
    """Eight frames at 64x64 (flash, shock ring, fireball, break-up, smoke),
    one scale for all, on banks 98-99: fire and smoke each get a palette."""
    paintings = [source(f'explosion_{k}') for k in range(8)
                 if (SOURCES / f'explosion_{k}.png').exists()]
    largest = max(max(p.shape[:2]) for p in paintings)
    frames = {}
    for k, p in enumerate(paintings):
        frames[f'explosion_{k}'] = fit(p, (64, 64), 62 * max(p.shape[:2]) / largest)
    return family('explosions', 98, 2, frames, [32, 32])


def ships():
    return [family('missile_boat', 87, 3, {'missile_boat': outline(fit(source('boat'), (32, 64), 62))}),
            family('submarine', 120, 1, {'submarine': outline(fit(source('sub'), (32, 64), 62))})]


# Stage bosses after the first: one 112x112 drawing each, all on banks 56-71
# (only one boss is ever on screen).
BOSSES = {'battleship': 'battleship', 'gold_core': 'boss_gold_core', 'stealth': 'boss_stealth',
          'heli_carrier': 'boss_heli_carrier', 'tank_fort': 'boss_tank_fort',
          'castle': 'boss_castle'}


def bosses():
    return [family(name, 56, 16, {name: outline(fit(source(src), (112, 112), 110))})
            for name, src in BOSSES.items() if (SOURCES / f'{src}.png').exists()]


def title_cards():
    """The pilots' cards on the title screens, drawn by pilot_cards.py: all
    three show at once on six banks; those are the bosses' 56-61, as no
    boss is ever on a title screen (each scene loads its own palettes)."""
    return family('title_cards', 56, 6, pilot_cards.frames())


def cockpit_instruments():
    """The HUD column's attitude indicator, radar scope and G meter, drawn by
    instruments.py: banks 95-97."""
    return family('instruments', 95, 3, instruments.frames())


BUILDERS = {'crimson_keep': (keep, ('keep', 'keep_turret')),
            'explosions': (explosions, tuple(f'explosion_{k}' for k in range(6))),
            'ships': (ships, ('boat', 'sub')),
            'ridge_warden': (warden, ('warden',)),
            'interceptor': (interceptor, ('interceptor',)),
            'clouds': (clouds, ('cloud_far', 'cloud_over')),
            'warships': (warships, ('naval_aa', 'naval_radar')),
            'title': (title, ('title_screen',)),
            'minibosses': (minibosses, ()),
            'bosses': (bosses, ()),
            'instruments': (cockpit_instruments, ()),
            'title_cards': (title_cards, ())}


def run():
    manifest = ROOT / 'painted.json'
    current = json.loads(manifest.read_text()) if manifest.exists() else []
    built = []
    for key, (builder, needs) in BUILDERS.items():
        if all((SOURCES / f'{n}.png').exists() for n in needs):
            out = builder()
            built += out if isinstance(out, list) else [out]
    names = {f['name'] for f in built}
    keep_rest = [f for f in current if f['name'] not in names]
    manifest.write_text(json.dumps(keep_rest + built, indent=1) + '\n')
    print('Painted sprites:', ', '.join(sorted(names)) or 'none')


if __name__ == '__main__':
    run()
