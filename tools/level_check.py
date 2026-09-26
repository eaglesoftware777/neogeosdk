#!/usr/bin/env python3
"""Check a game's level tables for placement mistakes.

    python3 tools/level_check.py --game maiya      (make test runs it)

The tables are read from the game's own C headers: games/<game>/tools/
level_export.c is built with the host C compiler and prints them as JSON,
so nothing here parses C. A game without that exporter has nothing to
check. A game whose game.mk names a GAME_LEVEL_BUILDER (stages authored
outside C, as Maiya's JSON files are) has it run first, so the check sees
what the build compiles; a finding then names the stage's file too.

Rules (every one reported as: level, object and index, coordinates, rule):
  outside       any object outside the world (x beyond 0..width, y beyond
                the 224-line screen)
  inside        a pickup overlapping a platform's drawn body, or sunk into
                the ground, deeper than --sink px (resting on top is fine)
  under         a pickup tucked right under a platform: its top within
                --under-gap px below the platform's drawn underside
  floating      a pickup more than --max-float px above the nearest surface
                below its middle (platforms, and the ground except over pits)
  overlap       two platforms whose bodies overlap

Platforms are one-way ledges: y is the surface they are stood on (width w
for standing), and they are drawn draw_w wide and ledge_depth deep below it
(the exporter reports both, as the game lays them out). Pickups and secrets
are pickup_size squares placed by their top-left corner.

Intentional cases go in games/<game>/level_check.ignore, one per line:
    <level index> <pickup|secret|platform|...> <object index> <rule>  # why
The reason is required; ignored findings are still printed, marked, and an
ignore line that matches nothing is reported too. Any finding that is not
ignored makes the exit status non-zero.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

SCREEN_H = 224
RULES = ("outside", "inside", "under", "floating", "overlap")


def host_compiler():
    cc = os.environ.get("HOST_CC") or os.environ.get("CC")
    if cc and shutil.which(cc):
        return cc
    for name in ("cc", "gcc", "clang"):
        if shutil.which(name):
            return name
    raise SystemExit("level_check: no host C compiler found (set HOST_CC)")


def export_levels(game_dir):
    source = game_dir / "tools" / "level_export.c"
    with tempfile.TemporaryDirectory() as tmp:
        exe = Path(tmp) / ("level_export.exe" if os.name == "nt" else "level_export")
        build = subprocess.run([host_compiler(), "-Wall", "-o", str(exe), f"-I{game_dir}", str(source)],
                               capture_output=True, text=True)
        if build.returncode != 0:
            raise SystemExit(f"level_check: building {source} failed:\n{build.stdout}{build.stderr}")
        out = subprocess.run([str(exe)], capture_output=True, text=True, check=True).stdout
    return json.loads(out)


def game_setting(game_dir, key):
    """A setting from the game's game.mk, as the makefile reads it."""
    mk = game_dir / "game.mk"
    if not mk.is_file():
        return ""
    for line in mk.read_text(encoding="utf-8").splitlines():
        name, _, value = line.partition("=")
        if name.strip().rstrip("?:+") == key:
            return value.strip()
    return ""


def build_levels(game_dir):
    """Run the game's level builder, if it has one; the stage files, in order."""
    builder = game_setting(game_dir, "GAME_LEVEL_BUILDER")
    if not builder:
        return []
    run = subprocess.run([sys.executable, builder], capture_output=True, text=True)
    if run.returncode != 0:
        raise SystemExit(f"level_check: {builder} failed:\n{run.stdout}{run.stderr}")
    return sorted(p.name for p in (game_dir / "levels").glob("*.json"))


def read_ignores(path):
    entries = []
    if not path.is_file():
        return entries
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line, _, reason = raw.partition("#")
        if not line.strip():
            continue
        parts = line.split()
        if len(parts) != 4 or not parts[0].isdigit() or not parts[2].isdigit() or parts[3] not in RULES:
            raise SystemExit(f"level_check: {path}:{number}: expected '<level> <kind> <index> <rule>  # why'")
        if not reason.strip():
            raise SystemExit(f"level_check: {path}:{number}: an ignore needs its reason after '#'")
        entries.append({"key": (int(parts[0]), parts[1], int(parts[2]), parts[3]),
                        "reason": reason.strip(), "line": number, "used": False})
    return entries


