"""Generate cute kawaii Neo Geo FIX layer graphics (S1 ROM tiles) for Maiya.

    python3 games/maiya/tools/build_fix_assets.py

Generates indexed (mode P, max 15 colors) PNGs in games/maiya/artbox/infix/:
- 0.png: HUD icons (Maiya portrait avatar, Heart icons, Rose icons, Score numbers)
- 1.png: Cute Ornamental Dialog Frame (for Wizard / Spirit / Girl hints)
- 2.png: Secret Art Power Surge Gauge
- 3.png: Bonus Stage Reticle & Shuriken / Rose counters
- 4.png: "EARTH RESTORED!" Cleansing Banner
- 5.png: Continue Countdown & Extra Life icons
- 6.png: Storyline Cutscene Emblems (Sun Crest & Rose Crest)
"""

from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

GAME = Path(__file__).resolve().parents[1]
INFIX_DIR = GAME / "artbox/infix"
INFIX_DIR.mkdir(parents=True, exist_ok=True)

# Fixed arcade palette for HUD and FIX layer (16 colors)
# 0: transparent
# 1: black outline, 2: dark crimson, 3: rose pink, 4: bright pink
# 5: deep green, 6: emerald green, 7: mint lime, 8: dark gold
# 9: bright gold, 10: sun yellow, 11: white, 12: dark purple
# 13: lavender, 14: cyan, 15: skin peach
PALETTE = [
    0, 0, 0,        # 0: transparent
    20, 20, 24,     # 1: black / dark border
    160, 24, 60,    # 2: crimson
    230, 70, 120,   # 3: rose pink
    255, 150, 180,  # 4: light pink
    30, 90, 40,     # 5: deep green
    60, 180, 80,    # 6: emerald green
    140, 240, 130,  # 7: mint lime
    180, 120, 30,   # 8: dark gold
    240, 190, 50,   # 9: bright gold
    255, 240, 100,  # 10: sun yellow
    255, 255, 255,  # 11: white
    70, 40, 90,     # 12: dark purple
    160, 130, 210,  # 13: lavender
    90, 210, 240,   # 14: cyan
    255, 210, 175,  # 15: skin peach
]
# Pad palette to 256 colors for PIL mode P
PALETTE += [0] * (768 - len(PALETTE))


def make_p_image(width, height):
    img = Image.new("P", (width, height), 0)
    img.putpalette(PALETTE)
    return img


def draw_hud():
    """0.png: 160 x 32 (20 x 4 tiles). Maiya avatar, Hearts, Roses, Star."""
    im = make_p_image(160, 32)
    d = ImageDraw.Draw(im)

    # 1. Cute Maiya Mini-Avatar (24x24 at x=4, y=4)
    # Face background
    d.ellipse((4, 4, 27, 27), fill=15, outline=1)
    # Blonde hair curls
    d.chord((3, 2, 28, 16), start=160, end=380, fill=9, outline=1)
    d.arc((3, 2, 28, 16), start=160, end=380, fill=10)
    # Rose hair ornament
    d.ellipse((21, 5, 27, 11), fill=3, outline=2)
    d.point((24, 8), fill=4)
    # Eyes (cute anime eyes)
    d.rectangle((9, 13, 11, 18), fill=5)
    d.point((10, 14), fill=11)  # highlight
    d.rectangle((18, 13, 20, 18), fill=5)
    d.point((19, 14), fill=11)  # highlight
    # Blushing cheeks
    d.ellipse((6, 18, 9, 21), fill=4)
    d.ellipse((21, 18, 24, 21), fill=4)
    # Smiling mouth
    d.arc((12, 19, 17, 23), start=0, end=180, fill=2)

    # 2. Heart Icons (Full & Empty) at x=36
    # Full Heart (12x12)
    hx, hy = 36, 6
    d.polygon([(hx+6, hy+11), (hx+1, hy+5), (hx+1, hy+3), (hx+3, hy+1),
               (hx+5, hy+1), (hx+6, hy+3), (hx+7, hy+1), (hx+9, hy+1),
               (hx+11, hy+3), (hx+11, hy+5)], fill=3, outline=1)
    d.point((hx+3, hy+3), fill=11)

    # Empty Heart (12x12)
    hx = 52
    d.polygon([(hx+6, hy+11), (hx+1, hy+5), (hx+1, hy+3), (hx+3, hy+1),
               (hx+5, hy+1), (hx+6, hy+3), (hx+7, hy+1), (hx+9, hy+1),
               (hx+11, hy+3), (hx+11, hy+5)], fill=0, outline=1)

    # 3. Rose Icons (Full & Empty) at x=70
    rx, ry = 70, 6
    d.ellipse((rx+2, ry+2, rx+10, ry+10), fill=3, outline=1)
    d.ellipse((rx+4, ry+4, rx+8, ry+8), fill=2)
    d.point((rx+5, ry+5), fill=4)
    d.line([(rx+6, ry+10), (rx+6, ry+13)], fill=6)
    d.point((rx+7, ry+12), fill=7)

    # 4. Sun Star / Gem at x=90
    sx, sy = 90, 6
    d.polygon([(sx+6, sy+1), (sx+8, sy+4), (sx+11, sy+6), (sx+8, sy+8),
               (sx+6, sy+11), (sx+4, sy+8), (sx+1, sy+6), (sx+4, sy+4)],
              fill=10, outline=8)
    d.point((sx+6, sy+6), fill=11)

    # 5. Little Fairy Guide Avatar at x=110
    fx, fy = 110, 4
    d.ellipse((fx+5, fy+7, fx+15, fy+19), fill=15, outline=1)
    d.ellipse((fx+1, fy+4, fx+7, fy+14), fill=14, outline=11)
    d.ellipse((fx+13, fy+4, fx+19, fy+14), fill=14, outline=11)
    d.point((fx+8, fy+12), fill=1)
    d.point((fx+12, fy+12), fill=1)
    d.arc((fx+8, fy+14, fx+12, fy+17), start=0, end=180, fill=2)

    # 6. Mini Wizard Avatar at x=135
    wx, wy = 135, 4
    d.polygon([(wx+6, wy+1), (wx+1, wy+12), (wx+11, wy+12)], fill=12, outline=1)
    d.ellipse((wx+2, wy+10, wx+10, wy+18), fill=15, outline=1)
    d.ellipse((wx+1, wy+14, wx+11, wy+22), fill=11, outline=1)  # white beard
    d.point((wx+4, wy+13), fill=1)
    d.point((wx+8, wy+13), fill=1)

    im.save(INFIX_DIR / "0.png")


