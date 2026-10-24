"""Prevent content from overflowing either complete depth-layout bank."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NoSignalRenderBudget(unittest.TestCase):
    def test_all_stages_fit_on_expert(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / "budget"
            subprocess.run(["cc", "-std=c99", "-I", str(ROOT), "-I", str(ROOT / "sdk"),
                            "-I", str(ROOT / "games/nosignal/scenes"),
                            str(ROOT / "tests/nosignal_render_budget_test.c"), "-o", str(exe)],
                           check=True, capture_output=True, text=True)
            subprocess.run([str(exe)], check=True, capture_output=True, text=True)
