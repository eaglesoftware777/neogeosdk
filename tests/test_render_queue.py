from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class RenderQueue(unittest.TestCase):
    def test_snapshot_and_budget(self):
        with tempfile.TemporaryDirectory() as tmp:
            for compiler, folder, ext, defer in (("cc", "2d_engine", "c", False),
                                                ("cc", "2d_engine", "c", True),
                                                ("c++", "2d_engine_plus", "cpp", False)):
                with self.subTest(compiler=compiler, defer=defer):
                    exe = Path(tmp) / (compiler.replace("+", "p") + str(defer))
                    subprocess.run([compiler, "-Wall", "-Wextra", "-Werror",
                                    "-I" + str(ROOT / "sdk"), "-I" + str(ROOT / "sdk" / folder),
                                    *(["-DNG_VRAM_DEFER"] if defer else []),
                                    str(ROOT / "tests/render_queue_test.c"),
                                    str(ROOT / "sdk" / folder / ("ng_render_queue." + ext)),
                                    "-o", str(exe)], check=True)
                    subprocess.run([str(exe)], check=True)
