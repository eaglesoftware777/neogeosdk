"""The cockpit instruments in the HUD column right of the field, under the
pilot's portrait: an attitude indicator (seven bank angles), a radar scope
(eight sweep steps, calm green and alert red) and a G meter (four loads,
afterburner lamp off and on).

Each is a 32x32 frame: a gunmetal housing with corner screws round a glass
dial. They are drawn here at four times the size and reduced, so the dials
keep smooth rings and needles; the housing's outline stays hard-edged.
"""
import math

import numpy as np
from PIL import Image, ImageDraw

N, S = 32, 4                     # frame size, supersampling
C = N * S / 2                    # centre in the large drawing
DIAL = 12.5 * S                  # dial radius


def housing_mask():
    """Rounded 30x30 square, one transparent pixel round it."""
    im = Image.new('L', (N, N), 0)
    ImageDraw.Draw(im).rounded_rectangle((1, 1, N - 2, N - 2), radius=5, fill=255)
    return np.asarray(im) > 0


def housing():
    """The large housing: gunmetal, lit from the upper left, four screws."""
    im = Image.new('RGB', (N * S, N * S), (0, 0, 0))
    d = ImageDraw.Draw(im)
    box = (S, S, N * S - S - 1, N * S - S - 1)
    d.rounded_rectangle(box, radius=5 * S, fill=(22, 26, 32))
    d.rounded_rectangle((box[0] + S, box[1] + S, box[2] - S, box[3] - S), radius=4 * S, fill=(62, 70, 82))
    # top-left bevel light, bottom-right shade
    d.arc((box[0] + S, box[1] + S, box[2] - S, box[3] - S), 180, 270, fill=(128, 140, 156), width=S)
    d.line((box[0] + 5 * S, box[1] + S + 1, box[2] - 5 * S, box[1] + S + 1), fill=(118, 130, 146), width=S)
    d.line((box[0] + S + 1, box[1] + 5 * S, box[0] + S + 1, box[3] - 5 * S), fill=(104, 116, 132), width=S)
    d.line((box[0] + 5 * S, box[3] - S - 1, box[2] - 5 * S, box[3] - S - 1), fill=(34, 40, 48), width=S)
    d.line((box[2] - S - 1, box[1] + 5 * S, box[2] - S - 1, box[3] - 5 * S), fill=(34, 40, 48), width=S)
    for sx in (box[0] + 3.2 * S, box[2] - 3.2 * S):
        for sy in (box[1] + 3.2 * S, box[3] - 3.2 * S):
            d.ellipse((sx - 1.3 * S, sy - 1.3 * S, sx + 1.3 * S, sy + 1.3 * S), fill=(156, 164, 176))
            d.line((sx - S, sy + S * 0.6, sx + S, sy - S * 0.6), fill=(40, 46, 56), width=2)
    # the dial's black bezel ring
    r = DIAL + 1.3 * S
    d.ellipse((C - r, C - r, C + r, C + r), fill=(8, 10, 14))
    return im


def coords():
    y, x = np.mgrid[0:N * S, 0:N * S].astype(np.float32) + 0.5
    return x - C, y - C


def glass(rgb):
    """A soft glint across the dial's upper left."""
    x, y = coords()
    r = np.hypot(x, y)
    band = (r < DIAL) & (np.hypot(x + 0.35 * DIAL, y + 0.35 * DIAL) < 0.62 * DIAL) & \
           (np.hypot(x + 0.15 * DIAL, y + 0.15 * DIAL) > 0.55 * DIAL)
    out = rgb.astype(np.float32)
    out[band] = out[band] * 0.72 + 255 * 0.28
    return out


def finish(big):
    """Reduce the large drawing to the frame; outside the housing is clear."""
    small = np.asarray(Image.fromarray(np.clip(big, 0, 255).astype(np.uint8)).resize((N, N), Image.Resampling.BOX))
    out = np.zeros((N, N, 4), np.uint8)
    mask = housing_mask()
    out[mask, :3] = small[mask]
    out[mask, 3] = 255
    return out


def ramp(t, a, b):
    t = np.clip(t, 0, 1)[..., None]
    return np.array(a, np.float32) * (1 - t) + np.array(b, np.float32) * t


