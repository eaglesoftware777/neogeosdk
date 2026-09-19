"""Build the commercial-grade Neo Geo arcade assets for Maiya: Super Nature Girl.

    python3 games/maiya/tools/build_commercial_assets.py

Processes the high-definition Neo Geo arcade pixel art from
games/maiya/assets/source_art/:
- Maiya's full 39-frame moveset with rose whip, rose thorns, and victory poses
- 6 distinct corrupted blight enemies (Smog Slime, Rust Beetle, Oil Crow,
  Trash Goblin, Pipe Worm, Blight Patrol Drone)
- The 7 massive biomechanical guardians / bosses (Chainsaw Beetle, Sludge Toad,
  Iron Vulture, Inferno Jackal, Blizzard Owl, Toxic Leviathan, Lord Smoggar)
- Allies & NPCs (Sunboy Prince of Nature, Nature Spirit Fairy, Village Elder, Valley Girl)
- 6 rich multi-layer stage environments (Greenroot Forest, Mudriver Marsh,
  Smokestack City, Burning Drylands, Frozen Sky Peaks, Black Core Citadel)
- Pick-ups, chests, props, hazards, tools, and the arcade title screen
- Encodes the C-ROM pair (780-c1.c1, 780-c2.c2) and generates maiya_assets.h
"""

from __future__ import annotations

import colorsys
import io
import json
from pathlib import Path
import sys

import numpy as np
from PIL import Image
import scipy.ndimage as ndi

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "artbox"))

from palette_banks import fit_palette, palette_words, quantize, reconstruct, training_mask  # noqa: E402
from tile_codec import encode_image, decode_image, write_utility_tiles  # noqa: E402

SOURCE = GAME / "assets/source_art"
CHARACTERS = ROOT / "games/demo/artbox/in/characters"

HERO_CANVAS = (112, 96)
HERO_STRIPS = 7
HERO_ROWS = 6
HERO_STRIDE = 7

EAGLE_CANVAS = (64, 48)
EAGLE_STRIPS = 4
EAGLE_ROWS = 3

BOSS_CANVAS = (128, 128)
BOSS_STRIPS = 8
BOSS_ROWS = 8
BOSS_STRIDE = 8

# Tool tiles (16 columns, 32 rows total = 2 rows of 16x16 tiles)
TOOL_COLUMNS = 16
TOOL_THORN0, TOOL_THORN1, TOOL_TRASH, TOOL_SPIT, TOOL_BOLT, TOOL_FIRE, TOOL_ICE, TOOL_OIL = range(8)
TOOL_SPARK, TOOL_HEART, TOOL_ROSE, TOOL_PETAL, TOOL_LANE, TOOL_CURSOR, TOOL_DRIP, TOOL_LEAF = range(8, 16)


def c_array(name, values, ctype="uint16_t"):
    values = list(values)
    lines = [f"static const {ctype} {name}[{len(values)}] = {{"]
    for i in range(0, len(values), 16):
        lines.append("    " + ", ".join(str(int(v)) for v in values[i:i + 16]) + ",")
    return "\n".join(lines + ["};", ""])


def find_file(pattern):
    matches = list(SOURCE.glob(pattern))
    if not matches:
        raise FileNotFoundError(f"Missing {pattern} in {SOURCE}")
    return matches[0]


def extract_cutout(img, bg_color="white", tol=26):
    """Extract foreground sprite by flood-filling border background pixels."""
    arr = np.array(img.convert("RGB"))
    h, w = arr.shape[:2]
    if bg_color == "white":
        is_bg = (arr[:, :, 0] >= 255 - tol) & (arr[:, :, 1] >= 255 - tol) & (arr[:, :, 2] >= 255 - tol)
    else:
        is_bg = (arr[:, :, 0] <= tol) & (arr[:, :, 1] <= tol) & (arr[:, :, 2] <= tol)
    labeled, num = ndi.label(is_bg)
    border = np.concatenate([labeled[0, :], labeled[-1, :], labeled[:, 0], labeled[:, -1]])
    border_labels = set(np.unique(border)) - {0}
    bg_mask = np.isin(labeled, list(border_labels))
    alpha = np.where(bg_mask, 0, 255).astype(np.uint8)
    return Image.fromarray(np.dstack((arr, alpha)))


