#!/usr/bin/env python3

import argparse
import json
import re
import sys
from pathlib import Path


NEOPAL_RECORD_SIZE = 136  # int32 index + 16*uint64 palette words


def fail(msg: str) -> int:
    print(f"ERROR: {msg}", file=sys.stderr)
    return 1


def main() -> int:
    ap = argparse.ArgumentParser(description="Validate per-game artbox palette outputs.")
    ap.add_argument("--game", required=True)
    ap.add_argument("--root", default=".")
    args = ap.parse_args()

    root = Path(args.root).resolve()
    game_art = root / "games" / args.game / "artbox"
    manifest_path = game_art / "assets_manifest.json"
    screens_path = game_art / "screens.c"
    neo_pal_path = game_art / "neo.pal"
    neopal_bin_path = game_art / "neopal.bin"

    for p in (manifest_path, screens_path, neo_pal_path, neopal_bin_path):
        if not p.exists():
            return fail(f"missing required file: {p}")

    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except Exception as exc:
        return fail(f"cannot parse {manifest_path}: {exc}")

    if not manifest:
        return fail(f"{manifest_path} is empty")

    manifest_banks = set()
    owners = {}
    for spec in manifest:
        slots = spec.get("palette_slots", [spec["palette_bank"]])
        if len(slots) != 1 + len(spec.get("extra_palettes", [])):
            return fail(f"palette count mismatch for {spec['name']}")
        for index, slot in enumerate(slots):
            if not 16 <= slot < 255:
                return fail(f"asset bank {slot} overlaps reserved palette RAM")
            # A base bank may be shared, but only between assets holding
            # the same palette - that is the whole point of sharing it.
            # An extra bank is private to its asset and sharing one is a
            # collision: two different palettes would fight over the slot.
            key = spec.get("palette_key") if index == 0 else None
            if slot in owners:
                prev_name, prev_key = owners[slot]
                if key is None or prev_key is None or key != prev_key:
                    return fail(f"asset bank {slot} shared by {prev_name} "
                                f"and {spec['name']} with different palettes")
            else:
                owners[slot] = (spec["name"], key)
        manifest_banks.update(slots)
        mapping = spec.get("tile_palette_banks")
        if mapping is not None:
            expected = spec["canvas_width"] * spec["canvas_height"] // 256
            if len(mapping) != expected or not set(mapping).issubset(slots):
                return fail(f"invalid tile palette map for {spec['name']}")
    if not manifest_banks:
        return fail("no palette_bank entries in assets_manifest.json")

    text = screens_path.read_text(encoding="utf-8", errors="replace")
    loads = {int(m.group(1)) for m in re.finditer(r"load_palettes\([^)]*PALOFFSET\*(\d+)\)", text)}
    loads.update(int(m.group(1)) for m in re.finditer(r"ng_palette_load_bank\((\d+),", text))
    if not loads:
        return fail(f"no load_palettes(...PALOFFSET*X) entries in {screens_path}")

    missing_in_screens = sorted(b for b in manifest_banks if b not in loads)
    if missing_in_screens:
        return fail(
            f"palette banks in manifest not loaded in screens.c: {missing_in_screens[:12]}"
            + (" ..." if len(missing_in_screens) > 12 else "")
        )

    neopal_size = neopal_bin_path.stat().st_size
    if neopal_size % NEOPAL_RECORD_SIZE != 0:
        return fail(f"{neopal_bin_path} size {neopal_size} is not a multiple of {NEOPAL_RECORD_SIZE}")

    expected_records = len(manifest) + 1  # index 0 reserved
    got_records = neopal_size // NEOPAL_RECORD_SIZE
    if got_records != expected_records:
        return fail(
            f"{neopal_bin_path} records mismatch: got {got_records}, expected {expected_records} "
            f"(manifest entries={len(manifest)})"
        )

    print(
        f"Palette check OK [{args.game}]: "
        f"manifest={len(manifest)} banks={min(manifest_banks)}..{max(manifest_banks)} "
        f"neopal_records={got_records}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
