#!/usr/bin/env python3
"""
Maiya & Luna world: the world files (world/) -> scenes/ml_world_data.h and scenes/ml_world_ids.h
(the build's GAME_LEVEL_BUILDER). The chapters are read in order; each adds
rooms, people, items and dialogue through tools/worldlib.py.
"""
import importlib
from pathlib import Path
import sys

GAME = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(GAME / "tools"))
sys.path.insert(0, str(GAME / "world"))
import worldlib  # noqa: E402

CHAPTER_FILES = ["chapter1", "chapter2", "chapter3", "chapter4", "chapter5", "chapter6", "chapter7", "finds"]


def load():
    for name in CHAPTER_FILES:
        importlib.import_module(name)
    return worldlib


def main():
    w = load()
    out = GAME / "scenes"
    writer = w.Writer()
    data = out / "ml_world_data.h"
    tmp = out / "ml_world_data.h.tmp"
    writer.write(tmp)
    if not data.exists() or data.read_bytes() != tmp.read_bytes():
        tmp.replace(data)
    else:
        tmp.unlink()
    ids = out / "ml_world_ids.h"
    tmp = out / "ml_world_ids.h.tmp"
    writer.header(tmp)
    if not ids.exists() or ids.read_bytes() != tmp.read_bytes():
        tmp.replace(ids)
    else:
        tmp.unlink()
    print(f"world: {len(w.ROOMS)} rooms, {sum(len(r['objects']) for r in w.ROOMS)} objects, "
          f"{len(w.NODE_ORDER)} dialogue pages, {len(w.FLAGS) - 1} flags, {len(w.ITEMS)} items")


if __name__ == "__main__":
    main()
