#!/usr/bin/env python3
import argparse
import hashlib
from pathlib import Path


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def first_diff(a: bytes, b: bytes):
    limit = min(len(a), len(b))
    for idx in range(limit):
        if a[idx] != b[idx]:
            return idx
    if len(a) != len(b):
        return limit
    return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("left", type=Path)
    ap.add_argument("right", type=Path)
    args = ap.parse_args()

    left = args.left
    right = args.right
    left_bytes = left.read_bytes()
    right_bytes = right.read_bytes()
    left_hash = sha256(left)
    right_hash = sha256(right)

    print(f"{left}: size={len(left_bytes)} sha256={left_hash}")
    print(f"{right}: size={len(right_bytes)} sha256={right_hash}")

    diff = first_diff(left_bytes, right_bytes)
    if diff is None:
        print("match: files are byte-identical")
        return 0

    left_byte = left_bytes[diff] if diff < len(left_bytes) else None
    right_byte = right_bytes[diff] if diff < len(right_bytes) else None
    print(f"mismatch: first difference at 0x{diff:04X}")
    print(f"left byte : {left_byte!r}" if left_byte is None else f"left byte : 0x{left_byte:02X}")
    print(f"right byte: {right_byte!r}" if right_byte is None else f"right byte: 0x{right_byte:02X}")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
