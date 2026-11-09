"""Painted figurine sheets -> palette-slot native frames.

Every figure is drawn in key colours (red shirt, blue shorts, green socks, its
own skin and hair) on flat magenta. Each source pixel is given one of fifteen
fixed colour roles, then each native pixel takes the role most of its source
pixels have. Because the role is the palette index, a frame carries no colour
of its own: at run time a man's palette bank supplies his nation's kit and his
own skin and hair, so one set of frames dresses all the teams.

    roles   1 outline, 2-4 skin, 5-7 hair, 8-10 shirt, 11-12 shorts,
            13 ground shadow, 14 socks, 15 white

A frame is CANVAS_W x CANVAS_H with the feet at (ANCHOR_X, ANCHOR_Y); the same
frame comes in one size per depth band, every band on the one canvas so a man
walking up the pitch changes frame, never sprite shape.
"""
import numpy as np
from PIL import Image
from scipy import ndimage as ndi

import painted_cast
from painted_cast import cut, _lab

CANVAS_W, CANVAS_H = 64, 80
ANCHOR_X, ANCHOR_Y = 32, 74
BANDS = (52, 56, 60)                 # standing height far, middle, near (46 and 53 made the far men tiny)

OUTLINE, SKIN0, HAIR0, SHIRT0, SHORTS0, SHADOW, SOCKS, WHITE = 1, 2, 5, 8, 11, 13, 14, 15
KEY = [(16, 16, 24),
       (176, 112, 80), (232, 168, 128), (252, 216, 184),
       (32, 28, 44), (72, 64, 96), (136, 128, 168),
       (136, 16, 24), (224, 32, 32), (252, 96, 80),
       (20, 36, 140), (32, 64, 224),
       (30, 96, 44), (32, 192, 64),
       (248, 248, 248)]

# skin dark, mid, light; hair dark, mid, light, as drawn on each character
ANCHORS = {
    'c1': ((186, 118, 88), (238, 176, 134), (252, 214, 178), (24, 24, 36), (58, 62, 86), (110, 120, 150)),
    'c2': ((176, 112, 74), (222, 158, 112), (246, 198, 150), (18, 18, 24), (44, 44, 58), (84, 84, 104)),
    'c3': ((196, 132, 100), (240, 186, 150), (254, 222, 192), (192, 140, 20), (240, 196, 40), (252, 236, 120)),
    'c6': ((176, 112, 80), (228, 164, 122), (246, 204, 162), (58, 48, 46), (92, 74, 66), (120, 98, 86)),
    'c7': ((66, 38, 26), (104, 64, 44), (140, 92, 66), (18, 16, 18), (40, 34, 34), (70, 60, 58)),
    'c4': ((186, 118, 88), (238, 176, 134), (252, 214, 178), (112, 48, 20), (168, 80, 30), (206, 122, 52)),
    'c5': ((186, 118, 88), (238, 176, 134), (252, 214, 178), (24, 22, 28), (60, 56, 64), (120, 112, 120)),
}

# how much a role counts when a native pixel is decided: thin lines and eye
# whites must survive the reduction, and the outside must not eat the outline
WEIGHT = np.ones(16, np.float32)
WEIGHT[0] = 0.70
WEIGHT[OUTLINE] = 1.35
WEIGHT[WHITE] = 1.25


def no_magenta(rgb):
    """The flat background, its fringe, and any ground shadow painted into it."""
    h, s, v = hsv(rgb)
    return (h > 0.76) & (h < 0.93) & (s > 0.35)


painted_cast.background = no_magenta


def hsv(rgb):
    f = rgb.astype(np.float32) / 255.0
    r, g, b = f[..., 0], f[..., 1], f[..., 2]
    mx, mn = f.max(-1), f.min(-1)
    d = mx - mn
    dd = np.maximum(d, 1e-6)
    h = np.where(mx == r, ((g - b) / dd) % 6, np.where(mx == g, (b - r) / dd + 2, (r - g) / dd + 4)) / 6.0
    return h, np.where(mx > 0, d / np.maximum(mx, 1e-6), 0), mx


def tones(v, n, cuts):
    """A class's brightness to its n tones (dark first)."""
    return np.digitize(v, cuts).astype(np.uint8)


