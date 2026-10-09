"""Direct Sky Lance ROM data, boot isolation and cartridge package checks."""

import importlib.util
import json
from pathlib import Path
import sys
import unittest

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "artbox"))
sys.path.insert(0, str(ROOT / "tools"))
from tile_codec import decode_image
from pack_neosd import build_image

spec = importlib.util.spec_from_file_location("sky_installer", ROOT / "tools/build_skylance_mame.py")
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)
ART = ROOT / "games/skylance/artbox/generated"


@unittest.skipUnless((ART / "assets.json").exists(), "Build Sky Lance art first")
class SkyAssetsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.assets = json.loads((ART / "assets.json").read_text())
        cls.lanes = [(ART / f"779-{part}.{part}").read_bytes() for part in ("c1", "c2")]

    def test_assets_do_not_overlap_boot_or_utility_tiles(self):
        previous = -1
        for asset in self.assets:
            self.assertGreater(asset["tile_base"], previous)
            self.assertLess(asset["tile_last"], 0xFFFD)
            self.assertFalse(asset["tile_base"] <= 0x49FF and asset["tile_last"] >= 0x4900)
            self.assertLessEqual(asset["bank_base"] + asset["palette_count"], 104)
            previous = asset["tile_last"]
        for lane in self.lanes:
            self.assertEqual(len(lane), 0x400000)
            self.assertEqual(lane[0xFFFF * 64:], bytes(64))
        # The logo uses only low colour bits, so C2 can legitimately be zero.
        self.assertTrue(any(any(lane[0x4900 * 64:0x4A00 * 64]) for lane in self.lanes))

    def test_seven_native_terrain_variants_wrap_with_equal_rows(self):
        terrain = [asset for asset in self.assets if asset["height"] == 512 and asset["name"] != "clouds"]
        self.assertEqual(len(terrain), 7)
        images = set()
        for asset in terrain:
            image = np.asarray(Image.open(ART / f'{asset["name"]}.png'))
            self.assertEqual(image.shape, (512, 352, 4))
            np.testing.assert_array_equal(image[0], image[-1])
            base = asset["tile_base"] * 64
            decoded = decode_image(*(lane[base:] for lane in self.lanes), 352, 512, swapped=True)
            np.testing.assert_array_equal(decoded[0], decoded[-1])
            images.add(image.tobytes())
        self.assertEqual(len(images), 7)

    def test_font_and_hud_are_outside_boot_page(self):
        rom = (ROOT / "games/skylance/artbox/779-s1.s1").read_bytes()
        self.assertEqual(len(rom), 0x20000)
        self.assertEqual(rom[0xFF * 32:0x100 * 32], bytes(32))
        self.assertEqual(rom[(0xD00 + ord("S")) * 32:(0xD00 + ord("S") + 1) * 32],
                         rom[ord("S") * 32:(ord("S") + 1) * 32])
        self.assertTrue(any(rom[0xE00 * 32:0xF00 * 32]))

    def test_explosion_frames_share_a_bank_without_reusing_shot_art(self):
        frames = [asset for asset in self.assets if asset["name"].startswith("explosion_")]
        self.assertEqual([asset["id"] for asset in frames], list(range(41, 47)))
        images = set()
        for asset in frames:
            self.assertEqual((asset["bank_base"], asset["palette_count"]), (99, 1))
            image = Image.open(ART / f'{asset["name"]}.png')
            self.assertEqual(image.size, (32, 32))
            images.add(image.tobytes())
        self.assertEqual(len(images), 6)

    def test_actual_package_round_trips_every_rom_lane(self):
        parts = {part: (ROOT / "roms/skylance" / f"779-{part}.{part}").read_bytes()
                 for part in installer.PARTS}
        installer.verify_parts(parts, "mvs")
        image = build_image(parts, "Sky Lance", "Eagle Software", 2026, 5, 0x779)
        installer.verify_image(image, parts)


if __name__ == "__main__":
    unittest.main()
