"""The v1.7.7 genre engines: host logic, 68000 code, and the build selector."""
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "sdk/2d_engine"
FLAGS = ["-std=c99", "-Wall", "-Wextra", "-Werror", "-Wno-missing-field-initializers",
         "-I" + str(ROOT / "sdk"), "-I" + str(ENGINE)]
TESTS = {
    "puzzle": ["tests/puzzle_test.c", "sdk/2d_engine/ng_puzzle.c", "sdk/2d_engine/ng_trig.c", "sdk/2d_engine/ng_trig_table.c"],
    "mahjong": ["tests/mahjong_test.c", "sdk/2d_engine/ng_mahjong.c"],
    "rpg": ["tests/rpg_test.c", "sdk/2d_engine/ng_rpg.c"],
    "beatemup": ["tests/beatemup_test.c", "sdk/2d_engine/ng_beatemup.c"],
}


def m68k_gcc():
    sdkhome = Path(os.environ.get("SDKHOME", ROOT.parent))
    for d in ("x-tools-v3", "x-tools-v2", "x-tools"):
        gcc = sdkhome / d / "m68k-unknown-elf/bin/m68k-unknown-elf-gcc"
        if gcc.is_file():
            return gcc
    return None


class GenreEngineTests(unittest.TestCase):
    def run_host(self, name):
        with tempfile.TemporaryDirectory(prefix="ng-genre-") as d:
            binary = Path(d) / name
            subprocess.run(["cc"] + FLAGS + [str(ROOT / s) for s in TESTS[name]] + ["-o", str(binary)], check=True)
            out = subprocess.run([str(binary)], check=True, capture_output=True, text=True).stdout
            self.assertIn("all tests passed", out)

    def test_puzzle(self):
        self.run_host("puzzle")

    def test_mahjong(self):
        self.run_host("mahjong")

    def test_rpg(self):
        self.run_host("rpg")

    def test_beatemup(self):
        self.run_host("beatemup")

    def test_doc_examples(self):
        """The four minimal programs quoted in docs/GENRE_ENGINES_V1.7.7.md."""
        with tempfile.TemporaryDirectory(prefix="ng-genre-doc-") as d:
            binary = Path(d) / "genre_examples"
            sources = ["tests/genre_examples.c"] + [s for name in TESTS for s in TESTS[name][1:]]
            subprocess.run(["cc"] + FLAGS + [str(ROOT / s) for s in dict.fromkeys(sources)] + ["-o", str(binary)],
                           check=True)
            out = subprocess.run([str(binary)], check=True, capture_output=True, text=True).stdout
            self.assertIn("all tests passed", out)

    def test_cpp14_consumer(self):
        if not shutil.which("c++"):
            self.skipTest("no host C++ compiler")
        with tempfile.TemporaryDirectory(prefix="ng-genre-cpp-") as d:
            objs = []
            for src in ("ng_puzzle", "ng_mahjong", "ng_rpg", "ng_beatemup", "ng_trig", "ng_trig_table"):
                obj = Path(d) / (src + ".o")
                subprocess.run(["cc", "-c", "-std=c99", "-I" + str(ROOT / "sdk"), "-I" + str(ENGINE),
                                str(ENGINE / (src + ".c")), "-o", str(obj)], check=True)
                objs.append(str(obj))
            binary = Path(d) / "genre_cpp"
            subprocess.run(["c++", "-std=c++14", "-Wall", "-Wextra", "-Werror", "-Wno-missing-field-initializers",
                            "-I" + str(ROOT / "sdk"), "-I" + str(ENGINE), str(ROOT / "tests/genre_cpp_test.cpp")]
                           + objs + ["-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, capture_output=True)

    def test_68000_code(self):
        if not m68k_gcc():
            self.skipTest("no m68k toolchain")
        subprocess.run(["python3", str(ROOT / "tools/check_genre_codegen.py")], check=True, capture_output=True)

    def test_selector(self):
        if not shutil.which("make"):
            self.skipTest("no make")

        def info(value):
            return subprocess.run(["make", "-s", "-C", str(ROOT), "GAME=helloworld",
                                   "GAME_CFG_FILE=games/helloworld/game.cfg", "GAME_GENRE_ENGINE=" + value, "genre-info"],
                                  capture_output=True, text=True)

        r = info("")
        self.assertEqual(r.returncode, 0)
        self.assertIn("genre=none", r.stdout)
        for g in ("puzzle", "mahjong", "rpg", "beatemup"):
            r = info(g)
            self.assertEqual(r.returncode, 0, r.stderr)
            self.assertIn(f"source=sdk/2d_engine/ng_{g}.c", r.stdout)
        r = info("puzle")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("is not a genre engine", r.stderr)
        r = info("puzzle rpg")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("one genre engine", r.stderr)


if __name__ == "__main__":
    unittest.main()
