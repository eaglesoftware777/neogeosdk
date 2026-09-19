#!/usr/bin/env python3
"""Build the LSPC vertical shrink lookup from its bit-reversed row ordering."""

from pathlib import Path
import sys


def reverse_byte(value):
    result = 0
    for _ in range(8):
        result = (result << 1) | (value & 1)
        value >>= 1
    return result


def build_rom():
    # Each successive zoom level admits one more source row. Sorting the
    # admitted rows preserves scan order; unused entries select the last row.
    order = [reverse_byte(row) for row in range(256)]
    table = bytearray()
    for zoom in range(256):
        rows = [row for row in range(256) if order[row] <= zoom]
        table.extend(rows)
        table.extend([255] * (256 - len(rows)))
    return bytes(table) * 2


def generate_lo_rom(output_path):
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(build_rom())
    print(f"Generated {output_path} (131072 bytes)")


if __name__ == "__main__":
    generate_lo_rom(sys.argv[1] if len(sys.argv) > 1 else "000-lo.lo")
