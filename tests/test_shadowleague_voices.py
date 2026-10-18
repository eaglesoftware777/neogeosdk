"""Approved recordings must reach the hardware sample table intact."""
import json,re,sys,unittest,wave
from pathlib import Path
import numpy as np
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'games/shadowleague/tools'))
import slf_voices
from voice_locales import LOCALES
class VoiceTests(unittest.TestCase):
    def test_localized_wave_integrity_and_stable_sample_numbers(self):
        rows=json.loads((slf_voices.VOICES/'localized-recordings.json').read_text())
        entries={x['key']:x for x in slf_voices.plan(slf_voices.load_fighters())}
        self.assertEqual({x['fighter'] for x in rows},set(LOCALES))
        self.assertEqual(len({e['number'] for e in entries.values()}),len(entries))
        self.assertLessEqual(max(e['number'] for e in entries.values()),255)
        for row in rows:
            with self.subTest(key=row['key']):
                source=slf_voices.VOICES/(row['key']+'.wav')
                with wave.open(str(source)) as w:
                    self.assertEqual((w.getnchannels(),w.getsampwidth(),w.getframerate()),(1,2,18519))
                    pcm=np.frombuffer(w.readframes(w.getnframes()),'<i2').astype(float)/32768
                self.assertGreater(len(pcm),1851)
                self.assertLessEqual(np.abs(pcm).max(),.97)
                self.assertGreater(np.sqrt(np.mean(pcm**2)),.01)
                target=slf_voices.GAME/'sound/samples/in_wav_a'/(str(entries[row['key']]['number'])+'.wav')
                self.assertEqual(source.read_bytes(),target.read_bytes())
    def test_encoded_samples_match_the_actual_aligned_v_rom(self):
        rom=(ROOT/'roms/shadowleague/781-v1.v1').read_bytes()
        table=(ROOT/'sound/driver/sample_table.inc').read_text()
        rows=re.findall(r'; (\d+)\.adpcma \(start=\$(\w+) size=(\d+)\)',table)
        self.assertEqual(len(rows),slf_voices.NUM_SFX+len(slf_voices.plan(slf_voices.load_fighters())))
        previous=0
        for number,start,size in rows:
            start=int(start,16);size=int(size)
            with self.subTest(sample=number):
                self.assertEqual(start%256,0);self.assertEqual(size%256,0)
                self.assertGreaterEqual(start,previous)
                self.assertEqual(start//0x100000,(start+size-1)//0x100000)
                self.assertLessEqual(start+size,0x1000000)
                source=slf_voices.GAME/'sound/samples/out_a'/(number+'.adpcma')
                self.assertEqual(rom[start:start+size],source.read_bytes())
                previous=start+size

if __name__=='__main__':unittest.main()
