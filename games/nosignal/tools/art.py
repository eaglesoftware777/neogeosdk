#!/usr/bin/env python3
"""
No Signal Z Alpha art: fighters, stages, props, effects and screens, packed
with tools/ng_artpack.py into C1/C2 and S1 (the build's GAME_ARTBUILDER).

    python3 games/nosignal/tools/art.py            (everything)
    python3 games/nosignal/tools/art.py --fix-only (S1 only)

Painted art in artbox/painted/ is used when it is there (tools/import_sheets.py,
tools/paint_import.py): fighters/<name>/NN.png, stage<n>.png (1440 x 224),
title.png and end_<hero>.png (320 x 224), portrait_<speaker>.png (64 x 64).
Anything missing is drawn here as a plain stand-in so the game always builds
and runs; stand-ins are listed when the art is built.

Palette banks (the fades cover 16-111): 16-31 the picture on screen (a
stage, the title, the menu backdrop, an ending: one at a time), 32-43 the
heroes, 44-63 five shared slots for the street crews (a stage loads its own
crew's colours: no two fighters of one stage share a slot), 64-71 the boss
of the stage, 72-73 the portrait in the dialogue box, then props, sparks,
the dialogue box, the card, the logo and the status line's hero portrait.
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

PAINTED = GAME / "artbox/painted"
SLOT = {"hero_jay": 32, "hero_zoe": 36, "hero_kai": 40, "A": 44, "B": 48, "C": 52, "D": 56, "E": 60, "boss": 64}
# name: frames, frame size (pixels, square), bank slot, banks
FIGHTERS = {
    "jay": (30, 128, "hero_jay", 4), "zoe": (30, 128, "hero_zoe", 4), "kai": (30, 128, "hero_kai", 4),
    "thief": (10, 128, "A", 4), "runner": (10, 128, "A", 4), "walker": (10, 128, "A", 4),
    "skater": (10, 128, "B", 4), "android": (10, 128, "B", 4),
    "biker": (10, 128, "C", 4), "bloater": (10, 128, "C", 4), "alien": (10, 128, "C", 4),
    "drone": (10, 128, "D", 4), "robodog": (10, 128, "D", 4),
    "guard": (10, 128, "E", 4),
    "rico": (20, 128, "boss", 8), "bigflex": (20, 160, "boss", 8), "zerox": (20, 128, "boss", 8),
    "m08": (20, 160, "boss", 8), "behemoth": (20, 160, "boss", 8), "proxy": (20, 128, "boss", 8),
    "overmind": (10, 192, "boss", 8), "archivist": (30, 160, "boss", 8),
}
HEROES = ("jay", "zoe", "kai")
COLORS = {"jay": ((200, 40, 50), (240, 230, 200), (110, 120, 60)), "zoe": ((40, 180, 170), (240, 120, 180), (180, 160, 230)),
          "kai": ((130, 70, 180), (120, 120, 130), (70, 60, 60)), "thief": ((40, 150, 70), (230, 230, 230), (100, 100, 100)),
          "skater": ((120, 220, 60), (240, 220, 60), (70, 90, 160)), "biker": ((40, 40, 40), (200, 40, 40), (60, 70, 120)),
          "rico": ((210, 30, 40), (240, 220, 120), (30, 30, 40)), "guard": ((30, 40, 90), (200, 200, 210), (30, 40, 90)),
          "drone": ((230, 240, 250), (40, 200, 230), (230, 240, 250)), "runner": ((230, 230, 220), (120, 200, 120), (60, 60, 70)),
          "bloater": ((250, 140, 30), (120, 200, 120), (60, 70, 100)), "android": ((235, 235, 240), (40, 210, 230), (40, 40, 50)),
          "robodog": ((150, 150, 160), (200, 160, 120), (150, 150, 160)), "walker": ((240, 200, 30), (60, 60, 60), (60, 60, 60)),
          "alien": ((150, 120, 190), (200, 200, 220), (150, 120, 190)), "bigflex": ((30, 30, 30), (230, 190, 60), (60, 60, 80)),
          "zerox": ((200, 220, 230), (130, 210, 120), (200, 220, 230)), "m08": ((120, 140, 170), (230, 220, 80), (90, 100, 120)),
          "behemoth": ((90, 90, 100), (120, 220, 120), (70, 70, 80)), "proxy": ((70, 70, 80), (200, 30, 40), (70, 70, 80)),
          "overmind": ((20, 20, 20), (60, 230, 120), (20, 60, 30)), "archivist": ((190, 190, 210), (160, 90, 220), (120, 80, 170))}
# who speaks in scenes, in ns.h's NS_SPK_ order; all share the portrait slot
SPEAKERS = ["jay", "zoe", "kai", "rico", "thief", "biker", "skater",
            "bigflex", "zerox", "m08", "behemoth", "proxy", "overmind", "archivist"]
STAGES = 8
FACE_BANK = 72
FIX_ICONS = 0x0E00
stand_ins = []


def painted(rel, w, h):
    p = PAINTED / rel
    if p.is_file():
        im = Image.open(p).convert("RGBA")
        if im.size == (w, h):
            return np.asarray(im)
        print(f"art: {rel} is {im.size}, wanted {(w, h)}: drawing a stand-in")
    stand_ins.append(rel)
    return None


def stand_in_figure(name, frame):
    """A plain silhouette in the fighter's colours, posed roughly like the frame
    (drawn at 128 pixels, then scaled to the fighter's frame size)."""
    size = FIGHTERS[name][1]
    pic = stand_in_128(name, frame)
    if size == 128:
        return pic
    big = Image.fromarray(pic).resize((size, size), Image.NEAREST)
    return np.asarray(big)


def stand_in_128(name, frame):
    jacket, skin_ish, pants = COLORS[name]
    img = Image.new("RGBA", (128, 128), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    big = name in ("biker", "rico")
    h = 104 if big else 94
    top = 124 - h
    lean = {6: 6, 7: 6, 17: 8, 21: -6, 22: 4, 23: -10}.get(frame, 0)
    if frame in (24, 8) and name not in ("jay", "zoe", "kai"):
        lean = 0
    down = (frame == 24 and name in ("jay", "zoe", "kai")) or (frame == 8 and name not in ("jay", "zoe", "kai"))
    if down:
        d.rounded_rectangle([20, 108, 108, 124], radius=6, fill=jacket + (255,), outline=(20, 20, 20, 255))
        d.ellipse([96, 104, 116, 124], fill=(230, 180, 140, 255), outline=(20, 20, 20, 255))
        return np.asarray(img)
    air = frame in (8, 9, 15, 23) and name in ("jay", "zoe", "kai")
    lift = 18 if air else 0
    cx = 64 + lean
    d.rectangle([cx - 9, top + 50 - lift, cx + 9, 124 - lift - 2], fill=pants + (255,), outline=(20, 20, 20, 255))
    d.rounded_rectangle([cx - 14, top + 20 - lift, cx + 14, top + 56 - lift], radius=5, fill=jacket + (255,), outline=(20, 20, 20, 255))
    d.ellipse([cx - 10, top - lift, cx + 10, top + 22 - lift], fill=(230, 180, 140, 255), outline=(20, 20, 20, 255))
    d.rectangle([cx - 10, top - lift, cx + 10, top + 7 - lift], fill=skin_ish + (255,))
    reach = 0
    if (name in ("jay", "zoe", "kai") and frame in (11, 12, 13, 18, 19, 26, 27)) or (name not in ("jay", "zoe", "kai") and frame == 5):
        reach = 40
    if reach:
        d.rectangle([cx + 10, top + 26 - lift, cx + 10 + reach, top + 32 - lift], fill=jacket + (255,), outline=(20, 20, 20, 255))
    if name in ("jay", "zoe", "kai") and frame in (14, 15, 16):
        d.rectangle([cx + 6, top + 60 - lift, cx + 50, top + 68 - lift], fill=pants + (255,), outline=(20, 20, 20, 255))
    if name in ("jay", "zoe", "kai") and frame == 29:
        d.rectangle([cx - 30, 116, cx + 34, 120], fill=(80, 80, 90, 255))
        d.ellipse([cx - 34, 114, cx - 24, 124], fill=(30, 30, 30, 255))
        d.ellipse([cx + 26, 114, cx + 36, 124], fill=(30, 30, 30, 255))
        d.rectangle([cx + 28, top + 30, cx + 31, 118], fill=(80, 80, 90, 255))
    return np.asarray(img)


def fighter_sheet(name):
    frames = []
    count, size = FIGHTERS[name][0], FIGHTERS[name][1]
    for i in range(count):
        pic = painted(f"fighters/{name}/{i:02d}.png", size, size)
        if pic is None:
            stand_ins.pop()
            pic = stand_in_figure(name, i)
            if f"fighters/{name}" not in stand_ins:
                stand_ins.append(f"fighters/{name}")
        frames.append(pic)
    return np.concatenate(frames, axis=1)


STAGE_SKY = {1: (250, 170, 110), 2: (200, 120, 90), 3: (40, 60, 90), 4: (170, 200, 220), 5: (150, 170, 90),
             6: (60, 70, 120), 7: (200, 230, 210), 8: (90, 60, 140)}


def stage(n):
    pic = painted(f"stage{n}.png", 1440, 224)
    if pic is not None:
        return pic
    sky = STAGE_SKY[n]
    img = Image.new("RGBA", (1440, 224), (0, 0, 0, 255))
    d = ImageDraw.Draw(img)
    for y in range(130):
        t = y / 130
        d.line([(0, y), (1439, y)], fill=(int(sky[0] * (1 - 0.3 * t)), int(sky[1] * (1 - 0.2 * t) + 30 * t), int(sky[2] * (1 - 0.1 * t) + 60 * t), 255))
    rng = np.random.default_rng(4 + n)
    x = 0
    while x < 1440:
        w = int(rng.integers(80, 160)); h = int(rng.integers(60, 110))
        col = tuple(int(c) for c in rng.integers(70, 160, 3))
        d.rectangle([x, 130 - h, x + w - 6, 134], fill=col + (255,), outline=(40, 30, 40, 255))
        for wy in range(130 - h + 10, 126, 16):
            for wx in range(x + 8, x + w - 16, 18):
                d.rectangle([wx, wy, wx + 8, wy + 8], fill=(250, 220, 120, 255) if rng.random() < 0.5 else (60, 70, 100, 255))
        x += w
    d.rectangle([0, 134, 1439, 150], fill=(170, 170, 176, 255))
    d.rectangle([0, 150, 1439, 223], fill=(96, 96, 104, 255))
    for xx in range(0, 1440, 64):
        d.rectangle([xx + 10, 186, xx + 40, 189], fill=(220, 210, 120, 255))
    return np.asarray(img)


def title_pic():
    pic = painted("title.png", 320, 224)
    return pic if pic is not None else night_city()


def night_city():
    """The menus' backdrop (and the title's, until it is painted): a night skyline."""
    img = Image.new("RGBA", (320, 224), (10, 8, 30, 255))
    d = ImageDraw.Draw(img)
    for y in range(224):
        t = y / 224
        d.line([(0, y), (319, y)], fill=(int(30 + 90 * t), int(10 + 20 * t), int(60 + 40 * t), 255))
    rng = np.random.default_rng(9)
    x = 0
    while x < 320:
        w = int(rng.integers(20, 50)); h = int(rng.integers(50, 130))
        d.rectangle([x, 224 - h, x + w - 3, 223], fill=(20, 16, 40, 255))
        for wy in range(224 - h + 6, 220, 10):
            for wx in range(x + 4, x + w - 8, 8):
                if rng.random() < 0.4:
                    d.rectangle([wx, wy, wx + 3, wy + 4], fill=(250, 80, 200, 255) if rng.random() < 0.3 else (90, 220, 255, 255))
        x += w
    return np.asarray(img)


def prop_frames():
    """crate, barrel (a bin), bat, food (a pizza slice), score (a gold sneaker), scooter, ramp, cable, phone (a shot), wreck."""
    out = []

    def canvas():
        return Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    im = canvas(); d = ImageDraw.Draw(im)
    d.rectangle([3, 8, 28, 30], fill=(170, 120, 60, 255), outline=(60, 36, 20, 255)); d.line([(3, 8), (28, 30)], fill=(110, 70, 36, 255)); d.line([(28, 8), (3, 30)], fill=(110, 70, 36, 255))
    out.append(im)
    im = canvas(); d = ImageDraw.Draw(im)
    d.rectangle([7, 6, 25, 30], fill=(70, 120, 90, 255), outline=(20, 40, 30, 255)); d.rectangle([5, 4, 27, 8], fill=(90, 150, 110, 255), outline=(20, 40, 30, 255))
    out.append(im)
    im = canvas(); d = ImageDraw.Draw(im)
    d.line([(4, 28), (28, 8)], fill=(200, 160, 100, 255), width=4); d.line([(4, 28), (9, 24)], fill=(40, 40, 40, 255), width=4)
    out.append(im)
    im = canvas(); d = ImageDraw.Draw(im)
    d.polygon([(4, 26), (28, 26), (16, 6)], fill=(250, 200, 80, 255), outline=(120, 70, 20, 255)); d.ellipse([12, 16, 16, 20], fill=(200, 40, 40, 255)); d.ellipse([17, 20, 21, 24], fill=(200, 40, 40, 255))
    out.append(im)
    im = canvas(); d = ImageDraw.Draw(im)
    d.polygon([(4, 26), (28, 26), (26, 18), (16, 14), (12, 8), (6, 10)], fill=(250, 210, 60, 255), outline=(120, 80, 10, 255)); d.line([(6, 22), (26, 22)], fill=(255, 255, 255, 255))
    out.append(im)
    im = canvas(); d = ImageDraw.Draw(im)
    d.rectangle([4, 25, 28, 27], fill=(60, 200, 230, 255)); d.line([(25, 26), (24, 6)], fill=(80, 80, 90, 255), width=2); d.line([(20, 6), (28, 6)], fill=(40, 40, 40, 255), width=2)
    d.ellipse([2, 24, 9, 31], fill=(30, 30, 30, 255)); d.ellipse([23, 24, 30, 31], fill=(30, 30, 30, 255))
    out.append(im)
    im = canvas(); d = ImageDraw.Draw(im)
    d.polygon([(2, 30), (30, 30), (30, 16)], fill=(200, 150, 60, 255), outline=(90, 60, 20, 255))
    out.append(im)
    im = canvas(); d = ImageDraw.Draw(im)
    d.line([(2, 28), (12, 22), (20, 28), (30, 24)], fill=(30, 30, 30, 255), width=3); d.line([(14, 20), (18, 14)], fill=(120, 230, 255, 255), width=2); d.line([(18, 14), (16, 10)], fill=(255, 255, 160, 255), width=2)
    out.append(im)
    im = canvas(); d = ImageDraw.Draw(im)
    d.rounded_rectangle([10, 8, 22, 26], radius=2, fill=(30, 30, 40, 255), outline=(200, 200, 210, 255)); d.rectangle([12, 10, 20, 22], fill=(80, 160, 240, 255))
    out.append(im)
    im = canvas(); d = ImageDraw.Draw(im)
    d.line([(4, 28), (26, 10)], fill=(80, 80, 90, 255), width=3); d.ellipse([20, 4, 30, 14], fill=(30, 30, 30, 255)); d.rectangle([2, 26, 12, 29], fill=(60, 200, 230, 255))
    out.append(im)
    return np.concatenate([np.asarray(i) for i in out], axis=1)


def spark_frames():
    out = []
    for k, r in enumerate((6, 10, 13)):
        im = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        pts = []
        for i in range(16):
            rr = r if i % 2 == 0 else r * 0.45
            a = i * np.pi / 8 + k * 0.3
            pts.append((16 + rr * np.cos(a), 16 + rr * np.sin(a)))
        d.polygon(pts, fill=(255, 250, 200, 255), outline=(255, 140, 40, 255))
        out.append(np.asarray(im))
    im = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    for i in range(5):
        d.ellipse([4 + i * 5, 18 - (i % 2) * 3, 12 + i * 5, 26 - (i % 2) * 3], fill=(210, 200, 190, 255))
    out.append(np.asarray(im))
    return np.concatenate(out, axis=1)


def plate(w, h, fill=(18, 14, 30), edge=(80, 230, 255)):
    ss = 4
    img = Image.new("RGBA", (w * ss, h * ss), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([ss, ss, w * ss - ss, h * ss - ss], radius=6 * ss, fill=fill + (255,), outline=edge + (255,), width=ss * 2)
    small = np.asarray(img.resize((w, h), Image.LANCZOS)).copy()
    small[..., 3] = np.where(small[..., 3] >= 110, 255, 0)
    return small


def logo():
    """NO SIGNAL in chunky letters with a glitch shadow, Z ALPHA under it: 256 x 80."""
    w, h = 256, 80
    canvas = np.zeros((h, w, 4), np.uint8)
    glyphs = text_pixels("NO SIGNAL", scale=3)
    gh, gw = glyphs.shape
    x0, y0 = (w - gw) // 2, 6
    for dx, dy, col in ((3, 2, (255, 40, 160)), (-3, 0, (40, 230, 255)), (0, 0, (255, 255, 255))):
        for yy in range(gh):
            for xx in range(gw):
                if glyphs[yy, xx]:
                    canvas[y0 + yy + dy, x0 + xx + dx] = col + (255,)
    sub = text_pixels("Z ALPHA", scale=2)
    sh, sw = sub.shape
    x1, y1 = (w - sw) // 2, 46
    for yy in range(sh):
        for xx in range(sw):
            if sub[yy, xx]:
                canvas[y1 + yy, x1 + xx] = (255, 220, 60, 255)
    a = canvas[..., 3] > 0
    grow = a.copy()
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            grow |= np.roll(np.roll(a, dy, 0), dx, 1)
    canvas[grow & ~a] = (10, 6, 20, 255)
    return canvas


def fix_glyphs():
    """0-2 the old battery cells (unused), 3 a signal bar, 4-12 a health bar cell
    filled 0-8 pixels, 13 its left end, 14 its right end, 15 a battery's end.
    Bar colours (ns_draw.c bar_pal): 1 fill, 4 its light top, 5 its shade,
    2 the dark edge, 3 the empty part."""
    full = ["11111111", "12222221", "12222221", "12222221", "12222221", "12222221", "11111111", "........"]
    empty = ["11111111", "1......1", "1......1", "1......1", "1......1", "1......1", "11111111", "........"]
    tip = ["........", "11......", "11......", "11......", "11......", "11......", "........", "........"]
    bar = ["........", "........", "......11", "....1111", "..111111", "11111111", "11111111", "........"]
    out = []
    for g in (full, empty, tip, bar):
        out.append(np.array([[{'.': 0, '1': 1, '2': 2}[c] for c in row] for row in g], dtype=np.uint8))
    for k in range(9):
        cell = np.zeros((8, 8), np.uint8)
        cell[0, :] = 2
        cell[7, :] = 2
        for x in range(8):
            if x < k:
                cell[1, x], cell[2, x] = 4, 4
                cell[3:6, x] = 1
                cell[6, x] = 5
            else:
                cell[1:7, x] = 3
        out.append(cell)
    left = np.zeros((8, 8), np.uint8)
    left[1:7, 6] = 2
    left[0:8, 7] = 2
    left[0, 6] = left[7, 6] = 0
    right = np.zeros((8, 8), np.uint8)
    right[0:8, 0] = 2
    right[1:7, 1] = 2
    right[0, 1] = right[7, 1] = 0
    nub = right.copy()
    nub[2:6, 2:4] = 2
    out += [left, right, nub]
    return out


def ending_pic(hero):
    pic = painted(f"end_{hero}.png", 320, 224)
    return pic if pic is not None else night_city()


def build(fix_only=False):
    # text_edge: the letters ringed in colour 2 (black in every text palette,
    # ns_draw.c ns_fix_palettes), readable over the streets
    pack = ArtPack(prefix="ns", game_id="784", title="NO SIGNAL", first_bank=16, text_edge=2)
    for k, g in enumerate(fix_glyphs()):
        pack.font_glyphs(FIX_ICONS + k, g)
    if fix_only:
        pack.write_fix(GAME)
        print("art: S1 written")
        return
    # the paintings get refit passes: their skies and roads stay smooth
    for n in range(1, STAGES + 1):
        pack.background(f"stage{n}", stage(n), banks=16, bank=16, refine=6)
    title_painted = (PAINTED / "title.png").is_file()
    pack.background("title", title_pic(), banks=16, bank=16, refine=6)
    pack.background("menu", night_city(), banks=16, bank=16)
    for hero in HEROES:
        pack.background(f"end_{hero}", ending_pic(hero), banks=16, bank=16, refine=6)
    for name, (count, size, slot, banks) in FIGHTERS.items():
        # Refit the existing banks to the complete outfit across all poses.
        # This improves skin/cloth separation without extra runtime palettes.
        pack.sprite(f"f_{name}", fighter_sheet(name), frame=(size, size), banks=banks, bank=SLOT[slot], refine=4)
    faces = []
    for s in SPEAKERS:
        pic = painted(f"portrait_{s}.png", 64, 64)
        if pic is not None:
            pack.sprite(f"face_{s}", pic, frame=(64, 64), banks=2, bank=FACE_BANK, refine=4)
            faces.append(f"NS_ART_FACE_{s.upper()}")
        else:
            faces.append("0")
    pack.next_bank = FACE_BANK + 2
    pack.sprite("props", prop_frames(), frame=(32, 32), banks=2)
    pack.sprite("sparks", spark_frames(), frame=(32, 32), banks=1)
    pack.sprite("dialog", plate(320, 64), frame=(320, 64), banks=1)
    pack.sprite("card", plate(288, 192), frame=(288, 192), banks=1)
    pack.sprite("logo", logo(), frame=(256, 80), banks=2)
    # The status line's portrait of the hero playing (shrunk to 32 x 32 by the
    # hardware): the dialogue portraits again, in banks of their own, so a
    # speaker in the dialogue box never recolours it.
    hud_bank, huds = pack.next_bank, []
    for h in HEROES:
        pic = painted(f"portrait_{h}.png", 64, 64)
        if pic is not None:
            pack.sprite(f"hud_{h}", pic, frame=(64, 64), banks=2, bank=hud_bank, refine=4)
            huds.append(f"NS_ART_HUD_{h.upper()}")
        else:
            huds.append("0")
    pack.next_bank = hud_bank + 2
    if pack.next_bank > 112:
        raise SystemExit(f"art: palette banks {pack.next_bank} past the faded range (112)")
    manifest = pack.write(GAME)
    ids = [l for l in (GAME / "artbox/generated/ns_assets.h").read_text().splitlines() if l.startswith("#define NS_ART_")]
    (GAME / "artbox/generated/ns_art_ids.h").write_text(
        "/* Generated by games/nosignal/tools/art.py: do not edit. The asset numbers alone. */\n"
        "#ifndef NS_ART_IDS_H\n#define NS_ART_IDS_H\n" + "\n".join(ids) + "\n#endif\n", encoding="ascii")
    (GAME / "artbox/generated/ns_faces.h").write_text(
        "/* Generated by games/nosignal/tools/art.py: do not edit. */\n#ifndef NS_FACES_H\n#define NS_FACES_H\n"
        f"static const uint8_t ns_face_art[{len(SPEAKERS)}] = {{ {', '.join(faces)} }};\n"
        f"static const uint8_t ns_hud_art[{len(HEROES)}] = {{ {', '.join(huds)} }};\n"
        f"#define NS_FIX_ICONS {FIX_ICONS:#06x}\n"
        f"#define NS_TITLE_PAINTED {1 if title_painted else 0}   /* the heroes are in the picture */\n#endif\n", encoding="ascii")
    print(f"art: {len(manifest)} assets, banks 16-{pack.next_bank - 1}")
    if stand_ins:
        print("art: stand-ins (no painted art yet): " + ", ".join(sorted(set(stand_ins))))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--fix-only", action="store_true")
    args = ap.parse_args()
    build(args.fix_only)


if __name__ == "__main__":
    main()
