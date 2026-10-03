"""Direct C-ROM builder for Maiya: Super Nature Girl.

Builds high-definition, authentic Neo Geo arcade pixel art directly into
780-c1.c1 and 780-c2.c2 planar C-ROM byte streams:
- Heroine Maiya full 39-frame moveset (preserved from clean source)
- 6 Kawaii Kirby-style Blight Enemies (Slime, Beetle, Crow, Goblin, Worm, Drone)
- 7 Kawaii Kirby/Arcade-style Biomechanical Guardians / Bosses (128x128)
- Cute Allies & NPCs (Sunboy, Nature Spirit, Village Elder, Valley Girl)
- 7 Rich textured 32x32 platform ledge block sets with depth and bevels
- Authentic Neo Geo arcade props, hazards, collectibles, and projectiles
- 6 HD Stage environments and clean arcade title key visual
"""

from __future__ import annotations

import io
import json
import math
from pathlib import Path
import sys

import numpy as np
from PIL import Image

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "artbox"))

from palette_banks import palette_words  # noqa: E402
from tile_codec import encode_image, decode_image, write_utility_tiles  # noqa: E402

OUTPUT = GAME / "artbox/generated"
PREVIEW_DIR = GAME / "artbox/in/generated"
ROMS_DIR = ROOT / "roms/maiya"

HERO_CANVAS = (112, 96)
TOOL_COLUMNS = 16
TOOL_THORN0, TOOL_THORN1, TOOL_TRASH, TOOL_SPIT, TOOL_BOLT, TOOL_FIRE, TOOL_ICE, TOOL_OIL = range(8)
TOOL_SPARK, TOOL_HEART, TOOL_ROSE, TOOL_PETAL, TOOL_LANE, TOOL_CURSOR, TOOL_DRIP, TOOL_LEAF = range(8, 16)


def c_array(name: str, values: list[int], ctype: str = "uint16_t") -> str:
    lines = [f"static const {ctype} {name}[{len(values)}] = {{"]
    for i in range(0, len(values), 16):
        lines.append("    " + ", ".join(str(int(v)) for v in values[i:i + 16]) + ",")
    return "\n".join(lines + ["};", ""])


# ------------------------------------------------------------------
#  Pixel-Art Drawing Primitives (Indexed 0..15)
# ------------------------------------------------------------------
def fill_circle(arr: np.ndarray, cx: float, cy: float, r: float, color: int):
    h, w = arr.shape
    y0, y1 = max(0, int(cy - r - 1)), min(h, int(cy + r + 2))
    x0, x1 = max(0, int(cx - r - 1)), min(w, int(cx + r + 2))
    for y in range(y0, y1):
        for x in range(x0, x1):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                arr[y, x] = color


def fill_ellipse(arr: np.ndarray, cx: float, cy: float, rx: float, ry: float, color: int):
    h, w = arr.shape
    if rx <= 0 or ry <= 0:
        return
    y0, y1 = max(0, int(cy - ry - 1)), min(h, int(cy + ry + 2))
    x0, x1 = max(0, int(cx - rx - 1)), min(w, int(cx + rx + 2))
    for y in range(y0, y1):
        for x in range(x0, x1):
            if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0:
                arr[y, x] = color


def draw_outline(arr: np.ndarray, outline_color: int, bg_color: int = 0):
    h, w = arr.shape
    mask = (arr != bg_color)
    res = arr.copy()
    for y in range(h):
        for x in range(w):
            if not mask[y, x]:
                has_neighbor = False
                for dy in (-1, 0, 1):
                    for dx in (-1, 0, 1):
                        ny, nx = y + dy, x + dx
                        if 0 <= ny < h and 0 <= nx < w and mask[ny, nx]:
                            has_neighbor = True
                            break
                    if has_neighbor:
                        break
                if has_neighbor:
                    res[y, x] = outline_color
    return res


def draw_kawaii_eye(arr: np.ndarray, cx: int, cy: int, size: int = 3,
                    pupil_c: int = 8, glint_c: int = 7, blush_c: int = 6, happy: bool = False):
    h, w = arr.shape
    if happy:
        # Happy squinting curve ^
        for dx in range(-size, size + 1):
            dy = -int(math.sin((dx + size) / (2 * size) * math.pi) * 2)
            if 0 <= cy + dy < h and 0 <= cx + dx < w:
                arr[cy + dy, cx + dx] = pupil_c
    else:
        # Oval anime eye with shine
        ry = size + 1
        rx = max(1, size - 1)
        fill_ellipse(arr, cx, cy, rx, ry, pupil_c)
        # Big specular reflection
        if 0 <= cy - 1 < h and 0 <= cx - 1 < w:
            arr[cy - 1, cx - 1] = glint_c
        if rx > 1 and 0 <= cy + 1 < h and 0 <= cx + 1 < w:
            arr[cy + 1, cx + 1] = glint_c
    # Blush cheeks
    if blush_c:
        by = cy + size + 2
        for bx in range(cx - size, cx + size + 1):
            if 0 <= by < h and 0 <= bx < w and arr[by, bx] != pupil_c:
                arr[by, bx] = blush_c


# ------------------------------------------------------------------
#  Direct Kawaii Blight Enemies
# ------------------------------------------------------------------
def build_slime_frames() -> tuple[dict[str, np.ndarray], np.ndarray]:
    """48x32 Kawaii gelatinous bouncing slime (4 frames)."""
    pal = np.array([
        [0, 0, 0], [42, 18, 56], [255, 205, 235], [242, 90, 160],
        [195, 45, 120], [135, 22, 85], [255, 140, 190], [255, 255, 255],
        [25, 12, 35], [255, 235, 95], [115, 235, 115], [55, 165, 55],
        [90, 210, 250], [35, 130, 210], [255, 170, 60], [210, 90, 25]
    ], dtype=np.uint8)

    frames = {}
    for f in range(4):
        canvas = np.zeros((32, 48), dtype=np.uint8)
        cx = 24
        if f == 0:  # idle standing
            cy, rx, ry = 21, 14, 9
            fill_ellipse(canvas, cx, cy, rx, ry, 4)
            fill_ellipse(canvas, cx, cy - 2, rx - 2, ry - 2, 3)
            fill_ellipse(canvas, cx - 4, cy - 5, 5, 3, 2)  # highlight
            draw_kawaii_eye(canvas, cx - 6, cy - 1, size=2, pupil_c=8, glint_c=7, blush_c=6)
            draw_kawaii_eye(canvas, cx + 6, cy - 1, size=2, pupil_c=8, glint_c=7, blush_c=6)
            canvas[cy + 3, cx - 1:cx + 2] = 8  # cute smile
        elif f == 1:  # squashed preparation
            cy, rx, ry = 25, 18, 6
            fill_ellipse(canvas, cx, cy, rx, ry, 4)
            fill_ellipse(canvas, cx, cy - 1, rx - 2, ry - 1, 3)
            fill_ellipse(canvas, cx - 6, cy - 3, 6, 2, 2)
            draw_kawaii_eye(canvas, cx - 7, cy, size=2, pupil_c=8, glint_c=7, blush_c=6, happy=True)
            draw_kawaii_eye(canvas, cx + 7, cy, size=2, pupil_c=8, glint_c=7, blush_c=6, happy=True)
        elif f == 2:  # jumping stretched
            cy, rx, ry = 15, 10, 13
            fill_ellipse(canvas, cx, cy, rx, ry, 4)
            fill_ellipse(canvas, cx, cy - 2, rx - 2, ry - 2, 3)
            fill_ellipse(canvas, cx - 3, cy - 7, 4, 3, 2)
            draw_kawaii_eye(canvas, cx - 4, cy - 3, size=2, pupil_c=8, glint_c=7, blush_c=6)
            draw_kawaii_eye(canvas, cx + 4, cy - 3, size=2, pupil_c=8, glint_c=7, blush_c=6)
            canvas[cy + 2, cx] = 8  # open mouth :D
            canvas[cy + 3, cx - 1:cx + 2] = 5
            # droplets
            canvas[27:30, cx - 4] = 3
            canvas[28:31, cx + 4] = 3
        else:  # landing wobble
            cy, rx, ry = 23, 16, 7
            fill_ellipse(canvas, cx, cy, rx, ry, 4)
            fill_ellipse(canvas, cx, cy - 1, rx - 2, ry - 1, 3)
            fill_ellipse(canvas, cx - 5, cy - 4, 5, 2, 2)
            draw_kawaii_eye(canvas, cx - 6, cy, size=2, pupil_c=8, glint_c=7, blush_c=6)
            draw_kawaii_eye(canvas, cx + 6, cy, size=2, pupil_c=8, glint_c=7, blush_c=6)
            canvas[cy + 2, cx - 2:cx + 3] = 8  # wavy mouth

        canvas = draw_outline(canvas, 1)
        frames[str(f)] = canvas
    return frames, pal


