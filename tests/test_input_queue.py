"""Run interrupt snapshot ownership tests in both SDK language variants."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class InputQueue(unittest.TestCase):
    def test_c_and_cpp(self):
        with tempfile.TemporaryDirectory() as tmp:
            for compiler, standard in (("cc", "c99"), ("c++", "c++14")):
                with self.subTest(compiler=compiler):
                    exe = Path(tmp) / compiler.replace("+", "p")
                    subprocess.run([compiler, "-std=" + standard, "-Wall", "-Wextra", "-Werror",
                                    "-I" + str(ROOT / "sdk"), str(ROOT / "tests/input_queue_test.c"),
                                    "-o", str(exe)], check=True)
                    subprocess.run([str(exe)], check=True)
