"""Package a verified Sky Lance build for cartridges and MAME without changing game ROMs."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import zipfile
import zlib

from build_skylance_mame import PARTS, verify_parts, verify_image
from pack_neosd import build_image
from pack_darksoft import build as darksoft_build, unpack as darksoft_unpack
from pack_backbit import build as backbit_build

ROOT = Path(__file__).resolve().parents[1]
V_CAPACITY = 0x800000


def digest(data):
    return {"size": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def write_zip(path, files):
    with zipfile.ZipFile(path, "x", compression=zipfile.ZIP_DEFLATED) as archive:
        for name, data in sorted(files.items()):
            archive.writestr(name, data)
    with zipfile.ZipFile(path) as archive:
        assert {name: archive.read(name) for name in archive.namelist()} == files


def stage(target, output, version="1.7.3"):
    installed = json.loads((target / "skylance-build.json").read_text())
    output.mkdir(parents=True, exist_ok=False)
    manifest = {"version": version, "installed_build": installed["built_utc"],
                "status": "physical hardware validation still recommended",
                "bios_included": False, "v_capacity": V_CAPACITY,
                "platforms": {}, "artifacts": {}}
    for platform in ("mvs", "aes"):
        source = target / "tests/skylance" / platform / "roms/skylance"
        parts = {p: (source / f"779-{p}.{p}").read_bytes() for p in PARTS}
        verify_parts(parts, platform)
        for part, data in parts.items():
            expected = installed["roms"][platform][f"779-{part}.{part}"]
            if digest(data) != {k: expected[k] for k in ("size", "sha256")}:
                raise ValueError(f"Installed {platform}/{part} does not match its manifest")
        if len(parts["v1"]) > V_CAPACITY:
            raise ValueError("Samples exceed the candidate's 8 MiB region")
        padded = dict(parts)
        padded["v1"] += b"\xff" * (V_CAPACITY - len(parts["v1"]))
        prefix = f"SkyLance-{platform.upper()}_v{version}"
        image = build_image(padded, "Sky Lance", "Eagle Software", 2026, 5, 0x779)
        verify_image(image, padded)
        if struct.unpack_from("<I", image, 16)[0] != V_CAPACITY:
            raise ValueError("Unexpected NeoSD sample capacity")
        neo = output / f"{prefix}-NeoSD.neo"
        neo.write_bytes(image)

        dark = darksoft_build(source, "779", platform, "Sky Lance")
        dark["vroma0"] = padded["v1"]
        details = json.loads(dark["MANIFEST.json"])
        details["files"]["vroma0"].update(digest(dark["vroma0"]))
        details["files"]["vroma0"].update(size_hex=hex(V_CAPACITY),
            crc32=f'{zlib.crc32(dark["vroma0"]) & 0xffffffff:08x}')
        details["verification"]["vroma0_size"] = hex(V_CAPACITY)
        dark["MANIFEST.json"] = (json.dumps(details, indent=2) + "\n").encode()
        recovered = darksoft_unpack(dark, "779")
        if recovered != {f"779-{p}.{p}": data for p, data in parts.items()}:
            raise ValueError("Darksoft payload did not round-trip")
        write_zip(output / f"{prefix}-Darksoft.zip",
                  {f"skylance/{name}": data for name, data in dark.items()})
        backbit = backbit_build(parts, "skylance", "Sky Lance", platform)
        if any(backbit[f"skylance/skylance.{p}"] != parts[p] for p in PARTS):
            raise ValueError("BackBit changed a payload")
        write_zip(output / f"{prefix}-BackBit.zip", backbit)
        write_zip(output / f"{prefix}-MAME.zip", {
            **{f"roms/skylance/779-{p}.{p}": data for p, data in parts.items()},
            "hash/neogeo.xml": (source.parents[1] / "hash/neogeo.xml").read_bytes()})
        manifest["platforms"][platform] = {p: digest(data) for p, data in parts.items()}
    for path in sorted(output.iterdir()):
        manifest["artifacts"][path.name] = digest(path.read_bytes())
    (output / "MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (output / "TESTING.txt").write_text(
        f"Sky Lance {version}\n\n"
        "No BIOS ROM is included. Keep the existing board BIOS.\n"
        "Choose the package matching the target platform and cartridge.\n"
        "NeoSD: copy the .neo file to the cartridge card.\n"
        "Darksoft: extract the skylance folder to a game slot on the card.\n"
        "BackBit: extract the skylance folder; its six payloads are unmodified.\n"
        "MAME: use the included ROM directory and software-list hash directory.\n\n"
        "NeoSD and Darksoft V regions are padded to 8 MiB with FF bytes.\n"
        "No sample addresses, M1 protocol, or original ROM payloads are changed\n"
        "by packaging. All payloads and archives are round-trip checked.\n\n"
        "Real hardware checks still required: MVS/AES cold boot, sound, slot\n"
        "switch, cartridge return shortcuts, logo and sustained boss combat.\n")
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", type=Path, default=Path("/mnt/c/mame/neogeosdk"))
    parser.add_argument("--output", type=Path)
    parser.add_argument("--version", default="1.7.3")
    args = parser.parse_args()
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S")
    output = args.output or ROOT / f"dist/skylance-v{args.version}" / f"SkyLance-{stamp}"
    print(stage(args.target, output, args.version))


if __name__ == "__main__":
    main()