def crop_and_fit(img, box, canvas, anchor="feet", bg_color="white", pad=2, max_scale=None):
    """Crop from source image, remove background, scale and place on canvas."""
    crop = img.crop(box)
    cut = extract_cutout(crop, bg_color=bg_color)
    rgba = np.array(cut)
    alpha = rgba[:, :, 3] >= 128
    if not alpha.any():
        return np.zeros((canvas[1], canvas[0], 4), dtype=np.uint8)
    ys, xs = np.where(alpha)
    y0, y1 = ys.min(), ys.max() + 1
    x0, x1 = xs.min(), xs.max() + 1
    sprite = cut.crop((x0, y0, x1, y1))

    # Determine scale to fit canvas with padding
    avail_w = canvas[0] - pad * 2
    avail_h = canvas[1] - pad * 2
    scale = min(avail_w / max(1, sprite.width), avail_h / max(1, sprite.height))
    if max_scale is not None:
        scale = min(scale, max_scale)
    new_w = max(1, int(round(sprite.width * scale)))
    new_h = max(1, int(round(sprite.height * scale)))
    scaled = sprite.resize((new_w, new_h), Image.Resampling.LANCZOS)

    out = Image.new("RGBA", canvas, (0, 0, 0, 0))
    if anchor == "fill" or anchor == "center":
        ox = (canvas[0] - scaled.width) // 2
        oy = (canvas[1] - scaled.height) // 2
    else:  # feet: bottom aligned
        ox = (canvas[0] - scaled.width) // 2
        oy = canvas[1] - pad - scaled.height
    out.alpha_composite(scaled, (ox, oy))
    arr_out = np.asarray(out).copy()
    arr_out[:, :, 3] = np.where(arr_out[:, :, 3] >= 128, 255, 0)
    return arr_out


def hsv_map(rgb, fn):
    out = []
    for r, g, b in rgb:
        h, s, v = colorsys.rgb_to_hsv(r / 255.0, g / 255.0, b / 255.0)
        h, s, v = fn(h * 360.0, s, v)
        rr, gg, bb = colorsys.hsv_to_rgb((h % 360.0) / 360.0,
                                         min(max(s, 0.0), 1.0), min(max(v, 0.0), 1.0))
        out.append((int(rr * 255), int(gg * 255), int(bb * 255)))
    return np.asarray(out, dtype=np.uint8)


def corrupt(colors, kind):
    hue, sat, val = {"smog": (275, 0.30, 0.62), "toxic": (85, 0.45, 0.66),
                     "rust": (22, 0.45, 0.70), "oil": (225, 0.35, 0.52), "ash": (285, 0.25, 0.48)}[kind]
    def fn(h, s, v):
        return hue, min(1.0, sat * (0.6 + s)), v * val
    return hsv_map(colors, fn)


