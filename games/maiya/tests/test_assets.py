"""Validate generated C-ROM boundaries, alpha and native sprite encoding."""

import json
from pathlib import Path
import sys
import unittest

import numpy as np
from PIL import Image

GAME = Path(__file__).resolve().parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(GAME.parents[1] / "artbox"))
from tile_codec import decode_image, encode_image


class AssetsTest(unittest.TestCase):
    def test_codec_roundtrip(self):
        image = np.random.default_rng(7).integers(0, 16, (64, 96), dtype=np.uint8)
        lo, hi = encode_image(image, 24)
        np.testing.assert_array_equal(decode_image(lo, hi, 96, 64), image)

    def test_generated_roms(self):
        directory = GAME / "artbox/generated"
        manifest = json.loads((directory / "assets.json").read_text())
        lo = (directory / "780-c1.c1").read_bytes()
        hi = (directory / "780-c2.c2").read_bytes()
        self.assertEqual(len(lo), 0x400000)
        self.assertEqual(len(lo), len(hi))
        self.assertEqual(lo[-64:], bytes(64))
        self.assertEqual(hi[-64:], bytes(64))
        previous = -1
        for entry in manifest:
            self.assertGreater(entry["tile_base"], previous)
            previous = entry["tile_last"]
            start = entry["tile_base"] * 64
            end = (entry["tile_last"] + 1) * 64
            width, height = entry["strips"] * 16, entry["rows"] * 16
            pixels = decode_image(lo[start:end], hi[start:end], width, height, swapped=True)
            rgba = np.asarray(Image.open(GAME / "artbox/in/generated" / (entry["name"] + ".png")))
            np.testing.assert_array_equal(pixels == 0, rgba[:, :, 3] < 128)
        frames = [e for e in manifest if e["name"].startswith("hero_")]
        self.assertEqual(len(frames), 39)
        self.assertTrue(all(e["strips"] == 5 and e["rows"] == 4 for e in frames))
        eagle = [e for e in manifest if e["name"].startswith("eagle_")]
        self.assertEqual(len(eagle), 6)
        for i in range(6):
            far = next(e for e in manifest if e["name"] == f"bg{i}")
            road = next(e for e in manifest if e["name"] == f"ground{i}")
            self.assertEqual((far["strips"], far["rows"]), (32, 12))
            self.assertEqual((road["strips"], road["rows"]), (32, 2))
            self.assertEqual(far["palette_count"], road["palette_count"])

    def test_guardian_scale_and_feet(self):
        directory = GAME / "artbox/generated"
        for name in ("beetle", "toad", "jackal", "owl", "leviathan", "smoggar"):
            extents = []
            for frame in range(2):
                image = Image.open(directory / f"boss_{name}_{frame}.png").convert("RGBA")
                box = image.getchannel("A").getbbox()
                self.assertIsNotNone(box)
                self.assertEqual(box[3], 94)
                extents.append(max(box[2] - box[0], box[3] - box[1]))
            self.assertLessEqual(abs(extents[0] - extents[1]), 2, name)


if __name__ == "__main__":
    unittest.main()
