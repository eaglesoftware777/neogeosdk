"""Build isolated cartridge boot tiles without moving gameplay sprite data."""

import importlib.util
from pathlib import Path
import sys

import numpy as np

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "artbox"))
from tile_codec import encode_image

spec = importlib.util.spec_from_file_location("system_font", ROOT / "bios/tools/gen_sfix.py")
font = importlib.util.module_from_spec(spec)
spec.loader.exec_module(font)

BOOT_C_BANK = 0x49
GAME_FIX_BASE = 0xD00
MARKER_OFFSET = 0xCFF * 32
MARKER = b"EAGLE-MAIYA-BOOT-1".ljust(32, b"\0")
BOOT_TEXT_MAPS = (
    ((0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F, 0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F, 0x5E, 0x60, 0x7D),
     (0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E, 0x40, 0x5F, 0x7C, 0x7E)),
    ((0x7F, 0x9A, 0x9C, 0x9E, 0xFF, 0xBB, 0xBD, 0xBF, 0xDA, 0xDC, 0xDE, 0xFA, 0xFC, 0x100, 0x102, 0x104, 0x106),
     (0x99, 0x9B, 0x9D, 0x9F, 0xBA, 0xBC, 0xBE, 0xD9, 0xDB, 0xDD, 0xDF, 0xFB, 0xFD, 0x101, 0x103, 0x105, 0x107)),
)
BOOT_MARK_MAP = (
    (0x200, 0x201, 0x202, 0x203, 0x204, 0x205, 0x206, 0x207, 0x208, 0x209),
    (0x20A, 0x20B, 0x20C, 0x20D, 0x20E, 0x20F, 0x214, 0x215, 0x216, 0x217),
    (0x218, 0x219, 0x21A, 0x21B, 0x21C, 0x21D, 0x21E, 0x21F, 0x240, 0x25E),
)


def text_pixels(text, scale=1, pen=1):
    pixels = np.zeros((8 * scale, len(text) * 8 * scale), dtype=np.uint8)
    for i, char in enumerate(text):
        if char == " ":
            continue
        rows = [int(row, 16) for row in font.GLYPHS[char].split()]
        for y, row in enumerate(rows):
            for x in range(5):
                if row & (1 << (4 - x)):
                    pixels[y * scale:(y + 1) * scale,
                           (i * 8 + x + 1) * scale:(i * 8 + x + 2) * scale] = pen
    return pixels


def logo_lanes():
    canvas = np.zeros((64, 240), dtype=np.uint8)
    # Keep column 14 blank in row zero; that position is absent in the BIOS map.
    letters = text_pixels("MAIYA", scale=5)
    canvas[12:52, 12:212] = letters
    canvas[55:57, 16:208] = 3
    lo, hi = encode_image(canvas, 60)
    lanes = []
    for lane in (lo, hi):
        body = lane[:14 * 64] + lane[15 * 64:]
        page = body.ljust(256 * 64, b"\0")
        # The BIOS uses tile FF as a blank while shrinking the animation.
        assert page[255 * 64:] == bytes(64)
        final = bytearray(len(page))
        final[0::2], final[1::2] = page[1::2], page[0::2]
        lanes.append(bytes(final))
    return tuple(lanes)


def install_logo(c1, c2):
    if len(c1) != len(c2):
        raise ValueError("C-ROM lanes must have equal lengths")
    offset = BOOT_C_BANK * 256 * 64
    length = 256 * 64
    if offset + length > len(c1):
        raise ValueError("C-ROMs are too small for the reserved boot bank")
    old_logo = logo_lanes()
    result = []
    for lane, logo in zip((c1, c2), old_logo):
        previous = lane[offset:offset + length]
        if any(previous) and previous != logo:
            raise ValueError("Gameplay data overlaps the reserved boot bank")
        result.append(lane[:offset] + logo + lane[offset + length:])
    return tuple(result)


def install_fix(base):
    if len(base) != 0x20000:
        raise ValueError("Expected a 128 KiB S-ROM")
    rom = bytearray(base)
    if base[MARKER_OFFSET:MARKER_OFFSET + 32] != MARKER:
        start = GAME_FIX_BASE * 32
        if not set(base[start:start + 0x300 * 32]).issubset({0, 255}):
            raise ValueError("Private FIX banks already contain unrelated graphics")
        rom[start:start + 0x300 * 32] = base[:0x300 * 32]
    # This is an original system font. The game's original multicolor font
    # stays byte-identical in the private banks above, including HUD graphics.
    rom[:0x300 * 32] = font.build_rom()[:0x300 * 32]
    for text, positions in zip(("16-BIT POWERED ", "GAME DEVELOPMENT "), BOOT_TEXT_MAPS):
        image = np.repeat(text_pixels(text), 2, axis=0)
        for half, row in enumerate(positions):
            for i, tile in enumerate(row):
                rom[tile * 32:(tile + 1) * 32] = font.encode_tile(
                    image[half * 8:(half + 1) * 8, i * 8:(i + 1) * 8])
    mark = np.zeros((24, 80), dtype=np.uint8)
    mark[8:16, :80] = text_pixels("EAGLE SOFT", pen=5)
    for y, row in enumerate(BOOT_MARK_MAP):
        for x, tile in enumerate(row):
            rom[tile * 32:(tile + 1) * 32] = font.encode_tile(mark[y * 8:(y + 1) * 8, x * 8:(x + 1) * 8])
    copyright_rows = (0x3C, 0x42, 0x99, 0xA1, 0xA1, 0x99, 0x42, 0x3C)
    copyright_pixels = [[(row >> (7 - x)) & 1 for x in range(8)]
                        for row in copyright_rows]
    rom[0x7B * 32:0x7C * 32] = font.encode_tile(copyright_pixels)
    rom[0xFF * 32:0x100 * 32] = bytes(32)
    rom[MARKER_OFFSET:MARKER_OFFSET + 32] = MARKER
    return bytes(rom)
