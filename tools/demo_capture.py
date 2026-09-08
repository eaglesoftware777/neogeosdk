#!/usr/bin/env python3
"""Run the MAME showcase through its real input path and capture each chapter."""

import argparse
import csv
import os
from pathlib import Path
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--seconds", type=int, default=1000)
    parser.add_argument("--game", default="demo")
    parser.add_argument("--nm", default=None)
    parser.add_argument("--mame", default="mame")
    parser.add_argument("--controls", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    nm = args.nm or shutil.which("m68k-unknown-elf-nm")
    if not nm:
        suffix = ".exe" if os.name == "nt" else ""
        sdkhome = Path(os.environ.get("SDKHOME", root.parent))
        for name in ("x-tools-v2-win", "x-tools-v2", "x-tools"):
            candidate = sdkhome / name / "m68k-unknown-elf/bin" / ("m68k-unknown-elf-nm" + suffix)
            if candidate.exists():
                nm = str(candidate)
                break
    if not nm:
        parser.error("pass --nm with the cross-toolchain nm executable")
    result = subprocess.check_output([nm, "-n", str(root / "out/game")], text=True)
    (output / "symbols.txt").write_text(result, encoding="utf-8")
    symbols = {line.split()[2]: line.split()[0] for line in result.splitlines() if len(line.split()) == 3}
    env = os.environ.copy()
    env.update(DEMO_CAPTURE_DIR=str(output),
               DEMO_CHAPTER_ADDRESS=symbols["s_chapter_view_index"],
               DEMO_ELAPSED_ADDRESS=symbols["s_chapter_elapsed"],
               DEMO_CAPTURE_CONTROLS="1" if args.controls else "0")
    if os.name != "nt":
        env.update(DISPLAY="", SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    command = [args.mame, "neogeo", "-noreadconfig", "-rompath", str(root / "roms"),
               "-hashpath", ";".join(str(root / p) for p in (f"hash_eagle/{args.game}", "hash_eagle", "hash")),
               "-bios", "euro", "-cart1", args.game, "-video", "none", "-sound", "none",
               "-nothrottle", "-seconds_to_run", str(args.seconds), "-skip_gameinfo", "-nonvram_save",
               "-cfg_directory", str(output / "cfg"), "-nvram_directory", str(output / "nvram"),
               "-snapshot_directory", str(output / "snap"), "-autoboot_delay", "0",
               "-autoboot_script", str(root / "tools/demo_capture.lua")]
    with (output / "mame.log").open("w", encoding="utf-8") as log:
        subprocess.run(command, env=env, cwd=root, stdout=log, stderr=subprocess.STDOUT, check=True)
    with (output / "frames.tsv").open(encoding="utf-8") as stream:
        visited = {int(row["chapter"]) for row in csv.DictReader(stream, delimiter="\t")}
    missing = set(range(1, 27)) - visited
    if missing:
        raise SystemExit(f"Incomplete tour; missing chapters: {sorted(missing)}")
    if args.controls:
        with (output / "controls.tsv").open(encoding="utf-8") as stream:
            checks = {(int(row["chapter"]), row["action"]) for row in csv.DictReader(stream, delimiter="\t")}
        expected = {(chapter, "C_PASS") for chapter in range(1, 27)}
        expected |= {(chapter, "A_PASS") for chapter in range(1, 26)}
        if expected - checks:
            raise SystemExit(f"Controls not verified: {sorted(expected - checks)}")
    print(f"All 26 chapters captured: {output}")


if __name__ == "__main__":
    main()
