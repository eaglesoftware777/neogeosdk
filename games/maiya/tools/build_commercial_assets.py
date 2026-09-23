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

EAGLE_CANVAS = (64, 48)
EAGLE_STRIPS = 3
EAGLE_ROWS = 2
EAGLE_HEIGHT = 42

BOSS_CANVAS = (128, 96)
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
        forced = np.zeros_like(is_bg)
    elif bg_color == "corner":
        # Generated sheets sit on a flat key colour (usually magenta) that
        # drifts a little between images and picks up JPEG fringing, so key
        # on this crop's own corner colour.
        corners = np.array([arr[0, 0], arr[0, -1], arr[-1, 0], arr[-1, -1]], dtype=np.int32)
        key = np.median(corners, axis=0)
        dist = np.sqrt(((arr.astype(np.int32) - key) ** 2).sum(axis=2))
        r, g, b = (arr[:, :, k].astype(np.int32) for k in range(3))
        is_bg = dist < 80
        forced = np.zeros_like(is_bg)
        if key[0] > 150 and key[2] > 120 and key[1] < 110:
            # A magenta key: the fringe it leaves on the outline is itself
            # magenta-tinted, so a looser pass may reach in from the
            # background through magenta-ish pixels only -- never through
            # the sprite's own warm or neutral colours.
            tinted = (r > g + 40) & (b > g + 40)
            lab2, _ = ndi.label(is_bg | (tinted & (dist < 170)))
            seeds = set(np.unique(lab2[is_bg])) - {0}
            is_bg = np.isin(lab2, list(seeds))
            # It also hides in holes the body encloses (the loop of a
            # coiled tail), which no fill from the border can reach.
            magenta = (r > g + 80) & (b > g + 60) & (np.abs(r - b) < 100)
            forced = magenta & (dist < 110)
            spill = magenta & ~is_bg & ~forced
            arr = arr.copy()
            arr[spill] = (40, 22, 32)
    else:
        is_bg = (arr[:, :, 0] <= tol) & (arr[:, :, 1] <= tol) & (arr[:, :, 2] <= tol)
        forced = np.zeros_like(is_bg)
    labeled, num = ndi.label(is_bg)
    border = np.concatenate([labeled[0, :], labeled[-1, :], labeled[:, 0], labeled[:, -1]])
    border_labels = set(np.unique(border)) - {0}
    bg_mask = np.isin(labeled, list(border_labels)) | forced
    alpha = np.where(bg_mask, 0, 255).astype(np.uint8)
    return Image.fromarray(np.dstack((arr, alpha)))


