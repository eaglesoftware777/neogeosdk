"""Apply the slot-switch driver to a release image without replacing graphics."""

import argparse
import hashlib
import json
from pathlib import Path
import struct

HEADER_SIZE = 4096
REGIONS = ("p1", "s1", "m1", "v1", "v2", "c")
# Verified soundInit prologue in the v1.7.1 cartridge, decoded 68000 byte order.
INIT_SIGNATURE = bytes.fromhex("487800014ebaffb0588f4e714e75")


def read_regions(image):
    if len(image) < HEADER_SIZE or image[:4] != b"NEO\x01":
        raise ValueError("Not a NEO v1 image")
    sizes = struct.unpack_from("<6I", image, 4)
    if HEADER_SIZE + sum(sizes) != len(image):
        raise ValueError("Header sizes do not match the image length")
    regions = {}
    offset = HEADER_SIZE
    for name, size in zip(REGIONS, sizes):
        regions[name] = image[offset:offset + size]
        offset += size
    return regions


def swap_words(data):
    if len(data) & 1:
        raise ValueError("P ROM must contain complete 68000 words")
    result = bytearray(len(data))
    result[0::2], result[1::2] = data[1::2], data[0::2]
    return bytes(result)


def patch_image(base, m1):
    regions = read_regions(base)
    if len(m1) != len(regions["m1"]):
        raise ValueError("Replacement M1 must retain the release region size")
    if len(regions["v1"]) != 0x800000:
        raise ValueError("Use the previously aligned 8 MiB V1 release image")
    p = bytearray(swap_words(regions["p1"]))
    if p.count(INIT_SIGNATURE) != 1:
        raise ValueError("Expected exactly one verified release soundInit function")
    at = p.index(INIT_SIGNATURE) + 3
    p[at] = 9
    regions["p1"] = swap_words(p)
    regions["m1"] = m1
    output = base[:HEADER_SIZE] + b"".join(regions[name] for name in REGIONS)
    after = read_regions(output)
    before = read_regions(base)
    for name in ("s1", "v1", "v2", "c"):
        if before[name] != after[name]:
            raise AssertionError(f"Protected {name} region changed")
    differences = sum(a != b for a, b in zip(before["p1"], after["p1"]))
    if differences != 1:
        raise AssertionError("P1 must change only the game-init command byte")
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("base", type=Path)
    parser.add_argument("--m1", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    base = args.base.read_bytes()
    output = patch_image(base, args.m1.read_bytes())
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
