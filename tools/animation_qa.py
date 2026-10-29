#!/usr/bin/env python3
"""
Animation checks on the frames that go into a game's C ROM: for every named
cycle (walk, run, attacks...) the foot baseline, the horizontal anchor, the
frame size, colour counts and repeated identical frames, plus a contact sheet
per character with the cycle's baseline drawn across it.

    python3 tools/animation_qa.py --game nosignal [--out docs/qa/animation]

Writes <out>/<game>/<character>.png and <out>/<game>/metrics.json, and prints
one line per cycle. The numbers are evidence for review, not a verdict on
the art: a deliberate hop moves the baseline, and a human decides.

Measures, on the opaque pixels of each frame:
  foot      the lowest opaque row (the contact line; airborne frames excluded)
  anchor    the x centre of the opaque pixels in the lowest 6 rows (the feet)
  body      the x centre of the whole figure
  size      the opaque bounding box
Per cycle:
  baseline jitter   foot range across grounded frames (0 is a steady floor)
  anchor drift      body centre range (a figure that jumps sideways between
                    frames reads as the sprite origin moving)
  repeats           frames identical to an earlier frame of the same cycle
"""

import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]

# Frame numbers from games/nosignal/scenes/ns.h (HF_*, EF_*, BF_*).
NS_HERO = {
    "idle": [0, 1], "walk": [41, 42, 43, 44, 45, 46], "run": [47, 48, 49, 39],
    "jab": [31, 11, 32], "cross": [31, 12, 32], "upper": [33, 13, 34], "kick": [35, 14, 36],
    "sweep": [37, 16], "jump": [8, 9, 38], "hurt": [21, 22], "knockdown": [23, 24, 25],
}
NS_ENEMY = {"walk": [1, 2, 3, 2], "attack": [4, 5, 0], "hurt": [6, 7], "knockdown": [7, 8, 9]}
NS_AIR = {("run", 39), ("jump", 8), ("jump", 9), ("knockdown", 23), ("knockdown", 7)}

GAMES = {
    "nosignal": {
        "sheets": "games/nosignal/artbox/generated",
        "assets": "games/nosignal/artbox/generated/assets.json",
        "characters": {
            **{h: NS_HERO for h in ("jay", "kai", "zoe")},
            **{e: NS_ENEMY for e in ("thief", "runner", "walker", "skater", "android", "biker", "bloater",
                                    "alien", "drone", "robodog", "guard")},
            **{b: {**NS_ENEMY, "boss sheet": list(range(10, 20))} for b in ("rico", "bigflex", "zerox", "m08",
                                                                          "behemoth", "proxy", "overmind")},
            "archivist": {**NS_ENEMY, "boss sheet": list(range(10, 20)), "third sheet": list(range(20, 30))},
        },
        "air": NS_AIR,
        # fliers have no feet on the floor: only anchor and repeats apply
        "fliers": {"drone"},
    },
}


def frames_of(sheet, count):
    w = sheet.width // count
    return [sheet.crop((i * w, 0, (i + 1) * w, sheet.height)) for i in range(count)]


def measure(im):
    a = im.getchannel("A")
    box = a.getbbox()
    if not box:
        return None
    px = a.load()
    w, h = im.size
    foot = box[3] - 1
    feet = [x for y in range(max(box[1], foot - 5), foot + 1) for x in range(w) if px[x, y]]
    body = [x for y in range(box[1], box[3]) for x in range(w) if px[x, y]]
    colours = len({c for c, al in zip(im.convert("RGB").getdata(), a.getdata()) if al})
    return {
        "foot": foot, "anchor": round(sum(feet) / len(feet), 1), "body": round(sum(body) / len(body), 1),
        "box": list(box), "width": box[2] - box[0], "height": box[3] - box[1], "colours": colours,
        "hash": hashlib.sha1(im.tobytes()).hexdigest()[:12],
    }


