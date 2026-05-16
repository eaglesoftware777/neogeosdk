#!/usr/bin/env python3

import argparse
import os
import pathlib
import re


ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_DATA_DIR = pathlib.Path(
    os.environ.get("ARTBOX_DATA_DIR", str(ROOT / "artbox"))
).resolve()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--main-c", default=None,
                        help="Path to game's main.c (default: games/demo/main.c)")
    parser.add_argument("--screens-c", default=None,
                        help="Path to generated screens.c (default: $ARTBOX_DATA_DIR/screens.c)")
    args = parser.parse_args()

    if args.main_c:
        main_c_path = pathlib.Path(args.main_c).resolve()
    else:
        main_c_path = ROOT / "games" / "demo" / "main.c"

    screens_c_path = pathlib.Path(args.screens_c).resolve() if args.screens_c else (DEFAULT_DATA_DIR / "screens.c")

    if not screens_c_path.exists():
        raise RuntimeError(f"{screens_c_path} was not generated. Run artbox pipeline first.")
    if not main_c_path.exists():
        raise RuntimeError(f"{main_c_path} not found.")

    main_text = main_c_path.read_text(encoding="utf-8")
    screens_text = screens_c_path.read_text(encoding="utf-8").strip()

    match = re.search(
        r"^void NEOGEO_USER showScreen1\(.*\)\s*\{$",
        main_text,
        flags=re.MULTILINE,
    )
    if not match:
        raise RuntimeError(f"showScreen1() implementation not found in {main_c_path}")

    updated = main_text[: match.start()].rstrip() + "\n\n" + screens_text + "\n"
    main_c_path.write_text(updated, encoding="utf-8")


if __name__ == "__main__":
    main()
