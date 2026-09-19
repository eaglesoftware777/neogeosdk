"""Build the Maiya artwork with the SDK's palette and tile codecs.

    python3 games/maiya/tools/prepare_assets.py

Inputs: the six environment paintings in assets/environments.png, the
SDK's own character set (games/demo/artbox/in/characters) for Maiya, her
brother and the sun bird, and the CC0 Kenney sets staged in assets/kenney
for the corrupted creatures, the Blight Syndicate machines, the villagers,
the nature spirits, the seven guardians (assembled from monster parts and
animal heads), the pick-ups, the ground blocks and the road furniture.

Outputs: the C-ROM pair, a header of tile bases, palettes and maps, and a
preview of every asset exactly as the hardware will show it.
"""

from __future__ import annotations

import colorsys
import io
import json
from pathlib import Path
import sys

import numpy as np
from PIL import Image

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "artbox"))

from palette_banks import fit_palette, palette_words, quantize, reconstruct, training_mask  # noqa: E402
from tile_codec import encode_image, decode_image, write_utility_tiles  # noqa: E402

CHARACTERS = ROOT / "games/demo/artbox/in/characters"
KENNEY = GAME / "assets/kenney"

# Maiya's moveset, by source frame number.  Every frame is drawn on the
# same 112 x 96 canvas at the same scale, with the body centred and the
# feet on the bottom row, so the engine can swap frames without the
# figure sliding.  Rose arcs painted into a frame extend in front of it.
HERO_FRAMES = [
    ("idle0", 1), ("idle1", 2), ("idle2", 3),
    ("walk0", 4), ("walk1", 5), ("walk2", 6), ("walk3", 7),
    ("walk4", 8), ("walk5", 9), ("walk6", 10), ("walk7", 11),
    ("crouch", 12),
    ("run0", 13), ("run1", 15), ("run2", 16),
    ("jump0", 17), ("jump1", 18), ("jump2", 19), ("jump3", 20), ("jump4", 21), ("land", 22),
    ("atk0", 23), ("atk1", 24), ("atk2", 27), ("atk3", 26),
    ("sweep0", 33), ("sweep1", 34), ("sweep2", 36), ("sweep3", 35),
    ("low", 31), ("spin", 32),
    ("cast0", 42), ("cast1", 43), ("cast2", 53),
    ("hurt0", 56), ("hurt1", 58), ("down", 55), ("sit", 59), ("win", 62),
]
EAGLE_FRAMES = [("perch0", 73), ("perch1", 74), ("perch2", 75),
                ("fly0", 76), ("fly1", 77), ("fly2", 78)]
HERO_SCALE = 0.22
HERO_CANVAS = (112, 96)
HERO_FRAME_SCALE = {
    "sweep0": 0.24, "sweep1": 0.24, "sweep2": 0.24, "sweep3": 0.24, "low": 0.24,
    "spin": 0.23, "cast0": 0.255, "cast1": 0.255, "cast2": 0.24,
    "hurt0": 0.24, "hurt1": 0.25, "down": 0.24, "sit": 0.24, "win": 0.235,
}
EAGLE_CANVAS = (64, 48)

