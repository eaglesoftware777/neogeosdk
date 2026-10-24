"""
Maiya & Luna room painter: a 20 x 14 map (tools/worldlib.py LEGEND) -> a
320 x 224 picture, the room's background.

The ground is painted per pixel with wobbling borders between grass, paths,
fields and floors, then the solid things are drawn over it in rows from the
top down (trees, rocks, cliffs, houses, the mill and its wheel, fences,
channels), each with a soft shadow to the lower right. Everything comes from
seeded noise, so a room always paints the same. `wet` fills the channels
with water (a restored region's picture).
"""
import math

import numpy as np

TW = 16


def h2(x, y, s=0):
    """A repeatable pseudo-random value in [0, 1) for a pixel."""
    n = (x * 374761393 + y * 668265263 + s * 2246822519) & 0xFFFFFFFF
    n = ((n ^ (n >> 13)) * 1274126177) & 0xFFFFFFFF
    return ((n ^ (n >> 16)) & 0xFFFF) / 65536.0


def smooth(x, y, scale, s=0):
    """Value noise, smooth between lattice points."""
    gx, gy = x / scale, y / scale
    x0, y0 = int(math.floor(gx)), int(math.floor(gy))
    fx, fy = gx - x0, gy - y0
    fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
    a, b = h2(x0, y0, s), h2(x0 + 1, y0, s)
    c, d = h2(x0, y0 + 1, s), h2(x0 + 1, y0 + 1, s)
    return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy


