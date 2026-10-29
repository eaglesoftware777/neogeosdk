"""The select screen's art: every fighter's face on one octagonal wall.

Rows of 4, 6, 8, 8, 8, 6, 4 faces (all 44, nothing scrolls), each face a
24x24 painting in a 26-pixel cell. The wall is one 208x192 picture packed
like the stages and story cards, its top and bottom halves on sixteen
palettes each (32 in all), one chosen per tile. Beside it, a 64x64 face for
each fighter (two palettes, one a tile) for the corner panels, and the two
players' cursor frames.

Faces are kept in assets/authored/faces/ (cropped from the painted select
portraits, or the front view of a fighter's model sheet, when those are on
this machine); a fighter without one shows its 32x32 roster icon. A wall
cell shows the head alone (HEAD_BOX for the crops that are busts), averaged
down and sharpened: a whole bust at 24 pixels left the face a few pixels
wide.
"""
from pathlib import Path

import numpy as np
from PIL import Image, ImageEnhance, ImageFilter

from palette_banks import palette_words, quantize

GAME = Path(__file__).resolve().parents[1]
PAINTED = Path.home() / '.cache/slf-art/gen'
FACES = GAME / 'assets/authored/faces'
ROWS = (4, 6, 8, 8, 8, 6, 4)
CELL = 26
FACE = 24
WALL_W, WALL_H = 208, 192
BIG = 64
INK = (16, 14, 22)
GROUND = (28, 24, 34)
# Face centres (fractions of the portrait) where hair reads as skin.
FACE_CENTRE = {'amara': (0.56, 0.44)}
# The head in a kept 128x128 face (centre x, centre y, side) where the crop is
# a bust or the skin misleads (a mask, a robot); other heads are found by skin.
HEAD_BOX = {
    'diego': (64, 56, 104), 'maya': (64, 56, 88), 'aditya': (64, 56, 88), 'sokha': (58, 52, 80),
    'lucas': (62, 66, 96), 'noah': (56, 50, 76), 'zara': (60, 55, 84), 'thabo': (52, 40, 70),
    'maia': (72, 44, 76), 'tomas': (50, 46, 92), 'leila': (62, 52, 84), 'kaimana': (58, 48, 86),
    'aleksei': (66, 50, 80), 'hayato': (68, 58, 100), 'mason': (55, 55, 96), 'lotus': (88, 64, 88),
    'tororojo': (70, 60, 104), 'irina': (55, 50, 84), 'kofi': (58, 58, 90), 'xiaomei': (55, 50, 90),
    'zero': (75, 48, 84), 'echo': (70, 46, 76), 'atlas': (70, 46, 92), 'viper': (66, 48, 84),
    'mirage': (62, 52, 86), 'fahri': (60, 70, 112),
}
HEAD_SIDE = 100


def _magenta(rgb):
    f = rgb.astype(int)
    return (f[..., 0] > 150) & (f[..., 2] > 150) & (f[..., 1] < 110)


def _key_fringe(rgb):
    """The magenta key blended into a stroke's edge: red and blue alike, almost
    no green (a violet costume keeps far more green than this)."""
    f = rgb.astype(int)
    r, g, b = f[..., 0], f[..., 1], f[..., 2]
    return (g < 32) & (r > 70) & (b > 70) & (np.abs(r - b) < 70)


def _unfringe(rgb):
    """The face with its key fringe taken by the colours around it."""
    out = rgb.copy()
    pending = _key_fringe(out)
    for _ in range(4):
        if not pending.any():
            break
        for y, x in zip(*np.nonzero(pending)):
            win = (slice(max(0, y - 1), y + 2), slice(max(0, x - 1), x + 2))
            near = out[win][~pending[win]]
            out[y, x] = near.mean(0) if len(near) else GROUND
        pending = _key_fringe(out)
    out[pending] = GROUND
    return out


