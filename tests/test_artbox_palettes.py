"""Hardware format, palette budget, and source-fidelity regression checks."""

import io
import sys
import unittest
from unittest.mock import patch
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "artbox"))
from img2neo import rgb_to_lab
from img2neo_tile import ng_snap
from palette_banks import fit_palette, palette_words, quantize, reconstruct
from tile_codec import HALF_SOLID_TILE, decode_image, encode_image, write_utility_tiles
from romdbimgimport import allocate_extra_palettes, load_source_sprite, sprite_palette_group_key

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from verify_artbox_palettes import validate_palette_ownership


class PaletteTests(unittest.TestCase):
    def test_sprite_import_honors_budget_without_changing_geometry(self):
        rgba = np.full((32, 64, 4), 255, dtype=np.uint8)
        rgba[:, :, :3] = np.random.default_rng(31).integers(0, 256, (32, 64, 3), dtype=np.uint8)
        rgba[:2] = 0
        spec = {"name": "boss.png", "dither": "none", "palette_banks": 4}
        with patch("romdbimgimport.prepare_source_sprite", return_value=(rgba, {})):
            pixels, _ = load_source_sprite(spec)
        self.assertEqual(pixels.shape, (32, 64))
        self.assertTrue(np.all(pixels[:2] == 0))
        self.assertGreater(len(spec["extra_palettes"]), 0)
        self.assertLessEqual(len(spec["extra_palettes"]), 3)
        self.assertEqual(len(spec["tile_palette_offsets"]), 8)

    def test_animation_master_stays_single_bank(self):
        rgba = np.full((16, 16, 4), 255, dtype=np.uint8)
        master = fit_palette(rgba[:, :, :3])
        spec = {"dither": "none", "palette_banks": 8}
        with patch("romdbimgimport.prepare_source_sprite", return_value=(rgba, {})):
            _, palette = load_source_sprite(spec, master)
        np.testing.assert_array_equal(palette[1:], master)
        self.assertNotIn("extra_palettes", spec)

    def test_unrelated_pilots_and_craft_do_not_share_an_animation_palette(self):
        def key(name):
            return sprite_palette_group_key({"subdir": "characters", "name": name})
        self.assertNotEqual(key("sprite_p1_plane.png"), key("sprite_p1_pilot.png"))
        self.assertNotEqual(key("sprite_p1_plane.png"), key("sprite_p2_plane.png"))
        self.assertEqual(key("010_ship.png"), key("011_ship_alt.png"))
        self.assertEqual(key("sprite_076_r07_c11.png"), key("sprite_077_r07_c12.png"))

    def test_base_and_extra_banks_share_only_identical_ordered_words(self):
        first, second = list(range(16)), list(range(16, 32))
        third = list(reversed(second))
        specs = [{"db_index": i, "name": str(i), "canvas_width": 32,
                  "canvas_height": 32} for i in range(2)]
        specs[0].update(extra_palettes=[second, third], tile_palette_offsets=[0, 1, 2, 1])
        specs[1].update(extra_palettes=[first, third], tile_palette_offsets=[1, 0, 2, 1])
        bases = {0: first, 1: second}
        allocate_extra_palettes(specs, bases)
        self.assertEqual(specs[0]["palette_slots"], [16, 17, 18])
        self.assertEqual(specs[1]["palette_slots"], [17, 16, 18])
        self.assertEqual(validate_palette_ownership(specs, bases), {16, 17, 18})
        specs[1]["extra_palettes"][1] = second
        with self.assertRaisesRegex(ValueError, "different palettes"):
            validate_palette_ownership(specs, bases)

    def test_claimed_hash_does_not_allow_conflicting_palettes(self):
        specs = [{"db_index": i, "name": str(i), "palette_bank": 16,
                  "palette_key": "same claimed hash"} for i in range(2)]
        with self.assertRaisesRegex(ValueError, "different palettes"):
            validate_palette_ownership(specs, {0: list(range(16)), 1: list(range(16, 32))})

    def test_palette_budget_cannot_overwrite_backdrop(self):
        specs = [{"db_index": i, "name": str(i)} for i in range(240)]
        with self.assertRaisesRegex(ValueError, "budget exceeded"):
            allocate_extra_palettes(specs)

    def test_utility_tiles_preserve_art_and_blank_padding(self):
        c1, c2 = io.BytesIO(), io.BytesIO()
        c1.write(b"\x12" * 64)
        c2.write(b"\x34" * 64)
        write_utility_tiles(c1, c2)
        self.assertEqual(c1.getvalue()[:64], b"\x12" * 64)
        start = HALF_SOLID_TILE * 64
        pixels = decode_image(c1.getvalue()[start:], c2.getvalue()[start:], 48, 16)
        self.assertTrue(np.all(pixels[:8, :16] == 1))
        self.assertTrue(np.all(pixels[8:, :16] == 0))
        self.assertTrue(np.all(pixels[:, 16:32] == 1))
        self.assertTrue(np.all(pixels[:, 32:] == 0))
        c1.seek(start + 1)
        c2.seek(start + 1)
        with self.assertRaises(ValueError):
            write_utility_tiles(c1, c2)

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