# The corrupted creatures and the people, from the Kenney sets: name,
# canvas, scale, tint (hue, saturation, value) and frames.
CREATURES = {
    "slime":  dict(canvas=(48, 32), scale=0.40, tint=(280, 0.55, 0.85), dir="enemies",
                   frames=["slimePurple", "slimePurple_move", "slimePurple_hit", "slimePurple_dead"]),
    "beetle": dict(canvas=(64, 48), scale=0.95, tint=(22, 0.75, 0.80), dir="enemies",
                   frames=["ladyBug", "ladyBug_walk", "ladyBug_fly"]),
    "crow":   dict(canvas=(64, 48), scale=0.95, tint=(260, 0.30, 0.35), dir="enemies",
                   frames=["fly", "fly_fly"]),
    "bat":    dict(canvas=(80, 48), scale=0.85, tint=(120, 0.55, 0.75), dir="enemies",
                   frames=["bat_fly", "bat", "bat_hang"]),
    "goblin": dict(canvas=(48, 64), scale=0.50, tint=None, dir="people",
                   frames=["character_zombie_idle", "character_zombie_walk0", "character_zombie_walk2",
                           "character_zombie_walk4", "character_zombie_walk6", "character_zombie_attack1",
                           "character_zombie_hurt"]),
    "worm":   dict(canvas=(32, 80), scale=0.50, tint=(20, 0.65, 0.75), dir="enemies",
                   frames=["snakeLava", "snakeLava_ani"]),
    "roots":  dict(canvas=(48, 64), scale=0.95, tint=(290, 0.50, 0.55), dir="enemies",
                   frames=["barnacle", "barnacle_bite"]),
    "gear":   dict(canvas=(64, 64), scale=0.95, tint=(30, 0.35, 0.75), dir="enemies",
                   frames=["spinner", "spinner_spin"]),
    "robot":  dict(canvas=(80, 64), scale=0.36, tint=None, dir="robots",
                   frames=["robot_redDrive1", "robot_redDrive2", "robot_redJump", "robot_redHurt"]),
    "spider": dict(canvas=(80, 64), scale=0.95, tint=(230, 0.35, 0.30), dir="enemies",
                   frames=["spider_walk1", "spider_walk2"]),
    "spirit": dict(canvas=(48, 64), scale=0.70, tint=(150, 0.35, 1.05), dir="enemies",
                   frames=["ghost_normal"]),
    "elder":  dict(canvas=(48, 64), scale=0.50, tint=None, dir="people",
                   frames=["character_malePerson_idle", "character_malePerson_talk"]),
    "girl":   dict(canvas=(48, 64), scale=0.50, tint=None, dir="people",
                   frames=["character_femalePerson_idle", "character_femalePerson_talk", "character_femalePerson_cheer0"]),
    "sunboy": dict(canvas=(48, 64), scale=0.50, tint=(45, 0.6, 1.05), dir="people",
                   frames=["character_maleAdventurer_idle", "character_maleAdventurer_cheer0", "character_maleAdventurer_cheer1"]),
}

# The ground blocks each world builds its ledges from, at 32 x 32.
GROUNDS = ["grass", "sand", "stone", "planet", "snow", "dirt"]
BOSSES = ["beetle", "toad", "vulture", "jackal", "owl", "leviathan", "smoggar"]

# Tool tiles, by column of the 16-row tool sheet (two rows of tiles).
TOOL_THORN0, TOOL_THORN1, TOOL_TRASH, TOOL_SPIT, TOOL_BOLT, TOOL_FIRE, TOOL_ICE, TOOL_OIL = range(8)
TOOL_SPARK, TOOL_HEART, TOOL_ROSE, TOOL_PETAL, TOOL_LANE, TOOL_CURSOR, TOOL_DRIP, TOOL_LEAF = range(8, 16)
TOOL_COLUMNS = 16


def c_array(name, values, ctype="uint16_t"):
    values = list(values)
    lines = [f"static const {ctype} {name}[{len(values)}] = {{"]
    for i in range(0, len(values), 16):
        lines.append("    " + ", ".join(str(int(v)) for v in values[i:i + 16]) + ",")
    return "\n".join(lines + ["};", ""])


def source(index):
    path = next(CHARACTERS.glob(f"sprite_{index:03d}_*.png"))
    return Image.open(path).convert("RGBA")


def kenney(sub, name):
    return Image.open(KENNEY / sub / f"{name}.png").convert("RGBA")


def body_mask(rgba):
    """Opaque pixels that belong to the figure rather than an arc."""
    rgb = rgba[:, :, :3].astype(np.int16)
    r, g, b = rgb[:, :, 0], rgb[:, :, 1], rgb[:, :, 2]
    pale = (b > 150) & (b >= r + 20) & (g > 120) & ((r + g + b) > 420)
    return (rgba[:, :, 3] >= 128) & ~pale


