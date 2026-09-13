#!/usr/bin/env python3

import argparse
import os
import sys
from pathlib import Path


def fail(msg: str) -> int:
    print(f"ERROR: {msg}", file=sys.stderr)
    return 1


def main() -> int:
    ap = argparse.ArgumentParser(description="Validate generated FIX ROM output.")
    ap.add_argument("--root", default=".")
    ap.add_argument("--game", required=True)
    ap.add_argument("--game-id", required=True)
    args = ap.parse_args()

    root = Path(args.root).resolve()
    game_art = root / "games" / args.game / "artbox"
    rom_dir = root / "roms" / args.game
    s1_game = game_art / f"{args.game_id}-s1.s1"
    s1_rom = rom_dir / f"{args.game_id}-s1.s1"
    db_path = game_art / "neorom.db"
    infix_dir = game_art / "infix"

    if not s1_game.exists():
        return fail(f"missing generated FIX ROM: {s1_game}")
    if s1_game.stat().st_size != 131072:
        return fail(f"{s1_game} size is {s1_game.stat().st_size}, expected 131072")
    if not s1_rom.exists():
        return fail(f"missing copied FIX ROM in roms: {s1_rom}")
    if s1_rom.stat().st_size != 131072:
        return fail(f"{s1_rom} size is {s1_rom.stat().st_size}, expected 131072")

    with s1_game.open("rb") as f:
        head = f.read(4096)
    if not head:
        return fail(f"{s1_game} is empty")
    if all(b == 0 for b in head):
        return fail(f"{s1_game} header block is all zero; likely bad sfix seed/input")

    if not db_path.exists() and not infix_dir.exists():
        return fail(
            f"neither {db_path} nor {infix_dir} exists; FIX source assets are missing"
        )

    print(
        f"SFIX check OK [{args.game}]: {s1_game.name}={s1_game.stat().st_size} bytes, "
        f"copied={s1_rom.stat().st_size} bytes"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
