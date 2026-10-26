#!/usr/bin/env python3
"""
Seven Wonders Puzzles art: drawn here, packed with tools/ng_artpack.py.

    python3 games/sevenwonders/tools/art.py            (the build's GAME_ART_BUILDER)
    python3 games/sevenwonders/tools/art.py --fix-only (S1 only)

Every sprite is drawn by this script (shapes supersampled eight times, then
reduced), so the whole set is original and reproducible. The look is a cute
Japanese one: round puffball cats with big shining eyes and blushing
cheeks, candy pieces with little faces, a pink gingham board and soft
frames with polka-dot rims. Painted sources placed in artbox/painted/<name>.png
(RGBA, the same size) replace the drawn ones of the same name: the scenes,
the title painting and the logo (tools/logo.py renders it).

Pieces have a shape per colour as well as a hue (gem, triangle, star, clover,
drop, moon, heart, flower), so colour-blind players can tell them apart.

Picture logic draws its grid one cell a sprite: lcell (16 x 16) and lbig
(32 x 32) hold four marks (empty, filled, crossed, a mistake) in four
border variants each (a thicker line every fifth row or column), then the
sixteen picture colours the solved picture is shown in. lplate is the
cream board behind the clues and the grid, lpanel the narrow side panel.
"""

import argparse
import json
import math
from pathlib import Path
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from ng_artpack import ArtPack, text_pixels  # noqa: E402

SS = 8                                   # supersampling
PAINTED = GAME / "artbox/painted"

COLORS = [  # (main, light, dark, line) per piece colour 1..8
    ((246, 70, 96), (255, 170, 180), (196, 30, 70), (110, 14, 46)),     # ruby
    ((255, 150, 40), (255, 214, 140), (220, 96, 16), (120, 52, 10)),    # amber
    ((255, 214, 40), (255, 246, 170), (226, 160, 0), (118, 82, 0)),     # sunstone
    ((70, 200, 100), (180, 250, 170), (30, 150, 70), (14, 82, 40)),     # emerald
    ((50, 196, 236), (184, 246, 255), (16, 140, 200), (8, 76, 120)),    # aquamarine
    ((80, 110, 244), (176, 196, 255), (44, 60, 196), (20, 26, 112)),    # sapphire
    ((176, 90, 236), (230, 190, 255), (124, 50, 196), (66, 20, 112)),   # amethyst
    ((255, 110, 190), (255, 204, 234), (220, 60, 150), (120, 20, 80)),  # rose
]
# The picture-logic palette: a solved picture is drawn in these (layout 1-9, a-f).
PICTURE = [
    (255, 248, 236),   # 0 cream: the usual background
    (72, 32, 80),      # 1 plum
    (255, 255, 255),   # 2 white
    (255, 150, 196),   # 3 pink
    (236, 60, 92),     # 4 red
    (255, 150, 50),    # 5 orange
    (255, 222, 64),    # 6 yellow
    (96, 204, 104),    # 7 green
    (36, 140, 76),     # 8 dark green
    (130, 206, 252),   # 9 sky
    (70, 110, 230),    # a blue
    (172, 104, 232),   # b violet
    (172, 112, 62),    # c brown
    (156, 156, 178),   # d grey
    (40, 40, 76),      # e night
    (186, 242, 255),   # f ice
]
PLUM = (66, 18, 72)
FACE = (50, 22, 40)
BLUSH = (255, 120, 160)
WHITE = (255, 255, 255)
CREAM = (255, 248, 240)


def canvas(w, h):
    return Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))


def reduce(img, w, h):
    small = img.resize((w, h), Image.LANCZOS)
    a = np.asarray(small).copy()
    a[:, :, 3] = np.where(a[:, :, 3] >= 110, 255, 0)
    return a


def painted(name, w, h):
    p = PAINTED / f"{name}.png"
    if p.is_file():
        img = Image.open(p).convert("RGBA")
        if img.size == (w, h):
            return np.asarray(img)
        print(f"art: {p.name} is {img.size}, wanted {(w, h)}: drawing instead")
    return None


def S(v):
    return int(round(v * SS))


def grow(mask, px):
    """A mask dilated by px output pixels."""
    k = 2 * (S(px) // 2) + 1
    return mask.filter(ImageFilter.MaxFilter(k)) if k > 1 else mask


def sparkle(d, x, y, r, fill, thin=0.28):
    pts = []
    for i in range(8):
        a = i * math.pi / 4
        rr = r if i % 2 == 0 else r * thin
        pts.append((S(x + math.cos(a) * rr), S(y + math.sin(a) * rr)))
    d.polygon(pts, fill=fill)


# ---- pieces ----------------------------------------------------------------
def shape_mask(kind, cx, cy, r, size=32):
    """Piece shape `kind` (0..7) as a mask on a size x size supersampled canvas."""
    m = Image.new("L", (S(size), S(size)), 0)
    d = ImageDraw.Draw(m)

    def poly(points):
        d.polygon([(S(cx + x), S(cy + y)) for x, y in points], fill=255)

    def circle(x, y, rr):
        d.ellipse([S(cx + x - rr), S(cy + y - rr), S(cx + x + rr), S(cy + y + rr)], fill=255)

    def soften(blur, level):
        return m.filter(ImageFilter.GaussianBlur(S(blur))).point(lambda v: 255 if v > level else 0)
    if kind == 0:      # round-cut gem: a soft octagon
        poly([(math.cos(math.radians(22.5 + 45 * i)) * r * 1.02, math.sin(math.radians(22.5 + 45 * i)) * r * 1.02) for i in range(8)])
        m = soften(1.2 * r / 11.2, 128)
    elif kind == 1:    # triangle with round corners
        poly([(0, -r * 1.05), (r * 1.08, r * 0.82), (-r * 1.08, r * 0.82)])
        m = soften(1.8 * r / 11.2, 110)
    elif kind == 2:    # chubby star
        pts = []
        for i in range(10):
            a = -math.pi / 2 + i * math.pi / 5
            rr = r * (1.12 if i % 2 == 0 else 0.62)
            pts.append((math.cos(a) * rr, math.sin(a) * rr + r * 0.05))
        poly(pts)
        m = soften(1.4 * r / 11.2, 110)
    elif kind == 3:    # three-leaf clover with a stem
        for a in (-90, 30, 150):
            circle(math.cos(math.radians(a)) * r * 0.56, math.sin(math.radians(a)) * r * 0.56 - r * 0.08, r * 0.47)
        circle(0, -r * 0.08, r * 0.3)
        d.line([(S(cx), S(cy + r * 0.3)), (S(cx + r * 0.3), S(cy + r * 1.05))], fill=255, width=max(1, S(2.0 * r / 11.2)))
    elif kind == 4:    # droplet
        circle(0, r * 0.26, r * 0.82)
        poly([(0, -r * 1.12), (r * 0.72, 0), (-r * 0.72, 0)])
        m = soften(0.8 * r / 11.2, 128)
    elif kind == 5:    # crescent moon
        circle(0, 0, r * 1.02)
        d.ellipse([S(cx + r * 0.05), S(cy - r * 1.18), S(cx + r * 1.45), S(cy + r * 0.52)], fill=0)
        m = soften(0.8 * r / 11.2, 128)
    elif kind == 6:    # heart
        pts = []
        for i in range(0, 360, 5):
            t = math.radians(i)
            x = 16 * math.sin(t) ** 3
            y = -(13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t))
            pts.append((x * r * 1.1 / 16, y * r * 1.1 / 16 + r * 0.07))
        poly(pts)
    else:              # flower: five round petals
        for i in range(5):
            a = -math.pi / 2 + i * 2 * math.pi / 5
            circle(math.cos(a) * r * 0.64, math.sin(a) * r * 0.64 + r * 0.05, r * 0.44)
        circle(0, r * 0.05, r * 0.56)
    return m


FACE_AT = {0: (0, 0.09), 1: (0, 0.32), 2: (0, 0.14), 3: (0, -0.1), 4: (0, 0.38), 5: (-0.42, 0.05), 6: (0, -0.02), 7: (0, 0.05)}


