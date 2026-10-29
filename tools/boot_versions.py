#!/usr/bin/env python3
"""
Boot every ROM version installed by tools/install_rom_versions.py with the
Linux MAME in WSL, one at a time, using the test folder's own
tools/boot_check.lua. The results land where run_all_tests.bat puts them, so
generate_qa_report.bat reads either:

  reports/boot/<set>_<version>_<mode>.txt    facts from the run and MAME's exit code
  screenshots/<set>_<version>_<mode>/boot_06s.png, boot_26s.png

Linux MAME needs WSLg's X display (DISPLAY=:0) even with -video none; an
inherited DISPLAY pointing elsewhere stalls each run for minutes. It runs
here instead of the Windows build because its memory stays inside
WSL: a Windows MAME run reserves enough memory to grow the page file on a
full C: drive.

    python3 tools/boot_versions.py [--target /mnt/c/mame/neogeosdk] [--only skylance] [--mame mame]
"""

import argparse
import json
import os
import shutil
import subprocess
import tempfile
import sys
import time
from pathlib import Path


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--target", type=Path, default=Path("/mnt/c/mame/neogeosdk"))
    ap.add_argument("--only", action="append", default=[], help="set name or launcher prefix to run (repeatable)")
    ap.add_argument("--mame", default="mame")
    ap.add_argument("--seconds", type=int, default=300, help="wall-clock limit per run")
    args = ap.parse_args()
    t = args.target
    versions = json.loads((t / "manifests" / "versions.json").read_text())["versions"]
    if args.only:
        versions = [v for v in versions if v["set"] in args.only or any(v["launcher"].startswith(o) for o in args.only)]
    # WSLg's display: an inherited DISPLAY (a Windows-side X server that is not
    # running) makes MAME wait about 270 s before it starts.
    env = dict(os.environ, DISPLAY=":0")
    (t / "reports" / "boot").mkdir(parents=True, exist_ok=True)
    failures = 0
    for v in versions:
        tag = v["launcher"][:-4]
        vdir = t / "versions" / v["set"] / v["dir"] / v["mode"]
        shots = t / "screenshots" / tag
        shots.mkdir(parents=True, exist_ok=True)
        report = t / "reports" / "boot" / f"{tag}.txt"
        report.unlink(missing_ok=True)
        # The snap MAME runs about 50 times slower with its cfg, nvram and
        # snapshot folders on /mnt/c: run in RAM, copy the results after.
        work = Path(tempfile.mkdtemp(prefix="ngsdk-boot-", dir="/dev/shm"))
        cmd = [args.mame, "aes" if v["mode"] == "aes" else "neogeo", "-noreadconfig", "-skip_gameinfo",
               "-rompath", f"{vdir};{t / 'roms'}", "-hashpath", str(vdir / "hash"),
               "-bios", "asia" if v["mode"] == "aes" else "euro", "-cart1", v["game"],
               "-cfg_directory", str(work / "cfg"), "-nvram_directory", str(work / "nvram"),
               "-noautosave", "-video", "none", "-sound", "none", "-nothrottle",
               "-seconds_to_run", "40", "-autoboot_script", str(t / "tools" / "boot_check.lua")]
        if v["mode"] == "aes":
            card = work / "memcard.bin"
            if not card.exists():
                card.write_bytes(bytes(2048))
            cmd[cmd.index("-noautosave"):cmd.index("-noautosave")] = ["-memc", str(card)]
        run_env = dict(env, NGSDK_BOOT_REPORT=str(work / "report.txt"), NGSDK_BOOT_SHOTS=str(work), NGSDK_BOOT_MODE=v["mode"])
        start = time.time()
        try:
            with open(work / "mame.log", "w") as mlog:
                rc = subprocess.run(cmd, env=run_env, stdin=subprocess.DEVNULL, stdout=mlog, stderr=subprocess.STDOUT,
                                    timeout=args.seconds, cwd=str(Path.home())).returncode
        except subprocess.TimeoutExpired:
            rc = "timeout"
        out = (work / "mame.log").read_text(errors="replace") if (work / "mame.log").exists() else ""
        for png in work.glob("*.png"):
            shutil.copy(png, shots / png.name)
        if (work / "report.txt").exists():
            shutil.copy(work / "report.txt", report)
        shutil.rmtree(work, ignore_errors=True)
        log = t / "logs" / f"{tag}.log"
        log.parent.mkdir(exist_ok=True)
        with log.open("a") as f:
            f.write(f"==== {time.strftime('%Y-%m-%d %H:%M:%S')} boot (Linux MAME, tools/boot_versions.py)\n{out}\nexit {rc}\n")
        with report.open("a") as f:
            f.write(f"mame_exit={rc}\nrunner=linux-mame\nsnapshots={shots}\n")
        facts = dict(line.split("=", 1) for line in report.read_text().splitlines() if "=" in line)
        ok = rc == 0 and facts.get("main_cpu") == "RUNNING" and facts.get("audio_cpu") == "RUNNING" and facts.get("video") == "WRITTEN"
        failures += not ok
        print(f"{tag:34} exit {rc!s:8} main {facts.get('main_cpu', '-'):8} z80 {facts.get('audio_cpu', '-'):8} "
              f"video {facts.get('video', '-'):8} {time.time() - start:5.1f}s", flush=True)
    print(f"{len(versions) - failures} of {len(versions)} booted with both CPUs running and video written")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
