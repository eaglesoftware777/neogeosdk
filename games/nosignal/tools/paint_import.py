#!/usr/bin/env python3
"""
Painted backgrounds and portraits -> artbox/painted/, which tools/art.py then
uses in place of its drawn stand-ins.

    python3 games/nosignal/tools/paint_import.py SOURCE_DIR

SOURCE_DIR may hold:
  ns_bg<n>a.png ns_bg<n>b.png ns_bg<n>c.png   the three sections of stage n's street
                                        (any size near 3:2) -> stage<n>.png, 1440 x 224
  ns_title.png                          the title picture -> title.png, 320 x 224
  ns_end_<hero>.png                     a hero's ending -> end_<hero>.png, 320 x 224
  ns_p_<speaker>.png                    portraits -> portrait_<speaker>.png, 64 x 64

Each street section is scaled to 496 pixels wide and cut to 224 rows so its
kerb lands on row KERB (fighters walk below it, ns_data.c's y 140-206); the
joins are cross-faded over 24 columns, which leaves 1440; every road is given
the first section's road colour, then the whole road is cut to ROAD_COLOURS
shared colours, so its tiles can share palettes and show no tile-sized
patches on the flat asphalt. Images are reduced
by area averaging and written fresh, so no metadata from the sources is kept.
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image

GAME = Path(__file__).resolve().parents[1]
OUT = GAME / "artbox/painted"
SPEAKERS = ["jay", "zoe", "kai", "rico", "thief", "biker", "skater",
            "bigflex", "zerox", "m08", "behemoth", "proxy", "overmind", "archivist"]
STAGES = 8
# panels whose floor is brighter than the scenery above it (tiled interiors)
LIGHT_FLOOR = {"ns_bg3b", "ns_bg7a", "ns_bg8a", "ns_bg8b"}
# panels whose floor cracks fool the search: the kerb row, at the 496-pixel width
KERB_AT = {}
KERB = 146
ROAD_COLOURS = 15     # one full palette: the whole road shares it, and keeps its markings and puddles
SECTION_W, H = 496, 224
BLEND = 24
STAGE_W = 1440


def kerb_row(rgb, light_floor=False):
    """The kerb: the lowest strong step from the light pavement down to the dark
    road, between 45% and 80% of the height (light_floor: from a dark wall down
    to a light floor, for labs and ship decks)."""
    lum = rgb.astype(float).mean(axis=2).mean(axis=1)
    h = len(lum)
    ys = np.arange(int(h * 0.45), int(h * 0.8))
    step = lum[ys - 3] - lum[ys + 3]
    if light_floor:
        step = -step
    strong = ys[step > 0.5 * step.max()]
    return int(strong.max())


def section(path, light_floor=False, kerb=None):
    im = Image.open(path).convert("RGB")
    w, h = im.size
    sh = round(h * SECTION_W / w)
    if sh < H:
        raise SystemExit(f"paint_import: {path.name} is too short for a {H}-row street")
    rgb = np.asarray(im.resize((SECTION_W, sh), Image.BOX))
    top = min(max(0, (kerb if kerb is not None else kerb_row(rgb, light_floor)) - KERB), sh - H)
    return rgb[top:top + H].astype(float)


def street(src, names, out_name):
    parts = []
    for n in names:
        f = src / f"{n}.png"
        if not f.is_file():
            print(f"paint_import: no {f.name}, keeping the drawn {out_name}")
            return
        parts.append(section(f, n in LIGHT_FLOOR, KERB_AT.get(n)))
    road = slice(KERB + 4, H)
    want = parts[0][road].reshape(-1, 3).mean(axis=0)
    for p in parts[1:]:                     # one road colour for the whole street
        p[road] += want - p[road].reshape(-1, 3).mean(axis=0)
    strip = parts[0]
    for p in parts[1:]:
        a = np.linspace(0, 1, BLEND)[None, :, None]
        seam = strip[:, -BLEND:] * (1 - a) + p[:, :BLEND] * a
        strip = np.concatenate([strip[:, :-BLEND], seam, p[:, BLEND:]], axis=1)
    strip = np.clip(strip[:, :STAGE_W], 0, 255).astype(np.uint8)
    band = Image.fromarray(np.ascontiguousarray(strip[KERB + 4:]))
    strip[KERB + 4:] = np.asarray(band.quantize(ROAD_COLOURS, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE).convert("RGB"))
    rgba = np.dstack([strip, np.full((H, strip.shape[1]), 255, np.uint8)])
    Image.fromarray(rgba, "RGBA").save(OUT / f"{out_name}.png")
    print(f"paint_import: {out_name} <- {', '.join(names)} ({strip.shape[1]} x {H})")


def picture(src, name, out_name, w, h):
    f = src / f"{name}.png"
    if not f.is_file():
        print(f"paint_import: no {f.name}, keeping the drawn {out_name}")
        return
    im = Image.open(f).convert("RGB")
    sw, sh = im.size
    cw = min(sw, round(sh * w / h))
    ch = min(sh, round(sw * h / w))
    x0, y0 = (sw - cw) // 2, (sh - ch) // 2
    im = im.crop((x0, y0, x0 + cw, y0 + ch)).resize((w, h), Image.BOX)
    rgba = np.dstack([np.asarray(im), np.full((h, w), 255, np.uint8)])
    Image.fromarray(rgba, "RGBA").save(OUT / f"{out_name}.png")
    print(f"paint_import: {out_name} <- {f.name} {sw}x{sh}")


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    src = Path(sys.argv[1])
    OUT.mkdir(parents=True, exist_ok=True)
    for n in range(1, STAGES + 1):
        street(src, [f"ns_bg{n}a", f"ns_bg{n}b", f"ns_bg{n}c"], f"stage{n}")
    picture(src, "ns_title", "title", 320, 224)
    for hero in ("jay", "zoe", "kai"):
        picture(src, f"ns_end_{hero}", f"end_{hero}", 320, 224)
    for s in SPEAKERS:
        picture(src, f"ns_p_{s}", f"portrait_{s}", 64, 64)


if __name__ == "__main__":
    main()
