"""Refresh AES boot compatibility while retaining release sound and gameplay art."""

import argparse
import hashlib
import json
from pathlib import Path
import sys

from patch_neosd_sound import HEADER_SIZE, REGIONS, INIT_SIGNATURE, read_regions, swap_words

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "games/maiya/tools"))
from boot_assets import BOOT_C_BANK, GAME_FIX_BASE, install_fix, install_logo


def build(base, p1):
    original = read_regions(base)
    regions = dict(original)
    if len(original["v1"]) != 0x800000:
        raise ValueError("Use the aligned AES v1 release image")
    if len(p1) != len(original["p1"]):
        raise ValueError("P1 must retain the release ROM size")
    p = bytearray(swap_words(p1))
    if p[0x114:0x116] != bytes((0, BOOT_C_BANK)):
        raise ValueError("P1 is not the AES boot-compatible build")
    new_signature = INIT_SIGNATURE[:3] + b"\x09" + INIT_SIGNATURE[4:]
    if p.count(new_signature) != 1:
        raise ValueError("Cannot verify the rebuilt soundInit instruction")
    # This image deliberately keeps the original AES sound driver; its game
    # initialization remains command 01. Never combine a new P with an old M.
    p[p.index(new_signature) + 3] = 1
    regions["p1"] = swap_words(p)
    regions["s1"] = install_fix(original["s1"])
    c1, c2 = install_logo(original["c"][::2], original["c"][1::2])
    sprites = bytearray(len(original["c"]))
    sprites[0::2], sprites[1::2] = c1, c2
    regions["c"] = bytes(sprites)
    output = base[:HEADER_SIZE] + b"".join(regions[name] for name in REGIONS)
    assert regions["m1"] == original["m1"]
    assert regions["v1"] == original["v1"]
    assert regions["v2"] == original["v2"]
    start = GAME_FIX_BASE * 32
    assert regions["s1"][start:start + 0x300 * 32] == original["s1"][:0x300 * 32]
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("base", type=Path)
    parser.add_argument("--p1", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    base = args.base.read_bytes()
    output = build(base, args.p1.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(output)
    before, after = read_regions(base), read_regions(output)
    manifest = {"base": args.base.name, "output": args.output.name,
                "sha256": hashlib.sha256(output).hexdigest(),
                "hardware_test": "pending", "regions": {}}
    for name in REGIONS:
        manifest["regions"][name] = {
            "size": len(after[name]), "unchanged": before[name] == after[name],
            "sha256": hashlib.sha256(after[name]).hexdigest()}
    args.output.with_suffix(".json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
