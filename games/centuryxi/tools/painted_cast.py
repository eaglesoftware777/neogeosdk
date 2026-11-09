"""Painted character sheets -> native RGBA frame sheets for tools/ng_artpack.py.

A sheet is figures on flat magenta, in reading order (rows top to bottom,
left to right). One character's sheets share one scale, taken from a
reference pose, so every frame keeps the same size on screen.

    figs = cut('kane_s1.png', 2, ['stand', 'shoot', ...])  # name -> (rgb, mask)
    cast = Cast(height=88, ref='stand')
    cast.add(figs)                                          # more sheets: add again
    sheet, frame, anchor, names = cast.sheet(order, air={'jump', 'fall'})
    pack.sprite('kane', sheet, frame=frame, banks=4)

Reduction keeps pixel art crisp: the character's colours are clustered once
(Lab k-means over all its figures), every source pixel takes its nearest
colour, and each native pixel takes the colour most of its source pixels
have (transparent counts a little less, so outlines survive). ng_artpack
then fits the result to the character's palette banks.

Placement: the canvas is the smallest multiple of 16 that holds every frame
around a common anchor, the feet of the standing poses (bottom of the
opaque pixels, centre of the upper body). Airborne poses (`air`) are
centred on the standing pose's middle instead of resting on the floor.
"""
import numpy as np
from PIL import Image
from scipy import ndimage as ndi


def background(rgb):
    """Flat magenta and the fringe blended into it."""
    f = rgb.astype(np.int32)
    r, g, b = f[..., 0], f[..., 1], f[..., 2]
    return (r > 140) & (b > 140) & (g < 120) & (r - g > 70) & (b - g > 70)


def cut(path, rows, names, min_area=1200, merge_gap=12):
    """The figures of a sheet as {name: (rgb crop, mask crop)}, reading order."""
    rgb = np.asarray(Image.open(path).convert('RGB'))
    fg = ~ndi.binary_dilation(background(rgb), iterations=1)
    fg = ndi.binary_opening(fg, iterations=1)
    lab, n = ndi.label(fg, np.ones((3, 3), bool))
    boxes = []
    for i, sl in enumerate(ndi.find_objects(lab)):
        area = int((lab[sl] == i + 1).sum())
        if area >= 30:
            boxes.append([sl[1].start, sl[0].start, sl[1].stop, sl[0].stop, area, [i + 1]])
    boxes.sort(key=lambda b: -b[4])
    merged = []
    for b in boxes:                      # muzzle flashes, sparks, a detached scarf end
        host = None
        for m in merged:
            if (b[0] < m[2] + merge_gap and b[2] > m[0] - merge_gap and b[1] < m[3] + merge_gap
                    and b[3] > m[1] - merge_gap and (b[4] < min_area or m[4] < min_area)):
                host = m
                break
        if host is None:
            merged.append(list(b))
        else:
            host[0], host[1] = min(host[0], b[0]), min(host[1], b[1])
            host[2], host[3] = max(host[2], b[2]), max(host[3], b[3])
            host[4] += b[4]
            host[5] += b[5]
    merged = [m for m in merged if m[4] >= min_area]
    # a dropped gun or a flying shield is a figure of its own: fold the smallest
    # into its nearest neighbour until the count is right
    while len(merged) > len(names):
        k = min(range(len(merged)), key=lambda i: merged[i][4])
        b = merged.pop(k)
        bx, by = (b[0] + b[2]) / 2, (b[1] + b[3]) / 2
        host = min(merged, key=lambda m: (max(m[0] - bx, 0, bx - m[2]) ** 2 + max(m[1] - by, 0, by - m[3]) ** 2))
        host[0], host[1] = min(host[0], b[0]), min(host[1], b[1])
        host[2], host[3] = max(host[2], b[2]), max(host[3], b[3])
        host[4] += b[4]
        host[5] += b[5]
    cy = np.array([(m[1] + m[3]) / 2 for m in merged])
    order = list(np.argsort(cy))
    groups, cur = [], []
    for i in order:
        if cur and cy[i] - np.mean([cy[j] for j in cur]) > rgb.shape[0] / (rows * 2.4):
            groups.append(cur)
            cur = []
        cur.append(i)
    if cur:
        groups.append(cur)
    seq = [merged[i] for g in groups for i in sorted(g, key=lambda j: merged[j][0])]
    if len(seq) != len(names):
        raise ValueError(f'{path}: {len(seq)} figures, expected {len(names)}')
    out = {}
    for name, m in zip(names, seq):
        x0, y0, x1, y1 = m[0], m[1], m[2], m[3]
        mask = np.isin(lab[y0:y1, x0:x1], m[5])
        mask = ndi.binary_fill_holes(mask) & ~background(rgb[y0:y1, x0:x1])
        out[name] = (rgb[y0:y1, x0:x1].copy(), mask)
    return out


