#!/usr/bin/env python3
"""Capture the assembled demo M1 through MAME, with a register log and WAV."""

import argparse
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--mame", default="mame")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env["SOUND_CAPTURE_DIR"] = str(output)
    if os.name != "nt":
        env.update(DISPLAY="", SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    command = [args.mame, "neogeo", "-noreadconfig", "-rompath", str(root / "roms"),
               "-hashpath", ";".join(str(root / p) for p in ("hash_eagle/demo", "hash_eagle", "hash")),
               "-bios", "euro", "-cart1", "demo", "-video", "none", "-sound", "none",
               "-nothrottle", "-seconds_to_run", "165", "-skip_gameinfo", "-nonvram_save",
               "-samplerate", "48000", "-wavwrite", str(output / "sound.wav"),
               "-cfg_directory", str(output / "cfg"), "-nvram_directory", str(output / "nvram"),
               "-autoboot_delay", "0", "-autoboot_script", str(root / "tools/sound_capture.lua")]
    with (output / "mame.log").open("w") as log:
        subprocess.run(command, cwd=root, env=env, stdout=log,
                       stderr=subprocess.STDOUT, check=True)
    if "[LUA ERROR]" in (output / "mame.log").read_text():
        raise SystemExit(f"Capture script failed; inspect {output / 'mame.log'}")
    if (output / "loaded-m1.bin").read_bytes() != (root / "roms/demo/777-m1.m1").read_bytes()[:0x8000]:
        raise SystemExit("The Z80 is not executing the current cartridge M1")
    if not (output / "ym.tsv").exists() or (output / "ym.tsv").stat().st_size < 1000:
        raise SystemExit("No usable YM register trace was recorded")
    print(f"Sound capture: {output}")


if __name__ == "__main__":
    main()
