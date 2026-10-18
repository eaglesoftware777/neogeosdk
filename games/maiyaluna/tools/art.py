#!/usr/bin/env python3
"""
Maiya & Luna art: rooms, people, objects, portraits and screens, packed with
tools/ng_artpack.py into C1/C2 and S1 (the build's GAME_ART_BUILDER).

    python3 games/maiyaluna/tools/art.py            (everything)
    python3 games/maiyaluna/tools/art.py --fix-only (S1 only)

Rooms are painted from their own maps (tools/ml_tiles.py), so a picture
always matches the room's walls; people and objects are drawn pixel by pixel
(tools/ml_people.py, tools/ml_objects.py). Painted pictures in
artbox/painted/ (title.png, ch<N>.png, portrait_<speaker>.png; see
tools/paint_import.py and artbox/PROVENANCE.md) replace the drawn ones.
"""
import argparse
import hashlib
from pathlib import Path
import re
import sys

import numpy as np
from PIL import Image, ImageDraw

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(GAME / "tools"))
from ng_artpack import ArtPack, text_pixels  # noqa: E402
import world as world_tool  # noqa: E402
import ml_tiles  # noqa: E402
import ml_people  # noqa: E402
import ml_objects  # noqa: E402

PAINTED = GAME / "artbox/painted"
# Maiya and Luna are drawn with the first game's own frames (games/maiya), in
# its two palettes: Luna wears Maiya's pixels in her own colours, as there.
ORIGINAL = ROOT / "games/maiya/artbox/in/generated"
ORIGINAL_HEADER = ROOT / "games/maiya/artbox/generated/maiya_assets.h"
SISTER_FRAMES = (['hero_idle0', 'hero_idle1', 'hero_idle2'] + [f'hero_gait_walk{k}' for k in range(8)] +
                 ['hero_crouch', 'hero_atk0', 'hero_atk1', 'hero_atk2', 'hero_atk3', 'hero_art0', 'hero_hurt0',
                  'hero_cast2'])
# townsfolk who also lived in the first game, by their frames there
# (Sunboy's third frame there glows, and the girl's third is someone else: their first stands in)
ORIGINAL_FOLK = {'sunboy': ['sunboy_0', 'sunboy_1', 'sunboy_0'], 'hana': ['girl_0', 'girl_1', 'girl_0']}
ROOM_CACHE = Path.home() / ".cache/neogeosdk/maiyaluna-rooms"
PAL_LIMIT = 112
FIX_HEARTS = 0x0E00      # FIX tiles: full, half and empty heart, a small arrow, a star


def painted(name, w, h):
    p = PAINTED / f"{name}.png"
    if p.is_file():
        im = Image.open(p).convert("RGBA")
        if im.size == (w, h):
            return np.asarray(im)
        print(f"art: {p.name} is {im.size}, wanted {(w, h)}: drawing instead")
    return None


def room_picture(room, wet, seed):
    """A room painted from its map (slow in Python), kept by what it was painted from."""
    key = hashlib.sha1(repr((room['rows'], wet, not room['lit'], seed, room['theme'])).encode()
                       + (GAME / "tools/ml_tiles.py").read_bytes()).hexdigest()
    cached = ROOM_CACHE / f"{key}.png"
    if cached.is_file():
        return np.asarray(Image.open(cached).convert("RGBA"))
    pic = ml_tiles.paint(room['rows'], wet=wet, dark=not room['lit'], seed=seed, theme=room['theme'])
    try:
        ROOM_CACHE.mkdir(parents=True, exist_ok=True)
        Image.fromarray(pic, "RGBA").save(cached)
    except OSError:
        pass
    return pic


def ng_word_rgb(w):
    """A Neo Geo palette word as 8-bit RGB."""
    r = ((w >> 8) & 15) << 1 | ((w >> 14) & 1)
    g = ((w >> 4) & 15) << 1 | ((w >> 13) & 1)
    b = (w & 15) << 1 | ((w >> 12) & 1)
    return [(v << 3) | (v >> 2) for v in (r, g, b)]


