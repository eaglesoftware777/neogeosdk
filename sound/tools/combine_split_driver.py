#!/usr/bin/env python3
"""Merge the fixed Z80 prelude with compiler-generated driver assembly."""

from pathlib import Path
import sys


def main() -> int:
    if len(sys.argv) != 4:
        print(
            "usage: combine_split_driver.py <prelude.asm> <driver.gen.asm> <out.asm>",
            file=sys.stderr,
        )
        return 1

    prelude = Path(sys.argv[1]).read_text()
    generated_lines = Path(sys.argv[2]).read_text().splitlines()
    out_path = Path(sys.argv[3])

    start = None
    for i, line in enumerate(generated_lines):
        if line.strip() == ".org $00D0":
            start = i
            break

    if start is None:
        print("could not find .org $00D0 in generated driver asm", file=sys.stderr)
        return 1

    # Keep the fixed vector/prelude block exactly as-authored, then splice in
    # the generated body from the first gameplay entry point onward.
    body = "\n".join(generated_lines[start:]).rstrip() + "\n"
    out_path.write_text(prelude.rstrip() + "\n\n" + body)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
