"""Sky Lance's HUD lettering: one squared 5x7 face for the text page, the
counter digits, the SCORE / HI / 1P-3P labels, the energy cells and the life
pip.

Text glyphs sit in their 8x8 cell at x 1-5, y 0-6 with a one-pixel drop
shadow; the upper four rows take ink entry 1, the lower three entry 3 (a
darker shade), the shadow entry 2. user.c sets those entries per ink. The
infix images are drawn in full colour and fitted by art.py like before.
"""
from pathlib import Path

import numpy as np
from PIL import Image

INFIX = Path(__file__).resolve().parents[1] / 'artbox/infix'
SOURCES = Path(__file__).resolve().parents[1] / 'assets/painted/sources'

GLYPHS = {
    'A': ['#####', '#...#', '#...#', '#####', '#...#', '#...#', '#...#'],
    'B': ['####.', '#...#', '#...#', '#####', '#...#', '#...#', '#####'],
    'C': ['#####', '#....', '#....', '#....', '#....', '#....', '#####'],
    'D': ['####.', '#...#', '#...#', '#...#', '#...#', '#...#', '####.'],
    'E': ['#####', '#....', '#....', '####.', '#....', '#....', '#####'],
    'F': ['#####', '#....', '#....', '####.', '#....', '#....', '#....'],
    'G': ['#####', '#....', '#....', '#.###', '#...#', '#...#', '#####'],
    'H': ['#...#', '#...#', '#...#', '#####', '#...#', '#...#', '#...#'],
    'I': ['#####', '..#..', '..#..', '..#..', '..#..', '..#..', '#####'],
    'J': ['....#', '....#', '....#', '....#', '#...#', '#...#', '#####'],
    'K': ['#...#', '#..#.', '#.#..', '###..', '#..#.', '#...#', '#...#'],
    'L': ['#....', '#....', '#....', '#....', '#....', '#....', '#####'],
    'M': ['#...#', '##.##', '#.#.#', '#.#.#', '#...#', '#...#', '#...#'],
    'N': ['#...#', '##..#', '#.#.#', '#..##', '#...#', '#...#', '#...#'],
    'O': ['#####', '#...#', '#...#', '#...#', '#...#', '#...#', '#####'],
    'P': ['#####', '#...#', '#...#', '#####', '#....', '#....', '#....'],
    'Q': ['#####', '#...#', '#...#', '#...#', '#.#.#', '#..#.', '##.##'],
    'R': ['#####', '#...#', '#...#', '#####', '#.#..', '#..#.', '#...#'],
    'S': ['#####', '#....', '#....', '#####', '....#', '....#', '#####'],
    'T': ['#####', '..#..', '..#..', '..#..', '..#..', '..#..', '..#..'],
    'U': ['#...#', '#...#', '#...#', '#...#', '#...#', '#...#', '#####'],
    'V': ['#...#', '#...#', '#...#', '#...#', '#...#', '.#.#.', '..#..'],
    'W': ['#...#', '#...#', '#...#', '#.#.#', '#.#.#', '##.##', '#...#'],
    'X': ['#...#', '#...#', '.#.#.', '..#..', '.#.#.', '#...#', '#...#'],
    'Y': ['#...#', '#...#', '#...#', '#####', '..#..', '..#..', '..#..'],
    'Z': ['#####', '....#', '...#.', '..#..', '.#...', '#....', '#####'],
    '0': ['#####', '#...#', '#..##', '#.#.#', '##..#', '#...#', '#####'],
    '1': ['..#..', '.##..', '..#..', '..#..', '..#..', '..#..', '#####'],
    '2': ['#####', '....#', '....#', '#####', '#....', '#....', '#####'],
    '3': ['#####', '....#', '....#', '.####', '....#', '....#', '#####'],
    '4': ['#...#', '#...#', '#...#', '#####', '....#', '....#', '....#'],
    '5': ['#####', '#....', '#....', '####.', '....#', '....#', '####.'],
    '6': ['#####', '#....', '#....', '#####', '#...#', '#...#', '#####'],
    '7': ['#####', '....#', '....#', '...#.', '..#..', '..#..', '..#..'],
    '8': ['#####', '#...#', '#...#', '#####', '#...#', '#...#', '#####'],
    '9': ['#####', '#...#', '#...#', '#####', '....#', '....#', '#####'],
    ':': ['.....', '..#..', '..#..', '.....', '..#..', '..#..', '.....'],
    '/': ['....#', '....#', '...#.', '..#..', '.#...', '#....', '#....'],
    '-': ['.....', '.....', '.....', '#####', '.....', '.....', '.....'],
    '+': ['.....', '..#..', '..#..', '#####', '..#..', '..#..', '.....'],
    '=': ['.....', '.....', '#####', '.....', '#####', '.....', '.....'],
    '%': ['##..#', '##.#.', '...#.', '..#..', '.#...', '.#.##', '#..##'],
    '.': ['.....', '.....', '.....', '.....', '.....', '.....', '..#..'],
    ',': ['.....', '.....', '.....', '.....', '.....', '..#..', '.#...'],
    '!': ['..#..', '..#..', '..#..', '..#..', '..#..', '.....', '..#..'],
    '?': ['#####', '....#', '....#', '..###', '..#..', '.....', '..#..'],
    "'": ['..#..', '..#..', '.....', '.....', '.....', '.....', '.....'],
    '"': ['.#.#.', '.#.#.', '.....', '.....', '.....', '.....', '.....'],
    '(': ['...#.', '..#..', '..#..', '..#..', '..#..', '..#..', '...#.'],
    ')': ['.#...', '..#..', '..#..', '..#..', '..#..', '..#..', '.#...'],
    '[': ['..##.', '..#..', '..#..', '..#..', '..#..', '..#..', '..##.'],
    ']': ['.##..', '..#..', '..#..', '..#..', '..#..', '..#..', '.##..'],
    '<': ['....#', '...#.', '..#..', '.#...', '..#..', '...#.', '....#'],
    '>': ['#....', '.#...', '..#..', '...#.', '..#..', '.#...', '#....'],
    '&': ['.##..', '#..#.', '.##..', '.#..#', '#.##.', '#..#.', '.##.#'],
    '#': ['.#.#.', '#####', '.#.#.', '.#.#.', '.#.#.', '#####', '.#.#.'],
    '*': ['.....', '#.#.#', '.###.', '#####', '.###.', '#.#.#', '.....'],
    '_': ['.....', '.....', '.....', '.....', '.....', '.....', '#####'],
}


