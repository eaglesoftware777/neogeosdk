#!/usr/bin/env python3
"""
No Signal's attack boxes against the pictures that show them.

    python3 games/nosignal/tools/boxes.py            the table
    python3 games/nosignal/tools/boxes.py --check    exit 1 if a box and its picture disagree

For every move in scenes/ns_data.c it finds the picture the fighter shows
while the move can hit (ns_draw.c: a hero shows the move's `anim`, a street
enemy its ATTACK frame, a boss the move's `anim`), and in that picture the
frontmost drawn pixel at the heights the box covers (box_z to box_z + box_h
above the feet). A fighter's feet are at the middle of its frame, 4 pixels
above the bottom (ns_draw.c fighter_position); every sheet faces right.

A box should end where the fist, foot or weapon does: no more than SHORT
pixels before it (a blow that visibly lands and misses) and no more than
LONG past it (one that lands from out of reach). Moves that also reach
behind (box_x < 0: specials, slams) and the bosses' (shockwaves that aren't
drawn) are only held to the first. Across the belt, a box's half depth plus
a typical target's (the street crew's deepest for a hero, the deepest hero
for the crew) must stay within DEPTH pixels, so fighters a lane apart don't
hit each other (bosses' area blows and the heroes' specials are left out). Moves that throw a shot
(box_w 0), grab or do no damage are left out.

--suggest prints, for the ones that disagree, a box from the picture: the
striking limb is what is drawn within LIMB pixels of the front, and the box
covers its heights (with MARGIN above and below) up to the front.
"""
import argparse
from pathlib import Path
import re
import sys

import numpy as np
from PIL import Image

GAME = Path(__file__).resolve().parents[1]
SHORT, LONG, DEPTH = 4, 4, 16
LIMB, MARGIN = 12, 4

HF = ("STANCE IDLE WALK1 WALK2 WALK3 WALK4 RUN1 RUN2 JUMP_UP JUMP_DOWN KEY2 JAB CROSS UPPER KICK JUMPKICK "
      "SWEEP DASH GRAB THROW KEY3 HURT HURT_LOW KNOCKED DOWN GETUP SPECIAL1 SPECIAL2 WIN RIDE "
      "KEY4 PUNCH_WIND PUNCH_BACK UPPER_WIND UPPER_BACK KICK_WIND KICK_BACK SWEEP_WIND LAND RUN3").split()
EF = "STANCE WALK1 WALK2 WALK3 WINDUP ATTACK HURT KNOCKED DOWN GETUP".split()
HEROES = {"JAY": "jay", "ZOE": "zoe", "KAI": "kai"}
CREW = {"THIEF": "thief", "BIKER": "biker", "SKATER": "skater", "RICO": "rico", "GUARD": "guard", "DRONE": "drone",
        "RUNNER": "runner", "BLOATER": "bloater", "ANDROID": "android", "ROBODOG": "robodog", "WALKER": "walker",
        "ALIEN": "alien"}
BOSSES = {"BIGFLEX": "bigflex", "ZEROX": "zerox", "M08": "m08", "BEHEMOTH": "behemoth", "PROXY": "proxy",
          "OVERMIND": "overmind", "ARCHIVIST": "archivist"}
# fighters' half depths (ns_data.c NGBFighterDef: the 7th number)
DEF_RE = re.compile(r"const NGBFighterDef ns_(\w+) = \{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)")
HERO_DEF_RE = re.compile(r"\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(JAY|ZOE|KAI)_MOVES")


def frame_index(anim, crew):
    """The picture shown while the move can hit, or None."""
    if anim.startswith("HF_"):
        return HF.index(anim[3:])
    if anim.startswith("EF_"):
        return EF.index("ATTACK") if crew else EF.index(anim[3:])
    m = re.match(r"NS_SHEET2\((\d+)\)", anim)
    if m:
        return 10 + int(m.group(1))
    m = re.match(r"NS_SHEET3\((\d+)\)", anim)
    if m:
        return 20 + int(m.group(1))
    if anim.startswith("BF_"):
        return 10 + "STANCE2 THROW_PHONE TAUNT BIKE1 BIKE2 BIKE_FALL DIZZY LAUGH LUNGE DEFEAT".split().index(anim[3:])
    if anim.isdigit():
        return int(anim)
    return None


