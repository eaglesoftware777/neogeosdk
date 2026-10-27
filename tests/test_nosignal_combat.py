"""No Signal's moves on the engine: chains land, boxes fit the pictures (tests/nosignal_combat_test.c,
games/nosignal/tools/boxes.py)."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NoSignalCombat(unittest.TestCase):
    def test_chains_and_reach(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / "combat"
            subprocess.run(["cc", "-std=c99", "-Wall", "-I", str(ROOT), "-I", str(ROOT / "sdk"),
                            "-I", str(ROOT / "sdk/2d_engine"), "-I", str(ROOT / "games/nosignal/scenes"),
                            str(ROOT / "tests/nosignal_combat_test.c"), str(ROOT / "sdk/2d_engine/ng_beatemup.c"),
                            "-o", str(exe)], check=True, capture_output=True, text=True)
            r = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)

    def test_boxes_fit_the_pictures(self):
        r = subprocess.run([sys.executable, str(ROOT / "games/nosignal/tools/boxes.py"), "--check"],
                           capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)


if __name__ == "__main__":
    unittest.main()