def roles(rgb, mask, who):
    """One role per source pixel of a figure; 0 outside it."""
    h, s, v = hsv(rgb)
    out = np.zeros(mask.shape, np.uint8)
    red = (s > 0.55) & (v > 0.28) & ((h < 0.035) | (h > 0.955))
    blue = (s > 0.45) & (h > 0.56) & (h < 0.74) & (v > 0.2)
    green = (s > 0.38) & (h > 0.22) & (h < 0.46) & (v > 0.2)
    white = (s < 0.14) & (v > 0.78)
    grey = (s < 0.12) & ~white
    done = red | blue | green | white | grey
    a = ANCHORS[who]
    anchors = np.array((KEY[0],) + a, np.uint8).reshape(1, -1, 3)
    al = _lab(anchors)
    px = _lab(rgb.reshape(-1, 3).astype(np.uint8))
    d = ((px[:, None, :] - al[None]) ** 2).sum(2)
    near = d.argmin(1).reshape(mask.shape)
    out[:] = np.array([OUTLINE, SKIN0, SKIN0 + 1, SKIN0 + 2, HAIR0, HAIR0 + 1, HAIR0 + 2], np.uint8)[near]
    if who not in ('c3', 'c6'):
        # a face's own shade (eye sockets, cheeks, the corners of the mouth) is warm and
        # lands on a hair tone; with another man's hair it was a mask. Warm and coloured
        # enough to be skin: the skin's shade. (c3's blond and c6's brown hair are warm
        # themselves and are sorted below and left alone.)
        hair = (out >= HAIR0) & (out <= HAIR0 + 2)
        warm = ((h < 0.12) | (h > 0.95)) & (s > 0.30) & (v > 0.16)
        out[hair & warm] = SKIN0
    if who == 'c3':
        # blond hair is a saturated gold; stubble and the shade of the jaw are skin, not
        # hair, or a dark-haired man built on this figure ends up wearing a mask
        hair = (out >= HAIR0) & (out <= HAIR0 + 2)
        pale = hair & ((s < 0.62) | (h < 0.095) | (h > 0.95))   # the face's orange shade is redder than gold
        d3 = d[:, 1:4].argmin(1).reshape(mask.shape).astype(np.uint8)
        out[pale] = SKIN0 + d3[pale]
    out[red] = SHIRT0 + tones(v[red], 3, (0.56, 0.93))
    out[blue] = SHORTS0 + tones(v[blue], 2, (0.70,))
    out[green] = SOCKS
    out[white] = WHITE
    out[grey] = np.where(v[grey] > 0.52, WHITE, OUTLINE)
    out[~mask] = 0
    return out


def reduce_roles(lab, scale):
    """The roles of a figure at `scale`: each native pixel the weighted majority."""
    h, w = lab.shape
    nh, nw = max(1, int(round(h * scale))), max(1, int(round(w * scale)))
    votes = np.zeros((16, nh, nw), np.float32)
    for r in np.unique(lab):
        plane = Image.fromarray((lab == r).astype(np.float32), 'F')
        votes[r] = np.asarray(plane.resize((nw, nh), Image.BOX)) * WEIGHT[r]
    return votes.argmax(0).astype(np.uint8)


def despeckle(f):
    """Lone pixels are noise: a pixel with almost no neighbours of its own role takes the role
    around it. The outline and the whites of the eyes are left alone: they are thin on purpose."""
    out = f.copy()
    h, w = f.shape
    for _ in range(2):
        src = out.copy()
        for y in range(1, h - 1):
            for x in range(1, w - 1):
                r = src[y, x]
                if r in (0, OUTLINE, WHITE):
                    continue
                n = src[y - 1:y + 2, x - 1:x + 2].ravel()
                if (n == r).sum() - 1 > 1:
                    continue
                c = np.bincount(n[n > 1], minlength=16)
                c[r] = 0
                c[WHITE] = 0
                if c.max() >= 3:
                    out[y, x] = c.argmax()
    return out