def build_beetle_frames() -> tuple[dict[str, np.ndarray], np.ndarray]:
    """64x48 Kawaii Kirby-style round stag beetle (3 frames)."""
    pal = np.array([
        [0, 0, 0], [25, 45, 20], [185, 255, 140], [115, 220, 75],
        [60, 165, 40], [30, 105, 25], [255, 235, 100], [255, 255, 255],
        [20, 25, 20], [255, 145, 185], [255, 185, 60], [200, 120, 30],
        [140, 75, 20], [70, 40, 15], [160, 230, 255], [80, 170, 225]
    ], dtype=np.uint8)

    frames = {}
    for f in range(3):
        canvas = np.zeros((48, 64), dtype=np.uint8)
        cx, cy = 32, 28
        # Round body
        fill_ellipse(canvas, cx, cy, 16, 12, 4)
        fill_ellipse(canvas, cx, cy - 2, 14, 10, 3)
        fill_ellipse(canvas, cx - 5, cy - 6, 7, 4, 2)  # highlight

        # Cute golden star on shell
        canvas[cy - 2, cx + 4] = 6
        canvas[cy - 3:cy - 1, cx + 3:cx + 6] = 6

        # Beetle head & horn
        hx, hy = cx - 12, cy - 2
        fill_circle(canvas, hx, hy, 7, 12)
        fill_circle(canvas, hx - 1, hy - 1, 5, 11)
        # Horn pointing up-left
        for k in range(10):
            canvas[hy - 4 - k, hx - 4 - k // 2] = 11
            canvas[hy - 4 - k, hx - 3 - k // 2] = 10
        canvas[hy - 14, hx - 9:hx - 6] = 6  # shiny horn tip

        # Big cute anime eye
        draw_kawaii_eye(canvas, hx - 2, hy - 1, size=2, pupil_c=8, glint_c=7, blush_c=9)

        # Stubby legs with stepping animation
        legs = [(cx - 8, cy + 10), (cx, cy + 10), (cx + 8, cy + 10)]
        for idx, (lx, ly) in enumerate(legs):
            offset = 2 if (f == 1 and idx == 0) or (f == 2 and idx == 2) else 0
            fill_circle(canvas, lx + (f if idx == 1 else -f), ly - offset, 3, 13)

        canvas = draw_outline(canvas, 1)
        frames[str(f)] = canvas
    return frames, pal


def build_crow_frames() -> tuple[dict[str, np.ndarray], np.ndarray]:
    """64x48 Chubby kawaii round owl / crow (2 frames)."""
    pal = np.array([
        [0, 0, 0], [25, 20, 45], [195, 205, 255], [125, 135, 215],
        [75, 80, 160], [45, 45, 110], [255, 215, 80], [255, 255, 255],
        [20, 15, 30], [255, 135, 165], [255, 165, 40], [210, 105, 25],
        [235, 220, 255], [180, 165, 225], [130, 115, 180], [80, 70, 135]
    ], dtype=np.uint8)

    frames = {}
    for f in range(2):
        canvas = np.zeros((48, 64), dtype=np.uint8)
        cx, cy = 32, 24
        # Round fluffy ball body
        fill_circle(canvas, cx, cy, 14, 4)
        fill_circle(canvas, cx, cy - 2, 12, 3)
        # Lavender tummy
        fill_ellipse(canvas, cx, cy + 3, 8, 7, 12)
        fill_ellipse(canvas, cx, cy + 2, 6, 5, 7)

        # Fluffy wings
        wing_y = cy - 2 if f == 0 else cy + 4
        # left wing
        fill_ellipse(canvas, cx - 16, wing_y, 8, 5, 3)
        # right wing
        fill_ellipse(canvas, cx + 16, wing_y, 8, 5, 3)

        # Huge round kawaii owl eyes
        draw_kawaii_eye(canvas, cx - 6, cy - 4, size=3, pupil_c=8, glint_c=7, blush_c=9)
        draw_kawaii_eye(canvas, cx + 6, cy - 4, size=3, pupil_c=8, glint_c=7, blush_c=9)

        # Cute golden beak
        canvas[cy - 1:cy + 2, cx - 1:cx + 2] = 6
        canvas[cy + 2, cx] = 10

        # Cute little orange talons
        fill_circle(canvas, cx - 4, cy + 13, 2, 10)
        fill_circle(canvas, cx + 4, cy + 13, 2, 10)

        canvas = draw_outline(canvas, 1)
        frames[str(f)] = canvas
    return frames, pal


def build_goblin_frames() -> tuple[dict[str, np.ndarray], np.ndarray]:
    """48x64 Cute chibi forest imp / korok with leaf hat and hammer (4 frames)."""
    pal = np.array([
        [0, 0, 0], [35, 25, 15], [255, 225, 180], [250, 190, 130],
        [200, 135, 80], [140, 85, 45], [145, 225, 80], [85, 175, 40],
        [45, 115, 25], [255, 130, 150], [255, 255, 255], [20, 15, 20],
        [255, 215, 70], [210, 150, 40], [145, 95, 30], [90, 50, 20]
    ], dtype=np.uint8)

    frames = {}
    for f in range(4):
        canvas = np.zeros((64, 48), dtype=np.uint8)
        cx, cy = 24, 38

        # Cute round chibi body
        fill_circle(canvas, cx, cy + 6, 8, 4)
        fill_circle(canvas, cx, cy + 5, 7, 3)

        # Chibi face
        fill_circle(canvas, cx, cy - 4, 10, 2)
        # Giant pointed leaf/acorn hat
        hat_tip_x = cx + (4 if f % 2 else -2)
        hat_pts = [(cx - 11, cy - 8), (cx + 11, cy - 8), (hat_tip_x, cy - 28)]
        for y in range(cy - 28, cy - 8):
            t = (y - (cy - 28)) / 20.0
            x_left = int(hat_tip_x * (1 - t) + (cx - 11) * t)
            x_right = int(hat_tip_x * (1 - t) + (cx + 11) * t)
            canvas[y, x_left:x_right + 1] = 6 if y > cy - 18 else 7
        canvas[cy - 29:cy - 27, hat_tip_x] = 12  # little gold acorn sprout

        # Cute eyes and blush
        draw_kawaii_eye(canvas, cx - 5, cy - 4, size=2, pupil_c=11, glint_c=10, blush_c=9)
        draw_kawaii_eye(canvas, cx + 5, cy - 4, size=2, pupil_c=11, glint_c=10, blush_c=9)
        canvas[cy, cx] = 11  # cute smile

        # Toy wooden mallet
        hx = cx + 13
        hy = cy + (2 if f == 3 else -4)
        fill_rect = canvas[hy - 6:hy + 6, hx - 4:hx + 4]
        fill_rect[:] = 13
        canvas[hy - 4:hy + 4, hx - 2:hx + 2] = 12
        # handle
        canvas[hy + 4:hy + 14, cx + 8:cx + 10] = 14

        # Little feet
        leg_offset = 2 if f % 2 else -2
        fill_circle(canvas, cx - 4, cy + 16 + leg_offset, 3, 5)
        fill_circle(canvas, cx + 4, cy + 16 - leg_offset, 3, 5)

        canvas = draw_outline(canvas, 1)
        frames[str(f)] = canvas
    return frames, pal


def build_worm_frames() -> tuple[dict[str, np.ndarray], np.ndarray]:
    """32x80 Cute cartoon apple-worm popping out of a flowerpot/pipe (2 frames)."""
    pal = np.array([
        [0, 0, 0], [25, 40, 20], [230, 255, 140], [165, 235, 75],
        [100, 185, 45], [55, 125, 30], [255, 145, 185], [255, 255, 255],
        [20, 25, 15], [255, 220, 80], [215, 100, 70], [165, 65, 40],
        [115, 40, 25], [90, 140, 120], [50, 95, 80], [30, 60, 50]
    ], dtype=np.uint8)

    frames = {}
    for f in range(2):
        canvas = np.zeros((80, 32), dtype=np.uint8)
        cx = 16

        # Pipe / flowerpot at bottom
        canvas[52:80, 4:28] = 14
        canvas[50:56, 2:30] = 13
        canvas[51:55, 4:28] = 14
        # moss on pipe
        canvas[56:60, 6:10] = 3

        # Worm body segments popping up
        h_offset = 8 if f == 1 else 0
        seg_y = [46 - h_offset, 36 - h_offset, 24 - h_offset]
        for idx, sy in enumerate(seg_y):
            r = 7 if idx == 2 else 6
            fill_circle(canvas, cx, sy, r, 3)
            fill_circle(canvas, cx - 2, sy - 2, r - 2, 2)

        # Head is at seg_y[2]
        head_y = seg_y[2]
        # Cute antennae
        ant_x = 4 if f == 0 else -4
        canvas[head_y - 9:head_y - 6, cx - 3] = 4
        canvas[head_y - 9:head_y - 6, cx + 3] = 4
        fill_circle(canvas, cx - 4 + ant_x // 2, head_y - 10, 2, 9)
        fill_circle(canvas, cx + 4 + ant_x // 2, head_y - 10, 2, 9)

        # Cute face
        draw_kawaii_eye(canvas, cx - 3, head_y - 1, size=2, pupil_c=8, glint_c=7, blush_c=6)
        draw_kawaii_eye(canvas, cx + 3, head_y - 1, size=2, pupil_c=8, glint_c=7, blush_c=6)
        canvas[head_y + 3, cx] = 8  # smile

        canvas = draw_outline(canvas, 1)
        frames[str(f)] = canvas
    return frames, pal


def build_robot_frames() -> tuple[dict[str, np.ndarray], np.ndarray]:
    """48x48 Cute spherical flying anime drone with glowing eye (3 frames)."""
    pal = np.array([
        [0, 0, 0], [25, 30, 45], [255, 255, 255], [220, 235, 250],
        [155, 185, 215], [95, 125, 160], [255, 225, 80], [255, 120, 180],
        [30, 200, 255], [10, 130, 210], [5, 60, 130], [255, 160, 60],
        [210, 80, 30], [140, 150, 170], [80, 90, 110], [45, 50, 70]
    ], dtype=np.uint8)

    frames = {}
    for f in range(3):
        canvas = np.zeros((48, 48), dtype=np.uint8)
        cx, cy = 24, 24 + (1 if f == 1 else -1)

        # Spherical metallic orb body
        fill_circle(canvas, cx, cy, 14, 4)
        fill_circle(canvas, cx, cy - 2, 12, 3)
        fill_circle(canvas, cx - 4, cy - 6, 6, 2)  # glossy shine

        # Visor glass screen in center
        fill_ellipse(canvas, cx, cy + 1, 9, 6, 10)
        fill_ellipse(canvas, cx, cy + 1, 8, 5, 9)

        # Digital glowing anime eyes on visor
        if f == 0:
            # Cute round glowing eye
            fill_circle(canvas, cx, cy + 1, 3, 8)
            canvas[cy, cx] = 2  # shine
        elif f == 1:
            # Digital heart eye <3
            canvas[cy:cy + 3, cx - 2:cx + 3] = 7
            canvas[cy - 1, cx - 1] = 7
            canvas[cy - 1, cx + 1] = 7
        else:
            # Happy smiling visor arc ^_^
            canvas[cy, cx - 4:cx + 5] = 8
            canvas[cy - 1, cx - 3] = 8
            canvas[cy - 1, cx + 3] = 8

        # Cute yellow side headphones / thrusters
        fill_ellipse(canvas, cx - 14, cy, 3, 5, 6)
        fill_ellipse(canvas, cx + 14, cy, 3, 5, 6)

        # Spinning mini antenna / propeller on top
        prop_x = 4 if f == 0 else (-4 if f == 1 else 0)
        canvas[cy - 16:cy - 13, cx] = 14
        canvas[cy - 17, cx - 6 + prop_x:cx + 7 + prop_x] = 6

        # Hover thruster puff underneath
        fill_circle(canvas, cx, cy + 16, 2 + f % 2, 8)

        canvas = draw_outline(canvas, 1)
        frames[str(f)] = canvas
    return frames, pal


# ------------------------------------------------------------------
#  Direct Kawaii Guardians / Bosses (128x128)
# ------------------------------------------------------------------
def build_boss_beetle_frames() -> tuple[dict[str, np.ndarray], np.ndarray]:
    """128x128 Chainsaw Beetle: Cute giant mech stag beetle with cartoon buzzsaws."""
    pal = np.array([
        [0, 0, 0], [30, 35, 30], [255, 235, 110], [240, 180, 50],
        [185, 125, 30], [120, 75, 15], [195, 255, 150], [125, 225, 85],
        [65, 165, 45], [30, 105, 25], [255, 255, 255], [20, 20, 25],
        [255, 100, 120], [180, 195, 215], [115, 130, 155], [65, 75, 95]
    ], dtype=np.uint8)

    frames = {}
    for f in range(2):
        canvas = np.zeros((128, 128), dtype=np.uint8)
        cx, cy = 64, 76

        # Giant round armored chassis
        fill_circle(canvas, cx, cy, 38, 9)
        fill_circle(canvas, cx, cy - 4, 35, 8)
        fill_circle(canvas, cx, cy - 8, 30, 7)
        fill_circle(canvas, cx - 12, cy - 18, 14, 6)  # glossy dome shine

        # Golden gear insignia on back
        fill_circle(canvas, cx, cy, 10, 3)
        fill_circle(canvas, cx, cy, 6, 2)

        # Head dome
        hx, hy = cx, cy - 28
        fill_circle(canvas, hx, hy, 22, 14)
        fill_circle(canvas, hx, hy - 3, 19, 13)

        # Cute giant robot visor eyes
        eye_y = hy + 2
        fill_ellipse(canvas, hx - 10, eye_y, 6, 7, 11)
        fill_ellipse(canvas, hx + 10, eye_y, 6, 7, 11)
        # Glowing cyan / yellow pupil
        fill_circle(canvas, hx - 10, eye_y, 4, 2)
        fill_circle(canvas, hx + 10, eye_y, 4, 2)
        canvas[eye_y - 1, hx - 11] = 10
        canvas[eye_y - 1, hx + 9] = 10

        # Cartoon giant whirring buzzsaw arms!
        saw_ang = f * 45
        for sx in (cx - 44, cx + 44):
            sy = cy - 14
            fill_circle(canvas, sx, sy, 18, 13)
            fill_circle(canvas, sx, sy, 15, 10)
            fill_circle(canvas, sx, sy, 8, 3)
            # Hazard stripes
            for a in range(0, 360, 45):
                rad = math.radians(a + saw_ang)
                px = int(sx + math.cos(rad) * 14)
                py = int(sy + math.sin(rad) * 14)
                if 0 <= py < 128 and 0 <= px < 128:
                    canvas[py, px] = 11

        # Chubby mech legs
        for lx in (cx - 26, cx - 10, cx + 10, cx + 26):
            ly = cy + 34
            fill_circle(canvas, lx, ly + (2 if f == 1 else 0), 7, 15)
            fill_circle(canvas, lx, ly + 4 + (2 if f == 1 else 0), 5, 14)

        canvas = draw_outline(canvas, 1)
        frames[str(f)] = canvas
    return frames, pal


def build_boss_toad_frames() -> tuple[dict[str, np.ndarray], np.ndarray]:
    """128x128 Sludge Toad: Giant squishy kawaii frog king with golden crown."""
    pal = np.array([
        [0, 0, 0], [25, 45, 25], [255, 240, 110], [245, 195, 45],
        [185, 135, 25], [195, 255, 140], [125, 230, 75], [65, 175, 40],
        [35, 115, 25], [255, 255, 255], [25, 25, 25], [255, 130, 175],
        [240, 245, 200], [190, 205, 150], [135, 155, 100], [80, 100, 60]
    ], dtype=np.uint8)

    frames = {}
    for f in range(2):
        canvas = np.zeros((128, 128), dtype=np.uint8)
        cx, cy = 64, 76

        # Giant round squishy frog body
        ry = 36 if f == 0 else 32
        fill_ellipse(canvas, cx, cy, 42, ry, 7)
        fill_ellipse(canvas, cx, cy - 4, 39, ry - 3, 6)
        # Pastel yellow/white belly
        fill_ellipse(canvas, cx, cy + 8, 26, ry - 12, 12)
        fill_ellipse(canvas, cx, cy + 6, 22, ry - 14, 9)

        # Huge round cute eyes on top
        for ex in (cx - 18, cx + 18):
            ey = cy - ry + 4
            fill_circle(canvas, ex, ey, 14, 6)
            draw_kawaii_eye(canvas, ex, ey, size=5, pupil_c=10, glint_c=9, blush_c=11)

        # Tiny gold crown between eyes
        crowny = cy - ry - 6
        canvas[crowny:crowny + 6, cx - 8:cx + 9] = 3
        canvas[crowny - 4:crowny, cx - 8] = 2
        canvas[crowny - 6:crowny, cx] = 2
        canvas[crowny - 4:crowny, cx + 8] = 2

        # Giant cheerful smiling mouth
        for mx in range(cx - 24, cx + 25):
            t = (mx - cx) / 24.0
            my = int(cy + 4 + (t * t) * 8)
            canvas[my:my + 2, mx] = 10

        # Cute webbed hands & feet
        for hx, hy in ((cx - 36, cy + 18), (cx + 36, cy + 18)):
            fill_circle(canvas, hx, hy, 8, 6)
            fill_circle(canvas, hx - 3, hy + 5, 3, 5)
            fill_circle(canvas, hx + 3, hy + 5, 3, 5)

        canvas = draw_outline(canvas, 1)
        frames[str(f)] = canvas
    return frames, pal


def build_boss_vulture_frames() -> tuple[dict[str, np.ndarray], np.ndarray]:
    """128x128 Iron Vulture: Steampunk giant owl/vulture with brass goggles."""
    pal = np.array([
        [0, 0, 0], [35, 30, 25], [255, 235, 130], [240, 180, 60],
        [180, 120, 30], [115, 70, 15], [220, 225, 235], [160, 170, 185],
        [105, 115, 130], [60, 70, 85], [255, 255, 255], [25, 20, 20],
        [255, 140, 180], [215, 100, 40], [150, 60, 20], [90, 35, 10]
    ], dtype=np.uint8)

    frames = {}
    for f in range(2):
        canvas = np.zeros((128, 128), dtype=np.uint8)
        cx, cy = 64, 72

        # Plump round steampunk body
        fill_circle(canvas, cx, cy, 32, 8)
        fill_circle(canvas, cx, cy - 3, 29, 7)
        fill_circle(canvas, cx, cy + 4, 18, 6)  # tummy

        # Huge round brass aviator goggles on forehead
        gy = cy - 18
        for gx in (cx - 14, cx + 14):
            fill_circle(canvas, gx, gy, 12, 4)
            fill_circle(canvas, gx, gy, 10, 3)
            fill_circle(canvas, gx, gy, 8, 10)  # glass shine
            canvas[gy - 2:gy + 3, gx - 2:gx + 3] = 2

        # Giant cute owl eyes right below goggles
        draw_kawaii_eye(canvas, cx - 12, cy - 4, size=4, pupil_c=11, glint_c=10, blush_c=12)
        draw_kawaii_eye(canvas, cx + 12, cy - 4, size=4, pupil_c=11, glint_c=10, blush_c=12)

        # Golden beak
        canvas[cy + 4:cy + 10, cx - 3:cx + 4] = 3
        canvas[cy + 10:cy + 13, cx - 1:cx + 2] = 4

        # Steampunk clockwork wings with turning gears
        w_offset = 6 if f == 1 else -4
        for wx, sign in ((cx - 40, -1), (cx + 40, 1)):
            fill_ellipse(canvas, wx, cy + w_offset, 18, 12, 7)
            # Gear teeth
            fill_circle(canvas, wx, cy + w_offset, 8, 3)
            fill_circle(canvas, wx, cy + w_offset, 4, 4)

        canvas = draw_outline(canvas, 1)
        frames[str(f)] = canvas
    return frames, pal


def build_boss_jackal_frames() -> tuple[dict[str, np.ndarray], np.ndarray]:
    """128x128 Inferno Jackal: Fluffy cute anime fire kitsune with 3 flaming tails."""
    pal = np.array([
        [0, 0, 0], [45, 15, 10], [255, 245, 140], [255, 185, 45],
        [240, 100, 30], [180, 45, 15], [255, 220, 200], [255, 255, 255],
        [30, 15, 15], [255, 120, 160], [255, 60, 40], [190, 25, 20],
        [120, 15, 15], [255, 200, 80], [200, 140, 30], [130, 80, 15]
    ], dtype=np.uint8)

    frames = {}
    for f in range(2):
        canvas = np.zeros((128, 128), dtype=np.uint8)
        cx, cy = 64, 76

        # 3 Giant fluffy cartoon fire tails waving behind
        tail_wave = (f * 6) - 3
        for tx, ty, r in ((cx - 36, cy - 10 + tail_wave, 18), (cx + 36, cy - 10 - tail_wave, 18), (cx, cy - 28 + tail_wave, 16)):
            fill_circle(canvas, tx, ty, r, 4)
            fill_circle(canvas, tx, ty, r - 4, 3)
            fill_circle(canvas, tx, ty, r - 8, 2)

        # Fluffy body
        fill_circle(canvas, cx, cy + 4, 26, 4)
        fill_circle(canvas, cx, cy + 2, 23, 3)
        # White fluffy chest
        fill_circle(canvas, cx, cy + 8, 14, 7)

        # Fox head
        hy = cy - 14
        fill_circle(canvas, cx, hy, 20, 3)
        fill_circle(canvas, cx, hy + 2, 16, 6)

        # Big cute fox ears
        for ex, sign in ((cx - 14, -1), (cx + 14, 1)):
            ear_y = hy - 16
            fill_ellipse(canvas, ex, ear_y, 7, 12, 4)
            fill_ellipse(canvas, ex, ear_y, 4, 8, 6)

        # Anime fox eyes with ruby sparkles
        draw_kawaii_eye(canvas, cx - 8, hy, size=3, pupil_c=8, glint_c=7, blush_c=9)
        draw_kawaii_eye(canvas, cx + 8, hy, size=3, pupil_c=8, glint_c=7, blush_c=9)

        # Cute little black nose & smile
        canvas[hy + 6, cx] = 8
        canvas[hy + 8, cx - 2:cx + 3] = 8

        # Cute paws with flame puffs
        for px in (cx - 16, cx + 16):
            fill_circle(canvas, px, cy + 28, 6, 7)
            fill_circle(canvas, px, cy + 30, 3, 2)  # tiny spark

        canvas = draw_outline(canvas, 1)
        frames[str(f)] = canvas
    return frames, pal


def build_boss_owl_frames() -> tuple[dict[str, np.ndarray], np.ndarray]:
    """128x128 Blizzard Owl: Huge round fluffy snow owl with winter scarf."""
    pal = np.array([
        [0, 0, 0], [20, 25, 45], [255, 255, 255], [225, 240, 255],
        [175, 205, 245], [115, 150, 210], [255, 220, 80], [240, 160, 40],
        [25, 20, 30], [255, 130, 175], [255, 90, 120], [195, 40, 75],
        [135, 20, 45], [130, 220, 240], [70, 160, 195], [35, 95, 135]
    ], dtype=np.uint8)

    frames = {}
    for f in range(2):
        canvas = np.zeros((128, 128), dtype=np.uint8)
        cx, cy = 64, 72

        # Giant fluffy round snowy body
        fill_circle(canvas, cx, cy, 38, 4)
        fill_circle(canvas, cx, cy - 3, 35, 3)
        fill_circle(canvas, cx, cy - 6, 30, 2)  # pure white fluff

        # Fluffy wings flapping
        wing_y = cy + (4 if f == 1 else -4)
        fill_ellipse(canvas, cx - 42, wing_y, 16, 22, 3)
        fill_ellipse(canvas, cx + 42, wing_y, 16, 22, 3)

        # Knitted striped red winter scarf around neck!
        sy = cy + 4
        fill_ellipse(canvas, cx, sy, 28, 9, 10)
        # scarf stripes
        canvas[sy - 3:sy + 4, cx - 18:cx - 12] = 2
        canvas[sy - 3:sy + 4, cx - 2:cx + 4] = 2
        canvas[sy - 3:sy + 4, cx + 12:cx + 18] = 2
        # scarf tail hanging
        canvas[sy + 8:sy + 24, cx + 14:cx + 22] = 10
        canvas[sy + 14:sy + 18, cx + 14:cx + 22] = 2

        # Giant warm golden eyes
        draw_kawaii_eye(canvas, cx - 14, cy - 14, size=6, pupil_c=8, glint_c=2, blush_c=9)
        draw_kawaii_eye(canvas, cx + 14, cy - 14, size=6, pupil_c=8, glint_c=2, blush_c=9)

        # Beak
        canvas[cy - 6:cy + 1, cx - 2:cx + 3] = 6
        canvas[cy + 1, cx] = 7

        # Snowflake crystal tiara
        canvas[cy - 38:cy - 30, cx] = 13
        canvas[cy - 34, cx - 6:cx + 7] = 13

        canvas = draw_outline(canvas, 1)
        frames[str(f)] = canvas
    return frames, pal


def build_boss_leviathan_frames() -> tuple[dict[str, np.ndarray], np.ndarray]:
    """128x128 Toxic Leviathan: Cute sea dragon / baby Nessie with dorsal fins and bubbles."""
    pal = np.array([
        [0, 0, 0], [15, 30, 50], [215, 255, 255], [130, 230, 245],
        [65, 175, 215], [30, 110, 165], [255, 230, 100], [255, 255, 255],
        [15, 20, 35], [255, 135, 185], [175, 120, 230], [115, 65, 180],
        [65, 30, 120], [255, 165, 60], [200, 100, 30], [130, 50, 15]
    ], dtype=np.uint8)

    frames = {}
    for f in range(2):
        canvas = np.zeros((128, 128), dtype=np.uint8)
        cx, cy = 64, 76

        # Cute undulating aquatic body
        wave = math.sin(f * math.pi) * 4
        fill_ellipse(canvas, cx, cy + int(wave), 36, 26, 4)
        fill_ellipse(canvas, cx, cy - 3 + int(wave), 33, 23, 3)
        # Pastel purple tummy
        fill_ellipse(canvas, cx, cy + 6 + int(wave), 22, 14, 10)

        # Cute long dragon neck and round head
        head_y = cy - 26 - int(wave)
        fill_circle(canvas, cx, head_y, 20, 3)
        fill_circle(canvas, cx - 4, head_y - 4, 14, 2)  # glossy shine

        # Dorsal fins (scalloped sea shells)
        for fy in (cy - 38, cy - 14, cy + 10):
            fill_circle(canvas, cx - 22, fy + int(wave), 7, 10)
            fill_circle(canvas, cx + 22, fy + int(wave), 7, 10)

        # Big sparkling kawaii dragon eyes
        draw_kawaii_eye(canvas, cx - 8, head_y - 2, size=4, pupil_c=8, glint_c=7, blush_c=9)
        draw_kawaii_eye(canvas, cx + 8, head_y - 2, size=4, pupil_c=8, glint_c=7, blush_c=9)

        # Cheerful smile showing single cute tooth
        canvas[head_y + 8, cx - 3:cx + 4] = 8
        canvas[head_y + 6, cx - 1:cx + 1] = 7  # tooth!

        # Floating cartoon bubbles around
        for bx, by, br in ((cx - 38, cy - 30, 5), (cx + 42, cy - 20, 4), (cx - 44, cy + 10, 6)):
            fill_circle(canvas, bx, by, br, 2)
            fill_circle(canvas, bx, by, br - 1, 0)
            canvas[by - 1, bx - 1] = 7

        canvas = draw_outline(canvas, 1)
        frames[str(f)] = canvas
    return frames, pal


def build_boss_smoggar_frames() -> tuple[dict[str, np.ndarray], np.ndarray]:
    """128x128 Lord Smoggar: Kirby-style final boss floating dark sorcerer (Magolor/Marx style)."""
    pal = np.array([
        [0, 0, 0], [25, 15, 40], [255, 240, 120], [245, 185, 50],
        [180, 125, 25], [215, 140, 255], [160, 75, 220], [105, 35, 160],
        [55, 15, 95], [255, 255, 255], [20, 10, 25], [255, 105, 165],
        [75, 215, 255], [25, 145, 210], [255, 80, 80], [180, 30, 50]
    ], dtype=np.uint8)

    frames = {}
    for f in range(2):
        canvas = np.zeros((128, 128), dtype=np.uint8)
        cx, cy = 64, 68 + (4 if f == 1 else -4)

        # Flowing starry dark cape
        for y in range(cy, cy + 45):
            t = (y - cy) / 45.0
            spread = int(24 + t * 24 + math.sin(t * math.pi * 2 + f) * 4)
            canvas[y, cx - spread:cx + spread + 1] = 7
            canvas[y, cx - spread + 4:cx + spread - 3] = 6
        # Little gold stars on cape hem
        canvas[cy + 40:cy + 43, cx - 18:cx - 14] = 2
        canvas[cy + 40:cy + 43, cx + 14:cx + 18] = 2

        # Round cute sorcerer head / hood
        fill_circle(canvas, cx, cy - 6, 24, 7)
        fill_circle(canvas, cx, cy - 8, 21, 6)
        # Shadowed inner face
        fill_circle(canvas, cx, cy - 4, 16, 10)

        # Glowing golden anime eyes peering out of darkness!
        draw_kawaii_eye(canvas, cx - 7, cy - 5, size=4, pupil_c=2, glint_c=9, blush_c=11)
        draw_kawaii_eye(canvas, cx + 7, cy - 5, size=4, pupil_c=2, glint_c=9, blush_c=11)

        # Golden crown with ruby gem
        cy_crown = cy - 30
        canvas[cy_crown:cy_crown + 8, cx - 14:cx + 15] = 3
        canvas[cy_crown - 4:cy_crown, cx - 14] = 2
        canvas[cy_crown - 6:cy_crown, cx] = 2
        canvas[cy_crown - 4:cy_crown, cx + 14] = 2
        fill_circle(canvas, cx, cy_crown + 4, 2, 14)  # ruby!

        # Disembodied floating cartoon magic hands casting sparkling star orbs!
        hand_y = cy + 10 + (f * 4)
        for hx, sign in ((cx - 40, -1), (cx + 40, 1)):
            fill_circle(canvas, hx, hand_y, 10, 2)
            fill_circle(canvas, hx, hand_y, 7, 9)
            # Magic star sparkle orb floating above hand
            fill_circle(canvas, hx, hand_y - 14, 6, 12)
            canvas[hand_y - 14, hx] = 9

        canvas = draw_outline(canvas, 1)
        frames[str(f)] = canvas
    return frames, pal


# ------------------------------------------------------------------
#  Direct Cute Allies & NPCs (48x64)
# ------------------------------------------------------------------
def build_allies_frames() -> dict[str, tuple[dict[str, np.ndarray], np.ndarray]]:
    res = {}

    # 1. Sunboy (3 frames)
    sun_pal = np.array([
        [0, 0, 0], [30, 25, 20], [255, 245, 130], [255, 195, 45],
        [200, 135, 25], [255, 225, 185], [250, 185, 135], [255, 255, 255],
        [20, 20, 30], [255, 130, 160], [80, 175, 255], [30, 115, 215],
        [15, 65, 150], [255, 155, 45], [200, 95, 20], [130, 50, 10]
    ], dtype=np.uint8)
    sun_frames = {}
    for f in range(3):
        canvas = np.zeros((64, 48), dtype=np.uint8)
        cx, cy = 24, 34
        # Body / tunic
        fill_circle(canvas, cx, cy + 8, 9, 11)
        fill_circle(canvas, cx, cy + 7, 8, 10)
        # Face
        fill_circle(canvas, cx, cy - 6, 10, 5)
        # Golden spiky anime hair
        fill_circle(canvas, cx, cy - 12, 11, 3)
        for hx, hy in ((cx - 8, cy - 18), (cx, cy - 22), (cx + 8, cy - 18), (cx - 12, cy - 10), (cx + 12, cy - 10)):
            fill_circle(canvas, hx, hy, 4, 2)
        # Big cheerful eyes
        draw_kawaii_eye(canvas, cx - 4, cy - 6, size=2, pupil_c=8, glint_c=7, blush_c=9)
        draw_kawaii_eye(canvas, cx + 4, cy - 6, size=2, pupil_c=8, glint_c=7, blush_c=9)
        canvas[cy - 2, cx] = 8  # smile
        # Glowing sun staff in hand
        staff_x = cx + 14
        canvas[cy - 12:cy + 22, staff_x] = 4
        fill_circle(canvas, staff_x, cy - 14, 5, 2)  # sun crystal
        canvas[cy - 14, staff_x] = 7
        # Feet
        fill_circle(canvas, cx - 4, cy + 20, 3, 12)
        fill_circle(canvas, cx + 4, cy + 20, 3, 12)
        canvas = draw_outline(canvas, 1)
        sun_frames[str(f)] = canvas
    res["sunboy"] = (sun_frames, sun_pal)

    # 2. Nature Spirit Fairy (2 frames)
    spirit_pal = np.array([
        [0, 0, 0], [25, 20, 45], [255, 255, 255], [215, 245, 255],
        [150, 215, 255], [85, 155, 230], [255, 205, 235], [245, 125, 185],
        [185, 55, 125], [255, 235, 95], [255, 130, 170], [20, 15, 30],
        [185, 255, 170], [115, 225, 100], [55, 160, 45], [25, 95, 25]
    ], dtype=np.uint8)
    sp_frames = {}
    for f in range(2):
        canvas = np.zeros((64, 48), dtype=np.uint8)
        cx, cy = 24, 30 + (2 if f == 1 else -2)
        # Translucent sparkling dragonfly wings
        fill_ellipse(canvas, cx - 12, cy - 8, 8, 14, 3)
        fill_ellipse(canvas, cx + 12, cy - 8, 8, 14, 3)
        fill_ellipse(canvas, cx - 10, cy - 8, 5, 10, 2)
        fill_ellipse(canvas, cx + 10, cy - 8, 5, 10, 2)
        # Flower petal dress
        fill_ellipse(canvas, cx, cy + 8, 7, 10, 7)
        fill_ellipse(canvas, cx, cy + 7, 5, 8, 6)
        # Fairy chibi face
        fill_circle(canvas, cx, cy - 4, 8, 6)
        # Flower blossom hair
        fill_circle(canvas, cx, cy - 10, 9, 13)
        fill_circle(canvas, cx - 6, cy - 12, 4, 9)
        fill_circle(canvas, cx + 6, cy - 12, 4, 9)
        # Sparkling anime eyes
        draw_kawaii_eye(canvas, cx - 3, cy - 4, size=2, pupil_c=11, glint_c=2, blush_c=10)
        draw_kawaii_eye(canvas, cx + 3, cy - 4, size=2, pupil_c=11, glint_c=2, blush_c=10)
        canvas = draw_outline(canvas, 1)
        sp_frames[str(f)] = canvas
    res["spirit"] = (sp_frames, spirit_pal)

    # 3. Elder (2 frames)
    elder_pal = np.array([
        [0, 0, 0], [35, 25, 20], [255, 255, 255], [225, 230, 240],
        [170, 175, 190], [255, 220, 185], [240, 180, 135], [180, 120, 75],
        [125, 75, 35], [255, 215, 85], [255, 130, 160], [25, 20, 25],
        [165, 235, 95], [95, 175, 45], [160, 105, 50], [95, 55, 20]
    ], dtype=np.uint8)
    el_frames = {}
    for f in range(2):
        canvas = np.zeros((64, 48), dtype=np.uint8)
        cx, cy = 24, 34
        # Robes
        fill_ellipse(canvas, cx, cy + 10, 12, 16, 7)
        fill_ellipse(canvas, cx, cy + 9, 10, 14, 6)
        # Face
        fill_circle(canvas, cx, cy - 6, 8, 5)
        # Giant fluffy white cloud beard
        fill_ellipse(canvas, cx, cy + 6, 10, 12, 2)
        fill_circle(canvas, cx - 5, cy + 8, 6, 3)
        fill_circle(canvas, cx + 5, cy + 8, 6, 3)
        # Happy smiling squinty eyes
        draw_kawaii_eye(canvas, cx - 4, cy - 6, size=2, pupil_c=11, glint_c=2, blush_c=10, happy=True)
        draw_kawaii_eye(canvas, cx + 4, cy - 6, size=2, pupil_c=11, glint_c=2, blush_c=10, happy=True)
        # Wooden staff with sprouted leaf
        staff_x = cx + 12
        canvas[cy - 16:cy + 24, staff_x] = 14
        canvas[cy - 18, staff_x - 1:staff_x + 3] = 12  # leaf!
        canvas = draw_outline(canvas, 1)
        el_frames[str(f)] = canvas
    res["elder"] = (el_frames, elder_pal)

    # 4. Valley Girl (2 frames)
    girl_pal = np.array([
        [0, 0, 0], [35, 20, 30], [255, 245, 140], [255, 195, 45],
        [200, 135, 25], [255, 225, 185], [250, 185, 135], [255, 255, 255],
        [25, 15, 25], [255, 125, 175], [145, 225, 85], [75, 165, 40],
        [35, 105, 25], [255, 150, 50], [200, 90, 20], [130, 45, 10]
    ], dtype=np.uint8)
    girl_frames = {}
    for f in range(2):
        canvas = np.zeros((64, 48), dtype=np.uint8)
        cx, cy = 24, 34
        # Apron dress
        fill_circle(canvas, cx, cy + 8, 9, 11)
        fill_circle(canvas, cx, cy + 7, 7, 10)
        # Face
        fill_circle(canvas, cx, cy - 6, 9, 5)
        # Blonde hair with twin-tails
        fill_circle(canvas, cx, cy - 10, 10, 3)
        fill_ellipse(canvas, cx - 10, cy - 6, 4, 8, 2)
        fill_ellipse(canvas, cx + 10, cy - 6, 4, 8, 2)
        # Red flower hair ribbon
        canvas[cy - 14, cx - 8:cx - 5] = 9
        canvas[cy - 14, cx + 5:cx + 8] = 9
        # Sparkling anime eyes
        draw_kawaii_eye(canvas, cx - 4, cy - 6, size=2, pupil_c=8, glint_c=7, blush_c=9)
        draw_kawaii_eye(canvas, cx + 4, cy - 6, size=2, pupil_c=8, glint_c=7, blush_c=9)
        canvas[cy - 2, cx] = 8  # smile
        # Waving hand
        wave_y = cy - 8 if f == 1 else cy
        fill_circle(canvas, cx + 12, wave_y, 3, 5)
        canvas = draw_outline(canvas, 1)
        girl_frames[str(f)] = canvas
    res["girl"] = (girl_frames, girl_pal)

    return res


# ------------------------------------------------------------------
#  Direct Textured Platform Ledge Blocks (32x32, 3 pieces each)
# ------------------------------------------------------------------
def build_blocks() -> dict[str, tuple[dict[str, np.ndarray], np.ndarray]]:
    grounds = {
        "grass": np.array([
            [0, 0, 0], [25, 35, 20], [210, 255, 120], [135, 220, 60],
            [75, 160, 35], [35, 95, 20], [165, 110, 65], [115, 70, 40],
            [75, 45, 25], [45, 25, 15], [255, 120, 160], [255, 235, 90],
            [140, 160, 175], [90, 105, 120], [55, 65, 80], [30, 35, 45]
        ], dtype=np.uint8),
        "sand": np.array([
            [0, 0, 0], [45, 30, 15], [255, 235, 150], [245, 195, 85],
            [195, 140, 50], [135, 90, 25], [225, 165, 75], [175, 115, 40],
            [120, 70, 20], [80, 45, 10], [255, 215, 60], [200, 150, 30],
            [150, 100, 20], [100, 60, 15], [70, 40, 10], [40, 20, 5]
        ], dtype=np.uint8),
        "stone": np.array([
            [0, 0, 0], [25, 30, 35], [235, 240, 245], [185, 195, 205],
            [135, 145, 155], [85, 95, 105], [50, 60, 70], [155, 215, 100],
            [90, 155, 55], [45, 95, 30], [200, 210, 220], [150, 160, 170],
            [100, 110, 120], [70, 75, 85], [40, 45, 55], [20, 25, 30]
        ], dtype=np.uint8),
        "planet": np.array([
            [0, 0, 0], [30, 15, 45], [255, 215, 255], [225, 140, 245],
            [170, 70, 200], [110, 30, 145], [55, 15, 85], [100, 220, 255],
            [40, 150, 220], [15, 85, 155], [255, 240, 110], [240, 175, 40],
            [180, 110, 20], [120, 60, 10], [70, 30, 5], [40, 15, 5]
        ], dtype=np.uint8),
        "snow": np.array([
            [0, 0, 0], [20, 30, 50], [255, 255, 255], [220, 240, 255],
            [165, 205, 245], [105, 155, 215], [55, 105, 165], [145, 225, 245],
            [85, 170, 210], [40, 115, 160], [255, 255, 255], [210, 230, 250],
            [150, 185, 220], [95, 135, 175], [50, 85, 125], [25, 50, 80]
        ], dtype=np.uint8),
        "dirt": np.array([
            [0, 0, 0], [35, 25, 15], [215, 175, 130], [175, 125, 80],
            [130, 85, 45], [85, 50, 25], [50, 30, 15], [255, 215, 90],
            [200, 150, 40], [140, 95, 20], [255, 140, 180], [195, 60, 110],
            [125, 30, 65], [75, 15, 35], [45, 10, 20], [25, 5, 10]
        ], dtype=np.uint8),
        "metal": np.array([
            [0, 0, 0], [30, 35, 40], [255, 255, 255], [215, 225, 235],
            [155, 165, 180], [100, 110, 125], [55, 65, 75], [255, 220, 50],
            [220, 160, 20], [160, 105, 10], [255, 100, 100], [195, 40, 40],
            [130, 20, 20], [180, 190, 200], [120, 130, 140], [70, 80, 90]
        ], dtype=np.uint8),
    }

    res = {}
    for gname, pal in grounds.items():
        pieces = {}
        for piece in range(3):  # 0: left, 1: mid, 2: right
            canvas = np.zeros((32, 32), dtype=np.uint8)
            # Base stone/soil block with layered texture
            canvas[8:32, :] = 8
            canvas[12:28, :] = 7
            # Brick seams / textures
            for y in (16, 24):
                canvas[y, :] = 9
            for x in (8, 16, 24):
                canvas[8:16, x] = 9
                canvas[16:24, (x + 8) % 32] = 9
                canvas[24:32, x] = 9

            # Bevel edges
            canvas[8, :] = 6
            if piece == 0:  # left cap
                canvas[8:32, :4] = 9
                canvas[8:32, 0] = 1
            elif piece == 2:  # right cap
                canvas[8:32, 28:] = 9
                canvas[8:32, 31] = 1

            # Top surface layer (grass/snow/metal rim)
            if gname == "grass":
                # Lush green grass clumps hanging over edge
                canvas[4:10, :] = 3
                canvas[3:8, :] = 2  # bright highlight
                # Scalloped grass fringe
                for x in range(32):
                    fringe_len = int(4 + math.sin(x * 0.8) * 3)
                    canvas[8:8 + fringe_len, x] = 4
                # Cute little red flowers
                for fx in (6, 18, 26):
                    canvas[4:6, fx:fx + 2] = 10
                    canvas[5, fx] = 11
            elif gname == "snow":
                # Puffy white snow cap with icicles
                canvas[2:9, :] = 2
                canvas[5:10, :] = 3
                for x in range(2, 30, 5):
                    canvas[9:14, x] = 3
                    canvas[9:12, x] = 2
            elif gname == "metal":
                # Yellow & black hazard stripes on rim
                canvas[4:9, :] = 7
                for x in range(32):
                    if (x // 4) % 2 == 0:
                        canvas[4:9, x] = 1
                # Rivets
                canvas[12, 4] = 2
                canvas[12, 27] = 2
            else:
                # Beveled stone/sand top
                canvas[4:9, :] = 3
                canvas[3:7, :] = 2

            canvas = draw_outline(canvas, 1)
            pieces[str(piece)] = canvas
        res[f"block_{gname}"] = (pieces, pal)
    return res


# ------------------------------------------------------------------
#  Main Build Routine
# ------------------------------------------------------------------
def build_direct():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    PREVIEW_DIR.mkdir(parents=True, exist_ok=True)
    ROMS_DIR.mkdir(parents=True, exist_ok=True)

    c1, c2 = io.BytesIO(), io.BytesIO()
    header = [
        "/* Generated by games/maiya/tools/direct_c_rom_builder.py. */",
        "#ifndef MAIYA_ASSETS_H",
        "#define MAIYA_ASSETS_H",
        "#include <stdint.h>",
        "",
    ]
    manifest = []

    def store_indexed(name: str, indices: np.ndarray, pal_rgb: np.ndarray,
                      emit_palette: bool = True, c_pal_name: str | None = None):
        h, w = indices.shape
        count = h * w // 256
        base = c1.tell() // 64
        lo, hi = encode_image(indices, count)
        assert np.array_equal(decode_image(lo, hi, w, h), indices)
        c1.write(lo)
        c2.write(hi)
        header.append(f"#define MG_{name.upper()}_TILE {base}u")

        # Visual preview
        rgb = pal_rgb[indices]
        alpha = np.where(indices > 0, 255, 0).astype(np.uint8)
        rgba = np.dstack([rgb, alpha])
        Image.fromarray(rgba).save(OUTPUT / f"{name}.png")
        Image.fromarray(rgba).save(PREVIEW_DIR / f"{name}.png")

        manifest.append(dict(name=name, tile_base=base, tile_last=base + count - 1,
                             strips=w // 16, rows=h // 16, stride=w // 16,
                             palette_count=1))
        if emit_palette:
            pname = c_pal_name or f"mg_{name}_pal"
            header.append(c_array(pname, palette_words(pal_rgb)))
        return base

    print("== 1. Preserving 6 HD Stage Environments ==", flush=True)
    # Stage backgrounds bg0..bg5 (512x192) and ground0..ground5 (512x32)
    # Load from previous generated assets to retain environment layout
    with open(OUTPUT / "assets.json") as f:
        prev_manifest = json.load(f)

    for i in range(6):
        # bg
        bg_rgba = np.asarray(Image.open(PREVIEW_DIR / f"bg{i}.png"))
        bg_idx = np.asarray(Image.open(OUTPUT / f"bg{i}.png").convert("P"))
        # load existing palette from header
        # store
        h, w = bg_rgba.shape[:2]
        count = h * w // 256
        base = c1.tell() // 64
        # We can read original bytes from existing C-ROM
        orig_c1 = (OUTPUT / "780-c1.c1").read_bytes()
        orig_c2 = (OUTPUT / "780-c2.c2").read_bytes()
        start = base * 64
        end = (base + count) * 64
        c1.write(orig_c1[start:end])
        c2.write(orig_c2[start:end])
        header.append(f"#define MG_BG{i}_TILE {base}u")
        manifest.append(dict(name=f"bg{i}", tile_base=base, tile_last=base + count - 1,
                             strips=32, rows=12, stride=32, palette_count=16))

        # ground
        base_g = c1.tell() // 64
        count_g = 32 * 2
        start_g = base_g * 64
        end_g = (base_g + count_g) * 64
        c1.write(orig_c1[start_g:end_g])
        c2.write(orig_c2[start_g:end_g])
        header.append(f"#define MG_GROUND{i}_TILE {base_g}u")
        manifest.append(dict(name=f"ground{i}", tile_base=base_g, tile_last=base_g + count_g - 1,
                             strips=32, rows=2, stride=32, palette_count=16))

        # Re-emit stage palettes and maps from existing header
        # (read from maiya_assets.h directly)
        print(f"  Stage {i + 1}/6 preserved (512x224)", flush=True)

    # Copy the palette lines for backgrounds from existing header
    orig_header = (OUTPUT / "maiya_assets.h").read_text(encoding="ascii")
    # Extract background palettes up to hero_idle0
    bg_section = orig_header[orig_header.find("static const uint16_t mg_bg0_pal"):orig_header.find("#define MG_HERO_IDLE0_TILE")]
    header.append(bg_section)

    print("== 2. Preserving Maiya Heroine Moveset (39 frames) ==", flush=True)
    hero_frames = [e for e in prev_manifest if e["name"].startswith("hero_")]
    hero_tiles = []
    for entry in hero_frames:
        name = entry["name"]
        h, w = entry["rows"] * 16, entry["strips"] * 16
        count = h * w // 256
        base = c1.tell() // 64
        start = entry["tile_base"] * 64
        end = (entry["tile_last"] + 1) * 64
        c1.write(orig_c1[start:end])
        c2.write(orig_c2[start:end])
        header.append(f"#define MG_{name.upper()}_TILE {base}u")
        hero_tiles.append(base)
        manifest.append(dict(name=name, tile_base=base, tile_last=base + count - 1,
                             strips=entry["strips"], rows=entry["rows"], stride=entry["stride"],
                             palette_count=1))
    header.append(c_array("mg_hero_tiles", hero_tiles))
    header.append("#define MG_HERO_FRAMES 39u")
    for k, entry in enumerate(hero_frames):
        short_name = entry["name"].replace("hero_", "")
        header.append(f"#define MG_F_{short_name.upper()} {k}u")

    # Extract hero palettes from existing header
    hero_pal_section = orig_header[orig_header.find("static const uint16_t mg_hero_pal"):orig_header.find("#define MG_EAGLE_PERCH0_TILE")]
    header.append(hero_pal_section)
    print("  Maiya 39 moves preserved perfectly", flush=True)

    print("== 3. Compiling Eagle Guardian ==", flush=True)
    eagle_entries = [e for e in prev_manifest if e["name"].startswith("eagle_")]
    eagle_tiles = []
    for entry in eagle_entries:
        name = entry["name"]
        count = entry["rows"] * entry["strips"]
        base = c1.tell() // 64
        start = entry["tile_base"] * 64
        end = (entry["tile_last"] + 1) * 64
        c1.write(orig_c1[start:end])
        c2.write(orig_c2[start:end])
        header.append(f"#define MG_{name.upper()}_TILE {base}u")
        eagle_tiles.append(base)
        manifest.append(dict(name=name, tile_base=base, tile_last=base + count - 1,
                             strips=entry["strips"], rows=entry["rows"], stride=entry["stride"],
                             palette_count=1))
    header.append(c_array("mg_eagle_tiles", eagle_tiles))
    eagle_pal_section = orig_header[orig_header.find("static const uint16_t mg_eagle_pal"):orig_header.find("#define MG_FACE_TILE")]
    header.append(eagle_pal_section)

    print("== 4. Compiling Cute Face HUD & Portrait ==", flush=True)
    for name, strips, rows in (("face", 2, 2), ("portrait", 7, 6)):
        entry = next(e for e in prev_manifest if e["name"] == name)
        count = strips * rows
        base = c1.tell() // 64
        start = entry["tile_base"] * 64
        end = (entry["tile_last"] + 1) * 64
        c1.write(orig_c1[start:end])
        c2.write(orig_c2[start:end])
        header.append(f"#define MG_{name.upper()}_TILE {base}u")
        manifest.append(dict(name=name, tile_base=base, tile_last=base + count - 1,
                             strips=strips, rows=rows, stride=strips, palette_count=entry["palette_count"]))
    face_pal_section = orig_header[orig_header.find("static const uint16_t mg_portrait_pal"):orig_header.find("#define MG_SLIME_0_TILE")]
    header.append(face_pal_section)

    print("== 5. Directly Drawing 6 Kawaii Kirby-Style Blight Enemies ==", flush=True)
    enemy_builders = [
        ("slime", build_slime_frames, 3, 2),
        ("beetle", build_beetle_frames, 4, 3),
        ("crow", build_crow_frames, 4, 3),
        ("goblin", build_goblin_frames, 3, 4),
        ("worm", build_worm_frames, 2, 5),
        ("robot", build_robot_frames, 3, 3),
    ]
    for ename, builder, strips, rows in enemy_builders:
        frames, pal = builder()
        tiles = []
        for k, canvas in frames.items():
            base = store_indexed(f"{ename}_{k}", canvas, pal, emit_palette=False)
            tiles.append(base)
        header.append(c_array(f"mg_{ename}_tiles", tiles))
        header.append(c_array(f"mg_{ename}_pal", palette_words(pal)))
        header.append(f"#define MG_{ename.upper()}_FRAMES {len(frames)}u")
        header.append(f"#define MG_{ename.upper()}_W {strips * 16}u")
        header.append(f"#define MG_{ename.upper()}_H {rows * 16}u")
        print(f"  Enemy {ename} drawn directly ({len(frames)} cute frames, {strips}x{rows} tiles)", flush=True)

    print("== 6. Directly Drawing Cute Allies & NPCs ==", flush=True)
    allies = build_allies_frames()
    for nname, (frames, pal) in allies.items():
        tiles = []
        for k, canvas in frames.items():
            base = store_indexed(f"{nname}_{k}", canvas, pal, emit_palette=False)
            tiles.append(base)
        header.append(c_array(f"mg_{nname}_tiles", tiles))
        header.append(c_array(f"mg_{nname}_pal", palette_words(pal)))
        header.append(f"#define MG_{nname.upper()}_FRAMES {len(frames)}u")
        header.append(f"#define MG_{nname.upper()}_W 48u")
        header.append(f"#define MG_{nname.upper()}_H 64u")
        print(f"  NPC {nname} drawn directly ({len(frames)} frames)", flush=True)

    print("== 7. Directly Drawing 7 Kawaii Guardians / Bosses (128x128) ==", flush=True)
    boss_builders = [
        ("beetle", build_boss_beetle_frames),
        ("toad", build_boss_toad_frames),
        ("vulture", build_boss_vulture_frames),
        ("jackal", build_boss_jackal_frames),
        ("owl", build_boss_owl_frames),
        ("leviathan", build_boss_leviathan_frames),
        ("smoggar", build_boss_smoggar_frames),
    ]
    for bname, builder in boss_builders:
        frames, pal = builder()
        tiles = []
        for k, canvas in frames.items():
            base = store_indexed(f"boss_{bname}_{k}", canvas, pal, emit_palette=False)
            tiles.append(base)
        header.append(c_array(f"mg_boss_{bname}_tiles", tiles))
        header.append(c_array(f"mg_boss_{bname}_pal", palette_words(pal)))
        print(f"  Boss {bname} drawn directly (128x128, 2 frames)", flush=True)

    print("== 8. Compiling Props, Hazards, and Pickups ==", flush=True)
    # Props and items from existing clean reservations
    prop_entries = [e for e in prev_manifest if e["name"].startswith("prop_")]
    for entry in prop_entries:
        name = entry["name"]
        count = 4  # 2x2
        base = c1.tell() // 64
        start = entry["tile_base"] * 64
        end = (entry["tile_last"] + 1) * 64
        c1.write(orig_c1[start:end])
        c2.write(orig_c2[start:end])
        header.append(f"#define MG_{name.upper()}_TILE {base}u")
        manifest.append(dict(name=name, tile_base=base, tile_last=base + count - 1,
                             strips=2, rows=2, stride=2, palette_count=1))
    prop_section = orig_header[orig_header.find("static const uint16_t mg_prop_tiles"):orig_header.find("#define MG_ITEM_ROSE_GOLD_TILE")]
    header.append(prop_section)

    item_entries = [e for e in prev_manifest if e["name"].startswith("item_")]
    for entry in item_entries:
        name = entry["name"]
        count = 4  # 2x2
        base = c1.tell() // 64
        start = entry["tile_base"] * 64
        end = (entry["tile_last"] + 1) * 64
        c1.write(orig_c1[start:end])
        c2.write(orig_c2[start:end])
        header.append(f"#define MG_{name.upper()}_TILE {base}u")
        manifest.append(dict(name=name, tile_base=base, tile_last=base + count - 1,
                             strips=2, rows=2, stride=2, palette_count=1))
    item_section = orig_header[orig_header.find("static const uint16_t mg_item_tiles"):orig_header.find("#define MG_BLOCK_GRASS_0_TILE")]
    header.append(item_section)

    print("== 9. Directly Drawing Textured Platform Ledge Blocks ==", flush=True)
    blocks_dict = build_blocks()
    for gname, (pieces, pal) in blocks_dict.items():
        tiles = []
        for piece in range(3):
            canvas = pieces[str(piece)]
            base = store_indexed(f"{gname}_{piece}", canvas, pal, emit_palette=False)
            tiles.append(base)
        header.append(c_array(f"mg_{gname}_tiles", tiles))
        header.append(c_array(f"mg_{gname}_pal", palette_words(pal)))
        print(f"  {gname} 3-piece blocks drawn directly with depth & bevels", flush=True)

    print("== 10. Compiling Projectile and HUD Tools Sheet ==", flush=True)
    # Read tool sheet from original
    tool_entry = next(e for e in prev_manifest if e["name"] == "tools") if any(e["name"] == "tools" for e in prev_manifest) else None
    # Use draw_tools() from build_commercial_assets
    import build_commercial_assets
    tool_sheet, tool_colors = build_commercial_assets.draw_tools()
    base_tool = c1.tell() // 64
    lo, hi = encode_image(tool_sheet, tool_sheet.size // 256)
    c1.write(lo)
    c2.write(hi)
    header.append(f"#define MG_TOOL_TILE {base_tool}u")
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
    header.append(c_array("mg_tool_pal", palette_words(tool_colors)))

    print("== 11. Compiling Clean Title Screen Key Visual ==", flush=True)
    title_entry = next(e for e in prev_manifest if e["name"] == "title")
    count_t = 19 * 14
    base_t = c1.tell() // 64
    start_t = title_entry["tile_base"] * 64
    end_t = (title_entry["tile_last"] + 1) * 64
    c1.write(orig_c1[start_t:end_t])
    c2.write(orig_c2[start_t:end_t])
    header.append(f"#define MG_TITLE_TILE {base_t}u")
    manifest.append(dict(name="title", tile_base=base_t, tile_last=base_t + count_t - 1,
                         strips=19, rows=14, stride=19, palette_count=2))
    title_section = orig_header[orig_header.find("static const uint16_t mg_title_pal"):orig_header.find("#endif")]
    header.append(title_section)

    write_utility_tiles(c1, c2)

    # Output C-ROM pair
    for data, suffix in ((c1.getvalue(), "c1"), (c2.getvalue(), "c2")):
        raw = np.frombuffer(data, dtype=np.uint8).reshape(-1, 2)[:, ::-1].tobytes()
        (OUTPUT / f"780-{suffix}.{suffix}").write_bytes(raw)
        (ROMS_DIR / f"780-{suffix}.{suffix}").write_bytes(raw)
        assert raw[0xFFFF * 64:] == bytes(64)

    header.append("#endif\n")
    (OUTPUT / "maiya_assets.h").write_text("\n".join(header), encoding="ascii")
    (OUTPUT / "assets.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="ascii")

    print(f"\nSUCCESS: Directly built {len(manifest)} assets ({c1.tell() // 1024} KB per C-ROM)")
    print(f"C-ROMs written to: {OUTPUT} and {ROMS_DIR}")


if __name__ == "__main__":
    build_direct()
