"""Host tests for bounded gameplay resources and conservative art inspection."""
import ctypes
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

from PIL import Image
from alpha_audit import inspect

GAME = Path(__file__).resolve().parents[1]


class Resources(ctypes.Structure):
    _fields_ = [(n, ctypes.c_uint8) for n in
                "power fuel shields cores turbo boosting chain extends special special_pulses".split()] + [
                    ("idle_recovery", ctypes.c_uint16), ("chain_time", ctypes.c_uint16)]


class Shot(ctypes.Structure):
    _fields_ = [("x", ctypes.c_int8), ("vx", ctypes.c_int8),
                ("damage", ctypes.c_uint8), ("art", ctypes.c_uint8)]


class Roll(ctypes.Structure):
    _fields_ = [("angle", ctypes.c_int16), ("held", ctypes.c_uint8), ("direction", ctypes.c_int8)]


class Gameplay(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        library = Path(cls.temp.name) / "combat.so"
        subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-shared", "-fPIC",
                        "-DSKY_HOST_TEST", str(GAME / "scenes/sky_combat.c"), "-o", str(library)], check=True)
        cls.lib = ctypes.CDLL(str(library))
        cls.lib.sky_score_add.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
        cls.lib.sky_score_add.restype = ctypes.c_uint32
        cls.lib.sky_weapon_pattern.argtypes = [ctypes.c_uint8, ctypes.c_uint8, ctypes.POINTER(Shot)]
        cls.lib.sky_weapon_pattern.restype = ctypes.c_uint8
        cls.lib.sky_roll_step.restype = ctypes.c_int8
        cls.lib.sky_hull_damage.restype = ctypes.c_uint16
        cls.lib.sky_flight_axis.restype = ctypes.c_int16
        cls.lib.sky_radar_contains.argtypes = [ctypes.c_int16] * 4
        cls.lib.sky_radar_contains.restype = ctypes.c_uint8

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def setUp(self):
        self.r = Resources()
        self.lib.sky_resources_init(ctypes.byref(self.r))

    def collect(self, kind):
        values = [ctypes.c_uint8(v) for v in (8, 5, 80)]
        accepted = self.lib.sky_resources_collect(ctypes.byref(self.r), kind,
                                                  *(ctypes.byref(v) for v in values))
        return accepted, [v.value for v in values]

    def test_sustained_roll_and_release(self):
        for direction in (-1, 1):
            roll = Roll()
            for _ in range(5):
                self.lib.sky_roll_step(ctypes.byref(roll), direction, 0)
            self.assertLessEqual(abs(roll.angle), 180)
            for _ in range(115):
                pose = self.lib.sky_roll_step(ctypes.byref(roll), direction, 0)
            self.assertEqual(abs(roll.angle), 720)
            self.assertEqual(ctypes.c_int8(pose).value, direction*3)
            for _ in range(120):
                self.lib.sky_roll_step(ctypes.byref(roll), 0, 0)
            self.assertEqual(roll.angle, 0)

    def test_boss_hull_and_priority_target_boundaries(self):
        for health in (180, 220, 260, 300, 340, 380, 450):
            remaining = health
            while remaining:
                next_health = self.lib.sky_hull_damage(remaining, 19)
                self.assertEqual(remaining-next_health, min(19, remaining))
                remaining = next_health
        self.assertEqual([i for i in range(10) if self.lib.sky_is_command_target(i)], [7, 8])

    def test_flight_acceleration_braking_and_special_progression(self):
        for pilot in range(3):
            velocity = 0
            for _ in range(120):
                next_velocity = self.lib.sky_flight_axis(velocity, 1, pilot, 0)
                self.assertGreaterEqual(next_velocity, velocity)
                self.assertLessEqual(next_velocity-velocity, 96)
                velocity = next_velocity
            self.assertTrue(700 <= velocity <= 832)
            for _ in range(30):
                velocity = self.lib.sky_flight_axis(velocity, 0, pilot, 0)
            self.assertEqual(velocity, 0)
            for boss in range(2):
                damage = [self.lib.sky_special_stage_damage(pilot, stage, boss) for stage in range(7)]
                self.assertEqual(damage, sorted(set(damage)))

    def test_pickup_caps(self):
        for kind in range(9):
            self.assertEqual(self.collect(kind)[0], 1)
        for _ in range(20):
            self.collect(0)
            self.collect(2)
            self.collect(5)
        self.assertEqual((self.r.power, self.r.shields, self.r.cores), (4, 2, 3))
        self.assertEqual(self.collect(1)[1][0], 9)
        self.assertEqual(self.collect(6)[1][1], 6)
        self.assertEqual(self.collect(8)[1][2], 100)
        self.assertEqual(self.collect(9)[0], 0)

    def test_fuel_counts_mission_time_without_recovery(self):
        for _ in range(59):
            self.lib.sky_resources_tick(ctypes.byref(self.r), 1)
        self.assertEqual(self.r.fuel, 240)
        self.lib.sky_resources_tick(ctypes.byref(self.r), 1)
        self.assertEqual(self.r.fuel, 239)
        for _ in range(60):
            self.lib.sky_resources_tick(ctypes.byref(self.r), 0)
        self.assertEqual(self.r.fuel, 238)
        self.r.fuel = 0
        for _ in range(180):
            self.lib.sky_resources_tick(ctypes.byref(self.r), 0)
        self.assertEqual(self.r.fuel, 0)
        self.lib.sky_resources_tick(ctypes.byref(self.r), 0)
        self.assertEqual(self.r.fuel, 0)

    def test_radar_boundary_is_a_circle_with_safe_routes(self):
        for x,y,expected in ((128,96,1),(158,96,1),(159,96,0),(149,117,1),(150,118,0),(24,96,0),(276,176,0)):
            self.assertEqual(self.lib.sky_radar_contains(x,y,128,96),expected)

    def test_special_consumed_once(self):
        self.assertEqual(self.lib.sky_special_begin(ctypes.byref(self.r)), 1)
        self.assertEqual(self.lib.sky_special_begin(ctypes.byref(self.r)), 0)
        self.assertEqual(self.r.cores, 0)
        pulses = sum(self.lib.sky_special_tick(ctypes.byref(self.r)) for _ in range(100))
        self.assertEqual(pulses, 6)
        self.assertEqual(self.r.special, 0)

    def test_all_weapon_levels(self):
        for pilot in range(3):
            for power in range(5):
                shots = (Shot * 3)()
                count = self.lib.sky_weapon_pattern(pilot, power, shots)
                self.assertTrue(1 <= count <= 3)
                self.assertEqual(shots[0].art, pilot * 5 + power)
                self.assertTrue(all(1 <= shots[i].damage <= 4 for i in range(count)))
                self.assertEqual(sum(shots[i].x for i in range(count)), 0)

    def test_score_overflow_and_finite_extends(self):
        self.assertEqual(self.lib.sky_score_add(999998, 0xffffffff), 999999)
        self.assertEqual(self.lib.sky_score_add(0xffffffff, 1), 999999)
        lives = ctypes.c_uint8(0)
        for _ in range(100):
            self.lib.sky_score_extends(ctypes.byref(self.r), ctypes.c_uint32(999999), ctypes.byref(lives))
        self.assertEqual((lives.value, self.r.extends), (3, 3))

    def test_death_preserves_earned_resources(self):
        self.r.power, self.r.fuel, self.r.shields = 4, 0, 2
        self.lib.sky_resources_death(ctypes.byref(self.r))
        self.assertEqual((self.r.power, self.r.fuel, self.r.shields), (3, 0, 0))
        self.assertEqual(self.r.cores, 1)


