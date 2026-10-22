"""A completed video batch may cross the main-loop/IRQ boundary only once."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class FrameSyncTests(unittest.TestCase):
    def test_partial_batches_stale_flags_and_guarded_upload(self):
        with tempfile.TemporaryDirectory() as folder:
            executable = Path(folder) / 'frame-sync'
            subprocess.run(['cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                            '-I', str(ROOT), str(ROOT / 'tests/frame_sync_test.c'),
                            '-o', str(executable)], check=True)
            subprocess.run([str(executable)], check=True)
