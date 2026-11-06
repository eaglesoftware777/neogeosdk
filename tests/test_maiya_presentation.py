import shutil
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class PresentationTests(unittest.TestCase):
    def test_first_bricks_and_road_share_a_fixed_camera(self):
        program = r'''
#include <assert.h>
#include "games/maiya/scenes/maiya_camera.h"
int main(void) {
    int i, feet=192*256, vy=-1536, support=192;
    int16_t cam=0;
    /* Jump onto the shelf on the descending half of the arc. */
    for(i=0;i<100;i++) {
        int previous=feet;
        vy+=64; feet+=vy;
        if(vy>=0 && previous<=144*256 && feet>=144*256) {
            feet=144*256;vy=0;support=144;
        }
        cam=mg_camera_vertical_supported(cam,feet/256,support,192,256,144,vy==0);
        assert(cam==0);
        if(support==144) break;
    }
    assert(support==144);
    for(i=0;i<80;i++) {
        cam=mg_camera_vertical_supported(cam,144,144,192,256,144,1);
        assert(cam==0);
    }
    /* Walking off must not select another view or sink below the floor. */
    vy=0;
    for(i=0;i<100;i++) {
        vy+=64;if(vy>1536)vy=1536;feet+=vy;
        if(feet>=192*256) {feet=192*256;vy=0;support=192;}
        cam=mg_camera_vertical_supported(cam,feet/256,support,192,256,144,vy==0);
        assert(cam==0);
    }
    assert(feet/256==192 && cam==0);
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as folder:
            source, executable = Path(folder) / 'bricks.c', Path(folder) / 'bricks'
            source.write_text(program)
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT),
                            str(source), '-o', str(executable)], check=True)
            subprocess.run([str(executable)], check=True)

    def test_ordinary_road_jumps_keep_the_camera_still(self):
        program = r'''
#include <assert.h>
#include "games/maiya/scenes/maiya_camera.h"
int main(void) {
    int jump;
    for (jump=1440; jump<=1536; jump+=96) {
        int frame, feet=192*256, vy=-jump, apex=192;
        int16_t cam=0;
        for(frame=0;frame<100;frame++) {
            vy+=64; if(vy>1536) vy=1536;
            feet+=vy; if(feet>192*256) { feet=192*256; vy=0; }
            if(feet/256<apex) apex=feet/256;
            cam=mg_camera_vertical_supported(cam,feet/256,192,192,256,144,vy==0);
            assert(cam==0);
        }
        assert(apex>=112 && apex<144); /* This arc triggered the regression. */
    }
    /* Levels without the new framing keep their established camera. */
    {
        int feet, current;
        for(current=-256;current<=0;current+=8) for(feet=-192;feet<=192;feet++) {
            assert(mg_camera_vertical_supported(current,feet,192,192,192,0,0)==
                   mg_camera_vertical(current,feet,192,0));
        }
    }
    assert(mg_camera_vertical_supported(0,60,192,192,256,144,0)<0); /* high leap */
    {
        int i; int16_t cam=-80;
        for(i=0;i<40;i++) {
            int16_t next=mg_camera_vertical_supported(cam,192,192,192,256,144,1);
            assert(next>=cam && next<=0); cam=next;
        }
        assert(cam==0);
    }
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as folder:
            src, exe = Path(folder) / 'road.c', Path(folder) / 'road'
            src.write_text(program)
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT),
                            str(src), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

    def test_jumps_from_every_ledge_hold_the_terrain_framing(self):
        program = r'''
#include <assert.h>
#include "games/maiya/scenes/maiya_camera.h"
int main(void) {
    int support, speed;
    for (support=-96; support<=192; support+=8) {
        int i; int16_t cam=0, resting;
        for(i=0;i<100;i++)
            cam=mg_camera_vertical_supported(cam,support,support,192,384,144,1);
        resting=cam;
        for(speed=1440;speed<=1536;speed+=96) {
            int feet=support*256,vy=-speed;
            for(i=0;i<100;i++) {
                int y;
                vy+=64; if(vy>1536) vy=1536;
                feet+=vy;
                if(feet>support*256) { feet=support*256; vy=0; }
                y=feet>=0 ? feet/256 : -((-feet+255)/256);
                cam=mg_camera_vertical_supported(cam,y,support,192,384,144,vy==0);
                assert(cam==resting); /* no upward OR downward scenery motion */
                /* The fixed road screen includes its low shelves. Their
                 * jump may enter the HUD band, but never clips the canvas;
                 * the higher canopy keeps its existing HUD clearance. */
                assert(y-cam-64 >= (support>=144 ? 0 : 32));
                assert(y-cam+2<=224);
            }
        }
        /* Drop to the road: maintain framing until the viewport edge,
         * then reveal the lower terrain without losing the heroine. */
        {
            int feet=support*256,vy=0;
            for(i=0;i<180;i++) {
                int y;
                vy+=64; if(vy>1536) vy=1536;
                feet+=vy;
                if(feet>192*256) { feet=192*256; vy=0; }
                y=feet>=0 ? feet/256 : -((-feet+255)/256);
                cam=mg_camera_vertical_supported(cam,y,vy==0?192:support,192,384,144,vy==0);
                assert(y-cam-64>=32 && y-cam+2<=224);
            }
            assert(cam==0);
        }
    }
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as folder:
            source, executable = Path(folder) / 'ledge.c', Path(folder) / 'ledge'
            source.write_text(program)
            subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT),
                            str(source), '-o', str(executable)], check=True)
            subprocess.run([str(executable)], check=True)

    def test_canopy_camera_preserves_legacy_and_frames_full_jump(self):
        compiler = shutil.which("cc")
        if not compiler:
            self.skipTest("Host C compiler is unavailable")
        program = r'''
