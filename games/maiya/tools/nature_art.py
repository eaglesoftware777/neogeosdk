"""Hand-built nature pixel art for Maiya: Super Nature Girl.

Every decoration, ledge block and gate the missions are dressed with is drawn
here on a 15 colour nature palette so the whole playfield reads as one set:
soft rounded silhouettes, a dark outline, two light steps and one shadow step.

Each painter returns an RGBA array ready for the C-ROM encoder.
"""

from __future__ import annotations

import numpy as np

# 15 drawing colours + transparent.  Index 0 is always transparent.
NATURE = np.array([
    (0, 0, 0),          # 0  transparent
    (34, 28, 42),       # 1  outline
    (40, 92, 56),       # 2  leaf shadow
    (74, 148, 70),      # 3  leaf mid
    (140, 202, 100),    # 4  leaf light
    (74, 50, 36),       # 5  wood shadow
    (122, 84, 52),      # 6  wood mid
    (178, 134, 88),     # 7  wood light
    (84, 84, 104),      # 8  stone shadow
    (132, 132, 152),    # 9  stone mid
    (188, 192, 206),    # 10 stone light
    (248, 208, 96),     # 11 sun gold
    (232, 148, 62),     # 12 warm amber
    (240, 132, 172),    # 13 blossom
    (86, 158, 222),     # 14 water
    (168, 218, 248),    # 15 water light
], dtype=np.uint8)

OUT, LF_D, LF_M, LF_L = 1, 2, 3, 4
WD_D, WD_M, WD_L = 5, 6, 7
ST_D, ST_M, ST_L = 8, 9, 10
GOLD, AMBER, BLOSSOM, WATER, WATER_L = 11, 12, 13, 14, 15


def canvas(w=32, h=32):
    return np.zeros((h, w), dtype=np.uint8)


def to_rgba(idx, palette=NATURE):
    rgb = palette[idx]
    alpha = np.where(idx > 0, 255, 0).astype(np.uint8)
    return np.dstack((rgb, alpha)).astype(np.uint8)


def disc(a, cx, cy, rx, ry, color, squash=1.0):
    h, w = a.shape
    ys, xs = np.mgrid[0:h, 0:w]
    m = ((xs - cx) / max(rx, 0.001)) ** 2 + ((ys - cy) / max(ry * squash, 0.001)) ** 2 <= 1.0
    a[m] = color
    return m


def box(a, x0, y0, x1, y1, color):
    a[max(0, y0):max(0, y1), max(0, x0):max(0, x1)] = color


def outline(a, color=OUT):
    """Wrap every drawn shape in a one pixel dark border."""
    solid = a > 0
    pad = np.pad(solid, 1)
    edge = (
        pad[:-2, 1:-1] | pad[2:, 1:-1] | pad[1:-1, :-2] | pad[1:-1, 2:]
        | pad[:-2, :-2] | pad[:-2, 2:] | pad[2:, :-2] | pad[2:, 2:]
    )
    a[(~solid) & edge] = color
    return a


def speckle(a, mask, color, step=3, offset=0):
    h, w = a.shape
    ys, xs = np.mgrid[0:h, 0:w]
    a[mask & (((xs * 2 + ys * 3 + offset) % step) == 0)] = color


def leafy_crown(a, cx, cy, rx, ry):
    """A soft cluster of three overlapping leaf blobs with a lit top-left."""
    disc(a, cx, cy, rx, ry, LF_M)
    disc(a, cx - rx * 0.55, cy + ry * 0.15, rx * 0.62, ry * 0.66, LF_M)
    disc(a, cx + rx * 0.55, cy + ry * 0.15, rx * 0.62, ry * 0.66, LF_M)
    shade = a == LF_M
    h, w = a.shape
    ys, xs = np.mgrid[0:h, 0:w]
    a[shade & ((ys - cy) * 1.4 + (xs - cx) > ry * 0.7)] = LF_D
    a[shade & ((ys - cy) * 1.6 + (xs - cx) < -ry * 0.9)] = LF_L
    speckle(a, a == LF_M, LF_L, step=7, offset=2)


# --------------------------------------------------------------------------
#  Decorations (32 x 32 unless noted)
# --------------------------------------------------------------------------

