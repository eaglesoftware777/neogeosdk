"""The 48 parody flags, drawn small: 30 x 20 inside a 32 x 32 frame. Each keeps its
nation's colours and layout; where the real flag has an emblem there is a cute
symbol instead (the names are in tools/nations.py)."""
import numpy as np
from PIL import Image, ImageDraw

W, H = 30, 20
R, B, G, Y, WH, K = (206, 17, 38), (0, 56, 147), (0, 122, 61), (252, 209, 22), (248, 248, 248), (18, 18, 22)


def v3(a, b, c):
    def f(d):
        d.rectangle((0, 0, 9, H), fill=a); d.rectangle((10, 0, 19, H), fill=b); d.rectangle((20, 0, W, H), fill=c)
    return f


def h3(a, b, c, split=(7, 13)):
    def f(d):
        d.rectangle((0, 0, W, split[0] - 1), fill=a); d.rectangle((0, split[0], W, split[1] - 1), fill=b); d.rectangle((0, split[1], W, H), fill=c)
    return f


def star(d, cx, cy, r, col):
    pts = []
    for i in range(10):
        a = -np.pi / 2 + i * np.pi / 5
        rr = r if i % 2 == 0 else r * 0.42
        pts.append((cx + rr * np.cos(a), cy + rr * np.sin(a)))
    d.polygon(pts, fill=col)


def nordic(bg, cross, inner=None):
    def f(d):
        d.rectangle((0, 0, W, H), fill=bg)
        d.rectangle((8, 0, 13, H), fill=cross); d.rectangle((0, 7, W, 12), fill=cross)
        if inner:
            d.rectangle((10, 0, 11, H), fill=inner); d.rectangle((0, 9, W, 10), fill=inner)
    return f


# ---------------------------------------------------------------- the symbols
# Each parody nation keeps its colours and layout; where the real flag has an emblem,
# a cute symbol stands instead (tools/nations.py names the nation and the symbol).
BR = (150, 92, 44)          # browns: bread, wood, chocolate
BRL = (214, 150, 74)
PK = (240, 120, 150)


def banana(d, cx, cy):
    d.arc((cx - 7, cy - 9, cx + 5, cy + 5), 20, 160, fill=Y, width=3)
    d.point((cx + 4, cy - 2), fill=BR); d.point((cx - 6, cy - 2), fill=BR)


def sunface(d, cx, cy, col=(246, 180, 14)):
    d.ellipse((cx - 4, cy - 4, cx + 4, cy + 4), fill=col)
    for dx, dy in ((-6, 0), (6, 0), (0, -6), (0, 6)):
        d.point((cx + dx, cy + dy), fill=col)
    d.rectangle((cx - 3, cy - 1, cx + 3, cy - 1), fill=K)        # the sunglasses
    d.line((cx - 1, cy + 2, cx + 1, cy + 2), fill=K)


def guava(d, cx, cy):
    d.ellipse((cx - 4, cy - 4, cx + 4, cy + 4), fill=(110, 190, 70))
    d.ellipse((cx - 2, cy - 2, cx + 2, cy + 2), fill=PK)
    d.point((cx + 1, cy - 5), fill=(0, 98, 51))


def coffee(d, cx, cy):
    d.rectangle((cx - 4, cy - 2, cx + 3, cy + 4), fill=WH)
    d.rectangle((cx - 3, cy - 1, cx + 2, cy), fill=BR)
    d.arc((cx + 2, cy - 1, cx + 6, cy + 3), 270, 90, fill=WH)
    d.line((cx - 2, cy - 5, cx - 1, cy - 3), fill=WH); d.line((cx + 1, cy - 5, cx + 2, cy - 3), fill=WH)


def taco(d, cx, cy):
    d.pieslice((cx - 6, cy - 4, cx + 6, cy + 8), 180, 360, fill=(240, 196, 70))
    d.rectangle((cx - 4, cy - 1, cx + 4, cy), fill=(70, 170, 60))
    d.point((cx - 2, cy - 2), fill=R); d.point((cx + 2, cy - 2), fill=R)


