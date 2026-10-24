"""Keep puzzle content within the game renderer's fixed cell arrays."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
BUILDER = ROOT / "games/sevenwonders/tools/levels.py"
spec = importlib.util.spec_from_file_location("sw_levels", BUILDER)
levels = importlib.util.module_from_spec(spec)
spec.loader.exec_module(levels)


class SevenWondersLevels(unittest.TestCase):
    def test_shipped_content_is_current_without_writes(self):
        before = levels.OUT.read_bytes()
        subprocess.run(["python3", "-B", str(BUILDER), "--check"], check=True, capture_output=True)
        self.assertEqual(before, levels.OUT.read_bytes())

    def test_larger_engine_boards_are_rejected_by_game_renderer(self):
        data = json.loads(levels.SRC.read_text())
        for mode, width, height in (("match", 8, 7), ("match", 7, 8),
                                    ("bubble", 11, 10), ("bubble", 10, 13)):
            lv = copy.deepcopy(next(x for x in data["levels"] if x["mode"] == mode))
            lv.update(w=width, h=height)
            with self.subTest(mode=mode, width=width, height=height):
                with self.assertRaisesRegex(SystemExit, "renderer supports"):
                    levels.level_c(lv, "oversized", "test")

    def test_unknown_mode_is_rejected(self):
        with self.assertRaisesRegex(SystemExit, "unknown puzzle mode"):
            levels.level_c({"mode": "other", "w": 7, "h": 7}, "bad", "test")

    def test_malformed_rules_are_rejected_without_output(self):
        data = json.loads(levels.SRC.read_text())
        base = next(x for x in data["levels"] if x["mode"] == "match")
        bad = ({"colors": True}, {"colors": 9}, {"seed": -1}, {"seed": 2**32},
               {"moves": 65536}, {"time": -1}, {"miss_limit": 1},
               {"goals": [["other"]]}, {"goals": [["color", 8, 1]]},
               {"goals": [["score", 0, 65536]]}, {"goals": [["clear"]]},
               {"goals": [["ice"]]}, {"stars": [3, 2, 1]},
               {"goals": None}, {"goals": [[[]]]}, {"layout": 3},
               {"stars": [1, 2, 2**32]}, {"stars": [1]},
               {"layout": ["111...."] + ["......."] * 6})
        before = levels.OUT.read_bytes()
        for change in bad:
            lv = copy.deepcopy(base)
            lv.update(change)
            with self.subTest(change=change), self.assertRaises(SystemExit):
                levels.level_c(lv, "invalid", "test")
        bubble = copy.deepcopy(next(x for x in data["levels"] if x["mode"] == "bubble"))
        bubble["rows"] = ["." * (10 - (i & 1)) for i in range(13)]
        with self.assertRaisesRegex(SystemExit, "too many bubble rows"):
            levels.level_c(bubble, "invalid", "test")
        self.assertEqual(before, levels.OUT.read_bytes())

    def test_runtime_rejects_oversized_boards_before_clearing_live_state(self):
        source = r'''
#include <assert.h>
#include <string.h>
#include "games/sevenwonders/scenes/sw_limits.h"
#define SW_CHAPTERS 7u
#define SW_LEVELS 28u
enum { SW_CAT_MIMI, SW_CAT_KIKO };
typedef struct { const char *name, *line1, *line2; uint8_t cat; } SWChapter;
#include "games/sevenwonders/scenes/sw_levels_data.h"
static NGPuzzle p, before;
int main(void) {
    unsigned i, mode, w, h;
    NGPuzzleLevel bad;
    for (i = 0; i < SW_LEVELS; i++)
        assert(sw_level_init(&p, &sw_levels[i], 1) == NGP_OK);
    assert(sw_level_init(&p, &sw_demo_level, 1) == NGP_OK);
    before = p;
    for (mode = 0; mode < 3; mode++) for (w = 0; w <= 13; w++) for (h = 0; h <= 17; h++) {
        unsigned valid = w >= 3 && h >= 3 &&
            ((mode == 0 && w <= 7 && h <= 7) || (mode == 1 && w <= 10 && h <= 12));
        assert(sw_board_dimensions_valid(mode, w, h) == valid);
        if (!valid) {
            bad = sw_demo_level; bad.mode = mode; bad.w = w; bad.h = h;
            assert(sw_level_init(&p, &bad, 2) == NGP_ERR_LEVEL);
            assert(memcmp(&p, &before, sizeof(p)) == 0);
        }
    }
    bad = sw_demo_level; bad.colors = 9;
    assert(sw_level_init(&p, &bad, 2) == NGP_ERR_LEVEL);
    assert(memcmp(&p, &before, sizeof(p)) == 0);
    assert(sw_level_init(&p, 0, 2) == NGP_ERR_ARG);
    assert(sw_level_init(0, &sw_demo_level, 2) == NGP_ERR_ARG);
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as folder:
            src, exe = Path(folder) / "limits.c", Path(folder) / "limits"
            src.write_text(source)
            subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Werror",
                            "-I", str(ROOT), "-I", str(ROOT / "sdk"),
                            "-I", str(ROOT / "sdk/2d_engine"), str(src),
                            *[str(ROOT / "sdk/2d_engine" / f"{name}.c")
                              for name in ("ng_puzzle", "ng_trig", "ng_trig_table")],
                            "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)
