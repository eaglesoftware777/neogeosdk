#!/usr/bin/env python3
"""
Mahjong Demo art: tile faces, the felt table, the logo and the FIX mini
tiles, drawn here at native size and packed with tools/ng_artpack.py into
C1/C2 and S1 (the build's GAME_ART_BUILDER). All of it is plain test art
for the SDK's Mahjong engine demonstration.

    python3 games/mahjong_demo/tools/art.py            (everything)
    python3 games/mahjong_demo/tools/art.py --fix-only (S1 only)

Tiles: one 16 x 32 frame per kind (ng_mahjong.h's numbering: 0-8
characters, 9-17 dots, 18-26 bamboo, 27-30 winds, 31-33 dragons, 34-37
flowers, 38-41 seasons), then frame 42 the back. A face is 14 x 24 with a
2-pixel side on the right and a 3-pixel side below, so tiles laid 16 apart
touch like real ones, and a stacked tile shows its sides.

Mini tiles: two FIX cells per tile (rank or letter, then the suit's mark)
for discards and other seats' melds, drawn with FIX palette MINI_PAL.
"""
import argparse
from pathlib import Path
import sys

import numpy as np
from PIL import Image, ImageDraw

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from ng_artpack import ArtPack, text_pixels  # noqa: E402

# the tiles' colours (15 at most: the tile sheet is one palette)
OUTLINE = (52, 40, 28)
FACE = (242, 234, 206)
SHADE = (214, 200, 164)
SIDE = (196, 176, 120)
SIDE_DARK = (140, 118, 70)
INK = (24, 22, 20)
RED = (204, 34, 34)
BLUE = (34, 76, 196)
GREEN = (22, 138, 64)
PINK = (226, 92, 160)
ORANGE = (236, 140, 32)
BACK = (28, 108, 72)
BACK_LIGHT = (70, 168, 108)
SUIT_INK = (RED, BLUE, GREEN)

FIX_MINI = 0x0E00          # first FIX tile of the mini tiles
MINI_PAL = 8               # FIX palette they are drawn with
MINI_COLOURS = [(0, 0, 0), FACE, INK, RED, BLUE, GREEN, PINK, ORANGE, SIDE_DARK, BACK, BACK_LIGHT]


def glyph(text, colour, scale=1):
    """The firmware font's text as an RGBA picture in one colour."""
    g = text_pixels(text, scale=scale)
    out = np.zeros(g.shape + (4,), np.uint8)
    out[g > 0] = colour + (255,)
    return out


def paste(dst, src, x, y):
    """src over dst at (x, y), clipped to dst; transparent pixels left alone."""
    h, w = src.shape[:2]
    x0, y0 = max(0, -x), max(0, -y)
    x1, y1 = min(w, dst.shape[1] - x), min(h, dst.shape[0] - y)
    s = src[y0:y1, x0:x1]
    a = s[..., 3] > 0
    dst[y + y0:y + y1, x + x0:x + x1][a] = s[a]


def blank_tile():
    t = np.zeros((32, 16, 4), np.uint8)
    t[0:27, 0:16] = OUTLINE + (255,)
    t[1:26, 13:15] = SIDE + (255,)          # right side
    t[23:26, 1:15] = SIDE + (255,)          # bottom side
    t[24:26, 1:15] = SIDE_DARK + (255,)
    t[1:23, 1:13] = FACE + (255,)
    t[21:23, 1:13] = SHADE + (255,)
    t[1:23, 12:13] = SHADE + (255,)
    return t