def draw_tools():
    """Build the 32 x 256 indexed sheet of projectiles and HUD icons."""
    colors = np.array([[0, 0, 0], [255, 240, 232], [232, 72, 120], [160, 30, 80],
                       [120, 200, 90], [60, 130, 60], [255, 150, 40], [255, 224, 120],
                       [150, 140, 130], [80, 70, 70], [150, 220, 255], [70, 140, 220],
                       [40, 30, 50], [255, 90, 60], [190, 120, 220], [240, 190, 80]],
                      dtype=np.uint8)
    tool = np.zeros((32, TOOL_COLUMNS * 16), dtype=np.uint8)

    def cell(column, row=0):
        return tool[row * 16:(row + 1) * 16, column * 16:(column + 1) * 16]

    # Rose thorns: spinning petal-and-thorn in two frames
    for column, rotated in ((TOOL_THORN0, 0), (TOOL_THORN1, 1)):
        t = cell(column)
        for i in range(16):
            for j in range(16):
                dx, dy = j - 7.5, i - 7.5
                if rotated:
                    dx, dy = (dx + dy) * 0.7071, (dy - dx) * 0.7071
                d = dx * dx + dy * dy
                if d <= 12:
                    t[i, j] = 2 if d > 4 else 7
                elif abs(dy) < 1.5 and abs(dx) < 7.5:
                    t[i, j] = 5
                elif abs(dx) < 1.5 and abs(dy) < 7.5:
                    t[i, j] = 5

    # Trash, spit, bolt, fire, ice, oil
    trash = cell(TOOL_TRASH)
    trash[4:13, 3:13] = 8
    trash[5:8, 5:11] = 9
    trash[9:12, 4:8] = 1

    spit = cell(TOOL_SPIT)
    for i in range(16):
        for j in range(16):
            if (i - 8) ** 2 + (j - 8) ** 2 <= 20:
                spit[i, j] = 4 if (i - 8) ** 2 + (j - 8) ** 2 > 6 else 7

    bolt = cell(TOOL_BOLT)
    bolt[6:10, 1:15] = 10
    bolt[7:9, 1:15] = 1
    bolt[5:11, 12:15] = 11

    fire = cell(TOOL_FIRE)
    for i in range(16):
        for j in range(16):
            d = (i - 9) ** 2 * 0.6 + (j - 8) ** 2
            if d <= 30 and i > 1:
                fire[i, j] = 7 if d <= 8 else (6 if d <= 18 else 13)

    ice = cell(TOOL_ICE)
    for i in range(16):
        for j in range(16):
            if abs(i - 8) + abs(j - 8) * 2 <= 8:
                ice[i, j] = 10 if abs(i - 8) + abs(j - 8) * 2 > 3 else 1

    oil = cell(TOOL_OIL)
    for i in range(16):
        for j in range(16):
            d = (i - 8) ** 2 + (j - 8) ** 2
            if d <= 22:
                oil[i, j] = 12 if d > 6 else 14

    # Spark, heart, rose, petal, lane, cursor, drip, leaf
    spark = cell(TOOL_SPARK)
    for i in range(16):
        for j in range(16):
            d = abs(i - 7.5) + abs(j - 7.5)
            if d < 6:
                spark[i, j] = 7 if d < 3 else 1

    heart = cell(TOOL_HEART)
    for i in range(16):
        for j in range(16):
            x, y = (j - 7.5) / 6.5, (i - 6.5) / 6.5
            if (x * x + y * y - 1) ** 3 - x * x * y * y * y <= 0 and i > 1:
                heart[i, j] = 13 if (i > 4 and j > 4) else 1

    rose = cell(TOOL_ROSE)
    for i in range(16):
        for j in range(16):
            d = (i - 6) ** 2 + (j - 8) ** 2
            if d <= 24:
                rose[i, j] = 2 if d > 9 else (3 if (i + j) % 3 else 2)
    rose[12:16, 8] = 5
    rose[13, 5:8] = 4

    petal = cell(TOOL_PETAL)
    for i in range(16):
        for j in range(16):
            if (i - 8) ** 2 / 9 + (j - 8) ** 2 / 4 <= 3:
                petal[i, j] = 2 if (i + j) % 2 else 14

    lane = cell(TOOL_LANE)
    lane[14:16, :] = 1
    lane[12:14, 0:2] = 1
    lane[12:14, 14:16] = 1

    cursor = cell(TOOL_CURSOR)
    cursor[0:3, :] = 7
    cursor[13:16, :] = 7
    cursor[:, 0:3] = 7
    cursor[:, 13:16] = 7
    cursor[3:13, 3:13] = 0

    drip = cell(TOOL_DRIP)
    drip[0:10, 7:9] = 4
    drip[8:14, 5:11] = 4
    drip[10:12, 6:10] = 7

    leaf = cell(TOOL_LEAF)
    for i in range(16):
        for j in range(16):
            if (i - 8) ** 2 / 5 + (j - 8) ** 2 / 2.2 <= 6 and (i + j) > 4:
                leaf[i, j] = 4 if (i - j) % 4 else 5

    return tool, colors


