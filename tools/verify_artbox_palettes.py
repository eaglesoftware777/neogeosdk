#!/usr/bin/env python3

import argparse
import json
import re
import struct
import sys
from pathlib import Path


NEOPAL_RECORD_SIZE = 136  # int32 index + 16*uint64 palette words


def fail(msg: str) -> int:
    print(f"ERROR: {msg}", file=sys.stderr)
    return 1


def validate_palette_ownership(manifest, base_palettes):
    """Compare actual ordered words, not a manifest's claimed hash."""
    owners = {}
    for spec in manifest:
        slots = spec.get("palette_slots", [spec["palette_bank"]])
        palettes = [base_palettes[spec["db_index"]]] + spec.get("extra_palettes", [])
        if len(slots) != len(palettes) or slots[0] != spec["palette_bank"]:
            raise ValueError(f"palette count or base mismatch for {spec['name']}")
        for slot, palette in zip(slots, palettes):
            if not 16 <= slot < 255:
                raise ValueError(f"asset bank {slot} overlaps reserved palette RAM")
            words = tuple(int(word) for word in palette)
            if len(words) != 16 or any(word < 0 or word > 65535 for word in words):
                raise ValueError(f"invalid palette words for {spec['name']}")
            if slot in owners and owners[slot][1] != words:
                raise ValueError(f"asset bank {slot} shared by {owners[slot][0]} "
                                 f"and {spec['name']} with different palettes")
            owners[slot] = spec["name"], words
        mapping = spec.get("tile_palette_banks")
        if len(slots) > 1 and mapping is None:
            raise ValueError(f"missing tile palette map for {spec['name']}")
        if mapping is not None:
            width, height = spec["canvas_width"], spec["canvas_height"]
            if (width <= 0 or height <= 0 or width % 16 or height % 16
                    or len(mapping) != width * height // 256
                    or not set(mapping).issubset(slots)):
                raise ValueError(f"invalid tile palette map for {spec['name']}")
    return set(owners)


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

    data = neopal_bin_path.read_bytes()
    if len(data) != (len(manifest) + 1) * NEOPAL_RECORD_SIZE:
        return fail("neopal.bin record count does not match the manifest")
    try:
        bases = {spec["db_index"]: struct.unpack_from(
            "<16Q", data, spec["screen_id"] * NEOPAL_RECORD_SIZE + 8)
            for spec in manifest}
        manifest_banks = validate_palette_ownership(manifest, bases)
    except (ValueError, KeyError, struct.error) as exc:
        return fail(str(exc))
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