def glyph(char):
    rows = GLYPHS.get(char.upper())
    if rows is None:
        rows = GLYPHS['?']
    return np.array([[c == '#' for c in r] for r in rows], bool)


def text_tile(char):
    """8x8 indices for the text page: 1 upper ink, 3 lower ink, 2 shadow."""
    tile = np.zeros((8, 8), np.uint8)
    if char == ' ':
        return tile
    g = glyph(char)
    on = np.zeros((8, 8), bool)
    on[0:7, 1:6] = g
    shadow = np.zeros((8, 8), bool)
    shadow[1:8, 2:8] = on[0:7, 0:6]
    tile[shadow & ~on] = 2
    tile[on] = 3
    tile[0:4][on[0:4]] = 1
    return tile


def font_tiles():
    return {code: text_tile(chr(code)) for code in range(32, 127)}


def ramp(rows, top, bottom):
    t = np.linspace(0, 1, rows)[:, None]
    return (np.array(top) * (1 - t) + np.array(bottom) * t).astype(np.uint8)


def shaded(mask, top, bottom, outline=(16, 22, 38)):
    """RGBA: the mask filled with a vertical ramp, a one-pixel dark outline round it."""
    h, w = mask.shape
    pad = np.pad(mask, 1)
    ring = np.zeros_like(mask)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            ring |= pad[1 + dy:1 + dy + h, 1 + dx:1 + dx + w]
    ring &= ~mask
    out = np.zeros((h, w, 4), np.uint8)
    colors = ramp(h, top, bottom)
    out[mask, :3] = np.broadcast_to(colors[:, None, :], (h, w, 3))[mask]
    out[mask, 3] = 255
    out[ring, :3] = outline
    out[ring, 3] = 255
    return out


