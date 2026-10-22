#!/usr/bin/env python3
"""Seven Wonders levels: levels/levels.json -> scenes/sw_levels_data.h (the build's GAME_LEVEL_BUILDER)."""

import argparse
import json
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
SRC = GAME / "levels/levels.json"
OUT = GAME / "scenes/sw_levels_data.h"

MATCH_CHARS = set(".12345678#cCKiIlhvbp")
BUBBLE_CHARS = set(".12345678rASR")
GOALS = {"color": "NGP_OBJ_COLOR", "ice": "NGP_OBJ_ICE", "crate": "NGP_OBJ_CRATE", "special": "NGP_OBJ_SPECIAL",
         "rescue": "NGP_OBJ_RESCUE", "clear": "NGP_OBJ_CLEAR", "score": "NGP_OBJ_SCORE"}
BUBBLE = dict(d=16, row_h=14, loss_row=11, launch_x=80, launch_y=196, speed=6 * 256)


def text(s, n):
    if not isinstance(s, str):
        raise SystemExit(f"expected text, got {s!r}")
    s = s.upper()
    if len(s) > n:
        raise SystemExit(f"text longer than {n}: {s!r}")
    if any(not (32 <= ord(c) < 127) or c == '"' or c == "\\" for c in s):
        raise SystemExit(f"bad character in {s!r}")
    return '"' + s + '"'


def number(value, low, high, where):
    if type(value) is not int or not low <= value <= high:
        raise SystemExit(f"{where}: expected integer {low}..{high}, got {value!r}")
    return value


def layout(lv, where):
    if lv["mode"] == "match":
        rows = lv.get("layout")
        if rows is None:
            return None
        if (not isinstance(rows, list) or len(rows) != lv["h"] or
                any(not isinstance(r, str) or len(r) != lv["w"] for r in rows) or
                any(c not in MATCH_CHARS for r in rows for c in r)):
            raise SystemExit(f"{where}: match layout must be {lv['h']} rows of {lv['w']} from {''.join(sorted(MATCH_CHARS))}")
        return "".join(rows)
    rows = lv.get("rows", [])
    if not isinstance(rows, list) or any(not isinstance(r, str) for r in rows):
        raise SystemExit(f"{where}: bubble rows must be a list of strings")
    if len(rows) > lv["h"]:
        raise SystemExit(f"{where}: too many bubble rows")
    out = []
    for r in range(lv["h"]):
        cells = lv["w"] - (r & 1)            # row 0 unshifted
        row = rows[r] if r < len(rows) else "." * cells
        if len(row) != cells or any(c not in BUBBLE_CHARS for c in row):
            raise SystemExit(f"{where}: bubble row {r} must be {cells} of {''.join(sorted(BUBBLE_CHARS))}")
        out.append(row + ("-" if r & 1 else ""))
    return "".join(out)