def mix(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def sh(c, k):
    return tuple(max(0, min(255, int(v * k))) for v in c)


GRASS = [(68, 140, 56), (84, 162, 62), (104, 182, 72), (56, 118, 48)]
GRASS_DRY = [(130, 150, 66), (150, 166, 74), (174, 182, 92), (112, 128, 56)]
PATH = [(196, 160, 104), (178, 142, 90), (214, 182, 126), (150, 116, 72)]
FIELD_DRY = [(150, 112, 66), (126, 92, 52), (176, 140, 88)]
FIELD_WET = [(110, 82, 50), (92, 66, 40), (136, 104, 66)]
STONE = [(170, 166, 156), (146, 142, 134), (192, 188, 178), (112, 108, 104)]
CAVE = [(112, 96, 84), (98, 84, 74), (132, 116, 102), (72, 60, 54)]
WATER = [(56, 120, 200), (72, 146, 222), (120, 190, 240), (40, 92, 170)]
ROCK = [(150, 144, 136), (120, 114, 110), (186, 180, 170), (88, 82, 80)]
CLIFF = [(146, 132, 118), (112, 98, 88), (178, 166, 150), (78, 66, 60)]
CAVEWALL = [(98, 78, 92), (76, 60, 74), (124, 102, 116), (52, 40, 52)]
TREE = [(40, 110, 50), (56, 134, 58), (88, 166, 72), (26, 76, 40)]
TRUNK = [(110, 72, 42), (86, 54, 32)]
SAND = [(232, 212, 152), (218, 196, 136), (246, 230, 180), (194, 168, 112)]
SEA = [(30, 92, 166), (42, 112, 188), (110, 178, 230), (22, 66, 136)]
METAL = [(124, 132, 146), (104, 112, 126), (160, 168, 180), (70, 76, 88)]
IRON = [(84, 88, 100), (64, 68, 80), (120, 126, 140), (40, 42, 52)]
HEDGE = [(34, 96, 42), (26, 76, 36), (66, 138, 58), (14, 46, 24)]
BARK = [(120, 84, 54), (96, 64, 40), (150, 110, 72), (64, 42, 28)]
TOWER = [(236, 232, 220), (206, 200, 188), (252, 250, 244), (150, 144, 136)]
CRYSTAL = [(170, 140, 230), (120, 96, 200), (230, 220, 255), (80, 60, 150)]
MIRE = [(70, 84, 52), (56, 70, 44), (92, 104, 64), (40, 50, 34)]

# Each region has its own light: grass, trees, cave rock and what a drop looks like.
THEMES = {
    'valley': dict(grass=GRASS, tree=TREE, cave=CAVE, cavewall=CAVEWALL, cliff=CLIFF, moss=(110, 150, 70), pit='dark'),
    'forest': dict(grass=[(54, 124, 50), (68, 142, 56), (92, 164, 68), (40, 102, 42)],
                   tree=[(30, 92, 44), (44, 114, 52), (74, 146, 64), (20, 62, 34)], cave=CAVE, cavewall=CAVEWALL,
                   cliff=CLIFF, moss=(84, 140, 64), pit='mist'),
    'coast': dict(grass=[(118, 162, 70), (136, 178, 78), (162, 196, 96), (98, 140, 60)],
                  tree=[(50, 130, 60), (70, 156, 70), (110, 190, 90), (34, 92, 46)], cave=CAVE, cavewall=CAVEWALL,
                  cliff=[(172, 160, 138), (140, 128, 110), (204, 194, 172), (98, 88, 76)], moss=None, pit='sea'),
    'autumn': dict(grass=[(150, 150, 62), (170, 162, 70), (196, 180, 88), (124, 124, 52)],
                   tree=[(196, 96, 40), (224, 136, 52), (248, 186, 76), (132, 60, 30)], cave=CAVE, cavewall=CAVEWALL,
                   cliff=CLIFF, moss=(170, 140, 60), pit='dark'),
    'grotto': dict(grass=GRASS, tree=TREE, cave=[(86, 92, 116), (74, 80, 104), (108, 114, 140), (52, 56, 78)],
                   cavewall=[(98, 74, 130), (78, 58, 106), (128, 102, 164), (50, 36, 74)], cliff=CLIFF, moss=None,
                   pit='dark'),
    'tree': dict(grass=[(60, 128, 56), (76, 146, 62), (100, 168, 74), (46, 106, 46)], tree=TREE, cave=CAVE,
                 cavewall=CAVEWALL, cliff=BARK, moss=(90, 150, 70), pit='leaves'),
    'works': dict(grass=GRASS, tree=TREE, cave=[(120, 112, 104), (104, 96, 90), (140, 132, 122), (80, 74, 70)],
                  cavewall=[(150, 140, 124), (122, 112, 100), (180, 170, 152), (86, 78, 70)], cliff=CLIFF, moss=None,
                  pit='dark'),
}


class Room:
    def __init__(self, rows, wet=False, dark=False, seed=1, theme='valley'):
        self.rows = rows
        self.th = THEMES[theme]
        self.theme = theme
        self.H, self.W = len(rows), len(rows[0])
        self.wet = wet
        self.dark = dark
        self.seed = seed
        self.img = np.zeros((self.H * TW, self.W * TW, 3), np.uint8)
        self.shadow = np.zeros((self.H * TW, self.W * TW), np.float32)

    def at(self, x, y):
        if 0 <= x < self.W and 0 <= y < self.H:
            return self.rows[y][x]
        return None

    def put(self, x, y, c):
        if 0 <= x < self.img.shape[1] and 0 <= y < self.img.shape[0]:
            self.img[y, x] = c

    # ---- ground ----------------------------------------------------------------------
    def ground_kind(self, c):
        if c in '=SL': return 'path'
        if c == 'F': return 'field'
        if c in ':okZl': return 'stone'
        if c in 'gXeKpV': return 'cave'
        if c in '~w': return 'water'
        if c in 'OU': return 'sea'
        if c in 'fEJB': return 'wood'
        if c in 'cu': return 'stone'
        if c == 's': return 'sand'
        if c in 'mI': return 'metal'
        if c == 'q': return 'mire'
        return 'grass'

    def paint_ground(self):
        s = self.seed
        for py in range(self.H * TW):
            for px in range(self.W * TW):
                jx = int((smooth(px, py, 6, s + 1) - 0.5) * 7)
                jy = int((smooth(px, py, 6, s + 2) - 0.5) * 7)
                c = self.at(max(0, min(self.W - 1, (px + jx) // TW)), max(0, min(self.H - 1, (py + jy) // TW)))
                own = self.at(px // TW, py // TW)
                kind = self.ground_kind(c)
                if self.ground_kind(own) in ('water', 'cave', 'stone', 'wood', 'sea', 'metal'):
                    kind = self.ground_kind(own)        # hard materials keep straight edges
                n = smooth(px, py, 5, s) * 0.7 + h2(px, py, s) * 0.3
                if kind == 'grass':
                    pal = self.th['grass']
                    col = pal[1] if n < 0.45 else (pal[0] if n < 0.7 else pal[2])
                    if h2(px, py, s + 7) < 0.05:
                        col = pal[3]
                    if h2(px, py, s + 8) < 0.04 and py % TW > 1:
                        col = pal[2]
                        self.put(px, py - 1, pal[2])
                elif kind == 'path':
                    pal = PATH
                    col = pal[0] if n < 0.55 else pal[1]
                    if h2(px, py, s + 3) < 0.03:
                        col = pal[2]
                    elif h2(px, py, s + 4) < 0.03:
                        col = pal[3]
                elif kind == 'field':
                    pal = FIELD_WET if self.wet else FIELD_DRY
                    row = py % 6
                    col = pal[1] if row in (0, 1) else (pal[2] if row == 2 else pal[0])
                elif kind == 'stone':
                    pal = STONE
                    gx, gy = (px + (4 if (py // 8) % 2 else 0)) % 8, py % 8
                    col = pal[3] if (gx == 0 or gy == 0) else (pal[0] if n < 0.6 else pal[2])
                elif kind == 'cave':
                    pal = self.th['cave']
                    col = pal[0] if n < 0.5 else pal[1]
                    if h2(px, py, s + 5) < 0.04:
                        col = pal[2]
                elif kind == 'water':
                    pal = WATER
                    w = math.sin((px + py * 0.3) * 0.5 + smooth(px, py, 9, s) * 6)
                    col = pal[0] if w < 0.55 else pal[1]
                    if w > 0.93 and h2(px, py, s + 6) < 0.5:
                        col = pal[2]
                elif kind == 'wood':
                    col = (150, 104, 62) if (py % 4) else (110, 74, 44)
                    if py % 4 and (px + (py // 4) * 11) % 23 == 0:
                        col = (126, 86, 50)
                elif kind == 'sand':
                    col = SAND[0] if n < 0.5 else SAND[1]
                    if h2(px, py, s + 9) < 0.04:
                        col = SAND[2] if h2(px, py, s + 10) < 0.5 else SAND[3]
                elif kind == 'sea':
                    w = math.sin((px * 0.8 + py * 0.25) * 0.45 + smooth(px, py, 11, s) * 6)
                    col = SEA[0] if w < 0.5 else SEA[1]
                    if w > 0.94 and h2(px, py, s + 6) < 0.5:
                        col = SEA[2]
                elif kind == 'metal':
                    lx, ly = px % 16, py % 16
                    col = METAL[0] if (lx + ly) % 9 else METAL[1]
                    if lx == 0 or ly == 0:
                        col = METAL[3]
                    elif lx == 1 or ly == 1:
                        col = METAL[2]
                    if lx in (3, 13) and ly in (3, 13):
                        col = METAL[2]
                elif kind == 'mire':
                    col = MIRE[0] if n < 0.5 else MIRE[1]
                    if smooth(px, py, 4, s + 11) > 0.72:
                        col = (60, 78, 70)
                else:
                    col = GRASS[1]
                self.img[py, px] = col

    def flowers(self):
        s = self.seed
        cols = [(250, 250, 240), (250, 220, 80), (240, 120, 160), (150, 170, 250)]
        for y, row in enumerate(self.rows):
            for x, c in enumerate(row):
                if c != ',':
                    continue
                for k in range(5):
                    fx = x * TW + 2 + int(h2(x, y, s + 20 + k) * 12)
                    fy = y * TW + 2 + int(h2(x, y, s + 30 + k) * 12)
                    col = cols[int(h2(x, y, s + 40 + k) * 4)]
                    for dx, dy in ((0, 0), (1, 0), (0, 1), (1, 1)):
                        self.put(fx + dx, fy + dy, col)
                    self.put(fx, fy, sh(col, 1.1))
                    self.put(fx + 1, fy + 2, (50, 110, 44))

    def field_sprouts(self):
        s = self.seed
        green = (70, 160, 60) if self.wet else (170, 150, 70)
        light = (130, 210, 90) if self.wet else (200, 176, 96)
        for y, row in enumerate(self.rows):
            for x, c in enumerate(row):
                if c != 'F':
                    continue
                for ry in range(3, TW, 6):
                    for k in range(0, TW, 4):
                        if h2(x * TW + k, y * TW + ry, s) < 0.75:
                            px, py = x * TW + k + 1, y * TW + ry
                            self.put(px, py, green); self.put(px + 1, py - 1, light)
                            if self.wet:
                                self.put(px - 1, py - 1, light); self.put(px, py - 2, green)

    def water_edges(self):
        """Darker water along banks, a light foam line where water meets land."""
        img = self.img
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) not in '~O':
                    continue
                for py in range(y * TW, y * TW + TW):
                    for px in range(x * TW, x * TW + TW):
                        lx, ly = px % TW, py % TW
                        for (dx, dy, d) in ((0, -1, ly), (-1, 0, lx), (1, 0, TW - 1 - lx), (0, 1, TW - 1 - ly)):
                            n = self.at(x + dx, y + dy)
                            if n is not None and n not in '~wBOUp':
                                if d == 0:
                                    img[py, px] = (210, 236, 250)
                                elif d < 3:
                                    img[py, px] = WATER[3] if self.at(x, y) == '~' else SEA[3]

    # ---- shadows ---------------------------------------------------------------------
    def cast(self, x0, y0, x1, y1, depth=3):
        """A soft shadow below and right of a solid box (pixels)."""
        for y in range(y1, y1 + depth):
            for x in range(x0 + 2, x1 + depth):
                if 0 <= y < self.shadow.shape[0] and 0 <= x < self.shadow.shape[1]:
                    self.shadow[y, x] = max(self.shadow[y, x], 0.28)
        for y in range(y0 + 2, y1):
            for x in range(x1, x1 + depth - 1):
                if 0 <= y < self.shadow.shape[0] and 0 <= x < self.shadow.shape[1]:
                    self.shadow[y, x] = max(self.shadow[y, x], 0.2)

    def apply_shadow(self, solid_mask):
        k = 1.0 - self.shadow * (~solid_mask)
        self.img = (self.img * k[:, :, None]).astype(np.uint8)

    # ---- things on the ground ----------------------------------------------------------
    def disc(self, cx, cy, r, pal, ry=None):
        ry = ry or r
        for py in range(int(cy - ry) - 1, int(cy + ry) + 2):
            for px in range(int(cx - r) - 1, int(cx + r) + 2):
                dx, dy = (px + 0.5 - cx) / r, (py + 0.5 - cy) / ry
                d = dx * dx + dy * dy
                if d > 1:
                    continue
                if d > 0.82:
                    col = pal[3]
                elif dx + dy < -0.55 and d > 0.15:
                    col = pal[2]
                elif dx + dy > 0.6:
                    col = pal[0]
                else:
                    col = pal[1]
                if h2(px, py, self.seed + 50) < 0.08 and d < 0.8:
                    col = pal[2] if dx + dy < 0 else pal[0]
                self.put(px, py, col)

    def trees(self, solid):
        s = self.seed
        for y in range(self.H):
            for x in range(self.W):
                c = self.at(x, y)
                if c not in 'TA':
                    continue
                cx = x * TW + 8 + (h2(x, y, s + 60) - 0.5) * 3
                cy = y * TW + 7 + (h2(x, y, s + 61) - 0.5) * 3
                below = self.at(x, y + 1)
                if below is not None and below not in 'TA':
                    tx = int(cx) - 2
                    for py in range(y * TW + 10, y * TW + 16):
                        for px in range(tx, tx + 5):
                            self.put(px, py, TRUNK[0] if px < tx + 3 else TRUNK[1])
                    self.cast(tx, y * TW + 10, tx + 5, y * TW + 16, 3)
                r = 10.5 + h2(x, y, s + 62) * 1.5
                self.disc(cx, cy - 1, r, self.th['tree'], ry=r * 0.92)
                if c == 'A':
                    fruit = [(220, 50, 40), (250, 120, 60)] if self.theme != 'autumn' else [(214, 40, 34), (240, 200, 60)]
                    for k in range(6):
                        fx = int(cx - 7 + h2(x, y, s + 63 + k) * 14)
                        fy = int(cy - 8 + h2(x, y, s + 70 + k) * 13)
                        col = fruit[k % 2]
                        for dx, dy in ((0, 0), (1, 0), (0, 1), (1, 1)):
                            self.put(fx + dx, fy + dy, col)
                        self.put(fx, fy, (255, 230, 200))

    def bushes(self):
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) != 'b':
                    continue
                self.disc(x * TW + 8, y * TW + 9, 7.5, [(44, 116, 50), (62, 140, 58), (96, 172, 78), (28, 80, 40)], ry=6.5)
                for k in range(3):
                    self.put(x * TW + 4 + k * 4, y * TW + 7 + (k % 2) * 3, (210, 60, 70))
                self.cast(x * TW + 1, y * TW + 3, x * TW + 15, y * TW + 15)

    def rocks(self):
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) != 'r':
                    continue
                self.disc(x * TW + 8, y * TW + 9, 7, ROCK, ry=6)
                self.cast(x * TW + 1, y * TW + 4, x * TW + 15, y * TW + 15)

    def cliffs(self, ch, pal, moss, joints=False):
        s = self.seed
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) != ch:
                    continue
                below = self.at(x, y + 1)
                face = below is not None and below != ch and below not in 'wvKe'
                for py in range(y * TW, y * TW + TW):
                    for px in range(x * TW, x * TW + TW):
                        n = smooth(px, py, 4, s + 70)
                        if face and py >= y * TW + 4:
                            col = pal[1] if n < 0.5 else pal[0]
                            if (px + int(n * 6)) % 7 == 0:
                                col = pal[3]
                            if py == y * TW + 4:
                                col = pal[2]
                        else:
                            col = pal[0] if n < 0.45 else pal[2]
                            if moss and smooth(px, py, 6, s + 71) > 0.74:
                                col = moss
                            if h2(px, py, s + 72) < 0.06:
                                col = pal[3]
                        if joints:
                            jx, jy = (px + (8 if (py // 8) % 2 else 0)) % 16, py % 8
                            if jx == 0 or jy == 0:
                                col = pal[3]
                            elif jx == 1 or jy == 1:
                                col = pal[2]
                        self.img[py, px] = col
                if face:
                    self.cast(x * TW, y * TW, x * TW + 16, y * TW + 16, 3)

    def vines(self):
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) != 'v':
                    continue
                for py in range(y * TW, y * TW + TW):
                    for px in range(x * TW, x * TW + TW):
                        self.img[py, px] = CLIFF[1] if smooth(px, py, 4, 9) < 0.5 else CLIFF[0]
                for k in range(4):
                    vx = x * TW + 2 + k * 4
                    for py in range(y * TW, y * TW + TW):
                        wob = int(math.sin(py * 0.6 + k) * 1.3)
                        self.put(vx + wob, py, (50, 130, 50))
                        if py % 4 == k % 4:
                            self.put(vx + wob + 1, py, (110, 190, 80))

    def houses(self):
        for y in range(self.H):
            for x in range(self.W):
                c = self.at(x, y)
                X, Y = x * TW, y * TW
                if c == 'R':
                    top = self.at(x, y - 1) != 'R'
                    for py in range(Y, Y + TW):
                        for px in range(X, X + TW):
                            band = (py - Y) % 5
                            col = (190, 70, 52) if band else (130, 42, 36)
                            if (px + (py // 5) * 3) % 8 == 0 and band:
                                col = (156, 52, 40)
                            if top and py < Y + 3:
                                col = (214, 104, 80) if py == Y + 1 else (120, 40, 34)
                            self.img[py, px] = col
                elif c in 'HD':
                    for py in range(Y, Y + TW):
                        for px in range(X, X + TW):
                            col = (232, 220, 190)
                            if px % 16 in (0, 15) or py == Y + 15:
                                col = (110, 70, 40)
                            if py == Y:
                                col = (70, 44, 30)
                            self.img[py, px] = col
                    if c == 'D':
                        for py in range(Y + 3, Y + 16):
                            for px in range(X + 4, X + 12):
                                self.img[py, px] = (126, 76, 40) if (px - X) % 3 else (96, 56, 30)
                        self.put(X + 10, Y + 10, (230, 200, 90))
                    else:
                        for py in range(Y + 4, Y + 11):
                            for px in range(X + 4, X + 12):
                                self.img[py, px] = (120, 170, 220) if (px - X < 8) == (py - Y < 7) else (90, 140, 200)
                        for px in range(X + 3, X + 13):
                            self.put(px, Y + 11, (110, 70, 40))
                    self.cast(X, Y, X + 16, Y + 16, 3)
                elif c == 'P':
                    top = self.at(x, y - 1) != 'P'
                    for py in range(Y, Y + TW):
                        for px in range(X, X + TW):
                            band = (py - Y) % 4
                            col = (100, 112, 132) if band else (70, 78, 96)
                            if (px + (py // 4) * 5) % 7 == 0 and band:
                                col = (84, 94, 112)
                            if top and py < Y + 2:
                                col = (140, 150, 170)
                            self.img[py, px] = col
                elif c == 'M':
                    for py in range(Y, Y + TW):
                        for px in range(X, X + TW):
                            gx, gy = (px + (5 if (py // 6) % 2 else 0)) % 10, py % 6
                            self.img[py, px] = (96, 88, 82) if gx == 0 or gy == 0 else (156, 146, 134)
                    self.cast(X, Y, X + 16, Y + 16, 3)
                elif c == 'x':
                    for px in range(X, X + TW):
                        for ry in (Y + 5, Y + 10):
                            self.put(px, ry, (150, 104, 60)); self.put(px, ry + 1, (110, 72, 40))
                    for pxx in (X + 2, X + 10):
                        for py in range(Y + 2, Y + 15):
                            self.put(pxx, py, (160, 112, 66)); self.put(pxx + 1, py, (116, 78, 44))
                    self.cast(X, Y + 2, X + 16, Y + 14, 2)
                elif c == 'o':
                    cx, cy = X + 8, Y + 8
                    self.disc(cx, cy, 7.5, ROCK, ry=7)
                    self.disc(cx, cy - 0.5, 4.5, [(20, 40, 70), (30, 60, 100), (60, 100, 150), (16, 30, 50)])
                    self.cast(X + 1, Y + 1, X + 15, Y + 15)

    def wheels(self):
        cells = [(x, y) for y in range(self.H) for x in range(self.W) if self.at(x, y) == 'W']
        if not cells:
            return
        xs, ys = [c[0] for c in cells], [c[1] for c in cells]
        cx = (min(xs) + max(xs) + 1) * TW / 2
        cy = (min(ys) + max(ys) + 1) * TW / 2
        r = min((max(xs) - min(xs) + 1) * TW, (max(ys) - min(ys) + 1) * TW) / 2 - 1
        for py in range(min(ys) * TW, (max(ys) + 1) * TW):
            for px in range(min(xs) * TW, (max(xs) + 1) * TW):
                self.img[py, px] = (40, 36, 40)
        for py in range(int(cy - r) - 2, int(cy + r) + 3):
            for px in range(int(cx - r) - 2, int(cx + r) + 3):
                d = math.hypot(px + 0.5 - cx, py + 0.5 - cy)
                a = math.atan2(py + 0.5 - cy, px + 0.5 - cx)
                if r - 3 <= d <= r:
                    self.put(px, py, (140, 92, 52) if d > r - 1.5 else (104, 66, 36))
                elif d < r - 3:
                    spoke = min(abs(((a * 8 / math.pi) % 2) - 1) for _ in (0,))
                    if spoke < 0.2:
                        self.put(px, py, (126, 82, 46))
                    elif d < 3:
                        self.put(px, py, (90, 90, 96))
                    else:
                        self.put(px, py, (34, 30, 34))
                elif d <= r + 2 and abs(((a * 16 / math.pi) % 2) - 1) < 0.35:
                    self.put(px, py, (120, 80, 44))

    def channels(self):
        """Stone-lined grooves: a dark bed (or water) with a lighter rim, joined to their neighbours."""
        chars = 'cuk'
        for y in range(self.H):
            for x in range(self.W):
                c = self.at(x, y)
                if c not in chars:
                    continue
                X, Y = x * TW, y * TW
                conn = {d: (self.at(x + dx, y + dy) or '') in chars + 'w~' for d, (dx, dy) in
                        {'n': (0, -1), 's': (0, 1), 'w': (-1, 0), 'e': (1, 0)}.items()}
                if y == 0:
                    conn['n'] = True
                if y == self.H - 1:
                    conn['s'] = True
                bed = WATER if self.wet or c == 'u' and False else [(96, 86, 76), (84, 76, 68), (110, 100, 90), (70, 62, 56)]
                if self.wet:
                    bed = WATER

                def inside(px, py):
                    lx, ly = px - X, py - Y
                    if 4 <= lx <= 11 and 4 <= ly <= 11:
                        return True
                    if conn['n'] and 4 <= lx <= 11 and ly < 4: return True
                    if conn['s'] and 4 <= lx <= 11 and ly > 11: return True
                    if conn['w'] and 4 <= ly <= 11 and lx < 4: return True
                    if conn['e'] and 4 <= ly <= 11 and lx > 11: return True
                    return False

                def rim(px, py):
                    lx, ly = px - X, py - Y
                    if 2 <= lx <= 13 and 2 <= ly <= 13:
                        return True
                    if conn['n'] and 2 <= lx <= 13 and ly < 2: return True
                    if conn['s'] and 2 <= lx <= 13 and ly > 13: return True
                    if conn['w'] and 2 <= ly <= 13 and lx < 2: return True
                    if conn['e'] and 2 <= ly <= 13 and lx > 13: return True
                    return False

                for py in range(Y, Y + TW):
                    for px in range(X, X + TW):
                        if inside(px, py):
                            n = smooth(px, py, 3, 5)
                            col = bed[0] if n < 0.6 else bed[1]
                            if self.wet and math.sin(px * 0.7 + py * 0.4) > 0.92:
                                col = bed[2]
                            self.img[py, px] = col
                        elif rim(px, py):
                            self.img[py, px] = STONE[2] if (px + py) % 6 else STONE[3]
                if c == 'k':
                    horiz = conn['n'] or conn['s']
                    for py in range(Y + 1, Y + 15):
                        for px in range(X, X + TW):
                            if horiz:
                                col = (156, 108, 62) if (py - Y) % 4 else (110, 72, 40)
                            else:
                                col = (156, 108, 62) if (px - X) % 4 else (110, 72, 40)
                            self.img[py, px] = col

    def specials(self):
        s = self.seed
        for y in range(self.H):
            for x in range(self.W):
                c = self.at(x, y)
                X, Y = x * TW, y * TW
                if c == 'w':
                    for py in range(Y, Y + TW):
                        for px in range(X, X + TW):
                            n = h2(px, py // 3, s + 80)
                            self.img[py, px] = (230, 244, 252) if n < 0.35 else ((150, 200, 240) if n < 0.7 else (100, 160, 220))
                elif c == 'p':
                    above = self.at(x, y - 1)
                    rim = above is not None and above != 'p'
                    for py in range(Y, Y + TW):
                        for px in range(X, X + TW):
                            ly = py - Y
                            if self.th['pit'] == 'leaves':
                                n = smooth(px, py, 5, s + 81)
                                col = (16, 46, 26) if n < 0.55 else ((30, 74, 40) if n < 0.8 else (54, 110, 56))
                            elif self.th['pit'] == 'mist':
                                depth = ly + 16 * sum(1 for k in range(1, 4) if self.at(x, y - k) == 'p')
                                n = smooth(px, py, 7, s + 82)
                                col = mix((70, 96, 100), (14, 26, 30), min(1.0, depth / 40.0))
                                if n > 0.66:
                                    col = mix(col, (170, 196, 196), (n - 0.66) * 1.6)
                            elif self.th['pit'] == 'sea':
                                col = SEA[3] if (px + py) % 7 else SEA[0]
                            else:
                                col = (16, 12, 14)
                            if rim and ly <= 3:
                                col = (70, 54, 46) if self.th['pit'] != 'leaves' else BARK[1]
                            self.img[py, px] = col
                elif c == 'n':
                    for py in range(Y, Y + TW):
                        for px in range(X, X + TW):
                            lx = px - X
                            self.img[py, px] = ROCK[1] if (lx < 4 or lx > 11) else (60, 52, 44)
                            if lx in (4, 11):
                                self.img[py, px] = ROCK[3]
                elif c == 'h':
                    for k in range(7):
                        bx, by = X + int(h2(x, y, s + 90 + k) * 14), Y + int(h2(x, y, s + 100 + k) * 12) + 2
                        for d in range(5):
                            self.put(bx + d, by - d // 2, (110, 50, 40))
                            if d % 2:
                                self.put(bx + d, by - d // 2 - 1, (200, 190, 170))
                elif c == 'L':
                    for px in range(X, X + TW):
                        for py in (Y + 12, Y + 13):
                            self.put(px, py, STONE[3] if py == Y + 13 else STONE[2])
                        self.put(px, Y + 14, (60, 50, 44)); self.put(px, Y + 15, (90, 76, 66))
                elif c == 'e':
                    for py in range(Y, Y + TW):
                        for px in range(X, X + TW):
                            self.img[py, px] = (14, 10, 14)
                elif c == 'g':
                    pass


    # ---- the other regions' things -----------------------------------------------------
    def blocks(self, ch, texture):
        """Solid blocks seen from above: the top, and a face where one meets open ground below."""
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) != ch:
                    continue
                below = self.at(x, y + 1)
                face = below is not None and below != ch
                X, Y = x * TW, y * TW
                for py in range(Y, Y + TW):
                    for px in range(X, X + TW):
                        self.img[py, px] = texture(px, py, px - X, py - Y, face and py >= Y + 6, face and py == Y + 6)
                if face:
                    self.cast(X, Y, X + 16, Y + 16, 3)

    def tex_hedge(self, px, py, lx, ly, face, edge):
        n = smooth(px, py, 3, self.seed + 120)
        x, y = px // TW, py // TW
        if edge:
            return HEDGE[2]
        if face:
            col = HEDGE[1] if n < 0.6 else HEDGE[3]
            return HEDGE[0] if (px + int(n * 5)) % 5 == 0 else col
        # the clipped top: leafy, lit at its upper-left, with a dark rim where the hedge ends
        for nx, ny, d in ((x - 1, y, lx), (x, y - 1, ly), (x + 1, y, 15 - lx)):
            c = self.at(nx, ny)
            if c is not None and c != 'C' and d == 0:
                return HEDGE[3]
        col = HEDGE[0] if n < 0.5 else HEDGE[2]
        if (lx + ly) < 6 and n > 0.4:
            col = (96, 164, 80)
        return HEDGE[3] if h2(px, py, self.seed + 121) < 0.07 else col

    def tex_iron(self, px, py, lx, ly, face, edge):
        if edge:
            return IRON[2]
        if face:
            col = IRON[1] if lx % 4 else IRON[3]
            return (150, 100, 50) if ly in (10, 11) else col
        col = IRON[0]
        if lx in (0, 15) or ly in (0, 15):
            col = IRON[3]
        elif lx in (1, 14) or ly in (1, 14):
            col = IRON[2]
        if (lx, ly) in ((3, 3), (12, 3), (3, 12), (12, 12)):
            col = (170, 176, 190)
        return col

    def tex_tower(self, px, py, lx, ly, face, edge):
        if edge:
            return TOWER[2]
        jx, jy = (px + (8 if (py // 6) % 2 else 0)) % 16, py % 6
        col = TOWER[0] if not face else TOWER[1]
        if jx == 0 or jy == 0:
            col = TOWER[3] if face else TOWER[1]
        if face and 9 <= ly <= 11:
            col = (190, 60, 50)
        return col

    def tex_bark(self, px, py, lx, ly, face, edge):
        n = smooth(px, py * 0.3, 3, self.seed + 130)
        col = BARK[0] if (px + int(n * 7)) % 6 else BARK[3]
        if face:
            col = BARK[1] if (px + int(n * 7)) % 6 else BARK[3]
        elif h2(px, py, self.seed + 131) < 0.05:
            col = BARK[2]
        if edge:
            col = BARK[2]
        if not face and smooth(px, py, 6, self.seed + 132) > 0.78:
            col = (84, 140, 66)
        return col

    def tex_shelf(self, px, py, lx, ly, face, edge):
        if not face:
            return (130, 88, 52) if py % 4 else (100, 66, 38)
        if edge or ly in (11, 15):
            return (90, 58, 32)
        books = [(170, 50, 46), (60, 90, 160), (70, 140, 70), (200, 170, 80), (120, 70, 140)]
        k = (px // 3 + (py // 5) * 3) % len(books)
        return books[k] if px % 3 else (60, 40, 26)

    def crystals(self):
        s = self.seed
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) != 'K':
                    continue
                X, Y = x * TW, y * TW
                for k in range(3):
                    bx = X + 2 + int(h2(x, y, s + 140 + k) * 10)
                    hgt = 8 + int(h2(x, y, s + 150 + k) * 7)
                    wid = 3 + int(h2(x, y, s + 160 + k) * 2)
                    base = Y + 15
                    for py in range(base - hgt, base):
                        t = (py - (base - hgt)) / hgt
                        half = max(1, int(wid * min(1.0, t * 1.6)))
                        for px in range(bx - half, bx + half + 1):
                            col = CRYSTAL[2] if px < bx else (CRYSTAL[0] if px == bx else CRYSTAL[1])
                            if px in (bx - half, bx + half):
                                col = CRYSTAL[3]
                            self.put(px, py, col)
                self.cast(X + 1, Y + 4, X + 15, Y + 15)

    def bridges(self):
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) != 'B':
                    continue
                X, Y = x * TW, y * TW
                l, r, u, d = (self.at(x - 1, y) or ''), (self.at(x + 1, y) or ''), (self.at(x, y - 1) or ''), (self.at(x, y + 1) or '')
                across = 'B' in (l + r) or (u in 'p~O' or d in 'p~O')
                for py in range(Y, Y + TW):
                    for px in range(X, X + TW):
                        k = (px - X) if across else (py - Y)
                        side = (py - Y) if across else (px - X)
                        if side in (1, 14):
                            col = (200, 180, 130)          # the hand ropes
                        elif side in (0, 15):
                            col = (40, 30, 24)
                        else:
                            col = (166, 116, 66) if k % 4 else (94, 60, 34)
                        self.img[py, px] = col

    def steps(self):
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) != 'S':
                    continue
                X, Y = x * TW, y * TW
                for py in range(Y, Y + TW):
                    for px in range(X, X + TW):
                        b = (py - Y) % 4
                        self.img[py, px] = STONE[2] if b == 0 else (STONE[3] if b == 3 else STONE[0])

    def pipes(self):
        cu = [(198, 122, 72), (160, 92, 54), (238, 176, 116), (106, 58, 34)]
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) != 'Z':
                    continue
                X, Y = x * TW, y * TW
                link = lambda c: c is not None and c in 'ZI'
                conn = {'n': link(self.at(x, y - 1)), 's': link(self.at(x, y + 1)), 'w': link(self.at(x - 1, y)),
                        'e': link(self.at(x + 1, y))}
                if not any(conn.values()):
                    conn['w'] = conn['e'] = True
                for py in range(Y, Y + TW):
                    for px in range(X, X + TW):
                        lx, ly = px - X, py - Y
                        hor = 5 <= ly <= 10 and ((lx <= 10 and conn['w']) or (lx >= 5 and conn['e']) or 5 <= lx <= 10)
                        ver = 5 <= lx <= 10 and ((ly <= 10 and conn['n']) or (ly >= 5 and conn['s']) or 5 <= ly <= 10)
                        if hor or ver:
                            t = (ly - 5) if hor and not ver else (lx - 5)
                            col = cu[2] if t == 1 else (cu[3] if t in (0, 5) else cu[0] if t < 4 else cu[1])
                            if (hor and lx in (0, 15)) or (ver and ly in (0, 15)):
                                col = cu[3]
                            self.img[py, px] = col
                self.cast(X + 4, Y + 4, X + 12, Y + 12, 2)

    def rails(self):
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) != 'V':
                    continue
                X, Y = x * TW, y * TW
                hor = (self.at(x - 1, y) == 'V' or self.at(x + 1, y) == 'V')
                for py in range(Y, Y + TW):
                    for px in range(X, X + TW):
                        a, b = (py - Y, px - X) if hor else (px - X, py - Y)
                        if b % 5 == 1 and 2 <= a <= 13:
                            self.img[py, px] = (100, 70, 44)       # sleepers
                        if a in (4, 11):
                            self.img[py, px] = (150, 150, 160)
                        elif a in (5, 12):
                            self.img[py, px] = (90, 90, 100)

    def reeds(self):
        s = self.seed
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) != 'q':
                    continue
                for k in range(6):
                    bx = x * TW + 1 + int(h2(x, y, s + 170 + k) * 14)
                    by = y * TW + 5 + int(h2(x, y, s + 180 + k) * 10)
                    for d in range(5):
                        self.put(bx, by - d, (70, 120, 52) if d < 3 else (120, 160, 80))
                    self.put(bx, by - 5, (110, 70, 40)); self.put(bx, by - 6, (110, 70, 40))

    def tracks(self):
        s = self.seed
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) != '_':
                    continue
                for k in range(3):
                    bx = x * TW + 3 + int(h2(x, y, s + 190 + k) * 9)
                    by = y * TW + 2 + k * 5
                    for dx, dy in ((0, 0), (1, 0), (0, 1), (3, 0), (4, 0), (4, 1)):
                        self.put(bx + dx, by + dy, (80, 64, 40))

    def boats(self):
        cells = {(x, y) for y in range(self.H) for x in range(self.W) if self.at(x, y) == 'U'}
        while cells:
            seed = cells.pop()
            group, todo = {seed}, [seed]
            while todo:
                cx, cy = todo.pop()
                for n in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                    if n in cells:
                        cells.discard(n); group.add(n); todo.append(n)
            xs, ys = [g[0] for g in group], [g[1] for g in group]
            x0, x1, y0, y1 = min(xs) * TW, (max(xs) + 1) * TW, min(ys) * TW, (max(ys) + 1) * TW
            cx, cy, rx, ry = (x0 + x1) / 2, (y0 + y1) / 2, (x1 - x0) / 2 - 1, (y1 - y0) / 2 - 2
            for py in range(y0, y1):
                for px in range(x0, x1):
                    dx, dy = (px + 0.5 - cx) / rx, (py + 0.5 - cy) / ry
                    d = dx * dx + dy * dy
                    if d > 1:
                        continue
                    col = (150, 100, 56) if d < 0.62 else ((96, 60, 34) if d > 0.85 else (186, 130, 74))
                    if d < 0.62 and (px - x0) % 6 == 0:
                        col = (120, 78, 44)
                    self.put(px, py, col)
            self.cast(x0 + 2, y0 + 2, x1 - 2, y1 - 2, 3)

    def desks(self):
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) != 'E':
                    continue
                X, Y = x * TW, y * TW
                for py in range(Y + 2, Y + 14):
                    for px in range(X + 1, X + 15):
                        self.img[py, px] = (178, 128, 76) if py < Y + 11 else (110, 72, 40)
                for py in range(Y + 4, Y + 9):
                    for px in range(X + 4, X + 11):
                        self.img[py, px] = (240, 236, 220) if (py - Y) % 2 else (200, 196, 186)
                self.cast(X + 1, Y + 2, X + 15, Y + 14, 2)

    def lush(self):
        """A restored room: more flowers in the grass."""
        if not self.wet or self.theme in ('grotto', 'works'):
            return
        s = self.seed
        cols = [(250, 250, 240), (250, 220, 80), (240, 120, 160), (150, 170, 250)]
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) not in '.,_':
                    continue
                if h2(x, y, s + 200) > 0.35:
                    continue
                for k in range(3):
                    fx = x * TW + 2 + int(h2(x, y, s + 210 + k) * 12)
                    fy = y * TW + 2 + int(h2(x, y, s + 220 + k) * 12)
                    col = cols[int(h2(x, y, s + 230 + k) * 4)]
                    self.put(fx, fy, col); self.put(fx + 1, fy, col); self.put(fx, fy + 1, sh(col, 0.8))

    def paint(self):
        self.paint_ground()
        self.flowers()
        self.lush()
        self.field_sprouts()
        self.water_edges()
        self.channels()
        self.specials()
        self.steps()
        self.bridges()
        self.rails()
        self.reeds()
        self.tracks()
        self.cliffs('#', self.th['cliff'], self.th['moss'])
        self.cliffs('X', self.th['cavewall'], None, joints=self.theme == 'works')
        self.blocks('C', self.tex_hedge)
        self.blocks('I', self.tex_iron)
        self.blocks('l', self.tex_tower)
        self.blocks('Q', self.tex_bark)
        self.blocks('J', self.tex_shelf)
        self.vines()
        self.rocks()
        self.bushes()
        self.crystals()
        self.pipes()
        self.boats()
        self.desks()
        self.houses()
        self.wheels()
        solid = np.zeros(self.shadow.shape, bool)
        for y in range(self.H):
            for x in range(self.W):
                if self.at(x, y) in 'T#XRHDPMWbrxoACIlQJ':
                    solid[y * TW:(y + 1) * TW, x * TW:(x + 1) * TW] = True
        self.apply_shadow(solid)
        self.trees(solid)
        img = self.img
        if self.dark:
            img = (img * 0.86).astype(np.uint8)
        rgba = np.dstack([img, np.full(img.shape[:2], 255, np.uint8)])
        return rgba


def paint(rows, wet=False, dark=False, seed=1, theme='valley'):
    return Room(rows, wet=wet, dark=dark, seed=seed, theme=theme).paint()