def attitude(bank):
    """Bank -3 (hard left) .. 3 (hard right), 15 degrees a step. The world
    turns against the roll: banking left, the horizon's left end rises."""
    big = np.asarray(housing()).astype(np.float32)
    x, y = coords()
    r = np.hypot(x, y)
    a = math.radians(15 * bank)
    # distance below the horizon, in the turned frame
    v = x * math.sin(a) + y * math.cos(a)
    u = x * math.cos(a) - y * math.sin(a)
    sky = ramp(-v / DIAL, (110, 186, 250), (28, 92, 196))
    ground = ramp(v / DIAL, (176, 112, 56), (92, 54, 28))
    disc = np.where((v < 0)[..., None], sky, ground)
    inside = r < DIAL
    big[inside] = disc[inside]
    white = np.array((240, 244, 250), np.float32)
    line = inside & (np.abs(v) < 0.55 * S)
    big[line] = white
    for k, half in ((-1, 3.2), (1, 3.2), (-2, 4.8), (2, 4.8)):
        rung = inside & (np.abs(v - k * 3.4 * S) < 0.35 * S) & (np.abs(u) < half * S)
        big[rung] = white
    # bank scale: fixed ticks on the bezel, pointer turning with the horizon
    im = Image.fromarray(np.clip(big, 0, 255).astype(np.uint8))
    d = ImageDraw.Draw(im)
    for deg in (-60, -45, -30, -15, 0, 15, 30, 45, 60):
        t = math.radians(deg - 90)
        r0, r1 = DIAL - (2.6 if deg % 30 == 0 else 1.6) * S, DIAL
        d.line((C + r0 * math.cos(t), C + r0 * math.sin(t), C + r1 * math.cos(t), C + r1 * math.sin(t)),
               fill=(250, 250, 250), width=S)
    t = math.radians(-90 - 15 * bank)
    tip = (C + (DIAL - 3.0 * S) * math.cos(t), C + (DIAL - 3.0 * S) * math.sin(t))
    side = t + math.pi / 2
    base = (C + (DIAL - 5.6 * S) * math.cos(t), C + (DIAL - 5.6 * S) * math.sin(t))
    d.polygon((tip, (base[0] + 1.6 * S * math.cos(side), base[1] + 1.6 * S * math.sin(side)),
               (base[0] - 1.6 * S * math.cos(side), base[1] - 1.6 * S * math.sin(side))), fill=(255, 210, 60))
    # the fixed aircraft symbol
    amber, rim = (255, 176, 32), (20, 16, 10)
    for colour, w in ((rim, 2.6 * S), (amber, 1.3 * S)):
        d.line((C - 8 * S, C, C - 3.5 * S, C, C - 2 * S, C + 2 * S, C, C), fill=colour, width=int(w), joint='curve')
        d.line((C + 8 * S, C, C + 3.5 * S, C, C + 2 * S, C + 2 * S, C, C), fill=colour, width=int(w), joint='curve')
    d.ellipse((C - 1.2 * S, C - 1.2 * S, C + 1.2 * S, C + 1.2 * S), fill=amber, outline=rim, width=2)
    return finish(glass(np.asarray(im)))


SCOPE = {False: dict(bg=(4, 22, 12), ring=(26, 86, 46), sweep=(150, 255, 170), trail=(48, 190, 92),
                     blip=(210, 255, 190)),
         True: dict(bg=(30, 6, 4), ring=(110, 34, 24), sweep=(255, 150, 110), trail=(220, 64, 40),
                    blip=(255, 230, 150))}
BLIPS = ((-0.42, -0.38), (0.5, -0.12), (0.12, 0.52))


def radar(step, alert=False):
    """Sweep step 0..7 (45 degrees each, clockwise from north)."""
    p = SCOPE[alert]
    big = np.asarray(housing()).astype(np.float32)
    x, y = coords()
    r = np.hypot(x, y)
    inside = r < DIAL
    sweep = math.radians(step * 45)
    ang = np.arctan2(x, -y) % (2 * math.pi)                 # clockwise from north
    behind = (sweep - ang) % (2 * math.pi)                  # how long ago the beam passed
    big[inside] = p['bg']
    trail = inside & (behind < math.radians(80))
    t = (behind / math.radians(80))[trail]
    big[trail] = ramp(t, p['trail'], p['bg'])
    for rr in (DIAL / 3, 2 * DIAL / 3):
        big[inside & (np.abs(r - rr) < 0.4 * S)] = p['ring']
    big[inside & ((np.abs(x) < 0.3 * S) | (np.abs(y) < 0.3 * S))] = p['ring']
    for bx, by in BLIPS:
        cx, cy = bx * DIAL, by * DIAL
        since = (sweep - math.atan2(cx, -cy) % (2 * math.pi)) % (2 * math.pi)
        fade = min(1.0, since / math.radians(270))
        colour = ramp(np.float32(fade), p['blip'], p['ring'])
        big[np.hypot(x - cx, y - cy) < 1.15 * S] = colour
    im = Image.fromarray(np.clip(big, 0, 255).astype(np.uint8))
    d = ImageDraw.Draw(im)
    d.line((C, C, C + DIAL * math.sin(sweep), C - DIAL * math.cos(sweep)), fill=p['sweep'], width=S)
    d.ellipse((C - 0.9 * S, C - 0.9 * S, C + 0.9 * S, C + 0.9 * S), fill=p['sweep'])
    return finish(glass(np.asarray(im)))


