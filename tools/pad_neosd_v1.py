#!/usr/bin/env python3
"""Pad the V1 region of a NeoSD .neo image without changing the ROM data."""

from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path


HEADER_SIZE = 0x1000
V1_SIZE_OFFSET = 0x10


def parse_size(value: str) -> int:
    return int(value, 0)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument(
        "--size",
        type=parse_size,
        default=0x800000,
        help="target V1 size (default: 0x800000)",
    )
    parser.add_argument(
        "--pad-byte",
        type=parse_size,
        default=0xFF,
        help="padding byte (default: 0xFF)",
    )
    args = parser.parse_args()

    if not 0 <= args.pad_byte <= 0xFF:
        raise SystemExit("error: --pad-byte must be between 0x00 and 0xFF")

    src = args.input.read_bytes()
    if len(src) < HEADER_SIZE or src[:4] != b"NEO\x01":
        raise SystemExit("error: input is not a NEO v1 image")

    p_size, s_size, m_size, v1_size, v2_size, c_size = struct.unpack_from(
        "<6I", src, 0x04
    )

    expected_size = (
        HEADER_SIZE + p_size + s_size + m_size + v1_size + v2_size + c_size
    )
    if len(src) != expected_size:
        raise SystemExit(
            f"error: image length is 0x{len(src):X}, "
            f"header describes 0x{expected_size:X}"
        )

    if args.size < v1_size:
        raise SystemExit(
            f"error: requested V1 size 0x{args.size:X} is smaller than "
            f"current V1 size 0x{v1_size:X}"
        )
    if args.size == v1_size:
        raise SystemExit("error: V1 already has the requested size")

    v1_offset = HEADER_SIZE + p_size + s_size + m_size
    old_v1_end = v1_offset + v1_size
    pad_len = args.size - v1_size

    out = bytearray()
    out += src[:old_v1_end]
    out += bytes([args.pad_byte]) * pad_len
    out += src[old_v1_end:]

    struct.pack_into("<I", out, V1_SIZE_OFFSET, args.size)

    new_v1_end = v1_offset + args.size
    new_c_offset = HEADER_SIZE + p_size + s_size + m_size + args.size + v2_size

    # Hardware A/B test guard: only the V1 size field and inserted padding
    # may differ. The complete payload after the original V1 must be unchanged.
    if out[new_v1_end:] != src[old_v1_end:]:
        raise SystemExit("error: payload after V1 changed unexpectedly")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(out)

    print(f"Input V1     : 0x{v1_size:X}")
    print(f"Output V1    : 0x{args.size:X}")
    print(f"Padding      : 0x{pad_len:X} bytes")
    print(f"Old C offset : 0x{old_v1_end + v2_size:X}")
    print(f"New C offset : 0x{new_c_offset:X}")
    print(f"Output size  : 0x{len(out):X}")
    print(f"SHA-256      : {hashlib.sha256(out).hexdigest()}")
    print(f"Wrote        : {args.output}")


if __name__ == "__main__":
    main()
