#!/usr/bin/env python3
"""
Painted pictures -> artbox/painted/, which tools/art.py uses in place of its drawn ones.

    python3 games/maiyaluna/tools/paint_import.py SOURCE_DIR

SOURCE_DIR may hold ml_title.png and ml_ch<N>.png (pictures near 3:2, cut to
320 x 224) and ml_p_<speaker>.png (portraits drawn at 96 x 96 and enlarged:
read back at 96 and cut to the 64 x 64 face). Maiya's and Luna's portraits
come from the original game's own art (games/maiya), cut the same way. Every
file is written fresh, so no metadata from a source is kept.
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
OUT = GAME / "artbox/painted"
FACE = (16, 6, 80, 70)


def save(img, name):
    rgba = np.asarray(img.convert("RGBA")).copy()
    rgba[..., 3] = 255
    Image.fromarray(rgba, "RGBA").save(OUT / name)


def picture(src, name):
    im = Image.open(src).convert("RGB")
    w, h = im.size
    cw = min(w, round(h * 320 / 224))
    x0 = (w - cw) // 2
    save(im.crop((x0, 0, x0 + cw, h)).resize((320, 224), Image.BOX), name)


def portrait(src, name, native=96):
    im = Image.open(src).convert("RGB")
    if im.size != (native, native):
        im = im.resize((native, native), Image.NEAREST)
    save(im.crop(FACE), name)


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    src = Path(sys.argv[1])
    OUT.mkdir(parents=True, exist_ok=True)
    original = ROOT / "games/maiya/artbox/in/generated"
    for name, f in (("portrait_maiya.png", "portrait.png"), ("portrait_luna.png", "portrait_alt.png")):
        if (original / f).is_file():
            portrait(original / f, name)
            print(f"paint_import: {name} <- games/maiya {f}")
    for f in sorted(src.glob("ml_*.png")):
        stem = f.stem[3:]
        if stem == "title" or stem.startswith("ch"):
            picture(f, f"{stem}.png")
        elif stem.startswith("p_"):
            portrait(f, f"portrait_{stem[2:]}.png")
        else:
            continue
        print(f"paint_import: {f.name}")


if __name__ == "__main__":
    main()
