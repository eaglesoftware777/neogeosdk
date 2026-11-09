"""Century XI's match HUD on the FIX layer, the way a 1994 broadcast drew it.

A navy box with a gold rim: each side's flag and three-letter code, the score in tall
digits, a running match clock with the added time beside it, the mood bar under the box
and a short status line. Every piece is drawn here as a small picture whole tiles in size,
cut into 8 x 8 FIX tiles (equal tiles are kept once) and described in cx_hud.h: the tile
numbers, and the three FIX palettes the pieces are drawn with. Nothing here costs a
hardware sprite.
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

import flags

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "artbox"))
from ng_artpack import text_pixels  # noqa: E402
from palette_banks import palette_words  # noqa: E402

FIRST_TILE = 0x300           # FIX pages 3..C are free: the system font sits in 0-2, ASCII at 0xD00
PAL_BOX, PAL_FLAG, PAL_WARN = 5, 6, 7

# FIX palette 5, the box; 12-15 are the two sides' shirt colours, written at kick-off
(NAVY, NAVY_HI, GOLD, INK, WHITE, GREY, CYAN, RED, GREEN, YELLOW, DARK, T0, T1, T0S, T1S) = range(1, 16)
BOX_RGB = [(0, 0, 0), (16, 26, 76), (54, 80, 160), (244, 198, 56), (6, 8, 22), (252, 252, 255), (170, 182, 212),
           (84, 222, 250), (238, 48, 56), (56, 208, 100), (252, 226, 70), (8, 12, 38),
           (200, 30, 40), (30, 60, 200), (120, 16, 24), (16, 34, 120)]
# the warning palette: the same box with the clock's ink turned red (the game flashes it)
WARN_RGB = list(BOX_RGB)
WARN_RGB[WHITE] = (255, 96, 88)
WARN_RGB[GREY] = (200, 40, 40)
# flags: the box's own colours first, so the panel round a flag matches the rest
FLAG_COLOURS = [(248, 248, 248), (206, 17, 38), (128, 20, 52), (0, 56, 147), (110, 170, 224),
                (0, 132, 64), (252, 209, 22), (250, 136, 20), (18, 18, 22), (0, 86, 44)]
FLAG_RGB = [(0, 0, 0), BOX_RGB[NAVY], BOX_RGB[NAVY_HI], BOX_RGB[GOLD], BOX_RGB[INK], BOX_RGB[DARK]] + FLAG_COLOURS
F_NAVY, F_NAVY_HI, F_GOLD, F_INK, F_DARK = 1, 2, 3, 4, 5

CHARS = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!+-:.'"


class Tiles:
    """FIX tiles from FIRST_TILE up; a tile already made is used again."""

    def __init__(self, first):
        self.first = first
        self.index = {}
        self.glyphs = []

    def add(self, t):
        key = np.ascontiguousarray(t, np.uint8).tobytes()
        if key not in self.index:
            self.index[key] = self.first + len(self.glyphs)
            self.glyphs.append(np.array(t, np.uint8))
        return self.index[key]

    def block(self, img):
        """Tile numbers of a picture, row by row (top row first)."""
        return [self.add(img[y:y + 8, x:x + 8]) for y in range(0, img.shape[0], 8) for x in range(0, img.shape[1], 8)]


def panel(w, h=16, c=(NAVY, NAVY_HI, GOLD, DARK)):
    """The box: navy, a lit edge under the gold rim on top, a shadow over it below."""
    navy, hi, gold, dark = c
    a = np.full((h, w), navy, np.uint8)
    a[0], a[1], a[h - 2], a[h - 1] = gold, hi, dark, gold
    return a


def put_text(a, x, y, text, ink, shade=None, outline=INK):
    """The font's letters, with a dark shadow down and right; `shade` colours their lower rows."""
    g = text_pixels(text, 1)
    for yy in range(g.shape[0]):
        for xx in range(g.shape[1]):
            if g[yy, xx] and 0 <= y + yy + 1 < a.shape[0] and 0 <= x + xx + 1 < a.shape[1] and a[y + yy + 1, x + xx + 1] != ink:
                a[y + yy + 1, x + xx + 1] = outline
    for yy in range(g.shape[0]):
        for xx in range(g.shape[1]):
            if g[yy, xx] and 0 <= y + yy < a.shape[0] and 0 <= x + xx < a.shape[1]:
                a[y + yy, x + xx] = shade if (shade and yy >= 4) else ink


