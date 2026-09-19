"""Test generator for smaller, high-definition Neo Geo arcade pixel art sprites:
- Maya Heroine (48x48) with full idle, walk, sit, whip lash, jump, and victory poses
- Blight Enemies (32x32 / 32x48) with expressive personality
- Biomechanical Nature Guardians (64x64)
- Nature background elements
"""

import math
import numpy as np
from PIL import Image

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

# 16-Color Palette for Maya:
# 0: Transparent, 1: Outline Dark Charcoal, 2: Dark Shade,
# 3: Blonde Highlight, 4: Blonde Mid, 5: Blonde Shade,
# 6: Skin Highlight, 7: Skin Mid, 8: Skin Shade,
# 9: Emerald Green Highlight, 10: Green Mid, 11: Green Shade,
# 12: Rose Pink Highlight, 13: Rose Crimson Mid,
# 14: Leather Brown, 15: Dark Leather
MAYA_PALETTE = np.array([
    [0, 0, 0],       # 0: trans
    [24, 20, 28],    # 1: outline
    [48, 32, 44],    # 2: shadow
    [255, 248, 170], # 3: hair hi
    [248, 204, 70],  # 4: hair mid
    [192, 136, 32],  # 5: hair shade
    [255, 236, 218], # 6: skin hi
    [255, 204, 176], # 7: skin mid
    [224, 152, 136], # 8: skin shade
    [90, 230, 115],  # 9: emerald hi
    [40, 168, 72],   # 10: green mid
    [20, 104, 48],   # 11: green shade
    [255, 128, 184], # 12: rose hi
    [216, 44, 102],  # 13: rose mid
    [152, 96, 48],   # 14: boot brown
    [92, 52, 26],    # 15: boot dark
], dtype=np.uint8)

