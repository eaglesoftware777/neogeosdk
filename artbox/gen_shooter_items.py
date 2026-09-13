#!/usr/bin/env python3
"""
Draw the pick-ups and the super missile for the demo's shooter chapters.

Everything is drawn from the pixel maps below, so the art is original and
already at the size it is displayed - the sprite chip never has to shrink
it.  Badges are 16x16, the missile is 16x32.

    python3 artbox/gen_shooter_items.py games/demo/artbox/in/zzz_shooter
"""

import sys
from pathlib import Path
from PIL import Image

INK = {
    ".": (0, 0, 0, 0),
    "k": (16, 16, 32, 255),        # badge body
    "K": (48, 56, 96, 255),        # badge rim
    "w": (255, 255, 255, 255),
    "y": (255, 216, 48, 255),      # speed chevrons
    "Y": (255, 160, 32, 255),
    "r": (232, 40, 48, 255),       # missile nose / fins
    "R": (152, 16, 24, 255),
    "g": (200, 208, 224, 255),     # missile body
    "G": (120, 128, 152, 255),
    "c": (72, 232, 96, 255),       # life plane
    "C": (24, 144, 56, 255),
    "o": (255, 120, 24, 255),      # flame
    "O": (255, 224, 96, 255),
}

BADGE = [
    "....KKKKKKKK....",
    "..KKkkkkkkkkKK..",
    ".KkkkkkkkkkkkkK.",
    ".KkkkkkkkkkkkkK.",
    "KkkkkkkkkkkkkkkK",
    "KkkkkkkkkkkkkkkK",
    "KkkkkkkkkkkkkkkK",
    "KkkkkkkkkkkkkkkK",
    "KkkkkkkkkkkkkkkK",
    "KkkkkkkkkkkkkkkK",
    "KkkkkkkkkkkkkkkK",
    "KkkkkkkkkkkkkkkK",
    ".KkkkkkkkkkkkkK.",
    ".KkkkkkkkkkkkkK.",
    "..KKkkkkkkkkKK..",
    "....KKKKKKKK....",
]

SPEED = [
    "................",
    "................",
    "....y.....y.....",
    "....yy....yy....",
    ".....yy....yy...",
    "......yy....yy..",
    ".......yy....yy.",
    "........yy....yy",
    "........YY....YY",
    ".......YY....YY.",
    "......YY....YY..",
    ".....YY....YY...",
    "....YY....YY....",
    "....Y.....Y.....",
    "................",
    "................",
]

MISSILE_ICON = [
    "................",
    ".......rr.......",
    ".......rr.......",
    "......rrrr......",
    "......gggg......",
    "......gwgg......",
    "......gwgg......",
    "......gggg......",
    "......gggg......",
    ".....rggggr.....",
    "....rrggggrr....",
    "....RRGGGGRR....",
    ".....OOOOOO.....",
    "......oooo......",
    ".......oo.......",
    "................",
]

LIFE = [
    "................",
    ".......cc.......",
    ".......cc.......",
    "......cccc......",
    "......cccc......",
    "...c..cwwc..c...",
    "..cc..cwwc..cc..",
    ".ccccccwwcccccc.",
    "cccccccwwccccccc",
    "CCCCCCcwwcCCCCCC",
    "...C..cwwc..C...",
    "......CCCC......",
    ".....CCcCCC.....",
    "....CC.cc.CC....",
    "................",
    "................",
]

MISSILE = [
    ".......rr.......",
    ".......rr.......",
    "......rrrr......",
    "......rrrr......",
    ".....rrrrrr.....",
    ".....RRRRRR.....",
    ".....gggggg.....",
    ".....gwgggg.....",
    ".....gwgggg.....",
    ".....gwgggg.....",
    ".....gwgggg.....",
    ".....gwgggg.....",
    ".....gggggg.....",
    ".....gggggg.....",
    ".....GGGGGG.....",
    ".....gggggg.....",
    ".....gggggg.....",
    ".....gggggg.....",
    "....rgggggggr...",
    "...rrggggggrr...",
    "..rrrggggggrrr..",
    "..RRRGGGGGGRRR..",
    "..RR..OOOO..RR..",
    "......OOOO......",
    ".....OooooO.....",
    ".....oooooo.....",
    "......oooo......",
    "......oooo......",
    ".......oo.......",
    ".......oo.......",
    "........o.......",
    "................",
]


def paint(rows, over=None):
    img = Image.new("RGBA", (len(rows[0]), len(rows)), (0, 0, 0, 0))
    px = img.load()
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            px[x, y] = INK[ch]
    if over:
        for y, row in enumerate(over):
            for x, ch in enumerate(row):
                if ch != ".":
                    px[x, y] = INK[ch]
    return img


def main():
    out = Path(sys.argv[1] if len(sys.argv) > 1 else "games/demo/artbox/in/zzz_shooter")
    out.mkdir(parents=True, exist_ok=True)
    for name, rows, over in (("item_speed", BADGE, SPEED),
                             ("item_missile", BADGE, MISSILE_ICON),
                             ("item_life", BADGE, LIFE),
                             ("missile", MISSILE, None)):
        img = paint(rows, over)
        img.save(out / f"{name}.png")
        print(f"wrote {out / name}.png {img.size[0]}x{img.size[1]}")


if __name__ == "__main__":
    main()
