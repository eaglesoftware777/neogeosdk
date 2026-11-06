"""Stage 1 unit strips from their base drawings.

Each unit in assets/authored/units/<name>.png is an indexed drawing facing
down the screen: slot 0 transparent, 1-12 its own colours from darkest (the
outline) to lightest, 13-15 fire (hot, orange, red). From it this builds the
eight frames the game plays -- three idle, one damaged, four destruction --
and writes them to units/<name>.json for author_stage_one.lua.
"""
import json
import math
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

ROOT = Path(__file__).resolve().parents[1] / 'assets/authored'
UNITS = ROOT / 'units'
HOT, MID, LOW = 13, 14, 15
BODY = 12

# canvas, palette bank, bank copies, airborne, idle effect
SPEC = {
    'drone':      ((32, 32), 75, 1, True, 'eye'),
    'fighter':    ((48, 48), 78, 3, True, 'nozzle'),
    'heli':       ((48, 48), 81, 3, True, 'rotor'),
    'tank':       ((32, 48), 90, 3, False, 'tracks'),
    'bomber':     ((64, 64), 72, 3, True, 'glow'),
    'sam':        ((32, 48), 118, 1, False, 'beacon'),
    'radar_site': ((32, 48), 122, 1, False, 'dish'),
}
HELI_HUB = (9, 26)       # rotor mast in the base drawing
HELI_BLADE = 19
FIGHTER_NOZZLE = ((15, 3), (16, 3), (17, 3), (15, 4), (16, 4), (17, 4))


def base(name):
    with Image.open(UNITS / f'{name}.png') as im:
        if im.mode != 'P' or im.info.get('transparency') != 0:
            raise ValueError(f'{name}: indexed drawing with transparent slot 0 required')
        pal = np.array(im.getpalette()[:48], np.uint8).reshape(16, 3)
        return np.array(im), pal


def place(idx, canvas):
    w, h = canvas
    out = np.zeros((h, w), np.uint8)
    y0, x0 = (h - idx.shape[0]) // 2, (w - idx.shape[1]) // 2
    if x0 < 1 or y0 < 1:
        raise ValueError(f'{idx.shape[::-1]} does not fit {canvas} inside a 1-pixel margin')
    out[y0:y0 + idx.shape[0], x0:x0 + idx.shape[1]] = idx
    return out, (x0, y0)


PAL = None


