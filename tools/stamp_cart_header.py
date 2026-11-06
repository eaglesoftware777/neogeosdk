#!/usr/bin/env python3
"""
Set a game's cartridge header in its neogeo_mvs.c and neogeo_aes.c.

    python3 tools/stamp_cart_header.py games/<game> --ngh 0x0782 --save 0x200 \\
        --name "SEVEN WONDERS" --option "TRIES=1,2,3,4:2" --option "HOW TO PLAY=WITH,WITHOUT:0"

It rewrites, in both files: the NGH number, the size of the save block (at
0x100400, which the game's WORK_INIT leaves alone), and the software DIP
table -- the game's name and up to ten options, each NAME=CHOICE,CHOICE..:
default index, as the arcade board's settings menu shows them. The files
are otherwise the SDK's cartridge header as copied from another game.
"""

import argparse
import re
from pathlib import Path


def cstr(text, width):
    text = text.upper()[:width].ljust(width)
    return "{" + ",".join(f"'{c}'" for c in text) + "}"


def table(name, options):
    strings, codes = [], []
    for spec in options:
        label, rest = spec.split("=", 1)
        choices, default = rest.rsplit(":", 1)
        choices = choices.split(",")
        if not 2 <= len(choices) <= 15:
            raise SystemExit(f"{label}: 2..15 choices")
        codes.append(f"0x{int(default):X}{len(choices):X}")
        strings.append((label, [label] + choices))
    flat = [s for _, group in strings for s in group]
    lines = ["struct SOFT_DIP_TABLE {", "\tuint8_t game_name[16];", "\tuint8_t special_list[6];", "\tuint8_t option_list[10];",
             f"\tchar    strings[{len(flat)}][12];", "};",
             "struct SOFT_DIP_TABLE SOFT_DIPS __attribute__ ((section (\"neogeo_gamedata\"))) = {",
             "\t" + cstr(name, 16) + ",", "\t{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},", "\t{"]
    lines.append("\t\t" + ", ".join(codes + ["0x00"] * (10 - len(codes))))
    lines += ["\t},", "\t{"]
    lines += ["\t\t" + cstr(s, 12) + "," for s in flat]
    lines += ["\t}", "};"]
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("game")
    ap.add_argument("--ngh", required=True)
    ap.add_argument("--save", default="0x200")
    ap.add_argument("--name", required=True)
    ap.add_argument("--option", action="append", default=[])
    args = ap.parse_args()
    if len(args.option) > 10:
        raise SystemExit("at most ten options")
    for part in ("neogeo_mvs.c", "neogeo_aes.c"):
        path = Path(args.game) / part
        s = path.read_text(encoding="utf-8")
        s, n = re.subn(r'(uint16_t id __attribute__ \(\(section \("neogeo_init"\)\)\) = \s*)0x[0-9A-Fa-f]+;',
                       lambda m: m.group(1) + args.ngh + ";", s)
        if n != 1:
            raise SystemExit(f"{path}: NGH not found")
        s, n = re.subn(r'(uint16_t dipsize  __attribute__ \(\(section \("neogeo_init"\)\)\) = \s*)0x[0-9A-Fa-f]+;',
                       lambda m: m.group(1) + args.save + ";", s)
        if n != 1:
            raise SystemExit(f"{path}: save size not found")
        start = s.index("struct SOFT_DIP_TABLE {")
        end = s.index("};", s.index("SOFT_DIPS __attribute__")) + 2
        s = s[:start] + table(args.name, args.option) + s[end:]
        path.write_text(s, encoding="utf-8")
        print(f"{path}: NGH {args.ngh}, save {args.save}, {len(args.option)} options")


if __name__ == "__main__":
    main()
