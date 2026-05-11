#!/usr/bin/env python3
"""
Fix Neo Geo sprite transparency before C1/C2 generation.

Run from the repository root:
    python3 artbox/fix_sprite_alpha.py

It processes:
    artbox/in/sprite_*.png
    artbox/in/z_npc_*.png

It does NOT erase black clothing/internal pixels.  It only flood-fills
background/padding pixels connected to the image border, using the corner
colors as background candidates, then sets those pixels alpha=0.
"""
from pathlib import Path
from collections import deque
from PIL import Image

ROOT = Path(__file__).resolve().parent
IN_DIR = ROOT / "in"
PATTERNS = ("sprite_*.png", "z_npc_*.png")
TOL = 18

def close(a, b, tol=TOL):
    return all(abs(int(a[i]) - int(b[i])) <= tol for i in range(3))

def process(path: Path) -> bool:
    img = Image.open(path).convert("RGBA")
    w, h = img.size
    px = img.load()

    corners = [px[0, 0], px[w - 1, 0], px[0, h - 1], px[w - 1, h - 1]]
    bg = []
    for c in corners:
        if c[3] == 0:
            bg.append(c)
        elif not any(close(c, old) for old in bg):
            bg.append(c)

    q = deque()
    seen = set()

    def seed(x, y):
        if (x, y) in seen:
            return
        p = px[x, y]
        if p[3] == 0 or any(close(p, c) for c in bg):
            seen.add((x, y))
            q.append((x, y))

    for x in range(w):
        seed(x, 0)
        seed(x, h - 1)
    for y in range(h):
        seed(0, y)
        seed(w - 1, y)

    changed = False
    while q:
        x, y = q.popleft()
        r, g, b, a = px[x, y]
        if a != 0:
            px[x, y] = (r, g, b, 0)
            changed = True
        for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if nx < 0 or ny < 0 or nx >= w or ny >= h or (nx, ny) in seen:
                continue
            p = px[nx, ny]
            if p[3] == 0 or any(close(p, c) for c in bg):
                seen.add((nx, ny))
                q.append((nx, ny))

    if changed:
        img.save(path)
    return changed

def main():
    files = []
    for pattern in PATTERNS:
        files.extend(sorted(IN_DIR.glob(pattern)))

    changed = 0
    for path in files:
        if process(path):
            changed += 1
            print("alpha fixed", path)

    print("processed", len(files), "changed", changed)

if __name__ == "__main__":
    main()
