#!/usr/bin/env python3
"""
Validate generated sprite metadata for tile-window safety.

Usage:
  python3 artbox/validate_assets.py games/demo/artbox/sprite_meta.h
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROW_RE = re.compile(
    r"\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),.*?/\*\s*(.*?)\s*\*/"
)


def parse_rows(text: str):
    rows = []
    for line in text.splitlines():
        m = ROW_RE.search(line)
        if not m:
            continue
        tile_base = int(m.group(1))
        strips = int(m.group(3))
        active_rows = int(m.group(4))
        col_start = int(m.group(5))
        row_start = int(m.group(6))
        name = m.group(7)
        first = tile_base + row_start * 16 + col_start
        last = first + (active_rows - 1) * 16 + (strips - 1)
        rows.append((name, first, last, strips, active_rows))
    return rows


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: validate_assets.py <sprite_meta.h>")
        return 2

    path = Path(sys.argv[1])
    if not path.exists():
        print(f"error: file not found: {path}")
        return 2

    rows = parse_rows(path.read_text(encoding="utf-8"))
    if not rows:
        print("error: no asset rows parsed")
        return 2

    bad = 0
    for name, first, last, strips, active_rows in rows:
        if strips <= 0 or active_rows <= 0:
            bad += 1
            print(f"BAD size: {name}: strips={strips} rows={active_rows}")
        if first < 0 or last > 0xFFFF or last < first:
            bad += 1
            print(f"BAD range: {name}: first={first} last={last}")

    print(f"validated {len(rows)} assets from {path}")
    if bad:
        print(f"FAIL: {bad} issues")
        return 1
    print("OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