def original_palette(name):
    m = re.search(name + r"\[16\] = \{([^}]*)\}", ORIGINAL_HEADER.read_text())
    if not m:
        raise SystemExit(f"art: {name} not in {ORIGINAL_HEADER}")
    return np.array([ng_word_rgb(int(v)) for v in m.group(1).replace(',', ' ').split()], dtype=int)


def sister_sheets():
    """Maiya's and Luna's sheets (80 x 64 frames, SISTER_FRAMES order): each
    first-game frame mapped to her palette's pens, drawn in each palette."""
    maiya, luna = original_palette("mg_hero_pal"), original_palette("mg_hero_alt_pal")
    out_m, out_l = [], []
    for name in SISTER_FRAMES:
        im = np.asarray(Image.open(ORIGINAL / f"{name}.png").convert("RGBA")).astype(int)
        pen = ((im[..., None, :3] - maiya[None, None, 1:, :]) ** 2).sum(-1).argmin(-1) + 1
        pen[im[..., 3] < 128] = 0
        for pal, out in ((maiya, out_m), (luna, out_l)):
            fr = np.zeros(im.shape[:2] + (4,), np.uint8)
            fr[pen > 0, :3] = pal[pen[pen > 0]]
            fr[pen > 0, 3] = 255
            out.append(fr)
    return np.concatenate(out_m, axis=1), np.concatenate(out_l, axis=1)


def folk_sheet(look):
    """Three 32 x 48 frames (standing, swaying, talking) for a townsperson:
    the first game's own, or a painted sheet (tools/folk_import.py)."""
    if look in ORIGINAL_FOLK:
        return np.concatenate([np.asarray(Image.open(ORIGINAL / f"{n}.png").convert("RGBA")) for n in ORIGINAL_FOLK[look]],
                              axis=1)
    p = PAINTED / f"folk_{look}.png"
    if p.is_file():
        im = np.asarray(Image.open(p).convert("RGBA"))
        if im.shape[:2] == (48, 96):
            return im
        print(f"art: {p.name} is {im.shape[1]} x {im.shape[0]}, wanted 96 x 48")
    return None


