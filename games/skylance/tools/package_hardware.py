"""Package the hardware regression candidate without replacing a release."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools"))
from pack_backbit import build as backbit
from pack_neosd import build_image
from patch_neosd_sound import read_regions

PARTS = ("p1", "s1", "m1", "v1", "c1", "c2")


def digest(data):
    return {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def archive(path, files):
    with zipfile.ZipFile(path, "x", zipfile.ZIP_DEFLATED) as z:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name, (2026, 10, 14, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, data)
    with zipfile.ZipFile(path) as z:
        if {n: z.read(n) for n in z.namelist()} != files:
            raise ValueError("ZIP payload changed")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--mvs", type=Path, required=True)
    ap.add_argument("--aes", type=Path, required=True)
    ap.add_argument("--output", type=Path, required=True)
    args = ap.parse_args()
    with zipfile.ZipFile(ROOT / "dist/release/SkyLance-BackBit-MVS_v1.zip") as z:
        release = {p: z.read(f"skylance/skylance.{p}") for p in PARTS}
    args.output.mkdir(parents=True, exist_ok=False)
    manifest = {"candidate": "Sky Lance Hardware RC1 2026-10-14", "physical_hardware": "pending",
                "base_commit": "76088ef37bfeb9cfe115a979a77cb4453dc8ab35",
                "release_graphics_and_sound_unchanged": True,
                "backbit_platform": "MVS only", "platforms": {}, "artifacts": {}}
    for platform, folder in (("mvs", args.mvs), ("aes", args.aes)):
        roms = {p: (folder / f"779-{p}.{p}").read_bytes() for p in PARTS}
        for p in PARTS[1:]:
            if roms[p] != release[p]:
                raise ValueError(f"Protected release payload changed: {platform}/{p}")
        native = bytearray(roms["p1"])
        native[0::2], native[1::2] = roms["p1"][1::2], roms["p1"][0::2]
        if native[0x114] != (1 if platform == "mvs" else 2):
            raise ValueError("Program header platform mismatch")
        image = build_image(roms, "Sky Lance Hardware RC1", "Eagle Software", 2026, 5, 0x779)
        regions = read_regions(image)
        if struct.unpack_from("<I", image, 16)[0] != 0x800000:
            raise ValueError("Expected aligned 8 MiB sample region")
        for p in ("p1", "s1", "m1", "v1"):
            if regions[p][:len(roms[p])] != roms[p] or set(regions[p][len(roms[p]):]) - {255}:
                raise ValueError("NeoSD altered ROM bytes")
        # This is the byte-lane split used by NeoDecode's ParseNeoSD path.
        if regions["c"][0::2] != roms["c1"] or regions["c"][1::2] != roms["c2"]:
            raise ValueError("NeoSD/NeoDecode C-ROM round trip failed")
        (args.output / f"SkyLance-Hardware-RC1-NeoSD-{platform.upper()}.neo").write_bytes(image)
        archive(args.output / f"SkyLance-Hardware-RC1-MAME-{platform.upper()}.zip", {
            **{f"roms/skylance/779-{p}.{p}": b for p, b in roms.items()},
            "hash/neogeo.xml": (folder / "neogeo.xml").read_bytes()})
        if platform == "mvs":
            archive(args.output / "SkyLance-Hardware-RC1-BackBit-MVS.zip",
                    backbit(roms, "skylance_hwrc1", "Sky Lance Hardware RC1", "MVS"))
        manifest["platforms"][platform] = {p: digest(b) for p, b in roms.items()}
    for p in sorted(args.output.iterdir()):
        manifest["artifacts"][p.name] = digest(p.read_bytes())
    (args.output / "MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (args.output / "SHA256SUMS.txt").write_text("".join(
        f"{entry['sha256']}  {name}\n" for name, entry in manifest["artifacts"].items()))
    print(args.output)


if __name__ == "__main__":
    main()