def tall_digit(ch):
    """A digit two tiles tall: the font's letter stretched to twelve rows and made bold,
    white over grey."""
    a = panel(8)
    g = text_pixels(ch, 1)[:7]
    rows = [int(r * 7 / 12) for r in range(12)]
    big = g[rows].copy()
    big[:, 1:] |= g[rows][:, :-1]          # bold: strokes two pixels wide
    for yy in range(12):
        for xx in range(8):
            if big[yy, xx] and xx + 1 < 8 and yy + 3 < 16 and not big[min(yy + 1, 11), min(xx + 1, 7)]:
                a[2 + yy + 1, xx + 1] = INK
    for yy in range(12):
        for xx in range(8):
            if big[yy, xx]:
                a[2 + yy, xx] = WHITE if yy < 6 else GREY
    return a


def caps():
    """Round ends for the box: the rim follows the curve."""
    start = [3, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3]
    left = panel(8)
    for y, s in enumerate(start):
        left[y, :s] = 0
        left[y, s] = GOLD
    return left, left[:, ::-1].copy()


def flag_block(i):
    """Nation i's flag in the box: 18 x 10 inside a dark frame, on navy, three tiles by two."""
    im = Image.new('RGB', (flags.W, flags.H))
    flags.FLAGS[i](ImageDraw.Draw(im))
    small = np.asarray(im.resize((18, 10), Image.BOX)).astype(np.int32)
    a = panel(24, c=(F_NAVY, F_NAVY_HI, F_GOLD, F_DARK))
    a[2:14, 2:22] = F_INK
    cols = np.array(FLAG_COLOURS, np.int32)
    for y in range(10):
        for x in range(18):
            d = ((cols - small[y, x]) ** 2).sum(1)
            a[3 + y, 3 + x] = 6 + int(np.argmin(d))
    return a


def code_block(code):
    a = panel(24)
    put_text(a, 0, 4, code, WHITE)
    return a


def plus_block(n):
    a = panel(16)
    put_text(a, 1, 4, f"+{n}", YELLOW)
    return a


def bar_tiles():
    """The mood bar: a pill in the two sides' colours, split where the balance lies."""
    def base():
        a = np.zeros((8, 8), np.uint8)
        a[1] = INK
        a[6] = INK
        return a

    def fill(a, x0, x1, c, s):
        a[2:5, x0:x1] = c
        a[5, x0:x1] = s
    out = {}
    a = base(); fill(a, 0, 8, T0, T0S); out['t0'] = a
    a = base(); fill(a, 0, 8, T1, T1S); out['t1'] = a
    for k in (2, 4, 6):
        a = base(); fill(a, 0, k, T0, T0S); fill(a, k, 8, T1, T1S); a[2:6, k] = WHITE; out[f's{k}'] = a
    a = base(); fill(a, 2, 8, T0, T0S); a[1, :2] = 0; a[6, :2] = 0; a[2:6, 1] = INK; a[1, 2] = a[6, 2] = 0; out['capl'] = a
    a = base(); fill(a, 0, 6, T1, T1S); a[1, 6:] = 0; a[6, 6:] = 0; a[2:6, 6] = INK; a[1, 5] = a[6, 5] = 0; out['capr'] = a
    return out


def status_font():
    """One-row letters on navy for the status line, and its round ends."""
    out = {}
    for ch in CHARS:
        a = np.full((8, 8), NAVY, np.uint8)
        put_text(a, 0, 0, ch, WHITE)
        out[ch] = a
    left = np.full((8, 8), NAVY, np.uint8)
    for y, s in enumerate([2, 1, 0, 0, 0, 0, 1, 2]):
        left[y, :s] = 0
    out['<'] = left
    out['>'] = left[:, ::-1].copy()
    return out


def icons():
    """Little pictures for the status line: the physio's cross, the cards, the coach's arrows."""
    out = {}
    a = np.full((8, 8), NAVY, np.uint8); a[1:7, 3:5] = RED; a[3:5, 1:7] = RED; out['cross'] = a
    a = np.full((8, 8), NAVY, np.uint8); a[1:7, 2:6] = RED; a[1, 2:6] = WHITE; out['red'] = a
    a = np.full((8, 8), NAVY, np.uint8); a[1:7, 2:6] = YELLOW; a[1, 2:6] = WHITE; out['yellow'] = a
    a = np.full((8, 8), NAVY, np.uint8)
    for y in range(4):
        a[1 + y, 3 - y:5 + y] = GREEN
    a[5:7, 3:5] = GREEN; out['up'] = a
    out['down'] = np.full((8, 8), NAVY, np.uint8); out['down'][:] = out['up'][::-1]
    out['down'][out['down'] == GREEN] = CYAN
    return out


