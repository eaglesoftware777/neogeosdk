"""Direct Sky Lance ROM data, boot isolation and cartridge package checks."""

import importlib.util
import json
import re
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
GAME = ROOT / "games/skylance"


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
            self.assertLessEqual(asset["bank_base"] + asset["palette_count"], 128)
            previous = asset["tile_last"]
        for lane in self.lanes:
            self.assertEqual(len(lane), 0x400000)
            self.assertEqual(lane[0xFFFF * 64:], bytes(64))
        # The logo uses only low colour bits, so C2 can legitimately be zero.
        self.assertTrue(any(any(lane[0x4900 * 64:0x4A00 * 64]) for lane in self.lanes))

    def test_seven_long_routes_have_distinct_pages_and_shared_palettes(self):
        terrain = [a for a in self.assets if a["height"] == 512 and not a["name"].startswith("clouds")]
        expected_parts=(8,4,4,4,4,4,4)
        self.assertEqual(len(terrain),sum(expected_parts))
        header = (ART / "sky_assets.h").read_text()
        palettes = {int(i): words for i, words in re.findall(
            r"sky_asset_(\d+)_pal\[\] = \{([^}]+)\}", header)}
        for stage, base in enumerate((2, 1, 30, 29, 31, 32, 33)):
            family = [base, *[next(a['id'] for a in self.assets if a['name']==f'route_{stage}_{p}')
                             for p in range(1,expected_parts[stage])]]
            self.assertEqual(len({palettes[i] for i in family}), 1)
            images = set()
            for i in family:
                asset = self.assets[i-1]
                image = np.asarray(Image.open(ART / (asset["name"]+".png")))
                self.assertEqual(image.shape, (512, 352, 4))
                self.assertTrue((image[:, :, 3] == 255).all())
                decoded = decode_image(*(lane[asset["tile_base"]*64:] for lane in self.lanes), 352, 512, swapped=True)
                self.assertTrue((decoded != 0).all())
                images.add(image.tobytes())
            self.assertEqual(len(images),expected_parts[stage])

    def test_unit_canvases_and_distinct_military_weapons(self):
        names = {a["name"]: a for a in self.assets}
        # Units are drawn by class: a small drone, full-size jets and
        # helicopters, a heavy bomber, long vehicles on the ground, and
        # boats and submarines with their whole hull and wake.
        sizes = {"drone": (32, 32), "fighter": (48, 48), "heli": (48, 48), "bomber": (64, 64),
                 "tank": (32, 48), "sam": (32, 48), "radar_site": (32, 48),
                 "interceptor": (48, 48), "missile_boat": (32, 64), "submarine": (32, 64)}
        for name, size in sizes.items():
            self.assertEqual((names[name]["width"], names[name]["height"]), size, name)
        weapons = [a for a in self.assets if a["name"].startswith("hostile_")]
        self.assertEqual(len(weapons), 8)
        self.assertEqual(len({Image.open(ART / (a["name"]+".png")).tobytes() for a in weapons}), 8)
        self.assertEqual(len([a for a in self.assets if a["name"].startswith("maneuver_")]), 12)

    def test_font_and_hud_are_outside_boot_page(self):
        rom = (ROOT / "games/skylance/artbox/779-s1.s1").read_bytes()
        self.assertEqual(len(rom), 0x20000)
        self.assertEqual(rom[0xFF * 32:0x100 * 32], bytes(32))
        # The game's ASCII page holds Sky Lance's own face (tools/hud_art.py);
        # the low page keeps the BIOS font for the BIOS's menus.
        spec = importlib.util.spec_from_file_location("sky_hud_art", ROOT / "games/skylance/tools/hud_art.py")
        hud = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(hud)
        spec = importlib.util.spec_from_file_location("sky_sfix", ROOT / "bios/tools/gen_sfix.py")
        sfix = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(sfix)
        for ch in "S0:":
            tile = (0xD00 + ord(ch)) * 32
            self.assertEqual(rom[tile:tile + 32], bytes(sfix.encode_tile(hud.text_tile(ch))), ch)
        self.assertEqual(rom[ord("S") * 32:(ord("S") + 1) * 32], sfix.build_rom()[ord("S") * 32:(ord("S") + 1) * 32])
        self.assertTrue(any(rom[0xE00 * 32:0xF00 * 32]))
        empty = rom[0xCE0 * 32:0xCE1 * 32]
        filled = rom[0xCE1 * 32:0xCE2 * 32]
        self.assertTrue(any(empty))
        self.assertTrue(any(filled))
        self.assertNotEqual(empty, filled)
        header = (ROOT / "games/skylance/artbox/infix_palettes.h").read_text()
        self.assertIn("#define INFIX_IMAGE_COUNT 24u", header)

    def test_atomic_fade_can_fit_inside_the_upload_window(self):
        renderer = (ROOT / "sdk/2d_engine/ng_sprite_group.h").read_text()
        deadline = int(re.search(r"#define NG_VRAM_DEADLINE\s+(\d+)u", renderer)[1])
        draw = (ROOT / "games/skylance/scenes/sky_draw.c").read_text()
        count = int(re.search(r"ng_palfx_screen_init\([^;]+,\s*(\d+)u\)", draw)[1])
        # ng_palfx_vblank reserves two lines plus one per four banks.
        # Even at line 240, only the final 24 lines plus deadline remain.
        self.assertLessEqual(2 + count // 4, 24 + deadline)

    def test_damage_smoke_and_stage_clouds_are_distinct(self):
        smoke = [a for a in self.assets if a["name"].startswith("smoke_")]
        self.assertEqual([a["id"] for a in smoke], list(range(84, 88)))
        clouds = [a for a in self.assets if a["name"].startswith("clouds")]
        self.assertEqual(len(clouds), 7)
        self.assertEqual(len({Image.open(ART / (a["name"]+".png")).tobytes()
                              for a in clouds}), 7)
        for asset in smoke + clouds:
            image = np.asarray(Image.open(ART / (asset["name"]+".png")))
            decoded = decode_image(*(lane[asset["tile_base"]*64:] for lane in self.lanes),
                                   asset["width"], asset["height"], swapped=True)
            np.testing.assert_array_equal(decoded != 0, image[:, :, 3] != 0)
            self.assertTrue((image[:, :, 3] == 0).any())

    def test_explosion_frames_share_a_bank_without_reusing_shot_art(self):
        # Eight painted frames: the first six keep IDs 41-46, the last two
        # come after the legacy roster; all load the same two banks.
        frames = [asset for asset in self.assets if asset["name"].startswith("explosion_")]
        self.assertEqual([asset["name"] for asset in frames], [f"explosion_{k}" for k in range(8)])
        self.assertEqual([asset["id"] for asset in frames[:6]], list(range(41, 47)))
        self.assertEqual(frames[7]["id"], frames[6]["id"] + 1)
        self.assertGreater(frames[6]["id"], 193)
        images = set()
        for asset in frames:
            self.assertEqual((asset["bank_base"], asset["palette_count"]), (98, 2))
            image = Image.open(ART / f'{asset["name"]}.png')
            self.assertEqual(image.size, (64, 64))
            images.add(image.tobytes())
        self.assertEqual(len(images), 8)

    def test_actual_package_round_trips_every_rom_lane(self):
        parts = {part: (ROOT / "roms/skylance" / f"779-{part}.{part}").read_bytes()
                 for part in installer.PARTS}
        installer.verify_parts(parts, "mvs")
        image = build_image(parts, "Sky Lance", "Eagle Software", 2026, 5, 0x779)
        installer.verify_image(image, parts)

    def test_animation_families_have_identical_palette_words(self):
        header = (ART / "sky_assets.h").read_text()
        palettes = {int(i): words for i, words in re.findall(
            r"sky_asset_(\d+)_pal\[\] = \{([^}]+)\}", header)}
        # Each unit strip (idle, damaged, destruction) loads one palette.
        ids = {a["name"]: a["id"] for a in self.assets}
        for unit in ("drone", "fighter", "heli", "tank", "bomber", "sam", "radar_site", "miniboss_0"):
            family = [ids[unit]] + [ids[f"{unit}_{k}"] for k in range(1, 8)]
            self.assertEqual(len({palettes[i] for i in family}), 1, unit)
            self.assertEqual(len({self.assets[i-1]["bank_base"] for i in family}), 1, unit)
        # Units that share the screen in Stage 1 never share a palette bank.
        stage_one = ("drone", "fighter", "heli", "tank", "bomber", "sam", "radar_site", "miniboss_0")
        self.assertEqual(len({self.assets[ids[u]-1]["bank_base"] for u in stage_one}), len(stage_one))
        for pilot, neutral in enumerate((4, 6, 8)):
            family = (neutral, 35+pilot*2, 36+pilot*2, *range(72+pilot*4, 76+pilot*4))
            self.assertEqual(len({palettes[i] for i in family}), 1)
            self.assertEqual(len({Image.open(ART / f'{self.assets[i-1]["name"]}.png').tobytes()
                                  for i in family}), 7)
        for pilot in range(3):
            family = range(57 + pilot * 5, 62 + pilot * 5)
            self.assertEqual(len({palettes[i] for i in family}), 1)
        for pilot in range(3):
            family = (119+pilot, *range(170+pilot*8,178+pilot*8))
            self.assertEqual(len({palettes[i] for i in family}),1)
            self.assertEqual(len({Image.open(ART / (self.assets[i-1]['name']+'.png')).tobytes()
                                  for i in family[1:]}),8)

    def test_new_pickups_and_weapon_frames_are_native_and_complete(self):
        self.assertEqual([a["id"] for a in self.assets if a["name"].startswith("pickup_")],
                         list(range(48, 57)))
        weapons = [a for a in self.assets if a["name"].startswith("weapon_")]
        self.assertEqual(len(weapons), 15)
        for asset in self.assets[47:71]:
            self.assertEqual((asset["width"], asset["height"]), (16, 16))
            decoded = decode_image(*(lane[asset["tile_base"] * 64:] for lane in self.lanes),
                                   16, 16, swapped=True)
            self.assertLessEqual(int(decoded.max()), 15)
            alpha = np.asarray(Image.open(ART / f'{asset["name"]}.png'))[:, :, 3]
            np.testing.assert_array_equal(decoded != 0, alpha > 0)

    def test_redrawn_roster_is_complete_with_transparent_margins(self):
        sprites = [a for a in self.assets if a["name"].startswith("p") and
                   ("plane" in a["name"] or "pilot" in a["name"])]
        self.assertEqual(len(sprites), 24)
        sprites += [a for a in self.assets if a["id"] in
                    (10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 25)]
        for asset in sprites:
            image = np.asarray(Image.open(ART / f'{asset["name"]}.png'))
            alpha = image[:, :, 3]
            self.assertTrue((alpha > 0).any(), asset["name"])
            self.assertFalse(alpha[0].any() or alpha[-1].any() or
                             alpha[:, 0].any() or alpha[:, -1].any(), asset["name"])
            decoded = decode_image(*(lane[asset["tile_base"]*64:] for lane in self.lanes),
                                   asset["width"], asset["height"], swapped=True)
            np.testing.assert_array_equal(decoded != 0, alpha != 0)
            if "plane" in asset["name"]:
                _, nose = np.where(alpha[:8] > 0)
                self.assertLessEqual(abs(float(np.median(nose))-alpha.shape[1]/2), 3,
                                     asset["name"])


    def test_painted_families_share_their_palettes(self):
        """A painted family (a jet's poses, a boss and its turrets) is fitted
        as one picture: every member loads the same palettes into the same
        banks, so changing frame never recolours a live sprite."""
        manifest = GAME / "assets/painted/painted.json"
        if not manifest.exists():
            self.skipTest("no painted assets")
        header = (ART / "sky_assets.h").read_text()
        palettes = {int(i): words for i, words in re.findall(
            r"sky_asset_(\d+)_pal\[\] = \{([^}]+)\}", header)}
        by_name = {a["name"]: a for a in self.assets}
        for family in json.loads(manifest.read_text()):
            members = [by_name[n] for n in family["members"]]
            self.assertEqual({a["bank_base"] for a in members}, {family["bank"]}, family["name"])
            self.assertEqual(len({palettes[a["id"]] for a in members}), 1, family["name"])
            for a in members:
                self.assertLessEqual(a["palette_count"], family["banks"], a["name"])
                self.assertLessEqual(a["bank_base"] + a["palette_count"], 128, a["name"])
            if family["name"].startswith("pilot_"):
                plane = by_name[f'p{int(family["name"][-1]) + 1}_plane']
                self.assertEqual((plane["width"], plane["height"]), (64, 64))
                self.assertEqual(plane["palette_count"], 4)

if __name__ == "__main__":
    unittest.main()
