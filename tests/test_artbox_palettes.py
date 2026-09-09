"""Hardware format, palette budget, and source-fidelity regression checks."""

import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "artbox"))
from img2neo import rgb_to_lab
from img2neo_tile import ng_snap
from palette_banks import fit_palette, palette_words, quantize, reconstruct
from tile_codec import decode_image, encode_image


class PaletteTests(unittest.TestCase):
    def test_transparent_padding_and_opaque_black(self):
        rgba = np.zeros((32, 48, 4), dtype=np.uint8)
        rgba[:, :, :3] = (255, 0, 255)
        rgba[4:20, 9:25] = (0, 0, 0, 255)
        indices, banks, tiles = quantize(rgba, 4)
        self.assertTrue(np.all(indices[rgba[:, :, 3] == 0] == 0))
        self.assertTrue(np.all(indices[rgba[:, :, 3] == 255] != 0))
        self.assertTrue(np.all(reconstruct(indices, banks, tiles)[4:20, 9:25] == 0))

    def test_small_pixel_art_palette_is_exact_on_hardware(self):
        pixels = ng_snap(np.array([[0, 0, 0], [255, 255, 255], [220, 36, 80], [30, 90, 210]], dtype=np.uint8))
        palette = fit_palette(np.repeat(pixels, [100, 1, 70, 12], axis=0))
        for color in pixels:
            self.assertTrue(np.any(np.all(palette == color, axis=1)))

    def test_multiple_banks_improve_global_fit_and_are_deterministic(self):
        rgba = np.full((32, 64, 4), 255, dtype=np.uint8)
        rng = np.random.default_rng(19)
        rgba[:, :, :3] = rng.integers(0, 256, (32, 64, 3), dtype=np.uint8)
        one = quantize(rgba, 1)
        many = quantize(rgba, 4)
        repeat = quantize(rgba, 4)
        for a, b in zip(many, repeat):
            np.testing.assert_array_equal(a, b)
        source = rgb_to_lab(rgba[:, :, :3])
        error_one = np.mean((source - rgb_to_lab(reconstruct(*one))) ** 2)
        error_many = np.mean((source - rgb_to_lab(reconstruct(*many))) ** 2)
        self.assertLess(error_many, error_one)
        self.assertLessEqual(len(many[1]), 4)
        self.assertLess(int(many[2].max()), len(many[1]))
        np.testing.assert_array_equal(one[1][0], many[1][0])

    def test_semitransparent_edges_do_not_steal_core_colors(self):
        rgba = np.zeros((16, 16, 4), dtype=np.uint8)
        rgba[:] = (255, 240, 240, 180)
        rgba[4:12, 4:12] = (24, 60, 100, 255)
        _, banks, _ = quantize(rgba)
        self.assertTrue(np.all(banks[0, 1:] == ng_snap(np.array([24, 60, 100], dtype=np.uint8))))

    def test_palette_word_round_trip(self):
        for first in range(0, 65536, 1024):
            words = np.arange(first, first + 1024, dtype=np.uint32)
            parity = 1 - (words >> 15)
            rgb5 = np.stack((((words >> 7) & 30) | ((words >> 14) & 1),
                             ((words >> 3) & 30) | ((words >> 13) & 1),
                             ((words << 1) & 30) | ((words >> 12) & 1)), axis=1)
            rgb6 = (rgb5 << 1) | parity[:, None]
            rgb8 = ((rgb6 << 2) | (rgb6 >> 4)).astype(np.uint8)
            packed = palette_words(np.concatenate((np.zeros((1, 3), dtype=np.uint8), rgb8)))
            np.testing.assert_array_equal(packed[1:], words)

    def test_tiny_antialiased_craft_keeps_more_than_highlights(self):
        rgba = np.zeros((16, 16, 4), dtype=np.uint8)
        rgba[4:12, 4:12] = (20, 60, 90, 180)
        rgba[8, 8] = (255, 200, 90, 255)
        indices, banks, mapping = quantize(rgba)
        rendered = reconstruct(indices, banks, mapping)
        np.testing.assert_array_equal(rendered[4, 4], ng_snap(rgba[4, 4, :3]))

    def test_rectangular_tiles_round_trip_and_reservations(self):
        for width, height in ((16, 32), (48, 16), (32, 48), (256, 256)):
            pixels = np.arange(width * height, dtype=np.uint32).reshape(height, width) % 16
            c1, c2 = encode_image(pixels)
            self.assertEqual(len(c1), 256 * 64)
            np.testing.assert_array_equal(decode_image(c1, c2, width, height), pixels)
            used = width * height // 4
            self.assertFalse(any(c1[used:]) or any(c2[used:]))
            swapped1 = np.frombuffer(c1, dtype=np.uint8).reshape(-1, 2)[:, ::-1].tobytes()
            swapped2 = np.frombuffer(c2, dtype=np.uint8).reshape(-1, 2)[:, ::-1].tobytes()
            np.testing.assert_array_equal(decode_image(swapped1, swapped2, width, height, True), pixels)

    def test_invalid_pixel_indices_are_not_silently_truncated(self):
        with self.assertRaises(ValueError):
            encode_image(np.full((16, 16), 16))
        with self.assertRaises(ValueError):
            quantize(np.zeros((17, 16, 4), dtype=np.uint8))


if __name__ == "__main__":
    unittest.main()
