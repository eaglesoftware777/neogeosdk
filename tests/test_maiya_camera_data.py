"""Invalid opt-in camera profiles must fail before level output is written."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('maiya_levels', ROOT / 'games/maiya/tools/levels.py')
levels = importlib.util.module_from_spec(spec)
spec.loader.exec_module(levels)


class CameraData(unittest.TestCase):
    def test_existing_missions_keep_legacy_framing(self):
        _, stages = levels.load()
        self.assertEqual(stages[0]['camera_feet_y'], 144)
        self.assertTrue(all(s['camera_feet_y'] == 0 for s in stages[1:]))

    def test_invalid_framing_is_rejected_without_output_changes(self):
        source = levels.LEVELS / '01_emerald_forest.json'
        generated = levels.OUT.read_bytes()
        for framing, upper in [(True, 192), (95, 192), (161, 192), (112, 0)]:
            with self.subTest(framing=framing, upper=upper), tempfile.TemporaryDirectory() as folder:
                data = json.loads(source.read_text())
                data.update(camera_feet_y=framing, upper=upper)
                fixture = Path(folder) / 'stage.json'
                fixture.write_text(json.dumps(data))
                with self.assertRaises(levels.LevelError):
                    levels.Stage(fixture, {}, levels.caps()).build()
                self.assertEqual(levels.OUT.read_bytes(), generated)


if __name__ == '__main__':
    unittest.main()
