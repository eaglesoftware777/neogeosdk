#!/usr/bin/env python3
"""ROM budget: how much of each ROM a game uses, against what it allows.

    make GAME=<name> budget          print the table
    (every P ROM build)              --check: fail if a declared budget is exceeded

Budgets are optional keys in the game's game.cfg, in bytes; a K or M
suffix means KiB or MiB, and 0x... is hex:

    BUDGET_P=256K
    BUDGET_C=2M

A ROM with no key is only reported. What counts as used:

    P, M, V   everything up to the padding at the end (P is padded with
              0xFF; the M ROM's driver bank with 0x00, then 0xFF)
    C, S      tiles that hold any pixels, times the tile size (128 bytes
              for a C tile across its two planes, 32 for a fix tile), so a
              reserved tile far up the tile space doesn't count the gap

Only the Python standard library is used, so it runs the same on Linux and
Windows.
"""

import argparse
import re
import sys
from pathlib import Path

KINDS = ("P", "C", "S", "M", "V")
C_TILE = 64          # bytes of one tile in each file of a C pair
S_TILE = 32


def parse_cfg(path):
    data = {}
    if path and Path(path).is_file():
        for raw in Path(path).read_text(encoding="utf-8").splitlines():
            line = raw.split("#", 1)[0].strip()
            if "=" in line:
                key, value = line.split("=", 1)
                data[key.strip()] = value.strip()
    return data


def parse_size(key, text):
    m = re.fullmatch(r"\s*(0x[0-9a-fA-F]+|\d+)\s*([kKmM]?)\s*", text)
    if not m:
        raise SystemExit(f"rom_budget: {key}={text!r} is not a size (bytes, 0x..., or with K / M)")
    value = int(m.group(1), 0)
    return value * {"": 1, "k": 1024, "m": 1024 * 1024}[m.group(2).lower()]


def strip_tail(data, fills):
    """Length of `data` without trailing padding, taking each fill byte in turn."""
    end = len(data)
    for fill in fills:
        while end > 0 and data[end - 1] == fill:
            end -= 1
    return end


def rom_files(rom_dir, game_id, kind):
    k = kind.lower()
    pattern = re.compile(rf"{re.escape(str(game_id))}-{k}(\d+)\.{k}\d+$", re.IGNORECASE)
    found = []
    for path in Path(rom_dir).glob("*"):
        m = pattern.match(path.name)
        if m and path.is_file():
            found.append((int(m.group(1)), path))
    return [path for _, path in sorted(found)]


def used_bytes(kind, files):
    """(bytes used, detail) for one kind of ROM."""
    if kind == "C":
        tiles = 0
        for a, b in zip(files[0::2], files[1::2]):
            da, db = a.read_bytes(), b.read_bytes()
            zero = bytes(C_TILE)
            for i in range(0, min(len(da), len(db)), C_TILE):
                if da[i:i + C_TILE] != zero or db[i:i + C_TILE] != zero:
                    tiles += 1
        return tiles * C_TILE * 2, f"{tiles:,} tiles"
    if kind == "S":
        tiles = 0
        zero, full = bytes(S_TILE), b"\xff" * S_TILE
        for path in files:
            data = path.read_bytes()
            for i in range(0, len(data), S_TILE):
                tile = data[i:i + S_TILE]
                if tile != zero and tile != full:
                    tiles += 1
        return tiles * S_TILE, f"{tiles:,} tiles"
    fills = {"P": (0xFF,), "M": (0xFF, 0x00), "V": (0xFF, 0x00)}[kind]
    return sum(strip_tail(path.read_bytes(), fills) for path in files), ""


def measure(rom_dir, game_id, cfg):
    rows = []
    for kind in KINDS:
        files = rom_files(rom_dir, game_id, kind)
        key = f"BUDGET_{kind}"
        budget = parse_size(key, cfg[key]) if cfg.get(key) else None
        if not files:
            rows.append((kind, None, budget, "not built"))
            continue
        used, detail = used_bytes(kind, files)
        rows.append((kind, used, budget, detail))
    return rows


def show(game, game_id, rom_dir, cfg_path, rows):
    print(f"ROM budget: {game} ({game_id}), {rom_dir}, budgets from {cfg_path}")
    print(f"  {'ROM':<4}{'used (bytes)':>16}{'capacity (bytes)':>20}{'fill':>9}")
    for kind, used, budget, detail in rows:
        used_s = f"{used:,}" if used is not None else "-"
        cap_s = f"{budget:,}" if budget is not None else "not declared"
        fill_s = f"{used * 100.0 / budget:.1f}%" if used is not None and budget else "-"
        flag = "  OVER" if used is not None and budget is not None and used > budget else ""
        note = f"  {detail}" if detail else ""
        print(f"  {kind:<4}{used_s:>16}{cap_s:>20}{fill_s:>9}{flag}{note}")
    print("  Palette banks used: not available (the build does not record them)")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--game", required=True)
    ap.add_argument("--id", required=True, help="the game's NGH number, as in its ROM names")
    ap.add_argument("--rom-dir", help="default: roms/<game>")
    ap.add_argument("--cfg", help="default: games/<game>/game.cfg")
    ap.add_argument("--check", action="store_true", help="fail if a declared budget is exceeded")
    args = ap.parse_args()
    rom_dir = Path(args.rom_dir or Path("roms") / args.game)
    cfg_path = Path(args.cfg or Path("games") / args.game / "game.cfg")
    cfg = parse_cfg(cfg_path)
    rows = measure(rom_dir, args.id, cfg)

    if not args.check:
        show(args.game, args.id, rom_dir, cfg_path, rows)
        return 0

    over = [(k, u, b) for k, u, b, _ in rows if u is not None and b is not None and u > b]
    declared = [(k, u, b) for k, u, b, _ in rows if u is not None and b is not None]
    if over:
        for kind, used, budget in over:
            print(f"ERROR: ROM budget exceeded: the {kind} ROM uses {used:,} bytes, "
                  f"over its budget of {budget:,} bytes (BUDGET_{kind} in {cfg_path}) "
                  f"by {used - budget:,} bytes.", file=sys.stderr)
        print(f"ERROR: raise the budget in {cfg_path} or make the game smaller; "
              f"'make GAME={args.game} budget' shows every ROM.", file=sys.stderr)
        return 1
    if declared:
        print("[budget] within budget: " + ", ".join(f"{k} {u * 100.0 / b:.0f}%" for k, u, b in declared))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