def add_shadow(frame, pad=2, half_h=2.4, spread=0.42):
    """A ground shadow tucked under the feet: a flat dark ellipse on the feet
    line, drawn only where the frame is empty, so the figure stands on the
    road instead of floating over it. Widest where the figure's stance is."""
    a = np.array(frame, dtype=np.uint8, copy=True)
    opaque = a[..., 3] > 0
    if not opaque.any():
        return a
    h, w = opaque.shape
    ys, xs = np.where(opaque)
    low = ys >= ys.max() - max(4, (ys.max() - ys.min()) // 4)
    cx = float(np.median(xs[low]))
    half_w = max(4.0, (xs.max() - xs.min()) * spread)
    feet = h - pad - 0.5
    yy, xx = np.mgrid[0:h, 0:w]
    shade = (((xx - cx) / half_w) ** 2 + ((yy - feet) / half_h) ** 2 <= 1.0) & ~opaque
    a[shade] = (24, 18, 30, 255)
    return a


def clean_sprite(a, white_area=None, island=24):
    """Strip what keying on a white sheet leaves behind.

    Flood-filling from the border only removes white that touches the edge,
    so a whip's motion smear painted in white survived as solid white blobs
    inside the frame, and slivers of neighbouring poses came along at the
    crop edges. Near-white regions bigger than a highlight, and small loose
    islands of pixels, are made transparent.
    """
    a = np.array(a, dtype=np.uint8, copy=True)
    rgb = a[..., :3].astype(np.int32)
    opaque = a[..., 3] > 0
    hi, lo = rgb.max(axis=2), rgb.min(axis=2)
    whiteish = opaque & (lo >= 200) & (hi - lo <= 40)
    labels, n = ndi.label(whiteish) if white_area else (None, 0)
    if n:
        sizes = ndi.sum(whiteish, labels, range(1, n + 1))
        big = [i + 1 for i, size in enumerate(sizes) if size >= white_area]
        a[np.isin(labels, big), 3] = 0
    opaque = a[..., 3] > 0
    labels, n = ndi.label(opaque, structure=np.ones((3, 3)))
    if n > 1:
        sizes = ndi.sum(opaque, labels, range(1, n + 1))
        main = int(np.argmax(sizes)) + 1
        edge = set(np.unique(np.concatenate([labels[:, :4].ravel(), labels[:, -4:].ravel(),
                                             labels[:4, :].ravel()]))) - {0}
        # Keep the body and anything sizeable beside it; a secondary piece
        # pressed against a side or the top edge is the next pose on the
        # sheet or a caption from it.
        keep = [i + 1 for i, size in enumerate(sizes)
                if i + 1 == main or (size >= island and (i + 1) not in edge)]
        a[~np.isin(labels, keep), 3] = 0
    return dehalo(a)


def dehalo(a):
    """Keying a white sheet leaves a grey rim wherever the sprite was
    blended into the paper; it reads as a dull halo. A light, colourless
    pixel on the edge of a sprite becomes the dark outline the art is
    drawn with. Coloured edges are left alone."""
    a = np.array(a, dtype=np.uint8, copy=True)
    opaque = a[..., 3] > 0
    rgb = a[..., :3].astype(np.int32)
    hi, lo = rgb.max(axis=2), rgb.min(axis=2)
    clear = np.pad(~opaque, 1, constant_values=True)
    h, w = opaque.shape
    touches = np.zeros_like(opaque)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            if dy or dx:
                touches |= clear[1 + dy:h + 1 + dy, 1 + dx:w + 1 + dx]
    halo = opaque & touches & (hi - lo < 40) & (hi > 80)
    a[halo, :3] = (28, 18, 22)
    return a


def tidy_face(a):
    """Scaling a small painted face leaves stray dark and grey pixels on the
    skin -- a smear down the nose, grey at the mouth. Any pixel that is
    mostly surrounded by skin, and isn't part of an eye, takes the skin
    tone."""
    a = np.array(a, dtype=np.uint8, copy=True)
    rgb = a[..., :3].astype(np.int32)
    opaque = a[..., 3] > 0
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    skin = opaque & (r > 190) & (g > 140) & (b > 90) & (r > g + 20) & (g > b + 10)
    shade = opaque & (r > 150) & (r < 200) & (g > 80) & (g < 130) & (b > 50) & (b < 100) & (r > g)
    if not skin.any():
        return a
    face = skin | shade
    tone = np.median(rgb[skin], axis=0).astype(np.uint8)
    h, w = face.shape
    for _ in range(3):
        pad = np.pad(face, 1)
        n = sum(pad[1 + dy:h + 1 + dy, 1 + dx:w + 1 + dx].astype(np.int32)
                for dy in (-1, 0, 1) for dx in (-1, 0, 1) if dy or dx)
        odd = opaque & ~face & (n >= 4) & ~(g > r)
        a[odd, :3] = tone
        face = face | odd
    # Lone shadow specks in the middle of the skin read as freckles.
    rgb = a[..., :3].astype(np.int32)
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    skin = opaque & (r > 190) & (g > 140) & (b > 90) & (r > g + 20) & (g > b + 10)
    pad = np.pad(skin, 1)
    n = sum(pad[1 + dy:h + 1 + dy, 1 + dx:w + 1 + dx].astype(np.int32)
            for dy in (-1, 0, 1) for dx in (-1, 0, 1) if dy or dx)
    a[opaque & ~skin & (n >= 6) & ~(g > r), :3] = tone
    return a


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


def find_poses(img, close=3):
    """The poses on a generated sheet, in reading order: flat key colour,
    each separate figure its own box, small effect pieces (sparks, smoke,
    a flame's tail) folded into the figure they belong to."""
    a = np.asarray(img.convert("RGB")).astype(np.int32)
    key = np.median([a[0, 0], a[0, -1], a[-1, 0], a[-1, -1], a[5, a.shape[1] // 2]], axis=0)
    fg = np.sqrt(((a - key) ** 2).sum(axis=2)) > 90
    fg = ndi.binary_opening(fg, iterations=1)
    m = ndi.binary_closing(fg, structure=np.ones((3, 3)), iterations=close)
    lab, n = ndi.label(m)
    sizes = ndi.sum(m, lab, range(1, n + 1))
    big = [i for i, sz in enumerate(sizes) if sz >= sizes.max() * 0.06]
    boxes = {}
    for i in big:
        ys, xs = np.where(lab == i + 1)
        boxes[i] = [int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1]
    for i, sz in enumerate(sizes):
        if i in boxes or sz <= 60:
            continue
        ys, xs = np.where(lab == i + 1)
        cx, cy = xs.mean(), ys.mean()
        def gap(j):
            b = boxes[j]
            return max(0, b[0] - cx, cx - b[2]) + max(0, b[1] - cy, cy - b[3])
        best = min(boxes, key=gap)
        if gap(best) < 40:
            b = boxes[best]
            boxes[best] = [min(b[0], int(xs.min())), min(b[1], int(ys.min())),
                           max(b[2], int(xs.max()) + 1), max(b[3], int(ys.max()) + 1)]
    bl = list(boxes.values())
    areas = sorted((b[2] - b[0]) * (b[3] - b[1]) for b in bl)
    med = areas[len(areas) // 2]
    bl = [b for b in bl if (b[2] - b[0]) * (b[3] - b[1]) >= med * 0.18]
    # A box far wider than the rest swallowed two neighbouring poses:
    # split it at its emptiest column.
    widths = sorted(b[2] - b[0] for b in bl)
    mw = widths[len(widths) // 2]
    split = []
    for b in bl:
        w = b[2] - b[0]
        if w > 1.65 * mw:
            col = fg[b[1]:b[3], b[0]:b[2]].sum(axis=0)
            lo, hi = int(w * 0.3), int(w * 0.7)
            cut = b[0] + lo + int(np.argmin(col[lo:hi]))
            split += [[b[0], b[1], cut, b[3]], [cut, b[1], b[2], b[3]]]
        else:
            split.append(b)
    bl = sorted(split, key=lambda b: (b[1] + b[3]) / 2)
    rows = []
    for b in bl:
        cy = (b[1] + b[3]) / 2
        if rows and abs(cy - rows[-1][0]) < (b[3] - b[1]) * 0.45:
            rows[-1][1].append(b)
        else:
            rows.append([cy, [b]])
    out = []
    for _, rb in rows:
        out += sorted(rb, key=lambda b: b[0])
    return [tuple(b) for b in out]


def _cutout_bbox(img, box, bg_color):
    """Crop a source box, drop its flat background, return the tight sprite."""
    cut = extract_cutout(img.crop(box), bg_color=bg_color)
    arr = np.array(cut)
    alpha = arr[:, :, 3] >= 128
    if not alpha.any():
        return None
    # At source resolution, before anything is scaled: keep the body and any
    # sizeable piece standing apart from it (a spit, a splash, a thrown
    # rock), but drop captions and specks, and slivers of the neighbouring
    # pose that the crop box clipped at its border. Dropping them before
    # the bounding box is taken also stops them shrinking the sprite.
    labels, n = ndi.label(alpha, structure=np.ones((3, 3)))
    if n > 1:
        sizes = ndi.sum(alpha, labels, range(1, n + 1))
        main = int(np.argmax(sizes)) + 1
        border = set(np.unique(np.concatenate([labels[0, :], labels[-1, :],
                                               labels[:, 0], labels[:, -1]]))) - {0}
        keep = [i + 1 for i, size in enumerate(sizes)
                if i + 1 == main or (size >= sizes[main - 1] * 0.02 and (i + 1) not in border)]
        drop = ~np.isin(labels, keep)
        arr[drop, 3] = 0
        cut = Image.fromarray(arr)
        alpha = arr[:, :, 3] >= 128
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
              normalize_extent=False, clip_tall=False):
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
    # Normally the tallest pose sets a ceiling on the scale. With clip_tall
    # the body keeps its size and a raised weapon's tip is cropped instead.
    if not clip_tall:
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
        # Every animated sprite loses the loose specks and slivers of
        # neighbouring poses a crop box drags in; they read as flicker.
        frames[name] = clean_sprite(out)
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

    # --- Repaints: the first versions of these read as the wrong thing (a
    # rice-ball heart, a fried-egg spark, grey checkered dust, a floppy
    # disk for trash). Palette: 1 cream, 2 pink, 3 deep rose, 4 leaf,
    # 5 leaf shadow, 6 orange, 7 pale gold, 8 grey, 9 dark grey, 12 outline,
    # 13 red, 15 gold.
    yy, xx = np.mgrid[0:16, 0:16]

    heart[:] = 0
    heart_art = (
        "................",
        "................",
        "...rrr....rrr...",
        "..rrrrr..rrrrr..",
        ".rwwrrrrrrrrrrd.",
        ".rwwrrrrrrrrrrd.",
        ".rwrrrrrrrrrrdd.",
        ".rrrrrrrrrrrrdd.",
        "..rrrrrrrrrrdd..",
        "...rrrrrrrrdd...",
        "....rrrrrrdd....",
        ".....rrrrdd.....",
        "......rrdd......",
        ".......dd.......",
        "................",
        "................",
    )
    pens = {"r": 13, "d": 3, "w": 1}
    for i, row in enumerate(heart_art):
        for j, ch in enumerate(row):
            if ch in pens:
                heart[i, j] = pens[ch]

    spark[:] = 0
    for k in range(-6, 7):
        spark[7 + k, 7] = 15 if abs(k) > 2 else 7
        spark[7, 7 + k] = 15 if abs(k) > 2 else 7
    for k in range(-3, 4):
        spark[7 + k, 7 + k] = 6 if abs(k) > 1 else 7
        spark[7 + k, 7 - k] = 6 if abs(k) > 1 else 7
    spark[6:9, 6:9] = 1

    petal[:] = 0
    body = ((xx - 8) / 3.4) ** 2 + ((yy - 8) / 6.0) ** 2 <= 1.0
    petal[body] = 2
    petal[body & (xx > 8)] = 3
    petal[4:9, 6] = 1

    leaf[:] = 0
    body = ((xx - 8) / 3.6) ** 2 + ((yy - 8) / 6.4) ** 2 <= 1.0
    leaf[body] = 4
    leaf[body & (xx > 8)] = 5
    leaf[2:14, 8] = 5

    dust[:] = 0
    for cx, cy, r in ((4, 11, 3.2), (9, 9, 3.8), (13, 12, 2.8)):
        puff = (xx - cx) ** 2 + (yy - cy) ** 2 <= r * r
        dust[puff] = 7
        dust[puff & ((xx - cx) + (yy - cy) > 1)] = 15
    dust[5, 12] = 1
    dust[3, 6] = 1
    dust[8, 2] = 1

    trash[:] = 0
    trash[3:13, 4:12] = 8
    trash[3:13, 9:12] = 9
    trash[6:9, 4:12] = 13
    trash[3, 4:12] = 1
    trash[12, 5:11] = 9
    trash[8:10, 3] = 8          # the crush in its side

    # A dark rim on every projectile and effect, so each one reads cleanly
    # against any background.
    for column in (TOOL_THORN0, TOOL_THORN1, TOOL_TRASH, TOOL_SPIT, TOOL_BOLT, TOOL_FIRE,
                   TOOL_ICE, TOOL_OIL, TOOL_HEART, TOOL_ROSE, TOOL_PETAL, TOOL_DRIP, TOOL_LEAF):
        c = cell(column)
        solid = c > 0
        pad = np.pad(solid, 1)
        edge = np.zeros_like(solid)
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                if dy or dx:
                    edge |= pad[1 + dy:17 + dy, 1 + dx:17 + dx]
        c[edge & ~solid] = 12

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

    # Each guardian's own arena, shown while it fights: the painting fitted
    # to the 320-pixel screen (its floor on the road strip), mirrored out to
    # the 512-pixel layer width. Arena k follows the MG_B_* order.
    arena_files = ("arena_beetle.png", "arena_toad.png", "arena_leviathan.png", "arena_jackal.png",
                   "arena_owl.png", "arena_smoggar.png", "arena_vulture.png", "arena_eel.png",
                   "arena_wyrm_0.jpg", "arena_hyena.jpg")
    arenas_built = 0
    for k, fname in enumerate(arena_files):
        path = SOURCE / fname
        if not path.is_file():
            header.append(f"#define MG_ARENA{k}_READY 0u")
            continue
        art = Image.open(path).convert("RGBA")
        scale = max(320 / art.width, 224 / art.height)
        art = art.resize((max(320, round(art.width * scale)), max(224, round(art.height * scale))),
                         Image.Resampling.LANCZOS)
        x0 = (art.width - 320) // 2
        art = np.asarray(art.crop((x0, art.height - 224, x0 + 320, art.height)))
        panel = np.zeros((224, 512, 4), dtype=np.uint8)
        panel[:, :320] = art
        panel[:, 320:] = art[:, ::-1][:, :192]
        panel[:, :, 3] = 255
        indices, palettes, assignments = quantize(panel, 16, None, dither="none")
        store(f"arena{k}", indices[:192], palettes, assignments[:12], panel[:192])
        store(f"arenaground{k}", indices[192:], palettes, assignments[12:], panel[192:])
        header.append(c_array(f"mg_arena{k}_pal", [v for p in palettes for v in palette_words(p)]))
        header.append(c_array(f"mg_arena{k}_map", assignments[:12].flatten() + 16, "uint8_t"))
        header.append(c_array(f"mg_arenaground{k}_map", assignments[12:].flatten() + 16, "uint8_t"))
        header.append(f"#define MG_ARENA{k}_BANKS {len(palettes)}u")
        header.append(f"#define MG_ARENA{k}_READY 1u")
        arenas_built += 1
    print(f"  {arenas_built} guardian arenas compiled", flush=True)

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
    hf = {name: clean_sprite(f, white_area=10) for name, f in hf.items()}

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

    # Grounded poses stand on a shadow; in the air she doesn't.
    for name in list(maiya_frames):
        if not (name.startswith("jump") or name.startswith("hurt") or name == "spin"):
            maiya_frames[name] = add_shadow(maiya_frames[name])

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
    # girl.  Blonde and green becomes brown-haired and blue -- classed by
    # hue and saturation rather than by index, so it survives any future
    # repaint of the source art.  Skin, the rose whip and outlines are left
    # exactly as painted; only the hair and the dress move.
    #
    # The 15-color hero palette reuses a couple of warm mid/shadow tones
    # across hair AND skin/boots (there's no spare palette space to give
    # them separate entries), so this can't isolate "hair only" -- shifting
    # those shared entries nudges a few skin/boot shadow pixels the same
    # direction as the hair. That reads as a slightly warmer shadow, not a
    # wrong color, and is far less visible than the alternative: leaving
    # the lower end of the blonde hue range (~15-24, this source art's
    # darker hair shadow) untouched, which used to show up as stray
    # blonde/orange patches in an otherwise recolored head.
    def alt_tint(h, s, v):
        if 15.0 <= h <= 65.0 and s > 0.5:             # blonde hair -> brown
            return (25.0, min(1.0, s * 1.3), v * 0.6)
        if 80.0 <= h <= 170.0:                        # green dress -> blue
            return (226.0, min(1.0, s * 1.05), v)
        return (h, s, v)
    alt_pal = np.zeros((16, 3), dtype=np.uint8)
    alt_pal[1:] = hsv_map(hero_master, alt_tint)
    header.append(c_array("mg_hero_alt_pal", palette_words(alt_pal)))

    # Eagle (guardian sun bird): its own white-and-gold painting -- wings
    # up, spread, down and a level glide -- rather than the Iron Vulture's
    # body shrunk down, which read as a brown smudge.
    eagle_src = Image.open(find_file("flying_mount_eagle*.jpg")).convert("RGB")
    up, spread, down, glide = (21, 19, 451, 465), (524, 203, 1282, 465), \
        (65, 475, 651, 737), (693, 509, 1351, 737)
    eagle_frames = fit_group(eagle_src, {
        "perch0": glide, "perch1": spread, "perch2": glide,
        "fly0": up, "fly1": spread, "fly2": down,
    }, EAGLE_CANVAS, EAGLE_HEIGHT, bg_color="corner")
    shared_set("eagle", eagle_frames)

    # Face HUD icon and portrait
    # The select-screen portraits: the painted pair (Maiya, Luna) when it
    # exists, otherwise Maiya cleaned up from the moveset sheet and Luna
    # recoloured from her. The small HUD/face icon is taken from the
    # portrait's head rather than the tiny in-game sprite, so it keeps real
    # eyes and a face instead of a smudge.
    pair = SOURCE / "faces_select.jpg"
    luna_painted = None
    if pair.is_file():
        faces_img = Image.open(pair).convert("RGB")
        w2 = faces_img.width // 2
        busts = fit_group(faces_img, {"maiya": (0, 0, w2, faces_img.height),
                                      "luna": (w2, 0, faces_img.width, faces_img.height)},
                          (96, 96), 92, bg_color="corner")
        portrait = dehalo(np.asarray(busts["maiya"]))
        luna_painted = dehalo(np.asarray(busts["luna"]))
    else:
        portrait = dehalo(tidy_face(crop_and_fit(m_img, (27, 70, 144, 268), (96, 96), anchor="center")))
    # Centre the icon on her face (the skin), with a little hair around it.
    rgb = portrait[..., :3].astype(np.int32)
    skin = (portrait[..., 3] > 0) & (rgb[..., 0] > 190) & (rgb[..., 1] > 130) & (rgb[..., 0] > rgb[..., 2] + 40)
    ys, xs = np.where(skin[:64])
    cy, cx = int(np.median(ys)), int(np.median(xs))
    side = 50
    y0 = max(0, cy - side // 2 - 4)
    head = Image.fromarray(portrait).crop((cx - side // 2, y0, cx + side // 2, y0 + side)).resize((32, 32), Image.Resampling.LANCZOS)
    def face_icon(img):
        h = np.array(img)
        h[..., 3] = np.where(h[..., 3] >= 128, 255, 0)
        return dehalo(tidy_face(h))
    face = face_icon(head)
    # Its own palette: squeezed into the sprite's few skin tones the face
    # came out blotchy.
    append("face", face)

    append("portrait", portrait, banks=2, bank_base=41)
    # Luna's own portrait for the select screen, beside Maiya's: the same
    # painting through the same hair and dress recolour as her sprites.
    if luna_painted is not None:
        luna = luna_painted
    else:
        luna = portrait.copy()
        lit = luna[..., 3] > 0
        luna[lit, :3] = hsv_map(luna[lit, :3], alt_tint)
    append("portrait_alt", luna, banks=2, bank_base=62)
    luna_head = Image.fromarray(np.ascontiguousarray(luna)).crop(
        (cx - side // 2, y0, cx + side // 2, y0 + side)).resize((32, 32), Image.Resampling.LANCZOS)
    append("face_alt", face_icon(luna_head))
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
            # Its own six-pose sheet: four wing-beats, a folded dive and a
            # perch, instead of two stiff frames that snapped between.
            "canvas": (48, 32), "height": 30, "lift": 1.2,
            "sheet": "crow_moves.jpg", "poses": 6,
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
        sheet = SOURCE / spec["sheet"] if "sheet" in spec else None
        if sheet is not None and sheet.is_file():
            src = Image.open(sheet).convert("RGB")
            found = find_poses(src)[:spec["poses"]]
            frames = fit_group(src, {str(k): box for k, box in enumerate(found)},
                               spec["canvas"], spec["height"], bg_color="corner")
        else:
            boxes = {str(k): box for k, box in enumerate(spec["boxes"])}
            frames = fit_group(en_img, boxes, spec["canvas"], spec["height"])
        if "lift" in spec:
            # Night-black art turns to mud at 26 px: lift it until the
            # silhouette reads against a dark canopy.
            for name, f in frames.items():
                rgb = np.clip(f[:, :, :3].astype(np.float32) * spec["lift"] + 14.0, 0, 255)
                frames[name] = np.dstack((rgb.astype(np.uint8), f[:, :, 3]))
        if cname in ("slime", "beetle", "goblin", "worm"):
            frames = {k: add_shadow(f) for k, f in frames.items()}
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

    print("== 5. Compiling 10 Blight Guardians (128x96, 8 poses) ==", flush=True)
    # Each guardian's move sheet, eight poses in this order: idle, walk,
    # wind-up, attack, special, jump, hurt, defeated. A pose is either an
    # index into the poses found on the sheet (reading order) or a box; a
    # pair of boxes is one pose drawn in two pieces (a body and its breath).
    I = lambda k: k
    boss_sheets = {
        "beetle": ("boss_beetle_moves.jpg", [(13, 231, 288, 509), (292, 258, 497, 505), (501, 296, 751, 507),
                   (756, 284, 1022, 505), (30, 610, 300, 895), (300, 535, 545, 875), (530, 590, 760, 895),
                   (675, 845, 1022, 980)]),
        "toad": ("boss_toad_moves2.jpg", [I(0), I(1), I(2), I(3), I(3), I(5), I(6), I(7)]),
        "leviathan": ("boss_leviathan_moves.jpg", [I(0), I(1), I(2), I(3), I(4), I(5), I(6), I(7)]),
        "jackal": ("boss_jackal_moves.jpg", [I(0), I(1), I(3), I(2), (408, 310, 1004, 509), I(7), I(8), I(9)]),
        "owl": ("boss_owl_moves.jpg", [I(0), I(1), I(2), I(3), I(4), I(5), I(6), I(7)]),
        "smoggar": ("boss_smoggar_moves.jpg", [(50, 135, 235, 440), (285, 140, 470, 440), (505, 150, 690, 440),
                    (650, 110, 1010, 440), (10, 600, 335, 950), (330, 545, 510, 915), (515, 580, 735, 950),
                    (735, 830, 1015, 975)]),
        "vulture": ("boss_vulture_moves.jpg", [I(0), I(1), I(3), I(4), (53, 501, 595, 689), I(7), I(8), I(9)]),
        "eel": ("boss_eel_moves.jpg", [I(0), I(1), I(2), I(3), (41, 366, 364, 715), I(6), I(7), I(8)]),
        "wyrm": ("boss_wyrm_moves.jpg", [I(0), I(1), I(2), I(3), I(3), I(5), I(6), I(7)]),
        "hyena": ("boss_hyena_moves.jpg", [I(0), I(1), I(2), I(3), I(4), I(5), I(6), I(7)]),
    }
    pose_names = ("idle", "walk", "windup", "attack", "special", "jump", "hurt", "dead")
    boss_masters = {}
    for bname, (fname, spec) in boss_sheets.items():
        b_img = Image.open(SOURCE / fname).convert("RGB")
        found = find_poses(b_img)
        boxes = {}
        for k, item in enumerate(spec):
            boxes[pose_names[k]] = found[item] if isinstance(item, int) else item
        frames = fit_group(b_img, boxes, BOSS_CANVAS, BOSS_HEIGHT, bg_color="corner", clip_tall=True)
        assert len(frames) == 8, bname
        fliers = bname in ("owl", "vulture")
        for pose in frames:
            if pose == "dead" or (not fliers and pose != "jump"):
                frames[pose] = add_shadow(frames[pose], half_h=3.2)
        boss_masters[bname] = shared_set(f"boss_{bname}", frames)
        print(f"  Boss {bname} compiled (128x96, 8 poses)", flush=True)
    for k, name in enumerate(pose_names):
        header.append(f"#define MG_BF_{name.upper()} {k}u")
    header.append(f"#define MG_BOSS_FRAMES {len(pose_names)}u")

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

    # Ground hazards on their own palette, two frames each for the game to
    # alternate: flickering fire, bubbling sludge, a leaking drum, spikes.
    hazards = {name: painter() for name, painter in nature_art.HAZARDS}
    shared_set("hazard", hazards)
    for k, name in enumerate(hazards.keys()):
        header.append(f"#define MG_HZ_{name.upper()} {k}u")

    # Pits, one art set and palette per theme: water, toxic, fire, void.
    for theme in ("water", "toxic", "fire", "void"):
        shared_set(f"pit_{theme}", nature_art.pit_blocks(theme))

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
    # The redrawn key visual when it's there (same composition, sharper,
    # and without the coin prompts painted into it); the original otherwise.
    title_hq = SOURCE / "title_hq.jpg"
    title_raw = Image.open(title_hq if title_hq.is_file() else find_file("maiya_title*.jpg")).convert("RGBA")
    title_fit = title_raw.resize((304, 224), Image.Resampling.LANCZOS)
    title_arr = np.asarray(title_fit, dtype=np.uint8)
    # Sixteen palettes, one chosen per tile, instead of two for the whole
    # picture: the title is the first thing anyone sees and 30 colours made
    # it muddy. It borrows the stage-background banks, idle on the title.
    title_base, title_pals = append("title", title_arr, banks=16, bank_base=16)
    header.append("#define MG_TITLE_PAL_BANK 16u")

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
