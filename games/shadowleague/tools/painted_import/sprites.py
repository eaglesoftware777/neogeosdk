"""Painted fighter sheets -> native indexed 160x160 frames for Shadow League.

    figures(sheet, rows)            -> list of (box, rgb crop, alpha crop) in reading order
    palette(crops, k)               -> fighter palette (darkest first), Lab k-means
    native(rgb, alpha, scale, pal)  -> indexed native drawing (majority of palette labels)
    place(idx, bottom=141)          -> 160x160 canvas, body centre at x=80
"""
import numpy as np
from PIL import Image
from scipy import ndimage as ndi

CANVAS = 192
ROOT = (96, 173)


def load(path):
    return np.asarray(Image.open(path).convert('RGB'))


def background(rgb):
    """Pure magenta and its blended fringe."""
    f = rgb.astype(int)
    r, g, b = f[..., 0], f[..., 1], f[..., 2]
    return (r > 150) & (b > 150) & (g < 110) & (r - g > 80) & (b - g > 80)


def figures(rgb, rows, min_area=1500, merge_gap=10):
    bg = background(rgb)
    fg = ndi.binary_opening(~bg, iterations=1)
    lab, n = ndi.label(fg, np.ones((3, 3), bool))
    objs = ndi.find_objects(lab)
    areas = ndi.sum(fg, lab, range(1, n + 1))
    boxes = []
    for i, (sl, a) in enumerate(zip(objs, areas)):
        if a < 40:
            continue
        boxes.append([sl[1].start, sl[0].start, sl[1].stop, sl[0].stop, a, [i + 1]])
    # merge detached bits (sweat, motion marks, a glove gap) into the figure they touch
    boxes.sort(key=lambda b: -b[4])
    merged = []
    for b in boxes:
        host = None
        for m in merged:
            if b[0] < m[2] + merge_gap and b[2] > m[0] - merge_gap and b[1] < m[3] + merge_gap and b[3] > m[1] - merge_gap \
                    and (b[4] < min_area or m[4] < min_area):
                host = m
                break
        if host is None:
            merged.append(list(b))
        else:
            host[0] = min(host[0], b[0]); host[1] = min(host[1], b[1])
            host[2] = max(host[2], b[2]); host[3] = max(host[3], b[3])
            host[4] += b[4]; host[5] += b[5]
    merged = [m for m in merged if m[4] >= min_area]
    # rows by vertical centre, then left to right
    cy = np.array([(m[1] + m[3]) / 2 for m in merged])
    order = np.argsort(cy)
    groups, cur = [], []
    for i in order:
        if cur and cy[i] - np.mean([cy[j] for j in cur]) > rgb.shape[0] / (rows * 2.2):
            groups.append(cur); cur = []
        cur.append(i)
    if cur:
        groups.append(cur)
    out = []
    for g in groups:
        for i in sorted(g, key=lambda j: merged[j][0]):
            x0, y0, x1, y1, _, labels = merged[i]
            mask = np.isin(lab[y0:y1, x0:x1], labels)
            out.append(((x0, y0, x1, y1), rgb[y0:y1, x0:x1], mask))
    return out, len(groups)


# ---------------------------------------------------------------- colour
def to_lab(rgb):
    c = rgb.astype(float) / 255.0
    c = np.where(c > 0.04045, ((c + 0.055) / 1.055) ** 2.4, c / 12.92)
    m = np.array([[0.4124, 0.3576, 0.1805], [0.2126, 0.7152, 0.0722], [0.0193, 0.1192, 0.9505]])
    xyz = c @ m.T / np.array([0.9505, 1.0, 1.089])
    f = np.where(xyz > 0.008856, np.cbrt(xyz), 7.787 * xyz + 16 / 116)
    return np.stack([116 * f[..., 1] - 16, 500 * (f[..., 0] - f[..., 1]), 200 * (f[..., 1] - f[..., 2])], -1)


def kmeans(x, k, iters=40, seed=7):
    rng = np.random.default_rng(seed)
    # k-means++ seeding keeps small saturated accents (gloves, belts) alive
    c = [x[rng.integers(len(x))]]
    for _ in range(1, k):
        d = np.min(((x[:, None, :] - np.array(c)[None]) ** 2).sum(2), 1)
        c.append(x[rng.choice(len(x), p=d / d.sum())])
    c = np.array(c)
    for _ in range(iters):
        lab = ((x[:, None, :] - c[None]) ** 2).sum(2).argmin(1)
        for j in range(k):
            sel = x[lab == j]
            if len(sel):
                c[j] = sel.mean(0)
    return c, lab


def palette(crops, k=15, sample=120000, outline=(16, 12, 20)):
    """k colours for one fighter; slot 1 is the outline ink."""
    px = []
    for rgb, mask in crops:
        inner = ndi.binary_erosion(mask, iterations=2)
        px.append(rgb[inner])
    px = np.concatenate(px)
    rng = np.random.default_rng(1)
    px = px[rng.choice(len(px), min(sample, len(px)), replace=False)]
    lab = to_lab(px)
    centres, labels = kmeans(lab, k - 1)
    rgbc = np.array([px[labels == j].mean(0) if (labels == j).any() else px[0] for j in range(k - 1)])
    rgbc = rgbc[np.argsort(to_lab(rgbc)[:, 0])]
    pal = np.vstack([np.array(outline, float), rgbc])
    return snap(pal)


def snap(pal):
    """Neo Geo colour: 5 bits per gun (plus the shared dark bit, ignored here)."""
    return (np.round(np.asarray(pal, float) / 255 * 31) * 255 / 31).round().astype(np.uint8)


