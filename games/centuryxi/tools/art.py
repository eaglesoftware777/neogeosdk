#!/usr/bin/env python3
"""Century XI: Road to 2030 (Game ID 787): the art build.

Painted figurine sheets go through tools/figures.py into palette-role frames,
tools/arena.py paints the ground in perspective and writes the projection
tables, tools/nations.py writes the team and roster tables, and everything is
packed into C1/C2 and S1 by tools/ng_artpack.py.
"""
import argparse
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
PAINTED = GAME / "artbox" / "painted"
sys.path.insert(0, str(ROOT / "tools"))
from ng_artpack import ArtPack, text_pixels  # noqa: E402
sys.path.insert(0, str(GAME / "tools"))
import arena  # noqa: E402
import figures  # noqa: E402
import flags  # noqa: E402
import hud  # noqa: E402
import nations  # noqa: E402
from painted_cast import Cast, cut, logo  # noqa: E402

FIG_DIR = PAINTED / "fig"
BANK_ROLE = 32            # the figures' own bank in previews; a man's real bank is set at run time

INK = (16, 18, 28, 255)
BOOT_DARK = (38, 44, 58, 255)
SKIN_SHADOW = (168, 112, 76, 255)
SKIN_MID = (218, 162, 118, 255)
SKIN_LIGHT = (248, 206, 168, 255)
WHITE = (248, 250, 255, 255)
SILVER = (180, 192, 212, 255)
GOLD = (248, 208, 56, 255)
GOLD_DARK = (168, 108, 20, 255)
GOLD_LIGHT = (255, 242, 132, 255)
RED = (208, 36, 52, 255)
RED_DARK = (140, 18, 32, 255)
RED_LIGHT = (248, 84, 96, 255)
BLUE = (36, 96, 224, 255)
BLUE_DARK = (18, 52, 148, 255)
BLUE_LIGHT = (88, 152, 252, 255)
CYAN = (72, 228, 248, 255)
GREEN = (26, 164, 82, 255)
GREEN_DARK = (14, 104, 52, 255)
GREEN_LIGHT = (72, 216, 120, 255)
NAVY = (28, 48, 120, 255)
NAVY_DARK = (16, 28, 76, 255)
GLOVE_LIGHT = (68, 224, 116, 255)
GLOVE_DARK = (28, 148, 72, 255)


def px(img, x, y, color):
    h, w = img.shape[:2]
    if 0 <= x < w and 0 <= y < h and color[3]:
        img[y, x] = color


def rect(img, x0, y0, x1, y1, color):
    h, w = img.shape[:2]
    xa, xb = max(0, min(x0, x1)), min(w, max(x0, x1))
    ya, yb = max(0, min(y0, y1)), min(h, max(y0, y1))
    if xa < xb and ya < yb and color[3]:
        img[ya:yb, xa:xb] = color


def line(img, x0, y0, x1, y1, color):
    dx = abs(x1 - x0)
    sx = 1 if x0 < x1 else -1
    dy = -abs(y1 - y0)
    sy = 1 if y0 < y1 else -1
    err = dx + dy
    while True:
        px(img, x0, y0, color)
        if x0 == x1 and y0 == y1:
            break
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x0 += sx
        if e2 <= dx:
            err += dx
            y0 += sy


def circle_fill(img, cx, cy, r, color):
    for y in range(cy - r, cy + r + 1):
        for x in range(cx - r, cx + r + 1):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                px(img, x, y, color)


def circle_ring(img, cx, cy, r, color):
    for deg in range(360):
        rad = math.radians(deg)
        x = int(round(cx + r * math.cos(rad)))
        y = int(round(cy + r * math.sin(rad)))
        px(img, x, y, color)


def sheet(frames):
    h, w = frames[0].shape[:2]
    out = np.zeros((h, w * len(frames), 4), dtype=np.uint8)
    for i, frame in enumerate(frames):
        out[:, i * w:(i + 1) * w] = frame
    return out


def draw_text_rgba(img, x, y, text, color, shadow=INK, scale=1):
    tp = text_pixels(text, scale=scale, ink=1)
    th, tw = tp.shape
    for ty in range(th):
        for tx in range(tw):
            if tp[ty, tx]:
                if shadow is not None:
                    px(img, x + tx + 1, y + ty + 1, shadow)
                px(img, x + tx, y + ty, color)


# ============================================================================
# PAINTED ASSET LOADER & CHROMA KEYING (games/centuryxi/artbox/painted/)
# ============================================================================

def chroma_key(arr, tol=36):
    """Remove solid background color sampled from image corners if fully opaque."""
    out = arr.copy()
    if np.any(out[:, :, 3] < 128):
        out[:, :, 3] = np.where(out[:, :, 3] >= 128, 255, 0)
        out[out[:, :, 3] == 0] = 0
        return out
    h, w = out.shape[:2]
    corners = np.array([
        out[0, 0, :3], out[0, w - 1, :3],
        out[h - 1, 0, :3], out[h - 1, w - 1, :3]
    ], dtype=np.int16)
    bg = np.median(corners, axis=0)
    diff = np.max(np.abs(out[:, :, :3].astype(np.int16) - bg), axis=2)
    mask = diff <= tol
    out[mask] = (0, 0, 0, 0)
    return out


def painted_bg(name, w=320, h=224):
    p = PAINTED / f"{name}.png"
    if not p.is_file():
        return None
    im = Image.open(p).convert("RGBA")
    if im.size != (w, h):
        im = im.resize((w, h), Image.LANCZOS)
    arr = np.asarray(im, dtype=np.uint8).copy()
    arr[:, :, 3] = 255
    return arr


