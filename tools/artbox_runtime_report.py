#!/usr/bin/env python3
"""Compare captured sprite tile attributes and palette RAM with generated art.

Run game_capture.py first. Palette effects deliberately change RAM, so use
strict checks on unmodified game palettes, not palette-effect chapters.
"""

import argparse
import json
from pathlib import Path
import re
import struct

from artbox_quality_report import ROOT, load_database, palette_words


def expected_tiles(folder):
    manifest = json.loads((folder / "assets_manifest.json").read_text(encoding="utf-8"))
    database = load_database(folder / "neorom.db")
    tiles = {}
    for spec in manifest:
        pixels, palette = database[spec["db_index"]]
        slots = spec.get("palette_slots", [spec["palette_bank"]])
        banks = [palette_words(palette)] + spec.get("extra_palettes", [])
        rows, cols = pixels.shape[0] // 16, pixels.shape[1] // 16
        mapping = spec.get("tile_palette_banks", [slots[0]] * (rows * cols))
        for row in range(rows):
            for col in range(cols):
                if not pixels[row * 16:row * 16 + 16, col * 16:col * 16 + 16].any():
                    continue
                offset = row * cols + col
                slot = mapping[offset]
                tiles[spec["tile_base"] + offset] = (
                    slot, banks[slots.index(slot)], f"{spec['subdir']}/{spec['name']}"
                )
    return tiles


def check_frame(vram, palette, tiles):
    checked = wrong_bank = wrong_colors = 0
    examples = []
    for sprite in range(1, 381):
        active_rows = vram[0x8200 + sprite] & 63
        if not active_rows:
            continue
        for row in range(32 if active_rows > 16 else 16):
            tile, attr = vram[sprite * 64 + row * 2:sprite * 64 + row * 2 + 2]
            tile |= (attr & 0xF0) << 12
            if tile not in tiles:
                continue
            bank, words, name = tiles[tile]
            actual = attr >> 8
            checked += 1
            bank_error = actual != bank
            color_error = list(palette[actual * 16 + 1:actual * 16 + 16]) != words[1:]
            wrong_bank += bank_error
            wrong_colors += color_error
            if (bank_error or color_error) and len(examples) < 8:
                examples.append({"sprite": sprite, "row": row, "asset": name,
                                 "expected_bank": bank, "actual_bank": actual,
                                 "colors_differ": color_error})
    return {"checked": checked, "wrong_bank": wrong_bank,
            "wrong_colors": wrong_colors, "examples": examples}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game", required=True)
    parser.add_argument("--capture", type=Path, required=True)
    parser.add_argument("--after", type=float, default=20.0,
                        help="Ignore power-on and title initialization before this time.")
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args()
    tiles = expected_tiles(ROOT / "games" / args.game / "artbox")
    frames = []
    for path in sorted(args.capture.glob("*.vram.bin")):
        timestamp = re.match(r"t([0-9.]+)_", path.name)
        if timestamp and float(timestamp[1]) < args.after:
            continue
        vram = struct.unpack(">34816H", path.read_bytes())
        palette = struct.unpack(">4096H", path.with_name(
            path.name.replace(".vram.bin", ".palette.bin")).read_bytes())
        frames.append({"frame": path.name, **check_frame(vram, palette, tiles)})
    totals = {key: sum(frame[key] for frame in frames)
              for key in ("checked", "wrong_bank", "wrong_colors")}
    report = {"game": args.game, "totals": totals, "frames": frames}
    (args.capture / "palette_report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"{args.game}: {len(frames)} frames, {totals}")
    if not totals["checked"] or (args.strict and (totals["wrong_bank"] or totals["wrong_colors"])):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
