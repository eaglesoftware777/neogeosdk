"""Build Shadow League's C1/C2 and S1 ROMs and its generated C tables.

C ROM (tiles, row-major per image): 256 blank tiles, then each fighter's
frames (112x128, 7x8 tiles), the hit/guard/dust effects, the floor shadow,
the Arata Mountain Dojo stage (512x224), the boot logo in bank $49 and the
utility tiles at $FFFD-$FFFF. S1: the system font, the game's text page,
life/power bar cells and large lettering.

    python3 games/shadowleague/tools/art.py [--fix-only]
"""

import argparse
import importlib.util
import io
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image

TOOLS = Path(__file__).resolve().parent
GAME = TOOLS.parent
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / "artbox"))
sys.path.insert(0, str(TOOLS))
from palette_banks import palette_words, quantize, reconstruct  # noqa: E402
from tile_codec import encode_image, write_utility_tiles  # noqa: E402
import fighter_cook  # noqa: E402

spec = importlib.util.spec_from_file_location("slf_system_font", ROOT / "bios/tools/gen_sfix.py")
font = importlib.util.module_from_spec(spec)
spec.loader.exec_module(font)

GAME_ID = 781
OUT = GAME / "artbox/generated"
ART = GAME / "artbox"
ROMS = ROOT / "roms/shadowleague"
BOOT_C_BANK = 0x49
GAME_FIX_BASE = 0xD00
FIX_HUD = 0xE00
FIX_BIG = 0x400
PAL_STAGE = 32


def text_pixels(text, scale=1):
    image = np.zeros((8 * scale, len(text) * 8 * scale), dtype=np.uint8)
    for i, char in enumerate(text):
        for y, row in enumerate(font.GLYPHS.get(char.upper(), "00 " * 7).split()):
            for x in range(5):
                if int(row, 16) & (1 << (4 - x)):
                    image[y * scale:(y + 1) * scale, (i * 8 + x + 1) * scale:(i * 8 + x + 2) * scale] = 1
    return image


def logo_pixels():
    image = np.zeros((64, 240), dtype=np.uint8)
    top = text_pixels("SHADOW LEAGUE", 2)
    image[12:28, 12:12 + top.shape[1]] = top
    low = text_pixels("FIGHTING", 2)
    image[34:50, 56:56 + low.shape[1]] = low * 3
    image[54:56, 24:216] = 3
    image[:16, 224:] = 0
    return image


def emit(name, values, ctype="uint16_t"):
    body = ",".join(str(int(v)) for v in np.asarray(values).flatten())
    return f"static const {ctype} {name}[] = {{{body}}};"


# --------------------------------------------------------------------------
#  Effects and stage
# --------------------------------------------------------------------------
def spark(frame, kind):
    """32x32 burst: kind 0 hit (gold), 1 guard (blue), 2 dust (tan)."""
    img = np.zeros((32, 32), np.uint8)
    ys, xs = np.mgrid[0:32, 0:32] - 15.5
    r = np.hypot(xs, ys)
    a = np.arctan2(ys, xs)
    if kind == 2:
        for k, (cx, cy) in enumerate(((-7, 6), (6, 5), (0, 2))):
            rr = np.hypot(xs - cx * (1 + frame * 0.3), ys - cy + frame * 2)
            rad = 4 + frame * 1.3 - k
            img[rr < rad] = 2 + (rr[rr < rad] < rad * 0.5)
        return img
    grow = [5, 9, 13, 15][frame]
    spikes = 0.65 + 0.35 * np.cos(a * (8 if kind == 0 else 6) + frame)
    edge = grow * spikes
    img[r < edge] = 2
    img[r < edge * 0.7] = 3
    img[r < edge * 0.4] = 4
    if frame >= 2:
        img[r < edge * (0.25 if frame == 2 else 0.6)] = 0
    return img


SPARK_PALS = [
    [(0, 0, 0), (40, 20, 10), (230, 110, 30), (255, 210, 60), (255, 255, 230)],
    [(0, 0, 0), (10, 20, 40), (40, 110, 230), (120, 200, 255), (240, 250, 255)],
    [(0, 0, 0), (40, 30, 20), (150, 120, 90), (200, 176, 140), (230, 214, 186)],
]


