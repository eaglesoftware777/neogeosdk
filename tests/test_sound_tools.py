import math
from pathlib import Path
import sys
import tempfile
import unittest
import subprocess

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "sound/tools"))
import adpcm_enc
from mml_inputs import ordered_inputs
from pcm_metadata import delta_n, read_rate, write_rate
import fm_compile
import mml_compile
from wav_to_raw_pcm import resample_bandlimited, resample_linear


def decode_a(data):
    # Independent 12-bit hardware recurrence, including the single rounding.
    acc, step_index = 0, 0
    result = []
    for byte in data:
        for code in (byte >> 4, byte & 15):
            delta = (2 * (code & 7) + 1) * adpcm_enc.STEP_TABLE_A[step_index] // 8
            acc = (acc + (-delta if code & 8 else delta)) & 4095
            step_index = min(48, max(0, step_index + adpcm_enc.STEP_ADJ_A[code & 7]))
            result.append((acc if acc < 2048 else acc - 4096) * 16)
    return np.array(result)


class SoundToolsTests(unittest.TestCase):
    def test_sample_rates_survive_metadata_and_legacy_fallback(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "music.adpcmb"
            self.assertEqual(read_rate(path), 16000)
            write_rate(path, 32000)
            self.assertEqual(read_rate(path), 32000)
            self.assertEqual(Path(str(path) + ".json").read_bytes(),
                             b'{\n  "sample_rate": 32000\n}\n')
            self.assertEqual(delta_n(read_rate(path)), 0x9375)
            self.assertEqual(delta_n(16000), 0x49BA)
            with self.assertRaises(ValueError):
                delta_n(96000)

    def test_fm_a440_table_and_music_layout(self):
        lo, hi = fm_compile.fm_note_word(69)
        fnum, block = lo | ((hi & 7) << 8), hi >> 3
        frequency = fnum * 8000000 / (144 * 2 ** (21 - block))
        self.assertLess(abs(frequency - 440), 0.5)
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "notes.inc"
            fm_compile.emit_inc([], path)
            self.assertNotIn(".org", path.read_text())
            mml_compile.emit_inc([], path)
            self.assertNotIn(".org", path.read_text())

    def test_overlapping_wla_writes_are_fatal_even_with_exit_zero(self):
        root = Path(__file__).resolve().parents[1]
        result = subprocess.run([sys.executable, str(root / "sound/tools/checked_wla.py"),
                                 sys.executable, "-c", "print('MEM_INSERT: 2. write into $7800')"],
                                capture_output=True, check=False)
        self.assertNotEqual(result.returncode, 0)

    def test_mixed_arrangement_has_matching_phrase_lengths(self):
        root = Path(__file__).resolve().parents[1] / "games/demo/sound"
        fm = fm_compile.parse_mml((root / "fm/3_fm_example_d.mml").read_text())
        music = mml_compile.parse_mml((root / "mml/3_mml_example_d.mml").read_text())
        duration = lambda events: sum(value for code, value in events if code <= 0x80)
        self.assertEqual(duration(fm), 28 * 12)
        self.assertEqual(duration(music), duration(fm))
        self.assertIn((0xF0, 112), music)
        self.assertIn((0xF4, 3), music)
        self.assertIn((0xF2, 9), music)

    def test_track_ids_do_not_depend_on_directory_order(self):
        names = [Path(f"{n}_track.mml") for n in (10, 2, 0, 1)]
        self.assertEqual([p.name for p in ordered_inputs(names)],
                         ["0_track.mml", "1_track.mml", "2_track.mml", "10_track.mml"])
        with tempfile.TemporaryDirectory() as folder:
            for name in names:
                (Path(folder) / name).touch()
            self.assertEqual([p.name for p in ordered_inputs([Path(folder) / "*.mml"])],
                             [p.name for p in ordered_inputs(names)])

    def test_adpcma_rounding_and_overflow_match_hardware(self):
        self.assertEqual(adpcm_enc._adpcma_delta(19, 7), 35)
        state = [2040, 48]
        adpcm_enc.adpcma_decode_nibble(state, 7)
        self.assertEqual(state[0], ((2040 + 2910 + 2048) & 4095) - 2048)

    def test_adpcma_has_no_preroll_and_settles_its_padding(self):
        samples = [round(12000 * math.sin(i * 0.16)) for i in range(1700)]
        data, preroll = adpcm_enc.encode_adpcma(samples)
        decoded = decode_a(data)
        self.assertEqual(preroll, 0)
        self.assertEqual(len(data) % 256, 0)
        self.assertLess(np.max(np.abs(decoded[-64:])), 80)
        error = np.mean((decoded[:len(samples)] - samples) ** 2)
        self.assertGreater(10 * math.log10(np.mean(np.square(samples)) / error), 24)

    def test_adpcmb_rounds_to_nearest_prediction(self):
        state = [0, 128]
        code = adpcm_enc.adpcmb_encode_nibble(state, 33)
        self.assertEqual(code, 1)
        self.assertEqual(state[0], 48)

    def test_downsampling_rejects_aliases_and_preserves_passband(self):
        t = np.arange(48000) / 48000.0
        high = np.sin(2 * np.pi * 12000 * t).astype(np.float32)
        filtered = resample_bandlimited(high, 48000, 16000)[100:-100]
        aliased = resample_linear(high, 48000, 16000)[100:-100]
        self.assertLess(np.mean(filtered ** 2), np.mean(aliased ** 2) * 0.001)
        low = np.sin(2 * np.pi * 1000 * t).astype(np.float32)
        result = resample_bandlimited(low, 48000, 16000)
        self.assertEqual(len(result), 16000)
        self.assertAlmostEqual(float(np.sqrt(np.mean(result[100:-100] ** 2))),
                               math.sqrt(0.5), places=3)

    def test_rational_resampling_keeps_time_alignment_and_edges(self):
        # A ratio with no common factor beyond 100 exercises every polyphase
        # branch; the result must line up with the tone sampled directly at
        # the new rate, or every sample would play late by the filter delay.
        t = np.arange(44100) / 44100.0
        tone = np.sin(2 * np.pi * 1000 * t).astype(np.float32)
        result = resample_bandlimited(tone, 44100, 18500)
        expected = np.sin(2 * np.pi * 1000 * np.arange(len(result)) / 18500.0)
        self.assertEqual(len(result), 18500)
        self.assertLess(float(np.max(np.abs(result[200:-200] - expected[200:-200]))), 1e-3)
        # A bed that does not start at silence must not ring at its ends.
        flat = np.full(4410, 0.5, dtype=np.float32)
        result = resample_bandlimited(flat, 44100, 32000)
        self.assertLess(float(np.max(np.abs(result - 0.5))), 1e-4)


if __name__ == "__main__":
    unittest.main()