def fit_frame(image, scale, canvas, anchor="feet"):
    """Scale a source frame and place it on a fixed canvas, body centred,
    lowest opaque row two pixels above the bottom edge."""
    w, h = image.size
    scaled = image.resize((max(1, round(w * scale)), max(1, round(h * scale))),
                          Image.Resampling.LANCZOS)
    rgba = np.asarray(scaled)
    alpha = rgba[:, :, 3] >= 128
    mask = body_mask(rgba) if anchor == "feet" else alpha
    if not mask.any():
        mask = alpha
    rows = np.where(mask.any(axis=1))[0]
    cols = np.where(mask.any(axis=0))[0]
    cx = (cols[0] + cols[-1] + 1) // 2
    bottom = rows[-1] + 1
    frame = Image.new("RGBA", canvas, (0, 0, 0, 0))
    if anchor == "fill":
        # Blocks and props: centred, flush with the bottom edge.
        ox = (canvas[0] - scaled.width) // 2
        oy = canvas[1] - scaled.height
    else:
        ox = canvas[0] // 2 - cx
        oy = canvas[1] - 2 - bottom
    frame.paste(scaled, (int(ox), int(oy)))
    out = np.asarray(frame).copy()
    visible = int(alpha.sum())
    kept = int((out[:, :, 3] >= 128).sum())
    if kept < visible - 8:
        raise SystemExit(f"frame does not fit its canvas ({visible - kept} px lost)")
    out[:, :, 3] = np.where(out[:, :, 3] >= 128, 255, 0)
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


def tint_image(image, hue, sat, val):
    """Recolour a cartoon sprite: every lit pixel takes the hue, keeps its
    shading, and scales its saturation and value."""
    a = np.asarray(image).astype(np.float32) / 255.0
    rgb = a[:, :, :3]
    mx = rgb.max(axis=2)
    mn = rgb.min(axis=2)
    v = mx
    s = np.where(mx > 0, (mx - mn) / np.maximum(mx, 1e-6), 0)
    new_s = np.clip(np.where(s > 0.08, sat, s), 0, 1)
    new_v = np.clip(v * val, 0, 1)
    h = np.full_like(v, hue / 360.0)
    i = np.floor(h * 6).astype(int)
    f = h * 6 - i
    p = new_v * (1 - new_s)
    q = new_v * (1 - new_s * f)
    t = new_v * (1 - new_s * (1 - f))
    i = i % 6
    r = np.select([i == 0, i == 1, i == 2, i == 3, i == 4, i == 5], [new_v, q, p, p, t, new_v])
    g = np.select([i == 0, i == 1, i == 2, i == 3, i == 4, i == 5], [t, new_v, new_v, q, p, p])
    b = np.select([i == 0, i == 1, i == 2, i == 3, i == 4, i == 5], [p, p, t, new_v, new_v, q])
    dark = v < 0.22
    out = np.dstack((np.where(dark, rgb[:, :, 0], r), np.where(dark, rgb[:, :, 1], g),
                     np.where(dark, rgb[:, :, 2], b), a[:, :, 3]))
    return Image.fromarray((out * 255).astype(np.uint8))


def costume(master, outfit_hue, outfit_sat, arc_hue, hair="gold", glow=False):
    """Maiya's palette from the character set: the navy outfit takes the
    nature green, the pale arcs become rose petals, the hair stays gold."""
    def fn(h, s, v):
        skin = 10 <= h <= 45 and 0.2 <= s <= 0.75 and v >= 0.45
        if skin:
            return (h, s, min(1.0, v * 1.05)) if glow else (h, s, v)
        pale = 170 <= h <= 250 and s < 0.45 and v >= 0.7
        if pale:
            return arc_hue, 0.55, v
        blue = 185 <= h <= 265 and s >= 0.15
        if blue:
            return outfit_hue, outfit_sat if v > 0.2 else outfit_sat * 0.6, v * (1.15 if glow else 1.0)
        gold = 30 <= h <= 60 and s >= 0.3 and v >= 0.5
        if gold and hair == "silver":
            return 210, 0.08, min(1.0, v * 1.05)
        return h, s, v
    return hsv_map(master, fn)


def corrupt(colors, kind):
    """The Blight over a palette: drained, tinted and darkened.  Restored
    palettes are the paintings as they are; these are what each world
    looks like before its guardian falls."""
    hue, sat, val = {"smog": (275, 0.30, 0.62), "toxic": (85, 0.45, 0.66),
                     "rust": (22, 0.45, 0.70), "oil": (225, 0.35, 0.52), "ash": (285, 0.25, 0.48)}[kind]
    def fn(h, s, v):
        return hue, min(1.0, sat * (0.6 + s)), v * val
    return hsv_map(colors, fn)


