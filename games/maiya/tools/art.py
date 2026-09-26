"""Build Maiya's frame-based art without passing it through screen conversion."""

import argparse
from pathlib import Path
import shutil
import subprocess
import sys

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fix-only", action="store_true")
    args = parser.parse_args()
    art = GAME / "artbox"
    roms = ROOT / "roms/maiya"
    roms.mkdir(parents=True, exist_ok=True)
    if not args.fix_only:
        subprocess.run([sys.executable, str(GAME / "tools/build_commercial_assets.py")], check=True)
        for part in ("c1", "c2"):
            name = f"780-{part}.{part}"
            shutil.copyfile(art / "generated" / name, art / name)
            shutil.copyfile(art / "generated" / name, roms / name)
    # Preserve the game's existing ASCII font; a clean tree can generate
    # the original system font before adding Maiya's low-code HUD glyphs.
    if not (art / "780-s1.s1").exists():
        subprocess.run([sys.executable, str(ROOT / "bios/tools/gen_sfix.py"), str(art / "780-s1.s1")], check=True)
    from build_fix_assets import inject_hud_glyphs
    inject_hud_glyphs()
    shutil.copyfile(art / "780-s1.s1", roms / "780-s1.s1")


if __name__ == "__main__":
    main()