def draw_maya_48(pose: str, tick: int = 0) -> np.ndarray:
    """Draw a 48x48 HD Neo Geo arcade sprite for Maya Heroine."""
    canvas = np.zeros((48, 48), dtype=np.uint8)
    cx = 24

    if pose == "sit":
        # Maya sitting cute on ground: y=24..44
        # Folded legs
        fill_ellipse(canvas, cx + 2, 41, 11, 4, 15)
        fill_ellipse(canvas, cx + 2, 40, 10, 3, 14)
        fill_ellipse(canvas, cx - 6, 41, 6, 3, 14)
        # Boots tucked
        canvas[40:43, cx + 10:cx + 14] = 14
        canvas[41:43, cx + 11:cx + 13] = 15

        # Tunic sitting flared
        fill_ellipse(canvas, cx - 1, 35, 10, 6, 11)
        fill_ellipse(canvas, cx - 1, 34, 9, 5, 10)
        fill_ellipse(canvas, cx - 1, 33, 7, 4, 9)
        # Rose pattern on skirt
        canvas[34, cx - 2] = 12
        canvas[34, cx - 1] = 13
        # Magenta sash tied at waist
        canvas[30:32, cx - 7:cx + 6] = 13
        canvas[30, cx - 5:cx + 4] = 12
        # Sash knot & rose clasp
        canvas[30:33, cx - 1:cx + 2] = 12
        canvas[31, cx] = 3

        # Torso
        fill_ellipse(canvas, cx - 1, 26, 6, 6, 10)
        fill_ellipse(canvas, cx - 1, 25, 5, 5, 9)
        # Neckline
        canvas[21:24, cx - 2:cx + 1] = 7

        # Cute hands resting on lap
        fill_circle(canvas, cx - 3, 31, 2, 7)
        fill_circle(canvas, cx + 3, 31, 2, 7)

        # Hair drape over shoulder
        fill_ellipse(canvas, cx - 8, 26, 4, 8, 4)
        fill_ellipse(canvas, cx - 8, 25, 3, 6, 3)

        # Head (y=15)
        hy = 16
        # Back hair volume
        fill_circle(canvas, cx, hy - 2, 10, 5)
        fill_circle(canvas, cx - 2, hy - 3, 9, 4)
        fill_circle(canvas, cx + 4, hy - 3, 7, 4)

        # Face
        fill_circle(canvas, cx - 1, hy, 7, 7)
        fill_circle(canvas, cx - 2, hy - 1, 6, 6)

        # Rose hair ornament
        fill_circle(canvas, cx - 7, hy - 5, 3, 13)
        fill_circle(canvas, cx - 7, hy - 6, 2, 12)
        canvas[hy - 5, cx - 8] = 9 # tiny green leaf

        # Blonde bangs
        canvas[hy - 6:hy - 2, cx - 5:cx + 4] = 4
        canvas[hy - 5:hy - 2, cx - 4:cx + 3] = 3
        # Twin bangs framing face
        canvas[hy - 3:hy + 3, cx - 6] = 4
        canvas[hy - 2:hy + 2, cx - 5] = 3
        canvas[hy - 3:hy + 3, cx + 5] = 4

        # Anime eyes (happy / peaceful cute expression)
        ey = hy - 1
        # Left eye
        canvas[ey, cx - 3] = 1
        canvas[ey - 1, cx - 4:cx - 2] = 1
        canvas[ey, cx - 4] = 9 # emerald eye
        canvas[ey - 1, cx - 4] = 6 # sparkle catchlight
        # Right eye
        canvas[ey, cx + 2] = 1
        canvas[ey - 1, cx + 1:cx + 3] = 1
        canvas[ey, cx + 1] = 9
        canvas[ey - 1, cx + 1] = 6

        # Soft blush cheeks
        canvas[ey + 2, cx - 4] = 8
        canvas[ey + 2, cx + 2] = 8
        # Cute gentle smile
        canvas[ey + 2, cx - 1:cx + 1] = 8
        canvas[ey + 3, cx] = 2

    elif pose == "idle":
        # Idle standing pose: y=10..45
        bob = (tick % 2) # subtle breathing
        hy = 14 + bob

        # Boots standing
        # Left boot
        canvas[41:45, cx - 6:cx - 2] = 14
        canvas[44:46, cx - 7:cx - 2] = 15
        # Right boot
        canvas[41:45, cx + 2:cx + 6] = 14
        canvas[44:46, cx + 2:cx + 7] = 15

        # Legs in stockings
        canvas[36:42, cx - 5:cx - 3] = 2
        canvas[36:42, cx + 3:cx + 5] = 2

        # Skirt / tunic bottom
        fill_ellipse(canvas, cx, 34 + bob, 8, 5, 11)
        fill_ellipse(canvas, cx, 33 + bob, 7, 4, 10)
        fill_ellipse(canvas, cx - 1, 32 + bob, 6, 3, 9)
        # Rose embroidery
        canvas[33 + bob, cx] = 12
        canvas[34 + bob, cx] = 13

        # Magenta sash
        canvas[28 + bob:30 + bob, cx - 6:cx + 6] = 13
        canvas[28 + bob, cx - 5:cx + 5] = 12
        # Clasp
        canvas[28 + bob:31 + bob, cx - 1:cx + 1] = 12
        canvas[29 + bob, cx] = 3

        # Torso
        fill_ellipse(canvas, cx, 24 + bob, 6, 5, 10)
        fill_ellipse(canvas, cx - 1, 23 + bob, 5, 4, 9)

        # Arms hanging naturally
        canvas[23 + bob:31 + bob, cx - 7:cx - 5] = 7 # left arm
        canvas[28 + bob:30 + bob, cx - 7:cx - 5] = 10 # bracer
        canvas[30 + bob:32 + bob, cx - 7:cx - 5] = 7 # hand

        canvas[23 + bob:31 + bob, cx + 5:cx + 7] = 7 # right arm
        canvas[28 + bob:30 + bob, cx + 5:cx + 7] = 10 # bracer
        canvas[30 + bob:32 + bob, cx + 5:cx + 7] = 7 # hand
        # Holding coiled whip handle in right hand
        canvas[31 + bob:34 + bob, cx + 6:cx + 8] = 14
        canvas[34 + bob:38 + bob, cx + 7] = 10 # rose stem
        canvas[37 + bob, cx + 8] = 12 # rose petal tip

        # Neck
        canvas[19 + bob:22 + bob, cx - 1:cx + 2] = 7

        # Head (hy)
        # Back hair volume flowing
        fill_circle(canvas, cx, hy - 3, 10, 5)
        fill_circle(canvas, cx - 2, hy - 4, 9, 4)
        fill_circle(canvas, cx + 3, hy - 4, 8, 4)
        # Flowing hair strands down back
        canvas[hy + 2:hy + 14, cx - 9:cx - 5] = 4
        canvas[hy + 3:hy + 12, cx - 8:cx - 6] = 3
        canvas[hy + 4:hy + 14, cx + 5:cx + 8] = 4

        # Face
        fill_circle(canvas, cx, hy, 7, 7)
        fill_circle(canvas, cx - 1, hy - 1, 6, 6)

        # Rose ornament
        fill_circle(canvas, cx - 7, hy - 5, 3, 13)
        fill_circle(canvas, cx - 7, hy - 6, 2, 12)
        canvas[hy - 5, cx - 8] = 9

        # Bangs
        canvas[hy - 6:hy - 2, cx - 5:cx + 5] = 4
        canvas[hy - 5:hy - 2, cx - 4:cx + 4] = 3
        canvas[hy - 3:hy + 4, cx - 6] = 4
        canvas[hy - 2:hy + 3, cx - 5] = 3
        canvas[hy - 3:hy + 4, cx + 6] = 4

        # Eyes
        ey = hy - 1
        canvas[ey, cx - 3] = 1
        canvas[ey - 1, cx - 4:cx - 2] = 1
        canvas[ey, cx - 4] = 9
        canvas[ey - 1, cx - 4] = 6

        canvas[ey, cx + 3] = 1
        canvas[ey - 1, cx + 2:cx + 4] = 1
        canvas[ey, cx + 2] = 9
        canvas[ey - 1, cx + 2] = 6

        # Blush
        canvas[ey + 2, cx - 4] = 8
        canvas[ey + 2, cx + 3] = 8
        # Mouth
        canvas[ey + 3, cx] = 2

    elif pose == "atk":
        # Whip lash strike forward!
        hy = 14
        # Forward lean stance
        # Back leg
        canvas[38:45, cx - 12:cx - 8] = 14
        canvas[44:46, cx - 14:cx - 8] = 15
        # Front leg lunging
        canvas[37:44, cx + 4:cx + 8] = 14
        canvas[43:46, cx + 5:cx + 11] = 15

        # Tunic angled
        fill_ellipse(canvas, cx - 2, 32, 9, 6, 10)
        fill_ellipse(canvas, cx - 3, 31, 8, 5, 9)
        canvas[27:30, cx - 7:cx + 4] = 13

        # Torso lunging forward
        fill_ellipse(canvas, cx - 1, 23, 6, 5, 10)
        fill_ellipse(canvas, cx, 22, 5, 4, 9)

        # Extended whipping right arm reaching out right
        canvas[19:23, cx + 2:cx + 10] = 7 # outstretched arm
        canvas[20:23, cx + 8:cx + 11] = 10 # bracer
        canvas[20:22, cx + 11:cx + 13] = 7 # hand
        # Whip stem crack snapping forward!
        whip_pts = [(cx + 13, 21), (cx + 16, 19), (cx + 18, 22), (cx + 20, 20)]
        for wx, wy in whip_pts:
            if 0 <= wy < 48 and 0 <= wx < 48:
                canvas[wy, wx] = 10
            if 0 <= wy - 1 < 48 and 0 <= wx + 1 < 48:
                canvas[wy - 1, wx + 1] = 12 # rose thorns
        # Blooming rose burst at whip tip
        canvas[19:23, cx + 20:cx + 23] = 13
        canvas[20:22, cx + 20:cx + 22] = 12
        canvas[20, cx + 21] = 3 # golden spark center

        # Head
        fill_circle(canvas, cx - 2, hy - 3, 9, 4)
        # Hair trailing backward dynamically
        canvas[hy:hy + 8, cx - 14:cx - 6] = 4
        canvas[hy + 1:hy + 6, cx - 13:cx - 7] = 3
        # Face
        fill_circle(canvas, cx, hy, 7, 7)
        fill_circle(canvas, cx + 1, hy - 1, 6, 6)
        # Rose ornament
        fill_circle(canvas, cx - 6, hy - 4, 3, 13)
        # Determined eye
        canvas[hy - 1, cx + 2] = 1
        canvas[hy - 2, cx + 1:cx + 4] = 1
        canvas[hy - 1, cx + 1] = 9
        canvas[hy - 2, cx + 1] = 6
        # Focus grin
        canvas[hy + 3, cx + 1:cx + 3] = 2

    canvas = draw_outline(canvas, 1)
    return canvas

def main():
    im_sit = draw_maya_48("sit")
    im_idle = draw_maya_48("idle", 0)
    im_atk = draw_maya_48("atk", 0)

    # Save PNG previews
    for name, arr in (("maya_sit", im_sit), ("maya_idle", im_idle), ("maya_atk", im_atk)):
        rgb = MAYA_PALETTE[arr]
        alpha = np.where(arr == 0, 0, 255).astype(np.uint8)
        rgba = np.dstack([rgb, alpha])
        # Scale 4x for easy viewing
        img = Image.fromarray(rgba).resize((192, 192), Image.Resampling.NEAREST)
        img.save(f"/home/kepler/.gemini/antigravity-cli/brain/33ece3e6-d497-4697-a85c-57e88ba314eb/{name}.png")
        print(f"Saved preview: {name}.png")

if __name__ == "__main__":
    main()