def label(text, top, bottom):
    """A label in the text face, 8 pixels tall, one cell per character."""
    mask = np.zeros((8, 8 * len(text)), bool)
    for i, ch in enumerate(text):
        if ch != ' ':
            mask[0:7, i * 8 + 1:i * 8 + 6] = glyph(ch)
    h, w = mask.shape
    out = np.zeros((h, w, 4), np.uint8)
    colors = ramp(7, top, bottom)
    for y in range(7):
        out[y][mask[y], :3] = colors[y]
        out[y][mask[y], 3] = 255
    shadow = np.zeros_like(mask)
    shadow[1:, 1:] = mask[:-1, :-1]
    shadow &= ~mask
    out[shadow, :3] = (16, 22, 38)
    out[shadow, 3] = 255
    return out


def digit(d):
    """Counter digit, 16x16: the face doubled to 10x14, gold ramp, dark outline."""
    mask = np.zeros((16, 16), bool)
    mask[1:15, 3:13] = np.kron(glyph(str(d)), np.ones((2, 2), bool))
    return shaded(mask, (255, 250, 214), (238, 150, 32))


def energy(full):
    mask = np.zeros((16, 16), bool)
    mask[4:12, 1:15] = True
    mask[4, 1] = mask[4, 14] = mask[11, 1] = mask[11, 14] = False
    if full:
        out = shaded(mask, (196, 255, 255), (24, 150, 214))
        out[5, 3:13, :3] = (240, 255, 255)
        return out
    out = shaded(mask, (44, 58, 78), (26, 34, 50), outline=(92, 112, 136))
    return out


def life():
    """The spare-plane pip: the F-16 painting at 14 pixels, outlined."""
    with Image.open(SOURCES / 'f16_top.png') as im:
        im = im.convert('RGBA')
        f = 13 / im.height
        small = np.asarray(im.resize((max(1, round(im.width * f)), 13), Image.Resampling.BOX)).copy()
    small[:, :, 3] = np.where(small[:, :, 3] >= 110, 255, 0)
    out = np.zeros((16, 16, 4), np.uint8)
    h, w = small.shape[:2]
    y0, x0 = (16 - h) // 2, (16 - w) // 2
    out[y0:y0 + h, x0:x0 + w] = small
    a = out[:, :, 3] > 0
    pad = np.pad(a, 1)
    ring = np.zeros_like(a)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            ring |= pad[1 + dy:1 + dy + 16, 1 + dx:1 + dx + 16]
    ring &= ~a
    out[ring] = (16, 22, 38, 255)
    return out


def build():
    images = {'SCORE': label('SCORE', (170, 236, 255), (60, 150, 220)),
              'HI': label('HI', (170, 236, 255), (60, 150, 220)),
              '1P': label('1P', (150, 210, 255), (40, 110, 230)),
              '2P': label('2P', (170, 255, 170), (40, 170, 70)),
              '3P': label('3P', (255, 200, 150), (230, 80, 50)),
              'energy': energy(True), 'energy_empty': energy(False), 'life': life()}
    for d in range(10):
        images[f'digit_{d}'] = digit(d)
    for name, rgba in images.items():
        Image.fromarray(rgba, 'RGBA').save(INFIX / f'fix_{name}.png')
    return images


if __name__ == '__main__':
    build()
    print('HUD lettering written')
