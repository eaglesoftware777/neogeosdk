"""Headless movement, climbing, arena and bonus checks using ELF debug symbols."""

import argparse
import json
import os
from pathlib import Path
import subprocess

from build import GAME, ROOT, WORK


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seconds", type=int, default=50)
    parser.add_argument("--mame", default="mame")
    parser.add_argument("--scenario", choices=("idle", "walk", "climb", "boss", "bonus"), default="walk")
    parser.add_argument("--idle", action="store_true", help="Capture startup without gameplay inputs")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--eagle-bios", action="store_true")
    args = parser.parse_args()
    if args.idle:
        args.scenario = "idle"
    output = (args.output or GAME / "build" / args.scenario).resolve()
    output.mkdir(parents=True, exist_ok=True)
    gdb = next(ROOT.parent.glob("x-tools-v*/m68k-unknown-elf/bin/m68k-unknown-elf-gdb"))
    fields = {name: "&mg." + name for name in (
        "stage", "state", "player", "boss", "boss_active", "state_timer",
        "gate_unlocked", "climbing", "demo", "shake_x")}
    fields["camera_x"] = "&mg.camera.x"
    fields.update({"char_" + name: f"&((NGCharacter*)0)->{name}"
                   for name in ("x", "y", "x_fp", "y_fp", "hp", "flip_x")})
    command = [str(gdb), "-batch", str(WORK / "out/game")]
    for name, expr in fields.items():
        command += ["-ex", f'printf "{name}=%lu\\n", (unsigned long){expr}']
    layout = dict(line.split("=", 1) for line in subprocess.check_output(command, text=True).splitlines() if "=" in line)
    (output / "layout.lua").write_text("return {" + ",".join(f"{key}={int(value)}" for key, value in layout.items()) + "}\n")
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", DISPLAY="",
               MG_CAPTURE_DIR=str(output), MG_SCENARIO=args.scenario)
    bios_path = str(ROOT / "bios/test_roms") + ";" if args.eagle_bios else ""
    command = [args.mame, "neogeo", "-noreadconfig", "-rompath",
               f"{bios_path}{WORK / 'roms'};{ROOT / 'roms'}", "-hashpath",
               str(WORK / "hash_eagle/maiya"), "-cart1", "maiya", "-bios", "euro",
               "-video", "none", "-sound", "none", "-nothrottle", "-nonvram_save",
               "-seconds_to_run", str(args.seconds), "-skip_gameinfo",
               "-cfg_directory", str(output / "cfg"), "-nvram_directory", str(output / "nvram"),
               "-snapshot_directory", str(output), "-autoboot_delay", "0",
               "-autoboot_script", str(GAME / "tools/regression.lua")]
    with (output / "mame.log").open("w") as log:
        subprocess.run(command, env=env, cwd=output, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=300)
    samples = [json.loads(line) for line in (output / "telemetry.jsonl").read_text().splitlines()]
    active = [s for s in samples if s["mode"] == 2 and s["player"]]
    assert active, "The cartridge never entered play"
    if args.scenario != "idle":
        assert max(s["x"] for s in active) > min(s["x"] for s in active), "Player did not move"
    if args.scenario == "climb":
        climbing = [s for s in active if s["climbing"]]
        assert climbing and max(s["y"] for s in climbing) - min(s["y"] for s in climbing) > 50
        assert len({s["flip"] for s in climbing}) == 1, "Climbing unexpectedly reverses facing"
    elif args.scenario == "boss":
        arena = [s for s in active if s["boss_active"] and s["boss"]]
        assert arena, "Guardian never spawned"
        assert max(s["boss_x"] for s in arena) - min(s["boss_x"] for s in arena) > 40, "Guardian does not walk"
        assert all(3540 <= s["x"] <= 3820 for s in arena), "Player escaped arena"
        settled = arena[4:]
        assert settled and all(s["camera_x"] - s["shake_x"] == 3520 for s in settled), "Arena camera did not lock"
    elif args.scenario == "bonus":
        bonus = [s for s in active if s["state"] == 3]
        assert bonus and all(s["camera_x"] == 0 for s in bonus), "Bonus reused the previous arena"
        assert any(s["stage"] == 2 and s["state"] == 1 for s in active), "Bonus did not advance to mission 3"
    assert "LUA ERROR" not in (output / "mame.log").read_text(), "Capture script failed"
    print(f"PASS {args.scenario}: {output}")


if __name__ == "__main__":
    main()
