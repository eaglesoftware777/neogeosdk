"""Build current Maiya AES/MVS programs, back up MAME, and install both."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ET
import zlib
from pack_neosd import build_image
from patch_neosd_sound import read_regions

PARTS = ("p1", "s1", "m1", "v1", "c1", "c2")


def verify_image(image, parts):
    regions = read_regions(image)
    for part in ("p1", "s1", "m1", "v1"):
        if regions[part][:len(parts[part])] != parts[part]:
            raise ValueError(f"NeoSD changed Maiya {part}")
    if regions["c"][0::2] != parts["c1"] or regions["c"][1::2] != parts["c2"]:
        raise ValueError("NeoSD changed Maiya graphics lane order or bytes")


def verify_set(root, rom_dir, hash_file):
    for part in ("c1", "c2"):
        name = f"780-{part}.{part}"
        data = (rom_dir / name).read_bytes()
        if data != (root / "games/maiya/artbox/generated" / name).read_bytes():
            raise ValueError(f"Stale Maiya graphics: {name}")
        if len(data) != 0x400000 or data[0xFFFF * 64:] != bytes(64):
            raise ValueError(f"Invalid Maiya tile reservation: {name}")
    for rom in ET.parse(hash_file).findall(".//rom"):
        data = (rom_dir / rom.attrib["name"]).read_bytes()
        if (int(rom.attrib["size"], 0) != len(data) or
            rom.attrib.get("sha1") != hashlib.sha1(data).hexdigest()):
            raise ValueError(f"Software list does not match {rom.attrib['name']}")


def backup_maiya(root, target, stamp):
    backup = target / "backups" / stamp
    backup.mkdir(parents=True)
    # Back up only Maiya: do not copy or replace Sky Lance's test installation.
    paths = ["roms/maiya", "hash/maiya", "tests/aes", "tests/mvs", "BUILD.json",
             "latest-backup.txt", "aes-build.log", "mvs-build.log", "README.txt"]
    paths += [p.name for p in target.glob("*maiya*") if p.is_file()]
    for name in paths:
        source = target / name
        if not source.exists():
            continue
        destination = backup / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        if source.is_dir():
            shutil.copytree(source, destination)
        else:
            shutil.copy2(source, destination)
    generated = root / "games/maiya/artbox/generated"
    shutil.copytree(generated, backup / "source-generated")
    write_backup_hash(backup)
    return backup


def write_backup_hash(backup):
    rom_dir = backup / "roms" / "maiya"
    listing = backup / "hash" / "maiya" / "neogeo.xml"
    if not listing.is_file():
        listing = backup / "hash" / "neogeo.xml"
    if not listing.is_file() or not rom_dir.is_dir():
        return
    tree = ET.parse(listing)
    for rom in tree.findall(".//rom"):
        source = rom_dir / rom.attrib["name"]
        if not source.is_file():
            continue
        data = source.read_bytes()
        rom.set("size", hex(len(data)))
        rom.set("crc", f"{zlib.crc32(data):08x}")
        rom.set("sha1", hashlib.sha1(data).hexdigest())
    destination = backup / "compare-hash"
    destination.mkdir(exist_ok=True)
    tree.write(destination / "neogeo.xml", encoding="utf-8", xml_declaration=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("target", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    target = args.target.resolve()
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-%f")
    target.mkdir(parents=True, exist_ok=True)
    backup = backup_maiya(root, target, stamp)
    print(f"Previous Maiya and generated art backed up: {backup}", flush=True)
    # Build both variants before touching the installed version.
    with tempfile.TemporaryDirectory(prefix="maiya-build-") as tmp:
        stage = Path(tmp)
        manifest = {"built_utc": stamp, "commit": subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(), "roms": {}}
        # Program tables and graphics must come from the same asset build.
        with (stage / "assets-build.log").open("w") as log:
            subprocess.run(["make", "GAME=maiya", "art", "sfix", "m1rom"],
                           cwd=root, stdout=log, stderr=subprocess.STDOUT, check=True)
        print("Built matching graphics, FIX and sound banks", flush=True)
        for mode in ("mvs", "aes"):
            with (stage / f"{mode}-build.log").open("w") as log:
                subprocess.run(["make", "GAME=maiya", f"PLATFORM={mode}", "GAME_OPTIMIZE=-O2 -g3", "p1"],
                               cwd=root, stdout=log, stderr=subprocess.STDOUT, check=True)
            roms = stage / "tests" / mode / "roms" / "maiya"
            roms.mkdir(parents=True)
            hashes = stage / "tests" / mode / "hash"
            hashes.mkdir()
            shutil.copy2(root / "hash_eagle/maiya/neogeo.xml", hashes / "neogeo.xml")
            manifest["roms"][mode] = {}
            for part in PARTS:
                name = f"780-{part}.{part}"
                shutil.copy2(root / "roms/maiya" / name, roms / name)
                manifest["roms"][mode][name] = hashlib.sha256((roms / name).read_bytes()).hexdigest()
            verify_set(root, roms, hashes / "neogeo.xml")
            shutil.copy2(root / "out/game", stage / "tests" / mode / "game.elf")
            parts = {part: (roms / f"780-{part}.{part}").read_bytes() for part in PARTS}
            image = build_image(parts, "Maiya: Super Nature Girl", "Eagle Software", 2026, 0, 0x780)
            verify_image(image, parts)
            (stage / "tests" / mode / f"Maiya-{mode.upper()}.neo").write_bytes(image)
            print(f"Built {mode.upper()}", flush=True)
        (target / "latest-backup.txt").write_text(stamp + "\n", encoding="ascii")
        shutil.copytree(stage / "tests", target / "tests", dirs_exist_ok=True)
        shutil.copytree(stage / "tests/mvs/roms/maiya", target / "roms/maiya", dirs_exist_ok=True)
        (target / "hash/maiya").mkdir(parents=True, exist_ok=True)
        shutil.copy2(stage / "tests/mvs/hash/neogeo.xml", target / "hash/maiya/neogeo.xml")
        for log in stage.glob("*-build.log"):
            shutil.copy2(log, target / log.name)
        for source in (root / "tools/mame_tests").glob("*.bat"):
            if "skylance" in source.name:
                continue
            (target / source.name).write_bytes(source.read_text().replace("\r\n", "\n")
                                              .replace("\n", "\r\n").encode("ascii"))
        shutil.copy2(root / "tools/mame_tests/backup_current.ps1", target / "backup_current.ps1")
        shutil.copy2(root / "tools/mame_tests/README.txt", target / "README.txt")
        (target / "BUILD.json").write_text(json.dumps(manifest, indent=2) + "\n")
        print(f"Installed in {target}; previous version: {backup}")


if __name__ == "__main__":
    main()