def label(rgb, mask, pal):
    lab = to_lab(rgb.reshape(-1, 3)).reshape(rgb.shape)
    pl = to_lab(pal)
    d = ((lab[..., None, :] - pl[None, None]) ** 2).sum(-1)
    idx = d.argmin(-1).astype(np.int16) + 1
    idx[~mask] = 0
    return idx


def majority(idx, H, W, k):
    h, w = idx.shape
    out = np.zeros((H, W), np.uint8)
    ys = np.linspace(0, h, H + 1).astype(int)
    xs = np.linspace(0, w, W + 1).astype(int)
    for j in range(H):
        a, b = ys[j], max(ys[j + 1], ys[j] + 1)
        for i in range(W):
            c, e = xs[i], max(xs[i + 1], xs[i] + 1)
            cnt = np.bincount(idx[a:b, c:e].ravel(), minlength=k + 1)
            if cnt[0] * 2 > cnt.sum():
                continue
            cnt[0] = 0
            # ink wins ties: thin dark lines survive the reduction
            if cnt[1] * 3 >= cnt.max():
                out[j, i] = 1
            else:
                out[j, i] = cnt.argmax()
    return out


def clean(idx, pal):
    on = idx > 0
    lab, n = ndi.label(on, np.ones((3, 3)))
    if n > 1:
        sizes = ndi.sum(on, lab, range(1, n + 1))
        big = sizes.max()
        for i, s in enumerate(sizes, 1):
            if s < max(6, big * 0.01):
                idx[lab == i] = 0
    holes = ndi.binary_fill_holes(idx > 0) & (idx == 0)
    for y, x in zip(*np.nonzero(holes)):
        nb = idx[max(0, y - 1):y + 2, max(0, x - 1):x + 2].ravel()
        nb = nb[nb > 0]
        if len(nb):
            idx[y, x] = np.bincount(nb).argmax()
    # isolated single pixels of a colour that appears in no neighbour
    out = idx.copy()
    h, w = idx.shape
    for y in range(1, h - 1):
        for x in range(1, w - 1):
            v = idx[y, x]
            if v <= 1:
                continue
            nb = idx[y - 1:y + 2, x - 1:x + 2].ravel().tolist()
            nb.remove(v)
            if v in nb:
                continue
            opq = [q for q in nb if q]
            if len(opq) >= 7:
                out[y, x] = np.bincount(opq).argmax()
    return out


def outline(idx, pal, light=55.0):
    """Ink outside every light edge pixel so the figure reads against any stage."""
    L = to_lab(pal)[:, 0]
    on = idx > 0
    lightpx = on & (L[np.clip(idx.astype(int) - 1, 0, len(pal) - 1)] > light)
    ring = ndi.binary_dilation(lightpx, np.array([[0, 1, 0], [1, 1, 1], [0, 1, 0]], bool)) & ~on
    out = idx.copy()
    out[ring] = 1
    return out


def native(rgb, mask, scale, pal):
    k = len(pal)
    idx = label(rgb, mask, pal)
    h, w = idx.shape
    H, W = max(1, round(h * scale)), max(1, round(w * scale))
    small = majority(idx, H, W, k)
    small = clean(small, pal)
    return small


def body_centre(idx):
    """Column of the torso: the most vertical mass between shoulders and hips."""
    ys, xs = np.nonzero(idx)
    top, bot = ys.min(), ys.max()
    h = bot - top + 1
    band = idx[top + int(h * 0.18): top + int(h * 0.62)] > 0
    col = ndi.uniform_filter1d(band.sum(0).astype(float), 9)
    return int(col.argmax())


def place(idx, centre=None, bottom=ROOT[1], dx=0):
    ys, xs = np.nonzero(idx)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    idx = idx[y0:y1, x0:x1]
    c = (body_centre(idx) if centre is None else centre - x0)
    out = np.zeros((CANVAS, CANVAS), np.uint8)
    top = bottom + 1 - idx.shape[0]
    left = ROOT[0] + dx - c
    left = max(1,min(left,CANVAS-1-idx.shape[1]))
    if top < 1 or left < 1 or left + idx.shape[1] > CANVAS - 1 or top + idx.shape[0] > CANVAS - 1:
        raise ValueError('Native placement would crop opaque pixels; reduce the complete pose set uniformly')
    out[top:top + idx.shape[0], left:left + idx.shape[1]] = idx
    return out, [int(left+c),int(bottom)]


def save_indexed(idx, pal, path):
    im = Image.fromarray(idx.astype(np.uint8), 'P')
    full = [[0, 0, 0]] + [list(map(int, c)) for c in pal]
    full += [[0, 0, 0]] * (256 - len(full))
    im.putpalette(sum(full, []))
    im.save(path, transparency=0)


def preview(frames, pal, path, scale=3, cols=10, bg=(52, 56, 72), names=None):
    full = np.vstack([[0, 0, 0], pal]).astype(np.uint8)
    rows = (len(frames) + cols - 1) // cols
    out = np.zeros((rows * CANVAS, cols * CANVAS, 3), np.uint8)
    out[:] = bg
    for i, f in enumerate(frames):
        y, x = (i // cols) * CANVAS, (i % cols) * CANVAS
        m = f > 0
        out[y:y + CANVAS, x:x + CANVAS][m] = full[f][m]
        out[y + ROOT[1] + 1, x:x + CANVAS] = (90, 96, 120)
    Image.fromarray(out).resize((out.shape[1] * scale, out.shape[0] * scale), Image.NEAREST).save(path)
