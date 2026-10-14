"""Independent Sky Lance ADPCM-B assets and playback metadata."""
import hashlib
import json
from pathlib import Path
import unittest
import wave

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "games/skylance"


class SkyMusicTests(unittest.TestCase):
    def test_tracks_are_independent_aligned_and_rate_tagged(self):
        reports = json.loads((GAME / "sound/diagnostics/music.json").read_text())
        self.assertEqual([r["id"] for r in reports], list(range(1, 10)))
        maiya = {hashlib.sha256(p.read_bytes()).hexdigest()
                 for p in (ROOT / "games/maiya/sound/samples/out_b").glob("*.adpcmb")}
        hashes = set()
        for row in reports:
            path = GAME / f'sound/samples/out_b/{row["id"]}.adpcmb'
            data = path.read_bytes()
            digest = hashlib.sha256(data).hexdigest()
            self.assertNotIn(digest, maiya)
            self.assertEqual(digest, row["encoded_sha256"])
            self.assertEqual(len(data) % 256, 0)
            self.assertLessEqual(len(data), 0x100000)
            self.assertEqual(json.loads(Path(str(path)+".json").read_text())["sample_rate"], 16000)
            self.assertEqual(row["clipped_samples"], 0)
            self.assertLess(abs(row["decoded_boundary_delta"]), 256)
            hashes.add(digest)
        self.assertEqual(len(hashes), 9)
        self.assertLess(sum(r["bytes"] for r in reports), 0x400000)

    def test_sources_and_decoded_previews_exist(self):
        for row in json.loads((GAME / "sound/diagnostics/music.json").read_text()):
            path = GAME / f'sound/samples/in_wav_b/{row["id"]}.wav'
            self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(), row["source_sha256"])
            with wave.open(str(path)) as audio:
                self.assertEqual((audio.getframerate(), audio.getnchannels(), audio.getsampwidth()),
                                 (16000, 1, 2))
                self.assertGreater(audio.getnframes(), 16000*20)
            self.assertTrue((GAME / f'sound/diagnostics/music/{row["id"]}_{row["name"]}.wav').is_file())


if __name__ == "__main__":
    unittest.main()
