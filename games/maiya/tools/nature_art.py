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


# Climbables are drawn a whole tall climb high (16 tiles); a shorter climb
# shows only its top rows.
CLIMB_H = 256


def vine():
    """A full 32 x 256 climbing vine column: one sprite group, sixteen tiles tall."""
    a = canvas(32, CLIMB_H)
    for y in range(CLIMB_H):
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


def rope_ladder():
    """A rope ladder: two twisted ropes, wooden rungs every eight rows."""
    a = canvas(32, CLIMB_H)
    for y in range(CLIMB_H):
        for x0 in (7, 23):
            a[y, x0:x0 + 3] = AMBER
            a[y, x0 + ((y // 2) % 3)] = WD_L          # the twist
        if y % 12 == 6:
            a[y:y + 3, 8:26] = WD_M
            a[y, 8:26] = WD_L
    return to_rgba(outline(a))


def wood_ladder():
    """A wooden ladder of two rails and rungs, the rails' grain showing."""
    a = canvas(32, CLIMB_H)
    a[:, 5:9] = WD_M
    a[:, 23:27] = WD_M
    a[:, 5] = WD_L
    a[:, 23] = WD_L
    a[:, 8] = WD_D
    a[:, 26] = WD_D
    for y in range(4, CLIMB_H, 12):
        a[y:y + 3, 9:23] = WD_M
        a[y, 9:23] = WD_L
        a[y + 2, 9:23] = WD_D
    return to_rgba(outline(a))


def iron_chain():
    """A hanging chain of iron links, turned alternately edge-on."""
    a = canvas(32, CLIMB_H)
    for k, y in enumerate(range(0, CLIMB_H - 10, 10)):
        if k % 2 == 0:
            disc(a, 16, y + 5, 5, 6, ST_M)
            disc(a, 16, y + 5, 2, 3, 0)
            a[y + 1:y + 3, 13:16] = ST_L
        else:
            a[y:y + 11, 14:19] = ST_D
            a[y + 1:y + 10, 15:17] = ST_M
    return to_rgba(outline(a))


def kelp():
    """A tall swaying frond of kelp with bladders along it."""
    a = canvas(32, CLIMB_H)
    for y in range(CLIMB_H):
        x = 14 + int(4.0 * np.sin(y * 0.12))
        a[y, x:x + 4] = LF_M
        a[y, x] = LF_D
        a[y, x + 3] = LF_L
        if y % 16 == 5:
            disc(a, x - 4, y, 4, 2, LF_M)
        if y % 16 == 13:
            disc(a, x + 8, y, 4, 2, LF_L)
        if y % 24 == 9:
            disc(a, x + 2, y + 2, 2, 2, AMBER)
    return to_rgba(outline(a))


def ice_vine():
    """A vine frozen in a sleeve of ice, frost on its leaves."""
    a = canvas(32, CLIMB_H)
    for y in range(CLIMB_H):
        x = 14 + int(3.0 * np.sin(y * 0.22))
        a[y, x - 2:x + 5] = WATER_L
        a[y, x:x + 3] = LF_D
        a[y, x + 1] = LF_M
        if y % 11 == 3:
            disc(a, x - 5, y, 4.5, 2.6, WATER_L)
            disc(a, x - 5, y, 2.4, 1.3, ST_L)
        if y % 11 == 8:
            disc(a, x + 8, y, 4.5, 2.6, WATER_L)
            disc(a, x + 8, y, 2.4, 1.3, ST_L)
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
    # What each valley climbs besides a vine (appended: no index moves).
    ("rope", lambda: rope_ladder()), ("ladder", lambda: wood_ladder()),
    ("chain", lambda: iron_chain()), ("kelp", lambda: kelp()), ("icevine", lambda: ice_vine()),
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
    "rust":   ((190, 120, 70), (140, 82, 48), (96, 88, 90), (58, 52, 56), (168, 104, 60)),
    # The last three valleys: reef rock crowned with coral, mine stone
    # flecked with the glowing seep, red savanna earth under dry grass.
    "coral":  ((255, 150, 164), (222, 96, 124), (70, 110, 142), (40, 64, 94), (122, 204, 214)),
    "stone":  ((190, 194, 200), (140, 148, 158), (92, 88, 98), (56, 52, 62), (150, 226, 86)),
    "savanna": ((238, 206, 112), (204, 160, 72), (172, 94, 56), (112, 60, 38), (216, 144, 92)),
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


def thorn_bundle():
    """A sheaf of thorns tied with a leaf: more thorns to throw."""
    a = canvas()
    for i, x in enumerate(range(9, 24, 3)):
        top = 5 + (i % 2) * 2
        a[top:27, x:x + 2] = LF_D
        a[top - 2:top, x] = BLOSSOM
    disc(a, 16, 17, 8.0, 2.4, LF_M)
    a[16:18, 9:24] = LF_L
    return to_rgba(outline(a))


def spread_fan():
    """Three petals fanned from one stem: the spread throw."""
    a = canvas()
    for ang in (-40, 0, 40):
        r = np.deg2rad(ang - 90)
        cx = int(16 + np.cos(r) * 8)
        cy = int(18 + np.sin(r) * 8)
        disc(a, cx, cy, 4.2, 4.2, BLOSSOM)
        disc(a, cx, cy, 1.8, 1.8, WATER_L)
    a[18:29, 15:17] = LF_M
    return to_rgba(outline(a))


def pierce_seed():
    """A hard golden seed with a trail: it passes straight through."""
    a = canvas()
    disc(a, 19, 16, 7.0, 5.0, GOLD)
    disc(a, 17, 14, 2.6, 1.8, AMBER)
    a[15:18, 4:12] = LF_L
    a[13:14, 6:11] = LF_L
    a[19:20, 6:11] = LF_L
    return to_rgba(outline(a))


def gale_leaf():
    """A curled wind leaf: thrown, it swings out and comes back."""
    a = canvas()
    for t in np.linspace(0, 1.6 * np.pi, 60):
        r = 3 + t * 2.2
        x = int(16 + np.cos(t) * r)
        y = int(16 + np.sin(t) * r)
        disc(a, x, y, 1.6, 1.6, LF_M)
    disc(a, 16, 16, 2.4, 2.4, LF_L)
    return to_rgba(outline(a))


def bloom_bud():
    """A fat red bud about to burst: one more Secret Art."""
    a = canvas()
    disc(a, 16, 15, 8.0, 9.0, BLOSSOM)
    disc(a, 13, 12, 2.4, 3.0, WATER_L)
    disc(a, 10, 24, 5.0, 3.0, LF_M)
    disc(a, 22, 24, 5.0, 3.0, LF_M)
    a[24:30, 15:17] = LF_D
    return to_rgba(outline(a))


def sky_lily():
    """A white sky lily, gold at its heart: while it lasts she can jump once
    more in the air. Hard to reach on purpose."""
    a = canvas()
    a[22:32, 15:17] = LF_M
    disc(a, 11, 27, 4.5, 2.0, LF_M)
    disc(a, 21, 28, 4.5, 2.0, LF_L)
    for k in range(6):
        ang = np.pi / 2 + k * np.pi / 3
        for r in range(11):
            w = 2.8 * (1.0 - r / 12.0) + 0.5
            x, y = 16 + np.cos(ang) * r, 14 - np.sin(ang) * r * 0.85
            disc(a, x, y, w, w, ST_L)
            if 2 <= r <= 7:
                disc(a, x, y, 0.6, 0.6, WATER_L)
    disc(a, 16, 14, 3.0, 3.0, BLOSSOM)
    for dx, dy in ((-2, -3), (2, -3), (0, 1)):
        disc(a, 16 + dx, 14 + dy, 1.0, 1.0, GOLD)
    return to_rgba(outline(a))


TRINKETS = (
    ("gold", gold_coin), ("silver", silver_coin), ("flower", cut_flower),
    ("critter", critter), ("life", life_heart), ("swift", swift_leaf),
    ("might", might_berry), ("veil", veil_orb), ("charm", wizard_charm),
    ("spring", spring_bud), ("crown", thorn_crown),
    ("thorns", thorn_bundle), ("spread", spread_fan), ("pierce", pierce_seed),
    ("gale", gale_leaf), ("bloom", bloom_bud),
    ("lily", sky_lily),
)


# ---------------------------------------------------------------------------
#  Ground hazards, on their own palette: fire, sludge, a leaking toxic drum
#  and iron spikes. Each sits on the road along the bottom of a 32x32 block
#  and has two frames the game alternates so the fire flickers, the sludge
#  bubbles and the gas drifts.
# ---------------------------------------------------------------------------

HAZARD = np.array([
    (0, 0, 0),          # 0  transparent
    (30, 18, 22),       # 1  outline
    (120, 20, 16),      # 2  ember
    (208, 48, 24),      # 3  flame red
    (248, 128, 32),     # 4  flame orange
    (255, 216, 72),     # 5  flame yellow
    (255, 248, 200),    # 6  white heat
    (58, 56, 70),       # 7  iron shadow
    (110, 106, 120),    # 8  iron
    (176, 172, 186),    # 9  iron light
    (150, 76, 40),      # 10 rust
    (34, 70, 24),       # 11 sludge dark
    (84, 150, 30),      # 12 sludge
    (168, 224, 60),     # 13 sludge light
    (130, 70, 160),     # 14 toxic gas
    (196, 150, 220),    # 15 toxic gas light
], dtype=np.uint8)

HZ_OUT, EMBER, FL_R, FL_O, FL_Y, FL_W = 1, 2, 3, 4, 5, 6
IR_D, IR_M, IR_L, RUST = 7, 8, 9, 10
SL_D, SL_M, SL_L, GAS, GAS_L = 11, 12, 13, 14, 15


def _flame(a, cx, height, lean):
    """One tongue of flame rising from the ember bed: red skin, orange
    body, a yellow core and a white-hot root."""
    base = 27
    for layer, (color, shrink) in enumerate(((FL_R, 0.0), (FL_O, 0.35), (FL_Y, 0.62))):
        h = height * (1.0 - shrink * 0.55)
        for y in range(int(base - h), base + 1):
            t = min(1.0, (base - y) / max(h, 1.0))   # 0 at the root, 1 at the tip
            half = (1.0 - shrink) * 4.2 * (1.0 - t) ** 0.8 + 0.4
            x = cx + lean * t * t * 3.0
            x0, x1 = int(round(x - half)), int(round(x + half))
            if x1 >= x0:
                a[y, max(0, x0):min(32, x1 + 1)] = color
    a[base - 2:base + 1, cx - 1:cx + 2] = FL_W


def fire_bed(frame):
    a = canvas()
    heights = ((17, 23, 14, 20), (21, 16, 22, 15))[frame]
    leans = ((-1, 1, -1, 1), (1, -1, 1, -1))[frame]
    for cx, h, lean in zip((6, 13, 20, 27), heights, leans):
        _flame(a, cx, h, lean)
    box(a, 0, 27, 32, 32, EMBER)
    for x in range(0, 32, 3):
        a[28 + (x + frame) % 3, x] = FL_O
        a[30, (x + 1 + frame) % 32] = FL_Y
    return to_rgba(outline(a, HZ_OUT), HAZARD)


def sludge_pool(frame):
    a = canvas()
    disc(a, 16, 28, 16, 5, SL_D)
    disc(a, 16, 27, 14, 3.6, SL_M)
    a[25, 7:15] = SL_L
    a[26, 18:23] = SL_L
    bubbles = (((9, 24, 2.2), (22, 23, 1.6)), ((12, 21, 1.4), (24, 25, 2.4)))[frame]
    for bx, by, r in bubbles:
        disc(a, bx, by, r, r, SL_M)
        a[int(by - r * 0.5), int(bx - r * 0.3)] = SL_L
    if frame:
        a[18, 11:14] = SL_L          # a bubble popping
        a[17, 12] = SL_L
    return to_rgba(outline(a, HZ_OUT), HAZARD)


def toxic_drum(frame):
    """A leaking waste drum: a rusted barrel, a hazard band, a green spill
    at its foot and a violet cloud seeping from the lid."""
    a = canvas()
    box(a, 7, 12, 21, 30, IR_M)
    box(a, 7, 12, 9, 30, IR_L)
    box(a, 18, 12, 21, 30, IR_D)
    for y in (15, 26):
        box(a, 6, y, 22, y + 2, IR_D)
    box(a, 7, 19, 21, 23, FL_Y)
    for x in range(8, 21, 3):
        a[19:23, x] = HZ_OUT
    a[13, 11:15] = RUST
    a[24, 16:19] = RUST
    disc(a, 22, 30, 9, 2.4, SL_M)
    a[29, 17:24] = SL_L
    puffs = (((13, 8, 4.0), (19, 4, 3.0)), ((11, 5, 3.2), (18, 8, 4.2)))[frame]
    for px, py, r in puffs:
        disc(a, px, py, r, r * 0.8, GAS)
        disc(a, px - 1, py - 1, r * 0.45, r * 0.4, GAS_L)
    return to_rgba(outline(a, HZ_OUT), HAZARD)


def iron_spikes(frame):
    a = canvas()
    box(a, 0, 27, 32, 32, IR_D)
    a[27, :] = IR_M
    for i, cx in enumerate((5, 16, 27)):
        top = 8 + (i % 2) * 4
        for y in range(top, 27):
            half = (y - top) * 4.5 / (27 - top)
            x0, x1 = int(round(cx - half)), int(round(cx + half))
            a[y, x0:x1 + 1] = IR_M
            a[y, x0:max(x0 + 1, cx)] = IR_L
            a[y, x1] = IR_D
        a[top, cx] = IR_L
        a[top + 1, cx] = FL_W if frame else IR_L     # a glint that comes and goes
        a[22:25, cx - 2:cx] = RUST
    return to_rgba(outline(a, HZ_OUT), HAZARD)


def warn_sign(frame):
    """A caution board on a post, planted at the near edge of a pit: a
    yellow triangle, dark banding, a mark that catches the light."""
    a = canvas()
    a[21:31, 15:18] = IR_D
    a[21:31, 16] = IR_M
    a[26:28, 13:20] = RUST
    for y in range(4, 21):
        half = int((y - 3) * 0.62)
        a[y, 16 - half:17 + half] = FL_Y
        a[y, 16 - half] = HZ_OUT
        a[y, 16 + half] = HZ_OUT
    a[20, 4:29] = HZ_OUT
    a[8:15, 15:17] = HZ_OUT
    a[17:19, 15:17] = HZ_OUT
    a[7, 15] = FL_W if frame else FL_Y     # a glint that comes and goes
    return to_rgba(outline(a, HZ_OUT), HAZARD)


HAZARDS = (
    ("fire0", lambda: fire_bed(0)), ("fire1", lambda: fire_bed(1)),
    ("sludge0", lambda: sludge_pool(0)), ("sludge1", lambda: sludge_pool(1)),
    ("toxic0", lambda: toxic_drum(0)), ("toxic1", lambda: toxic_drum(1)),
    ("spikes0", lambda: iron_spikes(0)), ("spikes1", lambda: iron_spikes(1)),
    ("sign0", lambda: warn_sign(0)), ("sign1", lambda: warn_sign(1)),
)


# ---------------------------------------------------------------------------
#  Pits: a hole cut into the road, seen from a little above -- a broken lip,
#  the far wall's earth strata darkening with depth, side walls leaning in,
#  and at the bottom whatever the valley holds: water, toxic sludge, fire or
#  a bottomless void. Each theme has its own palette; widths 32, 48 and 64.
# ---------------------------------------------------------------------------

PIT_THEMES = {
    # 1 unused (was a caution stripe on the lip), 2-5 earth light->dark,
    # 6-7 wall, 8-12 content dark->light, 13-15 glints
    "water": [(0, 0, 0), (255, 200, 30), (138, 100, 64), (104, 72, 44), (74, 50, 32), (48, 32, 22),
              (60, 48, 40), (34, 26, 24), (8, 24, 60), (14, 48, 110), (24, 88, 170), (70, 150, 220),
              (150, 210, 245), (230, 246, 255), (120, 190, 235), (40, 120, 200)],
    "toxic": [(0, 0, 0), (255, 200, 30), (120, 104, 72), (92, 78, 54), (66, 54, 38), (44, 36, 26),
              (56, 50, 40), (32, 28, 24), (12, 36, 8), (30, 78, 12), (70, 140, 20), (140, 210, 40),
              (210, 250, 110), (245, 255, 200), (170, 240, 70), (90, 170, 30)],
    "fire":  [(0, 0, 0), (255, 200, 30), (132, 88, 56), (100, 64, 40), (72, 44, 28), (46, 28, 18),
              (70, 40, 30), (40, 22, 18), (70, 10, 6), (150, 30, 10), (220, 80, 20), (250, 150, 40),
              (255, 220, 90), (255, 250, 210), (255, 190, 60), (200, 60, 20)],
    "void":  [(0, 0, 0), (255, 200, 30), (120, 116, 140), (88, 84, 108), (60, 56, 80), (38, 34, 56),
              (46, 42, 64), (26, 22, 40), (4, 4, 10), (12, 10, 24), (26, 20, 48), (54, 40, 90),
              (100, 80, 150), (190, 170, 230), (140, 110, 200), (70, 56, 120)],
}


def _pit_lean(y, half_w):
    """How far the side walls step in by row `y`: not at all. The hole goes
    straight down past the bottom of the screen at its full width -- the
    width Maiya actually falls through. A mouth that bowed in to a point
    left road showing under both of its corners, so the pit read as a cup
    set into the ground rather than a gap in it."""
    return 0.0


def pit_hole(theme, width, frame):
    """One pit, `width` pixels across and 32 deep, as an indexed image."""
    a = np.zeros((32, width), dtype=np.uint8)
    rng = np.random.default_rng(width * 7 + len(theme))
    half_w = width / 2.0
    for x in range(width):
        edge = min(x, width - 1 - x)
        for y in range(32):
            lean = _pit_lean(y, half_w)
            if edge < lean:
                continue
            if edge <= lean + 2:
                a[y, x] = 6 if edge - lean < 1 else 7           # side wall: lit face, then shadow
            elif y < 10:
                band = (y + (x * 3 + int(rng.integers(0, 3))) // 11) // 3
                a[y, x] = min(5, 3 + band)                      # far wall strata, biased dark --
                                                                 # a void first, bare earth second
            else:
                a[y, x] = 8                                      # the depths, before contents fill it in
        # A hard dark crease is the real edge -- the ground doesn't fade
        # into the hole, it stops. No paint on it: the caution sign beside
        # the pit does the warning, and flecks along the lip only showed as
        # a dotted yellow line.
        a[0:2, x] = 7
    # rocks set into the far wall
    for x in range(4, width - 4, 9):
        y = 4 + (x * 13) % 7
        if min(x, width - 1 - x) >= _pit_lean(y, half_w) + 3:
            a[y:y + 2, x:x + 3] = 5
            a[y, x] = 3
    # The contents: bound by the same bowed walls as everything else above,
    # so the water/fire/void sits inside the hole instead of squaring off
    # underneath it.
    surf = 15 + (frame & 1)
    # Foam, bubbles and flame tongues fall at scattered places, not every
    # Nth pixel: evenly spaced they lined up into a dotted rule.
    spot = np.random.default_rng(width * 31 + len(theme) * 7 + frame).random(width)
    for x in range(width):
        edge = min(x, width - 1 - x)
        if theme == "void":
            for y in range(14, 32):
                if edge < _pit_lean(y, half_w) + 2:
                    continue
                a[y, x] = 8 if y > 24 else (9 if y > 19 else 10)
            if edge >= _pit_lean(20, half_w) + 2 and (x + frame * 4) % 13 < 4:
                a[18 + (x % 3), x] = 11                         # drifting mist
            if edge >= _pit_lean(26, half_w) + 2 and (x * 7 + frame * 5) % 23 == 0:
                a[26, x] = 13                                    # a far glint
        else:
            if edge < _pit_lean(surf, half_w) + 2:
                continue
            a[surf, x] = 12
            a[surf + 1, x] = 11
            for y in range(surf + 2, 32):
                if edge < _pit_lean(y, half_w) + 2:
                    continue
                a[y, x] = 10 if y < surf + 6 else (9 if y < surf + 11 else 8)
            if theme == "water" and spot[x] < 0.12:
                a[surf, x] = 13                                  # foam
                a[surf + 4, x - 1:x + 2] = 11                    # ripple
            if theme == "toxic" and spot[x] < 0.09:
                a[surf - 1, x] = 12                              # bubble
                a[surf - 2, x] = 13
                a[surf + 5, x] = 12
            if theme == "fire" and spot[x] < 0.22:
                h = 3 + (x * 7 + frame) % 5                      # flame tongues
                a[surf - h:surf, x] = 12
                a[surf - h, x] = 13
                a[surf - 1:surf + 1, x - 1:x + 2] = 11
    return a


def pit_blocks(theme):
    """The 32 x 32 blocks a pit of each width is drawn from, two frames each:
    S (32), L48/R48 (48), L64/R64 (64). The right-hand 48 block is half
    empty, so a 48-pixel pit ends exactly where it should."""
    pal = np.array(PIT_THEMES[theme], dtype=np.uint8)
    out = {}
    for frame in (0, 1):
        s = pit_hole(theme, 32, frame)
        out[f"s{frame}"] = to_rgba(s, pal)
        h48 = pit_hole(theme, 48, frame)
        r48 = np.zeros((32, 32), dtype=np.uint8)
        r48[:, :16] = h48[:, 32:]
        out[f"l48_{frame}"] = to_rgba(h48[:, :32], pal)
        out[f"r48_{frame}"] = to_rgba(r48, pal)
        h64 = pit_hole(theme, 64, frame)
        out[f"l64_{frame}"] = to_rgba(h64[:, :32], pal)
        out[f"r64_{frame}"] = to_rgba(h64[:, 32:], pal)
    return out


# --------------------------------------------------------------------------
#  Themed ledges: each valley builds its shelves out of its own stuff
# --------------------------------------------------------------------------
#
# A piece is 32 x 32: 0 the left end, 1 a middle that repeats, 2 the right
# end. The surface she stands on is the top few rows, as for the blocks.

def _finish(a, pal, piece, open_sides=True):
    """Outline the drawing; a middle piece stays open to its neighbours."""
    solid = a > 0
    pad = np.pad(solid, 1)
    edge = (pad[:-2, 1:-1] | pad[2:, 1:-1] | pad[1:-1, :-2] | pad[1:-1, 2:])
    a[(~solid) & edge] = 1
    if open_sides:
        if piece != 0:
            a[:, 0][a[:, 0] == 1] = 0
        if piece != 2:
            a[:, 31][a[:, 31] == 1] = 0
    rgb = np.asarray(pal, dtype=np.uint8)[a]
    return np.dstack((rgb, np.where(a > 0, 255, 0).astype(np.uint8))).astype(np.uint8)


def _round_ends(a, piece, top, bottom, r=4):
    """Round the outer corners of an end piece."""
    ys, xs = np.mgrid[0:32, 0:32]
    if piece == 0:
        for cy in (top + r, bottom - r):
            corner = (xs < r) & ((ys < top + r) if cy == top + r else (ys > bottom - r))
            a[corner & (((xs - r) ** 2 + (ys - cy) ** 2) > r * r)] = 0
    if piece == 2:
        for cy in (top + r, bottom - r):
            corner = (xs > 31 - r) & ((ys < top + r) if cy == top + r else (ys > bottom - r))
            a[corner & (((xs - (31 - r)) ** 2 + (ys - cy) ** 2) > r * r)] = 0


def pier_piece(piece):
    """The coast: a plank deck on a post, a coil of rope round the post."""
    pal = [(0, 0, 0), (40, 28, 24), (240, 206, 150), (214, 166, 108), (170, 122, 76),
           (116, 80, 50), (74, 52, 34), (222, 204, 150), (70, 70, 84)]
    a = canvas()
    a[2:12, :] = 4                      # plank faces
    a[2:4, :] = 2                       # the lit top of the deck
    a[4:6, :] = 3
    for x in range(3, 32, 8):           # plank seams
        a[4:12, x] = 5
    a[10:13, :] = 5                     # the fascia board under the planks
    a[12:13, :] = 6
    for x in (6, 22):                   # nails
        a[7, x] = 8
    a[13:32, 12:20] = 4                 # the post
    a[13:32, 12:14] = 3
    a[13:32, 18:20] = 5
    a[13:32, 19:20] = 6
    for y in range(18, 26, 2):          # rope wound round it
        a[y, 11:21] = 7
        a[y + 1, 11:21][a[y + 1, 11:21] > 0] = 5
    _round_ends(a, piece, 2, 12, r=3)
    return _finish(a, pal, piece)


def beam_piece(piece):
    """The grove: a red-lacquered beam under a strip of dark roof tiles."""
    pal = [(0, 0, 0), (30, 20, 24), (130, 130, 150), (82, 82, 100), (52, 50, 64),
           (246, 118, 84), (214, 62, 42), (150, 36, 30), (100, 24, 22), (244, 204, 92)]
    a = canvas()
    a[2:7, :] = 3                       # roof tiles
    a[2, :] = 2
    for x in range(0, 32, 6):
        a[3:7, x] = 4                   # tile ridges
    a[6:7, :] = 4
    a[7:17, :] = 6                      # the beam
    a[7:9, :] = 5
    a[15:17, :] = 7
    a[16, :] = 8
    if piece != 1:                      # a gold cap on the ends, and a post
        x0 = 0 if piece == 0 else 24
        a[8:16, x0:x0 + 8] = 9
        a[10:14, x0 + 2:x0 + 6] = 6
        a[17:32, x0 + 1:x0 + 7] = 6
        a[17:32, x0 + 1:x0 + 3] = 5
        a[17:32, x0 + 5:x0 + 7] = 7
    else:                               # a paper lantern hangs from the middle
        a[17:19, 15:17] = 4
        disc(a, 16, 24, 5, 5, 9)
        a[20:28, 13:19][a[20:28, 13:19] > 0] = 9
        a[21:27, 14:18] = 5
        a[29:31, 15:17] = 8
    _round_ends(a, piece, 2, 16, r=2)
    return _finish(a, pal, piece)


def ice_piece(piece):
    """The grotto: a block of clear ice capped with snow, icicles below."""
    pal = [(0, 0, 0), (30, 40, 72), (250, 252, 255), (206, 222, 242), (196, 238, 252),
           (132, 204, 238), (78, 150, 206), (44, 96, 156), (255, 255, 255)]
    a = canvas()
    a[4:22, :] = 5                      # ice
    a[16:22, :] = 6
    a[20:22, :] = 7
    ys, xs = np.mgrid[0:32, 0:32]
    a[(a == 5) & (((xs + ys) % 11) < 2) & (ys < 16)] = 4     # glints running through it
    a[(a == 5) & (((xs - ys) % 13) == 0)] = 6
    for x in range(32):                 # a lumpy snow cap
        top = 1 + ((x * 5 + piece * 3) % 3 == 0)
        a[top:6, x] = 3
        a[top:top + 2, x] = 2
    for x0, length in ((5, 8), (13, 5), (21, 9), (27, 4)):   # icicles
        if piece == 0 and x0 < 6:
            continue
        for k in range(length):
            half = max(0, 2 - k * 2 // max(1, length))
            a[22 + k, x0 - half:x0 + half + 1] = 5 if k < length - 2 else 4
    a[8, 6] = a[9, 7] = a[12, 20] = 8
    _round_ends(a, piece, 1, 21, r=3)
    return _finish(a, pal, piece)


def branch_piece(piece):
    """The world tree: a great mossy branch; a cut end on the left, the
    branch thinning into a leafy twig on the right."""
    pal = [(0, 0, 0), (30, 22, 18), (148, 206, 92), (84, 152, 62), (206, 164, 112),
           (158, 116, 76), (114, 80, 52), (74, 52, 34), (46, 32, 22), (58, 116, 50)]
    a = canvas()
    top, bottom = 4, 19
    a[top:bottom, :] = 6                # the bark, lit on top and dark beneath
    a[top:top + 3, :] = 5
    a[bottom - 4:bottom, :] = 7
    a[bottom - 1:bottom, :] = 8
    ys, xs = np.mgrid[0:32, 0:32]
    bark = (ys > top + 2) & (ys < bottom - 1)
    a[bark & (((xs * 3 + ys * 7) % 19) == 0)] = 7                # bark grain
    a[bark & (((xs * 5 + ys) % 23) == 0)] = 8
    for x in range(32):                 # moss along the top
        depth = 2 + ((x * 7 + 3) % 3)
        a[top - 2:top - 2 + depth, x] = 3
        a[top - 2, x] = 2 if (x % 5) else 3
    if piece == 0:                      # the cut end: rings
        disc(a, 5, 11, 5, 8, 4)
        disc(a, 5, 11, 3, 5, 5)
        disc(a, 5, 11, 1, 2, 4)
        a[:, 0] = np.where(a[:, 0] > 0, a[:, 0], 0)
    if piece == 2:                      # tapering off into a twig
        taper = (xs > 18) & ((ys < top + (xs - 18) // 2) | (ys > bottom - 1 - (xs - 18) // 3))
        a[taper] = 0
        disc(a, 27, 7, 4, 3, 9)
        disc(a, 24, 3, 3, 2, 3)
    else:                               # a few leaves hanging under the branch
        disc(a, 10 + piece * 6, 21, 2, 3, 9)
        disc(a, 23, 22, 2, 3, 3)
    return _finish(a, pal, piece)


def girder_piece(piece):
    """The works: a riveted steel I-beam, rusting, hazard stripes at the ends."""
    pal = [(0, 0, 0), (24, 24, 30), (196, 202, 212), (136, 142, 156), (86, 90, 104),
           (50, 52, 62), (174, 96, 54), (112, 58, 36), (242, 202, 44)]
    a = canvas()
    a[2:6, :] = 3                       # top flange
    a[2, :] = 2
    a[5, :] = 4
    a[6:16, 4:28] = 4                   # the web
    a[6:16, :] = 4
    for cx in (8, 24):                  # lightening holes
        disc(a, cx, 11, 3, 3, 0)
    for x in range(2, 32, 6):           # rivets
        a[3, x] = 5
        a[4, x] = 2
    a[16:19, :] = 3                     # bottom flange
    a[18, :] = 5
    ys, xs = np.mgrid[0:32, 0:32]
    a[(a == 4) & (((xs * 7 + ys * 3) % 17) == 0)] = 6          # rust streaks
    a[(a == 3) & (((xs * 5 + ys * 11) % 29) == 0)] = 7
    if piece != 1:
        x0 = 0 if piece == 0 else 26
        stripes = (xs >= x0) & (xs < x0 + 6) & (ys >= 6) & (ys < 16)
        a[stripes & (((xs + ys) // 2) % 2 == 0)] = 8
        a[stripes & (((xs + ys) // 2) % 2 == 1)] = 5
    return _finish(a, pal, piece)


def coral_piece(piece):
    """The reef: a shelf of rock crowned with bumps of living coral."""
    pal = [(0, 0, 0), (20, 24, 40), (255, 178, 176), (238, 112, 124), (172, 62, 84),
           (255, 176, 84), (112, 144, 176), (72, 98, 132), (42, 60, 90), (130, 220, 216)]
    a = canvas()
    a[8:22, :] = 7                      # reef rock
    a[8:11, :] = 6
    a[18:22, :] = 8
    ys, xs = np.mgrid[0:32, 0:32]
    a[(a == 7) & (((xs * 3 + ys * 5) % 13) == 0)] = 8            # pits in the rock
    for k, cx in enumerate(range(3 + piece, 32, 7)):             # coral heads along the top
        disc(a, cx, 7, 4, 4 + (k % 2), 3)
        disc(a, cx - 1, 6, 2, 2, 2)
        if cx + 1 < 32:
            a[7 + (k % 2), cx + 1] = 4
    for cx in (9 + piece * 3, 22):                               # polyps and an anemone
        a[4, cx] = 5
        a[3, cx + 1] = 5
    for x in range(11, 17):
        a[22:25 + (x % 3), x] = 9 if x % 2 else 0
    # the underside breaks off unevenly
    for x in range(32):
        cut = 21 - ((x * 7 + piece) % 4)
        a[cut + 1:23, x] = np.where(a[cut + 1:23, x] == 9, 9, 0)
    _round_ends(a, piece, 3, 21, r=4)
    return _finish(a, pal, piece)


def scaffold_piece(piece):
    """The mine: planks on a timber frame braced with an X, a seep of the
    glowing slime dripping from one edge."""
    pal = [(0, 0, 0), (26, 20, 18), (212, 176, 122), (160, 120, 78), (104, 76, 50),
           (130, 98, 64), (74, 52, 34), (152, 228, 88), (150, 150, 160)]
    a = canvas()
    a[2:9, :] = 3                       # planks
    a[2:4, :] = 2
    for x in range(5, 32, 10):
        a[4:9, x] = 4
    a[9:12, :] = 6                      # the frame's cross beam
    for x0 in (1, 27):                  # uprights
        a[12:32, x0:x0 + 4] = 5
        a[12:32, x0 + 3] = 6
    ys, xs = np.mgrid[0:32, 0:32]
    band = (ys >= 12) & (xs >= 5) & (xs <= 26)
    a[band & (np.abs((xs - 5) - (ys - 12) * 22 // 19) <= 1)] = 5              # the X brace
    a[band & (np.abs((26 - xs) - (ys - 12) * 22 // 19) <= 1)] = 5
    a[10, 3] = a[10, 29] = 8            # bolts
    if piece == 1:                      # slime seeping off the edge
        a[9:15, 20:22] = 7
        a[15:18, 20] = 7
    return _finish(a, pal, piece)


def slab_piece(piece):
    """The savanna: a slab of layered red sandstone, dry grass on top."""
    pal = [(0, 0, 0), (40, 22, 20), (240, 208, 114), (192, 150, 70), (250, 214, 166),
           (236, 170, 120), (200, 120, 80), (150, 80, 56), (100, 52, 40)]
    a = canvas()
    a[4:20, :] = 6
    a[4:7, :] = 5
    a[4, :] = 4
    for y in (10, 14, 17):              # strata
        a[y, :] = 7
    a[18:20, :] = 8
    ys, xs = np.mgrid[0:32, 0:32]
    a[(a == 6) & (((xs * 5 + ys * 3) % 23) == 0)] = 7            # cracks and grit
    for x in range(32):                 # an uneven underside
        cut = 19 - ((x * 5 + piece * 2) % 4)
        a[cut + 1:21, x] = 0
    for x in range(1, 32, 3):           # tufts of dry grass along the top
        h = 2 + ((x * 7 + piece) % 3)
        a[4 - h:4, x] = 2
        if x + 1 < 32:
            a[5 - h:4, x + 1] = 3
    _round_ends(a, piece, 4, 19, r=3)
    return _finish(a, pal, piece)


LEDGE_STYLES = {
    "sand": pier_piece, "autumn": beam_piece, "snow": ice_piece, "bark": branch_piece,
    "rust": girder_piece, "coral": coral_piece, "stone": scaffold_piece, "savanna": slab_piece,
}


def ledge_piece(biome, piece):
    """A valley's ledge piece: its own structure where it has one, else the
    grassy block."""
    painter = LEDGE_STYLES.get(biome)
    return painter(piece) if painter else ledge_block(biome, piece)
