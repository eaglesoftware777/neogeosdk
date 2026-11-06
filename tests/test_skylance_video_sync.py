"""Exercise the palette producer/interrupt handoff, including stale VBlank flags."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SkyVideoSyncTests(unittest.TestCase):
    def test_publish_acknowledgement_and_idle_interrupt(self):
        with tempfile.TemporaryDirectory() as temporary:
            binary = Path(temporary) / "video-sync"
            subprocess.run(["cc", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT),
                            str(ROOT / "tests/skylance_video_sync_test.c"),
                            "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
