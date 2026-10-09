"""Verify boot pages are isolated from the original game graphics and font."""

import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("boot_assets", ROOT / "games/maiya/tools/boot_assets.py")
boot = importlib.util.module_from_spec(spec)
spec.loader.exec_module(boot)


class BootAssetTests(unittest.TestCase):
    def test_fix_preserves_private_font_and_repeated_build_is_identical(self):
        base = bytes((i * 37) % 256 for i in range(0x300 * 32)) + bytes(0x20000 - 0x300 * 32)
        result = boot.install_fix(base)
        start = boot.GAME_FIX_BASE * 32
        self.assertEqual(result[start:start + 0x300 * 32], base[:0x300 * 32])
        self.assertEqual(result[255 * 32:256 * 32], bytes(32))
        self.assertEqual(boot.install_fix(result), result)

    def test_logo_preserves_game_tiles_and_blank_tile(self):
        off = boot.BOOT_C_BANK * 256 * 64
        base = b"G" * off + bytes(256 * 64) + b"T" * 64
        lo, hi = boot.install_logo(base, base)
        for lane in (lo, hi):
            self.assertEqual(lane[:off], base[:off])
            self.assertEqual(lane[off + 256 * 64:], base[off + 256 * 64:])
            self.assertEqual(lane[off + 255 * 64:off + 256 * 64], bytes(64))
        self.assertTrue(any(lo[off:off + 255 * 64]))
        self.assertEqual(boot.install_logo(lo, hi), (lo, hi))

    def test_occupied_boot_bank_is_rejected(self):
        size = (boot.BOOT_C_BANK + 1) * 256 * 64
        with self.assertRaises(ValueError):
            boot.install_logo(b"G" * size, bytes(size))

    def test_occupied_private_fix_bank_is_rejected(self):
        with self.assertRaises(ValueError):
            boot.install_fix(b"G" * 0x20000)


if __name__ == "__main__":
    unittest.main()