def grass_tuft():
    a = canvas()
    for i, (x, hgt, col) in enumerate(((5, 12, LF_D), (9, 18, LF_M), (14, 22, LF_M),
                                       (19, 17, LF_L), (24, 13, LF_M), (27, 9, LF_D))):
        for k in range(hgt):
            y = 31 - k
            bend = int((k * k) / max(hgt, 1) * 0.28) * (1 if i % 2 else -1)
            a[y, np.clip(x + bend, 0, 31)] = col
            if k < hgt - 4:
                a[y, np.clip(x + bend + 1, 0, 31)] = col
    return to_rgba(outline(a))


def flower_patch():
    a = canvas()
    for x, hgt in ((6, 13), (15, 18), (24, 11)):
        for k in range(hgt):
            a[31 - k, x] = LF_M
        a[31 - hgt // 2, x - 2:x] = LF_L
    for cx, cy, col in ((6, 17, GOLD), (15, 12, BLOSSOM), (24, 19, BLOSSOM)):
        disc(a, cx, cy, 3.4, 3.4, col)
        disc(a, cx, cy, 1.4, 1.4, GOLD if col == BLOSSOM else AMBER)
    for k in range(3, 30, 4):
        a[30, k] = LF_D
    return to_rgba(outline(a))


def bush():
    a = canvas()
    leafy_crown(a, 16, 20, 14, 9)
    for x in (9, 16, 23):
        a[28:31, x] = WD_D
    return to_rgba(outline(a))


def mushroom_cluster():
    a = canvas()
    for cx, cy, r, col in ((11, 19, 9.0, BLOSSOM), (23, 24, 6.5, AMBER)):
        stem_w = 2 if r < 8 else 3
        box(a, int(cx) - stem_w, int(cy), int(cx) + stem_w, 32, WD_L)
        box(a, int(cx) - stem_w, int(cy), int(cx) - stem_w + 1, 32, WD_M)
        m = disc(a, cx, cy, r, r * 0.82, col)
        a[int(cy):, :][m[int(cy):, :]] = 0
        box(a, int(cx - r), int(cy) - 1, int(cx + r) + 1, int(cy) + 1, col)
        disc(a, cx - r * 0.35, cy - r * 0.35, r * 0.4, r * 0.26, GOLD if col == BLOSSOM else LF_L)
        speckle(a, a == col, GOLD if col == BLOSSOM else BLOSSOM, step=7)
    return to_rgba(outline(a))


def sapling():
    a = canvas()
    box(a, 14, 14, 18, 32, WD_M)
    box(a, 14, 14, 15, 32, WD_D)
    a[26:32, 10:14] = WD_D
    a[26:32, 18:22] = WD_D
    leafy_crown(a, 16, 11, 13, 9)
    return to_rgba(outline(a))


def rock():
    a = canvas()
    disc(a, 16, 24, 13, 9, ST_M)
    disc(a, 12, 21, 6, 4, ST_L)
    disc(a, 22, 27, 6, 3.4, ST_D)
    speckle(a, a == ST_M, ST_D, step=9, offset=1)
    return to_rgba(outline(a))


def lantern():
    a = canvas()
    box(a, 14, 18, 18, 32, ST_M)
    box(a, 14, 18, 15, 32, ST_D)
    box(a, 11, 30, 21, 32, ST_D)
    box(a, 10, 12, 22, 19, ST_M)
    box(a, 12, 14, 20, 18, GOLD)
    box(a, 13, 15, 19, 17, AMBER)
    box(a, 9, 9, 23, 12, ST_L)
    box(a, 13, 6, 19, 9, ST_M)
    return to_rgba(outline(a))


def sign_post():
    a = canvas()
    box(a, 15, 16, 18, 32, WD_D)
    box(a, 5, 8, 27, 18, WD_M)
    box(a, 5, 8, 27, 10, WD_L)
    for x in range(8, 25, 4):
        a[12:16, x:x + 2] = WD_D
    a[19:21, 13:20] = WD_D
    return to_rgba(outline(a))


def lily_pad():
    a = canvas()
    disc(a, 16, 24, 14, 5, LF_M)
    disc(a, 12, 22, 7, 3, LF_L)
    a[24:27, 16:24] = LF_D
    disc(a, 22, 20, 3.4, 3.0, BLOSSOM)
    disc(a, 22, 20, 1.4, 1.2, GOLD)
    return to_rgba(outline(a))


def water_fall():
    """A 32 x 32 tileable curtain of falling water."""
    a = canvas()
    a[:, :] = WATER
    for x in range(0, 32):
        wave = np.sin((x + 3) * 0.9)
        if wave > 0.35:
            a[:, x] = WATER_L
    for y in range(0, 32, 6):
        a[y:y + 2, :] = WATER_L
    for y in range(3, 32, 6):
        a[y:y + 1, 2:30] = WATER
    a[:, 0:2] = WATER_L
    a[:, 30:32] = WATER_L
    return to_rgba(a)


def vine():
    """A full 32 x 128 climbing vine column: one sprite group, four tiles tall."""
    a = canvas(32, 128)
    for y in range(128):
        x = 15 + int(3.0 * np.sin(y * 0.22))
        a[y, x:x + 3] = WD_M
        a[y, x] = WD_D
        a[y, x + 2] = WD_L
        if y % 11 == 3:
            disc(a, x - 5, y, 4.5, 2.6, LF_M)
            disc(a, x - 5, y - 1, 2.4, 1.3, LF_L)
        if y % 11 == 8:
            disc(a, x + 8, y, 4.5, 2.6, LF_M)
            disc(a, x + 8, y - 1, 2.4, 1.3, LF_L)
        if y % 23 == 14:
            disc(a, x + 1, y + 3, 2.6, 2.6, BLOSSOM)
    return to_rgba(outline(a))


def door(open_state=False):
    a = canvas()
    box(a, 4, 4, 28, 32, WD_M)
    box(a, 4, 4, 28, 6, WD_L)
    disc(a, 16, 6, 12, 7, WD_M)
    box(a, 4, 12, 28, 32, WD_M)
    for x in range(6, 27, 6):
        a[8:31, x:x + 1] = WD_D
    box(a, 5, 5, 27, 8, WD_L)
    if open_state:
        box(a, 8, 9, 24, 31, 0)
        box(a, 8, 9, 10, 31, WD_D)
        box(a, 22, 9, 24, 31, WD_D)
    else:
        a[20:23, 21:24] = GOLD
    return to_rgba(outline(a))


def gate(open_state=False):
    """The Ancient Nature Gate: stone arch laced with vines and a sun lock."""
    a = canvas(32, 48)
    box(a, 2, 6, 30, 48, ST_M)
    box(a, 2, 6, 5, 48, ST_D)
    box(a, 27, 6, 30, 48, ST_L)
    box(a, 0, 0, 32, 7, ST_L)
    box(a, 0, 4, 32, 7, ST_D)
    for y in range(10, 46, 6):
        a[y:y + 1, 6:26] = ST_D
    if open_state:
        box(a, 6, 10, 26, 48, 0)
        box(a, 6, 10, 8, 48, ST_D)
        box(a, 24, 10, 26, 48, ST_D)
    else:
        box(a, 6, 10, 26, 48, LF_D)
        for y in range(10, 48, 5):
            a[y:y + 2, 6:26] = LF_M
            a[y + 2:y + 3, 7:25] = LF_L
        disc(a, 16, 26, 6, 6, GOLD)
        disc(a, 16, 26, 3.4, 3.4, AMBER)
        a[26:31, 15:18] = AMBER
    return to_rgba(outline(a))


DECOR = (
    ("grass", grass_tuft), ("flowers", flower_patch), ("bush", bush),
    ("mushroom", mushroom_cluster), ("sapling", sapling), ("rock", rock),
    ("lantern", lantern), ("sign", sign_post), ("lily", lily_pad),
    ("fall", water_fall), ("vine", vine),
    ("door", lambda: door(False)), ("door_open", lambda: door(True)),
)


# --------------------------------------------------------------------------
#  Ledge blocks: left / middle / right piece per biome
# --------------------------------------------------------------------------

BIOMES = {
    # name        top light    top mid     body mid    body dark   speck
    "grass":  ((150, 208, 104), (86, 158, 74), (118, 84, 54), (72, 50, 36), (168, 126, 82)),
    "moss":   ((124, 196, 148), (66, 142, 108), (86, 96, 78), (52, 60, 50), (132, 146, 118)),
    "sand":   ((246, 224, 158), (222, 188, 112), (188, 148, 92), (132, 100, 62), (232, 200, 140)),
    "autumn": ((246, 186, 96), (214, 138, 62), (140, 96, 58), (88, 58, 38), (196, 142, 86)),
    "snow":   ((244, 248, 252), (198, 218, 238), (136, 158, 186), (88, 106, 132), (216, 230, 244)),
    "bark":   ((158, 124, 84), (112, 82, 54), (86, 62, 42), (54, 38, 26), (132, 100, 68)),
}


def ledge_block(biome, piece):
    """A ledge block with a face, not a flat sticker.

    piece: 0 left cap, 1 middle, 2 right cap.

    The top is a lit surface in perspective, the body a shaded front face
    with a bevel down the lit side and a dark cast shadow under the lip, so
    a shelf reads as something standing out of the valley rather than a
    rectangle pasted onto it.
    """
    top_l, top_m, body_m, body_d, speck_c = BIOMES[biome]
    pal = np.array([
        (0, 0, 0),
        (24, 20, 28),                                   # 1 outline
        top_l,                                          # 2 lit top
        top_m,                                          # 3 top shade
        body_m,                                         # 4 front face
        body_d,                                         # 5 deep shadow
        speck_c,                                        # 6 speckle
        tuple(min(255, int(c * 1.18) + 12) for c in top_l),    # 7 crest highlight
        tuple(int(c * 0.72) for c in body_m),                  # 8 face shade
        tuple(int(c * 0.52) for c in body_d),                  # 9 cast shadow
    ], dtype=np.uint8)
    a = canvas()
    ys, xs = np.mgrid[0:32, 0:32]

    a[2:32, :] = 4            # front face
    a[2:6, :] = 2             # the top surface, seen at a shallow angle
    a[6:9, :] = 3             # the far edge of that surface rolling over
    a[2:3, :] = 7             # crest highlight
    a[9:12, :] = 8            # the face immediately under the lip: in shade
    a[27:32, :] = 5
    a[30:32, :] = 9           # the block's own shadow on what is below

    body = a == 4
    a[body & (((xs * 3 + ys * 5) % 13) == 0)] = 6
    a[body & (((xs * 7 + ys * 11) % 23) == 0)] = 8

    # Crest fringe: grass (or snow, or bark) hanging over the lip.
    for x in range(0, 32, 4):
        drop = 2 + ((x // 4) % 3)
        a[9:9 + drop, x:x + 2] = 3
        a[9:9 + drop - 1, x + 1:x + 2] = 2

    # A bevel down the lit side of the face, and depth on the shaded side.
    a[6:30, 0:2] = np.where(a[6:30, 0:2] == 0, 0, 8)
    a[6:30, 30:32] = np.where(a[6:30, 30:32] == 0, 0, 5)

    if piece == 0:
        corner = ((xs < 6) & (ys < 9)) & (((xs - 6) ** 2 / 36 + (ys - 9) ** 2 / 49) > 1)
        a[corner] = 0
        a[2:32, 0:2][a[2:32, 0:2] > 0] = 5
        a[4:30, 2:3][a[4:30, 2:3] > 0] = 8
    if piece == 2:
        corner = ((xs > 25) & (ys < 9)) & (((xs - 25) ** 2 / 36 + (ys - 9) ** 2 / 49) > 1)
        a[corner] = 0
        a[2:32, 30:32][a[2:32, 30:32] > 0] = 5
        a[4:30, 29:30][a[4:30, 29:30] > 0] = 8

    solid = a > 0
    pad = np.pad(solid, 1)
    edge = (pad[:-2, 1:-1] | pad[2:, 1:-1] | pad[1:-1, :-2] | pad[1:-1, 2:])
    a[(~solid) & edge] = 1
    if piece != 0:
        a[:, 0][a[:, 0] == 1] = 0
    if piece != 2:
        a[:, 31][a[:, 31] == 1] = 0

    rgb = pal[a]
    alpha = np.where(a > 0, 255, 0).astype(np.uint8)
    return np.dstack((rgb, alpha)).astype(np.uint8)


def standing_stone(biome):
    """A mossy boulder the ledges sit on: pure foreground depth."""
    top_l, top_m, body_m, body_d, speck_c = BIOMES[biome]
    pal = np.array([
        (0, 0, 0), (22, 18, 26),
        tuple(min(255, int(c * 1.14)) for c in body_m),
        body_m, body_d,
        tuple(int(c * 0.6) for c in body_d),
        top_m, top_l,
    ], dtype=np.uint8)
    a = canvas(32, 48)
    disc(a, 16, 31, 15, 16, 3)
    disc(a, 11, 25, 8.5, 9, 2)
    disc(a, 23, 38, 9, 8, 4)
    a[42:48, :][a[42:48, :] > 0] = 5
    ys, xs = np.mgrid[0:48, 0:32]
    a[(a == 3) & (((xs * 5 + ys * 3) % 17) == 0)] = 4

    # moss follows the crown of the stone rather than a straight line 
    for x in range(32):
        column = np.nonzero(a[:, x])[0]
        if not len(column):
            continue
        top = column[0]
        if top > 26:
            continue
        depth = 3 + ((x * 7) % 3)
        a[top:top + depth, x] = 6
        a[top:top + 1, x] = 7
    solid = a > 0
    pad = np.pad(solid, 1)
    edge = (pad[:-2, 1:-1] | pad[2:, 1:-1] | pad[1:-1, :-2] | pad[1:-1, 2:])
    a[(~solid) & edge] = 1
    rgb = pal[a]
    return np.dstack((rgb, np.where(a > 0, 255, 0).astype(np.uint8))).astype(np.uint8)


def fern_frond():
    """A big leaf for the front plane, scrolling faster than the road."""
    a = canvas(32, 48)
    for y in range(4, 46):
        t = (y - 4) / 42.0
        bend = int(4.0 * np.sin(t * 2.2))
        half = int(12.0 * np.sin(t * np.pi) ** 0.7)
        if half <= 0:
            continue
        cx = 16 + bend
        a[y, max(0, cx - half):min(32, cx + half)] = LF_M
        if (y % 4) == 0:
            a[y, max(0, cx - half):min(32, cx - half + 4)] = LF_D
            a[y, max(0, cx + half - 4):min(32, cx + half)] = LF_L
        a[y, max(0, cx - 1):min(32, cx + 1)] = LF_D
    speckle(a, a == LF_M, LF_L, step=11, offset=2)
    return to_rgba(outline(a))


# --------------------------------------------------------------------------
#  Trinkets: coins, charms and the power-ups Maiya eats on the run
# --------------------------------------------------------------------------

def _coin(rim, face, shine):
    a = canvas()
    disc(a, 16, 16, 11, 11, rim)
    disc(a, 16, 16, 8.5, 8.5, face)
    disc(a, 12, 12, 3.2, 3.2, shine)
    a[14:19, 15:18] = rim
    a[15:18, 13:20] = rim
    return a


def gold_coin():
    return to_rgba(outline(_coin(AMBER, GOLD, WATER_L)))


def silver_coin():
    return to_rgba(outline(_coin(ST_D, ST_L, WATER_L)))


def cut_flower():
    a = canvas()
    for k in range(10):
        a[31 - k, 15:17] = LF_M
    a[24:27, 11:15] = LF_L
    a[26:29, 17:21] = LF_M
    for dx, dy in ((0, -6), (6, -2), (4, 5), (-4, 5), (-6, -2)):
        disc(a, 16 + dx, 15 + dy, 4.2, 4.2, BLOSSOM)
    disc(a, 16, 15, 4.0, 4.0, GOLD)
    return to_rgba(outline(a))


def critter():
    """A forest friend freed from the blight: long ears, round body."""
    a = canvas()
    disc(a, 16, 22, 9, 7, WD_L)
    disc(a, 16, 14, 6.5, 6, WD_L)
    disc(a, 12, 8, 2.4, 5.5, WD_L)
    disc(a, 20, 8, 2.4, 5.5, WD_L)
    disc(a, 12, 8, 1.2, 3.6, BLOSSOM)
    disc(a, 20, 8, 1.2, 3.6, BLOSSOM)
    disc(a, 22, 26, 3.4, 3.0, WD_L)
    a[13:15, 13:15] = OUT
    a[13:15, 18:20] = OUT
    a[16:18, 15:18] = BLOSSOM
    a[a == WD_L] = WD_L
    speckle(a, a == WD_L, WD_M, step=11, offset=3)
    return to_rgba(outline(a))


def life_heart():
    a = canvas()
    for y in range(32):
        for x in range(32):
            nx, ny = (x - 16) / 11.0, (y - 14) / 11.0
            if (nx * nx + ny * ny - 1) ** 3 - nx * nx * ny * ny * ny <= 0:
                a[y, x] = BLOSSOM
    disc(a, 12, 11, 3.0, 2.4, WATER_L)
    a[a == 0] = 0
    return to_rgba(outline(a))


def swift_leaf():
    a = canvas()
    for y in range(6, 28):
        half = int(7 * np.sin((y - 6) / 22.0 * np.pi))
        a[y, 16 - half:16 + half + 1] = LF_M
    a[6:28, 15:17] = LF_D
    for y in range(9, 26, 4):
        a[y, 17:17 + 5] = LF_L
        a[y + 1, 10:15] = LF_L
    return to_rgba(outline(a))


def might_berry():
    a = canvas()
    disc(a, 13, 19, 6.5, 6.5, BLOSSOM)
    disc(a, 21, 22, 5.5, 5.5, BLOSSOM)
    disc(a, 17, 13, 5.0, 5.0, BLOSSOM)
    disc(a, 11, 17, 2.0, 2.0, WATER_L)
    a[4:10, 15:18] = LF_M
    disc(a, 12, 6, 4.0, 2.0, LF_L)
    disc(a, 21, 6, 4.0, 2.0, LF_M)
    return to_rgba(outline(a))


def veil_orb():
    a = canvas()
    disc(a, 16, 16, 11, 11, WATER)
    disc(a, 16, 16, 7.5, 7.5, WATER_L)
    disc(a, 13, 12, 3.0, 3.0, ST_L)
    for k in range(0, 32, 5):
        a[k:k + 1, :] = np.where(a[k:k + 1, :] == WATER_L, WATER, a[k:k + 1, :])
    return to_rgba(outline(a))


def wizard_charm():
    """The elder's charm: read it and the valley gives up a secret."""
    a = canvas()
    box(a, 7, 6, 25, 27, ST_L)
    box(a, 7, 6, 25, 9, ST_M)
    box(a, 7, 24, 25, 27, ST_M)
    for y in range(12, 23, 3):
        a[y:y + 1, 10:22] = ST_D
    disc(a, 16, 16, 4.0, 4.0, GOLD)
    disc(a, 16, 16, 2.0, 2.0, AMBER)
    box(a, 5, 4, 7, 29, WD_M)
    box(a, 25, 4, 27, 29, WD_M)
    return to_rgba(outline(a))


def spring_bud():
    """A coiled fern bud: it gives her legs for a while."""
    a = canvas()
    for k in range(9):
        t = k / 9.0
        r = 11.0 - t * 9.0
        ang = t * 7.2
        cx = 16 + int(np.cos(ang) * (11 - r))
        cy = 20 + int(np.sin(ang) * (11 - r) * 0.7)
        disc(a, cx, cy, r * 0.5, r * 0.5, LF_M if k % 2 else LF_L)
    disc(a, 16, 20, 2.4, 2.4, GOLD)
    a[24:32, 15:18] = LF_D
    disc(a, 11, 28, 4.5, 2.2, LF_M)
    disc(a, 22, 29, 4.5, 2.2, LF_M)
    return to_rgba(outline(a))


def thorn_crown():
    """A ring of rose thorns: her throw comes out bigger and faster."""
    a = canvas()
    disc(a, 16, 18, 10, 8, BLOSSOM)
    disc(a, 16, 18, 6.5, 5.0, 0)
    for ang in range(0, 360, 45):
        r = np.deg2rad(ang)
        x = int(16 + np.cos(r) * 12)
        y = int(18 + np.sin(r) * 10)
        disc(a, x, y, 2.6, 2.6, GOLD)
    disc(a, 16, 18, 3.4, 2.6, AMBER)
    return to_rgba(outline(a))


TRINKETS = (
    ("gold", gold_coin), ("silver", silver_coin), ("flower", cut_flower),
    ("critter", critter), ("life", life_heart), ("swift", swift_leaf),
    ("might", might_berry), ("veil", veil_orb), ("charm", wizard_charm),
    ("spring", spring_bud), ("crown", thorn_crown),
)
