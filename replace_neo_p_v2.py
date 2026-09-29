#!/usr/bin/env python3
"""Replace only the P1 region inside a NeoSD .neo image."""

from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path

HEADER_SIZE = 0x1000


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("base", type=Path)
    ap.add_argument("p1", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()

    neo = bytearray(args.base.read_bytes())
    p1 = args.p1.read_bytes()

    if len(neo) < HEADER_SIZE or neo[:4] != b"NEO\x01":
        raise SystemExit("error: base file is not a NEO v1 image")

    p_size = struct.unpack_from("<I", neo, 0x04)[0]
    if len(p1) != p_size:
        raise SystemExit(
            f"error: P1 size is 0x{len(p1):X}, but .neo expects 0x{p_size:X}"
        )

    p_off = HEADER_SIZE
    tail_before = bytes(neo[p_off + p_size:])
    neo[p_off:p_off + p_size] = p1

    if bytes(neo[p_off + p_size:]) != tail_before:
        raise SystemExit("error: data after P1 changed unexpectedly")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(neo)

    print(f"P1 size  : 0x{p_size:X}")
    print(f"Output   : {args.output}")
    print(f"SHA-256  : {hashlib.sha256(neo).hexdigest()}")


if __name__ == "__main__":
    main()
