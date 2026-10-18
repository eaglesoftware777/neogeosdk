"""Build Shadow League's C1/C2 and S1 ROMs and its generated C tables.

C ROM (tiles, row-major per image): 256 blank tiles, then each fighter's
native frames (160x160, sparse shared 16x32 strips), the hit/guard/dust effects, the floor shadow,
four panoramic stages (512x224), the boot logo in bank $49 and the
utility tiles at $FFFD-$FFFF. S1: the system font, the game's text page,
life/power bar cells and large lettering.

    python3 games/shadowleague/tools/art.py [--fix-only]
"""

import argparse
import importlib.util
import io
import math
import re
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
import check_art_v2
import rebirth_assets
from strip_pack import StripPack
import json
import portrait_gen  # noqa: E402

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
                    image[y * scale:(y + 1) * scale, (i * 8 + x + 1 + max(0,(6-y)//3)) * scale:(i * 8 + x + 2 + max(0,(6-y)//3)) * scale] = 1
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


def stage_dojo_far():
    """Arata Mountain distant peaks and dawn sky."""
    h, w = 160, 512
    img = np.zeros((h, w, 3), np.float32)
    y = np.arange(h)[:, None] * np.ones((1, w))
    x = np.ones((h, 1)) * np.arange(w)[None, :]
    t = np.clip(y / 140.0, 0, 1)
    sky = (1 - t)[..., None] * np.array([70, 92, 150]) + t[..., None] * np.array([246, 170, 110])
    img[:] = sky
    sun = np.hypot(x - 300, y - 80) < 18
    img[sun] = (255, 226, 160)
    far = 60 + 26 * np.sin(x / 47.0) + 14 * np.sin(x / 19.0 + 1.3)
    near = 86 + 22 * np.sin(x / 61.0 + 2.0) + 9 * np.sin(x / 13.0)
    img[y > far] = (118, 104, 140)
    img[(y > far) & (y < far + 6) & (np.sin(x / 9.0) > 0.2)] = (220, 206, 220)
    img[y > near] = (64, 72, 98)
    trees = 110 + 6 * np.abs(np.sin(x / 5.0)) + 4 * np.sin(x / 23.0)
    img[y > trees] = (40, 58, 62)
    img = np.clip(img, 0, 255).astype(np.uint8)
    return np.dstack([img, np.full((h, w), 255, np.uint8)])

def stage_dojo_near():
    """Arata Mountain Dojo interior: beams, pillars, open/closed shoji, plank floor."""
    h, w = 224, 512
    img = np.zeros((h, w, 4), np.uint8)
    wall_top, floor_top = 40, 168
    y = np.arange(h)[:, None] * np.ones((1, w))
    x = np.ones((h, 1)) * np.arange(w)[None, :]
    img[:wall_top, :, :3] = (60, 38, 28)
    img[:wall_top, :, 3] = 255
    img[:wall_top][(np.arange(wall_top) % 10 < 2)] = (44, 28, 22, 255)
    img[wall_top - 6:wall_top] = (96, 62, 40, 255)
    img[wall_top:wall_top + 3] = (30, 20, 16, 255)
    for px in range(0, w + 1, 128):
        x0, x1 = px - 9, px + 9
        xs = slice(max(x0, 0), min(x1, w))
        img[wall_top:floor_top, xs] = (110, 70, 44, 255)
        img[wall_top:floor_top, max(x0, 0):max(x0 + 3, 0)] = (70, 44, 30, 255)
        img[wall_top:floor_top, min(x1 - 3, w):min(x1, w)] = (150, 100, 64, 255)
    for k, px in enumerate(range(0, w, 128)):
        if k in (0, 3):
            area = (slice(wall_top + 3, floor_top), slice(px + 9, px + 119))
            img[area] = (236, 226, 204, 255)
            for gx in range(px + 9, px + 119, 18):
                img[wall_top + 3:floor_top, gx:gx + 2] = (120, 84, 54, 255)
            for gy in range(wall_top + 3, floor_top, 22):
                img[gy:gy + 2, px + 9:px + 119] = (120, 84, 54, 255)
            if k == 0:
                img[60:128, px + 50:px + 78] = (226, 214, 180, 255)
                img[58:62, px + 46:px + 82] = (80, 50, 34, 255)
                img[126:130, px + 46:px + 82] = (80, 50, 34, 255)
                for sy in range(70, 120, 9):
                    img[sy:sy + 5, px + 60:px + 68] = (40, 30, 30, 255)
        else:
            img[wall_top + 3:wall_top + 8, px + 9:px + 119] = (96, 62, 40, 255)
    for lx in (128, 384):
        img[wall_top + 3:wall_top + 14, lx - 1:lx + 1] = (30, 20, 16, 255)
        lamp = ((x - lx) / 9) ** 2 + ((y - (wall_top + 26)) / 13) ** 2 < 1
        img[lamp] = (210, 60, 40, 255)
        img[lamp & (np.abs(x - lx) < 4)] = (250, 150, 80, 255)
        img[(np.abs(y - (wall_top + 14)) < 2) & (np.abs(x - lx) < 7)] = (40, 26, 18, 255)
        img[(np.abs(y - (wall_top + 39)) < 2) & (np.abs(x - lx) < 7)] = (40, 26, 18, 255)
    fy = y - floor_top
    floor = y >= floor_top
    plank = np.floor((x - 256) / (12 + fy * 0.55))
    shade = 0.82 + 0.18 * ((plank.astype(int) % 3) / 2.0)
    base = np.array([168, 112, 66]) * shade[..., None]
    grain = noise((h, w), 7, 4) * 14
    base = base + grain[..., None] - 7
    img[floor, :3] = np.clip(base[floor], 0, 255)
    img[floor, 3] = 255
    seam = floor & (np.abs(((x - 256) / (12 + fy * 0.55)) - np.round((x - 256) / (12 + fy * 0.55))) < 0.04)
    img[seam] = (96, 60, 34, 255)
    img[(y >= floor_top) & (y < floor_top + 3)] = (88, 56, 34, 255)
    glow = floor & (np.abs(x - 300 + (y - floor_top) * 0.6) < 40)
    img[glow, :3] = np.clip(img[glow, :3].astype(np.float32) * 1.12, 0, 255).astype(np.uint8)
    return img

def stage_bangkok_far():
    """Bangkok night skyline: deep indigo night sky, illuminated Buddhist spires and modern towers, stadium searchlights."""
    h, w = 160, 512
    img = np.zeros((h, w, 3), np.float32)
    y = np.arange(h)[:, None] * np.ones((1, w))
    x = np.ones((h, 1)) * np.arange(w)[None, :]
    t = np.clip(y / 150.0, 0, 1)
    sky = (1 - t)[..., None] * np.array([18, 14, 40]) + t[..., None] * np.array([44, 24, 72])
    img[:] = sky
    for beam_x, beam_angle in ((140, 0.4), (380, -0.4)):
        beam = np.abs((x - beam_x) - (160 - y) * beam_angle) < (8 + (160 - y) * 0.15)
        img[beam] = img[beam] * 0.7 + np.array([80, 110, 180]) * 0.3
    bldg_h = 70 + 25 * np.sin(x / 31.0) + 15 * np.cos(x / 11.0)
    img[y > bldg_h] = (24, 18, 38)
    spire = (np.abs(x - 256) < 3) & (y > 40)
    img[spire] = (240, 190, 60)
    windows = (y > bldg_h + 10) & ((x.astype(int) % 8 < 2) & (y.astype(int) % 12 < 3))
    img[windows] = (255, 220, 110)
    img = np.clip(img, 0, 255).astype(np.uint8)
    return np.dstack([img, np.full((h, w), 255, np.uint8)])

def stage_bangkok_near():
    """Bangkok night stadium: overhead trusses, stadium ring ropes, turnbuckles, cheering crowd, ring canvas."""
    h, w = 224, 512
    img = np.zeros((h, w, 4), np.uint8)
    wall_top, floor_top = 40, 168
    y = np.arange(h)[:, None] * np.ones((1, w))
    x = np.ones((h, 1)) * np.arange(w)[None, :]
    img[:wall_top, :, :3] = (32, 28, 48)
    img[:wall_top, :, 3] = 255
    img[:wall_top][(np.arange(wall_top) % 12 < 3)] = (20, 18, 32, 255)
    img[wall_top - 4:wall_top] = (60, 50, 80, 255)
    for lx in (100, 256, 412):
        lamp = ((x - lx) / 12) ** 2 + ((y - (wall_top + 10)) / 8) ** 2 < 1
        img[lamp] = (255, 240, 180, 255)
        img[lamp & (np.abs(x - lx) < 6)] = (255, 255, 230, 255)
    crowd_y = 120 + 8 * np.sin(x / 9.0) + 4 * np.cos(x / 4.0)
    crowd = (y > crowd_y) & (y < floor_top)
    img[crowd, :3] = (22, 18, 32)
    img[crowd, 3] = 255
    for ry, rope_col in ((floor_top - 44, (200, 30, 30)), (floor_top - 30, (230, 230, 230)), (floor_top - 16, (30, 80, 200))):
        img[ry:ry + 3, :, :3] = rope_col
        img[ry:ry + 3, :, 3] = 255
    for px in (32, 480):
        img[floor_top - 54:floor_top, px - 6:px + 6] = (80, 70, 100, 255)
        img[floor_top - 54:floor_top - 12, px - 4:px + 4] = (220, 40, 40, 255)
    canvas = y >= floor_top
    base = np.array([40, 60, 120]) + ((x - 256) / 256.0 * 10)[..., None]
    img[canvas, :3] = np.clip(base[canvas], 0, 255)
    img[canvas, 3] = 255
    img[(y >= floor_top) & (y < floor_top + 4)] = (210, 30, 30, 255)
    ring_circle = canvas & (np.hypot(x - 256, (y - floor_top - 28) * 1.8) < 80) & (np.hypot(x - 256, (y - floor_top - 28) * 1.8) > 74)
    img[ring_circle] = (240, 240, 250, 255)
    return img

def stage_brooklyn_far():
    """Brooklyn warehouse gym: dusk industrial skyline through grimy factory windows."""
    h, w = 160, 512
    img = np.zeros((h, w, 3), np.float32)
    y = np.arange(h)[:, None] * np.ones((1, w))
    x = np.ones((h, 1)) * np.arange(w)[None, :]
    t = np.clip(y / 140.0, 0, 1)
    sky = (1 - t)[..., None] * np.array([120, 50, 40]) + t[..., None] * np.array([210, 110, 50])
    img[:] = sky
    bridge = (np.abs(x - 180) < 6) | (np.abs(x - 330) < 6)
    img[bridge & (y > 30)] = (50, 30, 30)
    cable = (y > 40) & (np.abs((y - 40) - ((x - 255) / 100.0) ** 2 * 35) < 3)
    img[cable] = (60, 35, 35)
    factories = 95 + 16 * np.sin(x / 42.0) + 10 * np.cos(x / 17.0)
    img[y > factories] = (40, 24, 24)
    tower = (np.abs(x - 420) < 14) & (y > 70) & (y < 95)
    img[tower] = (45, 26, 26)
    img = np.clip(img, 0, 255).astype(np.uint8)
    return np.dstack([img, np.full((h, w), 255, np.uint8)])

def stage_brooklyn_near():
    """Brooklyn gym: weathered brick walls, steel pillars, hanging heavy bags, weathered canvas floor."""
    h, w = 224, 512
    img = np.zeros((h, w, 4), np.uint8)
    wall_top, floor_top = 40, 168
    y = np.arange(h)[:, None] * np.ones((1, w))
    x = np.ones((h, 1)) * np.arange(w)[None, :]
    img[:wall_top, :, :3] = (48, 36, 32)
    img[:wall_top, :, 3] = 255
    img[:wall_top][(np.arange(wall_top) % 10 < 2)] = (32, 22, 20, 255)
    wall = (y >= wall_top) & (y < floor_top)
    brick = ((y.astype(int) // 8) % 2) * 8
    mortar = ((x.astype(int) + brick) % 16 == 0) | (y.astype(int) % 8 == 0)
    img[wall, :3] = (140, 52, 42)
    img[wall & mortar, :3] = (80, 40, 34)
    img[wall, 3] = 255
    for px in range(0, w + 1, 128):
        img[wall_top:floor_top, px - 10:px + 10] = (60, 64, 72, 255)
        img[wall_top:floor_top, px - 3:px + 3] = (90, 96, 110, 255)
    for bx in (96, 352):
        img[wall_top:wall_top + 20, bx - 1:bx + 1] = (120, 120, 130, 255)
        bag = ((x - bx) / 10) ** 2 + ((y - (wall_top + 52)) / 30) ** 2 < 1
        img[bag] = (90, 36, 28, 255)
        img[bag & (x < bx - 3)] = (130, 54, 40, 255)
    canvas = y >= floor_top
    fy = y - floor_top
    base = np.array([130, 100, 75]) + ((fy * 0.4)[..., None])
    img[canvas, :3] = np.clip(base[canvas], 0, 255)
    img[canvas, 3] = 255
    img[(y >= floor_top) & (y < floor_top + 3)] = (60, 42, 32, 255)
    return img

def stage_chimera_far():
    """Project Chimera facility: subterranean server mainframe, cryogenic stasis pods with fighter silhouettes."""
    h, w = 160, 512
    img = np.zeros((h, w, 3), np.float32)
    y = np.arange(h)[:, None] * np.ones((1, w))
    x = np.ones((h, 1)) * np.arange(w)[None, :]
    t = np.clip(y / 150.0, 0, 1)
    sky = (1 - t)[..., None] * np.array([8, 24, 32]) + t[..., None] * np.array([14, 52, 64])
    img[:] = sky
    racks = (x.astype(int) % 48 < 36) & (y > 30)
    img[racks] = (18, 36, 44)
    leds = racks & (y > 45) & ((x.astype(int) % 12 < 3) & (y.astype(int) % 8 < 2))
    img[leds] = (40, 240, 210)
    for pod_x in (120, 256, 392):
        pod = (np.abs(x - pod_x) < 16) & (y > 40) & (y < 130)
        img[pod] = (20, 100, 120)
        glow = (np.abs(x - pod_x) < 12) & (y > 46) & (y < 124)
        img[glow] = (60, 220, 240)
        subject = glow & (((x - pod_x) ** 2 / 20.0 + (y - 85) ** 2 / 500.0) < 1)
        img[subject] = (10, 40, 50)
    img = np.clip(img, 0, 255).astype(np.uint8)
    return np.dstack([img, np.full((h, w), 255, np.uint8)])

def stage_chimera_near():
    """Project Chimera cybernetic facility: metallic catwalks, hazard yellow/black stripes, hexagonal floor, conduits."""
    h, w = 224, 512
    img = np.zeros((h, w, 4), np.uint8)
    wall_top, floor_top = 40, 168
    y = np.arange(h)[:, None] * np.ones((1, w))
    x = np.ones((h, 1)) * np.arange(w)[None, :]
    img[:wall_top, :, :3] = (24, 32, 40)
    img[:wall_top, :, 3] = 255
    img[:wall_top][(np.arange(wall_top) % 8 < 2)] = (14, 20, 26, 255)
    for cy in (16, 28):
        img[cy:cy + 2, :, :3] = (30, 200, 230)
    for hx in range(0, w, 16):
        stripe = (y >= wall_top - 6) & (y < wall_top) & (((x - hx + y) % 16) < 8)
        img[stripe, :3] = (230, 190, 20)
    for px in range(0, w + 1, 128):
        img[wall_top:floor_top, px - 12:px + 12] = (36, 48, 58, 255)
        img[wall_top:floor_top, px - 4:px + 4] = (60, 190, 220, 255)
    canvas = y >= floor_top
    fy = y - floor_top
    base = np.array([20, 28, 36]) + ((fy * 0.3)[..., None])
    img[canvas, :3] = np.clip(base[canvas], 0, 255)
    img[canvas, 3] = 255
    circuits = canvas & (((x.astype(int) % 32 == 0) & (fy % 16 < 12)) | (fy % 16 == 0))
    img[circuits, :3] = (40, 210, 240)
    img[(y >= floor_top) & (y < floor_top + 3)] = (30, 200, 230, 255)
    return img

STAGES = [
    # (master painting, name on the VS screen, home-stage keywords)
    ("dojo", "ARATA MOUNTAIN DOJO - JAPAN", ("ARATA", "MUNICIPAL DOJO")),
    ("bangkok", "BANGKOK NIGHT STADIUM - THAILAND", ("BANGKOK", "TEMPLE", "VIENTIANE")),
    ("brooklyn", "BROOKLYN UNDERGROUND GYM - USA", ("BROOKLYN", "NATIONAL TRAINING")),
    ("chimera", "PROJECT CHIMERA FACILITY - THE VEIL", ("CHIMERA", "VEIL")),
    ("mexico", "ARENA CORONA - MEXICO CITY", ("MEXICO",)),
    ("shanghai", "PUDONG ROOFTOP RING - SHANGHAI", ("SHANGHAI", "SEOUL")),
    ("london", "THE ARCHES FIGHT CLUB - LONDON", ("LONDON", "BARCELONA", "MARSEILLE", "MEDITERRANEAN", "CASABLANCA")),
    ("africa", "HARVEST FESTIVAL RING - ACCRA", ("ACCRA", "JOHANNESBURG", "WEST AFRICAN", "COASTAL")),
    ("salvador", "PELOURINHO SQUARE - SALVADOR", ("SALVADOR", "RAINFOREST", "BEACHSIDE")),
    ("steppe", "NAADAM GROUNDS - MONGOLIA", ("MONGOLIAN", "SAND", "TRADITIONAL FIELD", "AKHARA")),
    ("frozen", "ICEBOUND HARBOR - MURMANSK", ("FROZEN", "LODGE", "NORTHERN", "MOUNTAIN COMPOUND")),
    ("docks", "PIER 9 CONTAINER YARD - MANILA", ("MANILA", "WAREHOUSE")),
]
STAGE_BANKS = 16


def stage_idx_for_fighter(f):
    st = f.data.get("stage", "").upper()
    for i, (_, _, keys) in enumerate(STAGES):
        if any(k in st for k in keys):
            return i
    return 0


# --------------------------------------------------------------------------
#  FIX
# --------------------------------------------------------------------------
def fix_rom():
    rom = bytearray(font.build_rom())
    lo = GAME_FIX_BASE * 32
    rom[lo:lo + 256 * 32] = rom[:256 * 32]
    # The game's text page uses its own shaded face; the system font stays below for the BIOS.
    import slf_font
    for code, glyph in slf_font.glyph_tiles().items():
        rom[lo + code * 32:lo + (code + 1) * 32] = font.encode_tile(glyph)
    # The tall face (two tiles a character) for headings, names and menus.
    import slf_tall_font
    for number, glyph in slf_tall_font.glyph_tiles().items():
        rom[number * 32:(number + 1) * 32] = font.encode_tile(glyph)
    rom[0xFF * 32:0x100 * 32] = bytes(32)
    return rom


BIG_TEXT = [("TITLE1", "SHADOW", 3), ("TITLE1B", "LEAGUE", 3), ("TITLE2", "FIGHTING", 2), ("ROUND1", "ROUND 1", 2),
            ("ROUND2", "ROUND 2", 2), ("ROUND3", "ROUND 3", 2), ("FINAL", "FINAL ROUND", 2), ("FIGHT", "FIGHT!", 3),
            ("KO", "K.O.", 3), ("TIMEUP", "TIME UP", 2), ("WIN", "WINS", 2), ("DRAW", "DRAW", 2),
            ("VS", "VS", 3), ("PERFECT", "PERFECT", 2), ("GAMEOVER", "GAME OVER", 2), ("COUNTER", "COUNTER", 1),
            ("CONGRATS", "CONGRATULATIONS", 2), ("DIGITS", "0123456789", 2),
            ("CHALLENGER", "CHALLENGER!", 2)]
BIG_TEXT.append(("GUARDBREAK", "GUARD BREAK", 1))
NATIVE_WORDS = {"ROUND1", "ROUND2", "ROUND3", "FINAL", "FIGHT", "KO", "TIMEUP", "WIN", "DRAW",
                "VS", "PERFECT", "GAMEOVER", "COUNTER", "CONGRATS", "CHALLENGER", "GUARDBREAK"}


# FIX palettes: 0 text, 1 gold text, 2 dim text, 3 HUD bars, 4 gold lettering,
# 5 red lettering, 6 blue lettering, 7 white lettering. Entry 1 is the glyph
# ink (top half for lettering), 2 its shadow, 3 the lower half.
FIX_PALS = [
    [(0, 0, 0), (250, 250, 250), (16, 16, 24), (190, 196, 210)],
    [(0, 0, 0), (255, 220, 90), (40, 20, 0), (230, 160, 40)],
    [(0, 0, 0), (150, 150, 170), (16, 16, 24), (110, 110, 130)],
    [(0, 0, 0), (29, 36, 56), (255, 232, 96), (240, 156, 40), (44, 18, 30), (232, 44, 40),
     (149, 105, 231), (250, 215, 113), (179, 202, 219), (91, 108, 139)],
    [(0,0,0),(20,10,8),(255,240,176),(236,164,48),(160,80,32),(80,32,24),(255,255,224),(48,24,32)],
    [(0,0,0),(24,4,8),(255,184,136),(232,64,64),(144,32,56),(64,16,32),(255,240,208),(40,16,32)],
    [(0,0,0),(4,8,28),(192,232,255),(64,152,224),(32,72,144),(16,32,72),(240,248,255),(16,24,48)],
    [(0,0,0),(12,12,20),(248,248,240),(152,176,192),(80,112,136),(40,56,80),(255,255,248),(24,32,48)],
]
import slf_tall_font  # noqa: E402
FIX_PALS += [slf_tall_font.PALETTES[k] for k in ('GOLD', 'SILVER', 'RED', 'BLUE')]   # 8-11


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
            for trail in range(9):
                g = np.full((8, 8), 4, np.uint8)
                g[0] = 8;g[1] = 1;g[6] = 1;g[7] = 9
                for c in (range(trail) if side == "L" else range(8-trail,8)):g[2:6,c]=5
                for c in (range(level) if side == "L" else range(8-level,8)):
                    g[2:4,c]=2;g[4:6,c]=3
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
        g[1] = 8
        g[6] = 1
        for c in range(level):
            g[2:6, c] = 6 if level < 8 else 7
        put(g)
    defines.append(f"#define SLF_FIX_POWER {first}u")
    for kind,fill in [('GUARD',8),('GUARD_ALERT',5),('SPIRIT',6)]:
        for side in ('L','R'):
            first=tile
            for level in range(9):
                g=np.zeros((8,8),np.uint8);g[2]=9;g[3:5]=1;g[5]=1
                for c in (range(level) if side=='L' else range(8-level,8)):g[3:5,c]=fill
                put(g)
            defines.append(f'#define SLF_FIX_{kind}_{side} {first}u')
    # Round-win marker: empty and won.
    for name, fill in (("MARK_OFF", 4), ("MARK_ON", 2)):
        g = np.zeros((8, 8), np.uint8)
        ys, xs = np.mgrid[0:8, 0:8]
        disk = np.abs(xs - 3.5) + np.abs(ys - 3.5)
        g[disk < 3.6] = 1
        g[disk < 2.6] = fill
        defines.append(f"#define SLF_FIX_{name} {put(g)}u")
    # Large lettering, in the free tiles between the system font and the text page.
    bars_end = tile
    tile = FIX_BIG
    for name, text, scale in BIG_TEXT:
        if name in NATIVE_WORDS or name=='DIGITS':
            from native_art_io import indexed
            folder='hud' if name=='DIGITS' else 'announcements'
            g, _ = indexed(GAME / "assets/authored/rebirth" / folder / (name.lower()+".png"))
            if g.max()>7 or any(n%8 for n in g.shape) or g.shape[1]>304:
                raise ValueError(name+": invalid native FIX artwork")
            hgt,wid=g.shape
        else:
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
        utility = 0xFFFD * 64
        if c1.tell() < 0x10000 * 64 and c1.tell() + count * 64 > utility:
            # the utility tiles ($FFFD-$FFFF) go in last, at their fixed place
            for s in (c1, c2):
                s.write(bytes(0x10000 * 64 - s.tell()))
        if count == 2 and (c1.tell() // 64) & 0xFFFF == 0xFFFF:
            # a two-tile strip's second tile is reached by a 16-bit add
            for s in (c1, c2):
                s.write(bytes(64))
        tile = c1.tell() // 64
        a, b = encode_image(indexed, count)
        c1.write(a)
        c2.write(b)
        return tile

    for s in (c1, c2):
        s.write(bytes(256 * 64))
    header = ["/* Generated by games/shadowleague/tools/art.py. */", "#ifndef SLF_ASSETS_H", "#define SLF_ASSETS_H",
              "#include <stdint.h>", '#include "sdk/2d_engine/ng_fight.h"', "typedef struct {uint8_t slot, pal_bits; uint16_t tile;} SLFStripRef; typedef struct {uint8_t x,y;} SLFFrameAnchor;"]
    fighters = fighter_cook.load_all()
    order = [
        "kenji", "somchai", "amara", "jack", "claire", "viktor", "linwei", "batu",
        "minjun", "malik", "idris", "arun", "ryuhei", "diego", "niran", "chenbao",
        "aisha", "kiran", "elena", "fahri", "maya", "aditya", "sokha", "lucas",
        "noah", "zara", "thabo", "maia", "tomas", "leila", "kaimana", "aleksei",
        "hayato", "mason", "lotus", "tororojo", "irina", "kofi", "xiaomei", "zero",
        "echo", "atlas", "viper", "mirage"
    ]
    fighters.sort(key=lambda f: order.index(f.id) if f.id in order else 99)
    # Fighter Select portraits (32x32, 2x2 tiles each) and cursor bracket frame
    port_imgs = []
    for f in fighters:
        if f.authored is not None:
            from native_art_io import indexed
            manifest=json.loads((f.authored_folder/'manifest.json').read_text())
            portrait,_=indexed(f.authored_folder/manifest['portrait_small'],(32,32))
            port_imgs.append(portrait)
        else:port_imgs.append(portrait_gen.generate_portrait(f.id))
    cur_img = portrait_gen.generate_cursor_frame()
    port_block = np.vstack(port_imgs + [cur_img])
    port_tile = place(port_block)
    header.append(f"#define SLF_PORTRAIT_TILE {port_tile}u")
    header.append(f"#define SLF_CURSOR_TILE {port_tile + len(fighters) * 4}u")
    # The select screen's wall of faces (palettes 32-47: the stage is not up there)
    import select_wall
    ready = [int(f.authored is not None) for f in fighters]
    wall_lines, wall_preview = select_wall.build([f.id for f in fighters], place, emit, PAL_STAGE, ready)
    header.extend(wall_lines)
    Image.fromarray(wall_preview).save(OUT / "select-wall.png")
    cursor_pals = [
        pal16([(0, 0, 0)] + [(0, 0, 0)] * 13 + [(255, 255, 255), (255, 60, 60)]),
        pal16([(0, 0, 0)] + [(0, 0, 0)] * 13 + [(255, 255, 255), (60, 180, 255)]),
    ]
    header.append("static const uint16_t slf_cursor_pals[2][16] = {" + ",".join(
        "{" + ",".join(str(v) for v in palette_words(p)) + "}" for p in cursor_pals) + "};")
    # Effects and the projectile, painted: 48x48 frames, a palette a kind.
    import painted_fx
    import colorsys
    pfx = painted_fx.frames()
    header.append(f"#define SLF_PROJECTILE_TILE {place(np.vstack(pfx['projectile'][0]))}u")
    proj = pfx['projectile'][1]
    blue = np.array([[int(v * 255) for v in colorsys.hsv_to_rgb((colorsys.rgb_to_hsv(*(c / 255.0))[0] + 0.6) % 1.0,
                     *colorsys.rgb_to_hsv(*(c / 255.0))[1:])] for c in proj], np.uint8)
    header.append("static const uint16_t slf_proj_pals[2][16] = {" + ",".join(
        "{" + ",".join(str(v) for v in palette_words(p)) + "}" for p in (proj, blue)) + "};")
    fx = place(np.vstack([f for kind in ('hit', 'guard', 'dust', 'fire', 'super') for f in pfx[kind][0]]))
    header.append(f"#define SLF_FX_TILE {fx}u")
    guide_rows=[]
    for fighter in fighters:
        guide=[];seen=set()
        for move in fighter.data['moves']:
            if move['kind'] not in ('special','super') or not move.get('buttons') or move['name'] in seen:continue
            seen.add(move['name'])
            motion={'qcf':'D,DF,F','qcb':'D,DB,B','dp':'F,D,DF','dqcf':'QCF X2','dqcb':'QCB X2','360':'360','720':'720','charge_bf':'HOLD B,F','hcf':'B,DB,D,DF,F','hcb':'F,DF,D,DB,B'}.get(move.get('motion'),move.get('motion',''))
            text=(motion+'+'+move['buttons']+': '+move['name'])[:36]
            guide.append('"'+text.replace('"','')+'"')
        guide=guide[:6]+['0']*max(0,6-len(guide))
        guide_rows.append('{'+','.join(guide)+'}')
    header.append('static const char * const slf_command_guides[][6] = {'+','.join(guide_rows)+'};')

    header.append('static const uint8_t slf_fighter_fx[] = {'+','.join('1' if f.id in ('arun','zero') else '0' for f in fighters)+'};')
    header.append("static const uint16_t slf_fx_pals[5][16] = {" + ",".join(
        "{" + ",".join(str(v) for v in palette_words(pfx[k][1])) + "}" for k in ('hit', 'guard', 'dust', 'fire', 'super')) + "};")
    sh = place(shadow())
    header.append(f"#define SLF_SHADOW_TILE {sh}u")
    header.append("static const uint16_t slf_shadow_pal[16] = {" + ",".join(
        str(v) for v in palette_words(pal16([(0, 0, 0), (52, 30, 18)]))) + "};")
    identity_packed = []
    # Arenas: one painted 512x224 panorama each, sixteen palettes chosen
    # tile by tile. (The far layer is not drawn: its tables name the near art.)
    st_near_tiles, st_far_tiles, st_maps, st_far_maps, st_pals = [], [], [], [], []
    st_dark_pals = []
    for s_idx, (s_name, s_desc, _) in enumerate(STAGES):
        authored = GAME / "assets/authored" / (s_name + "-master.png")
        if not authored.exists():raise ValueError("Missing arena artwork: " + s_name)
        master = Image.open(authored).convert("RGBA")
        if master.size != (512, 224):
            master = master.resize((512, 224), Image.Resampling.LANCZOS)
        near_rgba = np.asarray(master)
        indices, banks, assignment = quantize(near_rgba, bank_limit=STAGE_BANKS)
        st_near = place(indices)
        st_near_tiles.append(st_near)
        st_far_tiles.append(st_near)
        st_maps.append(assignment.flatten() + PAL_STAGE)
        st_far_maps.append(assignment[:10].flatten() + PAL_STAGE)
        bank_pals = [palette_words(b) for b in banks]
        dark_pals = [palette_words((np.asarray(b,dtype=np.float32)*.38).astype(np.uint8)) for b in banks]
        while len(dark_pals) < STAGE_BANKS:dark_pals.append([0]*16)
        st_dark_pals.append(dark_pals)
        while len(bank_pals) < STAGE_BANKS:
            bank_pals.append([0] * 16)
        st_pals.append(bank_pals)
        if s_idx == 0:
            far_rgba = near_rgba
    n = len(STAGES)

    header.append(f"#define SLF_STAGE_COUNT {n}u\n#define SLF_STAGE_COLS 32u\n#define SLF_STAGE_ROWS 14u\n#define SLF_STAGE_W 512")
    header.append(f"#define SLF_STAGE_FAR_ROWS 10u\n#define SLF_PAL_STAGE {PAL_STAGE}u\n#define SLF_STAGE_BANKS {STAGE_BANKS}u")
    header.append(emit("slf_stage_tiles", st_near_tiles))
    header.append(emit("slf_stage_far_tiles", st_far_tiles))
    for s_idx in range(n):
        header.append(emit(f"slf_stage_map_{s_idx}", st_maps[s_idx], "uint8_t"))
        header.append(emit(f"slf_stage_far_map_{s_idx}", st_far_maps[s_idx], "uint8_t"))
    header.append(f"static const uint8_t * const slf_stage_maps[{n}] = {{" + ",".join(f"slf_stage_map_{i}" for i in range(n)) + "};")
    header.append(f"static const uint8_t * const slf_stage_far_maps[{n}] = {{" + ",".join(f"slf_stage_far_map_{i}" for i in range(n)) + "};")
    header.append(f"static const uint16_t slf_stage_pals[{n}][{STAGE_BANKS}][16] = {{" + ",".join(
        "{" + ",".join("{" + ",".join(str(v) for v in b) + "}" for b in p) + "}" for p in st_pals) + "};")
    header.append(f"static const char * const slf_stage_names[{n}] = {{" + ",".join(f'"{d[1]}"' for d in STAGES) + "};")
    header.append(f"static const uint16_t slf_stage_dark_pals[{n}][{STAGE_BANKS}][16] = {{" + ",".join(
        "{" + ",".join("{" + ",".join(str(v) for v in bank) + "}" for bank in stage) + "}" for stage in st_dark_pals) + "};")
    header.append("#define SLF_STAGE_TILE slf_stage_tiles[0]\n#define SLF_STAGE_FAR_TILE slf_stage_far_tiles[0]")
    header.append("#define slf_stage_map slf_stage_maps[0]\n#define slf_stage_far_map slf_stage_far_maps[0]")
    header.append("#define slf_stage_pal slf_stage_pals[0]")
    comp = np.array(far_rgba, copy=True)
    comp[:, :, 3] = 255
    Image.fromarray(comp).save(OUT / "stage.png")
    # The fighters after the shared art: every other picture keeps a 16-bit
    # tile number, and the fighters' strips carry their high address bits.
    bases, pals, names, styles, origins = [], [], [], [], []
    bank_pals, bank_offsets, bank_counts = [], [], []
    strips = StripPack(place)
    strip_tables = []
    offset_tables = []
    anchor_tables = []
    fighter_layout = []
    for f in fighters:
        maps = [strips.frame(image, f.tile_banks[i] if f.authored is not None else None) for i,image in enumerate(f.images)]
        fighter_layout.append({"id":f.id,"canvas":f.canvas,"anchor":f.anchor,"frames":[{"name":name,"strips":m} for name,m in zip(f.sprite_names,maps)]})
        roots = [fr.get('root',f.anchor) for fr in f.authored] if f.authored is not None else [f.anchor]*len(f.images)
        anchor_name='slf_'+f.id+'_frame_roots'
        header.append('static const SLFFrameAnchor '+anchor_name+'[] = {'+','.join('{%du,%du}'%tuple(root) for root in roots)+'};')
        anchor_tables.append(anchor_name)
        map_name = 'slf_' + f.id + '_strip_map'
        entries=[];offsets=[]
        for frame_map in maps:
            offsets.append(len(entries));entries.extend(frame_map);entries.append((255,0,0))
        header.append("static const SLFStripRef " + map_name + "[] = {" + ",".join("{%du,%du,%du}" % (item[0],item[2],item[1]) for item in entries) + "};")
        off_name="slf_"+f.id+"_strip_offsets"
        header.append(emit(off_name,offsets));offset_tables.append(off_name)
        strip_tables.append(map_name)
        bases.append(maps[0][0][1])
        normal_banks, alt_banks = f.palette_banks()
        bank_offsets.append(len(bank_pals));bank_counts.append(len(normal_banks))
        bank_pals.extend([[palette_words(n),palette_words(a)] for n,a in zip(normal_banks,alt_banks)])
        normal, alt = f.palettes()
        pals.append([palette_words(normal), palette_words(alt)])
        header.append(f.c_tables(f"slf_{f.id}"))
        names.append(f.data["name"])
        styles.append(f.data.get("discipline", f.data.get("style", "")))
        origins.append(f.data.get("origin", ""))
        f.sheet(OUT / f"{f.id}_sheet.png")
    header.append(f"#define SLF_FIGHTER_COUNT {len(fighters)}u")
    header.append("static const SLFStripRef * const slf_fighter_strip_maps[] = {" + ",".join(strip_tables) + "};")
    header.append("static const uint16_t * const slf_fighter_strip_offsets[] = {" + ",".join(offset_tables) + "};")
    header.append("static const NGFFighterDef * const slf_fighter_defs[] = {" + ",".join(f"&slf_{f.id}" for f in fighters) + "};")
    header.append(emit("slf_fighter_tiles", bases))
    header.append("static const uint16_t slf_fighter_pals[][2][16] = {" + ",".join(
        "{" + ",".join("{" + ",".join(str(v) for v in p) + "}" for p in pair) + "}" for pair in pals) + "};")
    header.append(emit("slf_fighter_bank_offsets",bank_offsets))
    header.append(emit("slf_fighter_bank_counts",bank_counts,"uint8_t"))
    header.append(emit("slf_fighter_columns",[f.canvas[0]//16 for f in fighters],"uint8_t"))
    header.append('static const SLFFrameAnchor * const slf_fighter_frame_anchors[] = {'+','.join(anchor_tables)+'};')
    header.append(emit("slf_fighter_anchor_x",[f.anchor[0] for f in fighters],"uint8_t"))
    header.append(emit("slf_fighter_anchor_y",[f.anchor[1] for f in fighters],"uint8_t"))
    header.append("static const uint16_t slf_fighter_bank_pals[][2][16] = {" + ",".join(
        "{" + ",".join("{" + ",".join(str(v) for v in p) + "}" for p in pair) + "}" for pair in bank_pals) + "};")
    header.append("static const char * const slf_fighter_origin[] = {" + ",".join(f'"{o}"' for o in origins) + "};")
    header.append("static const char * const slf_fighter_style[] = {" + ",".join(f'"{s}"' for s in styles) + "};")
    fighter_stages = [stage_idx_for_fighter(f) for f in fighters]
    header.append(emit("slf_fighter_stages", fighter_stages, "uint8_t"))
    # Fighters still waiting for their painted set stay locked on the select
    # wall and out of the CPU line-ups.
    header.append(emit("slf_fighter_ready", [int(f.authored is not None) for f in fighters], "uint8_t"))
    # Voices (slf_voices.py): the shout for each move, and each fighter's voice type.
    import slf_voices
    voice_plan = slf_voices.plan(slf_voices.load_fighters())
    header.append(f"#define SLF_SND_ANNOUNCER {slf_voices.announcer_base(voice_plan)}u")
    header.append(f"#define SLF_SND_TYPES {slf_voices.type_base(voice_plan)}u")
    header.append(emit("slf_voice_type", [slf_voices.CAST[f.id]["type"] for f in fighters], "uint8_t"))
    move_voice, move_voice_ofs = [], []
    for f in fighters:
        move_voice_ofs.append(len(move_voice))
        move_voice.extend(slf_voices.move_samples(f.data, voice_plan))
    header.append(emit("slf_move_voice_ofs", move_voice_ofs))
    header.append(emit("slf_move_voice", move_voice, "uint8_t"))
    identity_tables, high_packed = rebirth_assets.tables(place)
    header.extend(identity_tables);identity_packed.extend(high_packed)
    end = c1.tell()
    # Boot logo (bank $49), then the utility tiles at $FFFD-$FFFF (place()
    # stepped over both when the art ran past them).
    lo, hi = encode_image(logo_pixels(), 60)
    for s, lane in zip((c1, c2), (lo, hi)):
        if s.tell() < boot_start:
            s.write(bytes(boot_start - s.tell()))
        s.seek(boot_start)
        lane = lane[:14 * 64] + lane[15 * 64:]
        s.write(lane.ljust(256 * 64, b"\0"))
        s.seek(max(end, boot_end))
    utility = 0xFFFD * 64
    if c1.tell() <= utility:
        write_utility_tiles(c1, c2)
    else:
        top = c1.tell()
        for s in (c1, c2):
            s.seek(utility)
        write_utility_tiles(c1, c2)
        for s in (c1, c2):
            s.seek(top)
    end = c1.tell()
    if end > 0x800000:raise ValueError('C-ROM lane exceeds the 8 MiB cartridge capacity')
    for stream in (c1,c2):stream.write(bytes(0x800000-stream.tell()))
    for s, part in zip((c1, c2), ("c1", "c2")):
        final = np.frombuffer(s.getvalue(), dtype=np.uint8).reshape(-1, 2)[:, ::-1].tobytes()
        if len(final) != 0x800000:
            raise ValueError("C ROM lane must be 8 MiB")
        for dest in (ART, ROMS):
            (dest / f"{GAME_ID}-{part}.{part}").write_bytes(final)
    (OUT / "fighter-layout.json").write_text(json.dumps(fighter_layout,indent=2)+"\n")
    check_art_v2.check(fighters,fighter_layout,(ART / "781-c1.c1").read_bytes(),(ART / "781-c2.c2").read_bytes())
    rebirth_assets.roundtrip(identity_packed,check_art_v2.decode_tiles((ART / "781-c1.c1").read_bytes(),(ART / "781-c2.c2").read_bytes()))
    report = strips.report(); report["total_tiles"] = end // 64
    (OUT / "strip-budget.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"C ROM: {end // 64} tiles used; stage {len(banks)} palettes; fighters {len(fighters)}; {report}")
    return header


def fix_palette_definition():
    return "const uint16_t slf_fix_pals[][16] = {" + ",".join(
        "{" + ",".join(str(v) for v in palette_words(pal16(p))) + "}" for p in FIX_PALS) + "};"


def update_fix_tables(defines):
    path=OUT / "slf_assets.h"
    if not path.exists():
        raise ValueError("Build the complete asset tables before a FIX-only export")
    text=path.read_text()
    text=re.sub(r"^#define SLF_(?:BIG|FIX)_[A-Z0-9_]+[^\n]*\n", "", text, flags=re.M)
    text=text.replace("#ifdef SLF_ASSETS_IMPLEMENTATION", "\n".join(defines)+"\n#ifdef SLF_ASSETS_IMPLEMENTATION",1)
    text,count=re.subn(r"const uint16_t slf_fix_pals\[\]\[16\] = \{[^;]+;",fix_palette_definition(),text,count=1)
    if count!=1:raise ValueError("Missing FIX palette implementation")
    path.write_text(text,encoding="ascii")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--fix-only", action="store_true")
    args = ap.parse_args()
    fix_defines = build_fix()
    if args.fix_only:
        update_fix_tables(fix_defines)
        return
    header = build()
    header += fix_defines
    header.append("static "+fix_palette_definition())
    public_names = ("slf_fighter_defs", "slf_fighter_strip_maps", "slf_fighter_strip_offsets",
                    "slf_fighter_pals", "slf_fighter_origin", "slf_fighter_style", "slf_fighter_stages", "slf_fighter_ready", "slf_fighter_columns", "slf_fighter_frame_anchors", "slf_fighter_anchor_x", "slf_fighter_anchor_y", "slf_voice_type", "slf_move_voice_ofs", "slf_move_voice",
                    "slf_cursor_pals", "slf_command_guides", "slf_fighter_fx", "slf_fx_pals", "slf_proj_pals", "slf_shadow_pal", "slf_stage_pals",
                    "slf_stage_far_maps", "slf_stage_maps", "slf_stage_far_tiles", "slf_stage_tiles",
                    "slf_stage_names", "slf_fix_pals", "slf_stage_dark_pals",
                    "slf_rebirth_identity_pal", "slf_rebirth_portrait_tiles", "slf_rebirth_portrait_pals",
                    "slf_hud_plate_pal",
                    "slf_fighter_bank_offsets", "slf_fighter_bank_counts", "slf_fighter_bank_pals",
                    "slf_presentation_offsets", "slf_wall_map", "slf_wall_pals", "slf_wall_row_len",
                    "slf_wall_row_start", "slf_wall_row_off", "slf_face_pals", "slf_face_maps")
    macros=[item for item in header[6:] if item.startswith("#define ")]
    body="\n".join(item for item in header[6:] if not item.startswith("#define "))
    declarations=[]
    for name in public_names:
        match=re.search(r"static (const [^;\n{]+?\b"+name+r"(?:\[[^\]]*\])*)\s*=",body)
        if not match:raise ValueError("Missing public asset table: "+name)
        decl=match.group(1)
        declarations.append("extern "+decl+";")
        body=body[:match.start()]+body[match.start():].replace("static ","",1)
    result=header[:6]+macros+["#ifdef SLF_ASSETS_IMPLEMENTATION",body,"#else"]+declarations+["#endif","#endif",""]
    (OUT / "slf_assets.h").write_text("\n".join(result),encoding="ascii")


if __name__ == "__main__":
    main()
