"""Isolated Sky Lance cartridge logo and original system font."""

import importlib.util
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location("sky_system_font", ROOT / "bios/tools/gen_sfix.py")
font = importlib.util.module_from_spec(spec)
spec.loader.exec_module(font)
BOOT_C_BANK = 0x49
GAME_FIX_BASE = 0xD00


def text_pixels(text, scale=1):
    image = np.zeros((8 * scale, len(text) * 8 * scale), dtype=np.uint8)
    for i, char in enumerate(text):
        for y, row in enumerate(font.GLYPHS.get(char.upper(), "00 " * 7).split()):
            for x in range(5):
                if int(row, 16) & (1 << (4 - x)):
                    image[y * scale:(y + 1) * scale,
                          (i * 8 + x + 1) * scale:(i * 8 + x + 2) * scale] = 1
    return image


def logo_pixels():
    image = np.zeros((64, 240), dtype=np.uint8)
    text = text_pixels("SKY LANCE", 3)
    image[20:44, 12:228] = text
    image[50:52, 24:216] = 3
    # The BIOS map omits the final column of its first row.
    image[:16, 224:] = 0
    return image


def fix_rom():
    rom = bytearray(font.build_rom())
    # Gameplay has its own ASCII page; low tiles remain usable by BIOS menus.
    lo = GAME_FIX_BASE * 32
    rom[lo:lo + 256 * 32] = rom[:256 * 32]
    rom[0xFF * 32:0x100 * 32] = bytes(32)
    return rom
