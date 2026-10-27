"""Exercise the actual SDK projector and camera with host-side fixtures."""
import subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class ShooterDepthTests(unittest.TestCase):
    def test_camera_depth_and_shadow_projection(self):
        with tempfile.TemporaryDirectory(prefix='sky-depth-') as d:
            binary=Path(d)/'depth'
            subprocess.run(['cc','-Wall','-Wextra','-DNG_SHOOTER=1','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(ROOT/'sdk'),'-I'+str(ROOT/'sdk/2d_engine'),str(ROOT/'tests/shooter_depth_test.c'),str(ROOT/'sdk/2d_engine/ng_shooter.c'),str(ROOT/'sdk/2d_engine/ng_depthfx.c'),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)
if __name__=='__main__':unittest.main()