def dots_layout(n):
    """Centres of n circles in the 12 x 13 area under the rank."""
    cols = {1: [(6, 6)], 2: [(6, 2), (6, 10)], 3: [(2, 2), (6, 6), (10, 10)],
            4: [(3, 3), (9, 3), (3, 10), (9, 10)], 5: [(3, 2), (9, 2), (6, 6), (3, 10), (9, 10)],
            6: [(3, 2), (9, 2), (3, 6), (9, 6), (3, 10), (9, 10)]}
    if n in cols:
        return cols[n]
    pts = [(2, 2), (6, 2), (10, 2), (2, 6), (6, 6), (10, 6), (2, 10), (6, 10), (10, 10)]
    if n == 7:
        return [(2, 2), (6, 2), (10, 2), (2, 7), (10, 7), (2, 11), (10, 11)]
    if n == 8:
        return [p for p in pts if p != (6, 6)]
    return pts


def suit_tile(kind):
    t = blank_tile()
    suit, rank = kind // 9, kind % 9 + 1
    ink = SUIT_INK[suit]
    paste(t, glyph(str(rank), ink), 3, 0)
    img = Image.fromarray(t, "RGBA")
    d = ImageDraw.Draw(img)
    ox, oy = 1, 9
    if suit == 0:      # characters: a red seal under the rank
        d.rectangle([ox + 2, oy + 2, ox + 9, oy + 10], fill=RED)
        d.line([ox + 4, oy + 3, ox + 4, oy + 9], fill=FACE)
        d.line([ox + 7, oy + 3, ox + 7, oy + 9], fill=FACE)
        d.line([ox + 3, oy + 6, ox + 8, oy + 6], fill=FACE)
    elif suit == 1:    # dots: as many circles as the rank
        for cx, cy in dots_layout(rank):
            d.ellipse([ox + cx - 1, oy + cy - 1, ox + cx + 1, oy + cy + 1], fill=BLUE)
    else:              # bamboo: as many sticks as the rank
        cols = 1 if rank == 1 else 2 if rank <= 4 else 3
        rows = (rank + cols - 1) // cols
        for i in range(rank):
            c, r = i % cols, i // cols
            x = ox + 6 - (cols - 1) * 2 + c * 4
            y0 = oy + 1 + r * (12 // rows)
            d.line([x, y0, x, y0 + max(2, 12 // rows - 2)], fill=GREEN)
    return np.asarray(img)


def honour_tile(kind):
    t = blank_tile()
    img = Image.fromarray(t, "RGBA")
    d = ImageDraw.Draw(img)
    if kind < 31:      # winds: the letter, large
        letter = "ESWN"[kind - 27]
        paste(t, glyph(letter, INK, 2), 0, 4)
        return t
    if kind == 31:     # red dragon: a box with a stroke through it
        d.rectangle([3, 7, 10, 14], outline=RED)
        d.rectangle([4, 8, 9, 13], outline=RED)
        d.line([6, 3, 6, 19], fill=RED)
        d.line([7, 3, 7, 19], fill=RED)
    elif kind == 32:   # green dragon: a green sprig
        d.line([6, 3, 6, 19], fill=GREEN)
        for y in (5, 9, 13):
            d.line([2, y + 3, 6, y], fill=GREEN)
            d.line([10, y + 3, 6, y], fill=GREEN)
        d.line([3, 19, 9, 19], fill=GREEN)
    else:              # white dragon: a blue frame
        d.rectangle([2, 3, 10, 19], outline=BLUE)
        d.rectangle([4, 5, 8, 17], outline=BLUE)
    return np.asarray(img)


def bonus_tile(kind):
    t = blank_tile()
    img = Image.fromarray(t, "RGBA")
    d = ImageDraw.Draw(img)
    if kind < 38:      # flowers: pink petals
        for a in range(5):
            ang = a * 2 * np.pi / 5 - np.pi / 2
            cx, cy = 6 + 3 * np.cos(ang), 13 + 3 * np.sin(ang)
            d.ellipse([cx - 1.5, cy - 1.5, cx + 1.5, cy + 1.5], fill=PINK)
        d.point([6, 13], fill=ORANGE)
        ink, n = PINK, kind - 33
    else:              # seasons: an orange sun
        d.ellipse([3, 10, 9, 16], fill=ORANGE)
        for a in range(8):
            ang = a * np.pi / 4
            d.point([6 + 5 * np.cos(ang), 13 + 5 * np.sin(ang)], fill=ORANGE)
        ink, n = ORANGE, kind - 37
    t = np.asarray(img).copy()
    paste(t, glyph(str(n), ink), 3, 0)
    return t


def back_tile():
    t = blank_tile()
    t[1:23, 1:13] = BACK + (255,)
    for y in range(3, 21, 4):
        t[y, 3:11] = BACK_LIGHT + (255,)
    return t


def tile_sheet():
    frames = []
    for k in range(42):
        frames.append(suit_tile(k) if k < 27 else honour_tile(k) if k < 34 else bonus_tile(k))
    frames.append(back_tile())
    return np.concatenate(frames, axis=1)


def felt():
    rng = np.random.default_rng(785)
    base = np.array([22, 96, 62], float)
    noise = rng.normal(0, 5, (224, 320, 1))
    img = np.clip(base + noise, 0, 255)
    yy, xx = np.mgrid[0:224, 0:320]
    vignette = 1 - 0.35 * (((xx - 160) / 180) ** 2 + ((yy - 112) / 140) ** 2)
    img = np.clip(img * vignette[..., None], 0, 255)
    img[:6], img[-6:], img[:, :6], img[:, -6:] = (70, 42, 26), (70, 42, 26), (70, 42, 26), (70, 42, 26)
    img[6:8, 6:-6] = img[-8:-6, 6:-6] = (120, 84, 46)
    img[6:-6, 6:8] = img[6:-6, -8:-6] = (120, 84, 46)
    out = np.dstack([img.astype(np.uint8), np.full((224, 320), 255, np.uint8)])
    return out


def logo():
    w, h = 256, 48
    canvas = np.zeros((h, w, 4), np.uint8)
    g = text_pixels("MAHJONG", scale=3)
    gh, gw = g.shape
    x0 = (w - gw) // 2
    for dx, dy, col in ((2, 2, (60, 20, 10)), (0, 0, (236, 196, 64))):
        canvas[2 + dy:2 + dy + gh, x0 + dx:x0 + dx + gw][g > 0] = col + (255,)
    s = text_pixels("DEMO", scale=2)
    sh, sw = s.shape
    canvas[30:30 + sh, (w - sw) // 2:(w - sw) // 2 + sw][s > 0] = (240, 240, 240, 255)
    return canvas


# ---- FIX mini tiles ---------------------------------------------------------
TINY = {   # 3 x 5 digits and letters
    "1": ["010", "110", "010", "010", "111"], "2": ["110", "001", "010", "100", "111"],
    "3": ["110", "001", "010", "001", "110"], "4": ["101", "101", "111", "001", "001"],
    "5": ["111", "100", "110", "001", "110"], "6": ["011", "100", "110", "101", "010"],
    "7": ["111", "001", "010", "010", "010"], "8": ["010", "101", "010", "101", "010"],
    "9": ["010", "101", "011", "001", "110"], "E": ["111", "100", "110", "100", "111"],
    "S": ["011", "100", "010", "001", "110"], "W": ["101", "101", "101", "111", "101"],
    "N": ["101", "111", "111", "111", "101"],
}
ICONS = {  # 5 x 5 marks, colour index per mark
    "seal": (3, ["11111", "10101", "11111", "10101", "11111"]),
    "dot": (4, ["01110", "11011", "10001", "11011", "01110"]),
    "stick": (5, ["00100", "01110", "00100", "01110", "00100"]),
    "wind": (2, ["00000", "00000", "00000", "00000", "00000"]),
    "red": (3, ["00100", "11111", "10101", "11111", "00100"]),
    "green": (5, ["10101", "01110", "11111", "01110", "00100"]),
    "white": (4, ["11111", "10001", "10001", "10001", "11111"]),
    "flower": (6, ["01010", "11111", "01110", "11111", "01010"]),
    "season": (7, ["10101", "01110", "11111", "01110", "10101"]),
}
MINI_ICON_ORDER = list(ICONS)


def mini_cell(pattern, ink, left):
    """A cream cell (one half of a mini tile) with a pattern on it."""
    g = np.zeros((8, 8), np.uint8)
    g[0:7, :] = 1
    g[7, :] = 8
    g[:, 0 if left else 7] = 8
    w = len(pattern[0])
    x0 = 3 if left else 1
    if w == 3:
        x0 = 3 if left else 2
    for y, row in enumerate(pattern):
        for x, c in enumerate(row):
            if c == "1":
                g[1 + y, x0 + x] = ink
    return g


def mini_glyphs():
    """FIX tiles from FIX_MINI: ranks 1-9 (0-8), winds E S W N (9-12), suit and
    honour marks (13-21, as MINI_ICON_ORDER), a tile back (22, 23), a blank left cell (24)."""
    out = []
    for c in "123456789":
        out.append(mini_cell(TINY[c], 2, True))
    for c in "ESWN":
        out.append(mini_cell(TINY[c], 2, True))
    for name in MINI_ICON_ORDER:
        ink, pat = ICONS[name]
        out.append(mini_cell(pat, ink, False))
    back_l = np.full((8, 8), 9, np.uint8); back_l[7, :] = 8; back_l[:, 0] = 8; back_l[2:6:2, 2:8] = 10
    back_r = np.full((8, 8), 9, np.uint8); back_r[7, :] = 8; back_r[:, 7] = 8; back_r[2:6:2, 0:6] = 10
    out += [back_l, back_r, mini_cell(["000"] * 5, 2, True)]
    return out


def neo_word(rgb):
    r, g, b = (int(round(c * 31 / 255)) for c in rgb)
    return ((r & 1) << 14) | ((g & 1) << 13) | ((b & 1) << 12) | ((r >> 1) << 8) | ((g >> 1) << 4) | (b >> 1)


def build(fix_only=False):
    pack = ArtPack(prefix="mj", game_id="785", title="MAHJONG DEMO", first_bank=16)
    for k, g in enumerate(mini_glyphs()):
        pack.font_glyphs(FIX_MINI + k, g)
    if fix_only:
        pack.write_fix(GAME)
        print("art: S1 written")
        return
    pack.background("felt", felt(), banks=4, bank=16)
    pack.next_bank = 20
    pack.sprite("tiles", tile_sheet(), frame=(16, 32), banks=1)
    pack.sprite("logo", logo(), frame=(256, 48), banks=1)
    manifest = pack.write(GAME)
    words = [0] + [neo_word(c) for c in MINI_COLOURS[1:]]
    words += [0] * (16 - len(words))
    icons = "".join(f"#define MJ_MINI_{n.upper()} {13 + i}u\n" for i, n in enumerate(MINI_ICON_ORDER))
    (GAME / "artbox/generated/mj_mini.h").write_text(
        "/* Generated by games/mahjong_demo/tools/art.py: do not edit. */\n#ifndef MJ_MINI_H\n#define MJ_MINI_H\n"
        f"#define MJ_FIX_MINI {FIX_MINI:#06x}u\n#define MJ_MINI_PAL {MINI_PAL}u\n"
        "#define MJ_MINI_LETTER 9u\n" + icons +
        "#define MJ_MINI_BACK 22u\n#define MJ_MINI_BLANK 24u\n"
        f"static const uint16_t mj_mini_pal[16] = {{ {', '.join(f'{w:#06x}u' for w in words)} }};\n#endif\n",
        encoding="ascii")
    print(f"art: {len(manifest)} assets, banks 16-{pack.next_bank - 1} (test art)")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--fix-only", action="store_true")
    args = ap.parse_args()
    build(args.fix_only)


if __name__ == "__main__":
    main()
