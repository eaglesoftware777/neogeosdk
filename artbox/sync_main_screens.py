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
        # A game that does not draw generated screens has no showScreen1() to
        # anchor to.  There is nothing to splice in, and the rewrite below cuts
        # main.c at that anchor, so bailing out here leaves the file intact.
        print(f"No showScreen1() in {main_c_path}; nothing to sync.")
        return

    # main.c may open an `extern "C" {` block before showScreen1 so a C++
    # build keeps C linkage on the generated bodies.  That opening brace
    # survives the truncation below but its closing one does not, so
    # re-emit it after the spliced-in screens.
    head = main_text[: match.start()].rstrip()
    tail = '\n\n#ifdef __cplusplus\n}  /* extern "C" */\n#endif\n' \
        if 'extern "C" {' in head else "\n"
    updated = head + "\n\n" + screens_text + tail
    main_c_path.write_text(updated, encoding="utf-8")


if __name__ == "__main__":
    main()