def plate(w, h, fill=(20, 18, 34), edge=(240, 214, 140), inner=(80, 66, 110), radius=8):
    ss = 4
    img = Image.new("RGBA", (w * ss, h * ss), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([ss, ss, w * ss - ss, h * ss - ss], radius=radius * ss, fill=fill + (255,), outline=edge + (255,), width=ss * 2)
    d.rounded_rectangle([ss * 4, ss * 4, w * ss - ss * 4, h * ss - ss * 4], radius=(radius - 3) * ss, outline=inner + (255,), width=ss)
    small = np.asarray(img.resize((w, h), Image.LANCZOS)).copy()
    small[..., 3] = np.where(small[..., 3] >= 110, 255, 0)
    return small


def logo():
    """MAIYA & LUNA in a warm gold with a dark outline and a leaf, 256 x 80."""
    w, h = 256, 80
    canvas = np.zeros((h, w, 4), np.uint8)
    lines = [("MAIYA", 4), ("& LUNA", 4)]
    y = 4
    for text, scale in lines:
        glyphs = text_pixels(text, scale=scale)
        gh, gw = glyphs.shape
        x0 = (w - gw) // 2
        for yy in range(gh):
            t = yy / gh
            col = (int(255 - 20 * t), int(220 - 90 * t), int(90 - 50 * t))
            for xx in range(gw):
                if glyphs[yy, xx]:
                    canvas[y + yy, x0 + xx] = col + (255,)
        y += gh + 4
    a = canvas[..., 3] > 0
    grow = a.copy()
    for dy in (-2, -1, 0, 1, 2):
        for dx in (-2, -1, 0, 1, 2):
            if abs(dx) + abs(dy) <= 3:
                grow |= np.roll(np.roll(a, dy, 0), dx, 1)
    edge = grow & ~a
    canvas[edge] = (60, 30, 30, 255)
    shadow = np.roll(np.roll(grow, 2, 0), 2, 1) & ~grow
    canvas[shadow] = (20, 60, 40, 255)
    for k in range(9):                      # a leaf sprig by the ampersand
        canvas[48 + k, 30 + k // 2] = (70, 160, 70, 255)
        canvas[47 + k, 31 + k // 2] = (120, 210, 90, 255)
    return canvas


def heart_glyphs():
    full = ["..1..1..", ".111111.", "11111111", "11111111", ".111111.", "..1111..", "...11...", "........"]
    half = ["..1..2..", ".11112..", "1111.2..", "1111..2.", ".111.22.", "..11.2..", "...12...", "........"]
    empty = ["..2..2..", ".2.22.2.", "2......2", "2......2", ".2....2.", "..2..2..", "...22...", "........"]
    arrow = ["........", ".1111111", "..11111.", "...111..", "....1...", "........", "........", "........"]
    star = ["...1....", "...1....", "1111111.", ".11111..", "..111...", ".11.11..", ".1...1..", "........"]
    out = []
    for g in (full, half, empty, arrow, star):
        out.append(np.array([[{'.': 0, '1': 1, '2': 2}[c] for c in row] for row in g], dtype=np.uint8))
    return out


def build(fix_only=False):
    w = world_tool.load()
    pack = ArtPack(prefix="ml", game_id="783", title="MAIYA & LUNA", first_bank=16)
    for k, g in enumerate(heart_glyphs()):
        pack.font_glyphs(FIX_HEARTS + k, g)
    if fix_only:
        pack.write_fix(GAME)
        print("art: S1 written")
        return
    # ---- room pictures, then the title and chapter pictures, all at banks 16-31 --------
    bgs = w.bg_list()
    for name in bgs:
        room = next((r for r in w.ROOMS if r['bg'] == name or r['restored'] == name), None)
        wet = room['restored'] == name
        pic = painted(f"room_{name}", 320, 224)
        if pic is None:
            pic = room_picture(room, wet, w.ROOMS.index(room) + 1)
        pack.background(f"bg_{name}", pic, banks=16, bank=16)
    title = painted("title", 320, 224)
    if title is None:
        title = room_picture(w.ROOMS[0], True, 1)
    pack.background("bg_title", title, banks=16, bank=16)
    for ch in w.CHAPTERS:
        pic = painted(f"ch{ch['number']}", 320, 224)
        if pic is None:                     # the chapter's first room, until it has a painting
            r = next(r for r in w.ROOMS if r['name'] == ch['start'][0])
            pic = room_picture(r, False, 2)
        pack.background(f"bg_ch{ch['number']}", pic, banks=16, bank=16)
    # portraits are shown one at a time: they share banks 32-33, loaded with the dialogue box
    FACE_BANK = 32
    pack.next_bank = 34
    # ---- people: the sisters (80 x 64, the first game's frames), townsfolk (32 x 48, three
    # frames), or until a townsperson is painted, the drawn stand-in (13 frames of 32 x 32) ----
    people, folk = [], []
    maiya_sheet, luna_sheet = sister_sheets()
    for name, _ in w.SPEAKERS:
        look = name.lower()
        if look in ('maiya', 'luna'):
            pack.sprite(f"p_{look}", maiya_sheet if look == 'maiya' else luna_sheet, frame=(80, 64), banks=1)
            people.append(look)
            continue
        sheet = folk_sheet(look)
        if sheet is not None:
            pack.sprite(f"p_{look}", sheet, frame=(32, 48), banks=1)
            folk.append(look)
            continue
        if look == 'piko':
            sheet = ml_people.fox_sheet()
        elif look in ml_people.CAST:
            sheet = ml_people.sheet(ml_people.CAST[look])
        else:
            continue
        people.append(look)
        pack.sprite(f"p_{look}", sheet, frame=(32, 32), banks=2)
    # ---- objects ---------------------------------------------------------------------------
    # every object's frames in one sheet sharing eight banks (each 16 x 16 tile picks its
    # best); a look finds its frames from its first one (MLLookArt.base)
    sheets = ml_objects.object_sheets()
    obj_frames, obj_base = [], {}
    for look in w.look_list():
        if look in sheets:
            obj_base[look] = len(obj_frames)
            obj_frames += sheets[look][0]
    if len(obj_frames) > 256:
        raise SystemExit(f"art: {len(obj_frames)} object frames, more than a look's base can reach")
    pack.sprite("objects", ml_objects.sheet(obj_frames), frame=(16, 16), banks=8)
    for name, frames in ml_objects.effect_sheets().items():
        pack.sprite(f"fx_{name}", ml_objects.sheet(frames), frame=(16, 16), banks=1)
    pack.sprite("icons", ml_objects.sheet(ml_objects.item_icons([it[0] for it in w.ITEMS]) + ml_objects.category_icons()),
                frame=(16, 16), banks=2)
    # ---- portraits (64 x 64) ------------------------------------------------------------------
    for name, _ in w.SPEAKERS:
        pic = painted(f"portrait_{name.lower()}", 64, 64)
        if pic is None:
            continue
        pack.sprite(f"face_{name.lower()}", pic, frame=(64, 64), banks=2, bank=FACE_BANK)
    # ---- screens --------------------------------------------------------------------------------
    pack.sprite("dialog", plate(320, 64), frame=(320, 64), banks=1)
    pack.sprite("card", plate(288, 192), frame=(288, 192), banks=1)
    pack.sprite("logo", logo(), frame=(256, 80), banks=2)
    if pack.next_bank > PAL_LIMIT:
        raise SystemExit(f"art: palette banks {pack.next_bank} past the faded range ({PAL_LIMIT})")
    manifest = pack.write(GAME)
    # the look table for the game: which asset draws each look, and how
    looks = w.look_list()
    kinds = {'one': 0, 'glint': 1, 'state': 2, 'gear': 3, 'turning': 4, 'tone': 5, 'half': 6, 'creature': 7,
             'mover': 10}
    rows = []
    for look in looks:
        if look in people:
            rows.append(f"    {{ ML_ART_P_{look.upper()}, ML_LK_PERSON, 0 }},   /* {look} */")
        elif look in folk:
            rows.append(f"    {{ ML_ART_P_{look.upper()}, ML_LK_FOLK, 0 }},   /* {look} */")
        elif look in sheets:
            rows.append(f"    {{ ML_ART_OBJECTS, {kinds[sheets[look][1]]}, {obj_base[look]} }},   /* {look} */")
        else:
            rows.append(f"    {{ 0, ML_LK_NONE, 0 }},   /* {look} */")
    faces = []
    for name, _ in w.SPEAKERS:
        faces.append(f"ML_ART_FACE_{name}" if (PAINTED / f"portrait_{name.lower()}.png").is_file() else "0")
    bg_rows = [f"ML_ART_BG_{n.upper()}" for n in bgs]
    cards = ", ".join(f"ML_ART_BG_CH{c['number']}" for c in w.CHAPTERS)
    out = ["/* Generated by games/maiyaluna/tools/art.py: do not edit. */", "#ifndef ML_LOOKS_H", "#define ML_LOOKS_H",
           "#define ML_LK_PERSON 8", "#define ML_LK_NONE 9", "#define ML_LK_FOLK 11",
           "typedef struct { uint8_t art, kind, base; } MLLookArt;   /* base: its first frame in the sheet */",
           "static const MLLookArt ml_look_art[ML_LOOKS] = {", *rows, "};",
           f"static const uint8_t ml_face_art[ML_SPEAKERS] = {{ {', '.join(faces)} }};",
           f"static const uint8_t ml_bg_art[ML_BGS] = {{ {', '.join(bg_rows)} }};",
           f"static const uint8_t ml_card_art[ML_CHAPTERS] = {{ {cards} }};",
           f"#define ML_FIX_HEART {FIX_HEARTS:#06x}",
           "#endif", ""]
    (GAME / "artbox/generated/ml_looks.h").write_text("\n".join(out), encoding="ascii")
    print(f"art: {len(manifest)} assets, banks 16-{pack.next_bank - 1}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--fix-only", action="store_true")
    args = ap.parse_args()
    build(args.fix_only)


if __name__ == "__main__":
    main()
