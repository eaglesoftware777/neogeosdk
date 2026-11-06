#!/usr/bin/env python3
"""Big announcement words in the league's lettering, for words the hand-made
set in assets/authored/rebirth/announcements doesn't have.

The letters are blocky 6x8 shapes doubled to 12x16, their outer corners cut
at 45 degrees, slanted (the top three pixels further right than the foot),
then filled the way the native words are: a bright top edge, light, middle
and dark bands with a lit rim wherever a stroke's top is open, a dark
outline, and a two-step shadow to the lower right. Indices are the
announcements' own palette (project.json), so the result packs like them.

    python3 games/shadowleague/tools/big_word.py NAME "TEXT" [--preview out.png]

writes assets/authored/rebirth/announcements/NAME.png (24 pixels high, its
width a multiple of 8) and lists it in project.json.
"""
import argparse
import json
from pathlib import Path

import numpy as np
from PIL import Image

GAME = Path(__file__).resolve().parents[1]
WORDS = GAME / "assets/authored/rebirth/announcements"

GLYPHS = {
    "A": [".####.", "##..##", "##..##", "##..##", "######", "##..##", "##..##", "##..##"],
    "B": ["#####.", "##..##", "##..##", "#####.", "##..##", "##..##", "##..##", "#####."],
    "C": [".#####", "##....", "##....", "##....", "##....", "##....", "##....", ".#####"],
    "D": ["#####.", "##..##", "##..##", "##..##", "##..##", "##..##", "##..##", "#####."],
    "E": ["######", "##....", "##....", "#####.", "##....", "##....", "##....", "######"],
    "F": ["######", "##....", "##....", "#####.", "##....", "##....", "##....", "##...."],
    "G": [".#####", "##....", "##....", "##.###", "##..##", "##..##", "##..##", ".#####"],
    "H": ["##..##", "##..##", "##..##", "######", "##..##", "##..##", "##..##", "##..##"],
    "I": ["##", "##", "##", "##", "##", "##", "##", "##"],
    "J": ["....##", "....##", "....##", "....##", "....##", "##..##", "##..##", ".####."],
    "K": ["##..##", "##.##.", "####..", "###...", "####..", "##.##.", "##..##", "##..##"],
    "L": ["##....", "##....", "##....", "##....", "##....", "##....", "##....", "######"],
    "M": ["##...##", "###.###", "#######", "##.#.##", "##...##", "##...##", "##...##", "##...##"],
    "N": ["##..##", "###.##", "######", "##.###", "##..##", "##..##", "##..##", "##..##"],
    "O": [".####.", "##..##", "##..##", "##..##", "##..##", "##..##", "##..##", ".####."],
    "P": ["#####.", "##..##", "##..##", "#####.", "##....", "##....", "##....", "##...."],
    "Q": [".####.", "##..##", "##..##", "##..##", "##..##", "##.###", "##..##", ".###.#"],
    "R": ["#####.", "##..##", "##..##", "#####.", "####..", "##.##.", "##..##", "##..##"],
    "S": [".#####", "##....", "##....", ".####.", "....##", "....##", "....##", "#####."],
    "T": ["######", "..##..", "..##..", "..##..", "..##..", "..##..", "..##..", "..##.."],
    "U": ["##..##", "##..##", "##..##", "##..##", "##..##", "##..##", "##..##", ".####."],
    "V": ["##..##", "##..##", "##..##", "##..##", "##..##", "##..##", ".####.", "..##.."],
    "W": ["##...##", "##...##", "##...##", "##.#.##", "##.#.##", "#######", "###.###", "##...##"],
    "X": ["##..##", "##..##", ".####.", "..##..", ".####.", "##..##", "##..##", "##..##"],
    "Y": ["##..##", "##..##", "##..##", ".####.", "..##..", "..##..", "..##..", "..##.."],
    "Z": ["######", "....##", "...##.", "..##..", ".##...", "##....", "##....", "######"],
    "!": ["##", "##", "##", "##", "##", "..", "##", "##"],
    ".": ["..", "..", "..", "..", "..", "..", "##", "##"],
}
# Fill bands by row of the 16-pixel letter: bright edge, light, middle, a dark
# reflection band, middle again (the native words' metal).
BANDS = [6, 6, 6, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 3, 3, 3]
SLANT = 5          # rows per pixel of slant
TOP = 3            # canvas row of the letters' first filled row