def face_clean(f):
    """The head is skin, hair and the outline. Painted stubble and jaw shade come out
    nearest a hair tone and lips and blush nearest the shirt's red, and at native size
    those were a mask in the hair's colour and the kit's colour on a man's face. In the
    head band: a shirt pixel among skin is the skin's shade; a hair pixel mostly among
    skin is the skin's shade too (stubble), or the outline when it is a thin line with
    skin above and below (a brow, which must stay)."""
    out = f.copy()
    ys, xs = np.nonzero(f)
    if not len(ys):
        return out
    top, bottom = int(ys.min()), int(ys.max())
    head = top + max(4, int((bottom - top + 1) * 0.22))
    h, w = f.shape

    def is_skin(r):
        return SKIN0 <= r <= SKIN0 + 2

    def is_hair(r):
        return HAIR0 <= r <= HAIR0 + 2

    # the eyes: white among skin, the first such row from the top of the head. Below
    # them is cheek and jaw, where no hair tone belongs (the man faces away: no eyes)
    eye = None
    for y in range(max(1, top), min(head, h - 1)):
        for x in range(1, w - 1):
            if f[y, x] == WHITE and sum(1 for v in f[y - 1:y + 2, x - 1:x + 2].ravel() if is_skin(v)) >= 2:
                eye = y
                break
        if eye is not None:
            break
    for _ in range(3):
        src = out.copy()
        for y in range(max(1, top), min(head, h - 1)):
            for x in range(1, w - 1):
                r = src[y, x]
                n = src[y - 1:y + 2, x - 1:x + 2].ravel()
                skins = sum(1 for v in n if is_skin(v))
                if SHIRT0 <= r <= SHIRT0 + 2 and skins >= 3:
                    out[y, x] = SKIN0
                elif is_hair(r) and eye is not None and y > eye and skins >= 3:
                    out[y, x] = SKIN0                       # stubble and jaw shade: the skin's shade
                elif is_hair(r) and skins >= 5:
                    thin = is_skin(src[y - 1, x]) and is_skin(src[y + 1, x])
                    out[y, x] = OUTLINE if thin and skins <= 6 else SKIN0
    return out


def tidy(f):
    """Specks off, and a closed outline wherever the figure meets the outside."""
    f = despeckle(f)
    solid = f > 0
    lab, n = ndi.label(solid, np.ones((3, 3), bool))
    if n > 1:
        sizes = ndi.sum(solid, lab, range(1, n + 1))
        keep = np.isin(lab, [i + 1 for i, s in enumerate(sizes) if s >= 6])
        f = np.where(keep, f, 0).astype(np.uint8)
        solid = f > 0
    edge = solid & ~ndi.binary_erosion(solid, np.array([[0, 1, 0], [1, 1, 1], [0, 1, 0]], bool))
    keep_colour = (f == WHITE)                        # soles and bands may touch the outside
    f = np.where(edge & ~keep_colour, OUTLINE, f).astype(np.uint8)
    return f


def shadow(canvas, width):
    """A flat oval on the grass under the figure, a little toward the sun's far side."""
    rx, ry = width / 2.0, max(2.0, width / 5.0)
    cx0, cy0 = ANCHOR_X + 2, ANCHOR_Y - 1
    ys, xs = np.mgrid[0:CANVAS_H, 0:CANVAS_W]
    m = ((xs - cx0) ** 2) / (rx * rx) + ((ys - cy0) ** 2) / (ry * ry) <= 1.0
    canvas[m & (canvas == 0)] = SHADOW