def draw_dialog_box():
    """1.png: 320 x 24 (40 x 3 tiles). Cute ornamental border dialog frame."""
    im = make_p_image(320, 24)
    d = ImageDraw.Draw(im)

    # Top & bottom borders with rose gold studs
    d.rectangle((2, 1, 317, 22), fill=0, outline=8)
    d.rectangle((4, 3, 315, 20), fill=0, outline=9)

    # Decorative corner rose rosettes
    for cx, cy in ((3, 2), (314, 2), (3, 19), (314, 19)):
        d.rectangle((cx, cy, cx+3, cy+3), fill=3, outline=2)
        d.point((cx+1, cy+1), fill=10)

    # Center label crest
    d.rectangle((120, 0, 200, 7), fill=8, outline=1)
    d.rectangle((122, 1, 198, 5), fill=9)

    im.save(INFIX_DIR / "1.png")


def draw_gauge():
    """2.png: 160 x 16 (20 x 2 tiles). Power surge gauge bar."""
    im = make_p_image(160, 16)
    d = ImageDraw.Draw(im)

    # Gauge outline
    d.rectangle((2, 2, 157, 13), fill=1, outline=8)
    # Energy fill gradient
    for x in range(4, 155):
        c = 6 if x < 60 else (10 if x < 110 else 3)
        d.line([(x, 4), (x, 11)], fill=c)
    # Highlights
    d.line([(4, 4), (154, 4)], fill=11)

    im.save(INFIX_DIR / "2.png")


def draw_bonus_hud():
    """3.png: 128 x 16 (16 x 2 tiles). Bonus stage target cursor & counter."""
    im = make_p_image(128, 16)
    d = ImageDraw.Draw(im)

    # Reticle cursor
    d.rectangle((2, 2, 13, 13), fill=0, outline=10)
    d.line([(0, 7), (15, 7)], fill=11)
    d.line([(7, 0), (7, 15)], fill=11)

    # Shuriken / Rose icons
    for i in range(5):
        rx = 24 + i * 16
        d.ellipse((rx+2, 2, rx+10, 10), fill=3, outline=1)
        d.point((rx+6, 6), fill=10)

    im.save(INFIX_DIR / "3.png")


def draw_cleansing_banner():
    """4.png: 256 x 32 (32 x 4 tiles). Earth Restored victory banner."""
    im = make_p_image(256, 32)
    d = ImageDraw.Draw(im)

    d.rectangle((4, 4, 251, 27), fill=5, outline=9)
    d.rectangle((6, 6, 249, 25), fill=6, outline=10)
    # Sparkling stars across banner
    for sx in (12, 36, 80, 140, 190, 230):
        d.polygon([(sx, 12), (sx+3, 15), (sx, 18), (sx-3, 15)], fill=10)
        d.point((sx, 15), fill=11)

    im.save(INFIX_DIR / "4.png")