def burger(d, cx, cy):
    d.pieslice((cx - 5, cy - 4, cx + 5, cy + 2), 180, 360, fill=BRL)
    d.rectangle((cx - 5, cy - 1, cx + 5, cy - 1), fill=(70, 170, 60))
    d.rectangle((cx - 5, cy, cx + 5, cy + 1), fill=BR)
    d.rectangle((cx - 5, cy + 2, cx + 5, cy + 3), fill=BRL)


def pancakes(d, cx, cy):
    for k in range(3):
        d.ellipse((cx - 5, cy + 1 - k * 2, cx + 5, cy + 4 - k * 2), fill=BRL, outline=BR)
    d.rectangle((cx - 1, cy - 5, cx + 1, cy - 4), fill=Y)                  # the butter
    d.line((cx + 3, cy - 3, cx + 4, cy + 2), fill=BR)                      # the syrup


def frog(d, cx, cy):
    g = (60, 190, 70)
    d.ellipse((cx - 5, cy - 2, cx + 5, cy + 4), fill=g)
    d.ellipse((cx - 5, cy - 5, cx - 1, cy - 1), fill=g); d.ellipse((cx + 1, cy - 5, cx + 5, cy - 1), fill=g)
    d.point((cx - 3, cy - 3), fill=R); d.point((cx + 3, cy - 3), fill=R)
    d.line((cx - 2, cy + 2, cx + 2, cy + 2), fill=K)


