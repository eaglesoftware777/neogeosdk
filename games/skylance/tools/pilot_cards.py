"""The three pilots' cards for the title screens: each portrait reduced to
half size from its 160x240 drawing (a clean reduction, not the hardware's
shrink), in a bevelled frame over its squadron colour; the faces only, no
names. 48x64 frames; title_card_0..2 follow the pilot order.
"""
from pathlib import Path

import numpy as np
from PIL import Image

PORTRAITS = Path(__file__).resolve().parents[1] / 'artbox/in/characters'
CARDS = (('sprite_p1_pilot.png', (18, 46, 120), (70, 130, 230)),
         ('sprite_p2_pilot.png', (18, 64, 34), (70, 160, 90)),
         ('sprite_p3_pilot.png', (90, 20, 18), (220, 80, 60)))
W, H = 48, 64
CARD = (1, 1, 46, 52)                    # x, y, width, height
WINDOW = (5, 5, 38, 44)


def ramp(rows, top, bottom):
    t = np.linspace(0, 1, rows)[:, None]
    return np.array(top, np.float32) * (1 - t) + np.array(bottom, np.float32) * t


def portrait(name):
    with Image.open(PORTRAITS / name) as im:
        im = im.convert('RGBA')
        w = WINDOW[2]
        small = im.resize((w, round(im.height * w / im.width)), Image.Resampling.LANCZOS)
    a = np.asarray(small).astype(np.float32)
    # the drawing's magenta matte bleeds into the hair's edge: drop it
    r, g, b = a[:, :, 0], a[:, :, 1], a[:, :, 2]
    a[(r - g > 40) & (b - g > 40), 3] = 0
    return a[:WINDOW[3]]


def card(k):
    source, dark, light = CARDS[k]
    out = np.zeros((H, W, 4), np.float32)
    x0, y0, cw, ch = CARD
    # frame: dark outline, light bevel top-left, shade bottom-right
    out[y0:y0 + ch, x0:x0 + cw, :3] = (16, 22, 38)
    out[y0 + 1:y0 + ch - 1, x0 + 1:x0 + cw - 1, :3] = (196, 206, 224)
    out[y0 + 2:y0 + ch - 1, x0 + 2:x0 + cw - 1, :3] = (84, 94, 116)
    out[y0 + 2:y0 + ch - 2, x0 + 2:x0 + cw - 2, :3] = (138, 150, 172)
    out[y0:y0 + ch, x0:x0 + cw, 3] = 255
    # the window: squadron colour, lighter at the top, then the portrait
    wx, wy, ww, wh = WINDOW
    out[wy - 1:wy + wh + 1, wx - 1:wx + ww + 1, :3] = (16, 22, 38)
    out[wy:wy + wh, wx:wx + ww, :3] = ramp(wh, light, dark)[:, None, :]
    face = portrait(source)
    alpha = (face[:, :, 3:4] >= 128).astype(np.float32)
    region = out[wy:wy + wh, wx:wx + ww, :3]
    out[wy:wy + wh, wx:wx + ww, :3] = region * (1 - alpha) + face[:, :, :3] * alpha
    return np.clip(out, 0, 255).astype(np.uint8)


def frames():
    return {f'title_card_{k}': card(k) for k in range(3)}


if __name__ == '__main__':
    import sys
    sheet = Image.new('RGBA', (3 * 52, 68), (60, 90, 140, 255))
    for k, rgba in enumerate(frames().values()):
        sheet.alpha_composite(Image.fromarray(rgba, 'RGBA'), (k * 52 + 2, 2))
    sheet.resize((sheet.width * 4, sheet.height * 4), Image.Resampling.NEAREST).save(sys.argv[1])
