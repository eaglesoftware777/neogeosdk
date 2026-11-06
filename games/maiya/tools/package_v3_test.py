"""Assemble a versioned MVS test image without changing validated sound banks."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools"))
from patch_neosd_sound import HEADER_SIZE, REGIONS, read_regions


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom-dir", type=Path,
                        default=Path("/mnt/c/mame/neogeosdk/render-v3-tests/mvs/roms/maiya"))
    parser.add_argument("--base", type=Path,
                        default=ROOT / "dist/release/Maiya-WIP-NeoSD_MVS_SOUND_FIX_PROTOCOL_PRERELEASE_v1.neo")
    parser.add_argument("--output", type=Path,
                        default=ROOT / "dist/release/Maiya-WIP-NeoSD_MVS_v3_TEST.neo")
    args = parser.parse_args()
    if args.output.exists() or args.output.with_suffix(".json").exists():
        parser.error("Output already exists; use a new versioned name")
    base = args.base.read_bytes()
    old = read_regions(base)
    if old["m1"][0xC0:0xC4] != b"NGP2" or len(old["v1"]) != 0x800000:
        parser.error("Base is not the validated MVS protocol image")
    current = dict(old)
    for region in ("p1", "s1"):
        current[region] = (args.rom_dir / f"780-{region}.{region}").read_bytes()
    c1 = (args.rom_dir / "780-c1.c1").read_bytes()
    c2 = (args.rom_dir / "780-c2.c2").read_bytes()
    current["c"] = bytes(byte for pair in zip(c1, c2) for byte in pair)
    for region in REGIONS:
        if len(current[region]) != len(old[region]):
            parser.error(f"{region} changed its reserved region size")
    output = base[:HEADER_SIZE] + b"".join(current[region] for region in REGIONS)
    after = read_regions(output)
    for region in ("m1", "v1", "v2"):
        if after[region] != old[region]:
            raise AssertionError(f"Protected {region} changed")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(output)
    manifest = {
        "image": args.output.name,
        "base": args.base.name,
        "sha256": hashlib.sha256(output).hexdigest(),
        "hardware_test": "pending",
        "regions": {region: {
            "size": len(after[region]),
            "sha256": hashlib.sha256(after[region]).hexdigest(),
            "unchanged_from_base": after[region] == old[region],
        } for region in REGIONS},
    }
    args.output.with_suffix(".json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Built {args.output}")


if __name__ == "__main__":
    main()
