#!/usr/bin/env python3
"""Record the demo's real audio path: boot, coin, Start, chapters.

Unlike tools/sound_capture.py this parks nothing and injects nothing.  The
68000 sends its own commands; the script only listens, so what it records
is what a player hears.  Output: sound.wav plus events.tsv holding chapter
changes, every sound-latch byte with the 68000 PC that wrote it, and the
YM writes that decide whether a bed is audible.  Feed the folder to
tools/demo_audio_report.py.
"""
import argparse
import os
import shutil
import subprocess
from pathlib import Path


def find_nm(root):
    for name in ("m68k-unknown-elf-nm", "m68k-elf-nm"):
        found = shutil.which(name)
        if found:
            return found
    sdkhome = Path(os.environ.get("SDKHOME", root.parent))
    suffix = ".exe" if os.name == "nt" else ""
    for name in ("x-tools-v2-win" if os.name == "nt" else "x-tools-v2", "x-tools"):
        candidate = sdkhome / name / "m68k-unknown-elf/bin" / ("m68k-unknown-elf-nm" + suffix)
        if candidate.exists():
            return str(candidate)
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--seconds", type=int, default=600)
    parser.add_argument("--press-every", type=float, default=0.0,
                        help="press A this many seconds into every chapter (0 = never)")
    parser.add_argument("--stop-chapter", type=int, default=26)
    parser.add_argument("--coin-at", type=float, default=0.0,
                        help="insert a second coin at this many seconds (0 = never)")
    parser.add_argument("--mame", default="mame")
    parser.add_argument("--trace-z80", action="store_true", help="also log the Z80 command queue")
    parser.add_argument("--nm", default=None)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    nm = args.nm or find_nm(root)
    if not nm:
        parser.error("pass --nm with the cross-toolchain nm executable")
    listing = subprocess.check_output([nm, "-n", str(root / "out/game")], text=True)
    symbols = {line.split()[2]: line.split()[0] for line in listing.splitlines() if len(line.split()) == 3}
    if "s_chapter_view_index" not in symbols:
        parser.error("out/game has no demo chapter symbols; build the demo P1 first")
    env = os.environ.copy()
    env.update(DEMO_AUDIO_DIR=str(output), DEMO_CHAPTER_ADDRESS=symbols["s_chapter_view_index"],
               DEMO_AUDIO_PRESS_EVERY=str(args.press_every), DEMO_AUDIO_STOP_CHAPTER=str(args.stop_chapter),
               DEMO_AUDIO_TRACE_Z80="1" if args.trace_z80 else "0",
               DEMO_AUDIO_COIN_AT=str(args.coin_at))
    if os.name != "nt":
        env.update(DISPLAY="", SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    command = [args.mame, "neogeo", "-noreadconfig", "-rompath", str(root / "roms"),
               "-hashpath", ";".join(str(root / p) for p in ("hash_eagle/demo", "hash_eagle", "hash")),
               "-bios", "euro", "-cart1", "demo", "-video", "none", "-sound", "none",
               "-nothrottle", "-seconds_to_run", str(args.seconds), "-skip_gameinfo", "-nonvram_save",
               "-samplerate", "48000", "-wavwrite", str(output / "sound.wav"),
               "-cfg_directory", str(output / "cfg"), "-nvram_directory", str(output / "nvram"),
               "-autoboot_delay", "0", "-autoboot_script", str(root / "tools/demo_audio_capture.lua")]
    with (output / "mame.log").open("w", encoding="utf-8") as log:
        subprocess.run(command, cwd=root, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
    if "[LUA ERROR]" in (output / "mame.log").read_text(encoding="utf-8", errors="replace"):
        raise SystemExit(f"Capture script failed; inspect {output / 'mame.log'}")
    print(f"Live audio capture: {output}")


if __name__ == "__main__":
    main()
