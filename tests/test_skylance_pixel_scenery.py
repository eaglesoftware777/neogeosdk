"""Placement and pixel-grid contracts for the seven native shooter routes."""
import importlib.util
from pathlib import Path
import unittest
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('sky_pixels',ROOT/'games/skylance/tools/pixel_scenery.py')
pixels = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pixels)


def painted(stage):
    """Stages 2-7 use painted maps (tools/paint_terrain.py) once they are built."""
    return stage and (pixels.PAINTED / f'stage_{stage + 1}.png').exists()


def asphalt(rgb):
    rgb = np.asarray(rgb, int)
    return (rgb.max(-1) - rgb.min(-1) < 36) & (rgb.mean(-1) > 30) & (rgb.mean(-1) < 120)


class PixelScenery(unittest.TestCase):
    def test_hardware_indexing_preserves_every_authored_color(self):
        for stage in range(7):
            im=pixels.route(stage)
            indices,palettes,mapping=pixels.indexed_route(im)
            self.assertLessEqual(len(palettes),16)
            self.assertTrue((indices>0).all())
            actual=palettes[mapping.repeat(16,0).repeat(16,1),indices]
            np.testing.assert_array_equal(actual,np.asarray(im)[:,:,:3])
    def test_native_canvas_fixed_ramps_and_unique_routes(self):
        routes=[]
        colors={tuple(bytes.fromhex(c[1:])) for c in pixels.COLORS.values()}
        for stage in range(7):
            im=pixels.route(stage)
            self.assertEqual(im.size,(352,2048))
            a=np.asarray(im)
            self.assertTrue((a[:,:,3]==255).all())
            if not painted(stage):        # drawn maps keep to the fixed ramps
                self.assertLessEqual({tuple(c) for c in np.unique(a[:,:,:3].reshape(-1,3),axis=0)},colors)
            self.assertEqual(len({a[i*256:(i+1)*256].tobytes() for i in range(8)}),8)
            routes.append(im.tobytes())
        self.assertEqual(len(set(routes)),7)

    def test_naval_footprints_stay_in_water(self):
        water={tuple(bytes.fromhex(pixels.COLORS[c][1:])) for c in ('water','water_dark','water_hi')}
        for stage in (1,3):
            a=np.asarray(pixels.route(stage))
            # Opaque hulls fit within +/-14px of the world lane centres.
            for x in (80,192):
                if painted(stage):        # open water, allowing reef patches and a pier end
                    col=a[:,x-4:x+5,:3].reshape(-1,3).astype(int)
                    self.assertGreaterEqual((col[:,2]>col[:,0]+10).mean(),0.85)
                else:
                    self.assertLessEqual({tuple(c) for c in np.unique(a[:,x-14:x+15,:3].reshape(-1,3),axis=0)},water)

    def test_contact_targets_use_empty_pads_and_traffic_uses_road(self):
        pad=tuple(bytes.fromhex(pixels.COLORS['steel_dark'][1:]))
        road=tuple(bytes.fromhex(pixels.COLORS['road'][1:]))
        for stage in range(7):
            im=pixels.route(stage)
            if painted(stage):
                a=np.asarray(im)[:,:,:3]
                if stage not in (1,3):    # land: a concrete pad at each target, asphalt down the road
                    for page in (2,4,6):
                        self.assertTrue(asphalt(a[page*256+96,144]))
                    # the road runs to the boss arena, the last page, and stops
                    arena = a.shape[0]-256
                    self.assertGreaterEqual(asphalt(a[:arena,87]).mean(),0.98)
                    self.assertLessEqual(asphalt(a[arena:,87]).mean(),0.5)
                continue
            for page in (2,4,6):
                self.assertEqual(im.getpixel((144,page*256+96))[:3],pad)
            if stage not in (1,3):
                for y in range(2048): self.assertEqual(im.getpixel((87,y))[:3],road)

    def test_compact_console_and_countermeasure_transparency(self):
        for i in range(3):
            self.assertEqual(pixels.cockpit(i).size,(32,64))
            a=np.asarray(pixels.miniboss(i))
            self.assertEqual(a.shape,(48,64,4))
            self.assertFalse(a[0,:,3].any() or a[-1,:,3].any() or a[:,0,3].any() or a[:,-1,3].any())
        for im in (pixels.chaff(),pixels.radar(0),pixels.radar(1)):
            self.assertEqual(set(np.unique(np.asarray(im)[:,:,3])),{0,255})


if __name__=='__main__': unittest.main()