def level_c(lv, name, where):
    if not isinstance(lv, dict):
        raise SystemExit(f"{where}: expected level object")
    mode = lv.get("mode")
    w, h = lv.get("w", 0), lv.get("h", 0)
    if mode not in ("match", "bubble"):
        raise SystemExit(f"{where}: unknown puzzle mode {mode!r}")
    # These are the game's actual renderer bounds, smaller than the SDK's.
    max_w, max_h, capacity = (7, 7, 49) if mode == "match" else (10, 12, 120)
    if not (type(w) is int and type(h) is int and 3 <= w <= max_w and
            3 <= h <= max_h and w * h <= capacity):
        raise SystemExit(f"{where}: {mode} renderer supports at most {max_w}x{max_h} ({capacity} cells)")
    lay = layout(lv, where)
    colors = number(lv.get("colors"), 2, 8, f"{where} colors")
    number(lv.get("seed"), 0, 0xFFFFFFFF, f"{where} seed")
    for field in ("moves", "time"):
        number(lv.get(field, 0), 0, 65535, f"{where} {field}")
    for field in ("push_every", "miss_limit"):
        value = number(lv.get(field, 0), 0, 255, f"{where} {field}")
        if mode == "match" and value:
            raise SystemExit(f"{where}: {field} is a bubble rule")
    if lay:
        if any(c in "12345678" and int(c) > colors for c in lay):
            raise SystemExit(f"{where}: layout color exceeds colors")
        if mode == "match":
            for y in range(h):
                for x in range(w):
                    c = lay[y * w + x]
                    if c not in "12345678":
                        continue
                    if ((x + 2 < w and lay[y*w+x:y*w+x+3] == c * 3) or
                            (y + 2 < h and lay[(y+1)*w+x] == c and lay[(y+2)*w+x] == c)):
                        raise SystemExit(f"{where}: fixed match in initial layout")
    goals = []
    goal_data = lv.get("goals", [])
    if not isinstance(goal_data, list):
        raise SystemExit(f"{where}: goals must be a list")
    for g in goal_data:
        if (not isinstance(g, list) or not 1 <= len(g) <= 3 or
                not isinstance(g[0], str) or g[0] not in GOALS):
            raise SystemExit(f"{where}: invalid goal {g!r}")
        kind = g[0]
        target = g[1] if len(g) > 1 else 0
        count = g[2] if len(g) > 2 else 0
        number(target, 0, 255, f"{where} goal target")
        number(count, 0, 65535, f"{where} goal count")
        if kind == "color" and not 1 <= target <= colors:
            raise SystemExit(f"{where}: goal color exceeds colors")
        furniture = {"ice": ("match", "iI"), "crate": ("match", "cCK"),
                     "rescue": ("bubble", "R")}
        if kind in furniture:
            family, chars = furniture[kind]
            if mode != family or not lay or not any(c in chars for c in lay):
                raise SystemExit(f"{where}: {kind} goal requires {family} furniture")
        if kind == "clear" and mode != "bubble":
            raise SystemExit(f"{where}: clear goal requires bubble mode")
        goals.append(f"{{ {GOALS[kind]}, {target}, {count} }}")
    if len(goals) > 4:
        raise SystemExit(f"{where}: at most four goals")
    goals += ["{ NGP_OBJ_NONE, 0, 0 }"] * (4 - len(goals))
    stars = lv.get("stars", [0, 0, 0])
    if not isinstance(stars, list) or len(stars) != 3:
        raise SystemExit(f"{where}: expected three star thresholds")
    for value in stars:
        number(value, 0, 0xFFFFFFFF, f"{where} star threshold")
    if stars != sorted(stars):
        raise SystemExit(f"{where}: star thresholds must be ordered")
    if mode == "bubble" and BUBBLE["loss_row"] >= h:
        raise SystemExit(f"{where}: bubble loss row must fit board height")
    lay_c = f"{name}_layout" if lay else "0"
    mode = "NGP_MODE_MATCH" if lv["mode"] == "match" else "NGP_MODE_BUBBLE"
    tail = "0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0"
    if lv["mode"] == "bubble":
        tail = (f"{BUBBLE['d']}, {BUBBLE['row_h']}, {BUBBLE['loss_row']}, {lv.get('push_every', 0)}, "
                f"{lv.get('miss_limit', 0)}, 0, {BUBBLE['launch_x']}, {BUBBLE['launch_y']}, {BUBBLE['speed']}, 0, 0")
    decl = f'static const char {name}_layout[] = "{lay}";\n' if lay else ""
    body = (f"    {{ {mode}, {lv['w']}, {lv['h']}, {lv['colors']}, {lv['seed']}u, {lv.get('moves', 0)}, {lv.get('time', 0)}, {lay_c},\n"
            f"      {{ {', '.join(goals)} }}, {{ {stars[0]}u, {stars[1]}u, {stars[2]}u }}, 0, {tail} }}")
    return decl, body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="validate without rewriting the generated header")
    args = parser.parse_args()
    data = json.loads(SRC.read_text(encoding="utf-8"))
    chapters, levels = data["chapters"], data["levels"]
    if len(chapters) != 7 or len(levels) != 28:
        raise SystemExit("seven chapters of four levels")
    decls, bodies, tips = [], [], []
    for i, lv in enumerate(levels):
        d, b = level_c(lv, f"sw_l{i}", f"level {i + 1}")
        decls.append(d)
        bodies.append(b)
        tips.append(text(lv.get("tip", ""), 36))
    dd, db = level_c(data["demo"], "sw_demo", "demo")
    out = ["/* Generated by games/sevenwonders/tools/levels.py from levels/levels.json: do not edit. */",
           "#ifndef SW_LEVELS_DATA_H", "#define SW_LEVELS_DATA_H", *[d for d in decls if d], dd if dd else "",
           "const SWChapter sw_chapters[SW_CHAPTERS] = {"]
    for c in chapters:
        if c.get("cat") not in ("mimi", "kiko"):
            raise SystemExit(f"unknown chapter cat {c.get('cat')!r}")
        out.append(f"    {{ {text(c['name'], 26)}, {text(c['line1'], 30)}, {text(c['line2'], 30)}, "
                   f"{'SW_CAT_KIKO' if c['cat'] == 'kiko' else 'SW_CAT_MIMI'} }},")
    out += ["};", "const NGPuzzleLevel sw_levels[SW_LEVELS] = {", ",\n".join(bodies), "};",
            "const char *const sw_level_tips[SW_LEVELS] = {", ",\n".join("    " + t for t in tips), "};",
            "const NGPuzzleLevel sw_demo_level =", db.strip() + ";", "#endif", ""]
    text_out = "\n".join(out)
    if not OUT.exists() or OUT.read_text(encoding="ascii") != text_out:
        if args.check:
            raise SystemExit("levels: generated header is missing or stale; run the level builder")
        OUT.write_text(text_out, encoding="ascii")
    print(f"levels: {len(levels)} levels, {len(chapters)} chapters -> {OUT.relative_to(GAME.parents[1])}")


if __name__ == "__main__":
    main()
