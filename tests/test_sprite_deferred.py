"""Compile the actual deferred renderer with a controllable raster budget."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class DeferredSprites(unittest.TestCase):
    def run_renderer(self, remove_membership_fix=False):
        with tempfile.TemporaryDirectory() as temporary:
            executable = Path(temporary) / "deferred"
            fixture = ROOT / "tests/sprite_deferred_test.c"
            if remove_membership_fix:
                group = Path(temporary) / "group.c"
                group.write_text((ROOT / "sdk/2d_engine/ng_sprite_group.c").read_text().replace(
                    "g->dirty |= ngsg_queued_flag(g);", "/* Original membership reset. */"))
                copied = Path(temporary) / "fixture.c"
                copied.write_text(fixture.read_text().replace(
                    '"sprite_render_test.c"', f'"{ROOT / "tests/sprite_render_test.c"}"').replace(
                    '"../sdk/2d_engine/ng_sprite_group.c"', f'"{group}"'))
                fixture = copied
            sources = [fixture] + [
                ROOT / f"sdk/2d_engine/{name}.c" for name in
                ("ng_chars", "ng_camera", "ng_art_asset", "ng_palette_fx",
                 "ng_sprite_window", "ng_vram")]
            subprocess.run(["cc", "-DNG_VRAM_DEFER", "-DNG_VRAM_HOST_TEST",
                            "-I", str(ROOT / "sdk"), "-I", str(ROOT / "sdk/2d_engine"),
                            *(str(source) for source in sources),
                            "-o", str(executable)], check=True)
            return subprocess.run([str(executable)], capture_output=True, text=True)

    def test_pending_rebind_and_window_reuse(self):
        result = self.run_renderer()
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_original_rebind_duplicates_pending_group(self):
        result = self.run_renderer(remove_membership_fix=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("pending() == 1u", result.stderr)