def letter_mask(ch):
    rows = GLYPHS[ch]
    coarse = np.array([[c == "#" for c in r] for r in rows], bool)
    m = coarse.repeat(2, 0).repeat(2, 1)
    h, w = m.shape
    pad = np.zeros((h + 2, w + 2), bool)
    pad[1:-1, 1:-1] = m
    cut = m.copy()
    for y in range(h):
        for x in range(w):
            if not m[y, x]:
                continue
            up, dn, lf, rt = pad[y, x + 1], pad[y + 2, x + 1], pad[y + 1, x], pad[y + 1, x + 2]
            if (not up and not lf) or (not up and not rt) or (not dn and not lf) or (not dn and not rt):
                cut[y, x] = False        # a convex corner: cut at 45 degrees
    return cut


def render(text):
    letters, x = [], 0
    for ch in text.upper():
        if ch == " ":
            x += 8
            continue
        m = letter_mask(ch)
        letters.append((x, m))
        x += m.shape[1] + 3
    width = x + 6
    fill = np.zeros((24, width + 8), np.uint8)
    for x0, m in letters:
        h, w = m.shape
        for y in range(h):
            shift = (h - 1 - y) // SLANT
            for xx in range(w):
                if m[y, xx]:
                    above = y > 0 and m[y - 1, xx]
                    band = BANDS[y]
                    if y >= 3 and not above:
                        band = 6                 # a lit rim on a stroke's open top
                    fill[TOP + y, 2 + x0 + xx + shift] = band
    body = fill > 0
    out = fill.copy()
    H, W = body.shape
    outline = np.zeros_like(body)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            outline |= np.roll(np.roll(body, dy, 0), dx, 1)
    outline &= ~body
    out[outline] = 1
    solid = body | outline
    sh1 = np.roll(np.roll(solid, 1, 0), 1, 1) & ~solid
    sh2 = np.roll(np.roll(solid, 2, 0), 2, 1) & ~solid & ~sh1
    out[sh1] = 5
    out[sh2] = 7
    cols = np.nonzero(out.any(0))[0]
    right = cols.max() + 1
    width = (right + 7) // 8 * 8
    return out[:, :width]


def palette():
    return json.loads((WORDS / "project.json").read_text())["palette"]


def save(name, text, out_png):
    g = render(text)
    if g.shape[1] > 304:
        raise ValueError(f"{text}: {g.shape[1]} pixels wide (the FIX words stop at 304)")
    im = Image.fromarray(g, "P")
    flat = [v for c in palette() for v in c]
    im.putpalette(flat + [0] * (768 - len(flat)))
    im.save(out_png, transparency=0)
    return g


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("name")
    ap.add_argument("text")
    ap.add_argument("--preview", help="only write this PNG (scaled 3x, on a dark ground)")
    a = ap.parse_args()
    if a.preview:
        g = render(a.text)
        pal = np.array(palette() + [[0, 0, 0]] * 8, np.uint8)
        rgb = pal[g]
        rgb[g == 0] = (36, 36, 60)
        Image.fromarray(rgb).resize((g.shape[1] * 3, 72), Image.NEAREST).save(a.preview)
        return
    path = WORDS / f"{a.name.lower()}.png"
    g = save(a.name, a.text, path)
    project = json.loads((WORDS / "project.json").read_text())
    entry = {"name": a.name.upper(), "text": a.text, "canvas": [int(g.shape[1]), 24], "file": path.name}
    project["images"] = [i for i in project["images"] if i["name"] != entry["name"]] + [entry]
    (WORDS / "project.json").write_text(json.dumps(project, indent=2) + "\n")
    print(f"{path.name}: {g.shape[1]}x24")


if __name__ == "__main__":
    main()
