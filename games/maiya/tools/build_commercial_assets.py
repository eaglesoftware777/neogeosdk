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
from PIL import Image, ImageFilter
import scipy.ndimage as ndi

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "artbox"))

from palette_banks import fit_palette, palette_words, quantize, reconstruct, training_mask  # noqa: E402
from tile_codec import encode_image, decode_image, write_utility_tiles  # noqa: E402

sys.path.insert(0, str(Path(__file__).resolve().parent))
import nature_art  # noqa: E402

SOURCE = GAME / "assets/source_art"
CHARACTERS = ROOT / "games/demo/artbox/in/characters"

# Per-valley and boss-recolour tints, in one place and editable without
# touching this file: games/maiya/tools/maiya_palette_studio.py reads and
# writes this same JSON.  Deleting it (or any field in it) falls back to
# these defaults, so the file is optional and always safe to regenerate.
PALETTE_CONFIG_PATH = GAME / "artbox" / "palette_config.json"

DEFAULT_VALLEY_TINT = [
    {"name": "Emerald Forest", "hue": 0.0, "sat": 1.00, "val": 1.00},
    {"name": "Valley of Falls", "hue": -40.0, "sat": 0.95, "val": 0.96},
    {"name": "Azure Coast", "hue": -95.0, "sat": 0.85, "val": 1.06},
    {"name": "Autumn Grove", "hue": 95.0, "sat": 1.05, "val": 1.02},
    {"name": "Crystal Grotto", "hue": -60.0, "sat": 0.55, "val": 1.14},
    {"name": "World Tree", "hue": 150.0, "sat": 0.90, "val": 0.82},
    {"name": "Rio Negro Works", "hue": 40.0, "sat": 1.10, "val": 0.78},
    {"name": "Sunken Reef", "hue": -110.0, "sat": 0.80, "val": 0.98},
    {"name": "Silver Cave", "hue": 170.0, "sat": 0.45, "val": 1.05},
    {"name": "Golden Savanna", "hue": 60.0, "sat": 1.00, "val": 1.10},
]

DEFAULT_REUSED_BOSS_TINT = [
    {"name": "eel", "source": "leviathan", "hue": -70.0, "sat": 0.85, "val": 1.05},
    {"name": "wyrm", "source": "toad", "hue": 150.0, "sat": 0.70, "val": 1.15},
    {"name": "hyena", "source": "jackal", "hue": 25.0, "sat": 1.05, "val": 1.00},
]


