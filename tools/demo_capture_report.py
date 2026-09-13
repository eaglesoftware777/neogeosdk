#!/usr/bin/env python3
"""Summarize a demo_capture run and create chapter contact sheets."""

import argparse
import csv
from collections import defaultdict
from pathlib import Path

from PIL import Image, ImageDraw


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    chapters = defaultdict(list)
    with (args.directory / "frames.tsv").open(encoding="utf-8") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            chapters[int(row["chapter"])].append(row)
    for chapter, rows in sorted(chapters.items()):
        if chapter == 0:
            print(f"Boot: {len(rows)} diagnostic captures (excluded from chapter sprite budgets)")
            continue
        peak = max(int(row["max_scanline_strips"]) for row in rows)
        print(f"Chapter {chapter:02}: {len(rows):3} captures, peak {peak:2}/96 strips")

    for start in range(1, 27, 5):
        sheet = Image.new("RGB", (960, 5 * 248), "#252525")
        labels = ImageDraw.Draw(sheet)
        for row, chapter in enumerate(range(start, start + 5)):
            paths = sorted(args.directory.glob(f"ch{chapter:02}_*.png"))
            if not paths:
                continue
            for col, fraction in enumerate((0.2, 0.5, 0.8)):
                path = paths[min(len(paths) - 1, int(len(paths) * fraction))]
                with Image.open(path) as shot:
                    shot = shot.convert("RGB")
                    shot.thumbnail((320, 224), Image.Resampling.NEAREST)
                    sheet.paste(shot, (col * 320, row * 248 + 24))
                labels.text((col * 320 + 4, row * 248 + 4), path.name, fill="white")
        sheet.save(args.directory / f"overview-{start:02}.png")


if __name__ == "__main__":
    main()