def draw_countdown():
    """5.png: 160 x 32 (20 x 4 tiles). Big arcade countdown numbers & extra life."""
    im = make_p_image(160, 32)
    d = ImageDraw.Draw(im)

    # Extra life banner
    d.ellipse((4, 6, 26, 26), fill=15, outline=1)
    d.chord((3, 4, 27, 18), start=160, end=380, fill=9, outline=1)
    d.ellipse((20, 6, 25, 11), fill=3)

    # Golden numerals
    for i in range(5):
        nx = 36 + i * 24
        d.rectangle((nx, 6, nx+16, 26), fill=9, outline=1)
        d.rectangle((nx+2, 8, nx+14, 24), fill=10)

    im.save(INFIX_DIR / "5.png")




# ---------------------------------------------------------------------------
#  HUD glyphs injected straight into the FIX ROM
#
#  fixtext_out() can only address FIX tiles 0..255, and the imported infix
#  sheets land far above that, so the handful of icons the HUD prints as text
#  (lives, roses, the Sun Key, coins) are written into the low, unused codes
#  here.  They are drawn on pen 1 alone: the HUD palettes are flat inks, so a
#  silhouette takes whatever colour it is printed in.
#
#  Re-run this script after any `make art` rebuild of 780-s1.s1.
# ---------------------------------------------------------------------------

S1_PATH = GAME / "artbox/780-s1.s1"

GLYPHS = {
    0x01: (  # heart: one life
        " ##  ## ",
        "########",
        "########",
        "########",
        " ###### ",
        "  ####  ",
        "   ##   ",
        "        ",
    ),
    0x02: (  # rose: one Secret Art charge
        "  ####  ",
        " ###### ",
        "########",
        " ###### ",
        "  ####  ",
        "   ##   ",
        "  #### ",
        "   ##   ",
    ),
    0x03: (  # the Golden Sun Key
        "  ####  ",
        " ##  ## ",
        " ##  ## ",
        "  ####  ",
        "   ##   ",
        "   ###  ",
        "   ##   ",
        "   ###  ",
    ),
    0x04: (  # coin
        "  ####  ",
        " ###### ",
        "## ## ##",
        "## ## ##",
        "## ## ##",
        "## ## ##",
        " ###### ",
        "  ####  ",
    ),
    0x05: (  # leaf: swiftness
        "     ###",
        "   #####",
        "  ######",
        " ### ###",
        "#### ## ",
        "###  #  ",
        "##   #  ",
        "#    #  ",
    ),
    0x06: (  # spark
        "   ##   ",
        "   ##   ",
        "## ## ##",
        " ###### ",
        " ###### ",
        "## ## ##",
        "   ##   ",
        "   ##   ",
    ),
}

_COL_PAIRS = [(4, 5), (6, 7), (0, 1), (2, 3)]


def encode_fix_tile(rows):
    """8 rows of text art -> a 32 byte Neo Geo FIX tile on pen 1."""
    px = np.zeros((8, 8), dtype=np.uint8)
    for y, row in enumerate(rows):
        for x, ch in enumerate(row[:8]):
            px[y, x] = 1 if ch != ' ' else 0
    out = bytearray(32)
    for gi, (cl, cr) in enumerate(_COL_PAIRS):
        for row in range(8):
            out[gi * 8 + row] = ((int(px[row, cr]) & 0xF) << 4) | (int(px[row, cl]) & 0xF)
    return bytes(out)


def inject_hud_glyphs():
    if not S1_PATH.exists():
        print(f"  (no {S1_PATH.name} yet; run the art build first)")
        return
    rom = bytearray(S1_PATH.read_bytes())
    for code, rows in GLYPHS.items():
        rom[code * 32:(code + 1) * 32] = encode_fix_tile(rows)
    S1_PATH.write_bytes(bytes(rom))
    print(f"  Injected {len(GLYPHS)} HUD glyphs into {S1_PATH.name}")


def main():
    print("== Generating Cute Kawaii FIX Layer Assets ==")
    draw_hud()
    draw_dialog_box()
    draw_gauge()
    draw_bonus_hud()
    draw_cleansing_banner()
    draw_countdown()
    print(f"Generated 6 indexed FIX PNGs in {INFIX_DIR}")
    inject_hud_glyphs()


if __name__ == "__main__":
    main()