def place(f, air=False, lift=0, shift=0, height=60):
    """A native figure on the canvas: feet on the anchor, the body over it."""
    canvas = np.zeros((CANVAS_H, CANVAS_W), np.uint8)
    ys, xs = np.nonzero(f)
    if not len(ys):
        return canvas
    top, bottom, left, right = ys.min(), ys.max(), xs.min(), xs.max()
    f = f[top:bottom + 1, left:right + 1]
    fh, fw = f.shape
    upper = (f[:max(1, fh * 6 // 10)] > 0)
    mass = upper.sum(0).astype(np.float64)
    centre = int(round((np.arange(fw) * mass).sum() / max(1.0, mass.sum())))
    x0 = ANCHOR_X - centre + shift
    x0 = max(0, min(x0, CANVAS_W - fw))
    y0 = ANCHOR_Y - fh + 1 - lift
    if air:
        y0 = min(y0, ANCHOR_Y - fh - 4 - height // 6)
    shadow(canvas, max(16, height * 11 // 20))
    y0 = max(0, y0)
    region = canvas[y0:y0 + fh, x0:x0 + fw]
    sub = f[:region.shape[0], :region.shape[1]]
    region[sub > 0] = sub[sub > 0]
    return canvas


def height_of(lab):
    ys = np.nonzero((lab > 0).any(1))[0]
    return float(ys.max() - ys.min() + 1)


class Figure:
    """One character: named figures from its sheets, one standing height."""
    def __init__(self, who, stand='stand_f'):
        self.who = who
        self.stand = stand
        self.labs = {}
        self.scale = {}
        self.sheets = []

    def add_sheet(self, path, names):
        sheet = []
        for n, (rgb, mask) in cut(path, 2, names, min_area=2500, merge_gap=10).items():
            self.labs[n] = roles(rgb, mask, self.who)
            self.scale[n] = 1.0
            sheet.append(n)
        self.sheets.append(sheet)

    def normalise(self):
        """Sheets come out at different sizes: every man's head is one size, so
        take each sheet's typical head width to the first sheet's."""
        def head(lab):
            ys = np.nonzero((lab > 0).any(1))[0]
            band = lab[ys.min():ys.min() + max(1, (ys.max() - ys.min()) * 3 // 10)]
            xs = np.nonzero((band > 0).any(0))[0]
            return float(xs.max() - xs.min() + 1)
        sizes = [float(np.median([head(self.labs[n]) for n in sheet])) for sheet in self.sheets]
        if not sizes:
            return
        for sheet, size in zip(self.sheets, sizes):
            for n in sheet:
                self.scale[n] = sizes[0] / size

    def frame(self, name, band, air=False):
        target = BANDS[band]
        s = target / (height_of(self.labs[self.stand]) * self.scale[self.stand]) * self.scale[name]
        return place(tidy(face_clean(reduce_roles(self.labs[name], s))), air=air, height=target)

    def big_sheet(self, order, w, h, height):
        idx = np.concatenate([big(self, n, w, h, height) for n in order], 1)
        return rgba_of(idx)

    def sheet(self, order, air=(), cols=20):
        """All frames band by band on one role sheet; returns RGBA, frames per band, and for each
        frame the strips and rows it really uses, and where a shirt number goes."""
        frames = [self.frame(n, b, n in air) for b in range(len(BANDS)) for n in order]
        names = [n for b in range(len(BANDS)) for n in order]
        rows = (len(frames) + cols - 1) // cols
        idx = np.zeros((rows * CANVAS_H, cols * CANVAS_W), np.uint8)
        meta = []
        for i, f in enumerate(frames):
            y, x = (i // cols) * CANVAS_H, (i % cols) * CANVAS_W
            idx[y:y + CANVAS_H, x:x + CANVAS_W] = f
            meta.append(frame_meta(f, names[i]))
        return rgba_of(idx), len(order), meta


def big(fig, name, w, h, height):
    """A figure large, on its own canvas, feet at the bottom."""
    s = height / (height_of(fig.labs[fig.stand]) * fig.scale[fig.stand]) * fig.scale[name]
    f = tidy(face_clean(reduce_roles(fig.labs[name], s)))
    ys, xs = np.nonzero(f)
    f = f[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    canvas = np.zeros((h, w), np.uint8)
    fh, fw = min(f.shape[0], h), min(f.shape[1], w)
    y0, x0 = h - fh, (w - fw) // 2
    canvas[y0:y0 + fh, x0:x0 + fw] = f[:fh, :fw]
    return canvas


def frame_meta(f, name):
    """(first strip, last strip, first row, last row, number x, number y) of a frame."""
    ys, xs = np.nonzero(f)
    if not len(ys):
        return (0, 0, 0, 0, 0, 0)
    c0, c1, r0, r1 = int(xs.min()) // 16, int(xs.max()) // 16, int(ys.min()) // 16, int(ys.max()) // 16
    nx = ny = 0
    # numbers show on a shirt seen from behind or the front, where it is a broad flat shape
    if name.split('_')[0] in ('stand', 'run', 'walk') and name[:7] != 'stand_s' and name[:5] != 'run_s':
        sy, sx = np.nonzero((f >= SHIRT0) & (f <= SHIRT0 + 2))
        if len(sy) >= 40:
            nx, ny = int(sx.mean()), int(sy.mean())
    return (c0, c1, r0, r1, nx, ny)


def rgba_of(idx):
    pal = np.array([(0, 0, 0)] + KEY, np.uint8)
    out = np.zeros(idx.shape + (4,), np.uint8)
    out[..., :3] = pal[idx]
    out[..., 3] = np.where(idx > 0, 255, 0)
    return out
