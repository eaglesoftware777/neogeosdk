"""Chroma-key regression and native No Signal C-ROM alpha integrity."""
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import unittest

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/nosignal"
spec = importlib.util.spec_from_file_location("ns_import", GAME / "tools/import_sheets.py")
ns_import = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ns_import)
sys.path.insert(0, str(ROOT / "artbox"))
from tile_codec import decode_image


class NoSignalMatteTests(unittest.TestCase):
    def test_purple_clothing_is_not_a_background_key(self):
        rgb = np.full((40, 40, 3), (251, 2, 250), dtype=np.uint8)
        rgb[5:35, 5:35] = (24, 20, 32)  # outline
        rgb[7:33, 7:33] = (156, 78, 188)  # hoodie: the old key erased this
        guide = ~ns_import.magenta(rgb)
        matte = ns_import.figure_matte(rgb, guide)
        self.assertTrue(matte[7:33, 7:33].all())
        self.assertFalse(matte[:5].any())
        self.assertTrue(np.all(matte[guide]))
        exported = ns_import.reduce(rgb, matte, 1)
        np.testing.assert_array_equal(exported[15, 15], [156, 78, 188, 255])

    def test_enclosed_gaps_between_limbs_remain_transparent(self):
        rgb = np.full((40, 40, 3), (251, 2, 250), dtype=np.uint8)
        rgb[5:35, 5:35] = (24, 20, 32)
        rgb[7:33, 7:33] = (156, 78, 188)
        rgb[14:20, 14:20] = (251, 2, 250)
        # A less saturated key fringe is connected to the true key.
        rgb[13, 14:20] = (200, 60, 208)
        guide = ~ns_import.magenta(rgb)
        matte = ns_import.figure_matte(rgb, guide)
        self.assertFalse(matte[13:20, 14:20].any())
        self.assertTrue(matte[8:12, 8:12].all())

    def test_unseeded_purple_highlight_stays_opaque(self):
        rgb = np.full((16, 16, 3), (140, 65, 170), dtype=np.uint8)
        self.assertFalse(ns_import.background_mask(rgb).any())
        guide = np.zeros((16, 16), dtype=bool)
        guide[8, 8] = True
        self.assertTrue(ns_import.figure_matte(rgb, guide).all())


class NoSignalRomTests(unittest.TestCase):
    def test_recovered_clothing_pixels_remain_solid(self):
        # Interior fabric samples erased by the former whole-image key.
        for name, x, y in (("kai", 85, 64), ("zoe", 50, 70), ("archivist", 85, 114)):
            with self.subTest(fighter=name):
                rgba = np.array(Image.open(GAME / f"artbox/painted/fighters/{name}/00.png"))
                self.assertEqual(rgba[y, x, 3], 255)

    def test_no_additional_poses_touch_canvas_edges(self):
        # Existing oversized weapon/effect poses already reach the canvas.
        # Preserve their geometry while preventing new clipping elsewhere.
        existing = set("""alien/05 archivist/26 behemoth/12 behemoth/14 bigflex/05
            m08/05 m08/10 m08/11 m08/13 m08/14 m08/15 m08/18 m08/19 overmind/02
            proxy/05 proxy/10 proxy/11 proxy/12 proxy/15 runner/05""".split())
        for path in (GAME / "artbox/painted/fighters").glob("*/*.png"):
            alpha = np.array(Image.open(path))[..., 3]
            if alpha[[0, -1]].any() or alpha[:, [0, -1]].any():
                self.assertIn(f"{path.parent.name}/{path.stem}", existing)

    def test_all_fighter_frames_retain_exact_alpha_in_encoded_rom(self):
        manifest = json.loads((GAME / "artbox/generated/assets.json").read_text())
        lanes = []
        for part in ("c1", "c2"):
            lane = (GAME / f"artbox/784-{part}.{part}").read_bytes()
            self.assertEqual(len(lane), 0x400000)
            lanes.append(np.frombuffer(lane, np.uint8).reshape(-1, 2)[:, ::-1].tobytes())
        checked = 0
        for asset in manifest["assets"]:
            if not asset["name"].startswith("f_"):
                continue
            name = asset["name"][2:]
            size = ns_import.FRAME_PX.get(name, 128)
            self.assertEqual(asset["frames"], ns_import.SHEETS[name] * 10)
            self.assertLessEqual(asset["banks"], 8)
            for frame in range(asset["frames"]):
                with self.subTest(fighter=name, frame=frame):
                    rgba = np.array(Image.open(GAME / f"artbox/painted/fighters/{name}/{frame:02d}.png"))
                    self.assertEqual(rgba.shape, (size, size, 4))
                    self.assertTrue(np.isin(rgba[..., 3], [0, 255]).all())
                    start = (asset["tile_base"] + frame * asset["frame_tiles"]) * 64
                    end = start + asset["frame_tiles"] * 64
                    pixels = decode_image(lanes[0][start:end], lanes[1][start:end], size, size)
                    np.testing.assert_array_equal(pixels != 0, rgba[..., 3] == 255)
                    checked += 1
        self.assertEqual(checked, 420)

    def test_fighters_stand_out_from_their_streets(self):
        r = subprocess.run([sys.executable, str(GAME / "tools/contrast.py"), "--check"], capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)


if __name__ == "__main__":
    unittest.main()