def head_box(fighter_id, rgb):
    if fighter_id in HEAD_BOX:
        cx, cy, side = HEAD_BOX[fighter_id]
    else:
        hsv = np.asarray(Image.fromarray(rgb).convert('HSV')).astype(int)
        skin = (hsv[..., 0] < 36) & (hsv[..., 1] > 45) & (hsv[..., 1] < 205) & (hsv[..., 2] > 60)
        ys, xs = np.nonzero(skin)
        cx, cy = (int(np.median(xs)), int(np.median(ys))) if len(xs) > 200 else (64, 64)
        side = HEAD_SIDE
    h, w = rgb.shape[:2]
    x0, y0 = max(0, min(w - side, cx - side // 2)), max(0, min(h - side, cy - side // 2))
    return x0, y0, x0 + side, y0 + side


def face(fighter_id, size):
    """size x size RGB head shot on the cell ground."""
    kept = FACES / f'{fighter_id}.png'
    if not kept.exists():
        raise ValueError(f'Missing authored selection portrait: {kept}')
    rgb = _unfringe(np.asarray(Image.open(kept).convert('RGB')))
    img = Image.fromarray(rgb)
    if size >= BIG - 4:
        return np.asarray(img.resize((size, size), Image.Resampling.LANCZOS))
    # a wall cell: the head alone, area-averaged (Lanczos at 1/5 scale went soft)
    img = img.crop(head_box(fighter_id, rgb)).resize((size, size), Image.Resampling.BOX)
    img = ImageEnhance.Contrast(img).enhance(1.1).filter(ImageFilter.UnsharpMask(radius=1, percent=80, threshold=2))
    return np.asarray(img)


def refresh_faces(ids):
    """Crop each fighter's face from its painted portrait (or the front view
    of its model sheet) into assets/authored/faces/, 128x128."""
    FACES.mkdir(parents=True, exist_ok=True)
    for fid in ids:
        crop = painted_face(fid)
        if crop is not None:
            Image.fromarray(crop).save(FACES / f'{fid}.png')


def painted_face(fighter_id):
    painted = PAINTED / f'{fighter_id}_portrait.png'
    design = PAINTED / f'{fighter_id}_design.png'
    if painted.exists() or design.exists():
        rgb = np.asarray(Image.open(painted if painted.exists() else design).convert('RGB'))
        if not painted.exists():
            # the model sheet: the front view, its head in the top third
            rgb = rgb[:rgb.shape[0] // 3, :rgb.shape[1] // 3]
        body = ~_magenta(rgb)
        if not painted.exists():
            body &= np.abs(rgb.astype(int) - rgb[2, 2].astype(int)).sum(-1) >= 36
        h, w = body.shape
        side = int(w * (0.42 if painted.exists() else 0.5))
        # The face: the first rows with a good run of skin (any skin tone);
        # hair can stand far above it.
        hsv = np.asarray(Image.fromarray(rgb).convert('HSV')).astype(int)
        skin = body & (hsv[..., 0] < 36) & (hsv[..., 1] > 45) & (hsv[..., 1] < 205) & (hsv[..., 2] > 60)
        rows = np.nonzero(skin.sum(1) > w * 0.05)[0]
        if fighter_id in FACE_CENTRE:
            cx, cy = int(w * FACE_CENTRE[fighter_id][0]), int(h * FACE_CENTRE[fighter_id][1])
        elif len(rows):
            brow = rows[0]
            band = skin[brow:brow + side // 2]
            cx = int(np.median(np.nonzero(band)[1]))
            cy = brow + int(w * 0.13)
        else:
            ys, xs = np.nonzero(body)
            cx, cy = int(np.median(xs)), ys.min() + side // 2
        x0 = max(0, min(w - side, cx - side // 2))
        y0 = max(0, min(h - side, cy - side // 2))
        crop = rgb[y0:y0 + side, x0:x0 + side].copy()
        crop[_magenta(crop)] = GROUND
        if not painted.exists():
            # the model sheet's plain backdrop: whatever matches its corner
            corner = rgb[2, 2].astype(int)
            crop[np.abs(crop.astype(int) - corner).sum(-1) < 36] = GROUND
        return np.asarray(Image.fromarray(crop).resize((128, 128), Image.Resampling.NEAREST))
    return None


def cells():
    """(row, column offset in cells) of every fighter, in roster order."""
    out = []
    for r, n in enumerate(ROWS):
        off = (max(ROWS) - n) // 2
        out += [(r, off + k) for k in range(n)]
    return out


def locked(rgb):
    """A face whose fighter is not playable yet: grey and dark."""
    grey = rgb.astype(np.float32).mean(axis=-1, keepdims=True) * 0.3
    return np.repeat(grey, 3, axis=-1).astype(np.uint8)


def wall_image(ids, ready=None):
    if len(ids) != sum(ROWS):
        raise ValueError(f'The select wall holds {sum(ROWS)} fighters, roster has {len(ids)}')
    rgba = np.zeros((WALL_H, WALL_W, 4), np.uint8)
    for i, (fid, (r, c)) in enumerate(zip(ids, cells())):
        x, y = c * CELL, r * CELL
        rgba[y:y + CELL, x:x + CELL, :3] = INK
        rgba[y:y + CELL, x:x + CELL, 3] = 255
        cell = face(fid, FACE)
        rgba[y + 1:y + 1 + FACE, x + 1:x + 1 + FACE, :3] = cell if ready is None or ready[i] else locked(cell)
    return rgba


def big_face(fid):
    rgba = np.zeros((BIG, BIG, 4), np.uint8)
    rgba[..., :3] = INK
    rgba[..., 3] = 255
    rgba[2:BIG - 2, 2:BIG - 2, :3] = face(fid, BIG - 4)
    return rgba


def cursor_frames():
    """Two 32x32 frames for the 26-pixel cells: 1P outside, 2P one ring in.
    Colours: 14 the bright edge, 15 the player's colour."""
    out = []
    for inset, tag in ((0, '1P'), (2, '2P')):
        img = np.zeros((32, 32), np.uint8)
        a, b = 1 + inset, 30 - inset
        img[a:a + 2, a:b + 1] = 15; img[b - 1:b + 1, a:b + 1] = 15
        img[a:b + 1, a:a + 2] = 15; img[a:b + 1, b - 1:b + 1] = 15
        img[a, a:b + 1] = 14; img[a:b + 1, a] = 14
        # the player's tag on the top edge
        glyphs = {'1': ['010', '110', '010', '010', '111'], '2': ['110', '001', '010', '100', '111'],
                  'P': ['110', '101', '110', '100', '100']}
        tx, ty = (4, 0) if inset == 0 else (21, 27)
        img[ty:ty + 5, tx - 1:tx + 8] = 15
        for i, ch in enumerate(tag):
            for yy, row in enumerate(glyphs[ch]):
                for xx, bit in enumerate(row):
                    if bit == '1':
                        img[ty + yy, tx + i * 4 + xx] = 14
        out.append(img)
    return out


def build(ids, place, emit, pal_base, ready=None):
    """C tables for the wall, the big faces and the cursors. place(indexed)
    stores an image in the C ROM and returns its first tile."""
    header = []
    # Two halves of sixteen palettes each, 32-63 (the stage's 32-47 and the
    # unused 48-63; no stage is up here): sixteen for all 44 faces mixed one
    # face's skin and hair into the next.
    parts = [quantize(half, bank_limit=16, refine=3) for half in np.split(wall_image(ids, ready), 2)]
    indices = np.vstack([p[0] for p in parts])
    banks = np.concatenate([p[1] for p in parts])
    assign = np.vstack([parts[0][2], parts[1][2] + len(parts[0][1])])
    tile = place(indices)
    header.append(f'#define SLF_WALL_TILE {tile}u\n#define SLF_WALL_STRIPS {WALL_W // 16}u\n'
                  f'#define SLF_WALL_ROWS {WALL_H // 16}u\n#define SLF_WALL_PAL {pal_base}u\n'
                  f'#define SLF_WALL_BANKS {len(banks)}u\n#define SLF_WALL_CELL {CELL}u')
    header.append(emit('slf_wall_map', assign.flatten() + pal_base, 'uint8_t'))
    header.append('static const uint16_t slf_wall_pals[][16] = {' + ','.join(
        '{' + ','.join(str(v) for v in palette_words(b)) + '}' for b in banks) + '};')
    rows = ROWS
    starts = np.cumsum((0,) + rows[:-1])
    header.append(emit('slf_wall_row_len', rows, 'uint8_t'))
    header.append(emit('slf_wall_row_start', starts, 'uint8_t'))
    header.append(emit('slf_wall_row_off', [(max(rows) - n) // 2 for n in rows], 'uint8_t'))
    header.append(f'#define SLF_WALL_ROW_COUNT {len(rows)}u')
    faces, pals, maps = [], [], []
    for fid in ids:
        ind, fb, fa = quantize(big_face(fid), bank_limit=2)
        if len(fb) < 2:
            fb = np.concatenate([fb, fb])
        faces.append(ind)
        pals.append([palette_words(b) for b in fb])
        maps.append(fa.flatten())
    first = place(np.vstack(faces))
    header.append(f'#define SLF_FACE_TILE {first}u\n#define SLF_FACE_TILES {(BIG // 16) ** 2}u')
    header.append('static const uint16_t slf_face_pals[][2][16] = {' + ','.join(
        '{' + ','.join('{' + ','.join(str(v) for v in p) + '}' for p in pair) + '}' for pair in pals) + '};')
    header.append(emit('slf_face_maps', np.concatenate(maps), 'uint8_t'))
    cur = place(np.vstack(cursor_frames()))
    header.append(f'#define SLF_WALL_CURSOR_TILE {cur}u')
    preview = np.zeros((WALL_H, WALL_W, 3), np.uint8)
    for ty in range(WALL_H // 16):
        for tx in range(WALL_W // 16):
            b = banks[assign[ty, tx]]
            preview[ty * 16:ty * 16 + 16, tx * 16:tx * 16 + 16] = b[indices[ty * 16:ty * 16 + 16, tx * 16:tx * 16 + 16]]
    return header, preview