def audit(game):
    cfg = GAMES[game]
    meta = json.loads((ROOT / cfg["assets"]).read_text())
    assets = meta["assets"] if isinstance(meta, dict) and "assets" in meta else meta
    by_name = {a["name"]: a for a in (assets if isinstance(assets, list) else assets.values()) if isinstance(a, dict)}
    out = {}
    for name, cycles in cfg["characters"].items():
        a = by_name.get(f"f_{name}")
        if not a:
            out[name] = {"error": "no sheet"}
            continue
        sheet = Image.open(ROOT / cfg["sheets"] / f"f_{name}.png").convert("RGBA")
        frames = frames_of(sheet, a["frames"])
        m = [measure(f) for f in frames]
        res = {"frames": a["frames"], "frame_size": [frames[0].width, frames[0].height], "cycles": {}}
        for cyc, idx in cycles.items():
            idx = [i for i in idx if i < len(frames)]
            ms = [m[i] for i in idx]
            if not idx or any(x is None for x in ms):
                res["cycles"][cyc] = {"frames": idx, "error": "empty frame"}
                continue
            grounded = [x for i, x in zip(idx, ms) if (cyc, i) not in cfg["air"]]
            seen, repeats = {}, []
            for i, x in zip(idx, ms):
                if x["hash"] in seen and seen[x["hash"]] != i:
                    repeats.append([seen[x["hash"]], i])
                seen.setdefault(x["hash"], i)
            res["cycles"][cyc] = {
                "frames": idx,
                "distinct": len({x["hash"] for x in ms}),
                "baseline_jitter": None if name in cfg["fliers"] or not grounded else max(x["foot"] for x in grounded) - min(x["foot"] for x in grounded),
                "anchor_drift": round(max(x["body"] for x in ms) - min(x["body"] for x in ms), 1),
                "height_range": [min(x["height"] for x in ms), max(x["height"] for x in ms)],
                "max_colours": max(x["colours"] for x in ms),
                "repeats": repeats,
            }
        res["per_frame"] = m
        out[name] = res
        out[name]["_sheet"] = (frames, m, cycles)
    return out


def contact_sheet(name, frames, m, cycles, air, dest):
    fw, fh = frames[0].size
    rows = [(c, [i for i in idx if i < len(frames)]) for c, idx in cycles.items()]
    cols = max(len(i) for _, i in rows)
    im = Image.new("RGB", (110 + cols * (fw + 4), len(rows) * (fh + 18) + 4), (40, 44, 52))
    d = ImageDraw.Draw(im)
    for r, (cyc, idx) in enumerate(rows):
        y0 = 4 + r * (fh + 18)
        d.text((6, y0 + fh // 2 - 6), cyc, fill=(230, 230, 230))
        grounded = [m[i]["foot"] for i in idx if m[i] and (cyc, i) not in air]
        base = sorted(grounded)[len(grounded) // 2] if grounded else None
        for c, i in enumerate(idx):
            x0 = 110 + c * (fw + 4)
            d.rectangle((x0, y0, x0 + fw - 1, y0 + fh - 1), fill=(92, 98, 110))
            im.paste(frames[i], (x0, y0), frames[i])
            if base is not None:
                d.line((x0, y0 + base, x0 + fw - 1, y0 + base), fill=(255, 64, 64))
            if m[i]:
                ax = x0 + int(m[i]["anchor"])
                d.line((ax, y0 + fh - 6, ax, y0 + fh - 1), fill=(64, 160, 255), width=2)
            d.text((x0 + 2, y0 + fh + 2), f"#{i}", fill=(200, 200, 200))
    dest.parent.mkdir(parents=True, exist_ok=True)
    im.save(dest, optimize=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", required=True, choices=sorted(GAMES))
    ap.add_argument("--out", type=Path, default=ROOT / "docs" / "qa" / "animation")
    args = ap.parse_args()
    res = audit(args.game)
    clean = {}
    for name, r in res.items():
        if "_sheet" in r:
            frames, m, cycles = r.pop("_sheet")
            contact_sheet(name, frames, m, cycles, GAMES[args.game]["air"], args.out / args.game / f"{name}.png")
        clean[name] = r
        for cyc, c in r.get("cycles", {}).items():
            if "error" in c:
                print(f"{name:10} {cyc:12} {c['error']}")
                continue
            print(f"{name:10} {cyc:12} frames {len(c['frames']):2} distinct {c['distinct']:2}  baseline jitter "
                  f"{'-' if c['baseline_jitter'] is None else c['baseline_jitter']:>2}  anchor drift {c['anchor_drift']:5}  "
                  f"height {c['height_range'][0]}-{c['height_range'][1]}  colours {c['max_colours']:3}"
                  + (f"  repeats {c['repeats']}" if c["repeats"] else ""))
    (args.out / args.game).mkdir(parents=True, exist_ok=True)
    (args.out / args.game / "metrics.json").write_text(json.dumps(clean, indent=1) + "\n")


if __name__ == "__main__":
    main()