def _lab(rgb):
    c = rgb.reshape(-1, 3).astype(np.float64) / 255.0
    c = np.where(c > 0.04045, ((c + 0.055) / 1.055) ** 2.4, c / 12.92)
    xyz = c @ np.array([[0.4124, 0.3576, 0.1805], [0.2126, 0.7152, 0.0722], [0.0193, 0.1192, 0.9505]]).T
    xyz /= np.array([0.9505, 1.0, 1.089])
    f = np.where(xyz > 0.008856, np.cbrt(xyz), 7.787 * xyz + 16 / 116)
    return np.stack([116 * f[:, 1] - 16, 500 * (f[:, 0] - f[:, 1]), 200 * (f[:, 1] - f[:, 2])], 1)


def _kmeans(x, k, iters=30, seed=5):
    rng = np.random.default_rng(seed)
    cent = x[rng.choice(len(x), k, replace=False)]
    for _ in range(iters):
        d = ((x[:, None, :] - cent[None]) ** 2).sum(2)
        lab = d.argmin(1)
        for j in range(k):
            sel = x[lab == j]
            if len(sel):
                cent[j] = sel.mean(0)
    return cent, lab


class Cast:
    def __init__(self, height, ref='stand', colours=48, transparent_weight=0.75):
        self.height = height
        self.ref = ref
        self.colours = colours
        self.tw = transparent_weight
        self.figs = {}
        self._frames = None

    def add(self, figs):
        self.figs.update(figs)
        self._frames = None

    def _palette(self):
        samples = []
        for rgb, mask in self.figs.values():
            inner = ndi.binary_erosion(mask, iterations=1)      # no magenta fringe in the palette
            px = rgb[inner]
            if len(px) > 6000:
                px = px[np.random.default_rng(1).choice(len(px), 6000, replace=False)]
            samples.append(px)
        px = np.concatenate(samples)
        cent, lab = _kmeans(_lab(px), self.colours)
        pal = np.array([px[lab == j].mean(0) if (lab == j).any() else (0, 0, 0) for j in range(self.colours)])
        return np.clip(np.rint(pal), 0, 255).astype(np.uint8), cent

    def _native(self, rgb, mask, scale, pal, cent):
        h, w = mask.shape
        lab = _lab(rgb)
        idx = ((lab[:, None, :] - cent[None]) ** 2).sum(2).argmin(1).reshape(h, w) + 1
        idx[~mask] = 0
        oh, ow = max(1, int(round(h * scale))), max(1, int(round(w * scale)))
        ys = np.minimum((np.arange(h) * oh) // h, oh - 1)
        xs = np.minimum((np.arange(w) * ow) // w, ow - 1)
        cell = ys[:, None] * ow + xs[None, :]
        k = len(pal) + 1
        counts = np.bincount((cell * k + idx).ravel(), minlength=oh * ow * k).reshape(oh * ow, k).astype(np.float64)
        counts[:, 0] *= self.tw
        best = counts.argmax(1).reshape(oh, ow)
        out = np.zeros((oh, ow, 4), np.uint8)
        solid = best > 0
        out[solid, :3] = pal[best[solid] - 1]
        out[solid, 3] = 255
        return out

    def native_frames(self):
        """Every figure reduced to native pixels (computed once; several sheets
        can be cut from one cast)."""
        if self._frames is not None:
            return self._frames
        ref_mask = self.figs[self.ref][1]
        ys = np.nonzero(ref_mask.any(1))[0]
        scale = self.height / float(ys.max() - ys.min() + 1)
        pal, cent = self._palette()
        self._frames = ({n: self._native(rgb, mask, scale, pal, cent) for n, (rgb, mask) in self.figs.items()}, scale)
        return self._frames

    def sheet(self, order, air=(), cols=None, margin=1, recolour=None):
        """Frames in `order` on one canvas each, left to right (cols per row).
        `recolour`, if given, maps each native RGBA frame to a new one first."""
        frames, _ = self.native_frames()
        if recolour is not None:
            frames = {n: recolour(f) for n, f in frames.items() if n in order or n == self.ref}

        def anchor_of(f):
            a = f[..., 3] > 0
            ys, xs = np.nonzero(a)
            top, bottom = ys.min(), ys.max()
            upper = a[top:top + max(1, (bottom - top) * 6 // 10)]
            cx = np.nonzero(upper.any(0))[0]
            col_mass = upper.sum(0)
            centre = int(round((np.arange(len(col_mass)) * col_mass).sum() / max(1, col_mass.sum())))
            return centre, bottom + 1, top

        ref_cx, ref_bottom, ref_top = anchor_of(frames[self.ref])
        mid_from_feet = (ref_bottom - ref_top) // 2
        placed = {}
        lo_x = lo_y = 0
        hi_x = hi_y = 0
        for n in order:
            f = frames[n]
            cx, bottom, top = anchor_of(f)
            if n in air:
                ys = np.nonzero((f[..., 3] > 0).any(1))[0]
                dy = -(int((ys.min() + ys.max()) // 2) + mid_from_feet)   # middle at the standing middle
            else:
                dy = -bottom
            dx = -cx
            placed[n] = (f, dx, dy)
            lo_x, lo_y = min(lo_x, dx), min(lo_y, dy)
            hi_x, hi_y = max(hi_x, dx + f.shape[1]), max(hi_y, dy + f.shape[0])
        left, up = -lo_x + margin, -lo_y + margin
        fw = ((hi_x - lo_x + 2 * margin) + 15) // 16 * 16
        fh = ((hi_y - lo_y + 2 * margin) + 15) // 16 * 16
        cols = cols or len(order)
        rows = (len(order) + cols - 1) // cols
        sheet = np.zeros((rows * fh, cols * fw, 4), np.uint8)
        for i, n in enumerate(order):
            f, dx, dy = placed[n]
            x0 = (i % cols) * fw + left + dx
            y0 = (i // cols) * fh + up + dy
            region = sheet[y0:y0 + f.shape[0], x0:x0 + f.shape[1]]
            m = f[..., 3] > 0
            region[m] = f[m]
        return sheet, (int(fw), int(fh)), (int(left), int(up)), list(order)


def grid(path, rows, cols, min_px=12):
    """The cells of an evenly gridded sheet as a list of (rgb, mask), reading
    order; each crop is the bounding box of the cell's non-magenta pixels."""
    rgb = np.asarray(Image.open(path).convert('RGB'))
    h, w = rgb.shape[:2]
    fg = ~ndi.binary_dilation(background(rgb), iterations=1)
    out = []
    for r in range(rows):
        for c in range(cols):
            y0, y1 = r * h // rows, (r + 1) * h // rows
            x0, x1 = c * w // cols, (c + 1) * w // cols
            m = fg[y0:y1, x0:x1].copy()
            m[:3, :] = m[-3:, :] = False
            m[:, :3] = m[:, -3:] = False
            m = ndi.binary_opening(m, iterations=1)
            ys, xs = np.nonzero(m)
            if len(ys) < min_px:
                out.append((np.zeros((1, 1, 3), np.uint8), np.zeros((1, 1), bool)))
                continue
            a, b, cc, d = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
            out.append((rgb[y0 + a:y0 + b, x0 + cc:x0 + d].copy(), m[a:b, cc:d]))
    return out


def fit(cells, box, fill=None, colours=40):
    """Each (rgb, mask) reduced to fit `fill` pixels (default: the box less 2)
    and centred on a box x box canvas; one sheet, left to right."""
    fill = fill or box - 2
    cast = Cast(height=1, colours=colours)
    cast.figs = {i: c for i, c in enumerate(cells) if c[1].any()}
    pal, cent = cast._palette()
    sheet = np.zeros((box, box * len(cells), 4), np.uint8)
    for i, (rgb, mask) in enumerate(cells):
        if not mask.any():
            continue
        scale = min(fill / mask.shape[1], fill / mask.shape[0])
        f = cast._native(rgb, mask, scale, pal, cent)
        y0 = (box - f.shape[0]) // 2
        x0 = i * box + (box - f.shape[1]) // 2
        region = sheet[y0:y0 + f.shape[0], x0:x0 + f.shape[1]]
        m = f[..., 3] > 0
        region[m] = f[m]
    return sheet


def logo(path, w, h, colours=44):
    """A painted logo on flat magenta, reduced to fit w x h (less a pixel each
    side) and centred on a w x h RGBA canvas."""
    rgb = np.asarray(Image.open(path).convert('RGB'))
    mask = ~ndi.binary_dilation(background(rgb), iterations=1)
    mask = ndi.binary_opening(mask, iterations=1)
    lab, n = ndi.label(mask, np.ones((3, 3), bool))
    if n:
        sizes = ndi.sum(mask, lab, range(1, n + 1))
        mask = np.isin(lab, [i + 1 for i, s in enumerate(sizes) if s >= 40])   # specks off
    ys, xs = np.nonzero(mask)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    crop, m = rgb[y0:y1, x0:x1], mask[y0:y1, x0:x1]
    scale = min((w - 2) / m.shape[1], (h - 2) / m.shape[0])
    cast = Cast(height=1, colours=colours)
    cast.figs = {0: (crop, m)}
    pal, cent = cast._palette()
    f = cast._native(crop, m, scale, pal, cent)
    out = np.zeros((h, w, 4), np.uint8)
    oy, ox = (h - f.shape[0]) // 2, (w - f.shape[1]) // 2
    region = out[oy:oy + f.shape[0], ox:ox + f.shape[1]]
    region[f[..., 3] > 0] = f[f[..., 3] > 0]
    return out