def build(pack, nation_codes):
    """Adds the HUD's FIX tiles to the pack and answers cx_hud.h's text."""
    t = Tiles(FIRST_TILE)
    blank = t.block(panel(8))
    capl, capr = caps()
    cap = [t.block(capl), t.block(capr)]
    digits = [t.block(tall_digit(c)) for c in "0123456789"]
    colon = panel(8); colon[5:7, 3:5] = WHITE; colon[10:12, 3:5] = GREY; colon[6:8, 5] = INK; colon[11:13, 5] = INK
    dash = panel(8); dash[7:9, 1:7] = WHITE; dash[9, 2:7] = INK
    digits += [t.block(colon), t.block(dash)]
    plus = [t.block(plus_block(n)) for n in range(10)]
    flag = [t.block(flag_block(i)) for i in range(len(flags.FLAGS))]
    code = [t.block(code_block(c)) for c in nation_codes]
    bar = bar_tiles()
    bar_order = ['capl', 'capr', 't0', 't1', 's2', 's4', 's6']
    bars = [t.add(bar[k]) for k in bar_order]
    font = status_font()
    char = [t.add(font[ch]) for ch in CHARS]
    pill = [t.add(font['<']), t.add(font['>'])]
    ic = icons()
    icon_order = ['cross', 'red', 'yellow', 'up', 'down']
    icon = [t.add(ic[k]) for k in icon_order]
    if t.first + len(t.glyphs) > 0xD00:
        raise ValueError("HUD tiles run into the ASCII page")
    for i, g in enumerate(t.glyphs):
        pack.font_glyphs(t.first + i, g)

    def arr(name, rows, kind="uint16_t"):
        if rows and isinstance(rows[0], list):
            inner = ",".join("{" + ",".join(f"0x{v:03X}u" for v in r) + "}" for r in rows)
            return f"static const {kind} {name}[{len(rows)}][{len(rows[0])}] = {{{inner}}};"
        return f"static const {kind} {name}[{len(rows)}] = {{" + ",".join(f"0x{v:03X}u" for v in rows) + "};"

    pals = [palette_words(BOX_RGB), palette_words(FLAG_RGB), palette_words(WARN_RGB)]
    q = ["/* Generated by games/centuryxi/tools/hud.py: do not edit. */", "#ifndef CX_HUD_H", "#define CX_HUD_H",
         "#include <stdint.h>",
         f"#define CX_HUD_PAL_BOX {PAL_BOX}u", f"#define CX_HUD_PAL_FLAG {PAL_FLAG}u", f"#define CX_HUD_PAL_WARN {PAL_WARN}u",
         f"#define CX_HUD_TILES {len(t.glyphs)}",
         f"#define CX_HUD_T0 {T0}u", f"#define CX_HUD_T1 {T1}u", f"#define CX_HUD_T0S {T0S}u", f"#define CX_HUD_T1S {T1S}u",
         f"#define CX_HUD_INK {WHITE}u", f"#define CX_HUD_INK2 {GREY}u",
         "/* blocks are their tiles row by row: a 3 x 2 block is top three, then bottom three */",
         arr("cx_hud_pal", pals),
         arr("cx_hud_blank", blank), arr("cx_hud_cap", cap),
         "/* tall digits 0-9, then the colon (10) and the dash (11): top tile, bottom tile */",
         arr("cx_hud_digit", digits), arr("cx_hud_plus", plus),
         arr("cx_hud_flag", flag), arr("cx_hud_code", code),
         "/* the mood bar: left end, right end, all left side, all right side, split at 2, 4, 6 pixels */",
         arr("cx_hud_bar", bars),
         f'static const char cx_hud_chars[] = "{CHARS}";',
         arr("cx_hud_char", char), arr("cx_hud_pill", pill),
         "/* status icons: the physio's cross, red card, yellow card, attack, defend */",
         arr("cx_hud_icon", icon),
         "#endif", ""]
    return "\n".join(q), t
