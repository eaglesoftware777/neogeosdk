#!/usr/bin/env python3
"""
Seven Wonders Puzzles art: drawn here, packed with tools/ng_artpack.py.

    python3 games/sevenwonders/tools/art.py            (the build's GAME_ART_BUILDER)
    python3 games/sevenwonders/tools/art.py --fix-only (S1 only)

Every picture is drawn by this script (shapes supersampled four times, then
reduced), so the whole set is original and reproducible. It is the game's
interim art: readable and complete, ahead of a painted pass. Painted sources
placed in artbox/painted/<name>.png (RGBA, the same size) replace the drawn
ones of the same name; artbox/PROVENANCE.md records where each came from.

Pieces have a shape per colour as well as a hue (gem, triangle, star, clover,
drop, moon, heart, flower), so colour-blind players can tell them apart.
"""

import argparse
import math
from pathlib import Path
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from ng_artpack import ArtPack, text_pixels  # noqa: E402

SS = 4                                   # supersampling
PAINTED = GAME / "artbox/painted"

COLORS = [  # (main, light, dark) per piece colour 1..8
    ((226, 46, 62), (255, 150, 150), (130, 16, 36)),     # ruby
    ((246, 140, 30), (255, 212, 130), (150, 70, 10)),    # amber
    ((250, 214, 40), (255, 250, 170), (160, 120, 0)),    # sunstone
    ((60, 190, 80), (170, 250, 160), (16, 100, 40)),     # emerald
    ((40, 200, 230), (180, 250, 255), (10, 100, 150)),   # aquamarine
    ((60, 90, 230), (160, 180, 255), (20, 30, 130)),     # sapphire
    ((160, 70, 220), (220, 170, 255), (80, 20, 130)),    # amethyst
    ((250, 110, 180), (255, 200, 230), (160, 40, 100)),  # rose
]


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