def baguette(d, cx, cy):
    d.line((cx - 8, cy + 3, cx + 8, cy - 3), fill=BRL, width=3)
    for k in (-4, 0, 4):
        d.point((cx + k, cy - k * 3 // 8), fill=BR)


def sausage(d, cx, cy):
    d.arc((cx - 8, cy - 6, cx + 8, cy + 6), 200, 340, fill=(180, 80, 50), width=3)
    d.point((cx - 2, cy - 5), fill=Y); d.point((cx + 2, cy - 5), fill=Y)


def pizza(d, cx, cy):
    d.polygon([(cx - 5, cy - 5), (cx + 5, cy - 5), (cx, cy + 6)], fill=(250, 210, 90))
    d.rectangle((cx - 5, cy - 5, cx + 5, cy - 4), fill=BRL)
    d.point((cx - 2, cy - 2), fill=R); d.point((cx + 2, cy - 1), fill=R); d.point((cx, cy + 2), fill=R)


def paella(d, cx, cy):
    d.ellipse((cx - 6, cy - 3, cx + 6, cy + 4), fill=(40, 40, 46))
    d.ellipse((cx - 5, cy - 2, cx + 5, cy + 3), fill=(250, 200, 40))
    d.point((cx - 2, cy), fill=R); d.point((cx + 2, cy - 1), fill=(70, 170, 60)); d.point((cx + 1, cy + 1), fill=R)


def teacup(d, cx, cy):
    blue = (0, 56, 147)
    d.pieslice((cx - 5, cy - 4, cx + 4, cy + 5), 0, 180, fill=WH, outline=blue)
    d.line((cx - 5, cy, cx + 4, cy), fill=blue)
    d.arc((cx + 3, cy, cx + 7, cy + 4), 270, 90, fill=blue)
    d.line((cx - 1, cy - 4, cx, cy - 2), fill=(200, 200, 210))


def fish(d, cx, cy):
    d.ellipse((cx - 6, cy - 3, cx + 3, cy + 3), fill=(200, 200, 210))
    d.polygon([(cx + 3, cy), (cx + 7, cy - 3), (cx + 7, cy + 3)], fill=(200, 200, 210))
    d.point((cx - 4, cy - 1), fill=K)


def clog(d, cx, cy):
    d.polygon([(cx - 7, cy + 3), (cx - 6, cy - 1), (cx + 1, cy - 3), (cx + 6, cy - 1), (cx + 7, cy + 3)], fill=(250, 220, 120))
    d.ellipse((cx - 1, cy - 3, cx + 4, cy), fill=BR)


def waffle(d, cx, cy):
    d.rectangle((cx - 5, cy - 4, cx + 5, cy + 4), fill=BRL)
    for k in (-3, 0, 3):
        d.line((cx + k, cy - 4, cx + k, cy + 4), fill=BR)
        d.line((cx - 5, cy + k, cx + 5, cy + k), fill=BR)


def necktie(d, cx, cy):
    d.polygon([(cx - 2, cy - 6), (cx + 2, cy - 6), (cx + 1, cy - 4), (cx - 1, cy - 4)], fill=R)
    d.polygon([(cx - 1, cy - 4), (cx + 1, cy - 4), (cx + 3, cy + 4), (cx, cy + 7), (cx - 3, cy + 4)], fill=R)
    d.line((cx - 1, cy - 1, cx + 1, cy - 2), fill=WH); d.line((cx - 2, cy + 2, cx + 2, cy + 1), fill=WH)


def meatball(d, cx, cy):
    d.ellipse((cx - 5, cy - 4, cx + 5, cy + 5), fill=(140, 70, 40))
    d.point((cx - 2, cy - 2), fill=BRL); d.point((cx + 1, cy - 1), fill=BRL)
    d.ellipse((cx + 3, cy - 6, cx + 6, cy - 3), fill=(200, 30, 50))      # a lingonberry on top


def bat(d, cx, cy):
    d.polygon([(cx - 8, cy - 3), (cx - 2, cy - 1), (cx, cy - 3), (cx + 2, cy - 1), (cx + 8, cy - 3), (cx + 5, cy + 3),
               (cx + 2, cy + 1), (cx, cy + 3), (cx - 2, cy + 1), (cx - 5, cy + 3)], fill=K)
    d.point((cx - 1, cy - 1), fill=WH); d.point((cx + 1, cy - 1), fill=WH)


def viking(d, cx, cy):
    d.pieslice((cx - 5, cy - 4, cx + 5, cy + 6), 180, 360, fill=(200, 200, 210))
    d.line((cx - 5, cy + 1, cx - 7, cy - 4), fill=WH, width=2); d.line((cx + 5, cy + 1, cx + 7, cy - 4), fill=WH, width=2)


def teapot(d, cx, cy):
    gold = (220, 190, 80)
    d.ellipse((cx - 5, cy - 3, cx + 4, cy + 4), fill=gold)
    d.line((cx + 4, cy, cx + 7, cy - 3), fill=gold, width=2)
    d.rectangle((cx - 1, cy - 5, cx, cy - 4), fill=gold)
    d.point((cx - 6, cy - 4), fill=(70, 170, 60)); d.point((cx - 7, cy - 5), fill=(70, 170, 60))   # the mint


def pyramid(d, cx, cy):
    d.polygon([(cx, cy - 6), (cx + 8, cy + 5), (cx - 8, cy + 5)], fill=(230, 190, 90))
    d.line((cx, cy - 6, cx + 2, cy + 5), fill=BR)


def ricebowl(d, cx, cy):
    d.pieslice((cx - 6, cy - 6, cx + 6, cy + 6), 0, 180, fill=WH)
    d.ellipse((cx - 5, cy - 3, cx + 5, cy + 1), fill=(240, 110, 30))


def eyeball(d, cx, cy):
    d.ellipse((cx - 5, cy - 5, cx + 5, cy + 5), fill=WH, outline=K)
    d.point((cx - 2, cy - 1), fill=K); d.point((cx + 2, cy - 1), fill=K)
    d.line((cx - 2, cy + 2, cx + 2, cy + 2), fill=K)


def lion(d, cx, cy):
    d.ellipse((cx - 6, cy - 6, cx + 6, cy + 6), fill=(230, 140, 30))
    d.ellipse((cx - 4, cy - 4, cx + 4, cy + 4), fill=Y)
    d.point((cx - 2, cy - 1), fill=K); d.point((cx + 2, cy - 1), fill=K); d.point((cx, cy + 2), fill=K)


def chocolate(d, cx, cy):
    dark = (100, 56, 30)
    d.rectangle((cx - 6, cy - 4, cx + 6, cy + 4), fill=BR)
    for k in (-2, 2):
        d.line((cx + k, cy - 4, cx + k, cy + 4), fill=dark)
    d.line((cx - 6, cy, cx + 6, cy), fill=dark)
    d.polygon([(cx + 2, cy - 4), (cx + 6, cy - 4), (cx + 6, cy)], fill=(200, 200, 210))   # the foil


def maki(d, cx, cy):
    d.ellipse((cx - 6, cy - 6, cx + 6, cy + 6), fill=(30, 40, 30))       # the seaweed
    d.ellipse((cx - 4, cy - 4, cx + 4, cy + 4), fill=WH)                 # the rice
    d.ellipse((cx - 2, cy - 2, cx + 2, cy + 2), fill=(188, 0, 45))       # the tuna


def kimchi(d, cx, cy):
    d.ellipse((cx - 5, cy - 3, cx + 5, cy + 6), fill=(140, 70, 40))
    d.rectangle((cx - 3, cy - 5, cx + 3, cy - 3), fill=(140, 70, 40))
    d.pieslice((cx - 4, cy - 1, cx + 4, cy + 5), 180, 360, fill=(205, 46, 58))
    d.pieslice((cx - 4, cy - 1, cx + 4, cy + 5), 0, 180, fill=(0, 71, 160))


def boomerang(d, cx, cy):
    d.line((cx - 6, cy - 4, cx, cy + 3), fill=BRL, width=3); d.line((cx, cy + 3, cx + 7, cy - 3), fill=BRL, width=3)


def palm(d, cx, cy):
    d.line((cx, cy - 2, cx + 1, cy + 6), fill=BR, width=2)
    for dx, dy in ((-6, -1), (6, -1), (-4, -5), (4, -5)):
        d.line((cx, cy - 3, cx + dx, cy + dy), fill=(80, 200, 90), width=2)


def cat(d, cx, cy):
    d.ellipse((cx - 5, cy - 3, cx + 5, cy + 5), fill=WH)
    d.polygon([(cx - 5, cy - 1), (cx - 4, cy - 6), (cx - 1, cy - 3)], fill=WH)
    d.polygon([(cx + 5, cy - 1), (cx + 4, cy - 6), (cx + 1, cy - 3)], fill=WH)
    d.point((cx - 2, cy), fill=(0, 132, 64)); d.point((cx + 2, cy), fill=(0, 132, 64)); d.point((cx, cy + 2), fill=PK)


def pearl(d, cx, cy):
    d.pieslice((cx - 7, cy - 4, cx + 7, cy + 8), 180, 360, fill=(220, 210, 200))
    d.ellipse((cx - 3, cy - 3, cx + 3, cy + 3), fill=WH)
    d.point((cx - 1, cy - 1), fill=(200, 230, 255))


def toucan(d, cx, cy):
    d.ellipse((cx - 5, cy - 3, cx + 1, cy + 5), fill=K)                  # the body
    d.ellipse((cx - 3, cy - 2, cx, cy + 1), fill=WH)                     # the throat
    d.polygon([(cx, cy - 3), (cx + 7, cy - 1), (cx, cy + 1)], fill=(250, 140, 20))   # the beak
    d.point((cx - 2, cy - 2), fill=(30, 120, 220))


def guitar(d, cx, cy):
    d.ellipse((cx - 5, cy - 1, cx + 1, cy + 5), fill=BRL)
    d.ellipse((cx - 2, cy - 4, cx + 2, cy), fill=BRL)
    d.ellipse((cx - 3, cy + 1, cx - 1, cy + 3), fill=BR)
    d.line((cx, cy - 2, cx + 6, cy - 7), fill=BR, width=2)


def mountains(d, cx, cy):
    d.polygon([(cx - 8, cy + 5), (cx - 3, cy - 4), (cx + 2, cy + 5)], fill=(60, 120, 70))
    d.polygon([(cx - 2, cy + 5), (cx + 3, cy - 6), (cx + 8, cy + 5)], fill=(40, 100, 60))
    d.polygon([(cx + 1, cy - 2), (cx + 3, cy - 6), (cx + 5, cy - 2)], fill=WH)    # snow


def mate(d, cx, cy):
    d.ellipse((cx - 4, cy - 2, cx + 4, cy + 6), fill=(150, 92, 44))
    d.rectangle((cx - 3, cy - 3, cx + 3, cy - 1), fill=(200, 200, 210))
    d.ellipse((cx - 2, cy - 2, cx + 2, cy), fill=(80, 160, 60))          # the yerba
    d.line((cx + 1, cy - 2, cx + 4, cy - 8), fill=(200, 200, 210))        # the straw


def tulip(d, cx, cy):
    d.line((cx, cy, cx, cy + 6), fill=(0, 132, 64), width=1)
    d.polygon([(cx - 3, cy + 3), (cx - 1, cy + 1), (cx - 3, cy)], fill=(0, 132, 64))
    d.pieslice((cx - 4, cy - 5, cx + 4, cy + 3), 0, 180, fill=(240, 40, 60))
    d.polygon([(cx - 4, cy - 1), (cx - 3, cy - 6), (cx - 1, cy - 2), (cx, cy - 6), (cx + 1, cy - 2), (cx + 3, cy - 6), (cx + 4, cy - 1)], fill=(240, 40, 60))


# ---------------------------------------------------------------- the sixteen added for 48 sides
GY = (150, 150, 158)
OR = (247, 127, 0)


def spool(d, cx, cy):
    """a reel of silk thread"""
    d.rectangle((cx - 3, cy - 4, cx + 3, cy - 3), fill=Y); d.rectangle((cx - 3, cy + 3, cx + 3, cy + 4), fill=Y)
    d.rectangle((cx - 2, cy - 2, cx + 2, cy + 2), fill=(255, 236, 140))
    d.line((cx - 2, cy - 1, cx + 2, cy - 1), fill=(230, 180, 20)); d.line((cx - 2, cy + 1, cx + 2, cy + 1), fill=(230, 180, 20))
    d.line((cx + 3, cy + 2, cx + 7, cy + 5), fill=(255, 236, 140))


def candle(d, cx, cy):
    d.rectangle((cx - 1, cy - 1, cx + 1, cy + 5), fill=WH)
    d.ellipse((cx - 1, cy - 5, cx + 1, cy - 2), fill=(255, 200, 40)); d.point((cx, cy - 3), fill=(255, 120, 20))


def cheese_cross(d):
    d.rectangle((0, 0, W, H), fill=(213, 43, 30))
    d.rectangle((12, 3, 17, 16), fill=(252, 220, 110)); d.rectangle((8, 7, 21, 12), fill=(252, 220, 110))
    for x, y in ((13, 5), (16, 13), (10, 9), (19, 10), (15, 9)):
        d.point((x, y), fill=(214, 168, 50))


def pierogi(d, cx, cy):
    d.pieslice((cx - 6, cy - 4, cx + 6, cy + 6), 180, 360, fill=(244, 220, 170))
    for x in range(cx - 5, cx + 6, 2):
        d.point((x, cy + 1), fill=(200, 160, 100))


def cream_pot(d, cx, cy):
    d.rectangle((cx - 4, cy - 1, cx + 4, cy + 4), fill=(170, 100, 60))
    d.ellipse((cx - 4, cy - 4, cx + 4, cy), fill=(255, 248, 230))


def lokum(d, cx, cy):
    for dx, dy, col in ((-5, 0, (252, 190, 210)), (0, -3, WH), (4, 1, (252, 190, 210))):
        d.rectangle((cx + dx - 2, cy + dy - 2, cx + dx + 2, cy + dy + 2), fill=col)
        d.point((cx + dx - 1, cy + dy - 1), fill=WH)


def saltire_tartan(d):
    d.rectangle((0, 0, W, H), fill=(0, 94, 184))
    d.line((0, 0, W, H), fill=WH, width=3); d.line((W, 0, 0, H), fill=WH, width=3)
    d.rectangle((12, 7, 17, 12), fill=(196, 30, 40))
    d.line((14, 7, 14, 12), fill=(20, 100, 50)); d.line((12, 9, 17, 9), fill=(20, 100, 50))


def czech_base(d):
    d.rectangle((0, 0, W, 9), fill=WH); d.rectangle((0, 10, W, H), fill=(215, 20, 26))
    d.polygon([(0, 0), (14, 10), (0, 20)], fill=(17, 69, 126))


def goblet(d, cx, cy):
    d.polygon([(cx - 3, cy - 4), (cx + 3, cy - 4), (cx + 1, cy), (cx - 1, cy)], fill=(200, 230, 250))
    d.line((cx, cy, cx, cy + 3), fill=(200, 230, 250)); d.line((cx - 2, cy + 4, cx + 2, cy + 4), fill=(200, 230, 250))
    d.point((cx - 1, cy - 3), fill=WH)


def rose(d, cx, cy):
    d.ellipse((cx - 3, cy - 4, cx + 3, cy + 1), fill=(220, 40, 90))
    d.arc((cx - 2, cy - 3, cx + 2, cy), 0, 300, fill=(150, 20, 60))
    d.line((cx, cy + 1, cx, cy + 6), fill=(30, 120, 40)); d.point((cx + 1, cy + 3), fill=(30, 120, 40))


def greece_base(d):
    for k in range(9):
        d.rectangle((0, k * 20 // 9, W, (k + 1) * 20 // 9 - 1), fill=(13, 94, 175) if k % 2 == 0 else WH)
    d.rectangle((0, 0, 11, 8), fill=(13, 94, 175)); d.rectangle((5, 0, 6, 8), fill=WH); d.rectangle((0, 4, 11, 5), fill=WH)


def olive(d, cx, cy):
    d.ellipse((cx - 3, cy - 2, cx + 3, cy + 3), fill=(80, 110, 40))
    d.line((cx + 1, cy - 2, cx + 4, cy - 5), fill=(60, 80, 30)); d.ellipse((cx + 3, cy - 6, cx + 6, cy - 4), fill=(90, 150, 50))


def chile_base(d):
    d.rectangle((0, 0, W, 9), fill=WH); d.rectangle((0, 10, W, H), fill=(213, 43, 30)); d.rectangle((0, 0, 10, 9), fill=(0, 57, 166))


def condor(d, cx, cy):
    d.line((cx - 4, cy - 1, cx, cy + 1), fill=WH); d.line((cx, cy + 1, cx + 4, cy - 1), fill=WH)
    d.point((cx, cy), fill=WH); d.point((cx, cy + 2), fill=WH)


def llama(d, cx, cy):
    d.rectangle((cx - 3, cy, cx + 2, cy + 2), fill=(240, 236, 226)); d.rectangle((cx + 1, cy - 4, cx + 2, cy), fill=(240, 236, 226))
    d.point((cx + 3, cy - 4), fill=(240, 236, 226)); d.point((cx + 1, cy - 5), fill=(240, 236, 226))
    for x in (cx - 3, cx - 1, cx, cx + 2):
        d.line((x, cy + 3, x, cy + 5), fill=(200, 196, 186))
    d.point((cx + 2, cy - 3), fill=K)


def harp(d, cx, cy):
    d.line((cx - 3, cy + 5, cx - 3, cy - 4), fill=(200, 150, 60)); d.arc((cx - 3, cy - 6, cx + 5, cy + 2), 270, 30, fill=(200, 150, 60))
    d.line((cx - 3, cy + 5, cx + 4, cy - 1), fill=(200, 150, 60))
    for x in range(cx - 2, cx + 4, 2):
        d.line((x, cy - 4, x, cy + 3 - (x - cx)), fill=(240, 220, 160))


def equator(d, cx, cy):
    d.ellipse((cx - 4, cy - 4, cx + 4, cy + 4), fill=(60, 140, 220))
    d.point((cx - 2, cy - 2), fill=(60, 170, 70)); d.rectangle((cx, cy + 1, cx + 2, cy + 2), fill=(60, 170, 70))
    d.line((cx - 6, cy, cx + 6, cy), fill=Y)


def elephant(d, cx, cy):
    d.ellipse((cx - 5, cy - 3, cx + 3, cy + 3), fill=GY); d.ellipse((cx + 1, cy - 4, cx + 6, cy + 1), fill=GY)
    d.line((cx + 6, cy, cx + 6, cy + 4), fill=GY); d.ellipse((cx - 1, cy - 4, cx + 2, cy), fill=(176, 176, 184))
    for x in (cx - 4, cx - 1, cx + 1):
        d.line((x, cy + 3, x, cy + 5), fill=GY)
    d.point((cx + 4, cy - 2), fill=K)


def fennec(d, cx, cy):
    d.polygon([(cx - 5, cy - 6), (cx - 2, cy - 1), (cx - 4, cy)], fill=(232, 170, 90))
    d.polygon([(cx + 5, cy - 6), (cx + 2, cy - 1), (cx + 4, cy)], fill=(232, 170, 90))
    d.ellipse((cx - 3, cy - 2, cx + 3, cy + 3), fill=(240, 190, 110))
    d.point((cx - 1, cy), fill=K); d.point((cx + 1, cy), fill=K); d.point((cx, cy + 2), fill=K)


def algeria_base(d):
    d.rectangle((0, 0, 14, H), fill=(0, 98, 51)); d.rectangle((15, 0, W, H), fill=WH)


def on(base, sym, cx=15, cy=10):
    def f(d):
        base(d)
        sym(d, cx, cy)
    return f


def fill(col):
    def f(d):
        d.rectangle((0, 0, W, H), fill=col)
    return f


def brazil_base(d):
    d.rectangle((0, 0, W, H), fill=(0, 156, 59))
    d.polygon([(15, 1), (29, 10), (15, 19), (1, 10)], fill=Y)
    d.ellipse((9, 4, 21, 16), fill=(0, 39, 118))


def colombia_base(d):
    d.rectangle((0, 0, W, 9), fill=Y); d.rectangle((0, 10, W, 14), fill=(0, 56, 147)); d.rectangle((0, 15, W, H), fill=R)


def stripes_us(d):
    d.rectangle((0, 0, W, H), fill=WH)
    for y in range(0, H, 3):
        d.rectangle((0, y, W, y + 1), fill=(178, 34, 52))
    d.rectangle((0, 0, 13, 10), fill=(60, 59, 110))


def canada_base(d):
    d.rectangle((0, 0, W, H), fill=R); d.rectangle((8, 0, 21, H), fill=WH)


def costa_base(d):
    d.rectangle((0, 0, W, H), fill=(0, 43, 127)); d.rectangle((0, 3, W, 16), fill=WH); d.rectangle((0, 7, W, 12), fill=R)


def cross_base(d):
    d.rectangle((0, 0, W, H), fill=WH); d.rectangle((13, 0, 16, H), fill=R); d.rectangle((0, 8, W, 11), fill=R)


def portugal_base(d):
    d.rectangle((0, 0, 11, H), fill=(0, 102, 0)); d.rectangle((12, 0, W, H), fill=R)


def uruguay_base(d):
    d.rectangle((0, 0, W, H), fill=WH)
    for y in (2, 7, 11, 15):
        d.rectangle((0, y, W, y + 1), fill=(0, 56, 168))
    d.rectangle((0, 0, 11, 9), fill=WH)


def korea_base(d):
    d.rectangle((0, 0, W, H), fill=WH)
    for x, y in ((3, 3), (25, 3), (3, 15), (25, 15)):
        d.rectangle((x, y, x + 2, y + 1), fill=K)


def australia_base(d):
    d.rectangle((0, 0, W, H), fill=(0, 0, 139))
    d.rectangle((0, 0, 13, 9), fill=(0, 36, 125))
    d.line((0, 0, 13, 9), fill=WH); d.line((13, 0, 0, 9), fill=WH)
    d.rectangle((6, 0, 7, 9), fill=R); d.rectangle((0, 4, 13, 5), fill=R)
    for x, y in ((24, 3), (27, 9), (24, 16)):
        star(d, x, y, 2, WH)


def qatar_base(d):
    d.rectangle((0, 0, W, H), fill=(138, 21, 56)); d.rectangle((0, 0, 8, H), fill=WH)
    d.polygon([(8, 0), (12, 2), (8, 4), (12, 6), (8, 8), (12, 10), (8, 12), (12, 14), (8, 16), (12, 18), (8, 20)], fill=WH)


FLAGS = [
    on(brazil_base, toucan),                                        # SAMBALIA
    on(h3((116, 172, 223), WH, (116, 172, 223)), sunface),          # ARGENTANGO
    on(uruguay_base, mate, 5, 5),                                   # MATEGUAY
    on(colombia_base, coffee),                                      # COFFEELOMBIA
    on(v3((0, 104, 71), WH, R), guitar),                            # MARIACHIA
    on(stripes_us, burger, 6, 5),                                   # BURGERLAND
    on(canada_base, pancakes),                                      # MAPLEDONIA
    on(costa_base, frog),                                           # PURAVIDIA
    on(v3((0, 85, 164), WH, (239, 65, 53)), baguette),              # BAGUETTIA
    on(h3(K, (221, 0, 0), (255, 206, 0)), sausage),                 # WURSTLAND
    on(v3((0, 146, 70), WH, (206, 43, 55)), pizza),                 # PASTALIA
    on(h3((170, 21, 27), (241, 191, 0), (170, 21, 27), (5, 15)), paella, 9, 10),   # PAELLANIA
    on(cross_base, teacup),                                         # TEALAND
    on(portugal_base, fish, 11, 10),                                # BACALANDIA
    on(h3((174, 28, 40), WH, (33, 70, 139)), tulip),                # TULIPLANDS
    on(v3(K, (253, 218, 36), (239, 51, 64)), waffle),               # WAFFLEGIUM
    on(h3(R, WH, (23, 23, 150)), necktie),                          # CRAVATIA
    on(nordic((0, 106, 167), (254, 204, 0)), meatball, 22, 15),     # MEATBALLIA
    on(v3((0, 43, 127), (252, 209, 22), (206, 17, 38)), mountains), # CARPATHIA
    on(nordic((186, 12, 47), WH, (0, 32, 91)), viking, 22, 15),     # FJORDWAY
    on(fill((193, 39, 45)), teapot),                                # MINTOROCCO
    on(h3(R, WH, K), pyramid),                                      # PYRAMIDIA
    on(v3((0, 135, 81), WH, (0, 135, 81)), ricebowl),               # JOLLOFIA
    on(v3((0, 133, 63), Y, (227, 27, 35)), eyeball),                # SENEGOAL
    on(v3((0, 122, 94), (206, 17, 38), Y), lion),                   # CAMEROAR
    on(h3(R, Y, (0, 107, 63)), chocolate),                          # CHOCOGHANA
    on(fill(WH), maki),                                             # SUSHIPON
    on(korea_base, kimchi),                                         # KIMCHIREA
    on(australia_base, boomerang, 9, 15),                           # KANGAROOLIA
    on(fill((0, 108, 53)), palm),                                   # PALMARABIA
    on(h3((35, 159, 64), WH, (218, 0, 0)), cat),                    # PERSICATIA
    on(qatar_base, pearl, 21, 10),                                  # PEARLATAR
    on(fill((222, 41, 16)), spool, 7, 6),                           # SILKINA: the reel where the stars are
    on(nordic((200, 16, 46), WH), candle, 23, 15),                  # HYGGEMARK
    cheese_cross,                                                   # FONDUZERLAND: the cross, holes and all
    on(h3(WH, WH, (220, 20, 60), (5, 10)), pierogi, 15, 14),        # PIEROLAND
    on(h3((198, 54, 60), (12, 64, 118), WH), cream_pot, 9, 9),      # KAJMAKIA
    on(fill((227, 10, 23)), lokum),                                 # DELIGHTIYE
    saltire_tartan,                                                 # TARTANLAND
    on(czech_base, goblet, 5, 10),                                  # CRYSTALIA
    on(h3(WH, (0, 150, 110), (214, 38, 18)), rose, 24, 9),          # ROSEGARIA
    on(greece_base, olive, 21, 13),                                 # FETALLAS
    on(chile_base, condor, 5, 4),                                   # CONDORIA
    on(v3((217, 16, 35), WH, (217, 16, 35)), llama),                # LLAMARU
    on(h3((213, 43, 30), WH, (0, 56, 168)), harp),                  # HARPAGUAY
    on(h3((255, 209, 0), (3, 78, 162), (237, 28, 36), (10, 15)), equator, 15, 9),   # EQUATORIA
    on(v3(OR, WH, (0, 158, 96)), elephant),                         # ELEPHANTOIRE
    on(algeria_base, fennec),                                       # FENNECIA
]


def sheet():
    out = np.zeros((32, 32 * len(FLAGS), 4), np.uint8)
    for i, f in enumerate(FLAGS):
        im = Image.new('RGB', (W, H))
        f(ImageDraw.Draw(im))
        a = np.asarray(im)
        x, y = i * 32 + 1, 6
        out[y - 1:y + H + 1, x - 1:x + W + 1, :3] = (12, 12, 18)
        out[y - 1:y + H + 1, x - 1:x + W + 1, 3] = 255
        out[y:y + H, x:x + W, :3] = a
    return out
