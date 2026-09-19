"""Headless coin/start, scrolling and combat smoke test for the built cartridge."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess

from build import GAME, ROOT, WORK


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seconds", type=int, default=45)
    parser.add_argument("--idle", action="store_true")
    parser.add_argument("--mame", default="mame")
    args = parser.parse_args()
    output = GAME / "build" / ("capture_idle" if args.idle else "capture")
    output.mkdir(parents=True, exist_ok=True)
    nm = next(ROOT.parent.glob("x-tools-v*/m68k-unknown-elf/bin/m68k-unknown-elf-nm"))
    symbols = {}
    for line in subprocess.check_output([str(nm), str(WORK / "out/game")], text=True).splitlines():
        words = line.split()
        if len(words) == 3:
            symbols[words[2]] = int(words[0], 16)
    env = os.environ.copy()
    env.update(SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", DISPLAY="",
               MG_CAPTURE_DIR=str(output), MG_CAPTURE_IDLE="1" if args.idle else "0")
    for name, key in (("stage", "STAGE"), ("state", "STATE"), ("x", "X"),
                      ("y", "Y"), ("boss_hp", "BOSS"), ("rescues", "RESCUE")):
        env[f"MG_{key}_ADDR"] = str(symbols[f"mg_debug_{name}"])
    command = [args.mame, "neogeo", "-noreadconfig", "-rompath",
               f"{WORK / 'roms'};{ROOT / 'roms'}", "-hashpath",
               str(WORK / "hash_eagle/maiya"), "-cart1", "maiya", "-bios", "euro",
               "-video", "none", "-sound", "none", "-nothrottle", "-nonvram_save",
               "-seconds_to_run", str(args.seconds), "-skip_gameinfo",
               "-cfg_directory", str(output / "cfg"), "-nvram_directory", str(output / "nvram"),
               "-snapshot_directory", str(output), "-autoboot_delay", "0",
               "-autoboot_script", str(GAME / "tools/capture.lua")]
    with (output / "mame.log").open("w") as log:
        subprocess.run(command, env=env, cwd=output, stdout=log, stderr=subprocess.STDOUT, check=True)
    print(output)


if __name__ == "__main__":
    main()