def build():
    output = GAME / "artbox/generated"
    preview_dir = GAME / "artbox/in/generated"
    output.mkdir(parents=True, exist_ok=True)
    preview_dir.mkdir(parents=True, exist_ok=True)

    c1, c2 = io.BytesIO(), io.BytesIO()
    header = [
        "/* Generated by games/maiya/tools/build_commercial_assets.py. */",
        "#ifndef MAIYA_ASSETS_H",
        "#define MAIYA_ASSETS_H",
        "#include <stdint.h>",
        "",
    ]
    manifest = []

    def store(name, indices, palettes, assignments, rgba):
        h, w = indices.shape
        count = h * w // 256
        base = c1.tell() // 64
        lo, hi = encode_image(indices, count)
        assert np.array_equal(decode_image(lo, hi, w, h), indices)
        c1.write(lo)
        c2.write(hi)
        header.append(f"#define MG_{name.upper()}_TILE {base}u")
        preview = np.dstack((reconstruct(indices, palettes, assignments), rgba[:, :, 3]))
        Image.fromarray(preview.astype(np.uint8)).save(output / f"{name}.png")
        Image.fromarray(rgba).save(preview_dir / f"{name}.png")
        manifest.append(dict(name=name, tile_base=base, tile_last=base + count - 1,
                             strips=w // 16, rows=h // 16, stride=w // 16,
                             palette_count=len(palettes)))
        return base

    def append(name, rgba, banks=1, master=None, emit_palette=True, bank_base=16):
        rgba = np.asarray(rgba, dtype=np.uint8)
        indices, palettes, assignments = quantize(rgba, banks, master, dither="none")
        base = store(name, indices, palettes, assignments, rgba)
        if emit_palette:
            header.append(c_array(f"mg_{name}_pal", [v for p in palettes for v in palette_words(p)]))
            if banks > 1:
                header.append(c_array(f"mg_{name}_map", assignments.flatten() + bank_base, "uint8_t"))
            header.append(f"#define MG_{name.upper()}_BANKS {len(palettes)}u")
        return base, palettes

    def shared_set(prefix, frames, canvas_master=None):
        training_frames = [f[:, :, :3][training_mask(f)] for f in frames.values() if training_mask(f).any()]
        if not training_frames:
            training = np.zeros((1, 3), dtype=np.uint8)
        else:
            training = np.concatenate(training_frames)
        master = fit_palette(training) if canvas_master is None else canvas_master
        tiles = []
        for name, rgba in frames.items():
            base, _ = append(f"{prefix}_{name}", rgba, master=master, emit_palette=False)
            tiles.append(base)
        palette = np.zeros((16, 3), dtype=np.uint8)
        palette[1:] = master
        header.append(c_array(f"mg_{prefix}_tiles", tiles))
        header.append(c_array(f"mg_{prefix}_pal", palette_words(palette)))
        return master

    print("== 1. Compiling 6 HD Stage Environments ==", flush=True)
    sn = Image.open(find_file("stages_nature*.jpg")).convert("RGBA")
    sc = Image.open(find_file("stages_citadel*.jpg")).convert("RGBA")

    # 6 Worlds: 0:Forest, 1:Marsh, 2:City, 3:Drylands, 4:Peaks, 5:Citadel
    panels = [
        sn.crop((45, 0, 1376, 256)),
        sn.crop((45, 256, 1376, 512)),
        sn.crop((45, 512, 1376, 768)),
        sc.crop((0, 0, 1376, 256)),
        sc.crop((0, 256, 1376, 512)),
        sc.crop((0, 512, 1376, 768)),
    ]

    for i, panel in enumerate(panels):
        panel = np.asarray(panel.resize((512, 224), Image.Resampling.LANCZOS)).astype(np.float32)
        # Seamless wrap blend across edges
        blend = 64
        left, right = panel[:, :blend].copy(), panel[:, -blend:].copy()
        t = (np.arange(blend, dtype=np.float32) / blend)[None, :, None]
        panel[:, :blend] = left * (0.5 + 0.5 * t) + right * (0.5 - 0.5 * t)
        panel[:, -blend:] = right * (1.0 - 0.5 * t) + left * (0.5 * t)
        panel = panel.astype(np.uint8)
        panel[:, :, 3] = 255

        indices, palettes, assignments = quantize(panel, 16, None, dither="none")
        store(f"bg{i}", indices[:192], palettes, assignments[:12], panel[:192])
        store(f"ground{i}", indices[192:], palettes, assignments[12:], panel[192:])
        header.append(c_array(f"mg_bg{i}_pal", [v for p in palettes for v in palette_words(p)]))
        for kind in ("smog", "toxic", "rust", "oil", "ash"):
            words = []
            for p in palettes:
                dark = np.asarray(p, dtype=np.uint8).copy()
                dark[1:] = corrupt(dark[1:], kind)
                words.extend(palette_words(dark))
            header.append(c_array(f"mg_bg{i}_{kind}_pal", words))
        header.append(c_array(f"mg_bg{i}_map", assignments[:12].flatten() + 16, "uint8_t"))
        header.append(c_array(f"mg_ground{i}_map", assignments[12:].flatten() + 16, "uint8_t"))
        header.append(f"#define MG_BG{i}_BANKS {len(palettes)}u")
        print(f"  Stage {i + 1}/6 compiled (512x224)", flush=True)

    print("== 2. Compiling Maiya Heroine Moveset ==", flush=True)
    m_img = Image.open(find_file("maiya_heroine*.jpg")).convert("RGB")

    # Crop frames from the HD Maiya sheet
    # Row 0: idle / run
    f_idle0 = crop_and_fit(m_img, (27, 70, 144, 268), HERO_CANVAS)
    f_idle1 = crop_and_fit(m_img, (185, 70, 300, 268), HERO_CANVAS)
    f_idle2 = crop_and_fit(m_img, (27, 70, 144, 268), HERO_CANVAS)
    f_run0  = crop_and_fit(m_img, (354, 70, 488, 268), HERO_CANVAS)
    f_run1  = crop_and_fit(m_img, (515, 70, 664, 268), HERO_CANVAS)
    f_run2  = crop_and_fit(m_img, (682, 70, 834, 268), HERO_CANVAS)
    f_run3  = crop_and_fit(m_img, (852, 70, 997, 268), HERO_CANVAS)

    # Row 1: jump / crouch
    f_jump0 = crop_and_fit(m_img, (37, 310, 156, 520), HERO_CANVAS)
    f_jump1 = crop_and_fit(m_img, (189, 310, 341, 520), HERO_CANVAS)
    f_jump2 = crop_and_fit(m_img, (367, 310, 488, 520), HERO_CANVAS)
    f_jump3 = crop_and_fit(m_img, (525, 310, 656, 520), HERO_CANVAS)
    f_jump4 = crop_and_fit(m_img, (686, 310, 824, 520), HERO_CANVAS)
    f_land  = crop_and_fit(m_img, (846, 310, 983, 520), HERO_CANVAS)
    f_crouch = crop_and_fit(m_img, (830, 340, 983, 520), HERO_CANVAS)

    # Row 2: whip combat
    f_atk0 = crop_and_fit(m_img, (29, 551, 216, 740), HERO_CANVAS)
    f_atk1 = crop_and_fit(m_img, (231, 555, 470, 740), HERO_CANVAS)
    f_atk2 = crop_and_fit(m_img, (480, 555, 720, 740), HERO_CANVAS)
    f_atk3 = crop_and_fit(m_img, (730, 555, 960, 740), HERO_CANVAS)

    # Row 3: throw / win
    f_cast0 = crop_and_fit(m_img, (30, 780, 231, 990), HERO_CANVAS)
    f_cast1 = crop_and_fit(m_img, (256, 780, 465, 990), HERO_CANVAS)
    f_cast2 = crop_and_fit(m_img, (473, 780, 763, 990), HERO_CANVAS)
    f_win   = crop_and_fit(m_img, (810, 780, 964, 990), HERO_CANVAS)

    maiya_frames = {
        "idle0": f_idle0, "idle1": f_idle1, "idle2": f_idle2,
        "walk0": f_run0, "walk1": f_run1, "walk2": f_run2, "walk3": f_run3,
        "walk4": f_run0, "walk5": f_run1, "walk6": f_run2, "walk7": f_run3,
        "crouch": f_crouch,
        "run0": f_run0, "run1": f_run1, "run2": f_run2,
        "jump0": f_jump0, "jump1": f_jump1, "jump2": f_jump2, "jump3": f_jump3, "jump4": f_jump4, "land": f_land,
        "atk0": f_atk0, "atk1": f_atk1, "atk2": f_atk2, "atk3": f_atk3,
        "sweep0": f_atk0, "sweep1": f_atk1, "sweep2": f_atk2, "sweep3": f_atk3,
        "low": f_crouch, "spin": f_jump3,
        "cast0": f_cast0, "cast1": f_cast1, "cast2": f_cast2,
        "hurt0": f_jump4, "hurt1": f_jump2, "down": f_land, "sit": f_crouch, "win": f_win,
    }

    # Fit master palette for Maiya
    hero_training = np.concatenate([f[:, :, :3][training_mask(f)] for f in maiya_frames.values()])
    hero_master = fit_palette(hero_training)
    hero_tiles = []
    for name, f in maiya_frames.items():
        base, _ = append(f"hero_{name}", f, master=hero_master, emit_palette=False)
        hero_tiles.append(base)
    header.append(c_array("mg_hero_tiles", hero_tiles))
    header.append(f"#define MG_HERO_FRAMES {len(maiya_frames)}u")
    for k, name in enumerate(maiya_frames.keys()):
        header.append(f"#define MG_F_{name.upper()} {k}u")

    hero_pal = np.zeros((16, 3), dtype=np.uint8)
    hero_pal[1:] = hero_master
    header.append(c_array("mg_hero_pal", palette_words(hero_pal)))

    # Sun form palette (golden glow)
    def sun_tint(h, s, v):
        return (42, min(1.0, s * 1.1), min(1.0, v * 1.25))
    sun_pal = np.zeros((16, 3), dtype=np.uint8)
    sun_pal[1:] = hsv_map(hero_master, sun_tint)
    header.append(c_array("mg_hero_sun_pal", palette_words(sun_pal)))

    # Eagle (guardian sun bird)
    eagle_src = Image.open(find_file("iron_vulture*.jpg")).convert("RGB")
    eagle_frames = {
        "perch0": crop_and_fit(eagle_src, (4, 4, 208, 177), EAGLE_CANVAS),
        "perch1": crop_and_fit(eagle_src, (222, 21, 404, 177), EAGLE_CANVAS),
        "perch2": crop_and_fit(eagle_src, (4, 4, 208, 177), EAGLE_CANVAS),
        "fly0":   crop_and_fit(eagle_src, (4, 181, 197, 347), EAGLE_CANVAS),
        "fly1":   crop_and_fit(eagle_src, (200, 181, 390, 347), EAGLE_CANVAS),
        "fly2":   crop_and_fit(eagle_src, (4, 181, 197, 347), EAGLE_CANVAS),
    }
    shared_set("eagle", eagle_frames)

    # Face HUD icon and portrait
    face = crop_and_fit(m_img, (40, 75, 130, 165), (32, 32), anchor="center")
    append("face", face, master=hero_master)

    portrait = crop_and_fit(m_img, (27, 70, 144, 268), (112, 96), anchor="center")
    append("portrait", portrait, banks=2, bank_base=13)
    print(f"  Maiya compiled ({len(maiya_frames)} frames)", flush=True)

    print("== 3. Compiling 6 Corrupted Blight Enemies ==", flush=True)
    en_img = Image.open(find_file("corrupted_enemies*.jpg")).convert("RGB")

    creatures = {
        "slime": {
            "canvas": (48, 32),
            "boxes": [(12, 37, 165, 170), (184, 37, 335, 170), (353, 37, 505, 170), (522, 37, 666, 170)],
        },
        "beetle": {
            "canvas": (64, 48),
            "boxes": [(8, 216, 170, 339), (180, 216, 345, 339), (360, 216, 515, 339)],
        },
        "crow": {
            "canvas": (64, 48),
            "boxes": [(8, 350, 185, 508), (190, 350, 345, 508)],
        },
        "goblin": {
            "canvas": (48, 64),
            "boxes": [(10, 547, 170, 682), (180, 547, 340, 682), (350, 547, 510, 682), (520, 547, 680, 682)],
        },
        "worm": {
            "canvas": (32, 80),
            "boxes": [(20, 690, 160, 851), (170, 690, 310, 851)],
        },
        "robot": {
            "canvas": (48, 48),
            "boxes": [(20, 886, 140, 1011), (150, 886, 270, 1011), (270, 886, 390, 1011)],
        },
    }

    for cname, spec in creatures.items():
        frames = {}
        for k, box in enumerate(spec["boxes"]):
            frames[str(k)] = crop_and_fit(en_img, box, spec["canvas"])
        shared_set(cname, frames)
        header.append(f"#define MG_{cname.upper()}_FRAMES {len(frames)}u")
        header.append(f"#define MG_{cname.upper()}_W {spec['canvas'][0]}u")
        header.append(f"#define MG_{cname.upper()}_H {spec['canvas'][1]}u")
        print(f"  Enemy {cname} compiled ({len(frames)} frames)", flush=True)

    print("== 4. Compiling Allies & NPCs ==", flush=True)
    allies_img = Image.open(find_file("maiya_allies*.jpg")).convert("RGB")

    npcs = {
        "sunboy": [(25, 35, 230, 290), (270, 35, 490, 290), (510, 35, 740, 290)],
        "spirit": [(790, 45, 960, 250), (45, 320, 180, 520)],
        "elder":  [(550, 315, 730, 560), (550, 315, 730, 560)],
        "girl":   [(780, 315, 960, 560), (780, 315, 960, 560)],
    }

    for nname, boxes in npcs.items():
        frames = {str(k): crop_and_fit(allies_img, box, (48, 64)) for k, box in enumerate(boxes)}
        shared_set(nname, frames)
        header.append(f"#define MG_{nname.upper()}_FRAMES {len(frames)}u")
        header.append(f"#define MG_{nname.upper()}_W 48u")
        header.append(f"#define MG_{nname.upper()}_H 64u")
        print(f"  NPC {nname} compiled", flush=True)

    print("== 5. Compiling 7 Biomechanical Bosses (128x128) ==", flush=True)
    boss_specs = [
        ("beetle", "chainsaw_beetle*.jpg", "black", [(40, 15, 480, 500), (510, 500, 740, 750)]),
        ("toad", "sludge_toad*.jpg", "white", [(5, 6, 349, 336), (430, 520, 740, 680)]),
        ("vulture", "iron_vulture*.jpg", "white", [(4, 4, 208, 177), (200, 180, 390, 347)]),
        ("jackal", "inferno_jackal*.jpg", "white", [(34, 23, 273, 183), (220, 350, 480, 520)]),
        ("owl", "blizzard_owl*.jpg", "white", [(13, 12, 258, 193), (400, 390, 620, 580)]),
        ("leviathan", "toxic_leviathan*.jpg", "white", [(7, 5, 384, 162), (170, 550, 470, 680)]),
        ("smoggar", "lord_smoggar*.jpg", "white", [(21, 12, 218, 301), (680, 290, 990, 560)]),
    ]

    for bname, pattern, bg, boxes in boss_specs:
        b_img = Image.open(find_file(pattern)).convert("RGB")
        frames = {str(k): crop_and_fit(b_img, box, BOSS_CANVAS, bg_color=bg) for k, box in enumerate(boxes)}
        shared_set(f"boss_{bname}", frames)
        print(f"  Boss {bname} compiled (128x128, 2 frames)", flush=True)

    print("== 6. Compiling Props, Hazards, and Pickups ==", flush=True)
    prop_boxes = {
        "chest": (680, 640, 830, 760),
        "chest_open": (840, 610, 990, 770),
        "spikes": (40, 840, 180, 980),
        "lava": (680, 840, 970, 970),
        "sludge": (360, 840, 600, 960),
        "bush": (30, 640, 160, 760),
        "mushroom": (170, 640, 290, 760),
        "plant": (430, 640, 550, 760),
        "torch0": (40, 840, 180, 980),
        "torch1": (40, 840, 180, 980),
        "door_top": (680, 640, 830, 760),
        "door_mid": (680, 640, 830, 760),
        "water": (360, 840, 600, 960),
        "sign": (680, 640, 830, 760),
    }
    props = {pname: crop_and_fit(allies_img, box, (32, 32), anchor="fill") for pname, box in prop_boxes.items()}
    shared_set("prop", props)
    for k, pname in enumerate(props.keys()):
        header.append(f"#define MG_P_{pname.upper()} {k}u")

    item_boxes = {
        "rose_gold": (30, 640, 160, 760),
        "rose_red":  (170, 640, 290, 760),
        "heart":     (300, 640, 420, 760),
        "seed":      (430, 640, 550, 760),
        "gem":       (560, 640, 670, 760),
    }
    items = {iname: crop_and_fit(allies_img, box, (32, 32), anchor="fill") for iname, box in item_boxes.items()}
    shared_set("item", items)
    for k, iname in enumerate(items.keys()):
        header.append(f"#define MG_I_{iname.upper()} {k}u")

    # Ground platform blocks (one 3-piece set per world: Left, Mid, Right)
    grounds = ["grass", "sand", "stone", "planet", "snow", "dirt", "metal"]
    for gname in grounds:
        # 3 distinct 32x32 block tiles
        blocks = {}
        for k in range(3):
            blk = np.zeros((32, 32, 4), dtype=np.uint8)
            # Create textured block tile
            blk[4:, :] = [70, 130, 60, 255] if "grass" in gname else [120, 100, 80, 255]
            if k == 0:
                blk[2:, 2:] = [90, 160, 80, 255]
            elif k == 2:
                blk[2:, :-2] = [90, 160, 80, 255]
            else:
                blk[2:, :] = [100, 180, 90, 255]
            blocks[str(k)] = blk
        shared_set(f"block_{gname}", blocks)

    print("== 7. Compiling Projectile and HUD Tools Sheet ==", flush=True)
    tool, colors = draw_tools()
    base = c1.tell() // 64
    lo, hi = encode_image(tool, tool.size // 256)
    c1.write(lo)
    c2.write(hi)
    header.append(f"#define MG_TOOL_TILE {base}u")
    header.append(f"#define MG_TOOL_COLUMNS {TOOL_COLUMNS}u")
    tool_names = (
        ("THORN0", TOOL_THORN0), ("THORN1", TOOL_THORN1), ("TRASH", TOOL_TRASH),
        ("SPIT", TOOL_SPIT), ("BOLT", TOOL_BOLT), ("FIRE", TOOL_FIRE), ("ICE", TOOL_ICE),
        ("OIL", TOOL_OIL), ("SPARK", TOOL_SPARK), ("HEART", TOOL_HEART), ("ROSE", TOOL_ROSE),
        ("PETAL", TOOL_PETAL), ("LANE", TOOL_LANE), ("CURSOR", TOOL_CURSOR),
        ("DRIP", TOOL_DRIP), ("LEAF", TOOL_LEAF)
    )
    for name, col in tool_names:
        header.append(f"#define MG_T_{name} {col}u")
    header.append(c_array("mg_tool_pal", palette_words(colors)))

    preview = np.dstack((colors[tool], np.where(tool > 0, 255, 0))).astype(np.uint8)
    Image.fromarray(preview).save(output / "tools.png")

    print("== 8. Compiling Title Screen Key Visual ==", flush=True)
    title_raw = Image.open(find_file("maiya_title*.jpg")).convert("RGBA")
    title_fit = title_raw.resize((304, 224), Image.Resampling.LANCZOS)
    title_arr = np.asarray(title_fit, dtype=np.uint8)
    title_base, title_pals = append("title", title_arr, banks=2, bank_base=13)

    write_utility_tiles(c1, c2)

    # Output C-ROM pair
    for data, suffix in ((c1.getvalue(), "c1"), (c2.getvalue(), "c2")):
        raw = np.frombuffer(data, dtype=np.uint8).reshape(-1, 2)[:, ::-1].tobytes()
        (output / f"780-{suffix}.{suffix}").write_bytes(raw)
        assert raw[0xFFFF * 64:] == bytes(64)

    header.append("#endif\n")
    (output / "maiya_assets.h").write_text("\n".join(header), encoding="ascii")
    (output / "assets.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="ascii")

    print(f"\nSUCCESS: Built {len(manifest)} assets ({c1.tell() // 1024} KB per C-ROM)")
    print(f"Output directory: {output}")


if __name__ == "__main__":
    build()
