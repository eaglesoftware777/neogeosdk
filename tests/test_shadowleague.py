"""Shadow League's fighter data: definitions validate, cooked tables play full CPU matches."""
import subprocess, sys, tempfile, unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/shadowleague"


class ShadowLeagueTests(unittest.TestCase):
    def test_fighters_validate(self):
        subprocess.run([sys.executable, str(GAME / "tools/fighter_validate.py")], check=True)

    def test_validator_rejects_bad_data(self):
        sys.path.insert(0, str(GAME / "tools"))
        from fighter_validate import validate
        import json
        data = json.loads((GAME / "fighters/kenji.json").read_text())
        data["moves"][0]["anim"] = "missing"
        data["anims"]["jab"][0][0] = "no_such_pose"
        problems = validate(data, "kenji")
        self.assertTrue(any("unknown anim" in p for p in problems))
        self.assertTrue(any("unknown pose" in p for p in problems))

    def test_cooked_fighters_play(self):
        with tempfile.TemporaryDirectory(prefix="slf-data-") as d:
            binary = Path(d) / "slf"
            subprocess.run(["cc", "-std=c99", "-DNG_FIGHT=1", "-DSLF_ASSETS_IMPLEMENTATION=1", "-I" + str(ROOT), "-I" + str(ROOT / "sdk"),
                            "-I" + str(ROOT / "sdk/2d_engine"), str(ROOT / "tests/shadowleague_data_test.c"),
                            str(ROOT / "sdk/2d_engine/ng_fight.c"), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