def gmeter(load, burner):
    """Load 0..3 (cruise, banking, afterburner, roll or loop), lamp off/on."""
    big = np.asarray(housing()).astype(np.float32)
    x, y = coords()
    r = np.hypot(x, y)
    inside = r < DIAL
    big[inside] = (14, 16, 20)
    ang = (np.degrees(np.arctan2(x, -y)) + 360) % 360          # clockwise from north
    sweep = (ang - 225) % 360                                  # 0 at lower left .. 270 lower right
    band = inside & (r > DIAL - 2.2 * S) & (r < DIAL - 0.6 * S) & (sweep <= 270)
    for lo, hi, colour in ((0, 160, (60, 200, 90)), (160, 220, (250, 200, 50)), (220, 270, (240, 64, 48))):
        big[band & (sweep >= lo) & (sweep <= hi)] = colour
    im = Image.fromarray(np.clip(big, 0, 255).astype(np.uint8))
    d = ImageDraw.Draw(im)
    for k in range(10):
        t = math.radians(225 + 30 * k - 90)
        r0 = DIAL - (4.2 if k % 3 == 0 else 3.2) * S
        d.line((C + r0 * math.cos(t), C + r0 * math.sin(t),
                C + (DIAL - 2.4 * S) * math.cos(t), C + (DIAL - 2.4 * S) * math.sin(t)),
               fill=(232, 236, 242), width=max(2, S // 2 + (k % 3 == 0) * 2))
    # afterburner lamp
    lamp = (255, 150, 40) if burner else (70, 44, 24)
    d.rounded_rectangle((C - 3.2 * S, C + 4.6 * S, C + 3.2 * S, C + 7.4 * S), radius=S, fill=lamp,
                        outline=(10, 10, 12), width=2)
    if burner:
        d.rounded_rectangle((C - 2.2 * S, C + 5.2 * S, C + 2.2 * S, C + 6.2 * S), radius=S, fill=(255, 236, 170))
    # the needle: 1, 3, 6 and 9 G on a 0-10 dial
    g = (1.0, 3.0, 6.0, 9.0)[load]
    t = math.radians(225 + 27 * g - 90)
    tip = (C + (DIAL - 2.0 * S) * math.cos(t), C + (DIAL - 2.0 * S) * math.sin(t))
    tail = (C - 2.4 * S * math.cos(t), C - 2.4 * S * math.sin(t))
    d.line((tail, tip), fill=(16, 12, 8), width=int(1.9 * S))
    d.line((tail, tip), fill=(255, 120, 40) if load == 3 else (250, 250, 250), width=int(0.9 * S))
    d.ellipse((C - 1.6 * S, C - 1.6 * S, C + 1.6 * S, C + 1.6 * S), fill=(150, 156, 166), outline=(16, 12, 8), width=2)
    return finish(glass(np.asarray(im)))


def frames():
    """{name: RGBA}: attitude_ind_0..6, radar_scope_0..15 (8.. alert), g_meter_0..7 (load*2+lamp)."""
    out = {}
    for k in range(7):
        out[f'attitude_ind_{k}'] = attitude(k - 3)
    for k in range(16):
        out[f'radar_scope_{k}'] = radar(k % 8, k >= 8)
    for k in range(8):
        out[f'g_meter_{k}'] = gmeter(k // 2, k % 2)
    return out


if __name__ == '__main__':
    import sys
    f = frames()
    sheet = Image.new('RGBA', (34 * 8, 34 * 4), (40, 70, 50, 255))
    for i, (name, rgba) in enumerate(f.items()):
        sheet.alpha_composite(Image.fromarray(rgba, 'RGBA'), ((i % 8) * 34, (i // 8) * 34))
    sheet.resize((sheet.width * 4, sheet.height * 4), Image.Resampling.NEAREST).save(sys.argv[1] if len(sys.argv) > 1 else 'instruments.png')