def painted_sprite(name, w, h, key=True):
    p = PAINTED / f"{name}.png"
    if not p.is_file():
        return None
    im = Image.open(p).convert("RGBA")
    arr = np.asarray(im, dtype=np.uint8).copy()
    if key:
        arr = chroma_key(arr)
    if (arr.shape[1], arr.shape[0]) != (w, h):
        im2 = Image.fromarray(arr).resize((w, h), Image.NEAREST)
        arr = np.asarray(im2, dtype=np.uint8).copy()
    arr[:, :, 3] = np.where(arr[:, :, 3] >= 128, 255, 0)
    arr[arr[:, :, 3] == 0] = 0
    return arr


# ============================================================================
# 1. STADIUM & TITLE BACKGROUNDS (320x224)
# Geometry matching cx_sim.h:
#   y = 0..38   : Sky, stadium roof, floodlights, roaring crowd stands
#   y = 39..47  : Stadium ad-boards ("EAGLE SOFTWARE", "CENTURY XI", "ROAD TO 2030")
#   y = 48..176 : Pitch turf, touchlines (x=24..296, y=48..176), goals (y=88..136)
#   y = 177..223: Lower stadium track & broadcast HUD scoreboard frame
# ============================================================================

def make_title_bg():
    """The stadium painting's own pixels: the sunset, the mountain and the roof."""
    p = np.asarray(arena.panel(str(PAINTED / "stadium_94.png")))
    x0, y0 = (p.shape[1] - 320) // 2, 34
    crop = p[y0:y0 + 224, x0:x0 + 320]
    return np.dstack((crop, np.full(crop.shape[:2], 255, np.uint8)))


# The match ball of 1994: white leather with the black pentagons, lit from the top left.
# Rendered as a sphere and snapped to a few hand-picked tones, so it stays crisp pixel
# art at eleven pixels and at thirty.
BALL_INK = (14, 16, 26)
BALL_TONES = {                      # lit to dark
    'white': [(255, 255, 255), (240, 243, 250), (210, 216, 230), (162, 170, 192)],
    'black': [(92, 96, 116), (48, 50, 64), (26, 28, 38), (26, 28, 38)],
}
_PHI = (1 + 5 ** 0.5) / 2
_ICO = np.array([(0, 1, _PHI), (0, -1, _PHI), (0, 1, -_PHI), (0, -1, -_PHI), (1, _PHI, 0), (-1, _PHI, 0),
                 (1, -_PHI, 0), (-1, -_PHI, 0), (_PHI, 0, 1), (-_PHI, 0, 1), (_PHI, 0, -1), (-_PHI, 0, -1)], np.float64)
_ICO /= np.linalg.norm(_ICO[0])


def _rot(axis, ang):
    axis = np.asarray(axis, np.float64) / np.linalg.norm(axis)
    x, y, z = axis
    c, s_ = np.cos(ang), np.sin(ang)
    return np.array([[c + x * x * (1 - c), x * y * (1 - c) - z * s_, x * z * (1 - c) + y * s_],
                     [y * x * (1 - c) + z * s_, c + y * y * (1 - c), y * z * (1 - c) - x * s_],
                     [z * x * (1 - c) - y * s_, z * y * (1 - c) + x * s_, c + z * z * (1 - c)]])


def ball_pixels(size, cell, spin, outline=True, patch=0.935):
    """One ball of diameter `size` px centred in a `cell` square; `spin` 0..1 turns it a
    fifth of a revolution about a pentagon's axis, which brings the pattern back round."""
    # the spin axis: a pentagon's, tipped so the ball seems to roll across the screen
    tilt = _rot((1.0, 0.25, 0.0), 0.0)
    axis = _ICO[8]
    to_view = _rot(np.cross(axis, (0.0, 1.0, 0.0)), np.arccos(np.dot(axis, (0.0, 1.0, 0.0))) - 0.35)
    spin_m = _rot(axis, spin * 2 * np.pi / 5)
    verts = (to_view @ spin_m @ _ICO.T).T @ tilt.T
    light = np.array([-0.55, -0.62, 0.56]); light /= np.linalg.norm(light)
    ss = 6                                          # supersampling per pixel
    r = size / 2.0
    c0 = cell / 2.0
    out = np.zeros((cell, cell, 4), np.uint8)
    inside = np.zeros((cell, cell), bool)
    for py in range(cell):
        for px_ in range(cell):
            votes = {}
            hit = 0
            for sy in range(ss):
                for sx in range(ss):
                    x = (px_ + (sx + 0.5) / ss - c0) / r
                    y = (py + (sy + 0.5) / ss - c0) / r
                    q = x * x + y * y
                    if q > 1.0:
                        continue
                    hit += 1
                    n = np.array([x, y, (1.0 - q) ** 0.5])
                    near = np.max(verts @ n)
                    black = near > patch                 # inside a pentagon's patch
                    lam = max(0.0, float(n @ light))
                    spec = max(0.0, float(n @ (light + np.array([0, 0, 1.0])) / np.linalg.norm(light + np.array([0, 0, 1.0])))) ** 24
                    level = 0 if spec > 0.55 else (1 if lam > 0.62 else (2 if lam > 0.30 else 3))
                    key = ('black' if black else 'white', level)
                    votes[key] = votes.get(key, 0) + 1
            if hit * 2 < ss * ss:
                continue
            inside[py, px_] = True
            (kind, level) = max(votes.items(), key=lambda kv: kv[1])[0]
            out[py, px_] = BALL_TONES[kind][level] + (255,)
    if outline:
        # a round outline: the pixels whose centres lie just outside the ball
        yy, xx = np.mgrid[0:cell, 0:cell]
        q = np.sqrt((xx + 0.5 - c0) ** 2 + (yy + 0.5 - c0) ** 2)
        ring = ~inside & (q < r + 1.05)
        out[ring] = BALL_INK + (255,)
    return out


def make_ball():
    p = painted_sprite("ball", 64, 16)
    if p is not None:
        return p
    # ten pixels across: small patches, so it reads as a white ball with a few black panels
    return sheet([ball_pixels(10, 16, k / 4.0, patch=0.95) for k in range(4)])


