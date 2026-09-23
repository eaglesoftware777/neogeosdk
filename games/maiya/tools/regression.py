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
    parser.add_argument("--scenario", choices=("idle", "walk", "climb", "boss", "bonus", "continue",
                                              "continue-exit", "continue-timeout", "tray", "factory",
                                              "pickups", "pit"), default="walk")
    parser.add_argument("--idle", action="store_true", help="Capture startup without gameplay inputs")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--eagle-bios", action="store_true")
    parser.add_argument("--platform", choices=("mvs", "aes"), default="mvs")
    args = parser.parse_args()
    marker = WORK / "platform.txt"
    built_platform = marker.read_text().strip() if marker.exists() else "mvs"
    if built_platform != args.platform:
        parser.error(f"Build with --platform {args.platform} first")
    if args.scenario == "continue-exit" and args.platform != "aes":
        parser.error("The Exit choice is only offered on AES")
    if args.idle:
        args.scenario = "idle"
    output = (args.output or GAME / "build" / args.scenario).resolve()
    output.mkdir(parents=True, exist_ok=True)
    gdb = next(ROOT.parent.glob("x-tools-v*/m68k-unknown-elf/bin/m68k-unknown-elf-gdb"))
    fields = {name: "&mg." + name for name in (
        "stage", "state", "player", "boss", "boss_active", "state_timer",
        "gate_unlocked", "climbing", "demo", "shake_x", "lives", "continues",
        "has_key", "next_stage", "art", "coins", "flowers", "critters",
        "swift", "might", "veil", "spring", "crown", "items", "pick_mask", "hud_dirty")}
    fields["camera_x"] = "&mg.camera.x"
    # BIOS_PLAYER1_MODE (0x10FDB6), the same address regression.lua already
    # reads as "player_mode": Maiya no longer keeps a Start-press counter of
    # her own, since the BIOS's own mode flag is the live, race-free signal.
    fields["start_count"] = "0x10FDB6"
    fields.update({"char_" + name: f"&((NGCharacter*)0)->{name}"
                   for name in ("x", "y", "x_fp", "y_fp", "hp", "flip_x", "data0")})
    fields.update({"item_" + name: f"&((MGItem*)0)->{name}" for name in ("life", "key", "source")})
    fields["item_size"] = "sizeof(MGItem)"
    command = [str(gdb), "-batch", str(WORK / "out/game")]
    for name, expr in fields.items():
        command += ["-ex", f'printf "{name}=%lu\\n", (unsigned long){expr}']
    layout = dict(line.split("=", 1) for line in subprocess.check_output(command, text=True).splitlines() if "=" in line)
    (output / "layout.lua").write_text("return {" + ",".join(f"{key}={int(value)}" for key, value in layout.items()) + "}\n")
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", DISPLAY="",
               MG_CAPTURE_DIR=str(output), MG_SCENARIO=args.scenario, MG_PLATFORM=args.platform)
    bios_path = str(ROOT / "bios/test_roms") + ";" if args.eagle_bios else ""
    command = [args.mame, "aes" if args.platform == "aes" else "neogeo", "-noreadconfig", "-rompath",
               f"{bios_path}{WORK / 'roms'};{ROOT / 'roms'}", "-hashpath",
               str(WORK / "hash_eagle/maiya"), "-cart1", "maiya", "-bios", "asia" if args.platform == "aes" else "euro",
               "-video", "none", "-sound", "none", "-nothrottle", "-nonvram_save",
               "-seconds_to_run", str(args.seconds), "-skip_gameinfo",
               "-cfg_directory", str(output / "cfg"), "-nvram_directory", str(output / "nvram"),
               "-snapshot_directory", str(output), "-autoboot_delay", "0",
               "-autoboot_script", str(GAME / "tools/regression.lua")]
    with (output / "mame.log").open("w") as log:
        subprocess.run(command, env=env, cwd=output, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=300)
    samples = [json.loads(line) for line in (output / "telemetry.jsonl").read_text().splitlines()]
    active = [s for s in samples if s["mode"] == 2 and s["player"] and not s["demo"]]
    if args.scenario == "idle":
        active = [s for s in samples if s["player"]]
    assert active, "The cartridge never entered play"
    if args.scenario in ("walk", "climb", "boss", "bonus", "continue"):
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
    elif args.scenario == "continue":
        waiting = [s for s in active if s["state"] == 5 and s["continues"] == 3]
        assert waiting and all(s["player_mode"] == 2 for s in waiting), "BIOS was not told about Continue"
        if args.platform == "mvs":
            assert any(s["credit"] == 1 for s in waiting), "Coin did not reach Continue"
        resumed = [s for s in active if s["continues"] == 2 and s["state"] in (0, 1)]
        assert resumed, "Start did not resume the game"
        first = resumed[0]
        # A continue restarts the whole mission, not the spot she fell: the
        # same head-of-road entrance every fresh arrival uses, and whatever
        # she was carrying goes with the reset, the same as any other retry.
        assert first["x"] == 64 and first["y"] < 192, "Respawn did not return to the start of the mission"
        assert first["lives"] == 3, "Continue must restore three lives"
        if args.platform == "mvs":
            assert first["credit"] == 0, "Continue must spend exactly one credit"
        assert not first["has_key"] and not first["gate_unlocked"], "Continue should reset mission progress"
        assert any(s["state"] == 1 for s in resumed), "Respawn never returned control after landing"
    elif args.scenario in ("continue-exit", "continue-timeout"):
        waiting = [s for s in active if s["state"] == 5]
        assert waiting, "Continue was not offered"
        assert any(s["time"] > waiting[-1]["time"] and
                   (s["player_mode"] == 3 or s["demo"] or s["mode"] != 2)
                   for s in samples), "Exit did not end the player's run"
        assert not any(s["continues"] == 2 for s in active), "Exit spent a continuation"
    elif args.scenario == "factory":
        factory = [s for s in active if s["stage"] == 6 and s["state"] == 1]
        assert factory and any(s["boss_active"] for s in factory), "Factory guardian did not spawn"
        assert any(s["boss_kind"] == 5 for s in factory), "Second factory guardian did not spawn"
    elif args.scenario == "pickups":
        assert any(s["pick_mask"] & 2 for s in active), "Later road pickup never became collectible"
        assert any(s["pick_mask"] & 16 for s in active), "Uncollected shelf pickup was lost on backtracking"
        assert all(s["key_count"] == 1 for s in active if s["state"] == 1), "Mandatory key was discarded or duplicated"
        assert all(s["duplicate_items"] == 0 for s in active), "Map pickup spawned twice"
    elif args.scenario == "pit":
        falls = [s for s in active if s["stage"] == 1]
        assert falls, "Never reached Valley of Falls"
        assert max(s["x"] for s in falls) > 590, "Never walked past the first pit"
        assert any(s["state"] == 4 for s in falls), "Falling into the pit never registered a death"
        assert any(s["lives"] == 2 for s in falls), "Falling into the pit did not cost exactly one life"
    assert "LUA ERROR" not in (output / "mame.log").read_text(), "Capture script failed"
    print(f"PASS {args.scenario}: {output}")


if __name__ == "__main__":
    main()
