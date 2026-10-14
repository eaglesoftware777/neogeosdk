"""Pack a homebrew cartridge with one P/S/M/V ROM and one C1/C2 pair."""

import argparse
from pathlib import Path
import struct


def pad(data, alignment):
    return data + b"\xff" * (-len(data) % alignment)


def build_image(parts, name, manufacturer, year, genre, ngh, hardware_alignment=True):
    if not all(parts.values()):
        raise ValueError("All six cartridge ROMs must be nonempty")
    if len(parts["p1"]) > 0x100000:
        raise ValueError("Banked P ROMs are not supported by this packer")
    if len(parts["c1"]) != len(parts["c2"]):
        raise ValueError("C1 and C2 must have equal lengths")
    title = name.encode("ascii")
    maker = manufacturer.encode("ascii")
    if len(title) > 33 or len(maker) > 17:
        raise ValueError("Name/manufacturer exceed the NeoSD header fields")

    p, s, m, v = (pad(parts[key], 0x10000) for key in ("p1", "s1", "m1", "v1"))
    # NeoSD sample addressing needs a power-of-two region, not just 64 KiB.
    if hardware_alignment:
        v = pad(v, 1 << (len(v) - 1).bit_length())
    c = bytearray(len(parts["c1"]) * 2)
    c[0::2], c[1::2] = parts["c1"], parts["c2"]
    c = pad(bytes(c), 0x40000)
    # Offsets are derived from padded sizes; interleave bytes, never swap lanes.
    header = bytearray(4096)
    header[:4] = b"NEO\x01"
    struct.pack_into("<10I", header, 4, len(p), len(s), len(m), len(v),
                     0, len(c), year, genre, 0, ngh)
    header[44:77] = title.ljust(33, b"\0")
    header[77:94] = maker.ljust(17, b"\0")
    return bytes(header) + p + s + m + v + c


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom-dir", type=Path, required=True)
    parser.add_argument("--game-id", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--name", required=True)
    parser.add_argument("--manufacturer", default="Eagle Software")
    parser.add_argument("--year", type=int, default=2026)
    parser.add_argument("--genre", type=int, default=5)
    parser.add_argument("--legacy-alignment", action="store_true",
                        help="Use 64 KiB sample padding instead of hardware padding")
    args = parser.parse_args()
    parts = {key: (args.rom_dir / f"{args.game_id}-{key}.{key}").read_bytes()
             for key in ("p1", "s1", "m1", "v1", "c1", "c2")}
    data = build_image(parts, args.name, args.manufacturer,
                       args.year, args.genre, int(args.game_id, 16),
                       hardware_alignment=not args.legacy_alignment)
    args.output.write_bytes(data)
    print(f"Packaged {args.output}: {len(data)} bytes")


if __name__ == "__main__":
    main()
