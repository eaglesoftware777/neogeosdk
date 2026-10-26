#!/usr/bin/env python3
"""Seven Wonders levels: levels/levels.json -> scenes/sw_levels_data.h (the build's GAME_LEVEL_BUILDER).

Seven chapters of five levels: four of matching and bubbles, then a picture
(picture logic) that ends the chapter. A picture is written as its rows:
'.' an empty cell, 1-9 a-f a filled one in that picture colour (tools/art.py
PICTURE). Each must be 5 x 5 or 10 x 10, have at most four runs in a line
(the room the clues have on screen) and solve from its clues by line logic
alone, so no guess is ever needed; this script checks all three.
"""

import argparse
import json
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
SRC = GAME / "levels/levels.json"
OUT = GAME / "scenes/sw_levels_data.h"

MATCH_CHARS = set(".12345678#cCKiIlhvbp")
BUBBLE_CHARS = set(".12345678rASR")
LOGIC_CHARS = set(".123456789abcdef")
LOGIC_SIZES = ((5, 5), (10, 10))
LOGIC_RUNS = 4
LOGIC_FILL, LOGIC_LINE, LOGIC_HEART, LOGIC_SECOND = 10, 50, 300, 5   # ng_puzzle.c's points
GOALS = {"color": "NGP_OBJ_COLOR", "ice": "NGP_OBJ_ICE", "crate": "NGP_OBJ_CRATE", "special": "NGP_OBJ_SPECIAL",
         "rescue": "NGP_OBJ_RESCUE", "clear": "NGP_OBJ_CLEAR", "score": "NGP_OBJ_SCORE",
         "picture": "NGP_OBJ_PICTURE"}
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


def runs(line):
    """The clue of a line of picture cells."""
    out, r = [], 0
    for c in line:
        if c != ".":
            r += 1
        elif r:
            out.append(r)
            r = 0
    if r:
        out.append(r)
    return out


def placements(clue, n):
    """Every line of n cells (1 filled, 0 empty) with these runs."""
    if not clue:
        yield (0,) * n
        return
    first, rest = clue[0], clue[1:]
    need = sum(rest) + len(rest)
    for start in range(0, n - need - first + 1):
        head = (0,) * start + (1,) * first
        if rest:
            for tail in placements(rest, n - start - first - 1):
                yield head + (0,) + tail
        else:
            yield head + (0,) * (n - start - first)


def line_solvable(rows, cols, w, h):
    """1 when line logic alone (no guess) fills the whole grid from the clues."""
    grid = [[None] * w for _ in range(h)]
    changed = True
    while changed:
        changed = False
        for axis, clues, n, m in ((0, rows, h, w), (1, cols, w, h)):
            for i in range(n):
                known = [grid[i][k] if axis == 0 else grid[k][i] for k in range(m)]
                if None not in known:
                    continue
                cands = [q for q in placements(clues[i], m) if all(k is None or k == v for k, v in zip(known, q))]
                if not cands:
                    return False
                for k in range(m):
                    vals = {q[k] for q in cands}
                    if len(vals) == 1 and known[k] is None:
                        v = vals.pop()
                        if axis == 0:
                            grid[i][k] = v
                        else:
                            grid[k][i] = v
                        changed = True
    return all(v is not None for r in grid for v in r)


def picture(lv, where):
    """A picture level's rows, checked: size, characters, clue room, line logic."""
    rows = lv.get("picture")
    if not isinstance(rows, list) or not rows or any(not isinstance(r, str) for r in rows):
        raise SystemExit(f"{where}: a picture is a list of rows")
    h, w = len(rows), len(rows[0])
    if (w, h) not in LOGIC_SIZES or any(len(r) != w for r in rows):
        raise SystemExit(f"{where}: pictures are 5x5 or 10x10")
    if any(c not in LOGIC_CHARS for r in rows for c in r):
        raise SystemExit(f"{where}: picture cells are from {''.join(sorted(LOGIC_CHARS))}")
    row_clues = [runs(r) for r in rows]
    col_clues = [runs("".join(r[x] for r in rows)) for x in range(w)]
    if max(len(c) for c in row_clues + col_clues) > LOGIC_RUNS:
        raise SystemExit(f"{where}: at most {LOGIC_RUNS} runs in a line (the clues' room on screen)")
    if not any(row_clues):
        raise SystemExit(f"{where}: an empty picture")
    if not line_solvable(row_clues, col_clues, w, h):
        raise SystemExit(f"{where}: the clues need a guess; change the picture")
    filled = sum(c != "." for r in rows for c in r)
    lines = sum(1 for c in row_clues + col_clues if c)
    return w, h, "".join(rows), filled, lines


