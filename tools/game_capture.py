#!/usr/bin/env python3
"""Boot any game in this tree under MAME and capture frames from it.

    python3 tools/game_capture.py --game skylance --output /tmp/sky --seconds 90

Headless: no window, no sound, no throttle.  Use it to look at a game the
demo's chapter-aware capture cannot drive.
"""

import argparse
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--game", default="skylance")
    parser.add_argument("--seconds", type=int, default=90)
    parser.add_argument("--every", type=float, default=2.0,
                        help="Seconds between snapshots.")
    parser.add_argument("--no-play", action="store_true",
                        help="Coin and start only; leave the stick alone.")
    parser.add_argument("--bios", default="euro")
    parser.add_argument("--mame", default="mame")
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)

    env = os.environ.copy()
    env.update(GAME_CAPTURE_DIR=str(output),
               GAME_CAPTURE_EVERY=str(args.every),
               GAME_CAPTURE_PLAY="0" if args.no_play else "1")
    if os.name != "nt":
        env.update(DISPLAY="", SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")

    hashpath = ";".join(str(root / p) for p in
                        (f"hash_eagle/{args.game}", "hash_eagle", "hash"))
    command = [args.mame, "neogeo", "-noreadconfig",
               "-rompath", str(root / "roms"), "-hashpath", hashpath,
               "-bios", args.bios, "-cart1", args.game,
               "-video", "none", "-sound", "none", "-nothrottle",
               "-seconds_to_run", str(args.seconds), "-skip_gameinfo",
               "-nonvram_save",
               "-cfg_directory", str(output / "cfg"),
               "-nvram_directory", str(output / "nvram"),
               "-snapshot_directory", str(output),
               "-autoboot_delay", "0",
               "-autoboot_script", str(root / "tools/game_capture.lua")]

    with (output / "mame.log").open("w", encoding="utf-8") as log:
        subprocess.run(command, env=env, cwd=root, stdout=log,
                       stderr=subprocess.STDOUT, check=True)
    print(f"Capture complete: {output}")


if __name__ == "__main__":
    main()
