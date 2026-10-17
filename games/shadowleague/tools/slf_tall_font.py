"""Shadow League's tall FIX face: 8x16 characters (two FIX tiles each) for
headings, names and menus.

The bold glyphs of the 8x8 face (slf_font) doubled in height: a 6x14 body
with two-pixel strokes both ways, shaded from its lit top to its foot in
colours 2-7, inside a one-pixel outline in colour 1, so the text reads over
any painting. Tiles: the top half of character c at TOP + c, the bottom
half at BOTTOM + c (c < 0x80; lower case uses the capitals).

    glyph_tiles() -> {tile number: 8x8 uint8 array}
"""
import numpy as np
from scipy import ndimage as ndi

import slf_font

TOP, BOTTOM = 0x800, 0x880

# 0 clear, 1 outline, 2-7 the body from its lit top to its foot.
PALETTES = {
    'GOLD': [(0, 0, 0), (12, 6, 4), (255, 248, 208), (255, 226, 120), (250, 190, 64), (232, 146, 36), (196, 100, 24), (140, 60, 16)],
    'SILVER': [(0, 0, 0), (8, 8, 16), (255, 255, 255), (232, 236, 248), (200, 208, 228), (164, 174, 200), (128, 140, 172), (92, 104, 140)],
    'RED': [(0, 0, 0), (16, 4, 4), (255, 214, 190), (255, 140, 110), (240, 72, 56), (200, 32, 40), (150, 16, 32), (96, 8, 24)],
    'BLUE': [(0, 0, 0), (4, 8, 20), (224, 244, 255), (150, 210, 255), (80, 160, 240), (40, 112, 216), (24, 72, 168), (16, 40, 112)],
}


def glyph_tiles():
    tiles = {}
    for ch, rows in slf_font.G.items():
        lines = rows.split('|')
        width = max(len(r) for r in lines)
        body = np.zeros((16, 8), bool)
        x0 = 1 if width <= 6 else 0           # (M and W lean on the next cell's outline)
        for y, line in enumerate(lines):
            for x, c in enumerate(line):
                if c == '#':
                    body[1 + y * 2:3 + y * 2, x0 + x] = True
        ring = ndi.binary_dilation(body, np.ones((3, 3), bool)) & ~body
        g = np.zeros((16, 8), np.uint8)
        shade = (2 + np.clip((np.arange(16) - 1) * 6 // 14, 0, 5)).astype(np.uint8)
        g[body] = shade[np.nonzero(body)[0]]
        g[ring] = 1
        for c in [ord(ch)] + ([ord(ch.lower())] if ch.isalpha() else []):
            tiles[TOP + c] = g[:8]
            tiles[BOTTOM + c] = g[8:]
    return tiles