def gem_shape(d, kind, cx, cy, r, main, light, dark):
    """Draw piece shape `kind` (0..7) centred at (cx, cy), radius r (supersampled units)."""
    def poly(points, fill):
        d.polygon([(cx + x, cy + y) for x, y in points], fill=fill)
    out = (30, 20, 40, 255)
    if kind == 0:      # round-cut gem
        d.ellipse([cx - r - 3 * SS, cy - r - 3 * SS, cx + r + 3 * SS, cy + r + 3 * SS], fill=out)
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=dark + (255,))
        d.ellipse([cx - r * 0.86, cy - r * 0.92, cx + r * 0.86, cy + r * 0.7], fill=main + (255,))
        poly([(-r * 0.5, -r * 0.2), (0, -r * 0.65), (r * 0.5, -r * 0.2), (0, r * 0.35)], light + (255,))
    elif kind == 1:    # triangle
        pts = [(0, -r * 1.05), (r * 1.05, r * 0.8), (-r * 1.05, r * 0.8)]
        poly([(x * 1.15, y * 1.15 + SS) for x, y in pts], out)
        poly(pts, dark + (255,))
        poly([(0, -r * 0.8), (r * 0.78, r * 0.62), (-r * 0.78, r * 0.62)], main + (255,))
        poly([(0, -r * 0.62), (r * 0.3, 0), (-r * 0.35, r * 0.1)], light + (255,))
    elif kind == 2:    # star
        pts = []
        for i in range(10):
            a = -math.pi / 2 + i * math.pi / 5
            rr = r * (1.08 if i % 2 == 0 else 0.5)
            pts.append((math.cos(a) * rr, math.sin(a) * rr))
        poly([(x * 1.15, y * 1.15) for x, y in pts], out)
        poly(pts, dark + (255,))
        poly([(x * 0.8, y * 0.8 - SS) for x, y in pts], main + (255,))
        d.ellipse([cx - r * 0.25, cy - r * 0.45, cx + r * 0.1, cy - r * 0.1], fill=light + (255,))
    elif kind == 3:    # clover
        for ox, oy in ((-0.42, -0.2), (0.42, -0.2), (0, -0.62), (0, 0.22)):
            d.ellipse([cx + (ox - 0.5) * r - 2 * SS, cy + (oy - 0.5) * r - 2 * SS, cx + (ox + 0.5) * r + 2 * SS, cy + (oy + 0.5) * r + 2 * SS], fill=out)
        poly([(-r * 0.12, r * 0.4), (r * 0.12, r * 0.4), (r * 0.25, r * 1.0), (-r * 0.05, r * 1.0)], out)
        for ox, oy in ((-0.42, -0.2), (0.42, -0.2), (0, -0.62), (0, 0.22)):
            d.ellipse([cx + (ox - 0.48) * r, cy + (oy - 0.48) * r, cx + (ox + 0.48) * r, cy + (oy + 0.48) * r], fill=main + (255,))
        poly([(-r * 0.06, r * 0.45), (r * 0.06, r * 0.45), (r * 0.18, r * 0.95), (-r * 0.0, r * 0.95)], dark + (255,))
        d.ellipse([cx - r * 0.55, cy - r * 0.95, cx - r * 0.25, cy - r * 0.65], fill=light + (255,))
    elif kind == 4:    # droplet
        pts = [(0, -r * 1.1)]
        for i in range(0, 181, 15):
            a = math.radians(i)
            pts.append((math.cos(a) * r * 0.82 * (1 if i <= 90 else 1), r * 0.25 + math.sin(a) * r * 0.75))
        pts = [(0, -r * 1.1)] + [(math.cos(math.radians(a)) * r * 0.82, r * 0.2 + math.sin(math.radians(a)) * r * 0.78) for a in range(-20, 201, 10)]
        poly([(x * 1.14, y * 1.1) for x, y in pts], out)
        poly(pts, dark + (255,))
        poly([(x * 0.8, y * 0.82 - SS) for x, y in pts], main + (255,))
        d.ellipse([cx - r * 0.42, cy - r * 0.2, cx - r * 0.12, cy + r * 0.25], fill=light + (255,))
    elif kind == 5:    # crescent moon
        d.ellipse([cx - r - 3 * SS, cy - r - 3 * SS, cx + r + 3 * SS, cy + r + 3 * SS], fill=out)
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=main + (255,))
        d.ellipse([cx - r * 0.2, cy - r * 1.05, cx + r * 1.3, cy + r * 0.6], fill=(0, 0, 0, 0))
        d.ellipse([cx - r * 0.85, cy - r * 0.5, cx - r * 0.45, cy - r * 0.1], fill=light + (255,))
    elif kind == 6:    # heart
        def heart(s, dy, fill):
            pts = []
            for i in range(0, 360, 6):
                t = math.radians(i)
                x = 16 * math.sin(t) ** 3
                y = -(13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t))
                pts.append((x * r * s / 16, y * r * s / 16 + dy))
            poly(pts, fill)
        heart(1.12, SS, out)
        heart(1.0, 0, dark + (255,))
        heart(0.8, -SS, main + (255,))
        d.ellipse([cx - r * 0.6, cy - r * 0.6, cx - r * 0.25, cy - r * 0.25], fill=light + (255,))
    else:              # flower
        for i in range(5):
            a = -math.pi / 2 + i * 2 * math.pi / 5
            px, py = math.cos(a) * r * 0.55, math.sin(a) * r * 0.55
            d.ellipse([cx + px - r * 0.5 - 2 * SS, cy + py - r * 0.5 - 2 * SS, cx + px + r * 0.5 + 2 * SS, cy + py + r * 0.5 + 2 * SS], fill=out)
        for i in range(5):
            a = -math.pi / 2 + i * 2 * math.pi / 5
            px, py = math.cos(a) * r * 0.55, math.sin(a) * r * 0.55
            d.ellipse([cx + px - r * 0.47, cy + py - r * 0.47, cx + px + r * 0.47, cy + py + r * 0.47], fill=main + (255,))
        d.ellipse([cx - r * 0.32, cy - r * 0.32, cx + r * 0.32, cy + r * 0.32], fill=(255, 230, 120, 255))
        d.ellipse([cx - r * 0.2, cy - r * 0.9, cx + r * 0.05, cy - r * 0.6], fill=light + (255,))