def face(d, x, y, kind, s=1.0):
    """A little face centred at (x, y): dot eyes with a shine, blush, a mouth."""
    if kind == 5:      # the moon dozes
        for side in (-1, 1):
            ex = x + side * 2.3 * s
            d.arc([S(ex - 1.6 * s), S(y - 1.6 * s), S(ex + 1.6 * s), S(y + 1.0 * s)], 20, 160, fill=FACE + (255,), width=max(1, S(0.9 * s)))
        for side in (-1, 1):
            d.ellipse([S(x + side * 3.5 * s - 1.1 * s), S(y + 1.4 * s), S(x + side * 3.5 * s + 1.1 * s), S(y + 2.6 * s)], fill=BLUSH + (255,))
        return
    for side in (-1, 1):
        ex = x + side * 3.0 * s
        d.ellipse([S(ex - 1.05 * s), S(y - 1.9 * s), S(ex + 1.05 * s), S(y + 1.1 * s)], fill=FACE + (255,))
        d.ellipse([S(ex - 0.75 * s), S(y - 1.7 * s), S(ex + 0.05 * s), S(y - 0.8 * s)], fill=WHITE + (255,))
        bx = ex + side * 2.2 * s
        d.ellipse([S(bx - 1.4 * s), S(y + 1.3 * s), S(bx + 1.4 * s), S(y + 2.4 * s)], fill=BLUSH + (255,))
    if kind in (2, 6):  # an open smile
        d.chord([S(x - 1.4 * s), S(y + 0.2 * s), S(x + 1.4 * s), S(y + 2.6 * s)], 0, 180, fill=(190, 40, 70, 255))
    else:              # a cat's mouth
        for side in (-1, 1):
            d.arc([S(x + (side - 1) * 0.9 * s), S(y + 0.2 * s), S(x + (side + 1) * 0.9 * s), S(y + 1.9 * s)], 10, 170,
                  fill=FACE + (255,), width=max(1, S(0.6 * s)))