def make_cursor():
    frames = []
    for n in range(4):
        f = n & 1
        hi, lo = (GOLD_LIGHT, GOLD) if n < 2 else (CYAN, BLUE_LIGHT)
        im = np.zeros((16, 16, 4), dtype=np.uint8)
        dy = 1 if f == 1 else 0
        rect(im, 3, 1 + dy, 13, 7 + dy, INK)
        if n < 2:      # "1"
            line(im, 5, 2 + dy, 5, 5 + dy, WHITE)
            px(im, 4, 3 + dy, hi)
        else:          # "2": player 2's cursor names him too
            line(im, 4, 2 + dy, 6, 2 + dy, WHITE)
            px(im, 6, 3 + dy, WHITE)
            px(im, 5, 4 + dy, hi)
            line(im, 4, 5 + dy, 6, 5 + dy, WHITE)
        line(im, 8, 2 + dy, 8, 5 + dy, WHITE)
        line(im, 9, 2 + dy, 10, 2 + dy, hi)
        line(im, 9, 4 + dy, 10, 4 + dy, hi)
        px(im, 10, 3 + dy, WHITE)
        for row in range(6):
            y = 8 + dy + row
            x0, x1 = 3 + row, 12 - row
            line(im, x0 - 1, y, x1 + 1, y, INK)
            line(im, x0, y, x1, y, hi if row < 2 else lo)
        px(im, 7, 9 + dy, WHITE)
        px(im, 8, 9 + dy, WHITE)
        if f == 1:
            px(im, 2, 8, CYAN)
            px(im, 13, 8, CYAN)
        frames.append(im)
    return sheet(frames)


