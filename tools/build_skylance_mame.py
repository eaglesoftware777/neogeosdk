"""Build, verify and install separate Sky Lance AES/MVS test cartridges."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import xml.etree.ElementTree as ET

from pack_neosd import build_image

ROOT = Path(__file__).resolve().parents[1]
PARTS = ("p1", "s1", "m1", "v1", "c1", "c2")
INSTALL_PATHS = ("tests/skylance", "previews/skylance", "skylance-build.json",
                 "run_skylance.bat", "run_skylance_aes.bat", "run_skylance_mvs.bat",
                 "run_skylance_backup.bat", "SKYLANCE-TEST.txt")


def run_make(stage, label, *targets):
    log_path = stage / f"{label}-build.log"
    print(f"Building {label}; log: {log_path}", flush=True)
    with log_path.open("w") as log:
        result = subprocess.run(["make", "GAME=skylance", *targets], cwd=ROOT,
                                stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        print(log_path.read_text()[-12000:])
        raise RuntimeError(f"Build failed: {label}")


def verify_parts(parts, platform):
    if len(parts["p1"]) != 0x80000 or len(parts["s1"]) != 0x20000 or len(parts["m1"]) != 0x20000:
        raise ValueError("Invalid P/S/M ROM size")
    if len(parts["c1"]) != 0x400000 or len(parts["c2"]) != 0x400000:
        raise ValueError("C-ROMs must be matching 4 MiB lanes")
    for key in ("c1", "c2"):
        if parts[key][0xFFFF * 64:] != bytes(64):
            raise ValueError("Hardware blank tile is corrupt")
    program = bytearray(parts["p1"])
    program[0::2], program[1::2] = parts["p1"][1::2], parts["p1"][0::2]
    if program[0x108:0x10A] != b"\x07\x79" or program[0x115] != 0x49:
        raise ValueError("Cartridge identity or isolated boot logo bank is wrong")
    if program[0x114] != (2 if platform == "aes" else 1):
        raise ValueError("AES/MVS boot header mismatch")
    if struct.unpack_from(">IH", program, 0x10E) != (0x100400, 0x100):
        raise ValueError("Backup RAM overlaps engine memory")


def verify_image(image, parts):
    sizes = struct.unpack_from("<6I", image, 4)
    if len(image) != 4096 + sum(sizes) or sizes[3] & (sizes[3] - 1):
        raise ValueError("Invalid NeoSD sample alignment or size")
    offset = 4096
    for key, size in zip(("p1", "s1", "m1", "v1"), sizes):
        if image[offset:offset + len(parts[key])] != parts[key]:
            raise ValueError(f"NeoSD altered {key}")
        offset += size
    sprites = image[offset + sizes[4]:]
    for lane, key in enumerate(("c1", "c2")):
        if sprites[lane::2][:len(parts[key])] != parts[key]:
            raise ValueError(f"NeoSD altered {key}")


def snapshot(target, stamp):
    backup = target / "backups/skylance" / stamp
    backup.mkdir(parents=True)
    found = False
    for name in INSTALL_PATHS:
        source = target / name
        if not source.exists():
            continue
        found = True
        destination = backup / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        if source.is_dir():
            shutil.copytree(source, destination)
        else:
            shutil.copy2(source, destination)
    if not found:
        # Preserve the checked-in pre-migration MVS set on the first install.
        roms = backup / "tests/skylance/mvs/roms/skylance"
        roms.mkdir(parents=True)
        hashes = backup / "tests/skylance/mvs/hash"
        hashes.mkdir()
        for part in PARTS:
            name = f"779-{part}.{part}"
            data = subprocess.check_output(["git", "show", f"HEAD:roms/skylance/{name}"], cwd=ROOT)
            (roms / name).write_bytes(data)
        (hashes / "neogeo.xml").write_bytes(subprocess.check_output(
            ["git", "show", "HEAD:hash_eagle/skylance/neogeo.xml"], cwd=ROOT))
    (target / "latest-skylance-backup.txt").write_text(stamp + "\n", encoding="ascii")
    return backup


def install(target, stage, manifest):
    target.mkdir(parents=True, exist_ok=True)
    backup = snapshot(target, manifest["built_utc"])
    shutil.copytree(stage / "tests/skylance", target / "tests/skylance", dirs_exist_ok=True)
    shutil.copytree(ROOT / "games/skylance/artbox/generated", target / "previews/skylance", dirs_exist_ok=True)
    for log in stage.glob("*-build.log"):
        shutil.copy2(log, target / "tests/skylance" / log.name)
    for source in (ROOT / "tools/mame_tests").glob("*skylance*.bat"):
        data = source.read_text(encoding="ascii").replace("\r\n", "\n").replace("\n", "\r\n")
        (target / source.name).write_bytes(data.encode("ascii"))
    shutil.copy2(ROOT / "games/skylance/TESTING.txt", target / "SKYLANCE-TEST.txt")
    (target / "skylance-build.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="ascii")
    print(f"Installed: {target}\nPrevious Sky Lance: {backup}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("target", type=Path)
    parser.add_argument("--skip-art", action="store_true", help="Reuse the already generated direct artwork")
    args = parser.parse_args()
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-%f")
    manifest = {"built_utc": stamp, "sdk_version": "1.7.3", "framework_version": "1.3",
                "base_commit": subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "working_tree_build": True, "roms": {}}
    with tempfile.TemporaryDirectory(prefix="skylance-build-") as temp:
        stage = Path(temp)
        if not args.skip_art:
            run_make(stage, "art", "art", "sfix")
        run_make(stage, "sound", "m1rom")
        for platform in ("aes", "mvs"):
            run_make(stage, platform, f"PLATFORM={platform}", "p1")
            destination = stage / "tests/skylance" / platform
            roms = destination / "roms/skylance"
            roms.mkdir(parents=True)
            hashes = destination / "hash"
            hashes.mkdir()
            parts = {part: (ROOT / "roms/skylance" / f"779-{part}.{part}").read_bytes() for part in PARTS}
            verify_parts(parts, platform)
            image = build_image(parts, "Sky Lance", "Eagle Software", 2026, 5, 0x779)
            verify_image(image, parts)
            (destination / f"Sky-Lance-{platform.upper()}.neo").write_bytes(image)
            manifest["roms"][platform] = {}
            for part, data in parts.items():
                name = f"779-{part}.{part}"
                (roms / name).write_bytes(data)
                manifest["roms"][platform][name] = {"size": len(data), "sha256": hashlib.sha256(data).hexdigest()}
            shutil.copy2(ROOT / "hash_eagle/skylance/neogeo.xml", hashes / "neogeo.xml")
            tree = ET.parse(hashes / "neogeo.xml")
            for rom in tree.findall(".//rom"):
                if int(rom.attrib["size"], 0) != len((roms / rom.attrib["name"]).read_bytes()):
                    raise ValueError("MAME software list size mismatch")
            # Symbols let the hardware capture inspect the actual game state.
            nm = ROOT.parent / "x-tools-v3/m68k-unknown-elf/bin/m68k-unknown-elf-nm"
            (destination / "symbols.txt").write_bytes(subprocess.check_output([str(nm), "-n", str(ROOT / "out/game")]))
        shutil.copytree(ROOT / "bios/test_roms", stage / "tests/skylance/bios")
        install(args.target.resolve(), stage, manifest)


if __name__ == "__main__":
    main()
