#!/usr/bin/env python3
"""Measure Maiya's frame time, overruns, VRAM traffic and sprites per line.

    python3 games/maiya/tools/build.py --perf      # the measurement build
    python3 games/maiya/tools/perf_report.py [--out docs/perf/maiya.md]

Runs MAME headless over seven scenarios -- stages 1, 2, 4 and 6, a guardian
fight, the Sky Road and the citadel's rush -- for 3,600 frames each
(games/maiya/tests/perf_capture.lua) and prints one table. The counters are
the game's own (sdk/ng_perf.h); a PERF build only, never a release.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
WORK = GAME / "build/workspace"
HZ = 59.1856          # MVS refresh: docs/platforms/CLASSIC_BASELINE.md
LINES = 264           # scanlines a frame
N = 3600


def toolchain(given):
    if given:
        return given
    for name in ("x-tools-v3", "x-tools-v2", "x-tools"):
        if (ROOT.parent / name / "m68k-unknown-elf/bin").is_dir():
            return ROOT.parent / name
    raise SystemExit("Pass --toolchain with the cross compiler location")


def offsets(tc):
    """Field offsets and enum values the capture script needs, from the
    staged source the measurement build was made from."""
    fields = ("state", "player", "demo", "next_stage", "state_timer", "stage", "veil", "has_key")
    chars = ("x", "y", "x_fp", "y_fp", "hp")
    src = (WORK / "games/maiya/scenes/maiya_game.c").read_text()
    src += "\n#include <stddef.h>\n"
    src += "".join(f"const int OFF_{f} = offsetof(MGState, {f});\n" for f in fields)
    src += "".join(f"const int OFF_char_{f} = offsetof(NGCharacter, {f});\n" for f in chars)
    src += "const int OFF_st_play = MG_PLAY;\nconst int OFF_st_interlude = MG_INTERLUDE;\n"
    with tempfile.TemporaryDirectory() as tmp:
        c, s = Path(tmp) / "off.c", Path(tmp) / "off.s"
        c.write_text(src)
        subprocess.run([str(tc / "m68k-unknown-elf/bin/m68k-unknown-elf-gcc"), "-S", "-O0", "-w", "-m68000",
                        "-ffreestanding", "-std=gnu99", "-I.", "-Isdk", "-Isdk/2d_engine", "-Igames/maiya/scenes",
                        "-Igames/maiya/artbox", "-Igames/maiya", "-DNG_PALFX_SCREEN=1", str(c), "-o", str(s)],
                       cwd=WORK, check=True)
        asm = s.read_text()
    found = {m.group(1): int(m.group(2)) for m in re.finditer(r"^OFF_(\w+):\s*\n\s*\.long\s+(\d+)", asm, re.M)}
    found.update({m.group(1): 0 for m in re.finditer(r"^OFF_(\w+):\s*\n\s*\.zero\s+4", asm, re.M)})
    return found


def layout(tc):
    off = offsets(tc)
    nm = subprocess.check_output([str(tc / "m68k-unknown-elf/bin/m68k-unknown-elf-nm"), str(WORK / "out/game")],
                                 text=True)
    sym = {parts[2]: int(parts[0], 16) for parts in (line.split() for line in nm.splitlines()) if len(parts) == 3}
    if "ng_perf" not in sym:
        raise SystemExit("The workspace is not a measurement build: run games/maiya/tools/build.py --perf")
    lay = {"mg": sym["mg"], "ng_perf": sym["ng_perf"], "st_play": off["st_play"],
           "st_interlude": off["st_interlude"]}
    for f in ("state", "player", "demo", "next_stage", "state_timer", "stage"):
        lay[f] = sym["mg"] + off[f]
    for f in ("veil", "has_key"):
        lay[f] = off[f]
    for f in ("x", "y", "x_fp", "y_fp", "hp"):
        lay[f"char_{f}"] = off[f"char_{f}"]
    return lay


def scenarios():
    levels = sorted((GAME / "levels").glob("*.json"))
    gate = lambda k: json.loads(levels[k].read_text())["gate_x"] - 40
    return [
        ("Stage 1 Emerald Forest", 0, "walk", 120),
        ("Stage 2 Valley of Falls", 1, "walk", 120),
        ("Stage 4 Autumn Grove", 3, "walk", 120),
        ("Stage 6 World Tree", 5, "walk", 120),
        ("Guardian fight (forest)", 0, "gate", gate(0)),
        ("Stage 11 Sky Road", 10, "walk", 120),
        ("Stage 12 citadel and rush", 11, "gate", gate(11)),
    ]


def run(mame, lay, stage, mode, at):
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        (tmp / "layout.lua").write_text("return {" + ",".join(f"{k}={v}" for k, v in lay.items()) + "}\n")
        env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", DISPLAY="",
                   PERF_LAYOUT=str(tmp / "layout.lua"), PERF_OUT=str(tmp / "result.txt"),
                   PERF_STAGE=str(stage), PERF_MODE=mode, PERF_AT=str(at))
        subprocess.run([mame, "neogeo", "-noreadconfig", "-rompath", f"{WORK / 'roms'};{ROOT / 'roms'}",
                        "-hashpath", str(WORK / "hash_eagle/maiya"), "-cart1", "maiya", "-bios", "euro",
                        "-video", "none", "-sound", "none", "-nothrottle", "-skip_gameinfo", "-nonvram_save",
                        "-cfg_directory", str(tmp / "cfg"), "-nvram_directory", str(tmp / "nvram"),
                        "-autoboot_delay", "0", "-autoboot_script", str(GAME / "tests/perf_capture.lua")],
                       env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=1800)
        result = tmp / "result.txt"
        text = result.read_text() if result.is_file() else "ERROR no result"
    if text.startswith("ERROR"):
        return {"error": text.strip()}
    return {k: int(v) for k, v in re.findall(r"(\w+)=(-?\d+)", text)}


def table(rows):
    out = ["| Scenario | Game fps | Overran | Work, avg lines | Peak | VRAM words, avg / peak | Written on drawn lines | Most strips on a line |",
           "|---|---|---|---|---|---|---|---|"]
    for name, r in rows:
        if "error" in r:
            out.append(f"| {name} | {r['error']} | | | | | | |")
            continue
        f = max(1, r["frames"])
        peak = f"{r['lines_peak']}" + ("+" if r["lines_peak"] >= 2 * LINES - 1 else "")
        out.append(f"| {name} | {HZ * r['frames'] / N:.1f} | {100 * r['overruns'] / f:.0f}% | "
                   f"{r['lines_sum'] / f:.0f} of {LINES} | {peak} | {r['vram_sum'] / f:.0f} / {r['vram_peak']} | "
                   f"{100 * r['active_sum'] / max(1, r['vram_sum']):.0f}% | {r['strips_peak']} (line {r['strips_line']})"
                   + (f", over 96 in {r['over96']} samples" if r["over96"] else "") + " |")
    return "\n".join(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--mame", default="mame")
    ap.add_argument("--toolchain", type=Path)
    ap.add_argument("--out", type=Path, help="also write the table to this Markdown file")
    args = ap.parse_args()
    lay = layout(toolchain(args.toolchain))
    rows = []
    for name, stage, mode, at in scenarios():
        r = run(args.mame, lay, stage, mode, at)
        rows.append((name, r))
        print(f"{name}: {r}", file=sys.stderr, flush=True)
    text = table(rows)
    print(text)
    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(text + "\n")


if __name__ == "__main__":
    main()
