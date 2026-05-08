#!/usr/bin/env python3

import pathlib
import re


ROOT = pathlib.Path(__file__).resolve().parents[1]
MAIN_C = ROOT / "main.c"
SCREENS_C = ROOT / "artbox" / "screens.c"


def main():
    main_text = MAIN_C.read_text(encoding="utf-8")
    screens_text = SCREENS_C.read_text(encoding="utf-8").strip()

    match = re.search(
        r"^void NEOGEO_USER showScreen1\(.*\)\s*\{$",
        main_text,
        flags=re.MULTILINE,
    )
    if not match:
        raise RuntimeError("showScreen1() implementation not found in main.c")

    updated = main_text[: match.start()].rstrip() + "\n\n" + screens_text + "\n"
    MAIN_C.write_text(updated, encoding="utf-8")


if __name__ == "__main__":
    main()