def ramps(pal):
    """Body colours grouped by hue (greys apart), each family dark to light."""
    def lum(i):
        return float(pal[i] @ np.array([0.3, 0.59, 0.11]))
    family = {}
    for i in range(2, BODY + 1):
        r, g, b = pal[i] / 255.0
        hi, lo = max(r, g, b), min(r, g, b)
        if hi < 0.1 or (hi - lo) / hi < 0.22:
            key = 'grey'
        else:
            hue = (60 * ((g - b) / (hi - lo)) if hi == r else
                   60 * (2 + (b - r) / (hi - lo)) if hi == g else 60 * (4 + (r - g) / (hi - lo))) % 360
            key = int(((hue + 30) % 360) // 60)
        family.setdefault(key, []).append(i)
    by_lum = sorted(range(2, BODY + 1), key=lum)
    return {i: sorted(f, key=lum) for f in family.values() for i in f}, by_lum


def shade(idx, ranks, burnt=False):
    """Lighten or darken body colours by whole steps: along each colour's own
    hue family, or (burnt) along all body colours, so a hull chars toward
    black. The outline and fire stay."""
    family, by_lum = ramps(PAL)
    table = np.arange(16)
    for i in range(2, BODY + 1):
        ramp = by_lum if burnt else family[i]
        table[i] = ramp[int(np.clip(ramp.index(i) + ranks, 0, len(ramp) - 1))]
    return table[idx].astype(np.uint8)


def outline(idx):
    on = idx > 0
    pad = np.pad(on, 1)
    edge = on & ~(pad[:-2, 1:-1] & pad[2:, 1:-1] & pad[1:-1, :-2] & pad[1:-1, 2:])
    out = idx.copy()
    out[edge & (idx <= BODY)] = 1
    return out


def inner(idx):
    """Clear the canvas border; the hardware strip is cut there."""
    out = idx.copy()
    out[0, :] = out[-1, :] = out[:, 0] = out[:, -1] = 0
    return out


def burn(idx, cx, cy, r, core=True):
    """Fire over the drawn hull around (cx, cy): hot core, orange, red rim."""
    out = idx.copy()
    h, w = idx.shape
    for y in range(max(0, int(cy - r - 1)), min(h, int(cy + r + 2))):
        for x in range(max(0, int(cx - r - 1)), min(w, int(cx + r + 2))):
            if not idx[y, x]:
                continue
            d = math.hypot(x - cx, y - cy)
            if d <= r * 0.35 and core:
                out[y, x] = HOT
            elif d <= r * 0.7:
                out[y, x] = MID
            elif d <= r:
                out[y, x] = LOW
    return out


def scorch(idx, cx, cy, r, ranks=-4):
    yy, xx = np.mgrid[:idx.shape[0], :idx.shape[1]]
    near = np.hypot(xx - cx, yy - cy) <= r
    return np.where(near, shade(idx, ranks, burnt=True), idx).astype(np.uint8)


def glow_pixels(frame, pal):
    """Exhaust flames: orange, yellow or white-hot colours in the tail half (the top)."""
    hot = {i for i in range(2, BODY + 1)
           if (pal[i, 0] > 180 and pal[i, 1] > 110 and pal[i, 2] < 110) or pal[i].min() > 205}
    ys, xs = np.nonzero(np.isin(frame, list(hot)))
    keep = ys < frame.shape[0] * 0.5
    return list(zip(ys[keep], xs[keep]))


def flicker(frame, pts, colours):
    frames = [frame]
    for step in colours:
        f = frame.copy()
        for k, (y, x) in enumerate(pts):
            f[y, x] = step[k % len(step)]
        frames.append(f)
    return frames


def eye_pixels(frame):
    """The two lightest colours in the nose half (the bottom): a sensor eye."""
    ys, xs = np.nonzero(np.isin(frame, (BODY - 1, BODY)))
    keep = ys >= frame.shape[0] * 0.5
    return list(zip(ys[keep], xs[keep]))


def idle_rotor(frame, hub):
    frames = []
    hx, hy = hub
    for k, angle in enumerate((20, 50, 80)):
        f = frame.copy()
        for blade in (angle, angle + 90):
            a = math.radians(blade)
            dx, dy = math.cos(a), math.sin(a)
            for t in np.arange(-HELI_BLADE, HELI_BLADE + 0.5, 0.5):
                x, y = int(round(hx + dx * t)), int(round(hy + dy * t))
                # the trailing edge one pixel behind, darker
                bx, by = int(round(hx + dx * t - dy)), int(round(hy + dy * t + dx))
                tip = abs(t) > HELI_BLADE - 3
                if 0 < by < f.shape[0] - 1 and 0 < bx < f.shape[1] - 1 and abs(t) > 2:
                    f[by, bx] = 2
                if 0 < y < f.shape[0] - 1 and 0 < x < f.shape[1] - 1:
                    f[y, x] = 9 if tip else 5
        f[hy - 1:hy + 2, hx - 1:hx + 2] = 1
        f[hy, hx] = 8
        frames.append(f)
    return frames


def idle_tracks(frame):
    """Roll the tread pattern down the two track columns, a pixel a frame."""
    frames = [frame]
    ys, xs = np.nonzero(frame)
    left, right = xs.min(), xs.max()
    for step in (1, 2):
        f = frame.copy()
        for x in list(range(left + 1, left + 4)) + list(range(right - 3, right)):
            col = np.nonzero(frame[:, x])[0]
            if len(col) < 6:
                continue
            run = frame[col[1]:col[-1], x]
            f[col[1]:col[-1], x] = np.roll(run, step)
        frames.append(f)
    return frames


def idle_beacon(frame):
    ys, xs = np.nonzero(frame)
    x, y = (xs.min() + xs.max()) // 2, ys.max() - 6
    frames = [frame]
    for colour in (LOW, HOT):
        f = frame.copy()
        f[y, x] = colour
        frames.append(f)
    return frames


def idle_dish(frame, pal):
    """The dish turns: its outline narrows to an edge-on ellipse and back."""
    h, w = frame.shape
    grey = {i for i in range(2, BODY + 1) if pal[i].max() - pal[i].min() < 22 and pal[i].mean() > 110}
    top = np.zeros_like(frame, bool)
    top[:h // 2] = True
    dish = np.isin(frame, list(grey)) & top
    lab, n = ndi.label(dish)
    if n:
        sizes = ndi.sum(dish, lab, range(1, n + 1))
        dish = lab == (int(np.argmax(sizes)) + 1)
    dish = ndi.binary_dilation(dish, iterations=1) & (frame > 0) & top
    ys, xs = np.nonzero(dish)
    cx = (xs.min() + xs.max()) / 2
    deck = int(np.bincount(frame[(frame > 1) & ~dish & (frame <= BODY)]).argmax())
    frames = [frame]
    for squeeze in (0.6, 0.25):
        f = frame.copy()
        f[dish] = deck
        for y, x in zip(ys, xs):
            nx = int(round(cx + (x - cx) * squeeze))
            f[y, nx] = frame[y, x]
        frames.append(outline(f))
    return frames


def split(frame, angle):
    """Two pieces either side of a line through the hull's centre."""
    ys, xs = np.nonzero(frame)
    cy, cx = ys.mean(), xs.mean()
    yy, xx = np.mgrid[:frame.shape[0], :frame.shape[1]]
    a = math.radians(angle)
    # a torn edge, not a saw cut: the line wanders a pixel or two either way
    along = np.round((xx - cx) * math.cos(a) + (yy - cy) * math.sin(a)).astype(int)
    tear = np.array([0, 1, 2, 1, 0, -1, 1, 0, -2, -1])[along % 10]
    side = (xx - cx) * math.sin(a) - (yy - cy) * math.cos(a) + tear > 0
    return np.where(side, frame, 0), np.where(~side, frame, 0), (cx, cy)


def shift(idx, dx, dy):
    out = np.zeros_like(idx)
    h, w = idx.shape
    ys, xs = np.nonzero(idx)
    ny, nx = ys + dy, xs + dx
    ok = (ny >= 1) & (ny < h - 1) & (nx >= 1) & (nx < w - 1)
    out[ny[ok], nx[ok]] = idx[ys[ok], xs[ok]]
    return out


def cut_fire(piece, other_side_mask, colours):
    """Paint the freshly broken edge of a piece with fire."""
    on = piece > 0
    edge = on & ndi.binary_dilation(other_side_mask, iterations=1)
    out = piece.copy()
    ys, xs = np.nonzero(edge)
    for k, (y, x) in enumerate(zip(ys, xs)):
        out[y, x] = colours[k % len(colours)]
    return out


def merge(*layers):
    out = np.zeros_like(layers[0])
    for layer in layers:
        out = np.where(layer > 0, layer, out)
    return out


def destruction_air(frame):
    ys, xs = np.nonzero(frame)
    cy, cx = ys.mean(), xs.mean()
    span = min(xs.max() - xs.min(), ys.max() - ys.min())
    flash = burn(shade(frame, 3), cx, cy, max(3, span * 0.22))
    a, b, _ = split(frame, -32)
    frames = [flash]
    for (ax, ay, bx, by), darker, colours, erode in (
            ((-2, -1, 2, 2), -2, (HOT, MID), 0),
            ((-4, -3, 4, 4), -5, (MID, LOW), 0),
            ((-6, -5, 6, 7), -9, (LOW, 2, 2), 1)):
        pa, pb = shade(a, darker, burnt=True), shade(b, darker, burnt=True)
        if erode:
            pa = np.where(ndi.binary_erosion(pa > 0, iterations=erode), pa, 0)
            pb = np.where(ndi.binary_erosion(pb > 0, iterations=erode), pb, 0)
        pa = cut_fire(outline(pa), b > 0, colours)
        pb = cut_fire(outline(pb), a > 0, colours)
        frames.append(merge(shift(pa, ax, ay), shift(pb, bx, by)))
    # embers left drifting in the last frame
    last = frames[-1]
    for k, (dx, dy) in enumerate(((-9, 4), (8, -6), (2, 10), (-3, -10))):
        y, x = int(cy + dy), int(cx + dx)
        if 1 <= y < last.shape[0] - 1 and 1 <= x < last.shape[1] - 1:
            last[y, x] = (LOW, MID, 2, LOW)[k]
    return frames


def destruction_ground(frame):
    ys, xs = np.nonzero(frame)
    cy, cx = ys.mean(), xs.mean()
    w, h = xs.max() - xs.min(), ys.max() - ys.min()
    spots = ((cx, cy), (cx - w * 0.22, cy + h * 0.2), (cx + w * 0.2, cy - h * 0.22))
    frames = [burn(shade(frame, 3), cx, cy, max(3, w * 0.3))]
    for darker, radius, core in ((-3, 0.22, True), (-6, 0.26, True), (-10, 0.14, False)):
        f = shade(frame, darker, burnt=True)
        for k, (x, y) in enumerate(spots):
            f = burn(f, x, y, max(2, w * radius * (1 if k == 0 else 0.8)), core)
        frames.append(outline(f))
    return frames


def damaged(frame):
    ys, xs = np.nonzero(frame)
    cy, cx = ys.mean(), xs.mean()
    w, h = xs.max() - xs.min(), ys.max() - ys.min()
    f = scorch(frame, cx + w * 0.2, cy - h * 0.1, max(2, w * 0.14))
    f = scorch(f, cx - w * 0.22, cy + h * 0.16, max(2, w * 0.09), -3)
    # a charred core, not a fire: a burning spot on a unit this small is an
    # enemy orb's size and colour, and it read as a shot stuck to the wing
    return scorch(f, cx + w * 0.2, cy - h * 0.1, max(1.5, w * 0.08), -8)


def build(name):
    canvas, bank, banks, air, idle = SPEC[name]
    drawing, pal = base(name)
    frame, (x0, y0) = place(drawing, canvas)
    global PAL
    PAL = pal
    if idle in ('glow', 'eye', 'nozzle'):
        pts = (glow_pixels(frame, pal) if idle == 'glow' else eye_pixels(frame) if idle == 'eye' else
               [(y + y0, x + x0) for x, y in FIGHTER_NOZZLE])
        if not pts:
            raise ValueError(f'{name}: nothing to glow')
        frames = flicker(frame, pts, ((LOW,), (MID, HOT)) if idle == 'eye' else ((MID, HOT), (HOT, LOW, MID)))
    elif idle == 'rotor':
        frames = idle_rotor(frame, (HELI_HUB[0] + x0, HELI_HUB[1] + y0))
    elif idle == 'tracks':
        frames = idle_tracks(frame)
    elif idle == 'beacon':
        frames = idle_beacon(frame)
    else:
        frames = idle_dish(frame, pal)
    frames.append(damaged(frames[0]))
    frames += destruction_air(frames[0]) if air else destruction_ground(frames[0])
    frames = [inner(f) for f in frames]
    for k, f in enumerate(frames):
        if not f.any():
            raise ValueError(f'{name}: frame {k} is empty')
    doc = dict(name=name, width=canvas[0], height=canvas[1], bank=bank, banks=banks,
               pivot=[canvas[0] // 2, canvas[1] // 2], palette=pal.astype(int).tolist(),
               frames=[[''.join('%X' % v for v in row) for row in f] for f in frames])
    (UNITS / f'{name}.json').write_text(json.dumps(doc, separators=(',', ':')) + '\n')
    return frames, pal


def sheet(path, scale=4):
    rows = [build(n) for n in SPEC]
    cw = max(f.shape[1] for fr, _ in rows for f in fr) + 4
    ch = [max(f.shape[0] for f in fr) + 4 for fr, _ in rows]
    out = Image.new('RGB', (cw * 8 * scale, sum(ch) * scale), (92, 98, 84))
    y = 0
    for (frames, pal), h in zip(rows, ch):
        for k, f in enumerate(frames):
            rgba = np.dstack([pal[f], (f > 0) * 255]).astype(np.uint8)
            im = Image.fromarray(rgba, 'RGBA').resize((f.shape[1] * scale, f.shape[0] * scale), Image.NEAREST)
            out.paste(im, (k * cw * scale, y * scale), im)
        y += h
    out.save(path)


def run():
    for name in SPEC:
        build(name)
    print(f'Unit strips: {len(SPEC)} units, 8 frames each')


if __name__ == '__main__':
    import sys
    if len(sys.argv) > 1:
        sheet(sys.argv[1])
    else:
        run()
