#!/usr/bin/env python3
"""Generate 128KB Neo-Geo LSPC sprite scaling LO ROM (000-lo.lo)."""

import sys
import zlib
import base64
from pathlib import Path

LO_TABLE_B64 = (
    "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"
)

def generate_lo_rom(output_path: Path):
    table_64k = zlib.decompress(base64.b64decode("".join(LO_TABLE_B64)))
    if len(table_64k) != 65536:
        raise ValueError(f"Expected 64KB table, got {len(table_64k)}")
    # Neo-Geo 000-lo.lo is 128KB (64KB table mirrored twice)
    rom_128k = table_64k + table_64k
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(rom_128k)
    print(f"Generated {output_path} ({len(rom_128k)} bytes)")

def main():
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[1] / "000-lo.lo"
    generate_lo_rom(out)

if __name__ == "__main__":
    main()