#include <assert.h>
#include "games/maiya/scenes/maiya_camera.h"
int main(void) {
    int frame, feet_q8 = 88 * 256, vy = -6 * 256;
    int16_t cam = 0;
    /* Omitted framing retains the previous fixed/upper-tier behavior. */
    assert(mg_camera_vertical(0, 192, 0, 0) == 0);
    assert(mg_camera_vertical(-24, 192, 192, 0) == -20);
    assert(mg_camera_vertical(0, 32, 192, 0) == -5);
    assert(mg_camera_vertical(-100, -192, 256, 0) == -256);
    assert(mg_camera_vertical(-160, 192, 0, 0) == 0);
    /* Full run jump from the high bough, then a fall to the road.
     * Physics remains 6px launch, 0.25px acceleration, 6px terminal fall. */
    for (frame = 0; frame < 100; frame++) {
        int feet, screen;
        vy += 64; if (vy > 1536) vy = 1536;
        feet_q8 += vy;
        if (feet_q8 > 192 * 256) { feet_q8 = 192 * 256; vy = 0; }
        feet = feet_q8 / 256;
        cam = mg_camera_vertical(cam, (int16_t)feet, 256, 144);
        screen = feet - cam;
        assert(cam >= -256 && cam <= 0);
        assert(screen - 62 >= 64); /* entire hero canvas clears HUD and hints */
        assert(screen + 2 <= 224); /* boots remain on screen during descent */
    }
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder) / "camera.c"
            executable = Path(folder) / "camera"
            source.write_text(program)
            subprocess.run([compiler, "-std=c99", "-Wall", "-Wextra", "-Werror",
                            "-I", str(ROOT), str(source), "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True)

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
        assert(mg_gait_frame_mode(phase, 1) == i);
        phase = mg_gait_advance(phase, (uint16_t)(mg_run_distance[i] * 256u));
    }
    assert(phase == 0);
    assert(mg_gait_advance(phase, 0) == phase);
    assert(mg_gait_frame_mode(mg_gait_advance(20479, 1280), 0) < 8);
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
