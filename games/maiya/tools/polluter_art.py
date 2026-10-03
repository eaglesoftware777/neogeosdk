"""The blight's machines: the polluters who dirtied each valley for its
guardian. 64 x 48, facing right like the rest of the roster, two frames
each (treads rolling, a blade or drill turning, legs stepping, smoke
puffing). Drawn in code with landmark_art's kit.
"""
from __future__ import annotations

import math

import numpy as np
from PIL import Image, ImageDraw

import landmark_art as kit

W, H = 64, 48


class Pic(kit.Pic):
    def __init__(self, colors, seed):
        self.names = list(colors)
        self.pal = [(0, 0, 0)] + [colors[k] for k in self.names]
        self.img = Image.new("L", (W, H), 0)
        self.d = ImageDraw.Draw(self.img)
        self.rng = np.random.default_rng(seed)

    def rgba(self):
        a = np.array(self.img)
        rgb = np.array(self.pal, dtype=np.uint8)[a]
        solid = a > 0
        pad = np.pad(solid, 1)
        edge = solid & ~(pad[:-2, 1:-1] & pad[2:, 1:-1] & pad[1:-1, :-2] & pad[1:-1, 2:])
        rgb[edge] = kit.RIM
        return np.dstack([rgb, np.where(solid, 255, 0)]).astype(np.uint8)


METAL = {"m_d": (60, 58, 66), "m_m": (112, 110, 120), "m_l": (176, 174, 184), "tr": (40, 36, 40),
         "tr_l": (92, 88, 92), "eye": (255, 60, 40), "eye_l": (255, 200, 120)}


def treads(p, x0, x1, y, frame):
    p.rect(x0, y, x1, y + 9, "tr")
    p.ell(x0 + 1, y + 4.5, 4.5, 4.5, "tr")
    p.ell(x1 - 1, y + 4.5, 4.5, 4.5, "tr")
    for x in range(x0 + 2 + frame * 3, x1 - 1, 6):
        p.rect(x, y + 1, x + 1, y + 8, "tr_l")
    for x in range(x0 + 4, x1 - 2, 8):
        p.ell(x, y + 4.5, 2, 2, "m_m")


def sawbot(frame):
    """A logging machine: a cab in hazard yellow on treads, a buzzsaw out in
    front on its arm, exhaust from its stack."""
    p = Pic({**METAL, "ye_d": (170, 120, 20), "ye_m": (232, 184, 40), "ye_l": (255, 230, 120),
             "saw": (200, 204, 212), "saw_d": (120, 124, 136), "smoke": (120, 116, 110)}, 31 + frame)
    treads(p, 4, 40, 37, frame)
    p.rect(6, 18, 38, 36, "ye_m")
    p.rect(6, 18, 38, 21, "ye_l")
    p.rect(6, 32, 38, 36, "ye_d")
    for k in range(4):
        x = 8 + k * 8
        p.poly([(x, 36), (x + 4, 36), (x + 8, 32), (x + 4, 32)], "tr")
    p.rect(20, 8, 36, 19, "m_m")
    p.rect(22, 10, 34, 15, "m_d")
    p.rect(27, 11, 33, 14, "eye")
    p.rect(8, 6, 12, 18, "m_d")
    p.ell(10 + frame, 3 - frame, 3, 2.5, "smoke")
    p.line([(36, 26), (46, 24)], "m_d", 4)
    cx, cy, r = 52, 24, 11
    p.ell(cx, cy, r, r, "saw_d")
    p.ell(cx, cy, r - 2, r - 2, "saw")
    for k in range(12):
        a = k * math.pi / 6 + frame * math.pi / 12
        x, y = cx + math.cos(a) * r, cy + math.sin(a) * r
        p.poly([(x, y), (cx + math.cos(a + 0.2) * (r + 3), cy + math.sin(a + 0.2) * (r + 3)),
                (cx + math.cos(a + 0.35) * r, cy + math.sin(a + 0.35) * r)], "saw_d")
    p.ell(cx, cy, 3, 3, "m_d")
    return p.rgba()


def drillbot(frame):
    """A boring machine: a round hull on treads, a great drill in front, its
    headlamp burning."""
    p = Pic({**METAL, "bl_d": (40, 70, 120), "bl_m": (70, 110, 170), "bl_l": (140, 180, 220),
             "dr_d": (110, 100, 90), "dr_m": (170, 160, 140), "dr_l": (230, 222, 200)}, 41 + frame)
    treads(p, 4, 38, 37, frame)
    p.ell(21, 26, 17, 12, "bl_m")
    p.ell(17, 21, 10, 5, "bl_l")
    p.rect(6, 30, 36, 36, "bl_d")
    p.ell(27, 20, 4, 4, "eye_l")
    p.ell(27, 20, 2, 2, "eye")
    p.poly([(36, 16), (62, 25), (36, 34)], "dr_m")
    for k in range(5):
        x = 38 + k * 5 + frame * 2
        top = 16 + (x - 36) * 9 / 26
        bot = 34 - (x - 36) * 9 / 26
        if x < 60:
            p.line([(x, top), (x + 3, bot)], "dr_d", 1)
    p.poly([(36, 16), (62, 25), (36, 19)], "dr_l")
    p.rect(34, 18, 38, 32, "m_d")
    return p.rgba()