def check(data, opts):
    ground, size = data["ground_y"], data["pickup_size"]
    depth = opts.thickness or data["ledge_depth"]
    findings = []   # (level, kind, index, rule, where, message)

    def add(level, kind, index, rule, x, y, message):
        where = f"({x},{y})" if y is not None else f"(x {x})"
        findings.append((level, kind, index, rule, where, message))

    for lv in data["levels"]:
        width, plats = lv["width"], lv["platforms"]
        pits = [h for h in lv["hazards"] if h["pit"]]

        # outside the world
        for p in plats:
            if p["x"] < 0 or p["x"] + p["w"] > width or not 0 <= p["y"] < SCREEN_H:
                add(lv, "platform", p["i"], "outside", p["x"], p["y"], f"spans x {p['x']}..{p['x'] + p['w']} in a {width} px world")
        for h in lv["hazards"]:
            if h["x"] < 0 or h["x"] + h["w"] > width:
                add(lv, "hazard", h["i"], "outside", h["x"], None, f"spans x {h['x']}..{h['x'] + h['w']} in a {width} px world")
        for kind in ("encounters", "rescues"):
            for o in lv[kind]:
                if not 0 <= o["x"] < width:
                    add(lv, kind[:-1], o["i"], "outside", o["x"], None, f"x {o['x']} in a {width} px world")
        for a in lv["archers"]:
            if not 0 <= a["x"] < width or not 0 <= a["y"] < SCREEN_H:
                add(lv, "archer", a["i"], "outside", a["x"], a["y"], "off the world")
        if not 0 <= lv["gate_x"] < width:
            add(lv, "gate", 0, "outside", lv["gate_x"], None, f"x {lv['gate_x']} in a {width} px world")

        # platforms overlapping
        for a_i, a in enumerate(plats):
            for b in plats[a_i + 1:]:
                if a["x"] < b["x"] + b["draw_w"] and b["x"] < a["x"] + a["draw_w"] and abs(a["y"] - b["y"]) < depth:
                    add(lv, "platform", a["i"], "overlap", a["x"], a["y"],
                        f"overlaps platform {b['i']} at ({b['x']},{b['y']}): drawn x {a['x']}..{a['x'] + a['draw_w']} and "
                        f"{b['x']}..{b['x'] + b['draw_w']}, tops {abs(a['y'] - b['y'])} px apart")

        # pickups and secrets
        for kind, items in (("pickup", lv["pickups"]), ("secret", lv["secrets"])):
            for it in items:
                x, y = it["x"], it["y"]
                top, bottom, left, right, mid = y, y + size, x, x + size, x + size // 2
                if left < 0 or right > width or top < 0 or bottom > SCREEN_H:
                    add(lv, kind, it["i"], "outside", x, y, f"its {size} px box leaves the world")
                over_pit = any(h["x"] <= mid < h["x"] + h["w"] for h in pits)
                buried = False
                for p in plats:
                    if not (left < p["x"] + p["draw_w"] and p["x"] < right):
                        continue
                    body = f"platform {p['i']} at ({p['x']},{p['y']}), drawn {p['x']}..{p['x'] + p['draw_w']} x {p['y']}..{p['y'] + depth}"
                    below = top - (p["y"] + depth)          # gap from the ledge's drawn underside
                    if p["y"] <= top + size // 2 < p["y"] + depth or (top <= p["y"] and bottom > p["y"] + opts.sink):
                        # its middle is in the ledge, or it sinks through the surface from above
                        add(lv, kind, it["i"], "inside", x, y,
                            f"in {body}: {min(bottom, p['y'] + depth) - max(top, p['y'])} px of it overlap the ledge")
                        buried = True
                    elif top > p["y"] and below < opts.under_gap:
                        add(lv, kind, it["i"], "under", x, y,
                            f"its top is {-below} px up behind the underside of {body}" if below < 0 else
                            f"{below} px below the underside of {body}")
                if not over_pit and bottom > ground + opts.sink:
                    add(lv, kind, it["i"], "inside", x, y, f"{bottom - ground} px into the ground")
                surfaces = [(p["y"], f"platform {p['i']} at ({p['x']},{p['y']})") for p in plats
                            if p["x"] <= mid < p["x"] + p["w"] and p["y"] >= bottom - opts.sink]
                if not over_pit:
                    surfaces.append((ground, f"the ground at {ground}"))
                if buried:
                    pass      # inside a ledge already said it: not floating as well
                elif not surfaces:
                    add(lv, kind, it["i"], "floating", x, y, "over a pit, with nothing below it")
                else:
                    surface, what = min(surfaces)
                    if surface - bottom > opts.max_float:
                        add(lv, kind, it["i"], "floating", x, y, f"{surface - bottom} px above {what}")
    return findings


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--game", required=True)
    ap.add_argument("--max-float", type=int, default=24, help="px a pickup may hang above a surface (24)")
    ap.add_argument("--sink", type=int, default=12, help="px a pickup may sit into its surface (12)")
    ap.add_argument("--under-gap", type=int, default=8, help="px under a platform that counts as tucked under (8)")
    ap.add_argument("--thickness", type=int, default=0, help="px depth of a platform's body (default: as drawn)")
    opts = ap.parse_args()

    game_dir = Path("games") / opts.game
    if not (game_dir / "tools" / "level_export.c").is_file():
        print(f"level_check: {opts.game} has no level exporter (games/{opts.game}/tools/level_export.c); nothing to check")
        return 0
    stage_files = build_levels(game_dir)
    data = export_levels(game_dir)
    ignore_path = game_dir / "level_check.ignore"
    ignores = read_ignores(ignore_path)
    errors = ignored = 0
    for level, kind, index, rule, where, message in check(data, opts):
        key = (level["index"], kind, index, rule)
        match = next((e for e in ignores if e["key"] == key), None)
        source = f", {stage_files[level['index']]}" if level["index"] < len(stage_files) else ""
        line = f"{level['name']} (level {level['index']}{source}), {kind} {index} at {where}: {rule} -- {message}"
        if match:
            match["used"] = True
            ignored += 1
            print(f"  ignored: {line}  [{ignore_path}:{match['line']}: {match['reason']}]")
        else:
            errors += 1
            print(f"  ERROR:   {line}")
    for e in ignores:
        if not e["used"]:
            print(f"  WARNING: {ignore_path}:{e['line']} ignores {' '.join(map(str, e['key']))}, which no longer happens")
    print(f"level_check: {opts.game}: {len(data['levels'])} levels, {errors} error(s), {ignored} ignored "
          f"(max float {opts.max_float}, sink {opts.sink}, under gap {opts.under_gap}, "
          f"platform depth {opts.thickness or data['ledge_depth']})")
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