def moves(src):
    """(table, comment, fields) for every move row of every *_MOVES table."""
    out = []
    for name, body in re.findall(r"static const NGBMove (\w+)_MOVES\[\] = \{(.*?)\n\};", src, re.S):
        comment = ""
        for line in body.splitlines():
            c = re.search(r"/\*\s*\d*\s*([^*]*?)\s*\*/", line)
            if c and c.group(1):
                comment = c.group(1)
            m = re.search(r"\{([^{}]*)\}", line)
            if not m:
                continue
            f = [x.strip() for x in m.group(1).split(",")]
            if len(f) >= 18:
                out.append((name, comment, f))
                comment = ""
    return out


def limb(sheet, idx):
    """The picture's front and the heights of what is drawn within LIMB of it."""
    im = np.asarray(Image.open(GAME / f"artbox/generated/f_{sheet}.png").convert("RGBA"))
    h = im.shape[0]
    alpha = im[:, idx * h:(idx + 1) * h, 3] > 0
    foot, ground = h // 2, h - 4
    ys, xs = np.nonzero(alpha)
    front = int(xs.max())
    near = ys[xs >= front - LIMB]
    return front - foot, ground - int(near.max()), ground - int(near.min())


def measure(sheet, idx, z0, z1):
    im = np.asarray(Image.open(GAME / f"artbox/generated/f_{sheet}.png").convert("RGBA"))
    h = im.shape[0]
    if (idx + 1) * h > im.shape[1]:
        return None
    alpha = im[:, idx * h:(idx + 1) * h, 3] > 0
    foot, ground = h // 2, h - 4
    band = alpha[max(0, ground - z1):max(0, ground - z0 + 1), :]
    cols = np.nonzero(band.any(axis=0))[0]
    return int(cols.max()) - foot if cols.size else None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--suggest", action="store_true")
    args = ap.parse_args()
    src = (GAME / "scenes/ns_data.c").read_text()
    depth = {m.group(1): int(m.group(8)) for m in DEF_RE.finditer(src)}
    for m in HERO_DEF_RE.finditer(src):
        depth.setdefault(m.group(9), int(m.group(7)))
    # what each one hits: heroes hit the crowd (6-9 deep), the crowd hits heroes (6-7)
    crowd_depth = max(v for k, v in depth.items() if k.upper() in CREW)
    hero_depth = max(depth[h] for h in HEROES)
    bad = 0
    print(f"{'move':28s} {'box x':>9s} {'z':>8s} {'front':>5s} {'gap':>4s} {'depth':>5s}")
    for table, comment, f in moves(src):
        sheet = HEROES.get(table) or CREW.get(table) or BOSSES.get(table)
        if not sheet:
            continue
        dmg, flags = int(f[3]), f[5]
        bx, bw, bz, bh, bd = (int(v) for v in f[6:11])
        if not dmg or "GRAB" in flags or "PROJECTILE" in flags or bw == 0:
            continue
        idx = frame_index(f[17], table in CREW)
        if idx is None:
            continue
        front = measure(sheet, idx, bz, bz + bh)
        if front is None:
            continue
        end = bx + bw
        gap = end - front
        reach = bd + (crowd_depth if table in HEROES else hero_depth)
        problem = []
        # a box that reaches back past the feet (sweeps, specials) is measured at its front only
        if gap < -SHORT:
            problem.append(f"{-gap} short of the picture")
        if gap > LONG and table not in BOSSES and bx >= 0:
            problem.append(f"{gap} past it")
        if reach > DEPTH and table not in BOSSES and "INVULN" not in flags:   # specials clear the lanes around
            problem.append(f"depth {reach} > {DEPTH}")
        bad += bool(problem)
        print(f"{table.lower() + ' ' + comment:28.28s} {bx:4d}..{end:<4d} {bz:3d}..{bz + bh:<4d} {front:5d} {gap:+4d} {reach:5d}"
              + ("   " + "; ".join(problem) if problem else ""))
        if args.suggest and problem:
            lf, lz0, lz1 = limb(sheet, idx)
            want_d = DEPTH - (crowd_depth if table in HEROES else hero_depth)
            nbx = bx if bx < 0 else min(bx, lf - 8)
            print(f"{'':28s}   suggest bx {nbx} bw {lf - nbx} bz {max(0, lz0 - MARGIN)} bh {lz1 - lz0 + 2 * MARGIN}"
                  f" bd {min(bd, want_d)}   (limb front {lf}, heights {lz0}..{lz1})")
    print(f"boxes: {bad} disagree" if bad else "boxes: all agree with their pictures")
    return 1 if (args.check and bad) else 0


if __name__ == "__main__":
    sys.exit(main())