class Alpha(unittest.TestCase):
    def test_report_never_changes_source(self):
        original = Image.new("RGBA", (5, 5), (80, 90, 100, 255))
        original.putpixel((2, 2), (0, 0, 0, 0))
        result, report = inspect(original)
        self.assertEqual(result.tobytes(), original.tobytes())
        self.assertEqual(report["transparent_before"], 1)

    def test_only_uniform_single_pixel_is_repaired(self):
        original = Image.new("RGBA", (5, 5), (80, 90, 100, 255))
        original.putpixel((2, 2), (0, 0, 0, 0))
        repaired, _ = inspect(original, True)
        self.assertEqual(repaired.getpixel((2, 2)), (80, 90, 100, 255))
        original.putpixel((2, 1), (150, 90, 100, 255))
        repaired, report = inspect(original, True)
        self.assertEqual(repaired.tobytes(), original.tobytes())
        self.assertFalse(report["regions"][0]["automatic_candidate"])


class Effects(unittest.TestCase):
    def test_projectile_assets_fit_resident_slots(self):
        assets = {a["id"]: a for a in json.loads((GAME / "artbox/generated/assets.json").read_text())}
        # All fifteen pilot weapon drawings and all eight hostile drawings,
        # including the guided missile also used by the player's secondary.
        for asset_id in list(range(57, 72)) + list(range(111, 119)):
            with self.subTest(asset=assets[asset_id]["name"]):
                self.assertEqual(assets[asset_id]["width"], 16)

    def test_generated_effects_exist_and_match_manifest(self):
        manifest = GAME / "sound/diagnostics/effects.json"
        if not manifest.exists():
            self.skipTest("Generate effects with tools/sfx.py first")
        rows = json.loads(manifest.read_text())
        self.assertEqual([r["id"] for r in rows], list(range(1, 17)))
        for row in rows:
            source = GAME / row["source"]
            encoded = GAME / f'sound/samples/out_a/{row["id"]}.adpcma'
            self.assertEqual(hashlib.sha256(source.read_bytes()).hexdigest(), row["sha256"])
            self.assertEqual(hashlib.sha256(encoded.read_bytes()).hexdigest(), row["encoded_sha256"])
            self.assertEqual(encoded.stat().st_size % 256, 0)
            self.assertEqual(row["clipped_samples"], 0)
            self.assertLess(abs(row["dc"]), 150)
            self.assertGreater(row["decoded_rms"], 100)


if __name__ == "__main__":
    unittest.main()
