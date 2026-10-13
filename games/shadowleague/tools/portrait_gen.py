"""Procedural pixel-art face portrait generator for Shadow League fighters.
Each portrait is 32x32 pixels, rendered in the fighter's native 16-color palette.
Designed for the arcade character select screen grid.
"""

import numpy as np
from PIL import Image

def draw_frame_box(img, x0=0, y0=0, w=32, h=32):
    """Draw a bevelled metallic arcade border box."""
    # Outer outline
    img[y0, x0:x0+w] = 1
    img[y0+h-1, x0:x0+w] = 1
    img[y0:y0+h, x0] = 1
    img[y0:y0+h, x0+w-1] = 1
    # Bevel highlight (top and left)
    img[y0+1, x0+1:x0+w-1] = 14
    img[y0+1:y0+h-1, x0+1] = 14
    # Bevel shadow (bottom and right)
    img[y0+h-2, x0+1:x0+w-1] = 1
    img[y0+1:y0+h-1, x0+w-2] = 1

def generate_cursor_frame():
    """Generate 32x32 transparent bracket frame cursor sprite."""
    f = np.zeros((32, 32), dtype=np.uint8)
    # Top-left corner bracket
    f[0:3, 0:9] = 15
    f[0:9, 0:3] = 15
    f[1, 1:8] = 14
    f[1:8, 1] = 14
    # Top-right corner bracket
    f[0:3, 23:32] = 15
    f[0:9, 29:32] = 15
    f[1, 24:31] = 14
    f[1:8, 30] = 14
    # Bottom-left corner bracket
    f[29:32, 0:9] = 15
    f[23:32, 0:3] = 15
    f[30, 1:8] = 14
    f[24:31, 1] = 14
    # Bottom-right corner bracket
    f[29:32, 23:32] = 15
    f[23:32, 29:32] = 15
    f[30, 24:31] = 14
    f[24:31, 30] = 14
    return f

def draw_features(p, skin_tone=3, brow_col=1, hair_mat=12, hair_hi=13,
                  hair_style="short", clothes_mat=6, trim_mat=7,
                  extra="none"):
    # Neck
    p[22:30, 11:21] = skin_tone
    p[22:30, 10] = 1
    p[22:30, 21] = 1
    # Shoulders
    p[24:30, 2:11] = clothes_mat
    p[24:30, 21:30] = clothes_mat
    p[24:30, 2] = 1
    p[24:30, 29] = 1
    if trim_mat:
        p[24, 2:11] = trim_mat
        p[24, 21:30] = trim_mat

    # Head shape
    w_jaw = 7 if extra in ("heavy", "robot") else (5 if extra == "lean" else 6)
    for y in range(12, 23):
        w = w_jaw if y < 18 else (w_jaw - 1 if y < 21 else w_jaw - 2)
        p[y, 16-w:16+w+1] = skin_tone
        p[y, 16-w-1] = 1
        p[y, 16+w+1] = 1
    p[23, 16-(w_jaw-2):16+(w_jaw-2)+1] = skin_tone - 1 if skin_tone > 2 else skin_tone
    p[24, 16-(w_jaw-2):16+(w_jaw-2)+1] = 1

    # Hair
    if hair_style == "bald":
        p[5:11, 10:22] = skin_tone
        p[4, 11:21] = 1
    elif hair_style == "spiky":
        p[4:11, 8:24] = hair_mat
        p[3:5, 11:15] = hair_hi
        p[2:5, 17:21] = hair_hi
        p[3, 10:15] = 1
        p[2, 16:21] = 1
    elif hair_style == "topknot":
        p[5:12, 9:23] = hair_mat
        p[2:6, 14:18] = hair_hi
        p[1:3, 15:17] = hair_mat
        p[1, 14:18] = 1
    elif hair_style == "braids":
        p[5:12, 8:24] = hair_mat
        p[12:20, 6:9] = hair_hi
        p[12:20, 23:26] = hair_hi
    elif hair_style == "swept":
        p[4:12, 8:24] = hair_mat
        p[3:6, 10:20] = hair_hi
        p[5:11, 7:10] = hair_hi
    elif hair_style == "visor":
        p[6:12, 7:25] = clothes_mat
        p[12:16, 8:24] = 15  # glowing visor
        p[13:15, 10:22] = 14 # bright core
        p[12, 8:24] = 1
        p[16, 8:24] = 1
    else:  # short / buzz
        p[5:11, 9:23] = hair_mat
        p[5:8, 11:21] = hair_hi
        p[4, 10:22] = 1

    # Eyes & Brow (if not full visor)
    if hair_style != "visor":
        p[14, 11:15] = brow_col
        p[14, 17:21] = brow_col
        p[16, 12:15] = 14
        p[16, 13] = 1
        p[16, 17:20] = 14
        p[16, 18] = 1
        p[15, 11:15] = 1
        p[15, 17:21] = 1
        # Nose & mouth
        p[17:19, 15] = skin_tone + 1 if skin_tone < 4 else skin_tone
        p[19, 15:17] = 1
        p[21, 14:18] = 1
        p[22, 15:17] = skin_tone - 1 if skin_tone > 2 else skin_tone

    # Extra features
    if extra == "headband":
        p[10:13, 8:24] = 9
        p[10, 8:24] = 10
        p[12, 8:24] = 8
        p[9, 8:24] = 1
        p[13, 8:24] = 1
    elif extra == "mongkhon":
        p[8:10, 7:25] = 14
        p[7, 7:25] = 1
        p[10, 7:25] = 1
    elif extra == "beard":
        p[20:24, 12:20] = hair_mat
        p[22:25, 13:19] = hair_hi
        p[25, 14:18] = 1
    elif extra == "mask":
        p[12:22, 9:23] = clothes_mat
        p[15:18, 11:15] = 14
        p[15:18, 17:21] = 14
        p[16, 13] = 1
        p[16, 19] = 1

