"""Exercise ng_fight (input, combat, rounds, CPU determinism) on the host."""
import subprocess, tempfile, unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class FightTests(unittest.TestCase):
    def test_fight_core(self):
        with tempfile.TemporaryDirectory(prefix="ng-fight-") as d:
            binary = Path(d) / "fight"
            subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-DNG_FIGHT=1",
                            "-I" + str(ROOT / "sdk"), "-I" + str(ROOT / "sdk/2d_engine"),
                            str(ROOT / "tests/fight_test.c"), str(ROOT / "sdk/2d_engine/ng_fight.c"),
                            "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