def torchbot(frame):
    """A burning machine: a squat walker with a fuel tank on its back and a
    torch arm, its pilot flame lit."""
    p = Pic({**METAL, "rd_d": (110, 30, 24), "rd_m": (180, 56, 40), "rd_l": (230, 110, 80),
             "tank": (70, 110, 60), "tank_l": (120, 170, 100), "fl": (255, 150, 40), "fl_l": (255, 236, 140)}, 51 + frame)
    s = 1 if frame else -1
    p.rect(14 + s * 2, 34, 20 + s * 2, 46, "m_d")
    p.rect(28 - s * 2, 34, 34 - s * 2, 46, "m_d")
    p.rect(11 + s * 2, 44, 22 + s * 2, 47, "m_m")
    p.rect(26 - s * 2, 44, 37 - s * 2, 47, "m_m")
    p.ell(8, 22, 6, 11, "tank")
    p.ell(6, 18, 2, 6, "tank_l")
    p.ell(24, 24, 14, 12, "rd_m")
    p.ell(20, 19, 8, 5, "rd_l")
    p.rect(12, 30, 36, 36, "rd_d")
    p.rect(22, 12, 36, 20, "m_m")
    p.rect(30, 14, 35, 17, "eye")
    p.line([(34, 26), (46, 24)], "m_m", 4)
    p.rect(46, 21, 51, 27, "m_d")
    p.ell(55 + frame, 24, 4 + frame, 3, "fl")
    p.ell(54 + frame, 24, 2, 1.5, "fl_l")
    return p.rgba()


def smogstack(frame):
    """A walking chimney: a sooty stack on two stubby legs, belching smog."""
    p = Pic({**METAL, "br_d": (90, 50, 40), "br_m": (140, 80, 60), "br_l": (180, 116, 90),
             "sm_d": (90, 86, 84), "sm_m": (130, 124, 118), "sm_l": (170, 164, 156)}, 61 + frame)
    s = 1 if frame else -1
    p.rect(20 + s * 2, 38, 26 + s * 2, 46, "m_d")
    p.rect(36 - s * 2, 38, 42 - s * 2, 46, "m_d")
    p.rect(17 + s * 2, 44, 28 + s * 2, 47, "m_m")
    p.rect(34 - s * 2, 44, 45 - s * 2, 47, "m_m")
    p.poly([(18, 40), (22, 12), (42, 12), (46, 40)], "br_m")
    wall = p.area("br_m")
    for y in range(15, 40, 5):
        p.line([(16, y), (48, y)], "br_d", 1, clip=wall)
    p.rect(20, 8, 44, 13, "m_m")
    p.rect(26, 22, 38, 27, "m_d")
    p.rect(28, 23, 36, 26, "eye")
    for k, (x, y, r) in enumerate(((32, 4, 6), (24 + frame * 3, 1, 5), (41 - frame * 2, 2, 4))):
        p.ell(x, y, r, r * 0.7, ("sm_m", "sm_d", "sm_l")[k])
    return p.rgba()


def sludgebarrel(frame):
    """A leaking drum on stumpy legs: it waddles at her and spits sludge."""
    p = Pic({**METAL, "dr_d": (60, 80, 40), "dr_m": (96, 124, 60), "dr_l": (150, 176, 100),
             "rust": (150, 80, 40), "sl": (150, 220, 60), "sl_l": (210, 250, 140)}, 71 + frame)
    s = 1 if frame else -1
    for x in (20, 40):
        p.rect(x - 3 + s * (1 if x == 20 else -1), 38, x + 3 + s * (1 if x == 20 else -1), 46, "m_d")
    p.rect(16, 10, 48, 40, "dr_m")
    p.rect(16, 10, 22, 40, "dr_l")
    p.rect(42, 10, 48, 40, "dr_d")
    for y in (14, 25, 36):
        p.rect(15, y, 49, y + 2, "m_m")
    p.rect(16, 8, 48, 11, "m_l")
    p.speckle(p.area("dr_m"), "rust", 12, 1.2)
    p.rect(26, 18, 38, 22, "m_d")
    p.rect(27, 19, 31, 21, "eye")
    p.rect(33, 19, 37, 21, "eye")
    p.poly([(44, 26), (54, 28 + frame), (50, 34 + frame), (46, 30)], "sl")
    p.ell(52, 36 + frame * 2, 2.5, 3.5, "sl")
    p.ell(51, 35 + frame * 2, 1, 1.5, "sl_l")
    return p.rgba()


MACHINES = (
    ("sawbot", sawbot), ("drillbot", drillbot), ("torchbot", torchbot),
    ("smogstack", smogstack), ("sludgebarrel", sludgebarrel),
)


if __name__ == "__main__":
    import sys
    out = sys.argv[1] if len(sys.argv) > 1 else "machines.png"
    sheet = Image.new("RGBA", (W * 2 * len(MACHINES), H), (70, 120, 90, 255))
    for k, (name, fn) in enumerate(MACHINES):
        for f in range(2):
            sheet.alpha_composite(Image.fromarray(fn(f)), ((k * 2 + f) * W, 0))
    sheet.resize((sheet.width * 3, sheet.height * 3), Image.NEAREST).save(out)