def load_palette_config():
    """Return (valley_tint, reused_boss_tint), creating the config with the
    current defaults on first run so it's there to edit afterwards."""
    if PALETTE_CONFIG_PATH.is_file():
        try:
            data = json.loads(PALETTE_CONFIG_PATH.read_text(encoding="utf-8"))
            return (
                data.get("valley_tint", DEFAULT_VALLEY_TINT),
                data.get("reused_boss_tint", DEFAULT_REUSED_BOSS_TINT),
            )
        except (ValueError, OSError):
            return DEFAULT_VALLEY_TINT, DEFAULT_REUSED_BOSS_TINT
    PALETTE_CONFIG_PATH.parent.mkdir(parents=True, exist_ok=True)
    PALETTE_CONFIG_PATH.write_text(
        json.dumps(
            {"valley_tint": DEFAULT_VALLEY_TINT, "reused_boss_tint": DEFAULT_REUSED_BOSS_TINT},
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )
    return DEFAULT_VALLEY_TINT, DEFAULT_REUSED_BOSS_TINT


# Playfield scale.  Maiya stands 52 px on a 224-line screen so a mission reads
# as an arcade adventure -- long roads, tall canopies, room for a crowd --
# instead of two giant fighters filling the frame.
HERO_CANVAS = (80, 64)
HERO_STRIPS = 5
HERO_ROWS = 4
HERO_STRIDE = 5
HERO_HEIGHT = 52

EAGLE_CANVAS = (48, 32)
EAGLE_STRIPS = 3
EAGLE_ROWS = 2
EAGLE_HEIGHT = 26

BOSS_CANVAS = (96, 96)
BOSS_STRIPS = 6
BOSS_ROWS = 6
BOSS_STRIDE = 6
BOSS_HEIGHT = 86

NPC_CANVAS = (32, 48)
NPC_HEIGHT = 44

# Tool tiles (16 columns, 32 rows total = 2 rows of 16x16 tiles)
TOOL_COLUMNS = 16
TOOL_THORN0, TOOL_THORN1, TOOL_TRASH, TOOL_SPIT, TOOL_BOLT, TOOL_FIRE, TOOL_ICE, TOOL_OIL = range(8)
TOOL_SPARK, TOOL_HEART, TOOL_ROSE, TOOL_PETAL, TOOL_LANE, TOOL_CURSOR, TOOL_DRIP, TOOL_LEAF = range(8, 16)
TOOL_HALO, TOOL_DUST, TOOL_STAR = 16, 17, 18     # second row of the sheet


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


def _cutout_bbox(img, box, bg_color):
    """Crop a source box, drop its flat background, return the tight sprite."""
    cut = extract_cutout(img.crop(box), bg_color=bg_color)
    arr = np.array(cut)
    alpha = arr[:, :, 3] >= 128
    if not alpha.any():
        return None
    ys, xs = np.where(alpha)
    return cut.crop((xs.min(), ys.min(), xs.max() + 1, ys.max() + 1))


def _foot_center(arr):
    """Horizontal centre of the lowest quarter of a sprite: its stance."""
    alpha = arr[:, :, 3] >= 128
    ys, xs = np.where(alpha)
    if not len(ys):
        return arr.shape[1] // 2
    cut = ys.max() - max(1, (ys.max() - ys.min()) // 4)
    feet = xs[ys >= cut]
    return int(round(feet.mean())) if len(feet) else int(round(xs.mean()))


def sharpen_sprite(arr, amount=1.0, outline=0.20):
    """Put the bite back into a sprite that has been shrunk by four.

    A single Lanczos pass from 200 px down to 40 loses the line work the art
    was drawn with: edges go soft and the whole figure turns to mush at
    arcade size.  An unsharp pass restores the interior detail, and darkening
    the rim pixels gives the silhouette its outline back.
    """
    rgb = arr[:, :, :3].astype(np.uint8)
    alpha = arr[:, :, 3]
    sharp = np.asarray(Image.fromarray(rgb).filter(
        ImageFilter.UnsharpMask(radius=1.2, percent=int(amount * 100), threshold=2)))

    solid = alpha >= 128
    if solid.any() and outline > 0.0:
        pad_mask = np.pad(solid, 1)
        rim = solid & ~(pad_mask[:-2, 1:-1] & pad_mask[2:, 1:-1] &
                        pad_mask[1:-1, :-2] & pad_mask[1:-1, 2:])
        sharp = sharp.astype(np.float32)
        sharp[rim] *= (1.0 - outline)
        sharp = np.clip(sharp, 0, 255)

    out = arr.copy()
    out[:, :, :3] = sharp.astype(np.uint8)
    return out


def fit_group(img, boxes, canvas, target_h, bg_color="white", pad=2, sharpen=True,
              normalize_extent=False):
    """Scale a whole animation by ONE factor and stand every frame on its feet.

    Fitting each frame to the canvas on its own made the character swell and
    shrink between poses; a shared scale plus a foot-centre anchor keeps the
    walk, the whip and the leap the same girl.
    """
    sprites = {name: _cutout_bbox(img, box, bg_color) for name, box in boxes.items()}
    heights = [sp.height for sp in sprites.values() if sp is not None]
    if not heights:
        return {name: np.zeros((canvas[1], canvas[0], 4), dtype=np.uint8) for name in boxes}

    reference = float(np.median(heights))
    scale = target_h / reference
    scale = min(scale, (canvas[1] - pad) / max(heights))

    frames = {}
    for name, sprite in sprites.items():
        out = np.zeros((canvas[1], canvas[0], 4), dtype=np.uint8)
        if sprite is None:
            frames[name] = out
            continue
        # Guardian sheets mix a large pose and a thumbnail. Match their
        # silhouette extent, not their unrelated source-sheet resolution.
        frame_scale = target_h / max(sprite.size) if normalize_extent else scale
        w = max(1, int(round(sprite.width * frame_scale)))
        h = max(1, int(round(sprite.height * frame_scale)))
        scaled = np.asarray(sprite.resize((w, h), Image.Resampling.LANCZOS))
        scaled = scaled.copy()
        scaled[:, :, 3] = np.where(scaled[:, :, 3] >= 128, 255, 0)

        if sharpen:
            scaled = sharpen_sprite(scaled)

        ox = (canvas[0] - w) // 2 if normalize_extent else canvas[0] // 2 - _foot_center(scaled)
        oy = canvas[1] - pad - h
        sx0, sy0 = max(0, -ox), max(0, -oy)
        dx0, dy0 = max(0, ox), max(0, oy)
        cw = min(w - sx0, canvas[0] - dx0)
        ch = min(h - sy0, canvas[1] - dy0)
        if cw > 0 and ch > 0:
            out[dy0:dy0 + ch, dx0:dx0 + cw] = scaled[sy0:sy0 + ch, sx0:sx0 + cw]
        frames[name] = out
    return frames


def squash(frame, factor=0.62, lean=0):
    """Compress a standing frame onto its heels: a crouch, or a seated rest."""
    alpha = frame[:, :, 3] >= 128
    if not alpha.any():
        return frame.copy()
    ys, xs = np.where(alpha)
    y0, y1 = ys.min(), ys.max() + 1
    x0, x1 = xs.min(), xs.max() + 1
    body = Image.fromarray(frame[y0:y1, x0:x1], "RGBA")
    new_h = max(1, int(round(body.height * factor)))
    body = body.resize((body.width, new_h), Image.Resampling.LANCZOS)
    arr = np.asarray(body).copy()
    arr[:, :, 3] = np.where(arr[:, :, 3] >= 128, 255, 0)

    out = np.zeros_like(frame)
    dy = y1 - new_h
    dx = min(max(0, x0 + lean), frame.shape[1] - arr.shape[1])
    out[dy:dy + new_h, dx:dx + arr.shape[1]] = arr
    return out


def hsv_map(rgb, fn):
    out = []
    for r, g, b in rgb:
        h, s, v = colorsys.rgb_to_hsv(r / 255.0, g / 255.0, b / 255.0)
        h, s, v = fn(h * 360.0, s, v)
        rr, gg, bb = colorsys.hsv_to_rgb((h % 360.0) / 360.0,
                                         min(max(s, 0.0), 1.0), min(max(v, 0.0), 1.0))
        out.append((int(rr * 255), int(gg * 255), int(bb * 255)))
    return np.asarray(out, dtype=np.uint8)


def corrupt(colors, hue=286.0, pull=0.26, sat=0.74, val=0.88):
    """Blight a scene without erasing it.

    Repainting every colour in one flat hue turned the forest into a purple
    smear.  Pulling each hue part of the way toward the blight and draining
    some life keeps the place recognisable, so cleansing it reads as the same
    valley waking up.
    """
    def fn(h, s, v):
        delta = ((hue - h + 540.0) % 360.0) - 180.0
        return h + delta * pull, s * sat, v * val
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

    # Second row: the halo she rises under, a puff of road dust, a star.
    halo = cell(TOOL_HALO - TOOL_COLUMNS, 1)
    for i in range(16):
        for j in range(16):
            d = ((j - 7.5) / 7.0) ** 2 + ((i - 7.5) / 3.2) ** 2
            if 0.55 <= d <= 1.0:
                halo[i, j] = 7 if i < 8 else 15

    dust = cell(TOOL_DUST - TOOL_COLUMNS, 1)
    for cx, cy, r in ((4, 11, 3.0), (9, 9, 3.6), (13, 12, 2.6)):
        for i in range(16):
            for j in range(16):
                if (j - cx) ** 2 + (i - cy) ** 2 <= r * r:
                    dust[i, j] = 1 if (i + j) % 3 else 8

    star = cell(TOOL_STAR - TOOL_COLUMNS, 1)
    for k in range(-5, 6):
        star[7 + k, 7] = 7 if abs(k) < 3 else 1
        star[7, 7 + k] = 7 if abs(k) < 3 else 1
    for k in range(-2, 3):
        star[7 + k, 7 + k] = 1
        star[7 + k, 7 - k] = 1
    star[7, 7] = 15

    return tool, colors


def build():
    output = GAME / "artbox/generated"
    preview_dir = GAME / "artbox/in/generated"
    output.mkdir(parents=True, exist_ok=True)
    preview_dir.mkdir(parents=True, exist_ok=True)

    palette_valley_cfg, palette_boss_cfg = load_palette_config()

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

    print("== 1. Compiling 6 Nature Stage Environments ==", flush=True)
    sn = Image.open(find_file("stages_nature*.jpg")).convert("RGBA")
    env = Image.open(GAME / "assets/environments.png").convert("RGBA")

    # Six valleys of the living world.  Every box keeps the 320:140 playfield
    # ratio so nothing is squeezed on its way to a 512 x 224 wrap.
    panels = [
        sn.crop((45, 0, 630, 256)),          # 0 Emerald Forest
        env.crop((768, 0, 1536, 336)),       # 1 Valley of Sacred Falls
        env.crop((0, 341, 768, 677)),        # 2 Azure Coral Coast
        env.crop((0, 0, 768, 336)),          # 3 Golden Autumn Grove
        env.crop((768, 683, 1536, 1019)),    # 4 Crystal Grotto
        sn.crop((45, 256, 630, 512)),        # 5 Ancient World Tree
        None,                                # 6 Rio Negro Works, composed below
    ]

    # The works: the old plant's furnaces and gantries stand over the swamp
    # river's bank -- the painted factory for the far layer, the painted
    # marsh water for the road.
    works = np.asarray(sn.crop((600, 512, 1185, 768)).resize((512, 224), Image.Resampling.LANCZOS))
    river = np.asarray(sn.crop((45, 256, 630, 512)).resize((512, 224), Image.Resampling.LANCZOS))
    composed = works.copy()
    composed[192:] = river[192:]
    fade = np.linspace(0.0, 1.0, 12)[:, None, None]
    composed[180:192] = (works[180:192] * (1 - fade) + river[180:192] * fade).astype(np.uint8)
    panels[6] = Image.fromarray(composed)

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
        if i == 6:
            healed = []
            for p in palettes:
                green = np.asarray(p, dtype=np.uint8).copy()
                green[1:] = hsv_map(green[1:], lambda h, s, v: (h + (110.0 - h) * 0.35, min(1.0, s * 1.15), min(1.0, v * 1.08)))
                healed.extend(palette_words(green))
            header.append(c_array(f"mg_bg{i}_pal", healed))
            header.append(c_array(f"mg_bg{i}_blight_pal", [v for p in palettes for v in palette_words(p)]))
        else:
            header.append(c_array(f"mg_bg{i}_pal", [v for p in palettes for v in palette_words(p)]))
            blighted = []
            for p in palettes:
                dark = np.asarray(p, dtype=np.uint8).copy()
                dark[1:] = corrupt(dark[1:])
                blighted.extend(palette_words(dark))
            header.append(c_array(f"mg_bg{i}_blight_pal", blighted))
        header.append(c_array(f"mg_bg{i}_map", assignments[:12].flatten() + 16, "uint8_t"))
        header.append(c_array(f"mg_ground{i}_map", assignments[12:].flatten() + 16, "uint8_t"))
        header.append(f"#define MG_BG{i}_BANKS {len(palettes)}u")
        print(f"  Stage {i + 1}/7 compiled (512x224)", flush=True)

    print("== 2. Compiling Maiya Heroine Moveset ==", flush=True)
    m_img = Image.open(find_file("maiya_heroine*.jpg")).convert("RGB")

    hero_boxes = {
        "idle0": (27, 70, 144, 268), "idle1": (185, 70, 300, 268),
        "run0": (354, 70, 488, 268), "run1": (515, 70, 664, 268),
        "run2": (682, 70, 834, 268), "run3": (852, 70, 997, 268),
        "jump0": (37, 310, 156, 520), "jump1": (189, 310, 341, 520),
        "jump2": (367, 310, 488, 520), "jump3": (525, 310, 656, 520),
        "jump4": (686, 310, 824, 520), "land": (846, 310, 983, 520),
        "atk0": (29, 551, 216, 740), "atk1": (231, 555, 470, 740),
        "atk2": (480, 555, 720, 740), "atk3": (730, 555, 960, 740),
        "cast0": (30, 780, 231, 990), "cast1": (256, 780, 465, 990),
        "cast2": (473, 780, 763, 990), "win": (810, 780, 964, 990),
    }
    hf = fit_group(m_img, hero_boxes, HERO_CANVAS, HERO_HEIGHT)

    # Resting poses are not on the sheet: fold a standing frame onto its heels
    # so Maiya can duck under a swoop, take a knee, or sit down and listen.
    hf["crouch"] = squash(hf["land"], 0.70)
    hf["low"] = squash(hf["idle0"], 0.60)
    hf["sit"] = squash(hf["idle1"], 0.54)
    hf["down"] = squash(hf["land"], 0.44)

    maiya_frames = {
        "idle0": hf["idle0"], "idle1": hf["idle1"], "idle2": hf["idle0"],
        "walk0": hf["run0"], "walk1": hf["run1"], "walk2": hf["run2"], "walk3": hf["run3"],
        "walk4": hf["run0"], "walk5": hf["run1"], "walk6": hf["run2"], "walk7": hf["run3"],
        "crouch": hf["crouch"],
        "run0": hf["run0"], "run1": hf["run1"], "run2": hf["run2"],
        "jump0": hf["jump0"], "jump1": hf["jump1"], "jump2": hf["jump2"],
        "jump3": hf["jump3"], "jump4": hf["jump4"], "land": hf["land"],
        "atk0": hf["atk0"], "atk1": hf["atk1"], "atk2": hf["atk2"], "atk3": hf["atk3"],
        "sweep0": hf["atk0"], "sweep1": hf["atk1"], "sweep2": hf["atk2"], "sweep3": hf["atk3"],
        "low": hf["low"], "spin": hf["jump3"],
        "cast0": hf["cast0"], "cast1": hf["cast1"], "cast2": hf["cast2"],
        "hurt0": hf["jump4"], "hurt1": hf["jump2"], "down": hf["down"],
        "sit": hf["sit"], "win": hf["win"],
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

    # A second heroine to choose at the title: same sprites, a different
    # girl.  Blonde and green becomes black-haired and blue -- classed by
    # hue and saturation rather than by index, so it survives any future
    # repaint of the source art.  Skin, the rose whip and outlines are left
    # exactly as painted; only the hair and the dress move.
    def alt_tint(h, s, v):
        if 25.0 <= h <= 65.0 and s > 0.5:            # blonde hair -> near-black
            return (250.0, min(1.0, s * 0.55), v * 0.30)
        if 80.0 <= h <= 170.0:                        # green dress -> blue
            return (226.0, min(1.0, s * 1.05), v)
        return (h, s, v)
    alt_pal = np.zeros((16, 3), dtype=np.uint8)
    alt_pal[1:] = hsv_map(hero_master, alt_tint)
    header.append(c_array("mg_hero_alt_pal", palette_words(alt_pal)))

    # Eagle (guardian sun bird)
    eagle_src = Image.open(find_file("iron_vulture*.jpg")).convert("RGB")
    eagle_frames = fit_group(eagle_src, {
        "perch0": (4, 4, 208, 177), "perch1": (222, 21, 404, 177),
        "perch2": (4, 4, 208, 177), "fly0": (4, 181, 197, 347),
        "fly1": (200, 181, 390, 347), "fly2": (4, 181, 197, 347),
    }, EAGLE_CANVAS, EAGLE_HEIGHT)
    shared_set("eagle", eagle_frames)

    # Face HUD icon and portrait
    face = crop_and_fit(m_img, (40, 75, 130, 165), (32, 32), anchor="center")
    append("face", face, master=hero_master)

    portrait = crop_and_fit(m_img, (27, 70, 144, 268), (96, 96), anchor="center")
    append("portrait", portrait, banks=2, bank_base=41)
    print(f"  Maiya compiled ({len(maiya_frames)} frames)", flush=True)

    print("== 3. Compiling 6 Corrupted Blight Enemies ==", flush=True)
    en_img = Image.open(find_file("corrupted_enemies*.jpg")).convert("RGB")

    # Blight creatures at playfield scale: a slime comes up to Maiya's knee,
    # a goblin to her shoulder, so a crowd of them still fits the screen.
    creatures = {
        "slime": {
            "canvas": (32, 32), "height": 24,
            "boxes": [(12, 37, 165, 170), (184, 37, 335, 170), (353, 37, 505, 170), (522, 37, 666, 170)],
        },
        "beetle": {
            "canvas": (48, 32), "height": 26,
            "boxes": [(8, 216, 170, 339), (180, 216, 345, 339), (360, 216, 515, 339)],
        },
        "crow": {
            "canvas": (48, 32), "height": 26, "lift": 1.35,
            "boxes": [(10, 374, 180, 508), (188, 374, 350, 508)],
        },
        "goblin": {
            "canvas": (32, 48), "height": 40,
            "boxes": [(10, 547, 170, 682), (180, 547, 340, 682), (350, 547, 510, 682), (520, 547, 680, 682)],
        },
        "worm": {
            "canvas": (32, 48), "height": 42,
            "boxes": [(22, 716, 160, 845), (168, 716, 306, 845)],
        },
        "robot": {
            "canvas": (32, 32), "height": 28,
            "boxes": [(22, 908, 140, 1014), (152, 908, 270, 1014), (282, 908, 400, 1014)],
        },
    }

    # Each valley fields its own creatures: the same bodies wearing the
    # colours of the place they live in -- moss, river blue, coast sand,
    # autumn amber, grotto ice, and the blight of the world tree. Tunable
    # in games/maiya/artbox/palette_config.json (see load_palette_config).
    valley_tint = [(v["hue"], v["sat"], v["val"]) for v in palette_valley_cfg]

    for cname, spec in creatures.items():
        boxes = {str(k): box for k, box in enumerate(spec["boxes"])}
        frames = fit_group(en_img, boxes, spec["canvas"], spec["height"])
        if "lift" in spec:
            # Night-black art turns to mud at 26 px: lift it until the
            # silhouette reads against a dark canopy.
            for name, f in frames.items():
                rgb = np.clip(f[:, :, :3].astype(np.float32) * spec["lift"] + 14.0, 0, 255)
                frames[name] = np.dstack((rgb.astype(np.uint8), f[:, :, 3]))
        master = shared_set(cname, frames)
        words = []
        for shift, sat, val in valley_tint:
            tinted = np.zeros((16, 3), dtype=np.uint8)
            tinted[1:] = hsv_map(master, lambda h, s, v, _s=shift, _a=sat, _b=val:
                                 (h + _s, min(1.0, s * _a), min(1.0, v * _b)))
            words.extend(palette_words(tinted))
        header.append(c_array(f"mg_{cname}_valley_pal", words))
        header.append(f"#define MG_{cname.upper()}_FRAMES {len(frames)}u")
        header.append(f"#define MG_{cname.upper()}_W {spec['canvas'][0]}u")
        header.append(f"#define MG_{cname.upper()}_H {spec['canvas'][1]}u")
        print(f"  Enemy {cname} compiled ({len(frames)} frames)", flush=True)

    # Genuinely new creatures rather than more recolours of the shared six:
    # each of these is its own art, its own bank, so the roster finally has
    # more than seven bodies to draw encounters from.
    set1_img = Image.open(find_file("pollution_enemies_set1*.jpg")).convert("RGB")
    set2_img = Image.open(find_file("more_enemies_set2*.jpg")).convert("RGB")

    new_creatures = {
        "jellyfish": (set1_img, (32, 32), 26, {"0": (10, 55, 205, 265), "1": (215, 55, 410, 265)}),
        "toxiccrab": (set1_img, (48, 32), 28, {"0": (10, 345, 250, 540), "1": (250, 345, 490, 540)}),
        "acidmoth":  (set1_img, (48, 32), 30, {"0": (10, 765, 250, 1020), "1": (250, 765, 490, 1020)}),
        "sewerrat":  (set1_img, (32, 48), 40, {"0": (505, 790, 690, 1020), "1": (690, 790, 875, 1020)}),
        "smogbat":   (set1_img, (32, 48), 40, {"0": (505, 60, 675, 290), "1": (675, 60, 845, 290)}),
        "poachdrone":(set2_img, (48, 32), 28, {"0": (520, 398, 745, 528), "1": (750, 398, 975, 528)}),
        "chemfly":   (set2_img, (32, 32), 20, {"0": (15, 398, 195, 528), "1": (200, 398, 380, 528)}),
        "plasticbat":(set2_img, (32, 32), 28, {"0": (0, 60, 170, 210), "1": (170, 60, 340, 210)}),
        "slaggolem": (set2_img, (32, 48), 40, {"0": (510, 60, 681, 210), "1": (681, 60, 852, 210)}),
        "vinesting": (set2_img, (32, 64), 56, {"0": (250, 720, 375, 870), "1": (375, 720, 500, 870)}),
        "sporegob":  (set2_img, (32, 48), 40, {"0": (510, 720, 681, 900), "1": (681, 720, 852, 900)}),
    }
    for cname, (src, canvas, height, boxes) in new_creatures.items():
        frames = fit_group(src, boxes, canvas, height)
        shared_set(cname, frames)
        header.append(f"#define MG_{cname.upper()}_FRAMES {len(frames)}u")
        print(f"  Enemy {cname} compiled ({len(frames)} frames)", flush=True)

    print("== 4. Compiling Allies & NPCs ==", flush=True)
    allies_img = Image.open(find_file("maiya_allies*.jpg")).convert("RGB")

    npcs = {
        "sunboy": [(270, 35, 490, 290), (510, 35, 740, 290), (25, 35, 230, 290)],
        "spirit": [(790, 45, 960, 250), (45, 320, 180, 520), (190, 320, 330, 520)],
        "elder":  [(550, 315, 730, 560), (550, 315, 730, 560)],
        "girl":   [(780, 315, 960, 560), (780, 315, 960, 560)],
    }

    for nname, boxes in npcs.items():
        frames = fit_group(allies_img, {str(k): box for k, box in enumerate(boxes)},
                           NPC_CANVAS, NPC_HEIGHT)
        shared_set(nname, frames)
        header.append(f"#define MG_{nname.upper()}_FRAMES {len(frames)}u")
        header.append(f"#define MG_{nname.upper()}_W {NPC_CANVAS[0]}u")
        header.append(f"#define MG_{nname.upper()}_H {NPC_CANVAS[1]}u")
        print(f"  NPC {nname} compiled", flush=True)

    print("== 5. Compiling 6 Blight Guardians (96x96) ==", flush=True)
    boss_specs = [
        ("beetle", "chainsaw_beetle*.jpg", "white", [(40, 15, 480, 500), (510, 500, 740, 750)]),
        ("toad", "sludge_toad*.jpg", "white", [(5, 6, 349, 336), (430, 520, 740, 680)]),
        ("vulture", "iron_vulture*.jpg", "white", [(4, 4, 208, 177), (200, 180, 390, 347)]),
        ("jackal", "inferno_jackal*.jpg", "white", [(34, 23, 273, 183), (220, 350, 480, 520)]),
        ("owl", "blizzard_owl*.jpg", "white", [(13, 12, 258, 193), (400, 390, 620, 580)]),
        ("leviathan", "toxic_leviathan*.jpg", "white", [(7, 5, 384, 162), (170, 550, 470, 680)]),
        ("smoggar", "lord_smoggar*.jpg", "white", [(21, 12, 218, 301), (680, 290, 990, 560)]),
    ]

    boss_masters = {}
    for bname, pattern, bg, boxes in boss_specs:
        b_img = Image.open(find_file(pattern)).convert("RGB")
        frames = fit_group(b_img, {str(k): box for k, box in enumerate(boxes)},
                           BOSS_CANVAS, BOSS_HEIGHT, bg_color=bg, normalize_extent=True)
        boss_masters[bname] = shared_set(f"boss_{bname}", frames)
        print(f"  Boss {bname} compiled (96x96, 2 frames)", flush=True)

    # Three more valleys, three more guardians -- the same painted bodies in
    # new colours rather than new source photos: an eel wearing the
    # Leviathan's shape for the reef, a pale wyrm wearing the Toad's for the
    # cave, a spotted hyena wearing the Jackal's for the savanna. Tunable in
    # games/maiya/artbox/palette_config.json (see load_palette_config).
    reused_boss_tint = [
        (b["name"], b["source"], b["hue"], b["sat"], b["val"]) for b in palette_boss_cfg
    ]
    for new_name, source, shift, sat, val in reused_boss_tint:
        tinted = np.zeros((16, 3), dtype=np.uint8)
        tinted[1:] = hsv_map(boss_masters[source], lambda h, s, v, _s=shift, _a=sat, _b=val:
                             (h + _s, min(1.0, s * _a), min(1.0, v * _b)))
        header.append(c_array(f"mg_boss_{new_name}_pal", palette_words(tinted)))
        print(f"  Boss {new_name} recoloured from {source}", flush=True)

    print("== 6. Compiling Props, Hazards, Pickups and Decoration ==", flush=True)
    prop_boxes = {
        "chest": (680, 640, 830, 760),
        "chest_open": (840, 610, 990, 770),
        "spikes": (40, 840, 180, 980),
        "lava": (680, 840, 970, 970),
        "sludge": (360, 840, 600, 960),
    }
    props = {pname: crop_and_fit(allies_img, box, (32, 32), anchor="fill")
             for pname, box in prop_boxes.items()}
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
    items = {iname: crop_and_fit(allies_img, box, (32, 32), anchor="fill")
             for iname, box in item_boxes.items()}
    shared_set("item", items)
    for k, iname in enumerate(items.keys()):
        header.append(f"#define MG_I_{iname.upper()} {k}u")

    # Scenery the missions are dressed with: grass, blossoms, saplings, stone
    # lanterns, signposts, lily pads, waterfalls, climbing vines and doors.
    decor = {name: painter() for name, painter in nature_art.DECOR}
    shared_set("decor", decor)
    for k, name in enumerate(decor.keys()):
        header.append(f"#define MG_D_{name.upper()} {k}u")

    # Trinkets: coins, cut flowers, freed forest friends, extra lives and the
    # three power-ups Maiya eats on the run.
    trinkets = {name: painter() for name, painter in nature_art.TRINKETS}
    shared_set("trinket", trinkets)
    for k, name in enumerate(trinkets.keys()):
        header.append(f"#define MG_K_{name.upper()} {k}u")

    # The front plane: boulders and fronds that pass in front of the road.
    front = {"fern": nature_art.fern_frond()}
    for gname in ("grass", "moss", "sand", "autumn", "snow", "bark", "rust"):
        front[f"stone_{gname}"] = nature_art.standing_stone(gname)
    shared_set("front", front)
    for k, name in enumerate(front.keys()):
        header.append(f"#define MG_FR_{name.upper()} {k}u")

    # The Ancient Nature Gate, sealed and open (32 x 48).
    shared_set("gate", {"shut": nature_art.gate(False), "open": nature_art.gate(True)})

    # One ledge set per valley: left cap, middle, right cap.
    for gname in ("grass", "moss", "sand", "autumn", "snow", "bark", "rust"):
        blocks = {str(k): nature_art.ledge_block(gname, k) for k in range(3)}
        shared_set(f"block_{gname}", blocks)
    print("  Props, pickups, decoration and ledges compiled", flush=True)

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
        ("DRIP", TOOL_DRIP), ("LEAF", TOOL_LEAF),
        ("HALO", TOOL_HALO), ("DUST", TOOL_DUST), ("STAR", TOOL_STAR)
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
    title_base, title_pals = append("title", title_arr, banks=2, bank_base=41)

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