def candy(color, cx, cy, r, size=32, with_face=True, stripes=0):
    """A glossy candy piece of colour `color` (0..7) on a size x size canvas."""
    main, light, dark, line = COLORS[color]
    m = shape_mask(color, cx, cy, r, size)
    img = Image.new("RGBA", m.size, (0, 0, 0, 0))
    img.paste(line + (255,), (0, 0), grow(m, 1.3 * r / 11.2))
    mk = np.asarray(m) > 0
    yy, xx = np.mgrid[0:m.size[1], 0:m.size[0]]
    ys = np.where(mk)[0]
    t = ((yy - ys.min()) / max(1, ys.max() - ys.min()))[..., None]
    base = np.where(t < 0.45, np.asarray(light) * (1 - t / 0.45) + np.asarray(main) * (t / 0.45), np.asarray(main))
    shade = mk & ~np.roll(np.roll(mk, -S(1.6 * r / 11.2), 0), -S(1.4 * r / 11.2), 1)
    a = np.asarray(img).copy()
    a[mk, :3] = base[mk].astype(np.uint8)
    a[shade, :3] = dark
    a[mk, 3] = 255
    if stripes:
        band = ((yy // S(3.2)) % 2 == 0) if stripes == 1 else ((xx // S(3.2)) % 2 == 0)
        sel = mk & band & ~shade
        a[sel, :3] = (a[sel, :3].astype(int) * 0.45 + 255 * 0.55).astype(np.uint8)
    img = Image.fromarray(a, "RGBA")
    d = ImageDraw.Draw(img)
    k = r / 11.2
    gl = {0: (-4.5, -5), 1: (-2.5, -3.5), 2: (-2.0, -4.5), 3: (-4.5, -6.5), 4: (-3.2, -1.5), 5: (-6.0, -4.5), 6: (-5.0, -4.6), 7: (-5.5, -6.5)}[color]
    gx, gy = cx + gl[0] * k, cy + gl[1] * k
    d.ellipse([S(gx - 2.2 * k), S(gy - 1.4 * k), S(gx + 2.2 * k), S(gy + 1.4 * k)], fill=WHITE + (255,))
    d.ellipse([S(gx + 2.8 * k), S(gy - 1.2 * k), S(gx + 3.9 * k), S(gy - 0.1 * k)], fill=WHITE + (255,))
    if color == 7:     # the flower's heart, where its face sits
        d.ellipse([S(cx - r * 0.5), S(cy + r * 0.05 - r * 0.46), S(cx + r * 0.5), S(cy + r * 0.05 + r * 0.46)], fill=line + (255,))
        d.ellipse([S(cx - r * 0.44), S(cy + r * 0.05 - r * 0.40), S(cx + r * 0.44), S(cy + r * 0.05 + r * 0.40)], fill=(255, 236, 150, 255))
    if with_face:
        fx, fy = FACE_AT[color]
        face(d, cx + fx * r, cy + fy * r, color, (0.92 if color in (1, 4) else 1.0) * k)
    return img


def piece(color, kind):
    """32 x 32: colour 0..7, kind 0 plain, 1 row stripes, 2 column stripes, 3 bomb."""
    img = candy(color, 16, 16, 11.2, stripes=kind if kind in (1, 2) else 0)
    d = ImageDraw.Draw(img)
    line = COLORS[color][3]
    if kind in (1, 2):
        for sgn in (-1, 1):
            # a white arrow with a dark rim, pointing out of the piece
            def pt(u, v):
                return (S(16 + sgn * u), S(16 + v)) if kind == 1 else (S(16 + v), S(16 + sgn * u))
            d.polygon([pt(15.8, 0), pt(10.2, -5.4), pt(10.2, 5.4)], fill=line + (255,))
            d.polygon([pt(14.0, 0), pt(11.2, -2.9), pt(11.2, 2.9)], fill=WHITE + (255,))
    elif kind == 3:
        # a bomb: a golden ring with sparkles at the corners
        d.ellipse([S(1.2), S(1.2), S(30.8), S(30.8)], outline=(120, 70, 0, 255), width=S(2.6))
        d.ellipse([S(1.8), S(1.8), S(30.2), S(30.2)], outline=(255, 214, 60, 255), width=S(1.4))
        for a in (45, 135, 225, 315):
            sparkle(d, 16 + math.cos(math.radians(a)) * 14.2, 16 + math.sin(math.radians(a)) * 14.2, 3.4, (255, 255, 220, 255))
    return reduce(img, 32, 32)


def prism():
    img = canvas(32, 32)
    d = ImageDraw.Draw(img)
    d.ellipse([S(2.5), S(2.5), S(29.5), S(29.5)], fill=PLUM + (255,))
    for i, c in enumerate(COLORS):
        a0 = i * 45 - 90
        d.pieslice([S(4), S(4), S(28), S(28)], a0, a0 + 45, fill=c[0] + (255,))
    d.ellipse([S(8), S(8), S(24), S(24)], fill=(255, 250, 255, 255))
    d.ellipse([S(6.5), S(6), S(12), S(9.5)], fill=WHITE + (255,))
    face(d, 16, 16.5, 2, 1.05)
    return reduce(img, 32, 32)


def under(frame):
    """32 x 32 cell backs: 0 ice, 1 thick ice, 2-4 crate 1-3, 5 wall, 6 and 7 the gingham slots."""
    img = canvas(32, 32)
    d = ImageDraw.Draw(img)
    box = [S(0.5), S(0.5), S(31.5), S(31.5)]
    if frame in (6, 7):
        edge, fill = ((240, 196, 220), (255, 238, 246)) if frame == 6 else ((232, 178, 208), (255, 220, 236))
        d.rounded_rectangle(box, radius=S(6), fill=edge + (255,))
        d.rounded_rectangle([S(1.5), S(1.5), S(30.5), S(30.5)], radius=S(5), fill=fill + (255,))
        d.ellipse([S(3), S(3), S(5), S(5)], fill=WHITE + (255,))
    elif frame in (0, 1):
        edge, fill, deep = ((120, 180, 236), (210, 240, 255), (176, 222, 252)) if frame == 0 else \
                           ((80, 140, 220), (170, 214, 250), (130, 186, 240))
        d.rounded_rectangle(box, radius=S(6), fill=edge + (255,))
        d.rounded_rectangle([S(1.5), S(1.5), S(30.5), S(30.5)], radius=S(5), fill=fill + (255,))
        d.polygon([(S(2), S(31)), (S(31), S(2)), (S(31), S(31))], fill=deep + (255,))
        d.rounded_rectangle([S(1.5), S(1.5), S(30.5), S(30.5)], radius=S(5), outline=edge + (255,), width=S(1))
        for k in range(2 + frame):
            x0 = 5 + 7 * k
            d.line([S(x0), S(4), S(x0 - 3), S(10)], fill=WHITE + (255,), width=S(1.6))
        if frame == 1:
            for a in range(0, 180, 60):
                dx, dy = math.cos(math.radians(a)) * 5, math.sin(math.radians(a)) * 5
                d.line([S(22 - dx), S(22 - dy), S(22 + dx), S(22 + dy)], fill=WHITE + (255,), width=S(1.4))
    elif frame in (2, 3, 4):
        wood, deep, edge = [((246, 204, 140), (226, 170, 104), (150, 96, 50)),
                            ((226, 170, 104), (196, 134, 74), (120, 70, 34)),
                            ((190, 126, 70), (156, 96, 50), (96, 52, 24))][frame - 2]
        d.rounded_rectangle(box, radius=S(4), fill=edge + (255,))
        d.rounded_rectangle([S(2), S(2), S(30), S(30)], radius=S(3), fill=wood + (255,))
        for y in (11, 21):
            d.line([S(3), S(y), S(29), S(y)], fill=deep + (255,), width=S(1.2))
        d.rectangle([S(13), S(2), S(19), S(30)], fill=(255, 150, 190, 255))       # a ribbon
        d.rectangle([S(13), S(2), S(14), S(30)], fill=(255, 200, 224, 255))
        # a paw stamp per hit point
        for k in range(frame - 1):
            px = 7 + k * 9 if frame == 4 else 8 + k * 16
            py = 25
            d.ellipse([S(px - 2), S(py - 1.5), S(px + 2), S(py + 2)], fill=edge + (255,))
            for tx in (-2.2, 0, 2.2):
                d.ellipse([S(px + tx - 0.9), S(py - 4.2), S(px + tx + 0.9), S(py - 2.4)], fill=edge + (255,))
    else:              # a wall of lavender bricks
        d.rounded_rectangle(box, radius=S(3), fill=(92, 76, 130, 255))
        for row, y in enumerate((1.5, 11.5, 21.5)):
            off = 0 if row % 2 == 0 else -8
            for x in range(off, 32, 16):
                d.rounded_rectangle([S(max(1.5, x + 1)), S(y), S(min(30.5, x + 15)), S(y + 9)], radius=S(1.5), fill=(170, 156, 214, 255))
                d.line([S(max(2, x + 2)), S(y + 1), S(min(30, x + 14)), S(y + 1)], fill=(206, 196, 240, 255), width=S(1))
    return reduce(img, 32, 32)


def lock():
    """A heart padlock on a chain."""
    img = canvas(32, 32)
    d = ImageDraw.Draw(img)
    for k in range(4):
        x = 2 + k * 7.5
        d.ellipse([S(x), S(13), S(x + 8), S(19)], outline=(90, 80, 110, 255), width=S(2.2))
        d.ellipse([S(x + 0.8), S(13.8), S(x + 7.2), S(18.2)], outline=(220, 214, 236, 255), width=S(1))
    d.arc([S(11), S(4), S(21), S(16)], 180, 360, fill=(90, 80, 110, 255), width=S(3))
    d.arc([S(11.6), S(4.6), S(20.4), S(15.4)], 180, 360, fill=(220, 214, 236, 255), width=S(1.2))
    h = Image.new("L", img.size, 0)
    pts = []
    for i in range(0, 360, 6):
        t = math.radians(i)
        x = 16 * math.sin(t) ** 3
        y = -(13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t))
        pts.append((S(16 + x * 0.5), S(19.5 + y * 0.5)))
    ImageDraw.Draw(h).polygon(pts, fill=255)
    img.paste(PLUM + (255,), (0, 0), grow(h, 1.2))
    img.paste((255, 110, 170, 255), (0, 0), h)
    d = ImageDraw.Draw(img)
    d.ellipse([S(10.5), S(13.5), S(13.5), S(16)], fill=(255, 200, 226, 255))
    d.ellipse([S(14.8), S(17), S(17.2), S(19.4)], fill=PLUM + (255,))
    d.polygon([(S(15.4), S(19)), (S(16.6), S(19)), (S(16.9), S(22)), (S(15.1), S(22))], fill=PLUM + (255,))
    return reduce(img, 32, 32)


def bubble(color, kind):
    """16 x 16: 0-7 colours, 8 stone, 9 anchor; kind 1 holds a kitten."""
    img = canvas(16, 16)
    d = ImageDraw.Draw(img)
    if color == 8:
        d.ellipse([S(0.5), S(0.5), S(15.5), S(15.5)], fill=(60, 54, 78, 255))
        d.ellipse([S(1.5), S(1.5), S(14.5), S(14.5)], fill=(150, 144, 170, 255))
        d.ellipse([S(1.5), S(1.5), S(12.5), S(11.5)], fill=(186, 180, 204, 255))
        d.line([S(6), S(5), S(8), S(8), S(7), S(11)], fill=(90, 84, 110, 255), width=S(0.9))
        d.ellipse([S(3.5), S(3), S(6), S(5)], fill=WHITE + (255,))
        return reduce(img, 16, 16)
    if color == 9:
        d.ellipse([S(0.5), S(0.5), S(15.5), S(15.5)], fill=(110, 64, 10, 255))
        d.ellipse([S(1.5), S(1.5), S(14.5), S(14.5)], fill=(240, 186, 60, 255))
        d.ellipse([S(1.5), S(1.5), S(12.5), S(11)], fill=(255, 220, 110, 255))
        d.line([S(8), S(4), S(8), S(12.5)], fill=(110, 64, 10, 255), width=S(1.6))
        d.arc([S(3.6), S(6.5), S(12.4), S(13.2)], 20, 160, fill=(110, 64, 10, 255), width=S(1.6))
        d.ellipse([S(6.4), S(2.4), S(9.6), S(5.4)], outline=(110, 64, 10, 255), width=S(1.1))
        return reduce(img, 16, 16)
    main, light, dark, line = COLORS[color]
    d.ellipse([S(0.4), S(0.4), S(15.6), S(15.6)], fill=line + (255,))
    d.ellipse([S(1.3), S(1.3), S(14.7), S(14.7)], fill=dark + (255,))
    d.ellipse([S(1.4), S(1.2), S(13.6), S(13.2)], fill=main + (255,))
    d.ellipse([S(2.2), S(1.8), S(10.6), S(8.4)], fill=light + (255,))
    if kind:
        # a kitten looking out
        fur, inner = (255, 186, 92), (255, 236, 200)
        head = Image.new("L", img.size, 0)
        hd = ImageDraw.Draw(head)
        hd.ellipse([S(3.6), S(5.4), S(12.4), S(12.6)], fill=255)
        hd.polygon([(S(3.8), S(8)), (S(4.2), S(3.6)), (S(7.2), S(6))], fill=255)
        hd.polygon([(S(12.2), S(8)), (S(11.8), S(3.6)), (S(8.8), S(6))], fill=255)
        img.paste((110, 50, 26, 255), (0, 0), grow(head, 0.6))
        img.paste(fur + (255,), (0, 0), head)
        d = ImageDraw.Draw(img)
        d.ellipse([S(5.6), S(9.4), S(10.4), S(12.4)], fill=inner + (255,))
        d.ellipse([S(5.6), S(7.6), S(6.9), S(9.3)], fill=FACE + (255,))
        d.ellipse([S(9.1), S(7.6), S(10.4), S(9.3)], fill=FACE + (255,))
        d.ellipse([S(4.2), S(9.6), S(5.6), S(10.6)], fill=BLUSH + (255,))
        d.ellipse([S(10.4), S(9.6), S(11.8), S(10.6)], fill=BLUSH + (255,))
    else:
        mark = shape_mask(color, 8, 8.4, 3.7, 16)
        img.paste(line + (255,), (0, 0), grow(mark, 0.6))
        img.paste(WHITE + (255,), (0, 0), mark)
        d = ImageDraw.Draw(img)
    d.ellipse([S(3.2), S(2.4), S(6.4), S(4.6)], fill=WHITE + (255,))
    return reduce(img, 16, 16)


# ---- cats ------------------------------------------------------------------
CATS = {
    "mimi": dict(fur=(255, 172, 72), shade=(236, 122, 44), light=(255, 214, 140), line=(104, 46, 26),
                 cream=(255, 240, 214), stripe=(226, 110, 40), iris=(70, 170, 70), iris2=(150, 226, 110),
                 lid=(104, 46, 26)),
    "kiko": dict(fur=(78, 80, 116), shade=(50, 50, 80), light=(120, 124, 164), line=(28, 24, 48),
                 cream=(250, 250, 255), stripe=None, iris=(230, 160, 20), iris2=(255, 230, 90),
                 lid=(226, 226, 246)),
}
CAT_PINK = (255, 150, 180)


def ellipse_mask(cx, cy, rx, ry, size=64):
    m = Image.new("L", (S(size), S(size)), 0)
    ImageDraw.Draw(m).ellipse([S(cx - rx), S(cy - ry), S(cx + rx), S(cy + ry)], fill=255)
    return m


def cat(name, mood):
    """A round puffball cat on a 64-unit canvas (supersampled; reduce to 64 or 96).
    Moods: 0 idle, 1 blink, 2 happy, 3 wow, 4 sad, 5 cheer; 6 and 7 the lucky
    cat beckoning (paw up, paw down), with a red collar, a bell and a coin."""
    c = CATS[name]
    line = c["line"] + (255,)
    img = canvas(64, 64)
    lift = -2 if mood == 5 else 0
    bx, by, brx, bry = 32, 39 + lift, 25, 21.5
    droop = 3 if mood == 4 else 0
    sil = Image.new("L", img.size, 0)
    sd = ImageDraw.Draw(sil)
    tail = Image.new("L", img.size, 0)
    td = ImageDraw.Draw(tail)
    td.arc([S(44), S(28 + lift), S(62), S(56 + lift)], 270, 90, fill=255, width=S(5))
    td.ellipse([S(51), S(25 + lift), S(59), S(33 + lift)], fill=255)
    ears = []
    for side in (-1, 1):
        base_x = bx + side * 13
        tip = (bx + side * (19 + droop), 9 + lift + droop * 1.6)
        ears.append([(S(base_x - 9), S(26 + lift)), (S(tip[0]), S(tip[1])), (S(base_x + 9), S(22 + lift))])
    for e in ears:
        sd.polygon(e, fill=255)
    sd.ellipse([S(bx - brx), S(by - bry), S(bx + brx), S(by + bry)], fill=255)
    feet = Image.new("L", img.size, 0)
    fd = ImageDraw.Draw(feet)
    for side in (-1, 1):
        fd.ellipse([S(bx + side * 11 - 7), S(56 + lift), S(bx + side * 11 + 7), S(63)], fill=255)
    paws = Image.new("L", img.size, 0)
    pd = ImageDraw.Draw(paws)
    for side in (-1, 1):
        px, py = (bx + side * 25, 22 + lift) if mood == 5 else (bx + side * 22, 47 + lift)
        if mood in (6, 7) and side < 0:
            # the beckoning paw, up beside the head on its arm
            px, py = bx - 26, (13 if mood == 6 else 21)
            pd.line([S(bx - 21), S(by + 4), S(px + 1), S(py + 3)], fill=255, width=S(8))
        if mood in (6, 7) and side > 0:
            px, py = bx + 12, 56                            # the paw holding the coin
        pd.ellipse([S(px - 5), S(py - 5), S(px + 5), S(py + 5)], fill=255)
    everything = Image.fromarray(np.maximum.reduce([np.asarray(m) for m in (sil, tail, feet, paws)]))
    img.paste(line, (0, 0), grow(everything, 1.4))
    d = ImageDraw.Draw(img)
    img.paste(c["shade"] + (255,), (0, 0), tail)
    img.paste(line, (0, 0), grow(feet, 1.2))
    img.paste(c["shade"] + (255,), (0, 0), feet)
    if name == "kiko":
        img.paste(c["cream"] + (255,), (0, 0), ellipse_mask(55, 29 + lift, 3, 3))
    for e in ears:
        d.polygon(e, fill=c["fur"] + (255,))
    body = ellipse_mask(bx, by, brx, bry)
    img.paste(c["fur"] + (255,), (0, 0), body)
    lower = (np.asarray(body) > 0) & (np.asarray(ellipse_mask(bx - 2.5, by - 3, brx, bry)) == 0)
    img.paste(c["shade"] + (255,), (0, 0), Image.fromarray(np.where(lower, 255, 0).astype(np.uint8)))
    rim = (np.asarray(body) > 0) & (np.asarray(ellipse_mask(bx + 1.6, by + 2.2, brx, bry)) == 0) & \
          (np.arange(S(64))[:, None] < S(by - 4))
    img.paste(c["light"] + (255,), (0, 0), Image.fromarray(np.where(rim, 255, 0).astype(np.uint8)))
    for e in ears:
        (x0, y0), (x1, y1), (x2, y2) = e
        cxe, cye = (x0 + x1 + x2) / 3, (y0 + y1 + y2) / 3
        d.polygon([((x - cxe) * 0.55 + cxe, (y - cye) * 0.55 + cye + S(1)) for x, y in e], fill=CAT_PINK + (255,))
    if name == "kiko":
        blaze = Image.new("L", img.size, 0)
        bd = ImageDraw.Draw(blaze)
        bd.polygon([(S(bx - 1.5), S(by - bry + 3)), (S(bx + 1.5), S(by - bry + 3)), (S(bx + 5), S(by + 4)), (S(bx - 5), S(by + 4))], fill=255)
        bd.ellipse([S(bx - 9), S(by + 1.5), S(bx + 9), S(by + 11)], fill=255)
        bd.ellipse([S(bx - 9), S(by + 8), S(bx + 9), S(by + 22)], fill=255)
        img.paste(c["cream"] + (255,), (0, 0), Image.fromarray(np.minimum(np.asarray(blaze), np.asarray(body))))
        socks = Image.new("L", img.size, 0)
        ImageDraw.Draw(socks).rectangle([0, S(57 + lift), S(64), S(64)], fill=255)
        img.paste(c["cream"] + (255,), (0, 0), Image.fromarray(np.minimum(np.asarray(socks), np.asarray(feet))))
    else:
        for k in (-5, 0, 5):
            d.line([S(bx + k), S(by - bry + 2), S(bx + k * 0.8), S(by - bry + 7)], fill=c["stripe"] + (255,), width=S(2))
        for side in (-1, 1):
            for k in (0, 5):
                y = by - 4 + k
                d.line([S(bx + side * (brx - 0.5)), S(y), S(bx + side * (brx - 5)), S(y + 1)], fill=c["stripe"] + (255,), width=S(1.8))
        belly = ellipse_mask(bx, by + 12, 12, 8)
        img.paste(c["cream"] + (255,), (0, 0), Image.fromarray(np.minimum(np.asarray(belly), np.asarray(body))))
        img.paste(c["cream"] + (255,), (0, 0), ellipse_mask(bx, by + 5.5, 6.5, 4))
    if mood in (6, 7):
        # a red collar with a golden bell, and a gold coin held in front
        collar = Image.fromarray(np.minimum(np.asarray(ellipse_mask(bx, by + 2, brx, bry)),
                                            np.where((np.arange(S(64))[:, None] > S(by + 11.5)) &
                                                     (np.arange(S(64))[:, None] < S(by + 14.5)), 255, 0).astype(np.uint8)))
        img.paste((226, 40, 60, 255), (0, 0), collar)
        dd = ImageDraw.Draw(img)
        dd.ellipse([S(bx - 3.2), S(by + 13), S(bx + 3.2), S(by + 19)], fill=(120, 70, 0, 255))
        dd.ellipse([S(bx - 2.5), S(by + 13.6), S(bx + 2.5), S(by + 18.4)], fill=(255, 210, 60, 255))
        dd.line([S(bx), S(by + 16.5), S(bx), S(by + 18.4)], fill=(120, 70, 0, 255), width=S(0.8))
        dd.ellipse([S(bx + 6), S(by + 8), S(bx + 19), S(by + 24)], fill=(120, 70, 0, 255))
        dd.ellipse([S(bx + 7), S(by + 9), S(bx + 18), S(by + 23)], fill=(255, 206, 50, 255))
        dd.ellipse([S(bx + 9), S(by + 11), S(bx + 16), S(by + 21)], outline=(226, 150, 10, 255), width=S(0.8))
        dd.ellipse([S(bx + 9.5), S(by + 11), S(bx + 12), S(by + 14)], fill=(255, 250, 200, 255))
    img.paste(line, (0, 0), grow(paws, 1.2))
    img.paste((c["cream"] if name == "kiko" else c["fur"]) + (255,), (0, 0), paws)
    d = ImageDraw.Draw(img)

    def E(cx, cy, rx, ry, fill):
        d.ellipse([S(cx - rx), S(cy - ry), S(cx + rx), S(cy + ry)], fill=fill)
    lid = c["lid"] + (255,)
    ey = by + 0.5
    for side in (-1, 1):
        ex = bx + side * 10.5
        if mood == 1:            # blink: closed, content
            d.arc([S(ex - 4.5), S(ey - 4), S(ex + 4.5), S(ey + 2.5)], 15, 165, fill=lid, width=S(2.2))
        elif mood in (2, 5, 6, 7):   # happy: ^ ^
            d.arc([S(ex - 4.5), S(ey - 1.5), S(ex + 4.5), S(ey + 5.5)], 195, 345, fill=lid, width=S(2.3))
        else:
            big = mood == 3
            rx, ry = (5.6, 6.8) if big else (4.9, 6.2)
            E(ex, ey, rx + 0.9, ry + 0.9, line)
            E(ex, ey, rx, ry, c["iris"] + (255,))
            E(ex, ey + ry * 0.35, rx * 0.85, ry * 0.55, c["iris2"] + (255,))
            E(ex, ey - 0.3, rx * 0.55, ry * 0.62, (30, 20, 30, 255))
            E(ex - rx * 0.30, ey - ry * 0.36, 2.3 if big else 1.9, 2.3 if big else 1.9, WHITE + (255,))
            E(ex + rx * 0.34, ey + ry * 0.34, 1.0, 1.0, WHITE + (255,))
            if big:
                E(ex + rx * 0.3, ey - ry * 0.55, 0.7, 0.7, WHITE + (255,))
            if mood == 4:        # sad: a worried brow and a tear
                d.line([S(ex - side * 5), S(ey - 9.5), S(ex + side * 3.5), S(ey - 7.5)], fill=lid, width=S(1.8))
                if side > 0:
                    d.polygon([(S(ex + 4.5), S(ey + 3)), (S(ex + 2.6), S(ey + 8)), (S(ex + 6.4), S(ey + 8))], fill=(150, 220, 255, 255))
                    E(ex + 4.5, ey + 8.2, 1.9, 1.9, (150, 220, 255, 255))
        E(bx + side * 16.5, ey + 6.5, 3.6, 2.2, BLUSH + (255,))
    ny = ey + 5
    d.polygon([(S(bx - 1.6), S(ny)), (S(bx + 1.6), S(ny)), (S(bx), S(ny + 1.6))], fill=(240, 100, 130, 255))
    if mood in (2, 5, 6, 7):
        d.chord([S(bx - 3.6), S(ny + 1.2), S(bx + 3.6), S(ny + 7.5)], 0, 180, fill=(200, 50, 80, 255))
        d.chord([S(bx - 2.4), S(ny + 4.0), S(bx + 2.4), S(ny + 7.5)], 180, 360, fill=(255, 110, 130, 255))
    elif mood == 3:
        E(bx, ny + 4.2, 1.8, 2.2, (200, 50, 80, 255))
    elif mood == 4:
        d.arc([S(bx - 3), S(ny + 2.6), S(bx + 3), S(ny + 6.5)], 200, 340, fill=line, width=S(1.1))
    else:
        for side in (-1, 1):
            d.arc([S(bx + (side - 1) * 1.9), S(ny - 0.4), S(bx + (side + 1) * 1.9), S(ny + 3.4)], 10, 170, fill=line, width=S(1.1))
    if mood == 5:
        for sx, sy, r in ((6, 8, 3.5), (58, 6, 3), (4, 34, 2.5), (60, 44, 2.5)):
            sparkle(d, sx, sy, r, (255, 236, 120, 255), 0.3)
    return img


# ---- the board's small things ------------------------------------------------
def cursor(frame):
    """Rounded corner brackets: white (moving) or gold with a paw print (holding a piece)."""
    img = canvas(32, 32)
    d = ImageDraw.Draw(img)
    ink = WHITE if frame == 0 else (255, 214, 70)
    m = Image.new("L", img.size, 0)
    md = ImageDraw.Draw(m)
    for x0, y0, dx, dy in ((2, 2, 1, 1), (30, 2, -1, 1), (2, 30, 1, -1), (30, 30, -1, -1)):
        md.line([S(x0), S(y0), S(x0 + 8 * dx), S(y0)], fill=255, width=S(2.4))
        md.line([S(x0), S(y0), S(x0), S(y0 + 8 * dy)], fill=255, width=S(2.4))
        md.ellipse([S(x0 - 1.2), S(y0 - 1.2), S(x0 + 1.2), S(y0 + 1.2)], fill=255)
    img.paste(PLUM + (255,), (0, 0), grow(m, 1.0))
    img.paste(ink + (255,), (0, 0), m)
    if frame == 1:
        paw = Image.new("L", img.size, 0)
        pd = ImageDraw.Draw(paw)
        pd.ellipse([S(21), S(21), S(28), S(27)], fill=255)
        for tx, ty in ((20.2, 18.6), (23.2, 17.2), (26.4, 17.6), (28.6, 20.4)):
            pd.ellipse([S(tx - 1.3), S(ty - 1.5), S(tx + 1.3), S(ty + 1.5)], fill=255)
        img.paste(PLUM + (255,), (0, 0), grow(paw, 0.8))
        img.paste((255, 130, 180, 255), (0, 0), paw)
    return reduce(img, 32, 32)


def pop(frame):
    """A burst of little stars and hearts."""
    img = canvas(32, 32)
    d = ImageDraw.Draw(img)
    tints = [(255, 150, 200), (255, 230, 110), (150, 240, 190), (150, 210, 255)]
    r = 4 + frame * 3.6
    s = max(1.2, 3.6 - frame * 0.7)
    for i, a in enumerate(range(0, 360, 45)):
        x, y = 16 + math.cos(math.radians(a + frame * 12)) * r, 16 + math.sin(math.radians(a + frame * 12)) * r
        tint = tints[i % 4] + (255,)
        if i % 2:
            sparkle(d, x, y, s, tint, 0.34)
        else:
            d.ellipse([S(x - s * 0.55), S(y - s * 0.55), S(x - s * 0.05), S(y)], fill=tint)
            d.ellipse([S(x + s * 0.05), S(y - s * 0.55), S(x + s * 0.55), S(y)], fill=tint)
            d.polygon([(S(x - s * 0.55), S(y - s * 0.2)), (S(x + s * 0.55), S(y - s * 0.2)), (S(x), S(y + s * 0.55))], fill=tint)
    if frame < 2:
        d.ellipse([S(16 - r * 0.6), S(16 - r * 0.6), S(16 + r * 0.6), S(16 + r * 0.6)], outline=WHITE + (255,), width=S(1.2))
    return reduce(img, 32, 32)


def launcher():
    """A cat-eared cushion the bubble sits on."""
    img = canvas(32, 32)
    m = Image.new("L", img.size, 0)
    md = ImageDraw.Draw(m)
    md.ellipse([S(3), S(11), S(29), S(31)], fill=255)
    md.polygon([(S(4), S(17)), (S(5), S(6)), (S(12), S(12))], fill=255)
    md.polygon([(S(28), S(17)), (S(27), S(6)), (S(20), S(12))], fill=255)
    img.paste(PLUM + (255,), (0, 0), grow(m, 1.0))
    img.paste((214, 170, 250, 255), (0, 0), m)
    d = ImageDraw.Draw(img)
    d.polygon([(S(6), S(14)), (S(6.4), S(9)), (S(10), S(12.6))], fill=CAT_PINK + (255,))
    d.polygon([(S(26), S(14)), (S(25.6), S(9)), (S(22), S(12.6))], fill=CAT_PINK + (255,))
    d.ellipse([S(5), S(20), S(27), S(31)], fill=(182, 130, 236, 255))
    for side in (-1, 1):
        d.ellipse([S(16 + side * 7 - 1.2), S(22), S(16 + side * 7 + 1.2), S(25)], fill=FACE + (255,))
        d.ellipse([S(16 + side * 10.5 - 1.8), S(25.5), S(16 + side * 10.5 + 1.8), S(27)], fill=BLUSH + (255,))
    for side in (-1, 1):
        d.arc([S(16 + (side - 1) * 1.3), S(24.6), S(16 + (side + 1) * 1.3), S(27)], 10, 170, fill=FACE + (255,), width=S(0.7))
    return reduce(img, 32, 32)


def dot():
    img = canvas(16, 16)
    d = ImageDraw.Draw(img)
    d.ellipse([S(5), S(5), S(11), S(11)], fill=PLUM + (255,))
    d.ellipse([S(6), S(6), S(10), S(10)], fill=(255, 220, 240, 255))
    d.ellipse([S(6.6), S(6.4), S(8.2), S(7.8)], fill=WHITE + (255,))
    return reduce(img, 16, 16)


def star(full):
    img = canvas(16, 16)
    m = Image.new("L", img.size, 0)
    pts = []
    for i in range(10):
        a = -math.pi / 2 + i * math.pi / 5
        rr = 6.8 if i % 2 == 0 else 3.4
        pts.append((S(8 + math.cos(a) * rr), S(8.6 + math.sin(a) * rr)))
    ImageDraw.Draw(m).polygon(pts, fill=255)
    m = m.filter(ImageFilter.GaussianBlur(S(0.5))).point(lambda v: 255 if v > 110 else 0)
    img.paste(PLUM + (255,), (0, 0), grow(m, 1.1))
    img.paste(((255, 206, 50) if full else (176, 166, 206)) + (255,), (0, 0), m)
    d = ImageDraw.Draw(img)
    d.ellipse([S(5.6), S(5.4), S(7.4), S(7.4)], fill=(WHITE if full else (214, 208, 236)) + (255,))
    return reduce(img, 16, 16)


def logo():
    """304 x 112 without the painted one: the name in the firmware's letters, outlined, with a warm gradient."""
    w, h = 304, 112
    top = text_pixels("SEVEN", 4)          # 32 tall
    bot = text_pixels("WONDERS", 4)        # 32 tall
    mask = np.zeros((h, w), dtype=bool)
    mask[12:44, (w - top.shape[1]) // 2:(w - top.shape[1]) // 2 + top.shape[1]] = top > 0
    mask[56:88, (w - bot.shape[1]) // 2:(w - bot.shape[1]) // 2 + bot.shape[1]] = bot > 0
    img = Image.fromarray((mask * 255).astype(np.uint8)).resize((w * SS, h * SS), Image.NEAREST)
    soft = img.filter(ImageFilter.GaussianBlur(SS * 0.6))
    big = np.asarray(soft) > 100
    outline = np.asarray(soft.filter(ImageFilter.MaxFilter(SS * 2 + 1))) > 60
    rgba = np.zeros((h * SS, w * SS, 4), dtype=np.uint8)
    rgba[outline] = PLUM + (255,)
    yy = np.linspace(0, 1, h * SS)[:, None].repeat(w * SS, 1)
    grad = np.stack([255 - yy * 20, 170 - yy * 90, 214 - yy * 60], axis=2).astype(np.uint8)
    rgba[big, :3] = grad[big]
    rgba[big, 3] = 255
    return reduce(Image.fromarray(rgba), w, h)


# ---- plates ------------------------------------------------------------------
PLATE_FILL = (64, 30, 92)
PLATE_RIM = (255, 140, 196)


def plate(w, h, body_h=None, fill=PLATE_FILL, radius=10):
    """A dark berry plate with a pink rim and a white dotted line inside it."""
    body_h = body_h or h
    img = Image.new("RGBA", (S(w), S(h)), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([S(0.5), S(0.5), S(w - 0.5), S(body_h - 0.5)], radius=S(radius), fill=PLUM + (255,))
    d.rounded_rectangle([S(1.5), S(1.5), S(w - 1.5), S(body_h - 1.5)], radius=S(radius - 1), fill=PLATE_RIM + (255,))
    d.rounded_rectangle([S(3.5), S(3.5), S(w - 3.5), S(body_h - 3.5)], radius=S(radius - 3), fill=fill + (255,))
    # dotted stitching along the rim
    step = 6
    for x in range(int(radius), int(w - radius), step):
        for y in (2.5, body_h - 2.5):
            d.ellipse([S(x - 0.6), S(y - 0.6), S(x + 0.6), S(y + 0.6)], fill=WHITE + (255,))
    for y in range(int(radius), int(body_h - radius), step):
        for x in (2.5, w - 2.5):
            d.ellipse([S(x - 0.6), S(y - 0.6), S(x + 0.6), S(y + 0.6)], fill=WHITE + (255,))
    return img, d


def corner_hearts(d, w, body_h, bottom=True):
    for x, y in ((9, 9), (w - 9, 9)) + (((9, body_h - 9), (w - 9, body_h - 9)) if bottom else ()):
        heart(d, x, y, 2.4, PLATE_RIM + (255,))


def heart(d, x, y, r, fill):
    pts = []
    for i in range(0, 360, 10):
        t = math.radians(i)
        hx = 16 * math.sin(t) ** 3
        hy = -(13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t))
        pts.append((S(x + hx * r / 16), S(y + hy * r / 16)))
    d.polygon(pts, fill=fill)


def panel():
    """The left panel in play (216 tall, in a 224 sprite)."""
    img, d = plate(96, 224, 216)
    corner_hearts(d, 96, 216, bottom=False)       # the last text row runs along the bottom
    return reduce(img, 96, 224)


def banner():
    """A strip plate, 272 wide: inside the 24-pixel margin a television may hide."""
    img, d = plate(272, 48, radius=9)
    return reduce(img, 272, 48)


def card():
    img, d = plate(256, 192, radius=12)
    corner_hearts(d, 256, 192)
    return reduce(img, 256, 192)


# ---- picture logic -------------------------------------------------------------
LGRID_X, LGRID_Y = 84, 52         # the grid's top-left on screen
LPLATE_X, LPLATE_Y = 16, 4        # where the plate sprite goes
LINFO_X = 248                     # the column on the right: the cat, the buttons
GRID_LINE = (226, 206, 232)
GRID_FIVE = (156, 112, 176)


def logic_cell(state, border, px):
    """One grid cell, px a side: state 0 empty, 1 filled, 2 crossed, 3 a mistake;
    border bit 0 a thick right line, bit 1 a thick bottom line."""
    img = Image.new("RGBA", (S(px), S(px)), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    bg = (255, 226, 234) if state == 3 else CREAM
    d.rectangle([0, 0, S(px), S(px)], fill=bg + (255,))
    k = px / 16
    if state == 1:
        d.rounded_rectangle([S(1.2 * k), S(1.2 * k), S(px - 2.2 * k), S(px - 2.2 * k)], radius=S(2.6 * k), fill=(150, 40, 104, 255))
        d.rounded_rectangle([S(1.8 * k), S(1.8 * k), S(px - 2.8 * k), S(px - 3.2 * k)], radius=S(2.2 * k), fill=(236, 92, 156, 255))
        d.rounded_rectangle([S(2.6 * k), S(2.4 * k), S(px - 3.6 * k), S(7.2 * k)], radius=S(1.8 * k), fill=(255, 150, 196, 255))
        d.ellipse([S(3.4 * k), S(3.0 * k), S(5.6 * k), S(4.8 * k)], fill=WHITE + (255,))
    elif state in (2, 3):
        ink = (126, 116, 196) if state == 2 else (226, 44, 80)
        a, b = 4.4 * k, px - 5.4 * k
        width = S(2.0 * k)
        d.line([S(a), S(a), S(b), S(b)], fill=ink + (255,), width=width)
        d.line([S(b), S(a), S(a), S(b)], fill=ink + (255,), width=width)
        for x, y in ((a, a), (b, b), (b, a), (a, b)):
            d.ellipse([S(x) - width // 2, S(y) - width // 2, S(x) + width // 2, S(y) + width // 2], fill=ink + (255,))
    # grid lines on the right and the bottom (the plate draws the outer frame)
    d.rectangle([S(px - 1), 0, S(px), S(px)], fill=GRID_LINE + (255,))
    d.rectangle([0, S(px - 1), S(px), S(px)], fill=GRID_LINE + (255,))
    if border & 1:
        d.rectangle([S(px - 1), 0, S(px), S(px)], fill=GRID_FIVE + (255,))
    if border & 2:
        d.rectangle([0, S(px - 1), S(px), S(px)], fill=GRID_FIVE + (255,))
    out = reduce(img, px, px)
    out[:, :, 3] = 255
    return out


def logic_color(index, px):
    """A solved picture's cell: flat, in picture colour `index`."""
    out = np.zeros((px, px, 4), dtype=np.uint8)
    out[:, :, :3] = PICTURE[index]
    out[:, :, 3] = 255
    return out


def logic_sheet(px):
    frames = [logic_cell(s, b, px) for s in range(4) for b in range(4)]
    frames += [logic_color(i, px) for i in range(16)]
    return sheet(frames, px, px)


def logic_plate(cells):
    """The cream board behind everything of a picture (288 x 224; sprite at LPLATE_X, LPLATE_Y):
    the corner box (hearts, time, score, squares left), the clues, the grid and the column on
    the right (the cat, the buttons). Everything that matters lies within x 24..296 on screen.
    cells: 10 (16-pixel cells) or 5 (32-pixel cells); the grid is 160 pixels either way."""
    w, h = 288, 224
    img = Image.new("RGBA", (S(w), S(h)), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    x0, y0, x1, y1 = 4, 4, 284, 214            # the plate's own edge (screen 20..300, 8..218)
    gx, gy = LGRID_X - LPLATE_X, LGRID_Y - LPLATE_Y
    ix = LINFO_X - LPLATE_X
    d.rounded_rectangle([S(x0), S(y0), S(x1), S(y1)], radius=S(9), fill=PLUM + (255,))
    d.rounded_rectangle([S(x0 + 1), S(y0 + 1), S(x1 - 1), S(y1 - 1)], radius=S(8), fill=PLATE_RIM + (255,))
    d.rounded_rectangle([S(x0 + 3), S(y0 + 3), S(x1 - 3), S(y1 - 3)], radius=S(6), fill=CREAM + (255,))
    pitch = 160 // cells
    # stripes behind the clue numbers, one per row and per column
    for k in range(cells):
        tint = (255, 232, 242) if k % 2 == 0 else (255, 244, 249)
        d.rectangle([S(x0 + 4), S(gy + k * pitch), S(gx - 2), S(gy + (k + 1) * pitch - 1)], fill=tint + (255,))
        d.rectangle([S(gx + k * pitch), S(y0 + 4), S(gx + (k + 1) * pitch - 1), S(gy - 2)], fill=tint + (255,))
    # the corner box: hearts, time, score, squares left
    d.rounded_rectangle([S(x0 + 4), S(y0 + 4), S(gx - 3), S(gy - 3)], radius=S(4), fill=(255, 214, 232, 255))
    # the column on the right: the cat and the buttons
    d.rounded_rectangle([S(ix - 2), S(y0 + 4), S(x1 - 4), S(y1 - 4)], radius=S(5), fill=(255, 226, 238, 255))
    # the grid's frame
    d.rectangle([S(gx - 2), S(gy - 2), S(gx + 160 + 1), S(gy + 160 + 1)], fill=GRID_FIVE + (255,))
    d.rectangle([S(gx), S(gy), S(gx + 160), S(gy + 160)], fill=CREAM + (255,))
    return reduce(img, w, h)


def logic_panel():
    """The narrow side panel beside the logic board (72 wide in an 80 sprite)."""
    img, d = plate(72, 224, 216)
    corner_hearts(d, 72, 216, bottom=False)
    full = Image.new("RGBA", (S(80), S(224)), (0, 0, 0, 0))
    full.paste(img, (0, 0))
    return reduce(full, 80, 224)


def logic_cursor(px):
    """A paw that frames the cell: pink corners, two frames for a gentle pulse."""
    frames = []
    for f in range(2):
        img = Image.new("RGBA", (S(px), S(px)), (0, 0, 0, 0))
        m = Image.new("L", img.size, 0)
        md = ImageDraw.Draw(m)
        k = px / 16
        inset = 0.8 + (0.6 if f else 0)
        arm = 4.8 * k
        for x0, y0, dx, dy in ((inset, inset, 1, 1), (px - inset, inset, -1, 1), (inset, px - inset, 1, -1), (px - inset, px - inset, -1, -1)):
            md.line([S(x0), S(y0), S(x0 + arm * dx), S(y0)], fill=255, width=S(1.8 * max(1, k * 0.8)))
            md.line([S(x0), S(y0), S(x0), S(y0 + arm * dy)], fill=255, width=S(1.8 * max(1, k * 0.8)))
        img.paste(PLUM + (255,), (0, 0), grow(m, 0.8))
        img.paste((255, 92, 160, 255) if f == 0 else (255, 160, 206, 255), (0, 0), m)
        frames.append(reduce(img, px, px))
    return frames


def heart_glyph(full):
    """An 8 x 8 FIX heart: 1 fill, 2 outline, 3 shine."""
    g = np.zeros((8, 8), dtype=np.uint8)
    rows = ["01100110", "11111111", "11111111", "11111111", "01111110", "00111100", "00011000", "00000000"]
    for y, r in enumerate(rows):
        for x, ch in enumerate(r):
            if ch == "1":
                g[y, x] = 1
    edge = np.zeros_like(g)
    for y in range(8):
        for x in range(8):
            if g[y, x] and (x in (0, 7) or y == 0 or not g[max(0, y - 1), x] or not g[min(7, y + 1), x] or
                            not g[y, max(0, x - 1)] or not g[y, min(7, x + 1)]):
                edge[y, x] = 1
    out = np.where(edge > 0, 2, g)
    if full:
        out[1, 1] = 3
        out[2, 1] = 3
    else:
        out = np.where(out == 1, 0, out)
    return out


# ---- rewards: the lucky paw, its prizes, the album, the cheering words --------
def prize(kind):
    """32 x 32: 0 a coin, 1 a stack of coins, 2 a lucky bag, 3 a paw (one more hammer), 4 the selector."""
    img = canvas(32, 32)
    d = ImageDraw.Draw(img)
    gold, deep, light = (255, 206, 50), (150, 90, 0), (255, 246, 190)

    def coin(x, y, rx, ry):
        d.ellipse([S(x - rx - 1), S(y - ry - 1), S(x + rx + 1), S(y + ry + 1)], fill=deep + (255,))
        d.ellipse([S(x - rx), S(y - ry), S(x + rx), S(y + ry)], fill=gold + (255,))
        d.ellipse([S(x - rx * 0.62), S(y - ry * 0.62), S(x + rx * 0.62), S(y + ry * 0.62)], outline=(226, 150, 10, 255), width=S(1))
        d.ellipse([S(x - rx * 0.6), S(y - ry * 0.75), S(x - rx * 0.1), S(y - ry * 0.35)], fill=light + (255,))
    if kind == 0:
        coin(16, 16, 9, 12)
    elif kind == 1:
        for k in range(3):
            d.rounded_rectangle([S(6), S(22 - k * 5), S(26), S(28 - k * 5)], radius=S(2.5), fill=deep + (255,))
            d.rounded_rectangle([S(7), S(23 - k * 5), S(25), S(27 - k * 5)], radius=S(2), fill=gold + (255,))
        coin(16, 9, 9, 6)
    elif kind == 2:
        bag = Image.new("L", img.size, 0)
        bd = ImageDraw.Draw(bag)
        bd.ellipse([S(5), S(11), S(27), S(30)], fill=255)
        bd.polygon([(S(11), S(13)), (S(9), S(5)), (S(23), S(5)), (S(21), S(13))], fill=255)
        img.paste(PLUM + (255,), (0, 0), grow(bag, 1.0))
        img.paste((236, 60, 92, 255), (0, 0), bag)
        d = ImageDraw.Draw(img)
        d.ellipse([S(7), S(13), S(17), S(21)], fill=(255, 130, 150, 255))
        d.rectangle([S(10), S(11), S(22), S(13.5)], fill=(255, 210, 60, 255))
        d.ellipse([S(14), S(18), S(26), S(28)], fill=(255, 210, 60, 255))
        d.ellipse([S(16), S(20), S(24), S(26)], outline=(226, 150, 10, 255), width=S(1))
        sparkle(d, 6, 6, 3.5, WHITE + (255,))
        sparkle(d, 27, 9, 2.5, (255, 246, 190, 255))
    elif kind == 3:
        paw = Image.new("L", img.size, 0)
        pd = ImageDraw.Draw(paw)
        pd.ellipse([S(8), S(14), S(24), S(28)], fill=255)
        for tx, ty in ((7, 10), (12.5, 6), (19.5, 6), (25, 10)):
            pd.ellipse([S(tx - 3.2), S(ty - 3.6), S(tx + 3.2), S(ty + 3.6)], fill=255)
        img.paste(PLUM + (255,), (0, 0), grow(paw, 1.2))
        img.paste((255, 130, 180, 255), (0, 0), paw)
        d = ImageDraw.Draw(img)
        d.ellipse([S(11), S(16), S(17), S(20)], fill=(255, 200, 226, 255))
    else:
        m = Image.new("L", img.size, 0)
        md = ImageDraw.Draw(m)
        md.rounded_rectangle([S(0.5), S(0.5), S(31.5), S(31.5)], radius=S(7), outline=255, width=S(2.6))
        img.paste(PLUM + (255,), (0, 0), grow(m, 0.8))
        img.paste((255, 236, 120, 255), (0, 0), m)
    return reduce(img, 32, 32)


def pictures():
    """The seven pictures, in chapter order, from the level file: (rows, empty-cell colour)."""
    data = json.loads((GAME / "levels/levels.json").read_text(encoding="utf-8"))
    return [(lv["picture"], int(lv.get("bg", "0"), 16)) for lv in data["levels"] if lv.get("mode") == "logic"]


def album_frame(rows, bg):
    """48 x 48: a polaroid with the finished picture (or a question mark when rows is None)."""
    out = np.zeros((48, 48, 4), dtype=np.uint8)
    img = Image.new("RGBA", (S(48), S(48)), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([S(0.5), S(1.5), S(47.5), S(47.5)], radius=S(3), fill=(150, 110, 150, 255))
    d.rounded_rectangle([S(0.5), S(0.5), S(46.5), S(46.5)], radius=S(3), fill=(255, 252, 248, 255))
    out = reduce(img, 48, 48)
    if rows is None:
        out[4:44, 4:44, :3] = (232, 222, 238)
        q = text_pixels("?", 3)
        y0, x0 = 24 - q.shape[0] // 2, 24 - q.shape[1] // 2 + 1
        sel = q > 0
        out[y0:y0 + q.shape[0], x0:x0 + q.shape[1]][sel] = (176, 160, 200, 255)
        return out
    n = len(rows)
    k = 40 // n
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            col = PICTURE[bg if ch == "." else int(ch, 16)]
            out[4 + y * k:4 + (y + 1) * k, 4 + x * k:4 + (x + 1) * k, :3] = col
            out[4 + y * k:4 + (y + 1) * k, 4 + x * k:4 + (x + 1) * k, 3] = 255
    return out


def words_fallback():
    """Six 160 x 32 words without the painted ones: the firmware's letters, doubled."""
    frames = []
    for text in ("NICE!", "GREAT!", "LOVELY!", "AMAZING!", "DONE!", "LUCKY!"):
        t = text_pixels(text, 2)
        f = np.zeros((32, 160, 4), dtype=np.uint8)
        y0, x0 = 8, (160 - t.shape[1]) // 2
        ring = np.zeros((32, 160), dtype=bool)
        ink = np.zeros((32, 160), dtype=bool)
        ink[y0:y0 + 16, x0:x0 + t.shape[1]] = t > 0
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                ring |= np.roll(np.roll(ink, dy, 0), dx, 1)
        f[ring] = PLUM + (255,)
        f[ink] = (255, 150, 200, 255)
        frames.append(f)
    return sheet(frames, 160, 32)


# ---- backgrounds ---------------------------------------------------------
def gradient(w, h, top, bottom):
    t = np.linspace(0, 1, h)[:, None, None]
    g = np.asarray(top)[None, None, :] * (1 - t) + np.asarray(bottom)[None, None, :] * t
    return np.repeat(g, w, axis=1)


def scene(index):
    """320 x 224 without a painting: chapter scenes (0-6), the title (7) and the map (8)."""
    w, h = 320, 224
    themes = [
        ((120, 190, 255), (220, 240, 255), (110, 190, 100)),   # sky garden
        ((40, 90, 140), (150, 220, 230), (90, 170, 200)),      # crystal canal
        ((30, 20, 70), (90, 60, 140), (30, 60, 110)),          # moon lighthouse
        ((120, 60, 30), (240, 170, 90), (110, 70, 40)),        # clockwork tower
        ((10, 50, 110), (40, 150, 190), (220, 140, 120)),      # coral palace
        ((240, 120, 80), (255, 210, 140), (220, 170, 100)),    # sunstone pyramid
        ((8, 10, 40), (40, 50, 110), (60, 60, 100)),           # starlight dome
        ((200, 180, 250), (255, 220, 236), (180, 230, 190)),   # title
        ((210, 190, 140), (240, 225, 180), (200, 170, 120)),   # map (parchment)
    ]
    top, bottom, ground = themes[index]
    base = gradient(w, h, top, bottom)
    img = Image.fromarray(np.dstack((base.astype(np.uint8), np.full((h, w), 255, np.uint8)))).resize((w * SS, h * SS))
    d = ImageDraw.Draw(img)
    rnd = np.random.default_rng(1000 + index)
    if index in (2, 6):
        for _ in range(70):
            x, y = rnd.integers(0, w * SS), rnd.integers(0, int(h * SS * 0.6))
            s = rnd.integers(1, 3) * SS // 2
            d.ellipse([x, y, x + s, y + s], fill=(255, 255, 230, 255))
    if index in (0, 7):
        for _ in range(6):
            x, y = rnd.integers(0, w), rnd.integers(10, 110)
            for k in range(4):
                d.ellipse([S(x + k * 12), S(y - (k % 2) * 6), S(x + k * 12 + 30), S(y + 18 - (k % 2) * 6)], fill=(255, 255, 255, 220))
    ground_y = 168
    d.rectangle([0, S(ground_y), S(w), S(h)], fill=ground + (255,))
    if index == 8:      # the map: a dotted road through seven places
        pts = [(140, 190), (175, 150), (230, 165), (280, 125), (240, 85), (175, 70), (215, 30)]
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            for t in np.linspace(0, 1, 9):
                x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
                d.ellipse([S(x - 1.5), S(y - 1.5), S(x + 1.5), S(y + 1.5)], fill=(120, 80, 50, 255))
        for k, (x, y) in enumerate(pts):
            d.ellipse([S(x - 7), S(y - 7), S(x + 7), S(y + 7)], fill=(120, 80, 50, 255))
            d.ellipse([S(x - 5), S(y - 5), S(x + 5), S(y + 5)], fill=COLORS[k][0] + (255,))
    out = reduce(img, w, h)
    out[:, :, 3] = 255
    return out


def sheet(frames, fw, fh):
    w = fw * len(frames)
    s = np.zeros((fh, w, 4), dtype=np.uint8)
    for i, f in enumerate(frames):
        s[:, i * fw:(i + 1) * fw] = f
    return s


HEART_TILE = 0xE00                 # FIX: full heart, then the empty one
STAR_TILE = 0xE02                  # FIX: a star (gold or grey by palette)


def star_glyph():
    """An 8 x 8 FIX star: 1 fill, 2 outline, 3 shine."""
    rows = ["00011000", "00111100", "11111111", "01111110", "00111100", "01111110", "01100110", "11000011"]
    g = np.array([[1 if ch == "1" else 0 for ch in r] for r in rows], dtype=np.uint8)
    edge = np.zeros_like(g)
    for y in range(8):
        for x in range(8):
            if g[y, x] and (x in (0, 7) or y in (0, 7) or not g[y - 1, x] or not g[y + 1, x] or
                            not g[y, x - 1] or not g[y, x + 1]):
                edge[y, x] = 1
    out = np.where(edge > 0, 2, g)
    out[3, 3] = 3
    return out


def build(fix_only=False):
    pack = ArtPack(prefix="sw", game_id="782", title="SEVEN WONDERS", first_bank=16)
    pack.font_glyphs(HEART_TILE, heart_glyph(1))
    pack.font_glyphs(HEART_TILE + 1, heart_glyph(0))
    pack.font_glyphs(STAR_TILE, star_glyph())
    if fix_only:
        pack.write_fix(GAME)
        print("art: S1 written")
        return
    names = [f"bg{i}" for i in range(7)] + ["title", "map"]
    for i, name in enumerate(names):
        pic = painted(name, 320, 224)
        pack.background(name, pic if pic is not None else scene(i), banks=16, bank=16)
    pack.next_bank = 32
    pack.sprite("pieces", sheet([piece(c, k) for c in range(8) for k in range(4)] + [prism()], 32, 32), frame=(32, 32), banks=12)
    pack.sprite("under", sheet([under(f) for f in range(8)], 32, 32), frame=(32, 32), banks=4)
    pack.sprite("lock", lock(), frame=(32, 32), banks=1)
    pack.sprite("bubbles", sheet([bubble(c, 0) for c in range(8)] + [bubble(8, 0), bubble(9, 0)] + [bubble(c, 1) for c in range(8)], 16, 16), frame=(16, 16), banks=9)
    for name in ("mimi", "kiko"):
        pack.sprite(name, sheet([reduce(cat(name, m), 64, 64) for m in range(6)], 64, 64), frame=(64, 64), banks=3)
    # the small cats beside a picture: the same six moods at 48 x 48, in the big cats' palettes
    for name in ("mimi", "kiko"):
        pack.share("s" + name, sheet([reduce(cat(name, m), 48, 48) for m in range(6)], 48, 48), like=name, frame=(48, 48))
    # the title's big cats: Mimi happy, blinking, cheering; then Kiko the same
    pack.sprite("tcats", sheet([reduce(cat(n, m), 96, 96) for n in ("mimi", "kiko") for m in (2, 1, 5)], 96, 96), frame=(96, 96), banks=4)
    pack.sprite("cursor", sheet([cursor(0), cursor(1)], 32, 32), frame=(32, 32), banks=1)
    pack.sprite("pop", sheet([pop(f) for f in range(4)], 32, 32), frame=(32, 32), banks=1)
    pack.sprite("launcher", launcher(), frame=(32, 32), banks=1)
    pack.sprite("dot", dot(), frame=(16, 16), banks=1)
    pic = painted("logo", 304, 112)
    pack.sprite("logo", pic if pic is not None else logo(), frame=(304, 112), banks=8)
    pack.sprite("star", sheet([star(0), star(1)], 16, 16), frame=(16, 16), banks=1)
    pack.sprite("card", card(), frame=(256, 192), banks=1)
    pack.sprite("banner", banner(), frame=(272, 48), banks=1)
    pack.sprite("lcell", logic_sheet(16), frame=(16, 16), banks=3)
    pack.sprite("lbig", logic_sheet(32), frame=(32, 32), banks=3)
    pack.sprite("lcursor", sheet(logic_cursor(16), 16, 16), frame=(16, 16), banks=1)
    pack.sprite("lbcursor", sheet(logic_cursor(32), 32, 32), frame=(32, 32), banks=1)
    pack.sprite("lplate", sheet([logic_plate(10), logic_plate(5)], 288, 224), frame=(288, 224), banks=2)
    pack.sprite("lpanel", logic_panel(), frame=(80, 224), banks=1)
    words = painted("words", 960, 32)
    pack.sprite("words", words if words is not None else words_fallback(), frame=(160, 32), banks=4)
    pack.sprite("lucky", sheet([reduce(cat("mimi", m), 96, 96) for m in (6, 7)], 96, 96), frame=(96, 96), banks=3)
    pack.sprite("prizes", sheet([prize(k) for k in range(5)], 32, 32), frame=(32, 32), banks=2)
    pack.sprite("album", sheet([album_frame(r, b) for r, b in pictures()] + [album_frame(None, 0)], 48, 48), frame=(48, 48), banks=4)
    if pack.next_bank > 112:
        raise SystemExit(f"palette banks {pack.next_bank} past the faded range (112)")
    manifest = pack.write(GAME)
    print(f"art: {len(manifest)} assets, banks 16-{pack.next_bank - 1}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--fix-only", action="store_true")
    args = ap.parse_args()
    build(args.fix_only)


if __name__ == "__main__":
    main()
