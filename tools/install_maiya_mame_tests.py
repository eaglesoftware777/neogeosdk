"""Install separate, checksum-matched MVS and AES prerelease sets for MAME."""

import argparse
import hashlib
from pathlib import Path
import shutil
import time
import xml.etree.ElementTree as ET
import zlib

from patch_neosd_sound import read_regions


def install(root, target):
    backup = target / "backups" / time.strftime("%Y%m%d-%H%M%S")
    backup.mkdir(parents=True, exist_ok=True)
    for name in ("run_maiya.bat", "README.txt", "sv_mvs.bat", "v.bat"):
        source = target / name
        if source.exists():
            shutil.copy2(source, backup / name)

    template = root / "hash_eagle/maiya/neogeo.xml"
    for mode, suffix in (("mvs", "MVS_SOUND_FIX"),
                         ("aes", "AES_UNIBIOS_FIX")):
        image = root / f"dist/release/Maiya-WIP-NeoSD_{suffix}_PRERELEASE.neo"
        regions = read_regions(image.read_bytes())
        payloads = {f"780-{key}.{key}": regions[key]
                    for key in ("p1", "s1", "m1", "v1")}
        payloads["780-c1.c1"] = regions["c"][0::2]
        payloads["780-c2.c2"] = regions["c"][1::2]
        if regions["v2"]:
            raise ValueError("This installer expects the verified single-V1 images")
        rom_dir = target / "tests" / mode / "roms" / "maiya"
        hash_dir = target / "tests" / mode / "hash"
        if rom_dir.exists():
            shutil.copytree(rom_dir, backup / mode / "roms")
        if hash_dir.exists():
            shutil.copytree(hash_dir, backup / mode / "hash")
        rom_dir.mkdir(parents=True, exist_ok=True)
        hash_dir.mkdir(parents=True, exist_ok=True)
        tree = ET.parse(template)
        for rom in tree.findall(".//rom"):
            payload = payloads[rom.attrib["name"]]
            rom.set("size", hex(len(payload)))
            rom.set("crc", f"{zlib.crc32(payload):08x}")
            rom.set("sha1", hashlib.sha1(payload).hexdigest())
        for area in tree.findall(".//dataarea"):
            if area.attrib["name"] == "ymsnd:adpcma":
                area.set("size", hex(len(regions["v1"])))
        ET.indent(tree, space="    ")
        tree.write(hash_dir / "neogeo.xml", encoding="utf-8", xml_declaration=True)
        for name, payload in payloads.items():
            (rom_dir / name).write_bytes(payload)
        print(f"Installed {mode.upper()}: {image.name}")

    launchers = root / "tools/mame_tests"
    for source in launchers.glob("*.bat"):
        destination = target / source.name
        if destination.exists() and not (backup / source.name).exists():
            shutil.copy2(destination, backup / source.name)
        # CMD batch files use Windows line endings even when installed from WSL.
        destination.write_bytes(source.read_text().replace("\r\n", "\n")
                                .replace("\n", "\r\n").encode("ascii"))
    shutil.copy2(launchers / "README.txt", target / "README.txt")
    for name in ("sv_mvs.bat", "v.bat"):
        path = target / name
        if path.exists():
            text = path.read_text().replace('-rompath "roms"',
                                           '-rompath "tests\\mvs\\roms;roms;..\\roms"')
            text = text.replace('-hashpath "hash\\maiya"',
                                '-hashpath "tests\\mvs\\hash"')
            path.write_bytes(text.replace("\n", "\r\n").encode("ascii"))
    print(f"Existing files backed up to {backup}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("target", type=Path)
    args = parser.parse_args()
    install(Path(__file__).resolve().parents[1], args.target)


if __name__ == "__main__":
    main()