def compose(parts, size=(320, 320)):
    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    for part in parts:
        im = part["im"]
        scale = part.get("s", 1.0)
        if scale != 1.0:
            im = im.resize((max(1, int(im.width * scale)), max(1, int(im.height * scale))), Image.Resampling.LANCZOS)
        if part.get("flip"):
            im = im.transpose(Image.FLIP_LEFT_RIGHT)
        canvas.alpha_composite(im, (int(part["x"] - im.width / 2), int(part["y"] - im.height / 2)))
    return canvas


def boss_art(name, frame):
    """A guardian assembled from monster parts and an animal head.  Frame 1
    raises the arms and opens the mouth."""
    M = lambda n: kenney("monster", n)  # noqa: E731
    A = lambda n: kenney("animals", n)  # noqa: E731
    E = lambda n: kenney("enemies", n)  # noqa: E731
    C = 160
    up = 0 if frame == 0 else -45
    if name == "beetle":
        parts = [dict(im=M("leg_darkA"), x=C - 45, y=250, s=0.7), dict(im=M("leg_darkA"), x=C + 45, y=250, s=0.7, flip=True),
                 dict(im=tint_image(M("body_darkA"), 25, 0.55, 0.9), x=C, y=170, s=0.95),
                 dict(im=E("saw"), x=C - 105, y=150 + up, s=0.75), dict(im=E("saw"), x=C + 105, y=150 + up, s=0.75),
                 dict(im=M("eye_angry_red"), x=C - 30, y=140, s=0.8), dict(im=M("eye_angry_red"), x=C + 30, y=140, s=0.8, flip=True),
                 dict(im=M("mouth_closed_teeth" if frame == 0 else "mouthJ"), x=C, y=200, s=0.9),
                 dict(im=M("detail_dark_antenna_small"), x=C - 40, y=90, s=0.9), dict(im=M("detail_dark_antenna_small"), x=C + 40, y=90, s=0.9, flip=True)]
    elif name == "toad":
        parts = [dict(im=M("leg_greenD"), x=C - 60, y=245, s=0.7), dict(im=M("leg_greenD"), x=C + 60, y=245, s=0.7, flip=True),
                 dict(im=tint_image(M("body_greenB"), 95, 0.6, 0.8), x=C, y=170, s=1.05),
                 dict(im=M("eye_yellow"), x=C - 42, y=120, s=1.0), dict(im=M("eye_yellow"), x=C + 42, y=120, s=1.0, flip=True),
                 dict(im=M("mouth_closed_happy" if frame == 0 else "mouthE"), x=C, y=195, s=1.3),
                 dict(im=M("arm_greenB"), x=C - 90, y=190 + up, s=0.6), dict(im=M("arm_greenB"), x=C + 90, y=190 + up, s=0.6, flip=True)]
    elif name == "vulture":
        parts = [dict(im=M("leg_darkC"), x=C - 35, y=255, s=0.6), dict(im=M("leg_darkC"), x=C + 35, y=255, s=0.6, flip=True),
                 dict(im=tint_image(M("body_darkD"), 210, 0.12, 0.9), x=C, y=185, s=0.8),
                 dict(im=M("arm_darkE"), x=C - 95, y=170 + up, s=0.75), dict(im=M("arm_darkE"), x=C + 95, y=170 + up, s=0.75, flip=True),
                 dict(im=tint_image(A("parrot"), 15, 0.55, 0.85), x=C, y=110, s=0.85)]
    elif name == "jackal":
        parts = [dict(im=M("leg_redB"), x=C - 40, y=250, s=0.7), dict(im=M("leg_redB"), x=C + 40, y=250, s=0.7, flip=True),
                 dict(im=tint_image(M("body_redD"), 18, 0.85, 0.95), x=C, y=190, s=0.8),
                 dict(im=M("arm_redC"), x=C - 85, y=185 + up, s=0.65), dict(im=M("arm_redC"), x=C + 85, y=185 + up, s=0.65, flip=True),
                 dict(im=tint_image(A("dog"), 25, 0.7, 1.0), x=C, y=105, s=0.85),
                 dict(im=M("detail_red_horn_large"), x=C - 45, y=55, s=1.0), dict(im=M("detail_red_horn_large"), x=C + 45, y=55, s=1.0, flip=True)]
    elif name == "owl":
        parts = [dict(im=M("leg_whiteE"), x=C - 35, y=255, s=0.6), dict(im=M("leg_whiteE"), x=C + 35, y=255, s=0.6, flip=True),
                 dict(im=tint_image(M("body_whiteB"), 200, 0.08, 1.0), x=C, y=190, s=0.75),
                 dict(im=M("arm_whiteE"), x=C - 100, y=180 + up, s=0.85), dict(im=M("arm_whiteE"), x=C + 100, y=180 + up, s=0.85, flip=True),
                 dict(im=A("owl"), x=C, y=105, s=0.9)]
    elif name == "leviathan":
        parts = [dict(im=tint_image(M("body_blueF"), 200, 0.6, 0.7), x=C, y=205, s=0.85),
                 dict(im=M("arm_blueB"), x=C - 100, y=200 + up, s=0.8), dict(im=M("arm_blueB"), x=C + 100, y=200 + up, s=0.8, flip=True),
                 dict(im=A("whale"), x=C, y=105, s=1.0)]
    else:  # smoggar
        parts = [dict(im=M("leg_darkA"), x=C - 45, y=255, s=0.7), dict(im=M("leg_darkA"), x=C + 45, y=255, s=0.7, flip=True),
                 dict(im=tint_image(M("body_darkD"), 275, 0.45, 0.55), x=C, y=175, s=0.95),
                 dict(im=M("arm_darkA"), x=C - 100, y=175 + up, s=0.8), dict(im=M("arm_darkA"), x=C + 100, y=175 + up, s=0.8, flip=True),
                 dict(im=M("eye_psycho_dark"), x=C - 32, y=135, s=0.9), dict(im=M("eye_psycho_dark"), x=C + 32, y=135, s=0.9, flip=True),
                 dict(im=M("mouth_closed_fangs" if frame == 0 else "mouthD"), x=C, y=200, s=1.0),
                 dict(im=M("detail_dark_horn_large"), x=C - 55, y=75, s=1.2), dict(im=M("detail_dark_horn_large"), x=C + 55, y=75, s=1.2, flip=True)]
    return compose(parts)


