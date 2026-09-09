#!/usr/bin/env python3
"""Compare source pixels with reconstructed C-ROM artwork, not PNG previews."""

import argparse
import io
import json
from pathlib import Path
import sqlite3
import sys

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "artbox"))
from img2neo import rgb_to_lab
from img2neo_tile import fit_screen_for_display
from palette_banks import palette_words
from romdbimgimport import prepare_source_sprite
from tile_codec import decode_image


def colors(words):
    words = np.asarray(words, dtype=np.uint32)
    parity = 1 - (words >> 15)
    rgb5 = np.stack((((words >> 7) & 30) | ((words >> 14) & 1),
                     ((words >> 3) & 30) | ((words >> 13) & 1),
                     ((words << 1) & 30) | ((words >> 12) & 1)), axis=-1)
    rgb6 = (rgb5 << 1) | parity[..., None]
    return ((rgb6 << 2) | (rgb6 >> 4)).astype(np.uint8)


def load_database(path):
    with sqlite3.connect(path) as db:
        return {idx: (np.load(io.BytesIO(data)), np.load(io.BytesIO(pal)))
                for idx, data, pal in db.execute("select idx,data,palette from image")}


def render(spec, indexed, palette):
    banks = [palette_words(palette)] + spec.get("extra_palettes", [])
    rgb_banks = colors(banks)
    slots = spec.get("palette_slots", [spec["palette_bank"]])
    mapping = spec.get("tile_palette_banks", [slots[0]] * (indexed.size // 256))
    offsets = np.array([slots.index(bank) for bank in mapping], dtype=np.uint8)
    selected = offsets.reshape(indexed.shape[0] // 16, indexed.shape[1] // 16).repeat(16, 0).repeat(16, 1)
    rgb = rgb_banks[selected, indexed]
    rgb[indexed == 0] = 255
    return rgb


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game", default="demo")
    parser.add_argument("--before", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    folder = ROOT / "games" / args.game / "artbox"
    specs = json.loads((folder / "assets_manifest.json").read_text())
    old_specs = json.loads((args.before / "assets_manifest.json").read_text())
    before = load_database(args.before / "neorom.db")
    after = load_database(folder / "neorom.db")
    rom1 = next(folder.glob("*-c1.c1")).read_bytes()
    rom2 = next(folder.glob("*-c2.c2")).read_bytes()
    args.output.mkdir(parents=True, exist_ok=True)
    report = []
    examples = []
    wanted = {"backgrounds/0.png", "backgrounds/1.png", "titles/8.png", "titles/9.png",
              "characters/sprite_066_r07_c01.png", "characters/sprite_076_r07_c11.png",
              "characters/sprite_021_r02_c09.png", "effects/sprite_048.png",
              "npcs/zzzzzzzzzz_sky_plane.png", "npcs/zzzzzzzzzz_sky_enemy_a.png"}
    for spec, old_spec in zip(specs, old_specs):
        idx = spec["db_index"]
        if spec["name"] != old_spec["name"]:
            raise ValueError("Asset order changed; compare matching manifests")
        indexed, palette = after[idx]
        start = spec["tile_base"] * 64
        decoded = decode_image(rom1[start:], rom2[start:], indexed.shape[1], indexed.shape[0], True)
        np.testing.assert_array_equal(decoded, indexed, err_msg=f"ROM mismatch: {spec['name']}")
        if spec["mode"] == "sprite":
            rgba, _ = prepare_source_sprite(spec)
        else:
            with Image.open(spec["path"]) as img:
                canvas, *_ = fit_screen_for_display(img, spec["canvas_width"], spec["canvas_height"],
                    spec.get("display_shrink_y", 255), spec["fit"], spec["anchor"])
            rgba = np.array(canvas)
        source = rgba[:, :, :3].copy()
        visible = rgba[:, :, 3] >= 128
        source[~visible] = 255
        old_rgb = render(old_spec, *before[idx])
        new_rgb = render(spec, decoded, palette)
        lab = rgb_to_lab(source)
        entry = {"asset": f"{spec['subdir']}/{spec['name']}", "banks": len(spec.get("palette_slots", [0]))}
        if visible.any() and old_rgb.shape == source.shape:
            for label, rgb in (("before", old_rgb), ("after", new_rgb)):
                entry[label + "_delta_e"] = round(float(np.linalg.norm(lab - rgb_to_lab(rgb), axis=2)[visible].mean()), 3)
        report.append(entry)
        if entry["asset"] in wanted:
            examples.append((entry, source, old_rgb, new_rgb))
    (args.output / "quality.json").write_text(json.dumps(report, indent=2) + "\n")
    sheet = Image.new("RGB", (3 * 272, len(examples) * 290), "#dddddd")
    draw = ImageDraw.Draw(sheet)
    for row, (entry, *pictures) in enumerate(examples):
        draw.text((8, row * 290), entry["asset"], fill="black")
        for col, rgb in enumerate(pictures):
            picture = Image.fromarray(rgb)
            picture.thumbnail((256, 256), Image.Resampling.NEAREST)
            sheet.paste(picture, (col * 272 + 8, row * 290 + 30))
            label = ("Source", "Before", "After")[col]
            if col:
                label += f"  Delta E {entry.get(('before', 'after')[col - 1] + '_delta_e', '?')}"
            draw.text((col * 272 + 8, row * 290 + 16), label, fill="black")
    sheet.save(args.output / "comparison.png")
    print(f"Verified {len(specs)} assets against both C-ROMs; report: {args.output}")


if __name__ == "__main__":
    main()
