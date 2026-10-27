"""Review contracts must reject incomplete uploads without padding short walks."""
from pathlib import Path
import importlib.util
import unittest

TOOLS = Path(__file__).resolve().parents[1] / 'games/maiya/tools'


def module(name):
    spec = importlib.util.spec_from_file_location(name, TOOLS / (name + '.py'))
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


class ReviewTests(unittest.TestCase):
    def test_completed_publication_requires_owned_complete_pixels(self):
        check = module('publication_review').validate
        rows = [dict(seconds=i, published=i+1, consumed=i+1, visible=1,
                     owned=1, complete=1, rows=4) for i in range(100)]
        self.assertTrue(check(rows)['passed'])
        for key, value in [('owned', 0), ('complete', 0), ('rows', 0)]:
            bad = [dict(r) for r in rows]
            bad[50][key] = value
            self.assertFalse(check(bad)['passed'], key)
        with self.assertRaises(ValueError):
            check([dict(rows[0]) for _ in range(100)])

    @staticmethod
    def forest():
        tiles = [16000 + i * 20 for i in range(80)]
        rows = []
        for i in range(4):
            r = dict(seconds=i, tick=i, phase='door' if i < 3 else 'complete',
                     hero=0, stage=0, lives=3, state=10 if i < 3 else 2,
                     boss_hp=0, key=1, gate=1, state_timer=139-i,
                     x=3210+min(i, 2), distance=18000+min(i, 2)*320,
                     tile=tiles[67], visible=1, scb_rows=4, facing=0,
                     hp=5, rescues=15, art=1)
            for y in range(4):
                for x in range(5):
                    r[f'r{y}c{x}'] = tiles[67]+y*5+x
            rows.append(r)
        return rows, tiles

    def test_short_door_walk_keeps_real_pose_holds_and_distance(self):
        check = module('forest_review').validate
        rows, tiles = self.forest()
        result = check(rows, tiles, 0)
        self.assertTrue(result['passed'])
        self.assertEqual(result['door_drawings'], 1)
        bad = [dict(r) for r in rows]
        bad[2]['distance'] = bad[1]['distance']+256
        with self.assertRaisesRegex(ValueError, 'walking displacement'):
            check(bad, tiles, 0)

    def test_extra_life_is_allowed_but_life_loss_fails(self):
        check = module('forest_review').validate
        rows, tiles = self.forest()
        rows[-1]['lives'] = 4
        self.assertEqual(check(rows, tiles, 0)['lives_gained'], 1)
        rows[-1]['lives'] = 2
        with self.assertRaisesRegex(ValueError, 'lost a life'):
            check(rows, tiles, 0)

    def test_visible_missing_or_fragmented_body_still_fails(self):
        check = module('forest_review').validate
        rows, tiles = self.forest()
        rows[1]['scb_rows'] = 0
        self.assertFalse(check(rows, tiles, 0)['passed'])
        rows, tiles = self.forest()
        rows[1]['r2c3'] += 20
        self.assertFalse(check(rows, tiles, 0)['passed'])