def draw_tools():
    """A 32 x 256 indexed sheet of projectiles and icons on one palette."""
    colors = np.array([[0, 0, 0], [255, 240, 232], [232, 72, 120], [160, 30, 80],
                       [120, 200, 90], [60, 130, 60], [255, 150, 40], [255, 224, 120],
                       [150, 140, 130], [80, 70, 70], [150, 220, 255], [70, 140, 220],
                       [40, 30, 50], [255, 90, 60], [190, 120, 220], [240, 190, 80]],
                      dtype=np.uint8)
    tool = np.zeros((32, TOOL_COLUMNS * 16), dtype=np.uint8)

    def cell(column, row=0):
        return tool[row * 16:(row + 1) * 16, column * 16:(column + 1) * 16]

    # Rose thorns: a spinning petal-and-thorn in two frames.
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
    # Trash (goblin throw), spit (worm), bolt (robot), fire, ice, oil.
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
    # Spark, heart, rose (an art), petal, bonus lane marker, cursor, drip, leaf.
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
    header = ["/* Generated by tools/prepare_assets.py. */", "#ifndef MAIYA_ASSETS_H",
              "#define MAIYA_ASSETS_H", "#include <stdint.h>"]
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
        """Frames that share one palette: fit it over all of them first."""
        training = np.concatenate([f[:, :, :3][training_mask(f)] for f in frames.values()])
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

    # --- Environments: far layer, road, restored and corrupted palettes --
    atlas = Image.open(GAME / "assets/environments.png").convert("RGBA")
    for i in range(6):
        x = (i % 2) * (atlas.width // 2)
        y0 = (i // 2) * atlas.height // 3
        y1 = ((i // 2) + 1) * atlas.height // 3
        panel = atlas.crop((x, y0 + 2, x + atlas.width // 2, y1 - 2))
        panel = np.asarray(panel.resize((512, 224), Image.Resampling.LANCZOS)).astype(np.float32)
        blend = 96
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
        print(f"Environment {i + 1}/6", flush=True)

    # --- Maiya --------------------------------------------------------
    frames = {name: fit_frame(source(index), HERO_FRAME_SCALE.get(name, HERO_SCALE), HERO_CANVAS,
                              anchor="all" if name == "spin" else "feet")
              for name, index in HERO_FRAMES}
    training = np.concatenate([f[:, :, :3][training_mask(f)] for f in frames.values()])
    master = fit_palette(training)
    hero_tiles = []
    for name, _ in HERO_FRAMES:
        base, _ = append(f"hero_{name}", frames[name], master=master, emit_palette=False)
        hero_tiles.append(base)
    header.append(c_array("mg_hero_tiles", hero_tiles))
    header.append(f"#define MG_HERO_FRAMES {len(HERO_FRAMES)}u")
    for k, (name, _) in enumerate(HERO_FRAMES):
        header.append(f"#define MG_F_{name.upper()} {k}u")
    palette = np.zeros((16, 3), dtype=np.uint8)
    palette[1:] = costume(master, 130, 0.62, 335)
    header.append(c_array("mg_hero_pal", palette_words(palette)))
    palette[1:] = costume(master, 42, 0.85, 40, glow=True)
    header.append(c_array("mg_hero_sun_pal", palette_words(palette)))
    print("Maiya", flush=True)

    eagle = {name: fit_frame(source(index), HERO_SCALE, EAGLE_CANVAS, anchor="all")
             for name, index in EAGLE_FRAMES}
    shared_set("eagle", eagle)

    portrait = fit_frame(source(66), 0.5, (112, 96), anchor="all")
    _, portrait_palettes = append("portrait", portrait, banks=2, bank_base=13)
    face = fit_frame(source(66), 0.165, (32, 32), anchor="all")
    append("face", face, master=portrait_palettes[0][1:], bank_base=13)

    # --- Creatures, machines, spirits and villagers --------------------
    for name, spec in CREATURES.items():
        frames = {}
        for k, frame in enumerate(spec["frames"]):
            im = kenney(spec["dir"], frame)
            if spec["tint"]:
                im = tint_image(im, *spec["tint"])
            frames[str(k)] = fit_frame(im, spec["scale"], spec["canvas"], anchor="all")
        shared_set(name, frames)
        header.append(f"#define MG_{name.upper()}_FRAMES {len(frames)}u")
        header.append(f"#define MG_{name.upper()}_W {spec['canvas'][0]}u")
        header.append(f"#define MG_{name.upper()}_H {spec['canvas'][1]}u")
    print("Creatures", flush=True)

    # --- The seven guardians --------------------------------------------
    for name in BOSSES:
        frames = {str(f): fit_frame(boss_art(name, f), 0.4, (128, 128), anchor="all") for f in range(2)}
        shared_set(f"boss_{name}", frames)
    print("Guardians", flush=True)

    # --- Ground blocks, one set per world -------------------------------
    for ground in GROUNDS:
        frames = {}
        for k, part in enumerate(("Left", "Mid", "Right")):
            im = kenney("ground", f"{ground}{part}")
            frames[str(k)] = fit_frame(im, 0.25, (32, 32), anchor="fill")
        shared_set(f"block_{ground}", frames)
    industrial = {}
    for k, code in enumerate(("020", "021", "022")):
        try:
            industrial[str(k)] = fit_frame(kenney("industrial", f"platformIndustrial_{code}"), 32 / 70, (32, 32), anchor="fill")
        except FileNotFoundError:
            industrial[str(k)] = fit_frame(kenney("ground", ["stoneLeft", "stoneMid", "stoneRight"][k]), 0.25, (32, 32), anchor="fill")
    shared_set("block_metal", industrial)

    # --- Road furniture and pick-ups ------------------------------------
    furniture = {}
    for name, sub, src, scale, canvas in (("chest", "tiles", "boxItem", 0.25, (32, 32)),
                                          ("chest_open", "tiles", "boxCrate", 0.25, (32, 32)),
                                          ("door_top", "tiles", "doorClosed_top", 0.25, (32, 32)),
                                          ("door_mid", "tiles", "doorClosed_mid", 0.25, (32, 32)),
                                          ("sign", "tiles", "sign", 0.25, (32, 32)),
                                          ("torch0", "tiles", "torch1", 0.25, (32, 32)),
                                          ("torch1", "tiles", "torch2", 0.25, (32, 32)),
                                          ("spikes", "tiles", "spikes", 0.25, (32, 32)),
                                          ("lava", "tiles", "lavaTop_low", 0.25, (32, 32)),
                                          ("water", "tiles", "waterTop_low", 0.25, (32, 32)),
                                          ("bush", "tiles", "bush", 0.25, (32, 32)),
                                          ("mushroom", "tiles", "mushroomRed", 0.25, (32, 32)),
                                          ("plant", "tiles", "plantPurple", 0.25, (32, 32))):
        furniture[name] = fit_frame(kenney(sub, src), scale, canvas, anchor="fill")
    shared_set("prop", furniture)
    for k, name in enumerate(furniture):
        header.append(f"#define MG_P_{name.upper()} {k}u")
    items = {}
    for name, src in (("gem", "gemGreen"), ("gem_pink", "gemRed"), ("key", "keyYellow"),
                      ("seed", "coinGold"), ("star", "star")):
        items[name] = fit_frame(kenney("items", src), 0.2, (32, 32), anchor="fill")
    shared_set("item", items)
    for k, name in enumerate(items):
        header.append(f"#define MG_I_{name.upper()} {k}u")
    print("Props and items", flush=True)

    # --- Tools -------------------------------------------------------
    tool, colors = draw_tools()
    base = c1.tell() // 64
    lo, hi = encode_image(tool, tool.size // 256)
    c1.write(lo)
    c2.write(hi)
    header.append(f"#define MG_TOOL_TILE {base}u")
    header.append(f"#define MG_TOOL_COLUMNS {TOOL_COLUMNS}u")
    for name, column in (("THORN0", TOOL_THORN0), ("THORN1", TOOL_THORN1), ("TRASH", TOOL_TRASH),
                         ("SPIT", TOOL_SPIT), ("BOLT", TOOL_BOLT), ("FIRE", TOOL_FIRE), ("ICE", TOOL_ICE),
                         ("OIL", TOOL_OIL), ("SPARK", TOOL_SPARK), ("HEART", TOOL_HEART), ("ROSE", TOOL_ROSE),
                         ("PETAL", TOOL_PETAL), ("LANE", TOOL_LANE), ("CURSOR", TOOL_CURSOR),
                         ("DRIP", TOOL_DRIP), ("LEAF", TOOL_LEAF)):
        header.append(f"#define MG_T_{name} {column}u")
    header.append(c_array("mg_tool_pal", palette_words(colors)))
    preview = np.dstack((colors[tool], np.where(tool > 0, 255, 0))).astype(np.uint8)
    Image.fromarray(preview).save(output / "tools.png")
    Image.fromarray(preview).save(preview_dir / "tools.png")
    manifest.append(dict(name="tools", tile_base=base, tile_last=base + tool.size // 256 - 1,
                         strips=TOOL_COLUMNS, rows=2, stride=TOOL_COLUMNS, palette_count=1))

    write_utility_tiles(c1, c2)
    for data, suffix in ((c1.getvalue(), "c1"), (c2.getvalue(), "c2")):
        raw = np.frombuffer(data, dtype=np.uint8).reshape(-1, 2)[:, ::-1].tobytes()
        (output / f"780-{suffix}.{suffix}").write_bytes(raw)
        assert raw[0xFFFF * 64:] == bytes(64)
    header.append("#endif\n")
    (output / "maiya_assets.h").write_text("\n".join(header), encoding="ascii")
    (output / "assets.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="ascii")
    print(f"Built {len(manifest)} assets; outputs: {output}")


if __name__ == "__main__":
    build()
