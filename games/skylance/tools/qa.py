"""Capture the installed cartridge on Windows MAME, including Z80 state."""

import argparse
import csv
import json
from pathlib import Path
import subprocess
import wave
import struct
import os
import shutil

import numpy as np
from PIL import Image

def windows(path):
    return subprocess.check_output(["wslpath", "-w", str(path)], text=True).strip().replace("\\", "/")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", type=Path, default=Path("/mnt/c/mame/neogeosdk"))
    parser.add_argument("--platform", choices=("aes", "mvs"), default="mvs")
    parser.add_argument("--bios", choices=("eagle", "stock", "unibios40"), default="eagle")
    parser.add_argument("--stage", type=int, choices=range(7))
    parser.add_argument("--seconds", type=int, default=45)
    parser.add_argument("--pilot", type=int, choices=range(3), default=0)
    parser.add_argument("--stress", action="store_true", help="Force maximum resources in inspection runs")
    parser.add_argument("--attract-only", action="store_true", help="Capture the title without inserting credits")
    parser.add_argument("--boss", action="store_true", help="Skip waves in the inspection run to capture the stage boss")
    parser.add_argument("--features", action="store_true", help="Capture held banking, B maneuvers and C bomb strikes")
    parser.add_argument("--damage", action="store_true", help="Inject collisions to exercise tint, immunity and respawn")
    parser.add_argument("--encounters", action="store_true", help="Aim at defenders, then exposed targets; verify three stop/resume encounters")
    parser.add_argument("--radar-test", action="store_true", help="Enter radar coverage during encounter inspection")
    parser.add_argument("--fuel-out", action="store_true", help="Seed two seconds of fuel and verify mission failure")
    args = parser.parse_args()
    manifest = json.loads((args.target / "skylance-build.json").read_text())
    data = args.target / "tests/skylance" / args.platform
    label = f"{args.platform}-{args.bios}" + (f"-sector-{args.stage + 1}" if args.stage is not None else "")
    if args.attract_only:
        label += "-attract"
    if args.boss:
        label += "-boss"
    if args.damage:
        label += f"-damage-pilot-{args.pilot}"
    if args.features:
        label += f"-features-pilot-{args.pilot}"
    if args.stress:
        label += f"-pilot-{args.pilot}-stress"
    if args.encounters:
        label += "-encounters" + ("-radar" if args.radar_test else "-quiet")
    if args.fuel_out:
        label += "-fuel-out"
    output = args.target / "tests/skylance/qa" / manifest["built_utc"] / label
    output.mkdir(parents=True, exist_ok=True)
    symbols = {}
    for line in (data / "symbols.txt").read_text().splitlines():
        parts = line.split()
        if len(parts) == 3:
            symbols[parts[2]] = int(parts[0], 16)
    names = ("s_player", "s_stage", "s_energy", "s_invuln", "s_plane_pose", "s_camera",
             "s_pick", "s_resources", "s_frames", "sound_queue_head", "sound_queue_tail",
             "s_waves_left", "s_squad_left", "s_wave_timer", "s_lives", "ng_chars",
             "ngsg_high_n", "ngsg_norm_n", "ngsg_low_n", "ngsg_high", "ngsg_norm", "ngsg_low",
             "s_roll", "s_roll_maneuver", "s_loop_maneuver", "s_boss_health", "s_boss_hp_max",
             "s_targets_destroyed", "s_ground_page", "s_hud_face", "s_encounter",
             "s_radar_alert", "s_chaff_stock", "s_decoyed_missiles")
    config = '{output=' + json.dumps(windows(output)) + ',symbols={'
    config += ','.join(f'{name}={symbols[name]}' for name in names) + '}'
    config += f',pilot={args.pilot}'
    if args.damage or args.encounters:
        root = Path(__file__).resolve().parents[3]
        gcc = os.environ.get("M68K_CC") or shutil.which("m68k-unknown-elf-gcc") or \
            "/mnt/c/neogeo/x-tools-v2/m68k-unknown-elf/bin/m68k-unknown-elf-gcc"
        obj = output / "layout.o"
        raw = output / "layout.bin"
        subprocess.run([gcc, "-c", "-O2", "-ffreestanding", "-I", str(root),
                        "-I", str(root / "sdk"), str(Path(__file__).with_name("qa_layout.c")),
                        "-o", str(obj)], check=True)
        subprocess.run([gcc.replace("gcc", "objcopy"), "-O", "binary", "-j", ".rodata",
                        str(obj), str(raw)], check=True)
        layout = struct.unpack(">16H", raw.read_bytes())
        config += ',layout={' + ','.join(map(str, layout)) + '}'
    if args.damage:
        config += ',damage=true'
    if args.encounters:
        config += ',encounters=true'
    if args.radar_test:
        config += ',radar_test=true'
    if args.fuel_out:
        config += ',fuel_out=true'
    if args.features:
        config += ",features=true"
    if args.boss:
        config += ',boss=true'
    if args.stress:
        config += ',stress=true'
    if args.stage is not None:
        config += f',stage={args.stage}'
    if args.attract_only:
        config += ',attract_only=true'
    config += '}'
    script = output / "capture.lua"
    script.write_text((Path(__file__).with_suffix(".lua")).read_text().replace("__SKY_QA_CONFIG__", config), encoding="ascii")
    roms = [data / "roms", args.target / "roms", args.target.parent / "roms"]
    if args.bios == "eagle":
        roms.insert(0, args.target / "tests/skylance/bios")
    command = [str(args.target.parent / "mame.exe"), "aes" if args.platform == "aes" else "neogeo",
               "-noreadconfig", "-rompath", ";".join(windows(p) for p in roms),
               "-hashpath", windows(data / "hash"), "-bios",
               "unibios40" if args.bios == "unibios40" else "asia" if args.platform == "aes" else "euro",
               "-cart1", "skylance", "-video", "none", "-sound", "none", "-nothrottle",
               "-seconds_to_run", str(args.seconds), "-skip_gameinfo", "-nonvram_save",
               "-cfg_directory", windows(output / "cfg"), "-nvram_directory", windows(output / "nvram"),
               "-autoboot_delay", "0", "-autoboot_script", windows(script),
               "-wavwrite", windows(output / "audio.wav")]
    with (output / "mame.log").open("w") as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True,
                       timeout=max(120, args.seconds * 6))
    with (output / "state.csv").open() as stream:
        rows = list(csv.DictReader(stream))
    gameplay = [r for r in rows if int(r["player"], 16) and int(r["energy"]) > 0]
    if not gameplay and not args.attract_only:
        raise RuntimeError(f"Did not reach gameplay: {output}")
    if not all(r["protocol"] == "4e475331" and int(r["slot_wait"]) == 0
               and int(r["timer"]) & 2 for r in gameplay):
        raise RuntimeError("Z80 protocol did not remain active during play")
    if args.damage and any(not int(r["visible"]) for r in gameplay):
        raise RuntimeError("Player visibility was disabled during damage inspection")
    if any(int(r["vram_duplicates"]) for r in gameplay):
        raise RuntimeError("A sprite group was inserted into multiple pending queue entries")
    if any((int(r["face_x"]), int(r["face_y"])) != (304, 44)
           for r in gameplay if int(r["face_x"]) or int(r["face_y"])):
        raise RuntimeError("Cockpit portrait moved from its fixed anchor")
    rendered = []
    title_reference = np.asarray(Image.open(args.target / "previews/skylance/title.png").convert("RGB"))
    title_frames = []
    plane_frames = []
    roster = json.loads((args.target / "previews/skylance/assets.json").read_text())
    for row in rows if args.attract_only else gameplay:
        snapshot = output / f't{float(row["seconds"]):05.1f}.png'
        if not snapshot.is_file():
            continue
        pixels = np.asarray(Image.open(snapshot).convert("RGB"))
        coverage = float((pixels.max(axis=2) > 12).mean())
        if args.attract_only and pixels.shape == title_reference.shape:
            error = np.abs(pixels[38:78, 12:308].astype(np.int32) -
                           title_reference[38:78, 12:308].astype(np.int32)).mean()
            if error < 25:
                title_frames.append(snapshot.name)
        if coverage > 0.70 and len(np.unique(pixels.reshape(-1, 3), axis=0)) > 50:
            rendered.append(snapshot.name)
        if args.damage and int(row["hits"]) == 3 and int(row["visible"]) and not int(row["invuln"]):
            pose = int(row["pose"])
            craft = np.asarray(Image.open(args.target / "previews/skylance" /
                                          (roster[pose-1]["name"] + ".png")).convert("RGBA"))
            x, y = int(row["player_x"])-24, int(row["player_y"])-24
            if x >= 0 and y >= 0 and x+48 <= pixels.shape[1] and y+48 <= pixels.shape[0]:
                opaque = craft[:, :, 3] > 0
                difference = np.abs(pixels[y:y+48, x:x+48].astype(np.int32)-
                                    craft[:, :, :3].astype(np.int32)).max(axis=2)
                # Critical-hull tint blends at most 32/255 toward red;
                # allow its bounded shift plus hardware colour rounding.
                tolerance = 40 if int(row["energy"]) <= 34 else 8
                match = float((difference[opaque] <= tolerance).mean())
                if match < .4:
                    raise RuntimeError(f"Visible aircraft missing from snapshot {snapshot}: {match:.2%}")
                plane_frames.append({"snapshot": snapshot.name, "match": match,
                                     "hits": int(row["hits"])})
    if args.attract_only and not title_frames:
        raise RuntimeError(f"The new title did not render: {output}")
    if not args.attract_only and len(rendered) < 3:
        raise RuntimeError(f"Gameplay graphics are blank or incomplete: {output}")
    if args.damage and (int(rows[-1]["hits"]) < 3 or
                        len([p for p in plane_frames if p["hits"] == 3]) < 3):
        raise RuntimeError("Damage test did not verify recovery after three collisions")
    with wave.open(str(output / "audio.wav")) as wav:
        samples = np.frombuffer(wav.readframes(wav.getnframes()), dtype="<i2").astype(np.int32)
        audio_summary = {"frames": wav.getnframes(), "rate": wav.getframerate(),
                         "peak": int(np.abs(samples).max()),
                         "rms": float(np.sqrt(np.mean(samples.astype(np.float64) ** 2)))}
    if audio_summary["peak"] == 0:
        raise RuntimeError("Captured audio is silent")
    feature_summary = {}
    if args.features:
        feature_summary = {
            "maximum_roll_degrees": max(abs(int(r["roll_angle"]))/16 for r in gameplay),
            "maneuver_samples": sum(bool(int(r["roll_move"]) or int(r["climb_move"])) for r in gameplay),
            "bomb_samples": sum(int(r["special"]) > 0 for r in gameplay),
            "terrain_pages": sorted({int(r["terrain_page"]) for r in gameplay}),
            "maximum_boss_health": max(int(r["boss_health"]) for r in gameplay),
            "maximum_command_targets": max(int(r["targets"]) for r in gameplay)}
        if feature_summary["maximum_roll_degrees"] < 40:
            raise RuntimeError("Sustained banking failed to reach 40 degrees")
        if not feature_summary["maneuver_samples"] or not feature_summary["bomb_samples"]:
            raise RuntimeError("Did not capture B maneuver and C bomb")
    encounter_summary = {}
    if args.encounters:
        stopped = [r for r in gameplay if int(r['encounter']) == 2]
        exposed = [r for r in gameplay if int(r['encounter']) == 3]
        encounter_summary = {'stopped_samples': len(stopped), 'exposed_samples': len(exposed),
            'targets': max(int(r['targets']) for r in gameplay),
            'radar_alert_samples': sum(int(r['radar_alert']) for r in gameplay),
            'decoyed_missiles': max(int(r['decoys']) for r in gameplay),
            'minimum_chaff_stock': min(int(r['chaff_stock']) for r in gameplay)}
        if not stopped or not exposed or encounter_summary['targets'] != 3:
            raise RuntimeError(f'Three checkpoint encounters did not complete: {encounter_summary}')
        if not encounter_summary['decoyed_missiles']:
            raise RuntimeError('Chaff did not divert a guided missile')
        if bool(encounter_summary['radar_alert_samples']) != args.radar_test:
            raise RuntimeError('Radar detection did not match the inspection route')
        for a,b in zip(rows, rows[1:]):
            if a['encounter'] == b['encounter'] == '2' and a['travel'] != b['travel']:
                raise RuntimeError('Terrain scrolled during a defended checkpoint')
    if args.fuel_out and not any(int(r['fuel']) == 0 and int(r['player'],16) for r in rows):
        raise RuntimeError('Fuel exhaustion did not occur')
    result = {"features": feature_summary, "encounters": encounter_summary, "label": label, "built_utc": manifest["built_utc"],
              "roms": manifest["roms"][args.platform],
              "gameplay_samples": len(gameplay), "rendered_frames": rendered, "audio": audio_summary,
              "title_frames": title_frames,
              "verified_aircraft": plane_frames,
              "stage_override": args.stage, "last_state": rows[-1]}
    (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
