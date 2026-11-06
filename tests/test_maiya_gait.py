"""Maiya's walk/run tables, her authored frames, and the rhythm of B on the move."""
import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / 'games/maiya/assets/authored'


def host_program(test, body):
    compiler = shutil.which('cc')
    if not compiler:
        test.skipTest('Host C compiler is unavailable')
    program = ('#include <assert.h>\n#include <stdio.h>\n'
               '#include "games/maiya/scenes/maiya_feel.h"\n'
               '#include "games/maiya/scenes/maiya_presentation.h"\n' + body)
    with tempfile.TemporaryDirectory() as folder:
        source = Path(folder) / 'gait.c'
        executable = Path(folder) / 'gait'
        source.write_text(program)
        subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT),
                        '-I', str(ROOT / 'sdk/2d_engine'), '-I', str(ROOT / 'sdk'),
                        str(source), '-o', str(executable)], check=True)
        return subprocess.run([str(executable)], check=True, capture_output=True, text=True).stdout


class GaitTests(unittest.TestCase):
    def test_distance_tables(self):
        host_program(self, r'''
int main(void) {
    unsigned k, run, phase;
    for (run = 0; run < 2; run++) {
        const uint8_t *d = run ? mg_run_distance : mg_walk_distance;
        const uint16_t *t = run ? mg_run_threshold : mg_walk_threshold;
        unsigned sum = 0;
        for (k = 0; k < MG_GAIT_FRAMES; k++) {
            unsigned begin = sum * 256u;
            sum += d[k];
            assert(t[k] == sum * 256u);
            assert(mg_gait_frame_mode((uint16_t)begin, (uint8_t)run) == k);
            assert(mg_gait_frame_mode((uint16_t)(sum * 256u - 1u), (uint8_t)run) == k);
            /* the second foot repeats the first foot's timing */
            if (k < 4) assert(d[k] == d[k + 4]);
        }
        assert(sum == 80);
        for (phase = 0; phase < MG_GAIT_CYCLE; phase++) {
            assert(mg_gait_frame_mode((uint16_t)phase, (uint8_t)run) < MG_GAIT_FRAMES);
            assert(mg_gait_advance((uint16_t)phase, 0) == phase);
            assert(mg_gait_advance((uint16_t)phase, MG_GAIT_CYCLE) == phase);
        }
    }
    assert(mg_gait_footfall[0] && mg_gait_footfall[4]);
    /* a walk advances the clock 5/4 of the road: 64 px make one cycle */
    {
        uint16_t phase = 0;
        for (k = 0; k < 64; k++) phase = mg_gait_advance(phase, mg_gait_units(256, 0));
        assert(phase == 0);
        for (k = 0; k < 80; k++) phase = mg_gait_advance(phase, mg_gait_units(256, 1));
        assert(phase == 0);
    }
    /* a short settle, then straight back to idle */
    assert(MG_GAIT_STOP >= 4 && MG_GAIT_STOP <= 8);
    return 0;
}
''')

    def test_fire_rhythm(self):
        """Held B with a direction: throw, planted feet, a few steps, throw.
        Taps during the steps are kept, not lost; standing, nothing waits."""
        out = host_program(self, r'''
/* Mirrors mg_controls: a throw on the move plants her and starts the wait. */
static unsigned run(unsigned frames, const char *pad, uint8_t moving, unsigned *planted, unsigned *gap_min)
{
    MGFire f = { 0, 0, 0 };
    unsigned i, throws = 0, last = 0, plant = 0;
    uint8_t was_down = 0;
    *planted = 0; *gap_min = 999;
    for (i = 0; i < frames; i++) {
        uint8_t down = (uint8_t)(pad[i % 4] == 'B');
        uint8_t pressed = (uint8_t)(down && !was_down);
        was_down = down;
        if (plant) { plant--; (*planted)++; }
        if (mg_fire_tick(&f, pressed, down, moving, MG_FIRE_REPEAT)) {
            if (throws && i - last < *gap_min) *gap_min = i - last;
            throws++; last = i;
            if (moving) { plant = MG_STEP_PLANT; f.wait = MG_STEP_GAP; }
        }
    }
    return throws;
}
int main(void) {
    unsigned planted, gap, held, tapped, stand;
    held = run(240, "BBBB", 1, &planted, &gap);           /* held B, on the move */
    printf("held %u planted %u gap %u\n", held, planted, gap);
    assert(gap >= MG_STEP_GAP);
    assert(held >= 240 / (MG_STEP_GAP + 1));
    /* her feet are planted for a third of the time at most: she walks */
    assert(planted * 3 <= 240);
    tapped = run(240, "B.B.", 1, &planted, &gap);         /* mashing, on the move */
    printf("tapped %u gap %u\n", tapped, gap);
    assert(gap >= MG_STEP_GAP && tapped >= held);
    stand = run(240, "B...", 0, &planted, &gap);          /* tapping, standing */
    printf("standing %u gap %u planted %u\n", stand, gap, planted);
    assert(stand == 60 && gap == 4 && planted == 0);
    stand = run(240, "BBBB", 0, &planted, &gap);          /* held, standing */
    assert(gap == MG_FIRE_REPEAT);
    return 0;
}
''')
        self.assertIn('held', out)

    def test_frames_share_one_hip_and_head(self):
        """Every drawing keeps her original stride's head and tunic, moved
        only by its bob, so the runtime needs no per-frame offsets, the
        sprite never jumps and her face is the same drawing as her throws,
        jumps and landings. Her near arm alternates every step."""
        import numpy as np
        from PIL import Image
        poses = json.loads((ART / 'poses.json').read_text())
        self.assertEqual([p['mode'] for p in poses], ['walk'] * 8 + ['run'] * 8 + ['stop'])
        ref = np.array(Image.open(ART / 'reference_stride.png'))
        first = {}
        for p in poses:
            for layer in ('torso', 'head_hair'):
                pixels = np.array(Image.open(ART / 'layers' / f"{p['name']}__{layer}.png"))
                back = np.roll(pixels, -p['bob'], axis=0)
                lit = back != 0
                self.assertTrue(np.array_equal(back[lit], ref[lit]), (p['name'], layer))
                first.setdefault(layer, lit)
                self.assertTrue(np.array_equal(lit, first[layer]), (p['name'], layer))
            self.assertLessEqual(abs(p['bob']), 2)
        for mode in ('walk', 'run'):
            cycle = [p for p in poses if p['mode'] == mode]
            for a, b in zip(cycle[:4], cycle[4:]):
                self.assertNotEqual(a['near_arm'], b['near_arm'], (a['name'], b['name']))
        for mode in ('walk', 'run'):
            holds = [p['hold_distance'] for p in poses if p['mode'] == mode]
            self.assertEqual(sum(holds), 80)

    def test_strips_match_the_runtime_tables(self):
        import re
        header = (ROOT / 'games/maiya/scenes/maiya_presentation.h').read_text()
        poses = json.loads((ART / 'poses.json').read_text())
        for mode, table in (('walk', 'mg_walk_distance'), ('run', 'mg_run_distance')):
            body = re.search(table + r'\[MG_GAIT_FRAMES\] = \{(.*?)\}', header).group(1)
            self.assertEqual([int(x) for x in body.split(',')],
                             [p['hold_distance'] for p in poses if p['mode'] == mode])


if __name__ == '__main__':
    unittest.main()
