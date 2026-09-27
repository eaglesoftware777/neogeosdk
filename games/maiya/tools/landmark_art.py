"""The great landmarks the Ancient Nature Gate stands in, one for each valley
with a gate: 192 x 176 px, the gate's recess at the foot of each, centred
(the gate sprite, 32 x 48, is drawn over it at x 80..112 of the picture).

Drawn here in code, like the rest of nature_art.py: shapes filled from a
small palette of each landmark's own, leaf and stone texture scattered from
a fixed seed, and a dark rim round the whole. The art build quantizes each
to three palette banks.
"""
from __future__ import annotations

import math

import numpy as np
from PIL import Image, ImageDraw

W, H = 192, 176
RIM = (26, 20, 30)
DOOR_X, DOOR_W, DOOR_H = 96, 44, 60     # the recess the gate stands in


class Pic:
    def __init__(self, colors: dict[str, tuple[int, int, int]], seed: int):
        self.names = list(colors)
        self.pal = [(0, 0, 0)] + [colors[k] for k in self.names]
        self.img = Image.new("L", (W, H), 0)
        self.d = ImageDraw.Draw(self.img)
        self.rng = np.random.default_rng(seed)

    def c(self, name: str) -> int:
        return self.names.index(name) + 1

    def ell(self, cx, cy, rx, ry, col):
        self.d.ellipse([cx - rx, cy - ry, cx + rx, cy + ry], fill=self.c(col))

    def poly(self, pts, col):
        self.d.polygon([(float(x), float(y)) for x, y in pts], fill=self.c(col))

    def rect(self, x0, y0, x1, y1, col):
        self.d.rectangle([x0, y0, x1, y1], fill=self.c(col))

    def line(self, pts, col, width=1, clip=None):
        pts = [(float(x), float(y)) for x, y in pts]
        if clip is None:
            self.d.line(pts, fill=self.c(col), width=width)
            return
        tmp = Image.new("L", self.img.size, 0)
        ImageDraw.Draw(tmp).line(pts, fill=255, width=width)
        a = np.array(self.img)
        a[(np.array(tmp) > 0) & clip] = self.c(col)
        self.img = Image.fromarray(a)
        self.d = ImageDraw.Draw(self.img)

    def inside(self) -> np.ndarray:
        return np.array(self.img) > 0

    def speckle(self, mask, col, n, size=1.2):
        """Small dabs of `col` scattered where `mask` is set."""
        ys, xs = np.nonzero(mask)
        if len(xs) == 0:
            return
        for k in self.rng.choice(len(xs), size=min(n, len(xs)), replace=False):
            self.ell(xs[k], ys[k], size, size * 0.8, col)

    def area(self, col) -> np.ndarray:
        return np.array(self.img) == self.c(col)

    def cluster(self, cx, cy, r, dark, mid, light, dots=None):
        """A clump of leaves (or coral, or cloud of blossom): dark body,
        lit toward the upper left, broken up at the edge."""
        self.ell(cx, cy, r, r * 0.86, dark)
        self.ell(cx - r * 0.12, cy - r * 0.16, r * 0.84, r * 0.7, mid)
        self.ell(cx - r * 0.34, cy - r * 0.4, r * 0.42, r * 0.32, light)
        for _ in range(int(r * 1.4)):
            a = self.rng.uniform(0, 2 * math.pi)
            d = r * self.rng.uniform(0.75, 1.02)
            x, y = cx + math.cos(a) * d, cy + math.sin(a) * d * 0.86
            self.ell(x, y, 2.2, 1.8, dark if math.sin(a) > -0.3 else mid)
        for _ in range(int(r * 0.9)):
            a = self.rng.uniform(0, 2 * math.pi)
            d = r * self.rng.uniform(0.1, 0.7)
            x, y = cx - r * 0.15 + math.cos(a) * d, cy - r * 0.2 + math.sin(a) * d * 0.7
            self.ell(x, y, 1.3, 1.0, light if y < cy - r * 0.25 else dark)
        if dots:
            for _ in range(int(r * 0.5)):
                a = self.rng.uniform(0, 2 * math.pi)
                d = r * self.rng.uniform(0.2, 0.9)
                self.ell(cx + math.cos(a) * d, cy + math.sin(a) * d * 0.8, 1.5, 1.5, dots)

    def door(self, dark, shade):
        """The recess the gate stands in, at the foot, in the middle."""
        x0, x1 = DOOR_X - DOOR_W // 2, DOOR_X + DOOR_W // 2
        self.d.ellipse([x0, H - DOOR_H, x1, H - DOOR_H + DOOR_W], fill=self.c(shade))
        self.rect(x0, H - DOOR_H + DOOR_W // 2, x1, H, shade)
        self.d.ellipse([x0 + 4, H - DOOR_H + 4, x1 - 4, H - DOOR_H + DOOR_W - 4], fill=self.c(dark))
        self.rect(x0 + 4, H - DOOR_H + DOOR_W // 2, x1 - 4, H, dark)

    def rgba(self) -> np.ndarray:
        a = np.array(self.img)
        rgb = np.array(self.pal, dtype=np.uint8)[a]
        solid = a > 0
        pad = np.pad(solid, 1)
        edge = solid & ~(pad[:-2, 1:-1] & pad[2:, 1:-1] & pad[1:-1, :-2] & pad[1:-1, 2:])
        rgb[edge] = RIM
        return np.dstack([rgb, np.where(solid, 255, 0)]).astype(np.uint8)


def bark(p: Pic, pts, dark, mid, light, grooves=9):
    """A trunk: its outline filled, grooves running up it, lit on the left."""
    p.poly(pts, mid)
    body = p.area(mid)
    xs = np.nonzero(body.any(axis=0))[0]
    x0, x1 = xs.min(), xs.max()
    for k in range(grooves):
        x = x0 + (x1 - x0) * (k + 0.5) / grooves + p.rng.uniform(-3, 3)
        pts2 = [(x + math.sin(y * 0.09 + k) * 2.5, y) for y in range(0, H, 6)]
        p.line(pts2, dark, 1, clip=body)
    lit = body & (np.arange(W)[None, :] < x0 + (x1 - x0) * 0.3)
    p.speckle(lit & body, light, 60, 1.0)


def roots(p: Pic, col, cx, spread, n=5):
    for k in range(n):
        t = (k / (n - 1)) * 2 - 1
        x = cx + t * spread
        p.poly([(cx + t * 18 - 8, H - 34), (cx + t * 18 + 8, H - 34), (x + 10, H), (x - 10, H)], col)


# ---------------------------------------------------------------------------

def forest():
    p = Pic({"bk_d": (60, 38, 30), "bk_m": (106, 72, 46), "bk_l": (156, 112, 70), "moss": (96, 146, 62),
             "lf_d": (28, 82, 48), "lf_m": (56, 136, 66), "lf_l": (130, 198, 90), "bl": (244, 148, 186),
             "sh": (22, 16, 22), "sh2": (48, 32, 30)}, 11)
    bark(p, [(60, H), (70, 120), (78, 70), (86, 44), (106, 44), (114, 70), (122, 120), (132, H)], "bk_d", "bk_m", "bk_l")
    roots(p, "bk_m", 96, 78, 6)
    p.line([(84, 70), (48, 40), (30, 34)], "bk_m", 7)
    p.line([(108, 66), (146, 38), (166, 32)], "bk_m", 7)
    for cx, cy, r in ((96, 34, 40), (46, 50, 32), (148, 48, 32), (20, 74, 22), (174, 72, 22),
                      (70, 18, 26), (126, 16, 26), (96, 70, 24)):
        p.cluster(cx, cy, r, "lf_d", "lf_m", "lf_l", dots="bl")
    p.speckle(p.area("bk_m") & (np.arange(H)[:, None] > 110), "moss", 50, 1.6)
    p.door("sh", "sh2")
    return p.rgba()


def autumn():
    p = Pic({"bk_d": (58, 36, 28), "bk_m": (100, 66, 44), "bk_l": (150, 106, 66),
             "lf_d": (150, 56, 28), "lf_m": (222, 118, 40), "lf_l": (252, 196, 88), "red": (196, 48, 40),
             "sh": (22, 16, 22), "sh2": (50, 32, 28)}, 12)
    bark(p, [(58, H), (70, 116), (80, 64), (90, 48), (104, 48), (114, 64), (124, 116), (134, H)], "bk_d", "bk_m", "bk_l")
    roots(p, "bk_m", 96, 74, 5)
    p.line([(86, 66), (40, 34)], "bk_m", 6)
    p.line([(106, 62), (156, 30)], "bk_m", 6)
    for cx, cy, r in ((96, 30, 38), (44, 44, 30), (150, 42, 30), (18, 70, 20), (176, 68, 20),
                      (70, 14, 24), (124, 12, 24)):
        p.cluster(cx, cy, r, "lf_d", "lf_m", "lf_l", dots="red")
    p.door("sh", "sh2")
    return p.rgba()


def falls():
    p = Pic({"rk_d": (58, 62, 72), "rk_m": (102, 108, 116), "rk_l": (150, 156, 160),
             "ms_d": (40, 92, 50), "ms_m": (76, 140, 66), "ms_l": (140, 196, 92),
             "wt_m": (86, 158, 222), "wt_l": (168, 218, 248), "foam": (240, 250, 255),
             "sh": (20, 18, 26), "sh2": (44, 46, 56)}, 13)
    p.poly([(4, H), (10, 60), (30, 34), (58, 26), (84, 12), (116, 16), (146, 30), (170, 40), (186, 70), (190, H)], "rk_m")
    rock = p.area("rk_m")
    for y in range(40, H, 18):
        p.line([(6, y + 6), (60, y), (120, y + 4), (188, y - 2)], "rk_d", 1, clip=rock)
    p.speckle(rock & (np.arange(W)[None, :] < 70), "rk_l", 70, 1.2)
    p.speckle(rock, "rk_d", 60, 1.0)
    for cx, cy, r in ((40, 36, 16), (78, 20, 16), (116, 22, 14), (160, 42, 14), (14, 70, 10)):
        p.cluster(cx, cy, r, "ms_d", "ms_m", "ms_l")
    p.rect(132, 30, 158, H - 8, "wt_m")
    for k in range(7):
        x = 134 + k * 4
        p.line([(x, 34 + (k * 13) % 20), (x, H - 10)], "wt_l", 1)
    for k in range(9):
        p.ell(130 + k * 4, H - 8, 5, 4, "foam")
    p.door("sh", "sh2")
    return p.rgba()


def coast():
    p = Pic({"sd_d": (140, 94, 60), "sd_m": (200, 150, 96), "sd_l": (238, 204, 146),
             "pt_d": (104, 70, 44), "pt_m": (150, 108, 64), "pl_d": (30, 110, 70), "pl_m": (62, 168, 88),
             "pl_l": (140, 214, 110), "shell": (250, 214, 220), "sw": (40, 120, 96),
             "sh": (24, 18, 22), "sh2": (70, 48, 36)}, 14)
    p.poly([(14, H), (22, 120), (34, 84), (50, 58), (72, 46), (122, 44), (146, 56), (162, 82), (172, 120), (180, H)], "sd_m")
    body = p.area("sd_m")
    for k, y in enumerate(range(58, H, 13)):
        p.line([(10, y), (60, y + 3), (96, y + (3 if k % 2 else -2)), (184, y)], "sd_d", 2 if k % 2 else 1, clip=body)
    p.speckle(body & (np.arange(W)[None, :] < 80), "sd_l", 80, 1.3)
    p.line([(98, 46), (102, 30), (108, 18), (116, 10)], "pt_d", 6)
    p.line([(99, 46), (103, 30), (109, 18), (116, 10)], "pt_m", 3)
    for cx, cy, r in ((104, 12, 14), (126, 10, 13), (86, 20, 11), (142, 22, 11), (116, 26, 10)):
        p.cluster(cx, cy, r, "pl_d", "pl_m", "pl_l")
    for x in (30, 58, 150, 168):
        p.poly([(x, H), (x + 4, H - 22), (x + 8, H)], "sw")
    for x, y in ((44, 160), (140, 164), (160, 150)):
        p.ell(x, y, 4, 3, "shell")
    p.door("sh", "sh2")
    return p.rgba()


def ice():
    p = Pic({"ic_d": (64, 100, 150), "ic_m": (122, 172, 216), "ic_l": (198, 234, 252), "snow": (246, 250, 255),
             "cr_m": (110, 214, 236), "cr_l": (220, 250, 255), "sh": (18, 22, 40), "sh2": (40, 60, 96)}, 15)
    p.poly([(2, H), (40, 70), (62, 50), (82, 18), (96, 4), (110, 22), (132, 46), (158, 74), (190, H)], "ic_m")
    p.poly([(96, 4), (110, 22), (132, 46), (158, 74), (190, H), (120, H)], "ic_d")
    p.poly([(82, 18), (96, 4), (110, 22), (104, 34), (96, 28), (88, 36)], "snow")
    p.poly([(40, 70), (62, 50), (70, 60), (54, 74)], "snow")
    body = p.area("ic_m")
    p.speckle(body, "ic_l", 70, 1.2)
    for x, h, w in ((22, 60, 12), (40, 44, 9), (150, 56, 12), (170, 40, 9), (134, 34, 7)):
        p.poly([(x - w, H), (x, H - h), (x + w, H)], "cr_m")
        p.poly([(x - w * 0.4, H), (x, H - h), (x + 1, H)], "cr_l")
    p.door("sh", "sh2")
    return p.rgba()


def worldtree():
    p = Pic({"bk_d": (54, 38, 34), "bk_m": (98, 70, 52), "bk_l": (146, 110, 76), "glow": (248, 208, 96),
             "glow_l": (255, 240, 180), "moss": (84, 140, 64), "lf_d": (30, 90, 52), "lf_m": (60, 146, 72),
             "sh": (20, 14, 18), "sh2": (48, 34, 30)}, 16)
    bark(p, [(0, H), (18, 110), (30, 40), (34, 0), (158, 0), (162, 40), (174, 110), (192, H)], "bk_d", "bk_m", "bk_l", 14)
    roots(p, "bk_m", 96, 92, 7)
    for k in range(5):
        x = 30 + k * 34
        p.line([(x, 150), (x + 6, 120), (x - 2, 90), (x + 4, 60)], "glow", 1)
        p.ell(x + 4, 60, 2, 2, "glow_l")
    p.speckle(p.area("bk_m") & (np.arange(H)[:, None] > 120), "moss", 70, 1.6)
    for cx in (20, 60, 132, 172):
        p.cluster(cx, 0, 18, "lf_d", "lf_m", "lf_m")
    p.door("sh", "sh2")
    return p.rgba()


def works():
    p = Pic({"br_d": (96, 44, 36), "br_m": (150, 74, 56), "br_l": (196, 116, 86), "st_d": (86, 88, 96),
             "st_m": (132, 134, 140), "vn_d": (34, 96, 52), "vn_m": (70, 150, 70), "vn_l": (140, 204, 96),
             "fl": (250, 200, 70), "sh": (22, 18, 20), "sh2": (60, 40, 36)}, 17)
    p.poly([(40, H), (56, 40), (136, 40), (152, H)], "br_m")
    wall = p.area("br_m")
    for y in range(44, H, 8):
        p.line([(40, y), (152, y)], "br_d", 1, clip=wall)
        off = 0 if (y // 8) % 2 else 8
        for x in range(44 + off, 152, 16):
            p.line([(x, y), (x, y + 7)], "br_d", 1, clip=wall)
    p.speckle(p.area("br_m") & (np.arange(W)[None, :] < 80), "br_l", 50, 1.0)
    p.rect(50, 26, 142, 40, "st_m")
    p.rect(50, 36, 142, 40, "st_d")
    p.rect(60, 96, 132, 104, "st_m")
    for x0 in (48, 128):
        p.line([(x0, H), (x0 + 6, 130), (x0 - 4, 90), (x0 + 4, 50)], "vn_d", 3)
    for cx, cy, r in ((58, 60, 10), (138, 70, 10), (50, 120, 9), (140, 130, 10), (96, 18, 16), (74, 22, 10), (118, 22, 10)):
        p.cluster(cx, cy, r, "vn_d", "vn_m", "vn_l", dots="fl")
    p.door("sh", "sh2")
    return p.rgba()


def reef():
    p = Pic({"rk_d": (60, 66, 90), "rk_m": (96, 104, 132), "pk_d": (180, 60, 100), "pk_m": (240, 120, 152),
             "pk_l": (255, 190, 206), "or_m": (250, 160, 86), "or_l": (255, 214, 150), "pu_m": (170, 104, 204),
             "bub": (200, 236, 255), "sh": (18, 18, 34), "sh2": (40, 44, 70)}, 18)
    p.poly([(16, H), (30, 110), (60, 84), (132, 84), (162, 110), (176, H)], "rk_m")
    p.speckle(p.area("rk_m"), "rk_d", 80, 1.3)

    def branch(x, y, ang, length, col, depth):
        if depth == 0 or length < 4:
            p.ell(x, y, 3, 3, col)
            return
        x2, y2 = x + math.cos(ang) * length, y - math.sin(ang) * length
        p.line([(x, y), (x2, y2)], col, depth * 2 + 1)
        branch(x2, y2, ang + 0.45, length * 0.72, col, depth - 1)
        branch(x2, y2, ang - 0.45, length * 0.72, col, depth - 1)

    branch(52, 96, 1.7, 30, "pk_m", 4)
    branch(140, 96, 1.45, 30, "or_m", 4)
    branch(96, 86, 1.57, 34, "pu_m", 4)
    for cx, cy, r, c1, c2, c3 in ((30, 104, 14, "pk_d", "pk_m", "pk_l"), (164, 106, 14, "or_m", "or_m", "or_l"),
                                  (120, 92, 10, "pu_m", "pu_m", "pk_l"), (70, 92, 10, "pk_d", "pk_m", "pk_l")):
        p.cluster(cx, cy, r, c1, c2, c3)
    p.speckle(p.area("pk_m"), "pk_l", 40, 1.0)
    p.speckle(p.area("or_m"), "or_l", 40, 1.0)
    for x, y, r in ((30, 60, 3), (160, 40, 4), (110, 20, 3), (70, 30, 2)):
        p.ell(x, y, r, r, "bub")
    p.door("sh", "sh2")
    return p.rgba()


def mountain():
    p = Pic({"st_d": (70, 72, 84), "st_m": (118, 120, 132), "st_l": (170, 172, 184), "silver": (226, 234, 244),
             "snow": (246, 250, 255), "pn_d": (26, 70, 52), "pn_m": (46, 110, 70), "sh": (16, 16, 22),
             "sh2": (46, 48, 58)}, 19)
    p.poly([(0, H), (24, 96), (52, 58), (76, 36), (96, 10), (118, 32), (146, 56), (170, 90), (192, H)], "st_m")
    p.poly([(96, 10), (118, 32), (146, 56), (170, 90), (192, H), (124, H), (110, 60)], "st_d")
    p.poly([(76, 36), (96, 10), (118, 32), (106, 40), (96, 32), (86, 44)], "snow")
    p.speckle(p.area("st_m"), "st_l", 70, 1.2)
    for k in range(6):
        x = 30 + k * 26
        p.line([(x, 150 - k * 6), (x + 8, 120 - k * 4), (x + 4, 100)], "silver", 1)
    for x in (12, 28, 164, 180):
        for k in range(3):
            p.poly([(x - 10 + k * 2, H - k * 10), (x, H - 26 - k * 10), (x + 10 - k * 2, H - k * 10)], "pn_d" if k % 2 else "pn_m")
    p.door("sh", "sh2")
    return p.rgba()


def baobab():
    p = Pic({"tr_d": (110, 84, 70), "tr_m": (156, 124, 100), "tr_l": (200, 168, 136),
             "lf_d": (70, 96, 40), "lf_m": (112, 146, 56), "lf_l": (176, 196, 90), "gr": (206, 176, 90),
             "sh": (26, 20, 18), "sh2": (70, 54, 44)}, 20)
    p.poly([(48, H), (40, 130), (46, 90), (62, 56), (76, 40), (116, 40), (130, 56), (146, 90), (152, 130), (144, H)], "tr_m")
    trunk = p.area("tr_m")
    for k in range(6):
        x = 58 + k * 16
        p.line([(x, H), (x + 2, 100), (x + 6, 50)], "tr_d", 1, clip=trunk)
    p.speckle(p.area("tr_m") & (np.arange(W)[None, :] < 90), "tr_l", 70, 1.3)
    p.line([(84, 44), (62, 26), (40, 18)], "tr_m", 6)
    p.line([(108, 44), (130, 24), (154, 16)], "tr_m", 6)
    p.line([(96, 42), (94, 26), (98, 14)], "tr_m", 5)
    for cx, cy, r in ((36, 16, 16), (98, 12, 15), (158, 14, 16), (66, 22, 12), (128, 20, 12)):
        p.cluster(cx, cy, r, "lf_d", "lf_m", "lf_l")
    for x in range(8, 190, 14):
        p.poly([(x - 5, H), (x, H - 12 - (x % 5)), (x + 5, H)], "gr")
    p.door("sh", "sh2")
    return p.rgba()


def citadel():
    p = Pic({"mt_d": (44, 38, 54), "mt_m": (80, 74, 94), "mt_l": (128, 122, 144), "rust": (148, 80, 52),
             "glow": (250, 120, 60), "glow_l": (255, 208, 120), "smog": (110, 104, 96), "sh": (14, 10, 16),
             "sh2": (60, 30, 30)}, 21)
    p.poly([(30, H), (40, 50), (60, 30), (132, 30), (152, 50), (162, H)], "mt_m")
    p.poly([(96, 30), (132, 30), (152, 50), (162, H), (96, H)], "mt_d")
    for x0 in (44, 70, 96, 122, 148):
        p.rect(x0 - 6, 14, x0 + 6, 32, "mt_m")
        p.rect(x0 - 6, 14, x0 + 6, 18, "mt_l")
    for y in range(52, 150, 24):
        for x in (58, 80, 112, 134):
            p.rect(x - 4, y, x + 4, y + 10, "glow")
            p.rect(x - 2, y + 2, x + 2, y + 5, "glow_l")
    for x, y in ((36, 90), (156, 110), (40, 140), (150, 60)):
        p.rect(x - 4, y - 16, x + 4, y + 16, "rust")
    p.speckle(p.area("mt_m"), "mt_l", 50, 1.0)
    for cx, cy, r in ((70, 6, 10), (124, 4, 12)):
        p.ell(cx, cy, r, r * 0.6, "smog")
    p.door("sh", "sh2")
    return p.rgba()


LANDMARKS = (
    ("forest", forest), ("falls", falls), ("coast", coast), ("autumn", autumn), ("ice", ice),
    ("worldtree", worldtree), ("works", works), ("reef", reef), ("mountain", mountain),
    ("baobab", baobab), ("citadel", citadel),
)


if __name__ == "__main__":
    import sys
    out = sys.argv[1] if len(sys.argv) > 1 else "landmarks.png"
    sheet = Image.new("RGBA", (W * 4, H * 3), (60, 110, 150, 255))
    for k, (name, fn) in enumerate(LANDMARKS):
        im = Image.fromarray(fn())
        sheet.alpha_composite(im, ((k % 4) * W, (k // 4) * H))
    sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save(out)
