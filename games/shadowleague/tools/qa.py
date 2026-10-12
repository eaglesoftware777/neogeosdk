"""Run the built Shadow League cartridge headless in MAME and check a match plays.

    make GAME=shadowleague all            (or PLATFORM=aes)
    python3 games/shadowleague/tools/qa.py [--platform aes] [--seconds 100]
"""

import argparse
import json
import os
import subprocess
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
FIELDS = {"match": "&s_match", "state": "&((NGFMatch*)0)->state", "round": "&((NGFMatch*)0)->round",
          "seconds": "&((NGFMatch*)0)->seconds", "winner": "&((NGFMatch*)0)->winner",
          "frame": "&((NGFMatch*)0)->frame",
          "life0": "&((NGFMatch*)0)->f[0].life", "life1": "&((NGFMatch*)0)->f[1].life",
          "x0": "&((NGFMatch*)0)->f[0].x", "x1": "&((NGFMatch*)0)->f[1].x",
          "st0": "&((NGFMatch*)0)->f[0].state", "st1": "&((NGFMatch*)0)->f[1].state",
          "wins0": "&((NGFMatch*)0)->f[0].wins", "wins1": "&((NGFMatch*)0)->f[1].wins"}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--platform", choices=("mvs", "aes"), default="mvs")
    ap.add_argument("--seconds", type=int, default=100)
    ap.add_argument("--mame", default="mame")
    ap.add_argument("--output", type=Path, default=GAME / "build/qa")
    args = ap.parse_args()
    out = args.output.resolve() / args.platform
    out.mkdir(parents=True, exist_ok=True)
    for old in out.glob("shot_*.png"):
        old.unlink()
    gdb = next(ROOT.parent.glob("x-tools-v*/m68k-unknown-elf/bin/m68k-unknown-elf-gdb"))
    gcc = next(ROOT.parent.glob("x-tools-v3/m68k-unknown-elf/bin/m68k-unknown-elf-gcc"))
    # Field offsets from a debug object built by the cartridge's compiler;
    # the match's address from the cartridge itself.
    probe = out / "layout_probe.c"
    probe.write_text('#include "sdk/2d_engine/ng_fight.h"\nNGFMatch qa_match;\n')
    subprocess.run([str(gcc), "-g", "-c", "-DNG_FIGHT=1", "-I" + str(ROOT), "-I" + str(ROOT / "sdk"),
                    "-I" + str(ROOT / "sdk/2d_engine"), str(probe), "-o", str(out / "layout_probe.o")], check=True)
    layout = {}
    for binary, names in ((ROOT / "out/game", ["match"]), (out / "layout_probe.o", [n for n in FIELDS if n != "match"])):
        cmd = [str(gdb), "-batch", str(binary)]
        for name in names:
            cmd += ["-ex", f'printf "{name}=%lu\\n", (unsigned long){FIELDS[name]}']
        layout.update(line.split("=", 1) for line in subprocess.check_output(cmd, text=True).splitlines() if "=" in line)
    (out / "layout.lua").write_text("return {" + ",".join(f"{k}={int(v)}" for k, v in layout.items()) + "}\n")
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", DISPLAY="",
               SLF_CAPTURE_DIR=str(out), SLF_PLATFORM=args.platform)
    aes = args.platform == "aes"
    command = [args.mame, "aes" if aes else "neogeo", "-noreadconfig", "-rompath",
               f"{ROOT / 'bios/test_roms'};{ROOT / 'roms'}", "-hashpath", str(ROOT / "hash_eagle/shadowleague"),
               "-cart1", "shadowleague", "-bios", "asia" if aes else "euro", "-video", "none", "-sound", "none",
               "-nothrottle", "-nonvram_save", "-seconds_to_run", str(args.seconds), "-skip_gameinfo",
               "-cfg_directory", str(out / "cfg"), "-nvram_directory", str(out / "nvram"),
               "-snapshot_directory", str(out), "-autoboot_delay", "0",
               "-autoboot_script", str(GAME / "tools/qa.lua")]
    with (out / "mame.log").open("w") as log:
        subprocess.run(command, env=env, cwd=out, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=900)
    samples = [json.loads(line) for line in (out / "telemetry.jsonl").read_text().splitlines()]
    fighting = [s for s in samples if s["state"] == 1 and 0 < s["life0"] <= 1000 and 0 < s["life1"] <= 1000]
    assert fighting, "No match reached the fight state"
    assert any(s["life0"] < 1000 or s["life1"] < 1000 for s in fighting), "Nobody was ever hit"
    assert len({s["x0"] for s in fighting}) > 3, "Player 1 never moved"
    assert "LUA ERROR" not in (out / "mame.log").read_text(), "Capture script failed"
    print(f"PASS {args.platform}: {len(fighting)} fight samples, last {samples[-1]}")


if __name__ == "__main__":
    main()