def logic_c(lv, name, where):
    w, h, lay, filled, lines = picture(lv, where)
    colors = max(int(c, 16) for c in lay if c != ".")
    hearts = number(lv.get("moves", 5), 0, 99, f"{where} hearts")
    time = number(lv.get("time", 0), 0, 65535, f"{where} time")
    number(lv.get("seed", 0), 0, 0xFFFFFFFF, f"{where} seed")
    bg = lv.get("bg", "0")
    if not isinstance(bg, str) or len(bg) != 1 or bg not in "0123456789abcdef":
        raise SystemExit(f"{where}: bg is one picture colour, 0-9 a-f")
    # stars: finished; finished with three hearts left; no mistake and a minute to spare
    base = filled * LOGIC_FILL + lines * LOGIC_LINE
    stars = lv.get("stars") or [base // 2, base + min(3, hearts) * LOGIC_HEART,
                                base + hearts * LOGIC_HEART + (60 * LOGIC_SECOND if time else 0)]
    if len(stars) != 3 or stars != sorted(stars):
        raise SystemExit(f"{where}: expected three ordered star thresholds")
    decl = f'static const char {name}_layout[] = "{lay}";\n'
    body = (f"    {{ NGP_MODE_LOGIC, {w}, {h}, {colors}, {lv.get('seed', 0)}u, {hearts}, {time}, {name}_layout,\n"
            f"      {{ {{ NGP_OBJ_PICTURE, 0, 0 }}, {{ NGP_OBJ_NONE, 0, 0 }}, {{ NGP_OBJ_NONE, 0, 0 }}, {{ NGP_OBJ_NONE, 0, 0 }} }}, "
            f"{{ {stars[0]}u, {stars[1]}u, {stars[2]}u }}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }}")
    return decl, body, f"{{ {text(lv.get('name', ''), 22)}, {int(bg, 16)} }}"


def level_c(lv, name, where):
    if not isinstance(lv, dict):
        raise SystemExit(f"{where}: expected level object")
    if lv.get("mode") == "logic":
        return logic_c(lv, name, where)
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
    return decl, body, '{ "", 0 }'


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="validate without rewriting the generated header")
    args = parser.parse_args()
    data = json.loads(SRC.read_text(encoding="utf-8"))
    chapters, levels = data["chapters"], data["levels"]
    if len(chapters) != 7 or len(levels) != 35:
        raise SystemExit("seven chapters of five levels")
    for i, lv in enumerate(levels):
        if (lv.get("mode") == "logic") != (i % 5 == 4):
            raise SystemExit(f"level {i + 1}: each chapter ends with its picture, and only there")
    decls, bodies, tips, pics = [], [], [], []
    for i, lv in enumerate(levels):
        d, b, pic = level_c(lv, f"sw_l{i}", f"level {i + 1}")
        decls.append(d)
        bodies.append(b)
        pics.append(pic)
        tips.append(text(lv.get("tip", ""), 30))      # inside the level card, with a margin
    dd, db, _ = level_c(data["demo"], "sw_demo", "demo")
    ld, lb, lpic = level_c(data["demo_logic"], "sw_demo_logic", "picture demo")
    out = ["/* Generated by games/sevenwonders/tools/levels.py from levels/levels.json: do not edit. */",
           "#ifndef SW_LEVELS_DATA_H", "#define SW_LEVELS_DATA_H", *[d for d in decls if d], dd if dd else "", ld,
           "const SWChapter sw_chapters[SW_CHAPTERS] = {"]
    for c in chapters:
        if c.get("cat") not in ("mimi", "kiko"):
            raise SystemExit(f"unknown chapter cat {c.get('cat')!r}")
        out.append(f"    {{ {text(c['name'], 26)}, {text(c['line1'], 30)}, {text(c['line2'], 30)}, "
                   f"{'SW_CAT_KIKO' if c['cat'] == 'kiko' else 'SW_CAT_MIMI'} }},")
    out += ["};", "const NGPuzzleLevel sw_levels[SW_LEVELS] = {", ",\n".join(bodies), "};",
            "const char *const sw_level_tips[SW_LEVELS] = {", ",\n".join("    " + t for t in tips), "};",
            "const SWPicture sw_pictures[SW_LEVELS] = {", ",\n".join("    " + p for p in pics), "};",
            "const NGPuzzleLevel sw_demo_level =", db.strip() + ";",
            "const NGPuzzleLevel sw_demo_logic =", lb.strip() + ";",
            f"const SWPicture sw_demo_picture = {lpic};", "#endif", ""]
    text_out = "\n".join(out)
    if not OUT.exists() or OUT.read_text(encoding="ascii") != text_out:
        if args.check:
            raise SystemExit("levels: generated header is missing or stale; run the level builder")
        OUT.write_text(text_out, encoding="ascii")
    print(f"levels: {len(levels)} levels, {len(chapters)} chapters -> {OUT.relative_to(GAME.parents[1])}")


if __name__ == "__main__":
    main()
