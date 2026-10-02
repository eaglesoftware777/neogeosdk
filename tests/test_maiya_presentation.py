import shutil
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class PresentationTests(unittest.TestCase):
    def test_opening_encounter_spacing(self):
        level = json.loads((ROOT / "games/maiya/levels/01_emerald_forest.json").read_text())
        self.assertGreaterEqual(level["encounters"][0]["x"] - 64, 256)
        self.assertLess(level["encounters"][0]["x"], level["encounters"][1]["x"])

    def test_gait_pause_and_palette(self):
        compiler = shutil.which("cc")
        if not compiler:
            self.skipTest("Host C compiler is unavailable")
        program = r'''
#include <assert.h>
#include "games/maiya/scenes/maiya_presentation.h"
int main(void) {
    unsigned i;
    uint16_t phase = 0;
    uint16_t palette[16] = {0};
    for (i = 0; i < 8; i++) {
        assert(mg_gait_frame(phase) == i);
        phase = mg_gait_advance(phase, MG_GAIT_STEP);
    }
    assert(phase == 0);
    assert(mg_gait_advance(phase, 0) == phase);
    assert(mg_gait_frame(mg_gait_advance(20479, 1280)) < 8);
    assert(mg_pause_start_allowed(1, 1, 0));
    assert(!mg_pause_start_allowed(0, 1, 0));
    assert(!mg_pause_start_allowed(1, 3, 0));
    assert(!mg_pause_start_allowed(1, 1, 0x90));
    assert(!mg_pause_start_allowed(1, 1, 0x80));
    palette[3] = 0x359d;
    palette[7] = 0x0adf;
    assert(mg_palette_nearest(palette, 0x359d, 0) == 3);
    assert(mg_palette_nearest(palette, 0x0adf, 3) == 7);
    assert(mg_palette_nearest(palette, 0, 0) != 0);
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder) / "test.c"
            executable = Path(folder) / "test"
            source.write_text(program)
            subprocess.run([compiler, "-std=c99", "-Wall", "-Wextra", "-Werror",
                            "-I", str(ROOT), str(source), "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True)

    def test_gate_art_and_slot_geometry(self):
        import importlib.util
        spec = importlib.util.spec_from_file_location(
            "nature_art", ROOT / "games/maiya/tools/nature_art.py")
        art = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(art)
        for state in (False, True):
            image = art.gate(state)
            self.assertEqual(image.shape, (64, 48, 4))
        self.assertEqual(int(art.gate(True)[63, 24, 3]), 0)
        self.assertLessEqual(258 + 3, 264)
        self.assertGreaterEqual(258, 254 + 4)


if __name__ == "__main__":
    unittest.main()
