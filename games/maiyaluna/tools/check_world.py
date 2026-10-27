#!/usr/bin/env python3
"""Regenerate the world tables, build tools/check_world.c on this machine and run it
(it also writes scenes/ml_autoplay_<journey>.h, the ML_AUTOPLAY build's steps; extra arguments
--upto N, --journey maiya|luna go to the checker)."""
from pathlib import Path
import subprocess
import sys

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]


def main():
    subprocess.run([sys.executable, str(GAME / "tools/world.py")], check=True)
    exe = GAME / "tools/check_world"
    subprocess.run(["cc", "-std=c99", "-O1", "-Wall", "-Wextra", "-Wno-missing-field-initializers", "-I", str(ROOT),
                    "-I", str(ROOT / "sdk"), "-I", str(ROOT / "sdk/2d_engine"), "-o", str(exe),
                    str(GAME / "tools/check_world.c"), str(GAME / "scenes/ml_world.c"),
                    str(ROOT / "sdk/2d_engine/ng_rpg.c")], check=True)
    r = subprocess.run([str(exe), "--autoplay", str(GAME / "scenes")] + sys.argv[1:])
    exe.unlink()
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
