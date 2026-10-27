"""Direction repeat timing, reversal, independent axes and long holds."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class InputRepeatTests(unittest.TestCase):
    def test_repeat_contract_in_c_and_cpp(self):
        with tempfile.TemporaryDirectory() as folder:
            for compiler, standard in (("cc", "c99"), ("c++", "c++14")):
                exe = Path(folder) / compiler
                subprocess.run([compiler, "-std=" + standard, "-Wall", "-Wextra", "-Werror",
                                "-I", str(ROOT), str(ROOT / "tests/input_repeat_test.c"),
                                "-o", str(exe)], check=True)
                subprocess.run([str(exe)], check=True)
