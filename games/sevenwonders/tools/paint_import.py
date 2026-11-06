#!/usr/bin/env python3
"""
Painted backgrounds -> artbox/painted/<name>.png at 320 x 224, which tools/art.py
then uses in place of its drawn scenes.

    python3 games/sevenwonders/tools/paint_import.py SOURCE_DIR

SOURCE_DIR holds sw_bg0.png .. sw_bg6.png, sw_title.png and sw_map.png (any size
near 3:2). Each is cut to 320:224 (keeping the right side, where the landmark
stands; the title and map keep the centre), reduced by area averaging and
written as a fresh image, so no metadata from the source is kept.
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image

GAME = Path(__file__).resolve().parents[1]
NAMES = [f"bg{i}" for i in range(7)] + ["title", "map"]


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    src = Path(sys.argv[1])
    out = GAME / "artbox/painted"
    out.mkdir(parents=True, exist_ok=True)
    for name in NAMES:
        f = src / f"sw_{name}.png"
        if not f.is_file():
            print(f"paint_import: no {f.name}, keeping the drawn {name}")
            continue
        im = Image.open(f).convert("RGB")
        w, h = im.size
        cw = min(w, round(h * 320 / 224))
        x0 = (w - cw) // 2 if name in ("title", "map") else w - cw
        im = im.crop((x0, 0, x0 + cw, h)).resize((320, 224), Image.BOX)
        rgba = np.dstack([np.asarray(im), np.full((224, 320), 255, np.uint8)])
        Image.fromarray(rgba, "RGBA").save(out / f"{name}.png")
        print(f"paint_import: {name} <- {f.name} {w}x{h}")


if __name__ == "__main__":
    main()