def piece(color, kind):
    """32 x 32: colour 0..7, kind 0 plain, 1 row stripes, 2 column stripes, 3 bomb."""
    img = canvas(32, 32)
    d = ImageDraw.Draw(img)
    main, light, dark = COLORS[color]
    gem_shape(d, color, S(16), S(16), S(11), main, light, dark)
    if kind in (1, 2):
        arr = np.asarray(img).copy()
        mask = arr[:, :, 3] > 0
        yy, xx = np.mgrid[0:arr.shape[0], 0:arr.shape[1]]
        band = ((yy // (3 * SS)) % 2 == 0) if kind == 1 else ((xx // (3 * SS)) % 2 == 0)
        sel = mask & band & (arr[:, :, 0] + arr[:, :, 1] + arr[:, :, 2] > 120)
        arr[sel, :3] = np.minimum(255, arr[sel, :3].astype(int) + 90)
        img = Image.fromarray(arr)
        d = ImageDraw.Draw(img)
        if kind == 1:
            d.polygon([(S(2), S(16)), (S(7), S(12)), (S(7), S(20))], fill=(255, 255, 255, 255))
            d.polygon([(S(30), S(16)), (S(25), S(12)), (S(25), S(20))], fill=(255, 255, 255, 255))
        else:
            d.polygon([(S(16), S(2)), (S(12), S(7)), (S(20), S(7))], fill=(255, 255, 255, 255))
            d.polygon([(S(16), S(30)), (S(12), S(25)), (S(20), S(25))], fill=(255, 255, 255, 255))
    elif kind == 3:
        d.ellipse([S(3), S(3), S(29), S(29)], outline=(255, 255, 255, 255), width=S(2))
        for a in range(0, 360, 45):
            x, y = 16 + math.cos(math.radians(a)) * 13, 16 + math.sin(math.radians(a)) * 13
            d.ellipse([S(x - 1.5), S(y - 1.5), S(x + 1.5), S(y + 1.5)], fill=(255, 250, 200, 255))
    return reduce(img, 32, 32)


def prism():
    img = canvas(32, 32)
    d = ImageDraw.Draw(img)
    d.ellipse([S(3), S(3), S(29), S(29)], fill=(30, 20, 40, 255))
    for i, c in enumerate(COLORS):
        a0 = i * 45
        d.pieslice([S(5), S(5), S(27), S(27)], a0, a0 + 45, fill=c[0] + (255,))
    d.ellipse([S(11), S(11), S(21), S(21)], fill=(255, 255, 255, 255))
    d.ellipse([S(8), S(7), S(13), S(11)], fill=(255, 255, 255, 255))
    return reduce(img, 32, 32)


def under(frame):
    """32 x 32 cell backs: 0 ice, 1 thick ice, 2-4 crate 1-3, 5 wall, 6 slot."""
    img = canvas(32, 32)
    d = ImageDraw.Draw(img)
    box = [S(1), S(1), S(31), S(31)]
    if frame == 6:
        d.rounded_rectangle(box, radius=S(6), fill=(255, 255, 255, 255))
        arr = np.asarray(img).copy()
        arr[arr[:, :, 3] > 0] = (40, 34, 70, 255)
        img = Image.fromarray(arr)
        d = ImageDraw.Draw(img)
        d.rounded_rectangle([S(3), S(3), S(29), S(29)], radius=S(5), fill=(56, 50, 96, 255))
    elif frame in (0, 1):
        tone = (150, 220, 255) if frame == 0 else (110, 180, 245)
        d.rounded_rectangle(box, radius=S(5), fill=tone + (255,))
        d.rounded_rectangle([S(3), S(3), S(29), S(29)], radius=S(4), fill=(200, 240, 255, 255) if frame == 0 else (160, 210, 250, 255))
        for k in range(3 + frame * 2):
            x0 = 5 + 6 * k
            d.line([S(x0), S(5), S(x0 + 6), S(27)], fill=(255, 255, 255, 255), width=S(1))
    elif frame in (2, 3, 4):
        wood = [(190, 130, 70), (160, 100, 50), (120, 75, 40)][frame - 2]
        d.rectangle(box, fill=(70, 40, 20, 255))
        d.rectangle([S(3), S(3), S(29), S(29)], fill=wood + (255,))
        d.line([S(3), S(3), S(29), S(29)], fill=(90, 55, 25, 255), width=S(3))
        d.line([S(29), S(3), S(3), S(29)], fill=(90, 55, 25, 255), width=S(3))
        for k in range(frame - 1):
            d.rectangle([S(5 + 9 * k), S(5), S(10 + 9 * k), S(8)], fill=(230, 210, 120, 255))
    else:
        d.rectangle(box, fill=(60, 56, 70, 255))
        for y in (1, 11, 21):
            d.rectangle([S(1), S(y), S(31), S(y + 9)], outline=(30, 28, 36, 255), width=S(1))
        d.rectangle([S(2), S(2), S(30), S(4)], fill=(100, 96, 110, 255))
    return reduce(img, 32, 32)


def lock():
    img = canvas(32, 32)
    d = ImageDraw.Draw(img)
    for k in range(4):
        x = 4 + k * 7
        d.ellipse([S(x), S(13), S(x + 8), S(19)], outline=(70, 70, 80, 255), width=S(2))
        d.ellipse([S(x + 1), S(14), S(x + 7), S(18)], outline=(200, 200, 215, 255), width=S(1))
    d.rounded_rectangle([S(11), S(9), S(21), S(23)], radius=S(2), fill=(80, 60, 30, 255))
    d.rounded_rectangle([S(12), S(10), S(20), S(22)], radius=S(2), fill=(230, 190, 70, 255))
    d.ellipse([S(14), S(13), S(18), S(17)], fill=(80, 60, 30, 255))
    return reduce(img, 32, 32)


def bubble(color, kind):
    """16 x 16: 0-7 colours, 8 stone, 9 anchor; kind 1 holds a kitten."""
    img = canvas(16, 16)
    d = ImageDraw.Draw(img)
    if color == 8:
        d.ellipse([S(0.5), S(0.5), S(15.5), S(15.5)], fill=(50, 48, 56, 255))
        d.ellipse([S(1.5), S(1.5), S(14.5), S(14.5)], fill=(120, 116, 128, 255))
        d.ellipse([S(4), S(4), S(8), S(7)], fill=(170, 166, 178, 255))
        return reduce(img, 16, 16)
    if color == 9:
        d.ellipse([S(0.5), S(0.5), S(15.5), S(15.5)], fill=(60, 40, 20, 255))
        d.ellipse([S(1.5), S(1.5), S(14.5), S(14.5)], fill=(200, 160, 60, 255))
        d.line([S(8), S(3), S(8), S(13)], fill=(60, 40, 20, 255), width=S(2))
        d.arc([S(3), S(5), S(13), S(14)], 20, 160, fill=(60, 40, 20, 255), width=S(2))
        d.ellipse([S(6), S(2), S(10), S(5)], outline=(60, 40, 20, 255), width=S(1))
        return reduce(img, 16, 16)
    main, light, dark = COLORS[color]
    d.ellipse([S(0.5), S(0.5), S(15.5), S(15.5)], fill=(30, 20, 40, 255))
    d.ellipse([S(1.2), S(1.2), S(14.8), S(14.8)], fill=dark + (255,))
    d.ellipse([S(1.6), S(1.4), S(13.6), S(12.8)], fill=main + (255,))
    if kind:
        # a kitten's face looking out
        d.ellipse([S(4.5), S(5.5), S(11.5), S(12)], fill=(250, 245, 235, 255))
        d.polygon([(S(4.6), S(7)), (S(5.2), S(4)), (S(7.2), S(6))], fill=(250, 245, 235, 255))
        d.polygon([(S(11.4), S(7)), (S(10.8), S(4)), (S(8.8), S(6))], fill=(250, 245, 235, 255))
        d.ellipse([S(6), S(7.6), S(7.2), S(8.8)], fill=(20, 20, 30, 255))
        d.ellipse([S(8.8), S(7.6), S(10), S(8.8)], fill=(20, 20, 30, 255))
        d.polygon([(S(7.6), S(9.6)), (S(8.4), S(9.6)), (S(8), S(10.2))], fill=(240, 120, 140, 255))
    else:
        mark = light + (255,)
        gem_shape(d, color, S(8), S(8.4), S(3.6), light, (255, 255, 255), main)
    d.ellipse([S(3.5), S(2.6), S(6.8), S(5.2)], fill=(255, 255, 255, 255))
    return reduce(img, 16, 16)


def cat(name, mood):
    """64 x 64 cat bust. Mimi: an orange tabby; Kiko: a black and white cat."""
    img = canvas(64, 64)
    d = ImageDraw.Draw(img)
    if name == "mimi":
        fur, fur_d, belly, eye = (245, 160, 60), (190, 100, 30), (255, 230, 190), (90, 200, 90)
    else:
        fur, fur_d, belly, eye = (40, 40, 52), (16, 16, 24), (245, 245, 250), (250, 210, 60)
    out = (30, 20, 24, 255)
    # body and paws
    d.ellipse([S(12), S(40), S(52), S(72)], fill=out)
    d.ellipse([S(14), S(42), S(50), S(72)], fill=fur + (255,))
    d.ellipse([S(22), S(46), S(42), S(66)], fill=belly + (255,))
    if mood == 5:  # cheer: paws up
        for px in (8, 46):
            d.ellipse([S(px), S(18), S(px + 11), S(30)], fill=out)
            d.ellipse([S(px + 1), S(19), S(px + 10), S(29)], fill=belly + (255,))
    # ears
    ear_drop = 4 if mood == 4 else 0
    for sx in (1, -1):
        base = 32 + sx * 13
        tip = (32 + sx * 19, 7 + ear_drop)
        d.polygon([(S(base - 7), S(20)), (S(tip[0]), S(tip[1])), (S(base + 7 * (1 if sx < 0 else -1) + sx * 2), S(16))], fill=out)
        d.polygon([(S(base - 5), S(19)), (S(tip[0] - sx * 1), S(tip[1] + 3)), (S(base + 5 * (1 if sx < 0 else -1) + sx), S(17))], fill=fur + (255,))
        d.polygon([(S(base - 2), S(18)), (S(tip[0] - sx * 2), S(tip[1] + 6)), (S(base + 2 * (1 if sx < 0 else -1)), S(17))], fill=(250, 170, 180, 255))
    # head
    d.ellipse([S(9), S(11), S(55), S(49)], fill=out)
    d.ellipse([S(11), S(13), S(53), S(47)], fill=fur + (255,))
    if name == "mimi":
        for k in (-6, 0, 6):
            d.line([S(32 + k), S(14), S(32 + k * 0.6), S(21)], fill=fur_d + (255,), width=S(2))
    else:
        d.polygon([(S(32), S(19)), (S(22), S(47)), (S(42), S(47))], fill=belly + (255,))
    d.ellipse([S(20), S(30), S(44), S(47)], fill=belly + (255,))
    # eyes
    for ex in (23, 41):
        if mood == 1:                      # blink
            d.line([S(ex - 5), S(28), S(ex + 5), S(28)], fill=out, width=S(2))
        elif mood in (2, 5):               # happy: arcs
            d.arc([S(ex - 5), S(24), S(ex + 5), S(32)], 200, 340, fill=out, width=S(2))
        elif mood == 3:                    # wow
            d.ellipse([S(ex - 6), S(21), S(ex + 6), S(34)], fill=(255, 255, 255, 255), outline=out, width=S(1))
            d.ellipse([S(ex - 3), S(24), S(ex + 3), S(31)], fill=(20, 20, 30, 255))
            d.ellipse([S(ex - 2), S(24), S(ex), S(26)], fill=(255, 255, 255, 255))
        else:                              # idle, sad
            d.ellipse([S(ex - 5), S(22), S(ex + 5), S(33)], fill=eye + (255,), outline=out, width=S(1))
            d.ellipse([S(ex - 2), S(23), S(ex + 2), S(32)], fill=(20, 20, 30, 255))
            d.ellipse([S(ex - 1), S(24), S(ex + 1), S(26)], fill=(255, 255, 255, 255))
            if mood == 4:
                d.line([S(ex - 6), S(21 if ex < 32 else 23), S(ex + 6), S(23 if ex < 32 else 21)], fill=out, width=S(2))
    # nose, mouth, whiskers
    d.polygon([(S(30), S(35)), (S(34), S(35)), (S(32), S(38))], fill=(240, 120, 140, 255))
    if mood in (2, 5):
        d.chord([S(26), S(37), S(38), S(46)], 0, 180, fill=(170, 40, 60, 255))
    elif mood == 3:
        d.ellipse([S(29.5), S(39), S(34.5), S(44)], fill=(170, 40, 60, 255))
    elif mood == 4:
        d.arc([S(27), S(40), S(37), S(46)], 200, 340, fill=out, width=S(1))
    else:
        d.arc([S(26), S(35), S(32), S(41)], 0, 160, fill=out, width=S(1))
        d.arc([S(32), S(35), S(38), S(41)], 20, 180, fill=out, width=S(1))
    for sy in (37, 40):
        d.line([S(10), S(sy - 2), S(22), S(sy)], fill=(250, 250, 250, 255), width=S(1))
        d.line([S(54), S(sy - 2), S(42), S(sy)], fill=(250, 250, 250, 255), width=S(1))
    if mood == 5:
        for sx, sy in ((6, 8), (56, 10), (4, 40), (58, 38)):
            d.polygon([(S(sx), S(sy - 3)), (S(sx + 1), S(sy)), (S(sx), S(sy + 3)), (S(sx - 1), S(sy))], fill=(255, 240, 120, 255))
    return reduce(img, 64, 64)


def cursor(frame):
    img = canvas(32, 32)
    d = ImageDraw.Draw(img)
    c = (255, 255, 255, 255) if frame == 0 else (255, 220, 80, 255)
    w = S(2)
    for x0, y0, dx, dy in ((1, 1, 1, 1), (31, 1, -1, 1), (1, 31, 1, -1), (31, 31, -1, -1)):
        d.line([S(x0), S(y0), S(x0 + 9 * dx), S(y0)], fill=c, width=w)
        d.line([S(x0), S(y0), S(x0), S(y0 + 9 * dy)], fill=c, width=w)
    if frame == 1:
        d.rounded_rectangle([S(2), S(2), S(30), S(30)], radius=S(6), outline=(255, 200, 60, 255), width=S(1))
    return reduce(img, 32, 32)


def pop(frame):
    img = canvas(32, 32)
    d = ImageDraw.Draw(img)
    r = 4 + frame * 3
    for a in range(0, 360, 45):
        x, y = 16 + math.cos(math.radians(a + frame * 10)) * r, 16 + math.sin(math.radians(a + frame * 10)) * r
        s = max(1.0, 3.5 - frame * 0.7)
        d.polygon([(S(x), S(y - s)), (S(x + s * 0.4), S(y)), (S(x), S(y + s)), (S(x - s * 0.4), S(y))], fill=(255, 250, 200, 255))
    if frame < 2:
        d.ellipse([S(16 - r * 0.6), S(16 - r * 0.6), S(16 + r * 0.6), S(16 + r * 0.6)], outline=(255, 255, 255, 255), width=S(1))
    return reduce(img, 32, 32)


def launcher():
    img = canvas(32, 32)
    d = ImageDraw.Draw(img)
    d.ellipse([S(4), S(10), S(28), S(30)], fill=(40, 30, 50, 255))
    d.ellipse([S(6), S(12), S(26), S(28)], fill=(180, 120, 220, 255))
    d.ellipse([S(9), S(14), S(23), S(24)], fill=(230, 200, 255, 255))
    d.ellipse([S(12), S(15), S(20), S(21)], fill=(255, 255, 255, 255))
    return reduce(img, 32, 32)


def dot():
    img = canvas(16, 16)
    d = ImageDraw.Draw(img)
    d.ellipse([S(5.5), S(5.5), S(10.5), S(10.5)], fill=(40, 30, 50, 255))
    d.ellipse([S(6.5), S(6.5), S(9.5), S(9.5)], fill=(255, 255, 255, 255))
    return reduce(img, 16, 16)


def star(full):
    img = canvas(16, 16)
    d = ImageDraw.Draw(img)
    pts = []
    for i in range(10):
        a = -math.pi / 2 + i * math.pi / 5
        rr = 7.4 if i % 2 == 0 else 3.2
        pts.append((S(8 + math.cos(a) * rr), S(8.4 + math.sin(a) * rr)))
    d.polygon(pts, fill=(40, 30, 20, 255))
    inner = [(S(8) + (x - S(8)) * 0.78, S(8.4) + (y - S(8.4)) * 0.78) for x, y in pts]
    d.polygon(inner, fill=(255, 210, 60, 255) if full else (90, 80, 100, 255))
    return reduce(img, 16, 16)


def logo():
    """256 x 64: the name in the firmware's letters, outlined, with a warm gradient."""
    w, h = 256, 64
    top = text_pixels("SEVEN", 4)          # 32 tall
    bot = text_pixels("WONDERS", 3)        # 24 tall
    mask = np.zeros((h, w), dtype=bool)
    mask[0:32, (w - top.shape[1]) // 2:(w - top.shape[1]) // 2 + top.shape[1]] = top > 0
    mask[36:60, (w - bot.shape[1]) // 2:(w - bot.shape[1]) // 2 + bot.shape[1]] = bot > 0
    img = Image.fromarray((mask * 255).astype(np.uint8)).resize((w * SS, h * SS), Image.NEAREST)
    soft = img.filter(ImageFilter.GaussianBlur(SS * 0.6))
    big = np.asarray(soft) > 100
    outline = np.asarray(soft.filter(ImageFilter.MaxFilter(SS * 2 + 1))) > 60
    rgba = np.zeros((h * SS, w * SS, 4), dtype=np.uint8)
    rgba[outline] = (60, 20, 80, 255)
    yy = np.linspace(0, 1, h * SS)[:, None].repeat(w * SS, 1)
    grad = np.stack([255 - yy * 20, 220 - yy * 120, 80 + yy * 100], axis=2).astype(np.uint8)
    rgba[big, :3] = grad[big]
    rgba[big, 3] = 255
    return reduce(Image.fromarray(rgba), w, h)


# ---- backgrounds ---------------------------------------------------------
def gradient(w, h, top, bottom):
    t = np.linspace(0, 1, h)[:, None, None]
    g = np.asarray(top)[None, None, :] * (1 - t) + np.asarray(bottom)[None, None, :] * t
    return np.repeat(g, w, axis=1)


def scene(index):
    """320 x 224: chapter scenes (0-6), the title (7) and the map (8)."""
    w, h = 320, 224
    themes = [
        ((120, 190, 255), (220, 240, 255), (110, 190, 100)),   # sky garden
        ((40, 90, 140), (150, 220, 230), (90, 170, 200)),      # crystal canal
        ((30, 20, 70), (90, 60, 140), (30, 60, 110)),          # moon lighthouse
        ((120, 60, 30), (240, 170, 90), (110, 70, 40)),        # clockwork tower
        ((10, 50, 110), (40, 150, 190), (220, 140, 120)),      # coral palace
        ((240, 120, 80), (255, 210, 140), (220, 170, 100)),    # sunstone pyramid
        ((8, 10, 40), (40, 50, 110), (60, 60, 100)),           # starlight dome
        ((140, 200, 255), (255, 230, 245), (130, 200, 120)),   # title
        ((210, 190, 140), (240, 225, 180), (200, 170, 120)),   # map (parchment)
    ]
    top, bottom, ground = themes[index]
    base = gradient(w * SS // SS, h, top, bottom)
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
    if index == 0:      # floating islands with flowers
        for ix, iy, iw in ((150, 60, 90), (250, 100, 70), (190, 140, 60)):
            d.ellipse([S(ix), S(iy), S(ix + iw), S(iy + 22)], fill=(120, 90, 60, 255))
            d.ellipse([S(ix), S(iy - 6), S(ix + iw), S(iy + 12)], fill=(100, 190, 90, 255))
            for k in range(5):
                fx = ix + 8 + k * (iw - 16) // 4
                d.ellipse([S(fx - 3), S(iy - 10), S(fx + 3), S(iy - 4)], fill=COLORS[k % 8][0] + (255,))
            d.line([S(ix + iw // 2), S(iy + 20), S(ix + iw // 2 + 6), S(iy + 50)], fill=(80, 150, 70, 255), width=S(2))
    elif index == 1:    # crystal spires over a canal
        for k in range(7):
            x = 120 + k * 30
            hgt = 60 + (k * 37) % 70
            d.polygon([(S(x), S(ground_y)), (S(x + 10), S(ground_y - hgt)), (S(x + 20), S(ground_y))], fill=(170, 230, 245, 255))
            d.polygon([(S(x + 10), S(ground_y)), (S(x + 10), S(ground_y - hgt)), (S(x + 20), S(ground_y))], fill=(120, 190, 220, 255))
        d.rectangle([0, S(ground_y + 18), S(w), S(ground_y + 34)], fill=(60, 140, 190, 255))
    elif index == 2:    # moon and lighthouse
        d.ellipse([S(240), S(20), S(290), S(70)], fill=(250, 245, 210, 255))
        d.polygon([(S(180), S(ground_y)), (S(192), S(60)), (S(212), S(60)), (S(224), S(ground_y))], fill=(230, 230, 240, 255))
        for y in (90, 120, 150):
            d.rectangle([S(184), S(y), S(220), S(y + 8)], fill=(200, 60, 70, 255))
        d.rectangle([S(188), S(44), S(216), S(60)], fill=(250, 220, 120, 255))
        d.polygon([(S(202), S(52)), (S(320), S(30)), (S(320), S(80))], fill=(250, 240, 180, 120))
    elif index == 3:    # clockwork tower
        d.rectangle([S(190), S(30), S(240), S(ground_y)], fill=(150, 100, 60, 255))
        d.ellipse([S(195), S(40), S(235), S(80)], fill=(240, 220, 160, 255), outline=(90, 60, 30, 255), width=S(2))
        d.line([S(215), S(60), S(215), S(46)], fill=(60, 40, 20, 255), width=S(2))
        d.line([S(215), S(60), S(226), S(60)], fill=(60, 40, 20, 255), width=S(2))
        for gx, gy, gr in ((140, 120, 26), (285, 110, 20), (265, 150, 14)):
            for a in range(0, 360, 30):
                x, y = gx + math.cos(math.radians(a)) * gr, gy + math.sin(math.radians(a)) * gr
                d.rectangle([S(x - 3), S(y - 3), S(x + 3), S(y + 3)], fill=(190, 140, 70, 255))
            d.ellipse([S(gx - gr + 3), S(gy - gr + 3), S(gx + gr - 3), S(gy + gr - 3)], fill=(190, 140, 70, 255))
            d.ellipse([S(gx - 5), S(gy - 5), S(gx + 5), S(gy + 5)], fill=(120, 80, 40, 255))
    elif index == 4:    # coral palace
        for k in range(3):
            x = 150 + k * 50
            d.rectangle([S(x), S(90), S(x + 36), S(ground_y)], fill=(240, 220, 230, 255))
            d.ellipse([S(x - 4), S(66), S(x + 40), S(110)], fill=(250, 170, 190, 255))
        for k in range(9):
            x = 110 + k * 24
            d.line([S(x), S(ground_y), S(x - 6), S(ground_y - 30)], fill=(255, 120, 110, 255), width=S(3))
            d.line([S(x), S(ground_y), S(x + 8), S(ground_y - 22)], fill=(255, 150, 90, 255), width=S(3))
        for _ in range(20):
            x, y = rnd.integers(110, 320), rnd.integers(10, 160)
            d.ellipse([S(x), S(y), S(x + 5), S(y + 5)], outline=(220, 250, 255, 255), width=S(1))
    elif index == 5:    # sunstone pyramid at sunset
        d.ellipse([S(230), S(40), S(290), S(100)], fill=(255, 240, 160, 255))
        d.polygon([(S(130), S(ground_y)), (S(215), S(50)), (S(300), S(ground_y))], fill=(230, 170, 90, 255))
        d.polygon([(S(215), S(50)), (S(300), S(ground_y)), (S(215), S(ground_y))], fill=(190, 130, 70, 255))
        d.polygon([(S(205), S(64)), (S(215), S(50)), (S(225), S(64))], fill=(255, 220, 80, 255))
    elif index == 6:    # starlight dome
        d.ellipse([S(170), S(70), S(270), S(170)], fill=(200, 200, 220, 255))
        d.rectangle([S(170), S(120), S(270), S(ground_y)], fill=(170, 170, 200, 255))
        d.rectangle([S(212), S(70), S(228), S(120)], fill=(40, 40, 80, 255))
        d.line([S(220), S(95), S(260), S(50)], fill=(230, 230, 250, 255), width=S(5))
    elif index == 7:    # title: a hint of every wonder on the horizon
        for k in range(7):
            x = 30 + k * 42
            hgt = 30 + (k * 17) % 30
            d.polygon([(S(x), S(ground_y)), (S(x + 16), S(ground_y - hgt)), (S(x + 32), S(ground_y))], fill=(255, 255, 255, 140))
    elif index == 8:    # the map: a dotted road through seven places
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


def panel():
    """The left panel in play: a dark plate behind the FIX text (a sprite, so cards show the whole scene)."""
    img = Image.new("RGBA", (S(96), S(224)), (0, 0, 0, 0))     # 216 tall, in a 224 sprite
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([S(0.5), S(0.5), S(95.5), S(215.5)], radius=S(8), fill=(18, 14, 36, 255), outline=(255, 230, 160, 255), width=S(1))
    d.rounded_rectangle([S(3), S(3), S(93), S(213)], radius=S(6), outline=(70, 56, 110, 255), width=S(1))
    return reduce(img, 96, 224)


def banner():
    """A strip plate for a few lines of text over a scene."""
    img = Image.new("RGBA", (S(288), S(48)), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([S(1), S(1), S(287), S(47)], radius=S(8), fill=(22, 16, 40, 255), outline=(255, 230, 160, 255), width=S(1.5))
    return reduce(img, 288, 48)


def card():
    """The plate behind a card's text: the side panel's look, centred and larger."""
    img = Image.new("RGBA", (S(256), S(192)), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([S(1), S(1), S(255), S(191)], radius=S(10), fill=(22, 16, 40, 255), outline=(255, 230, 160, 255), width=S(1.5))
    d.rounded_rectangle([S(5), S(5), S(251), S(187)], radius=S(8), outline=(120, 90, 150, 255), width=S(1))
    return reduce(img, 256, 192)


def sheet(frames, fw, fh):
    w = fw * len(frames)
    s = np.zeros((fh, w, 4), dtype=np.uint8)
    for i, f in enumerate(frames):
        s[:, i * fw:(i + 1) * fw] = f
    return s


def build(fix_only=False):
    pack = ArtPack(prefix="sw", game_id="782", title="SEVEN WONDERS", first_bank=16)
    if not fix_only:
        names = [f"bg{i}" for i in range(7)] + ["title", "map"]
        for i, name in enumerate(names):
            pic = painted(name, 320, 224)
            pack.background(name, pic if pic is not None else scene(i), banks=16, bank=16)
        pack.next_bank = 32
        pack.sprite("pieces", sheet([piece(c, k) for c in range(8) for k in range(4)] + [prism()], 32, 32), frame=(32, 32), banks=10)
        pack.sprite("under", sheet([under(f) for f in range(7)], 32, 32), frame=(32, 32), banks=4)
        pack.sprite("lock", lock(), frame=(32, 32), banks=1)
        pack.sprite("bubbles", sheet([bubble(c, 0) for c in range(8)] + [bubble(8, 0), bubble(9, 0)] + [bubble(c, 1) for c in range(8)], 16, 16), frame=(16, 16), banks=9)
        for name in ("mimi", "kiko"):
            pic = painted(name, 64 * 6, 64)
            pack.sprite(name, pic if pic is not None else sheet([cat(name, m) for m in range(6)], 64, 64), frame=(64, 64), banks=3)
        pack.sprite("cursor", sheet([cursor(0), cursor(1)], 32, 32), frame=(32, 32), banks=1)
        pack.sprite("pop", sheet([pop(f) for f in range(4)], 32, 32), frame=(32, 32), banks=1)
        pack.sprite("launcher", launcher(), frame=(32, 32), banks=1)
        pack.sprite("dot", dot(), frame=(16, 16), banks=1)
        pic = painted("logo", 256, 64)
        pack.sprite("logo", pic if pic is not None else logo(), frame=(256, 64), banks=3)
        pack.sprite("star", sheet([star(0), star(1)], 16, 16), frame=(16, 16), banks=1)
        pack.sprite("card", card(), frame=(256, 192), banks=1)
        pack.sprite("panel", panel(), frame=(96, 224), banks=1)
        pack.sprite("banner", banner(), frame=(288, 48), banks=1)
        if pack.next_bank > 112:
            raise SystemExit(f"palette banks {pack.next_bank} past the faded range (112)")
    if fix_only:
        pack.write_fix(GAME)
        print("art: S1 written")
        return
    manifest = pack.write(GAME)
    print(f"art: {len(manifest)} assets, banks 16-{pack.next_bank - 1}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--fix-only", action="store_true")
    args = ap.parse_args()
    build(args.fix_only)


if __name__ == "__main__":
    main()
