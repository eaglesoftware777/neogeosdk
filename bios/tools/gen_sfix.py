#!/usr/bin/env python3
"""Assemble an original, single-ink 5x7 system font into FIX tile wiring."""

from pathlib import Path
import sys


# Seven rows of five bits, placed inside an 8x8 transparent cell.
GLYPHS = {
    "A": "0e 11 11 1f 11 11 11", "B": "1e 11 11 1e 11 11 1e",
    "C": "0f 10 10 10 10 10 0f", "D": "1e 11 11 11 11 11 1e",
    "E": "1f 10 10 1e 10 10 1f", "F": "1f 10 10 1e 10 10 10",
    "G": "0f 10 10 13 11 11 0f", "H": "11 11 11 1f 11 11 11",
    "I": "0e 04 04 04 04 04 0e", "J": "07 02 02 02 12 12 0c",
    "K": "11 12 14 18 14 12 11", "L": "10 10 10 10 10 10 1f",
    "M": "11 1b 15 15 11 11 11", "N": "11 19 19 15 13 13 11",
    "O": "0e 11 11 11 11 11 0e", "P": "1e 11 11 1e 10 10 10",
    "Q": "0e 11 11 11 15 12 0d", "R": "1e 11 11 1e 14 12 11",
    "S": "0f 10 10 0e 01 01 1e", "T": "1f 04 04 04 04 04 04",
    "U": "11 11 11 11 11 11 0e", "V": "11 11 11 11 11 0a 04",
    "W": "11 11 11 15 15 1b 11", "X": "11 11 0a 04 0a 11 11",
    "Y": "11 11 0a 04 04 04 04", "Z": "1f 01 02 04 08 10 1f",
    "0": "0e 11 13 15 19 11 0e", "1": "04 0c 04 04 04 04 0e",
    "2": "0e 11 01 02 04 08 1f", "3": "1e 01 01 0e 01 01 1e",
    "4": "02 06 0a 12 1f 02 02", "5": "1f 10 10 1e 01 01 1e",
    "6": "0e 10 10 1e 11 11 0e", "7": "1f 01 02 04 08 08 08",
    "8": "0e 11 11 0e 11 11 0e", "9": "0e 11 11 0f 01 01 0e",
    "!": "04 04 04 04 04 00 04", "?": "0e 11 01 02 04 00 04",
    ".": "00 00 00 00 00 00 04", ",": "00 00 00 00 00 04 08",
    ":": "00 04 00 00 04 00 00", ";": "00 04 00 00 04 04 08",
    "-": "00 00 00 1f 00 00 00", "_": "00 00 00 00 00 00 1f",
    "+": "00 04 04 1f 04 04 00", "=": "00 00 1f 00 1f 00 00",
    "/": "01 01 02 04 08 10 10", "\\": "10 10 08 04 02 01 01",
    "(": "02 04 08 08 08 04 02", ")": "08 04 02 02 02 04 08",
    "[": "0e 08 08 08 08 08 0e", "]": "0e 02 02 02 02 02 0e",
    "<": "01 02 04 08 04 02 01", ">": "10 08 04 02 04 08 10",
    "'": "04 04 08 00 00 00 00", '"': "0a 0a 0a 00 00 00 00",
    "*": "00 15 0e 1f 0e 15 00", "#": "0a 0a 1f 0a 1f 0a 0a",
    "%": "19 19 02 04 08 13 13", "&": "0c 12 14 08 15 12 0d",
    "@": "0e 11 17 15 17 10 0f", "|": "04 04 04 04 04 04 04",
    "$": "04 0f 14 0e 05 1e 04", "^": "04 0a 11 00 00 00 00",
    "~": "00 00 09 16 00 00 00", "`": "08 04 00 00 00 00 00",
    "{": "02 04 04 08 04 04 02", "}": "08 04 04 02 04 04 08",
}


def encode_tile(pixels):
    tile = bytearray(32)
    # Four byte-wide columns appear in this order on the S-ROM bus.
    for y in range(8):
        for pair, offset in enumerate((16, 24, 0, 8)):
            tile[offset + y] = pixels[y][pair * 2] | (pixels[y][pair * 2 + 1] << 4)
    return tile


def build_rom():
    rom = bytearray(131072)
    for code in range(33, 127):
        rows = [int(row, 16) for row in GLYPHS.get(chr(code).upper(), GLYPHS["?"]).split()]
        pixels = [[0] * 8 for _ in range(8)]
        for y, row in enumerate(rows):
            for x in range(5):
                pixels[y][x + 1] = (row >> (4 - x)) & 1
        rom[code * 32:(code + 1) * 32] = encode_tile(pixels)
        for half in range(2):
            tile = encode_tile([pixels[(y + half * 8) // 2] for y in range(8)])
            index = (half + 1) * 256 + code
            rom[index * 32:(index + 1) * 32] = tile
    rom[32:64] = bytes([0x11] * 32)
    return bytes(rom)


def generate_sfix_rom(output_path):
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(build_rom())
    print(f"Generated {output_path} (131072 bytes)")


if __name__ == "__main__":
    generate_sfix_rom(sys.argv[1] if len(sys.argv) > 1 else "sfix.sfix")