def make_trophy():
    p = painted_sprite("trophy", 48, 64, key=True)
    if p is not None:
        return p
    im = np.zeros((64, 48, 4), dtype=np.uint8)
    circle_fill(im, 24, 15, 11, INK)
    circle_fill(im, 24, 15, 10, GOLD)
    circle_fill(im, 22, 13, 7, GOLD_LIGHT)
    rect(im, 19, 9, 22, 12, WHITE)
    rect(im, 13, 14, 36, 18, INK)
    rect(im, 14, 15, 35, 17, GOLD_LIGHT)
    px(im, 18, 15, GREEN_LIGHT)
    px(im, 24, 15, RED_LIGHT)
    px(im, 30, 15, GREEN_LIGHT)
    # Handles
    line(im, 14, 11, 9, 14, INK)
    line(im, 9, 14, 9, 23, INK)
    line(im, 9, 23, 16, 28, INK)
    line(im, 14, 12, 10, 15, GOLD_LIGHT)
    line(im, 10, 15, 10, 22, GOLD_LIGHT)
    line(im, 10, 22, 16, 27, GOLD)

    line(im, 34, 11, 39, 14, INK)
    line(im, 39, 14, 39, 23, INK)
    line(im, 39, 23, 32, 28, INK)
    line(im, 34, 12, 38, 15, GOLD)
    line(im, 38, 15, 38, 22, GOLD)
    line(im, 38, 22, 32, 27, GOLD_DARK)
    # Stem
    for y in range(24, 46):
        hw = 9 - (y - 24) * 5 // 12 if y <= 36 else 4 + (y - 36) * 5 // 9
        rect(im, 24 - hw - 1, y, 24 + hw + 2, y + 1, INK)
        for x in range(24 - hw, 24 + hw + 1):
            dx = x - 24
            col = GOLD_LIGHT if dx < 0 else (GOLD if dx < hw // 2 else GOLD_DARK)
            px(im, x, y, col)
        px(im, 24 - hw // 2, y, WHITE)
    # Pedestal
    rect(im, 11, 46, 38, 51, INK)
    rect(im, 12, 47, 37, 50, (84, 96, 124, 255))
    rect(im, 8, 50, 41, 62, INK)
    rect(im, 9, 51, 40, 61, (48, 56, 76, 255))
    rect(im, 14, 53, 35, 59, INK)
    rect(im, 15, 54, 34, 58, GOLD)
    return im


def make_banner():
    p = painted_sprite("banner", 480, 32)
    if p is not None:
        return p
    labels = ["GOOOAL!!", "HALF TIME", "CHAMPIONS!", "FULL TIME", "PENALTIES", "REPLAY"]
    frames = []
    for text in labels:
        im = np.zeros((32, 160, 4), dtype=np.uint8)
        rect(im, 2, 2, 158, 30, INK)
        rect(im, 3, 3, 157, 29, GOLD)
        rect(im, 5, 5, 155, 27, (24, 44, 104, 255))
        rect(im, 6, 14, 154, 18, RED)
        line(im, 6, 7, 153, 7, CYAN)
        line(im, 6, 24, 153, 24, CYAN)
        tw = len(text) * 16
        sx = max(8, (160 - tw) // 2)
        draw_text_rgba(im, sx, 9, text, GOLD_LIGHT, shadow=INK, scale=2)
        frames.append(im)
    return sheet(frames)


# The figurines' frames. Sheets are 2 rows of 5 on magenta; '_' poses are not used.
FIG_SHEETS = {
    'a': ['stand_s', 'run_s1', 'run_s2', 'run_s3', 'run_s4', 'stand_f', 'run_f1', 'run_f2', 'run_f3', 'run_f4'],
    'b': ['stand_b', 'run_u1', 'run_u2', 'run_u3', 'run_u4', 'stand_d', 'run_d1', 'run_d2', 'run_d3', 'run_d4'],
    'c': ['_c1', '_c2', '_c3', '_c4', '_c5', 'kick_short', 'kick_strike', 'pass', 'lob', 'volley'],
    'd': ['slide1', 'slide2', 'poke', 'fall', 'down', 'getup', 'head_jump', 'head_hit', 'trap', 'throw1'],
    'e': ['throw2', 'celeb_run', 'celeb_jump', 'celeb_knee', 'thumbs', 'kneel', 'complain', 'disbelief', 'var_wait', 'feint1'],
    'f': ['skid', 'feint2', 'power', 'dive_head', 'dribble', 'kick_f1', 'kick_f2', 'kick_b1', 'kick_b2', 'slide_f'],
    'g': ['stand_u', 'run_b1', 'run_b2', 'run_b3', 'run_b4', 'kick_u', 'throw_b', 'celeb_b', 'walk_b', '_g10'],
    'h': ['roll1', 'roll2', 'argue', 'bike1', 'bike2', 'celeb_shirt', 'celeb_baby', 'celeb_dance', 'walk_cry', 'dizzy'],
    'i': ['hip1', 'hip2', 'belly', 'flip', 'samba1', 'samba2', 'pray', 'scream', 'sky', 'skate'],
}
# five drawn directions: side (right), down-right, down, up-right, up; the code mirrors the other three
FIG_DIRS = ['s', 'd', 'f', 'u', 'b']
FIG_MOVES = [n for d in FIG_DIRS for n in [f'stand_{d}'] + [f'run_{d}{k}' for k in range(1, 5)]]
FIG_ACTS = ['kick_short', 'kick_strike', 'pass', 'lob', 'volley', 'slide1', 'slide2', 'poke', 'fall', 'down', 'getup',
            'head_jump', 'head_hit', 'trap', 'throw1', 'throw2', 'celeb_run', 'celeb_jump', 'celeb_knee', 'thumbs', 'kneel',
            'complain', 'disbelief', 'var_wait', 'feint1', 'feint2', 'skid', 'power', 'dive_head', 'dribble', 'kick_b1', 'kick_b2', 'slide_f', 'celeb_b',
            'roll1', 'roll2', 'argue', 'bike1', 'bike2', 'celeb_shirt', 'celeb_baby', 'celeb_dance', 'walk_cry', 'dizzy',
            'hip1', 'hip2', 'belly', 'flip', 'samba1', 'samba2', 'pray', 'scream', 'sky', 'skate']
FIG_AIR = {'head_jump', 'head_hit', 'volley', 'celeb_jump', 'dive_head', 'bike1', 'bike2', 'flip'}
FIGS = ('c1', 'c2', 'c3', 'c6', 'c7')
KEEP_SHEETS = {
    'a': FIG_SHEETS['a'], 'b': FIG_SHEETS['b'], 'g': FIG_SHEETS['g'],
    'c': ['gk_ready', 'gk_crouch', 'gk_hold', 'gk_throw', 'gk_punt', 'gk_ready_f', 'gk_up1', 'gk_up2', 'gk_near1', 'gk_near2'],
    'd': ['gk_flat', 'gk_lie', 'gk_punch', 'gk_shout', 'gk_sit', 'gk_nerves', 'gk_hop', 'gk_point', 'gk_fear', 'gk_grin'],
}
KEEP_ACTS = KEEP_SHEETS['c'] + KEEP_SHEETS['d']
KEEP_AIR = {'gk_up2', 'gk_near2', 'gk_flat', 'gk_punch', 'gk_hop'}
REF_SHEETS = {
    'a': FIG_SHEETS['a'], 'b': FIG_SHEETS['b'], 'g': FIG_SHEETS['g'],
    'c': ['ref_whistle', 'ref_point', 'ref_card', 'ref_play_on', 'ref_tv', 'ref_ear', 'ref_peer', 'ref_mop', 'ref_cross', 'ref_side'],
    'd': ['ref_worry', 'ref_shrug', 'ref_tache', 'ref_jog', 'ref_laugh', 'ref_squint', 'ref_bow', 'ref_stand', 'ref_hips', 'ref_wave'],
    'e': ['ref_sandwich', 'ref_book', 'ref_flat', 'ref_blow', 'ref_dizzy', '_e6', '_e7', 'ref_ignore', 'ref_shock', '_e10'],
    'f': ['ref_cat', 'ref_flee', 'ref_jig', 'ref_lace', 'ref_rice', 'ref_watch', 'ref_yawn', 'ref_sneeze', 'ref_cry', 'ref_salute'],
}
REF_ACTS = REF_SHEETS['c'] + REF_SHEETS['d']
GAGS = ['ref_sandwich', 'ref_book', 'ref_flat', 'ref_blow', 'ref_dizzy', 'ref_ignore', 'ref_shock', 'ref_cat', 'ref_flee', 'ref_jig',
        'ref_lace', 'ref_rice', 'ref_watch', 'ref_yawn', 'ref_sneeze', 'ref_cry', 'ref_salute']


cast_lines = []


def cast_note(name, frame, anchor, names=None, prefix=None):
    up = name.upper()
    cast_lines.append(f"#define CX_{up}_W {frame[0]}")
    cast_lines.append(f"#define CX_{up}_H {frame[1]}")
    cast_lines.append(f"#define CX_{up}_AX {anchor[0]}")
    cast_lines.append(f"#define CX_{up}_AY {anchor[1]}")
    for i, n in enumerate(names or []):
        cast_lines.append(f"#define CX_F_{(prefix or '')}{n.upper()} {i}u")


def figure(who, prefix, sheets, order, air=()):
    """One character's sheets as a role sheet; a sheet not painted yet borrows from the first."""
    f = figures.Figure(who)
    for k, names in sheets.items():
        path = FIG_DIR / f"{prefix}{k}.png"
        if path.is_file():
            f.add_sheet(path, names)
    if prefix != 'c1' and any(n not in f.labs for n in order):
        ref = figure_base()
        for n in order:
            if n not in f.labs and n in ref.labs:
                f.labs[n] = ref.labs[n]
                f.scale[n] = ref.scale[n]
    f.normalise()
    return f


_base = []


def figure_base():
    if not _base:
        _base.append(figure('c1', 'c1', FIG_SHEETS, FIG_MOVES + FIG_ACTS))
    return _base[0]


def footballers(pack):
    """Three figurines, a keeper and a referee, each in every depth band on one
    canvas. All drawn in palette roles: the bank a man is shown with dresses him."""
    order = FIG_MOVES + FIG_ACTS
    metas = []
    for i, who in enumerate(FIGS):
        f = figure_base() if who == 'c1' else figure(who, who, FIG_SHEETS, order)
        sheet_, per, meta = f.sheet(order, air=FIG_AIR)
        metas.append(meta)
        pack.sprite(f"fig{i}", sheet_, frame=(figures.CANVAS_W, figures.CANVAS_H), banks=1, master=figures.KEY, bank=BANK_ROLE)
    k_order = FIG_MOVES + KEEP_ACTS
    k = figure('c4', 'k1', KEEP_SHEETS, k_order)
    sheet_, k_per, meta = k.sheet(k_order, air=KEEP_AIR)
    metas.append(meta)
    pack.sprite("keeper", sheet_, frame=(figures.CANVAS_W, figures.CANVAS_H), banks=1, master=figures.KEY, bank=BANK_ROLE)
    r_order = FIG_MOVES + REF_ACTS
    r = figure('c5', 'r1', REF_SHEETS, r_order)
    sheet_, r_per, meta = r.sheet(r_order)
    metas.append(meta)
    pack.sprite("ref", sheet_, frame=(figures.CANVAS_W, figures.CANVAS_H), banks=1, master=figures.KEY, bank=BANK_ROLE)
    # what each frame really uses: the men are written straight to video memory from this
    q = ["/* Generated by games/centuryxi/tools/art.py: do not edit. */", "#ifndef CX_FRAMES_H", "#define CX_FRAMES_H", "#include <stdint.h>"]
    tables = []
    for i, meta in enumerate(metas):
        q.append(f"static const uint8_t cx_span{i}[{len(meta)}][4] = {{" + ",".join("{%d,%d,%d,%d}" % (c0 << 4 | c1, r0 << 4 | r1, nx, ny) for c0, c1, r0, r1, nx, ny in meta) + "};")
        tables.append(f"cx_span{i}")
    q.append("static const uint8_t (*const cx_spans[])[4] = {" + ",".join(tables) + "};")
    q += ["#endif", ""]
    (GAME / "artbox" / "generated" / "cx_frames.h").write_text("\n".join(q))
    cast_lines.extend([f"#define CX_MAN_W {figures.CANVAS_W}", f"#define CX_MAN_H {figures.CANVAS_H}",
                       f"#define CX_MAN_AX {figures.ANCHOR_X}", f"#define CX_MAN_AY {figures.ANCHOR_Y}",
                       f"#define CX_FIG_FRAMES {len(order)}u", f"#define CX_KEEP_FRAMES {len(k_order)}u",
                       f"#define CX_REF_FRAMES {len(r_order)}u"])
    for i, n in enumerate(order):
        cast_lines.append(f"#define CX_F_{n.upper()} {i}u")
    for i, n in enumerate(KEEP_ACTS):
        cast_lines.append(f"#define CX_K_{n.upper()} {len(FIG_MOVES) + i}u")
    for i, n in enumerate(REF_ACTS):
        cast_lines.append(f"#define CX_R_{n.upper()} {len(FIG_MOVES) + i}u")


HERO = ['stand_f', 'celeb_jump', 'thumbs', 'kneel', 'complain', 'disbelief', 'var_wait', 'celeb_knee', 'argue', 'dizzy',
        'stand_b', 'kick_b1', 'kick_b2', 'roll1', 'walk_cry', 'celeb_dance', 'scream', 'samba1', 'samba2', 'pray', 'sky']


def heroes(pack):
    """The big figures for cut-ins and the penalty view, in the same roles."""
    for i, who in enumerate(FIGS):
        f = figure_base() if who == 'c1' else figure(who, who, FIG_SHEETS, FIG_MOVES + FIG_ACTS)
        pack.sprite(f"hero{i}", f.big_sheet(HERO, 96, 112, 98), frame=(96, 112), banks=1, master=figures.KEY, bank=BANK_ROLE)
    for i, n in enumerate(HERO):
        cast_lines.append(f"#define CX_H_{n.upper()} {i}u")
    k_order = ['gk_ready_f', 'gk_flat', 'gk_up2', 'gk_near2', 'gk_hop', 'gk_sit', 'gk_nerves', 'gk_grin']
    k = figure('c4', 'k1', KEEP_SHEETS, k_order)
    pack.sprite("herok", k.big_sheet(k_order, 96, 112, 84), frame=(96, 112), banks=1, master=figures.KEY, bank=BANK_ROLE)
    for i, n in enumerate(k_order):
        cast_lines.append(f"#define CX_HK_{n.upper()} {i}u")
    r_order = ['ref_card', 'ref_whistle', 'ref_cross', 'ref_hips', 'ref_laugh', 'ref_shrug', 'ref_ear', 'ref_side'] + GAGS
    r = figure('c5', 'r1', REF_SHEETS, r_order)
    pack.sprite("heror", r.big_sheet(r_order, 96, 112, 98), frame=(96, 112), banks=1, master=figures.KEY, bank=BANK_ROLE)
    for i, n in enumerate(r_order):
        cast_lines.append(f"#define CX_HR_{n.upper()} {i}u")


def plain_cast(pack, asset, path, names, order, height, prefix):
    """A character in his own colours: the coach, the linesman."""
    c = Cast(height=height, ref=order[0], colours=36)
    c.add(cut(path, 2, names, min_area=2500, merge_gap=10))
    sheet_, frame, anchor, got = c.sheet(order, cols=len(order))
    pack.sprite(asset, sheet_, frame=frame, banks=2, refine=1)
    up = asset.upper()
    cast_lines.extend([f"#define CX_{up}_W {frame[0]}", f"#define CX_{up}_H {frame[1]}", f"#define CX_{up}_AX {anchor[0]}", f"#define CX_{up}_AY {anchor[1]}"])
    for i, n in enumerate(order):
        cast_lines.append(f"#define CX_{prefix}_{n.upper()} {i}u")


def make_digits():
    """Shirt numbers 1 to 23 in a small square font, in white and in dark, as palette roles."""
    font = {'0': "111 101 101 101 111", '1': "010 110 010 010 111", '2': "111 001 111 100 111", '3': "111 001 011 001 111", '4': "101 101 111 001 001",
            '5': "111 100 111 001 111", '6': "111 100 111 101 111", '7': "111 001 010 010 010", '8': "111 101 111 101 111", '9': "111 101 111 001 111"}
    idx = np.zeros((16, 16 * 46), np.uint8)
    for ink, base in ((figures.WHITE, 0), (figures.OUTLINE, 23)):
        for n in range(1, 24):
            text = str(n)
            w = len(text) * 4 - 1
            x0 = (base + n - 1) * 16 + (16 - w) // 2
            for k, ch in enumerate(text):
                for r, row in enumerate(font[ch].split()):
                    for c, on in enumerate(row):
                        if on == '1':
                            idx[5 + r, x0 + k * 4 + c] = ink
    return figures.rgba_of(idx)


def make_goal_word():
    """One of the painted GOAL! words, cut from its pink ground."""
    src = np.asarray(Image.open(PAINTED / "goal_word.png").convert('RGB')).astype(np.int32)
    h = src.shape[0]
    band = src[:h // 3]
    r, g, b = band[..., 0], band[..., 1], band[..., 2]
    pink = (r > 170) & (g < 70) & (b > 70) & (b < 200)
    ys, xs = np.nonzero(~pink)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    rgb = band[y0:y1, x0:x1].astype(np.uint8)
    a = (~pink[y0:y1, x0:x1]).astype(np.uint8) * 255
    im = Image.fromarray(np.dstack((rgb, a)), 'RGBA')
    w, hh = im.size
    s = min(254 / w, 62 / hh)
    im = im.resize((int(w * s), int(hh * s)), Image.BOX)
    out = np.zeros((64, 256, 4), np.uint8)
    arr = np.asarray(im).copy()
    arr[..., 3] = np.where(arr[..., 3] > 150, 255, 0)
    oy, ox = (64 - arr.shape[0]) // 2, (256 - arr.shape[1]) // 2
    out[oy:oy + arr.shape[0], ox:ox + arr.shape[1]] = arr
    return out


def make_ballz():
    """The ball at five sizes, for a shot flying away from the camera."""
    sizes = (28, 21, 15, 10, 6)
    return np.concatenate([ball_pixels(d, 32, 0.15 * k) for k, d in enumerate(sizes)], 1)


def make_cards():
    """A yellow and a red card, large."""
    out = np.zeros((48, 64, 4), np.uint8)
    for k, c in enumerate(((248, 216, 32, 255), (224, 32, 40, 255))):
        rect(out, k * 32 + 6, 3, k * 32 + 25, 44, (20, 20, 28, 255))
        rect(out, k * 32 + 8, 5, k * 32 + 23, 42, c)
    return out


def make_inset():
    """The little television window the coach appears in."""
    im = np.zeros((96, 80, 4), dtype=np.uint8)
    rect(im, 0, 0, 79, 95, INK)
    rect(im, 2, 2, 77, 93, (34, 60, 120, 255))
    rect(im, 2, 2, 77, 3, GOLD)
    rect(im, 2, 92, 77, 93, GOLD_DARK)
    rect(im, 6, 8, 73, 88, (70, 126, 176, 255))
    rect(im, 6, 64, 73, 88, (70, 150, 70, 255))
    return im


def make_pen_view():
    im = Image.open(PAINTED / "penalty_view.png").convert('RGB')
    w, h = im.size
    s = max(320 / w, 224 / h)
    im = im.resize((int(round(w * s)), int(round(h * s))), Image.BOX)
    x0, y0 = (im.size[0] - 320) // 2, (im.size[1] - 224) // 2
    a = np.asarray(im.crop((x0, y0, x0 + 320, y0 + 224)))
    return np.dstack((a, np.full(a.shape[:2], 255, np.uint8)))


def make_var_room():
    p = PAINTED / "var_room.png"
    if not p.is_file():
        return np.dstack((np.full((224, 320, 3), 24, np.uint8), np.full((224, 320), 255, np.uint8)))
    im = Image.open(p).convert('RGB')
    w, h = im.size
    s = max(320 / w, 224 / h)
    im = im.resize((int(round(w * s)), int(round(h * s))), Image.BOX)
    x0, y0 = (im.size[0] - 320) // 2, (im.size[1] - 224) // 2
    a = np.asarray(im.crop((x0, y0, x0 + 320, y0 + 224)))
    return np.dstack((a, np.full(a.shape[:2], 255, np.uint8)))


# ============================================================================
# THE GROUND: tools/arena.py paints it; the tables it writes drive the match.
# ============================================================================
# the ad boards: made-up brands only (no real company, no real product)
BOARDS = ["EAGLE SOFTWARE", "BIG RICE COLA", "CENTURY XI", "GOAL GUM", "TOKYO SOCKS", "NINJA NOODLE", "ROAD TO 2030", "NEKO CAMERA",
          "TOFU TURBO", "SUSHI ROCKET", "GOAL TV"]


def board_text(img, x, y, text, col):
    pic = np.zeros((10, 8 * len(text) + 2, 4), np.uint8)
    draw_text_rgba(pic, 0, 1, text, tuple(col) + (255,), shadow=(0, 0, 0, 0))
    region = img[y:y + 10, x:x + pic.shape[1]]
    m = pic[:region.shape[0], :region.shape[1], 3] > 0
    region[m] = pic[:region.shape[0], :region.shape[1], :3][m]


STADIUMS = ["stadium_94", "st_yoko", "st_osaka", "st_kyoto", "st_sapporo", "st_kobe", "st_nagoya", "st_fukuoka"]


def make_ground(name):
    top, bottom = arena.make_ground(str(PAINTED / f"{name}.png"), board_text, BOARDS)
    return tuple(np.dstack((a, np.full(a.shape[:2], 255, np.uint8))) for a in (top, bottom))


def make_plate():
    """The scoreboard: a slim bar behind the FIX text, inside the safe area."""
    im = np.zeros((32, 176, 4), dtype=np.uint8)
    rect(im, 0, 4, 175, 27, INK)
    rect(im, 1, 5, 174, 26, (20, 26, 58, 255))
    rect(im, 1, 5, 174, 6, GOLD)
    rect(im, 1, 25, 174, 26, GOLD_DARK)
    rect(im, 60, 7, 115, 24, (8, 10, 24, 255))
    return im


def make_shadow():
    """The ball's shadow on the grass: a dark oval, 10 x 4."""
    im = np.zeros((16, 16, 4), np.uint8)
    for y, (x0, x1) in zip(range(6, 10), ((5, 10), (3, 12), (3, 12), (5, 10))):
        im[y, x0:x1 + 1] = (18, 58, 26, 255)
    return im


CROWD_MAX = 32


KEYS = arena.KEY_HOME + arena.KEY_AWAY
PITCH_BANK = 200          # 200-231: past every other asset's banks (they end below 90)
STAGE_BANK = 96           # 96-127: the title, the stands, the VAR room and the penalty view (one on screen at a time)
STAGE_BANKS = 32          # twice the twelve to sixteen they had: the hardware has 256 and used under a hundred
KEY_MAX = 96


def crowd_keys(asset, bank0):
    """Where the supporters' key colours ended up: (bank, entry, key 0-5) for every palette
    entry that is one of them (keys 0-2 the home side's shirt and flag, 3-5 the away's)."""
    out = []
    for b, pal in enumerate(asset["palettes"]):
        for k in range(1, len(pal)):
            c = tuple(int(v) for v in pal[k][:3])
            d = [sum(abs(a - x) for a, x in zip(c, key)) for key in KEYS]
            j = int(np.argmin(d))
            if d[j] < 60:
                out.append((bank0 + b, k, j))
    return out


def crowd_swaps(asset):
    """The terraces come alive by palette: the colours the crowd alone wears (in the rows
    of the stand just above the boards, hardly anywhere else) are paired with others of
    like brightness, and the game swaps a pair every few frames: shirts and scarves
    change colour here and there, as a crowd moves, while seats and roof stay still."""
    idx = np.asarray(asset["indices"])
    assign = np.asarray(asset["assignment"])
    pals = asset["palettes"]
    h, w = idx.shape
    y1 = arena.PY0 - 27
    crowd_rows = np.zeros(h, bool)
    crowd_rows[max(0, y1 - 80):max(0, y1 - 4)] = True
    # each pixel's bank from its 16 x 16 tile
    th, tw = assign.shape
    bank = np.repeat(np.repeat(assign, 16, 0), 16, 1)[:h, :w]
    pairs = []
    for b in range(len(pals)):
        mb = bank == b
        cand = []
        for k in range(1, len(pals[b])):
            m = mb & (idx == k)
            total = int(m.sum())
            if total < 24:
                continue
            inside = int(m[crowd_rows].sum())
            r, g, bl = (int(c) for c in pals[b][k][:3])
            if min(sum(abs(a - x) for a, x in zip((r, g, bl), key)) for key in KEYS) < 60:
                continue                          # a side's colour: the kits write it
            lum = (r * 3 + g * 6 + bl) // 10
            sat = max(r, g, bl) - min(r, g, bl)
            # mostly the crowd's, and not the black between the seats
            if inside * 4 >= total * 3 and lum > 30 and (sat > 20 or lum > 90):
                cand.append((lum, k))
        cand.sort()
        for j in range(0, len(cand) - 1, 2):
            if abs(cand[j][0] - cand[j + 1][0]) < 45:
                pairs.append((b, cand[j][1], cand[j + 1][1]))
    return pairs[:CROWD_MAX]


def crowd_header(per_ground, keys_per_ground, pitch_keys):
    rows, counts = [], []
    for pairs in per_ground:
        cells = [f"{{{b},{x},{y}}}" for b, x, y in pairs] + ["{0,0,0}"] * (CROWD_MAX - len(pairs))
        rows.append("{" + ",".join(cells) + "}")
        counts.append(str(len(pairs)))
    return "\n".join(["/* Generated by games/centuryxi/tools/art.py: do not edit. */",
                      "#ifndef CX_CROWD_H", "#define CX_CROWD_H", "#include <stdint.h>",
                      f"#define CX_CROWD_MAX {CROWD_MAX}",
                      "/* per ground: (stand palette 0..31 from the stands' first bank, entry, entry) pairs the game swaps to move the crowd */",
                      f"static const uint8_t cx_crowd_pair[8][CX_CROWD_MAX][3] = {{{','.join(rows)}}};",
                      f"static const uint8_t cx_crowd_n[8] = {{{','.join(counts)}}};",
                      f"#define CX_KEY_MAX {KEY_MAX}",
                      "/* the supporters' colours: (palette bank, entry, key) per ground's stands, then the near stand",
                      " * (the shared pitch). Keys 0-2: home shirt, flag, flag's second colour; 3-5 the away side's */",
                      "static const uint8_t cx_crowd_key[9][CX_KEY_MAX][3] = {" + ",".join(
                          "{" + ",".join([f"{{{b},{e},{k}}}" for b, e, k in ks[:KEY_MAX]] + ["{0,0,0}"] * (KEY_MAX - len(ks[:KEY_MAX]))) + "}"
                          for ks in keys_per_ground + [pitch_keys]) + "};",
                      "static const uint8_t cx_crowd_key_n[9] = {" + ",".join(str(min(KEY_MAX, len(ks))) for ks in keys_per_ground + [pitch_keys]) + "};",
                      "#endif", ""])


def build(fix_only):
    (GAME / 'artbox' / 'generated').mkdir(parents=True, exist_ok=True)
    (GAME / 'artbox' / 'generated' / 'cx_data.h').write_text(nations.header())
    (GAME / 'artbox' / 'generated' / 'cx_proj.h').write_text(arena.proj_header())
    pack = ArtPack("cx", 787, "CENTURY XI", first_bank=16, text_edge=2)
    # the match HUD lives on the FIX layer: its tiles go into S1 with the font
    hud_h, _ = hud.build(pack, [nations.shown(n)[1] for n in nations.NATIONS])
    (GAME / 'artbox' / 'generated' / 'cx_hud.h').write_text(hud_h)
    if fix_only:
        pack.write_fix(GAME)
        return

    # 1. The title painting, the stands (16..27) and the pitch (28..31) share banks: never on screen together
    pack.background("bg_title", make_title_bg(), banks=STAGE_BANKS, bank=STAGE_BANK, refine=1)
    # eight grounds, one pitch: each stadium is its own stands ring over the shared grass
    crowd, keys = [], []
    for i, name in enumerate(STADIUMS):
        stands, pitch = make_ground(name)
        pack.background(f"stands{i}", stands, banks=STAGE_BANKS, bank=STAGE_BANK, refine=1)
        crowd.append(crowd_swaps(pack.assets[-1]))
        keys.append(crowd_keys(pack.assets[-1], STAGE_BANK))
    # the pitch and the near stand: sixteen palettes of their own above everything else (four,
    # shared under the stands' range, made the grass a few flat greens)
    pack.background("pitch", pitch, banks=32, bank=PITCH_BANK, refine=2)
    pitch_keys = crowd_keys(pack.assets[-1], PITCH_BANK)
    (GAME / 'artbox' / 'generated' / 'cx_crowd.h').write_text(crowd_header(crowd, keys, pitch_keys))
    print("supporters' colour entries: stands", [len(k) for k in keys], "near stand", len(pitch_keys))
    pack.background("var_room", make_var_room(), banks=STAGE_BANKS, bank=STAGE_BANK, refine=1)
    pack.background("pen_view", make_pen_view(), banks=STAGE_BANKS, bank=STAGE_BANK, refine=1)
    front, origin = arena.make_goal_front(0)
    bulge, _ = arena.make_goal_front(0, True)
    cast_lines.extend([f"#define CX_GOAL_X {origin[0]}", f"#define CX_GOAL_Y {origin[1]}",
                       f"#define CX_GOAL_W {front.shape[1]}", f"#define CX_GOAL_H {front.shape[0]}"])

    # 2. Sprites: banks 32-55 are the men's, written at kick-off; the figures carry
    #    roles, not colours. Everything else follows from bank 56.
    pack.next_bank = 56
    footballers(pack)
    heroes(pack)
    pack.sprite("goal", np.concatenate((front, bulge), 1), frame=(front.shape[1], front.shape[0]), banks=1)
    plain_cast(pack, "coach", PAINTED / "fig" / "coach1.png",
               ['co_fold', 'co_shout', 'co_despair', 'co_jump', 'co_run', 'co_kick', 'co_beg', 'co_tear', 'co_chew', 'co_argue'],
               ['co_fold', 'co_shout', 'co_despair', 'co_jump', 'co_kick', 'co_beg', 'co_tear', 'co_chew', 'co_argue'], 66, 'CO')
    plain_cast(pack, "lines", PAINTED / "fig" / "lines1.png",
               ['ln_stand', 'ln_run1', 'ln_run2', 'ln_run3', 'ln_run4', 'ln_flag', 'ln_panic', 'ln_point', 'ln_shrug', 'ln_cower'],
               ['ln_stand', 'ln_run1', 'ln_run2', 'ln_run3', 'ln_run4', 'ln_flag', 'ln_panic'], 50, 'LN')
    pack.sprite("goalword", make_goal_word(), frame=(256, 64), banks=4, refine=1)
    pack.sprite("digits", make_digits(), frame=(16, 16), banks=1, master=figures.KEY, bank=BANK_ROLE)
    pack.sprite("ballz", make_ballz(), frame=(32, 32), banks=1)
    pack.sprite("cards", make_cards(), frame=(32, 48), banks=1)
    pack.sprite("inset", make_inset(), frame=(80, 96), banks=1)
    pack.sprite("flags", flags.sheet(), frame=(32, 32), banks=8)
    pack.sprite("shadow", make_shadow(), frame=(16, 16), banks=1)
    pack.sprite("plate", make_plate(), frame=(176, 32), banks=1)
    pack.sprite("ball", make_ball(), frame=(16, 16), banks=1)
    pack.sprite("cursor", make_cursor(), frame=(16, 16), banks=1)
    pack.sprite("trophy", make_trophy(), frame=(48, 64), banks=2)
    pack.sprite("banner", make_banner(), frame=(160, 32), banks=2)
    pack.sprite("logo", logo(PAINTED / "logo.png", 256, 96), frame=(256, 96), banks=4, refine=1)

    pack.write(GAME)
    out = GAME / "artbox" / "generated"
    ids = [l for l in (out / "cx_assets.h").read_text().splitlines() if l.startswith("#define CX_ART_") or l.startswith("#define CX_FIRST_FREE")]
    (out / "cx_ids.h").write_text("/* Generated by games/centuryxi/tools/art.py: do not edit. */\n#ifndef CX_IDS_H\n#define CX_IDS_H\n" +
                                  "\n".join(ids) + "\n#endif\n")
    (out / "cx_cast.h").write_text("/* Generated by games/centuryxi/tools/art.py: do not edit. */\n#ifndef CX_CAST_H\n"
                                   "#define CX_CAST_H\n" + "\n".join(cast_lines) + "\n#endif\n")
    print(f"art: Century XI packed, sprite banks 32..{pack.next_bank - 1}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--fix-only", action="store_true")
    args = ap.parse_args()
    build(args.fix_only)


if __name__ == "__main__":
    main()
