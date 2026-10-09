"""Capture the installed cartridge on Windows MAME, including Z80 state."""

import argparse
import csv
import json
from pathlib import Path
import subprocess
import wave

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
    parser.add_argument("--attract-only", action="store_true", help="Capture the title without inserting credits")
    args = parser.parse_args()
    manifest = json.loads((args.target / "skylance-build.json").read_text())
    data = args.target / "tests/skylance" / args.platform
    label = f"{args.platform}-{args.bios}" + (f"-sector-{args.stage + 1}" if args.stage is not None else "")
    if args.attract_only:
        label += "-attract"
    output = args.target / "tests/skylance/qa" / label
    output.mkdir(parents=True, exist_ok=True)
    symbols = {}
    for line in (data / "symbols.txt").read_text().splitlines():
        parts = line.split()
        if len(parts) == 3:
            symbols[parts[2]] = int(parts[0], 16)
    names = ("s_player", "s_stage", "s_energy", "s_invuln", "s_plane_pose", "s_camera")
    config = '{output=' + json.dumps(windows(output)) + ',symbols={'
    config += ','.join(f'{name}={symbols[name]}' for name in names) + '}'
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
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    with (output / "state.csv").open() as stream:
        rows = list(csv.DictReader(stream))
    gameplay = [r for r in rows if int(r["player"], 16) and int(r["energy"]) > 0]
    if not gameplay and not args.attract_only:
        raise RuntimeError(f"Did not reach gameplay: {output}")
    if not all(r["protocol"] == "4e475331" and int(r["slot_wait"]) == 0
               and int(r["timer"]) & 2 for r in gameplay):
        raise RuntimeError("Z80 protocol did not remain active during play")
    rendered = []
    title_reference = np.asarray(Image.open(args.target / "previews/skylance/title.png").convert("RGB"))
    title_frames = []
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
    if args.attract_only and not title_frames:
        raise RuntimeError(f"The new title did not render: {output}")
    if not args.attract_only and len(rendered) < 3:
        raise RuntimeError(f"Gameplay graphics are blank or incomplete: {output}")
    with wave.open(str(output / "audio.wav")) as wav:
        samples = np.frombuffer(wav.readframes(wav.getnframes()), dtype="<i2").astype(np.int32)
        audio_summary = {"frames": wav.getnframes(), "rate": wav.getframerate(),
                         "peak": int(np.abs(samples).max()),
                         "rms": float(np.sqrt(np.mean(samples.astype(np.float64) ** 2)))}
    if audio_summary["peak"] == 0:
        raise RuntimeError("Captured audio is silent")
    result = {"label": label, "built_utc": manifest["built_utc"],
              "roms": manifest["roms"][args.platform],
              "gameplay_samples": len(gameplay), "rendered_frames": rendered, "audio": audio_summary,
              "title_frames": title_frames,
              "stage_override": args.stage, "last_state": rows[-1]}
    (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
