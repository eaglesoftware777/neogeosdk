"""Conservative native-pixel transparency inspection for Sky Lance assets."""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw


def inspect(image, repair=False):
    pixels = np.array(image.convert("RGBA"), copy=True)
    before = pixels.copy()
    height, width = pixels.shape[:2]
    opaque = pixels[:, :, 3] >= 128
    seen = np.zeros((height, width), dtype=bool)
    regions = []
    for y, x in zip(*np.where(~opaque)):
        if seen[y, x]:
            continue
        pending, cells, neighbors = [(int(y), int(x))], [], set()
        seen[y, x] = True
        exterior = False
        while pending:
            cy, cx = pending.pop()
            cells.append((cx, cy))
            exterior |= cx in (0, width - 1) or cy in (0, height - 1)
            for ny, nx in ((cy - 1, cx), (cy + 1, cx), (cy, cx - 1), (cy, cx + 1)):
                if not (0 <= ny < height and 0 <= nx < width):
                    continue
                if opaque[ny, nx]:
                    neighbors.add(tuple(int(v) for v in before[ny, nx, :3]))
                elif not seen[ny, nx]:
                    seen[ny, nx] = True
                    pending.append((ny, nx))
        # Only a single, fully transparent pixel in one uniform color surface
        # is an automatic candidate. Openings and shaded regions need review.
        safe = (not exterior and len(cells) == 1 and len(neighbors) == 1
                and before[cells[0][1], cells[0][0], 3] == 0)
        applied = bool(repair and safe)
        if applied:
            cx, cy = cells[0]
            pixels[cy, cx] = (*next(iter(neighbors)), 255)
        regions.append({"coordinates": cells if not exterior else cells[:1],
                        "size": len(cells), "touches_exterior": bool(exterior),
                        "neighbor_colors": sorted(neighbors),
                        "automatic_candidate": bool(safe), "repaired": applied})
    return Image.fromarray(pixels), {"transparent_before": int((~opaque).sum()),
                                   "transparent_after": int((pixels[:, :, 3] < 128).sum()),
                                   "regions": regions}


def sheet(original, repaired):
    original, repaired = original.convert("RGBA"), repaired.convert("RGBA")
    width, height = original.size
    result = Image.new("RGB", (width * 4, height + 14), "#24343e")
    changed = np.any(np.array(original) != np.array(repaired), axis=2)
    marker = original.copy()
    data = np.array(marker)
    data[changed] = (255, 0, 128, 255)
    mask = Image.merge("RGB", (original.getchannel("A"), repaired.getchannel("A"),
                               Image.new("L", original.size)))
    for i, (label, panel) in enumerate(zip(("ORIGINAL", "REPAIRED", "CHANGES", "ALPHA R/G"),
                                          (original, repaired, Image.fromarray(data), mask))):
        if panel.mode == "RGBA":
            result.paste(panel, (i * width, 14), panel)
        else:
            result.paste(panel, (i * width, 14))
        ImageDraw.Draw(result).text((i * width, 1), label, fill="white")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--repair-uniform-pixels", action="store_true")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    reports = []
    files = sorted(args.source.rglob("*.png")) if args.source.is_dir() else [args.source]
    for path in files:
        relative = path.relative_to(args.source) if args.source.is_dir() else Path(path.name)
        image = Image.open(path).convert("RGBA")
        repaired, report = inspect(image, args.repair_uniform_pixels)
        destination = args.output / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        repaired.save(destination)
        sheet(image, repaired).save(destination.with_name(destination.stem + "_audit.png"))
        report.update(image=str(relative), sha256=hashlib.sha256(path.read_bytes()).hexdigest())
        reports.append(report)
    (args.output / "alpha_report.json").write_text(json.dumps(reports, indent=2) + "\n")


if __name__ == "__main__":
    main()
