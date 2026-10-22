"""The actual game IRQ consumer must upload only completed, safe batches."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SevenWondersFrameTests(unittest.TestCase):
    def test_completed_irq_consumption_and_ui_order(self):
        scene = (ROOT / 'games/sevenwonders/scenes/sw_draw.c').read_text()
        consumer = scene[scene.index('void NEOGEO_USER sw_vblank('):
                         scene.index('\nstatic const NGArtAsset *')]
        fixture = r'''
#include <assert.h>
#include "sdk/2d_engine/ng_frame_sync.h"
#define NEOGEO_USER
static NGFrameSync s_video_sync;
static unsigned ng_vram_busy, window, calls;
static unsigned ng_vram_window_open(void) { return window; }
static void sw_commit_ui(void) { assert(calls++==0); }
static void ng_palfx_vblank(void) { assert(calls++==1); }
static void ng_vram_commit(void) { assert(calls++==2); }
'''
        fixture += consumer + r'''
int main(void) {
    window=1; sw_vblank(); assert(calls==0);
    assert(ng_frame_sync_publish(&s_video_sync));
    ng_vram_busy=1; sw_vblank(); assert(calls==0 && s_video_sync.pending);
    ng_vram_busy=0; window=0; sw_vblank(); assert(calls==0 && s_video_sync.pending);
    window=1; sw_vblank(); assert(calls==3 && !s_video_sync.pending);
    sw_vblank(); assert(calls==3);
    calls=0; assert(ng_frame_sync_publish(&s_video_sync));
    sw_vblank(); assert(calls==3 && !s_video_sync.pending);
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as folder:
            src, exe = Path(folder) / 'frame.c', Path(folder) / 'frame'
            src.write_text(fixture)
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT),
                            str(src), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)
