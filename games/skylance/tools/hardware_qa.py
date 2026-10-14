"""Check the built Sky Lance cartridge's palettes, audio and live sprite load.

Use a private output directory visible to Windows MAME. No installed game or
BIOS is replaced. --bios-roms must contain the user's own system ROMs.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import wave
import zipfile

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
PARTS = ("p1", "s1", "m1", "v1", "c1", "c2")


def check(output, seconds, attract_only=False):
    rows = list(csv.DictReader((output / "hardware.csv").open()))
    if not rows or float(rows[-1]["seconds"]) < seconds - 1:
        raise ValueError("Hardware observer did not run to completion")
    settled = [r for r in rows if float(r["seconds"]) > 15]
    peak = max(int(r["peak_strips"]) for r in rows)
    run = longest = 0
    for row in rows:
        run = run + 1 if int(row["all_dirty"]) else 0
        longest = max(run, longest)
    if peak > 96:
        raise ValueError(f"Live LSPC scanline load exceeds 96: {peak}")
    if longest > 30:
        raise ValueError(f"Atomic palette upload starved for {longest} hardware frames")
    if not settled or not any(int(r["terrain_color"], 16) for r in settled):
        raise ValueError("Terrain palette remained black")
    states = list(csv.DictReader((output / "state.csv").open()))
    playing = [r for r in states if int(r["player"], 16) and int(r["energy"]) > 0]
    if not attract_only and not playing:
        raise ValueError("No credited gameplay observed")
    if any(int(r["vram_duplicates"]) for r in states):
        raise ValueError("Duplicate deferred sprite queue entries")
    if any((int(r["face_x"]), int(r["face_y"])) != (288, 44)
           for r in playing if int(r["face_x"]) or int(r["face_y"])):
        raise ValueError("Cockpit portrait left its anchor")
    with wave.open(str(output / "audio.wav")) as stream:
        if stream.getsampwidth() != 2:
            raise ValueError("Expected signed 16-bit PCM capture")
        samples = np.frombuffer(stream.readframes(stream.getnframes()), dtype="<i2").astype(float)
        rms = float(np.sqrt(np.mean(samples * samples)))
    if rms < 100:
        raise ValueError("Captured sound is silent or nearly silent")
    clouds = [r for r in rows if int(r["cloud_x"]) != -32768]
    moving_clouds = [r for r in clouds if abs(int(r["camera_x"])) > 128]
    if any(int(r["cloud_x"]) not in (96, 192) for r in clouds):
        raise ValueError("Overhead cloud followed the lateral camera")
    visible = []
    for r in playing:
        p = output / f't{float(r["seconds"]):05.1f}.png'
        if p.exists():
            rgb = np.asarray(Image.open(p).convert("RGB"))
            if float((rgb.max(axis=2) > 12).mean()) > .70:
                visible.append(p.name)
    if playing and not visible:
        raise ValueError("Gameplay screenshots remained mostly black")
    return {"hardware_frames": len(rows), "peak_scanline_strips": peak,
            "longest_pending_palette_frames": longest, "gameplay_samples": len(playing),
            "audio_rms": rms, "cloud_samples": len(clouds),
            "cloud_samples_during_camera_motion": len(moving_clouds),
            "irq_lines_min_after_boot": min(int(r["irq_lines"]) for r in settled),
            "visible_gameplay": visible}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--mame", type=Path, required=True)
    ap.add_argument("--bios-roms", type=Path, required=True)
    ap.add_argument("--bios", default="euro")
    ap.add_argument("--platform", choices=("aes", "mvs"), default="mvs")
    ap.add_argument("--output", type=Path, required=True)
    ap.add_argument("--seconds", type=int, default=80)
    ap.add_argument("--stress", action="store_true")
    ap.add_argument("--attract-only", action="store_true")
    ap.add_argument("--stage", type=int, choices=range(7))
    ap.add_argument("--pilot", type=int, choices=range(3), default=0)
    ap.add_argument("--m68k-prefix", default="/mnt/c/neogeo/x-tools-v3/m68k-unknown-elf/bin/m68k-unknown-elf-")
    args = ap.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    (out / "roms").mkdir()
    (out / "hash").mkdir()
    elf = ROOT / "out/game"
    parts = {k: (ROOT / f"roms/skylance/779-{k}.{k}").read_bytes() for k in PARTS}
    subprocess.run([args.m68k_prefix + "objcopy", "-O", "binary", str(elf), str(out / "decoded.bin")], check=True)
    native = bytearray(parts["p1"])
    native[0::2], native[1::2] = parts["p1"][1::2], parts["p1"][0::2]
    encoded = (out / "decoded.bin").read_bytes()
    if encoded != native[:len(encoded)]:
        raise ValueError("ELF does not correspond to program ROM")
    (out / "decoded.bin").unlink()
    shutil.copy2(elf, out / "game")
    with zipfile.ZipFile(out / "roms/skylance.zip", "x", zipfile.ZIP_DEFLATED) as z:
        for key, data in parts.items():
            z.writestr(f"779-{key}.{key}", data)
    shutil.copy2(ROOT / "hash_eagle/skylance/neogeo.xml", out / "hash/neogeo.xml")
    symbols = {}
    for line in subprocess.check_output([args.m68k_prefix + "nm", str(elf)], text=True).splitlines():
        fields = line.split()
        if len(fields) == 3:
            symbols[fields[2]] = int(fields[0], 16)
    obj, raw = out / "layout.o", out / "layout.bin"
    subprocess.run([args.m68k_prefix + "gcc", "-c", "-O2", "-ffreestanding", "-I", str(ROOT),
                    "-I", str(ROOT / "sdk"), str(Path(__file__).with_name("qa_layout.c")), "-o", str(obj)], check=True)
    subprocess.run([args.m68k_prefix + "objcopy", "-O", "binary", "-j", ".rodata", str(obj), str(raw)], check=True)
    layout = struct.unpack(">16H", raw.read_bytes())
    windows = str(args.mame).lower().endswith(".exe")

    def path(p):
        if windows:
            return subprocess.check_output(["wslpath", "-w", str(p)], text=True).strip().replace("\\", "/")
        return str(p)

    config = "{output=" + json.dumps(path(out)) + ",symbols={" + ",".join(
        "[" + json.dumps(k) + "]=" + str(v) for k, v in symbols.items()) + "},layout={" + ",".join(map(str, layout)) + "}"
    config += ",pilot=" + str(args.pilot)
    if args.stage is not None:
        config += ",stage=" + str(args.stage)
    for flag in ("stress", "attract_only"):
        if getattr(args, flag):
            config += "," + flag + "=true"
    config += "}"
    lua = out / "capture.lua"
    lua.write_text(Path(__file__).with_name("hardware_capture.lua").read_text().replace("__SKY_QA_CONFIG__", config))
    command = [str(args.mame), "aes" if args.platform == "aes" else "neogeo", "-noreadconfig",
               "-rompath", path(out / "roms") + ";" + path(args.bios_roms),
               "-hashpath", path(out / "hash"), "-bios", args.bios, "-cart1", "skylance",
               "-video", "none", "-sound", "none", "-nothrottle", "-seconds_to_run", str(args.seconds),
               "-skip_gameinfo", "-nonvram_save", "-cfg_directory", path(out / "cfg"),
               "-nvram_directory", path(out / "nvram"), "-autoboot_delay", "0", "-autoboot_script", path(lua),
               "-wavwrite", path(out / "audio.wav")]
    manifest = {"platform": args.platform, "bios": args.bios, "bios_path": str(args.bios_roms),
                "stress": args.stress, "seconds": args.seconds,
                "elf_sha256": hashlib.sha256(elf.read_bytes()).hexdigest(),
                "rom_sha256": {k: hashlib.sha256(v).hexdigest() for k, v in parts.items()}}
    (out / "run-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    with (out / "mame.log").open("w") as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=max(120, args.seconds * 6))
    report = check(out, args.seconds, args.attract_only)
    (out / "result.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