def generate_portrait(fighter_id):
    """Generate 32x32 indexed image (0..15) for fighter."""
    p = np.zeros((32, 32), dtype=np.uint8)
    p[2:30, 2:30] = 11

    # Individual customization per fighter
    specs = {
        # Tournament 32
        "kenji": {"skin_tone": 3, "hair_style": "spiky", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 7, "extra": "headband"},
        "somchai": {"skin_tone": 3, "hair_style": "buzz", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "mongkhon"},
        "amara": {"skin_tone": 3, "hair_style": "braids", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 9, "trim_mat": 12, "extra": "none"},
        "jack": {"skin_tone": 3, "hair_style": "short", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 14, "extra": "heavy"},
        "claire": {"skin_tone": 4, "hair_style": "swept", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "lean"},
        "viktor": {"skin_tone": 3, "hair_style": "short", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "heavy"},
        "linwei": {"skin_tone": 3, "hair_style": "spiky", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 9, "trim_mat": 12, "extra": "none"},
        "batu": {"skin_tone": 3, "hair_style": "topknot", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 9, "trim_mat": 12, "extra": "heavy"},
        "minjun": {"skin_tone": 3, "hair_style": "swept", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 12, "extra": "lean"},
        "malik": {"skin_tone": 2, "hair_style": "braids", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "heavy"},
        "idris": {"skin_tone": 3, "hair_style": "spiky", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 9, "trim_mat": 12, "extra": "none"},
        "arun": {"skin_tone": 3, "hair_style": "topknot", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "lean"},
        "ryuhei": {"skin_tone": 3, "hair_style": "short", "hair_mat": 12, "hair_hi": 14, "clothes_mat": 6, "trim_mat": 12, "extra": "beard"},
        "diego": {"skin_tone": 3, "hair_style": "visor", "hair_mat": 6, "hair_hi": 7, "clothes_mat": 6, "trim_mat": 9, "extra": "mask"},
        "niran": {"skin_tone": 3, "hair_style": "buzz", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "mongkhon"},
        "chenbao": {"skin_tone": 3, "hair_style": "short", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 9, "trim_mat": 12, "extra": "beard"},
        "aisha": {"skin_tone": 2, "hair_style": "braids", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 9, "trim_mat": 6, "extra": "none"},
        "kiran": {"skin_tone": 3, "hair_style": "swept", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "none"},
        "elena": {"skin_tone": 4, "hair_style": "topknot", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "lean"},
        "fahri": {"skin_tone": 3, "hair_style": "short", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "heavy"},
        "maya": {"skin_tone": 3, "hair_style": "topknot", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 9, "trim_mat": 12, "extra": "none"},
        "aditya": {"skin_tone": 3, "hair_style": "topknot", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "none"},
        "sokha": {"skin_tone": 3, "hair_style": "spiky", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "none"},
        "lucas": {"skin_tone": 3, "hair_style": "short", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 12, "extra": "none"},
        "noah": {"skin_tone": 3, "hair_style": "short", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 12, "extra": "heavy"},
        "zara": {"skin_tone": 3, "hair_style": "braids", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "none"},
        "thabo": {"skin_tone": 2, "hair_style": "buzz", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "lean"},
        "maia": {"skin_tone": 4, "hair_style": "short", "hair_mat": 12, "hair_hi": 14, "clothes_mat": 6, "trim_mat": 9, "extra": "lean"},
        "tomas": {"skin_tone": 3, "hair_style": "spiky", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "none"},
        "leila": {"skin_tone": 3, "hair_style": "topknot", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "lean"},
        "kaimana": {"skin_tone": 3, "hair_style": "short", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "heavy"},
        "aleksei": {"skin_tone": 3, "hair_style": "short", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 9, "trim_mat": 6, "extra": "heavy"},
        # Bonus 7
        "hayato": {"skin_tone": 3, "hair_style": "topknot", "hair_mat": 14, "hair_hi": 14, "clothes_mat": 6, "trim_mat": 12, "extra": "beard"},
        "mason": {"skin_tone": 3, "hair_style": "spiky", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 14, "extra": "heavy"},
        "lotus": {"skin_tone": 3, "hair_style": "topknot", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "none"},
        "tororojo": {"skin_tone": 3, "hair_style": "visor", "hair_mat": 6, "hair_hi": 7, "clothes_mat": 6, "trim_mat": 9, "extra": "mask"},
        "irina": {"skin_tone": 4, "hair_style": "topknot", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "lean"},
        "kofi": {"skin_tone": 2, "hair_style": "short", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "heavy"},
        "xiaomei": {"skin_tone": 4, "hair_style": "topknot", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 9, "trim_mat": 12, "extra": "lean"},
        # Hidden 2
        "zero": {"skin_tone": 3, "hair_style": "spiky", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 9, "extra": "headband"},
        "echo": {"skin_tone": 6, "hair_style": "visor", "hair_mat": 6, "hair_hi": 7, "clothes_mat": 6, "trim_mat": 14, "extra": "robot"},
        # Droids 3
        "atlas": {"skin_tone": 6, "hair_style": "visor", "hair_mat": 6, "hair_hi": 7, "clothes_mat": 6, "trim_mat": 14, "extra": "robot"},
        "viper": {"skin_tone": 6, "hair_style": "visor", "hair_mat": 6, "hair_hi": 7, "clothes_mat": 9, "trim_mat": 14, "extra": "robot"},
        "mirage": {"skin_tone": 6, "hair_style": "visor", "hair_mat": 6, "hair_hi": 7, "clothes_mat": 6, "trim_mat": 12, "extra": "robot"},
    }

    sp = specs.get(fighter_id, {"skin_tone": 3, "hair_style": "short", "hair_mat": 12, "hair_hi": 13, "clothes_mat": 6, "trim_mat": 7, "extra": "none"})
    draw_features(p, **sp)
    draw_frame_box(p)
    return p