def shadow():
    img = np.zeros((16, 64), np.uint8)
    ys, xs = np.mgrid[0:16, 0:64]
    e = ((xs - 31.5) / 30) ** 2 + ((ys - 8) / 5.5) ** 2
    img[e < 1] = 1
    return img


def noise(shape, seed, scale):
    rng = np.random.default_rng(seed)
    small = rng.random((shape[0] // scale + 2, shape[1] // scale + 2))
    big = np.kron(small, np.ones((scale, scale)))[:shape[0], :shape[1]]
    return big


def stage():
    """Arata Mountain Dojo at dawn: open screens onto the peaks, a plank floor."""
    h, w = 224, 512
    img = np.zeros((h, w, 3), np.float32)
    y = np.arange(h)[:, None] * np.ones((1, w))
    x = np.ones((h, 1)) * np.arange(w)[None, :]
    # Sky.
    t = np.clip(y / 150, 0, 1)
    sky = (1 - t)[..., None] * np.array([70, 92, 150]) + t[..., None] * np.array([246, 170, 110])
    img[:] = sky
    # Sun.
    sun = np.hypot(x - 300, y - 118) < 18
    img[sun] = (255, 226, 160)
    # Mountains, far and near.
    far = 96 + 26 * np.sin(x / 47.0) + 14 * np.sin(x / 19.0 + 1.3)
    near = 122 + 22 * np.sin(x / 61.0 + 2.0) + 9 * np.sin(x / 13.0)
    img[y > far] = (118, 104, 140)
    img[(y > far) & (y < far + 6) & (np.sin(x / 9.0) > 0.2)] = (220, 206, 220)
    img[y > near] = (64, 72, 98)
    # Misty pines at the foot.
    trees = 146 + 6 * np.abs(np.sin(x / 5.0)) + 4 * np.sin(x / 23.0)
    img[y > trees] = (40, 58, 62)
    # Dojo interior: walls, beams, pillars, shoji panels.
    wall_top, floor_top = 40, 168
    img[:wall_top] = (60, 38, 28)
    img[:wall_top][(np.arange(wall_top) % 10 < 2)] = (44, 28, 22)
    img[wall_top - 6:wall_top] = (96, 62, 40)
    img[wall_top:wall_top + 3] = (30, 20, 16)
    for px in range(0, w + 1, 128):
        x0, x1 = px - 9, px + 9
        xs = slice(max(x0, 0), min(x1, w))
        img[wall_top:floor_top, xs] = (110, 70, 44)
        img[wall_top:floor_top, max(x0, 0):max(x0 + 3, 0)] = (70, 44, 30)
        img[wall_top:floor_top, min(x1 - 3, w):min(x1, w)] = (150, 100, 64)
    for k, px in enumerate(range(0, w, 128)):
        if k in (0, 3):
            # Closed shoji: paper and lattice.
            area = (slice(wall_top + 3, floor_top), slice(px + 9, px + 119))
            img[area] = (236, 226, 204)
            for gx in range(px + 9, px + 119, 18):
                img[wall_top + 3:floor_top, gx:gx + 2] = (120, 84, 54)
            for gy in range(wall_top + 3, floor_top, 22):
                img[gy:gy + 2, px + 9:px + 119] = (120, 84, 54)
            if k == 0:
                # Hanging scroll.
                img[60:128, px + 50:px + 78] = (226, 214, 180)
                img[58:62, px + 46:px + 82] = (80, 50, 34)
                img[126:130, px + 46:px + 82] = (80, 50, 34)
                for sy in range(70, 120, 9):
                    img[sy:sy + 5, px + 60:px + 68] = (40, 30, 30)
        else:
            img[wall_top + 3:wall_top + 8, px + 9:px + 119] = (96, 62, 40)
    # Lanterns.
    for lx in (128, 384):
        img[wall_top + 3:wall_top + 14, lx - 1:lx + 1] = (30, 20, 16)
        lamp = ((x - lx) / 9) ** 2 + ((y - (wall_top + 26)) / 13) ** 2 < 1
        img[lamp] = (210, 60, 40)
        img[lamp & (np.abs(x - lx) < 4)] = (250, 150, 80)
        img[(np.abs(y - (wall_top + 14)) < 2) & (np.abs(x - lx) < 7)] = (40, 26, 18)
        img[(np.abs(y - (wall_top + 39)) < 2) & (np.abs(x - lx) < 7)] = (40, 26, 18)
    # Floor: planks running toward the viewer, polished.
    fy = y - floor_top
    floor = y >= floor_top
    plank = np.floor((x - 256) / (12 + fy * 0.55))
    shade = 0.82 + 0.18 * ((plank.astype(int) % 3) / 2.0)
    base = np.array([168, 112, 66]) * shade[..., None]
    grain = noise((h, w), 7, 4) * 14
    base = base + grain[..., None] - 7
    img[floor] = base[floor]
    seam = floor & (np.abs(((x - 256) / (12 + fy * 0.55)) - np.round((x - 256) / (12 + fy * 0.55))) < 0.04)
    img[seam] = (96, 60, 34)
    img[(y >= floor_top) & (y < floor_top + 3)] = (88, 56, 34)
    # Window light on the floor.
    glow = floor & (np.abs(x - 300 + (y - floor_top) * 0.6) < 40)
    img[glow] = img[glow] * 1.12
    img = np.clip(img, 0, 255).astype(np.uint8)
    rgba = np.dstack([img, np.full((h, w), 255, np.uint8)])
    return rgba


# --------------------------------------------------------------------------
#  FIX
# --------------------------------------------------------------------------
def fix_rom():
    rom = bytearray(font.build_rom())
    lo = GAME_FIX_BASE * 32
    rom[lo:lo + 256 * 32] = rom[:256 * 32]
    rom[0xFF * 32:0x100 * 32] = bytes(32)
    return rom


BIG_TEXT = [("TITLE1", "SHADOW", 3), ("TITLE1B", "LEAGUE", 3), ("TITLE2", "FIGHTING", 2), ("ROUND1", "ROUND 1", 2),
            ("ROUND2", "ROUND 2", 2), ("ROUND3", "ROUND 3", 2), ("FINAL", "FINAL ROUND", 2), ("FIGHT", "FIGHT!", 3),
            ("KO", "K.O.", 3), ("TIMEUP", "TIME UP", 2), ("WIN", "WINS", 2), ("DRAW", "DRAW", 2),
            ("VS", "VS", 3), ("PERFECT", "PERFECT", 2), ("GAMEOVER", "GAME OVER", 2), ("COUNTER", "COUNTER", 1),
            ("CONGRATS", "CONGRATULATIONS", 2), ("DIGITS", "0123456789", 2),
            ("CHALLENGER", "CHALLENGER!", 2)]


# FIX palettes: 0 text, 1 gold text, 2 dim text, 3 HUD bars, 4 gold lettering,
# 5 red lettering, 6 blue lettering, 7 white lettering. Entry 1 is the glyph
# ink (top half for lettering), 2 its shadow, 3 the lower half.
FIX_PALS = [
    [(0, 0, 0), (250, 250, 250), (16, 16, 24), (190, 196, 210)],
    [(0, 0, 0), (255, 220, 90), (40, 20, 0), (230, 160, 40)],
    [(0, 0, 0), (150, 150, 170), (16, 16, 24), (110, 110, 130)],
    [(0, 0, 0), (20, 16, 24), (255, 226, 70), (236, 130, 30), (110, 20, 24), (250, 250, 250),
     (70, 150, 255), (200, 240, 255)],
    [(0, 0, 0), (20, 10, 0), (255, 232, 96), (236, 140, 30)],
    [(0, 0, 0), (24, 4, 4), (255, 110, 90), (200, 30, 30)],
    [(0, 0, 0), (4, 8, 28), (150, 210, 255), (40, 110, 230)],
    [(0, 0, 0), (12, 12, 20), (255, 255, 255), (170, 180, 200)],
]


def build_fix():
    rom = fix_rom()
    tile = FIX_HUD
    defines = []

    def put(glyph):
        nonlocal tile
        rom[tile * 32:(tile + 1) * 32] = font.encode_tile(glyph.astype(np.uint8))
        tile += 1
        return tile - 1

    # Life bars: frame on rows 0/7, fill rows 1-6; level 0-8 pixels.
    # P1 fills from the left, P2 from the right. Colors: 1 frame, 2 fill hi,
    # 3 fill lo, 4 empty, 5 recent damage.
    for side in ("L", "R"):
        first = tile
        for level in range(9):
            g = np.full((8, 8), 4, np.uint8)
            g[0] = g[7] = 1
            cols = range(level) if side == "L" else range(8 - level, 8)
            for c in cols:
                g[1:4, c] = 2
                g[4:7, c] = 3
            put(g)
        defines.append(f"#define SLF_FIX_BAR_{side} {first}u")
    caps = []
    for side in ("L", "R"):
        g = np.zeros((8, 8), np.uint8)
        g[:, 3 if side == "L" else 4] = 1
        g[0, 3:] = 1 if side == "L" else 0
        if side == "R":
            g[0, :5] = 1
        g[7] = g[0]
        caps.append(put(g))
    defines.append(f"#define SLF_FIX_CAP_L {caps[0]}u\n#define SLF_FIX_CAP_R {caps[1]}u")
    # Power gauge cell: 1 frame, 6 power fill, 7 full stock.
    first = tile
    for level in range(9):
        g = np.full((8, 8), 0, np.uint8)
        g[1] = g[6] = 1
        for c in range(level):
            g[2:6, c] = 6 if level < 8 else 7
        put(g)
    defines.append(f"#define SLF_FIX_POWER {first}u")
    # Round-win marker: empty and won.
    for name, fill in (("MARK_OFF", 4), ("MARK_ON", 2)):
        g = np.zeros((8, 8), np.uint8)
        ys, xs = np.mgrid[0:8, 0:8]
        disk = np.hypot(xs - 3.5, ys - 3.5)
        g[disk < 3.6] = 1
        g[disk < 2.6] = fill
        defines.append(f"#define SLF_FIX_{name} {put(g)}u")
    # Large lettering, in the free tiles between the system font and the text page.
    bars_end = tile
    tile = FIX_BIG
    for name, text, scale in BIG_TEXT:
        px = text_pixels(text, scale)
        hgt, wid = px.shape
        shade = np.zeros_like(px)
        shade[1:, 1:] = px[:-1, :-1]
        g = np.where(px, np.where(np.arange(hgt)[:, None] < hgt // 2, 2, 3), np.where(shade, 1, 0)).astype(np.uint8)
        first = tile
        for y in range(0, hgt, 8):
            for x in range(0, wid, 8):
                put(g[y:y + 8, x:x + 8])
        defines.append(f"#define SLF_BIG_{name} {first}u\n#define SLF_BIG_{name}_W {wid // 8}u\n#define SLF_BIG_{name}_H {hgt // 8}u")
    if tile > GAME_FIX_BASE or bars_end > 0x1000:
        raise ValueError("FIX art overflows the game's private banks")
    for dest in (ART, ROMS):
        dest.mkdir(parents=True, exist_ok=True)
        (dest / f"{GAME_ID}-s1.s1").write_bytes(bytes(rom))
    return defines


# --------------------------------------------------------------------------
#  C ROM
# --------------------------------------------------------------------------
def pal16(colors):
    out = np.zeros((16, 3), np.uint8)
    out[:len(colors)] = colors
    return out


def build():
    OUT.mkdir(parents=True, exist_ok=True)
    ROMS.mkdir(parents=True, exist_ok=True)
    c1, c2 = io.BytesIO(), io.BytesIO()
    boot_start, boot_end = BOOT_C_BANK * 256 * 64, (BOOT_C_BANK + 1) * 256 * 64

    def place(indexed):
        count = indexed.shape[0] * indexed.shape[1] // 256
        if c1.tell() < boot_end and c1.tell() + count * 64 > boot_start:
            for s in (c1, c2):
                s.write(bytes(boot_end - s.tell()))
        tile = c1.tell() // 64
        a, b = encode_image(indexed, count)
        c1.write(a)
        c2.write(b)
        return tile

    for s in (c1, c2):
        s.write(bytes(256 * 64))
    header = ["/* Generated by games/shadowleague/tools/art.py. */", "#ifndef SLF_ASSETS_H", "#define SLF_ASSETS_H",
              "#include <stdint.h>", '#include "sdk/2d_engine/ng_fight.h"']
    fighters = fighter_cook.load_all()
    order = ["kenji", "jack"]
    fighters.sort(key=lambda f: order.index(f.id) if f.id in order else 99)
    bases, pals, names, styles, origins = [], [], [], [], []
    for f in fighters:
        first = None
        for image in f.images:
            t = place(image)
            first = t if first is None else first
        bases.append(first)
        normal, alt = f.palettes()
        pals.append([palette_words(normal), palette_words(alt)])
        header.append(f.c_tables(f"slf_{f.id}"))
        names.append(f.data["name"])
        styles.append(f.data.get("discipline", f.data.get("style", "")))
        origins.append(f.data.get("origin", ""))
        f.sheet(OUT / f"{f.id}_sheet.png")
    header.append(f"#define SLF_FIGHTER_COUNT {len(fighters)}u")
    header.append("static const NGFFighterDef * const slf_fighter_defs[] = {" + ",".join(f"&slf_{f.id}" for f in fighters) + "};")
    header.append(emit("slf_fighter_tiles", bases))
    header.append("static const uint16_t slf_fighter_pals[][2][16] = {" + ",".join(
        "{" + ",".join("{" + ",".join(str(v) for v in p) + "}" for p in pair) + "}" for pair in pals) + "};")
    header.append("static const char * const slf_fighter_origin[] = {" + ",".join(f'"{o}"' for o in origins) + "};")
    header.append("static const char * const slf_fighter_style[] = {" + ",".join(f'"{s}"' for s in styles) + "};")
    # Effects: 3 kinds x 4 frames, 32x32 (2x2 tiles).
    fx = place(np.vstack([spark(fr, kind) for kind in range(3) for fr in range(4)]))
    header.append(f"#define SLF_FX_TILE {fx}u")
    header.append("static const uint16_t slf_fx_pals[3][16] = {" + ",".join(
        "{" + ",".join(str(v) for v in palette_words(pal16(p))) + "}" for p in SPARK_PALS) + "};")
    sh = place(shadow())
    header.append(f"#define SLF_SHADOW_TILE {sh}u")
    header.append("static const uint16_t slf_shadow_pal[16] = {" + ",".join(
        str(v) for v in palette_words(pal16([(0, 0, 0), (52, 30, 18)]))) + "};")
    # Stage.
    rgba = stage()
    indices, banks, assignment = quantize(rgba, bank_limit=10)
    st = place(indices)
    header.append(f"#define SLF_STAGE_TILE {st}u\n#define SLF_STAGE_COLS 32u\n#define SLF_STAGE_ROWS 14u\n#define SLF_STAGE_W 512")
    header.append(f"#define SLF_PAL_STAGE {PAL_STAGE}u\n#define SLF_STAGE_BANKS {len(banks)}u")
    header.append(emit("slf_stage_map", assignment.flatten() + PAL_STAGE, "uint8_t"))
    header.append("static const uint16_t slf_stage_pal[][16] = {" + ",".join(
        "{" + ",".join(str(v) for v in palette_words(b)) + "}" for b in banks) + "};")
    Image.fromarray(np.dstack([reconstruct(indices, banks, assignment), np.full(indices.shape, 255, np.uint8)])).save(OUT / "stage.png")
    end = c1.tell()
    # Boot logo (bank $49), then the utility tiles.
    lo, hi = encode_image(logo_pixels(), 60)
    for s, lane in zip((c1, c2), (lo, hi)):
        if s.tell() < boot_start:
            s.write(bytes(boot_start - s.tell()))
        s.seek(boot_start)
        lane = lane[:14 * 64] + lane[15 * 64:]
        s.write(lane.ljust(256 * 64, b"\0"))
        s.seek(max(end, boot_end))
    write_utility_tiles(c1, c2)
    for s, part in zip((c1, c2), ("c1", "c2")):
        final = np.frombuffer(s.getvalue(), dtype=np.uint8).reshape(-1, 2)[:, ::-1].tobytes()
        if len(final) != 0x400000:
            raise ValueError("C ROM lane must be 4 MiB")
        for dest in (ART, ROMS):
            (dest / f"{GAME_ID}-{part}.{part}").write_bytes(final)
    print(f"C ROM: {end // 64} tiles used; stage {len(banks)} palettes; fighters {len(fighters)}")
    return header


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--fix-only", action="store_true")
    args = ap.parse_args()
    fix_defines = build_fix()
    if args.fix_only:
        return
    header = build()
    header += fix_defines
    header.append("static const uint16_t slf_fix_pals[][16] = {" + ",".join(
        "{" + ",".join(str(v) for v in palette_words(pal16(p))) + "}" for p in FIX_PALS) + "};")
    header += ["#endif", ""]
    (OUT / "slf_assets.h").write_text("\n".join(header), encoding="ascii")


if __name__ == "__main__":
    main()
