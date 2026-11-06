from pathlib import Path
import tempfile
import unittest
import wave
from tools.neocd.formats import bcd_track,validate_wav


class AudioTests(unittest.TestCase):
    def test_track_bcd(self):
        for track in range(2,100):
            encoded=bcd_track(track)
            self.assertEqual((encoded>>4)*10+(encoded&15),track)
        for track in (0,1,100,-1,True,2.0):
            with self.subTest(track=track),self.assertRaises(ValueError):bcd_track(track)

    def test_sector_and_audio_format(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'audio.wav'
            for channels,width,rate,n,valid in ((2,2,44100,588,True),(2,2,44100,1176,True),
                    (1,2,44100,588,False),(2,1,44100,588,False),(2,2,48000,588,False),
                    (2,2,44100,587,False),(2,2,44100,0,False)):
                with wave.open(str(path),'wb') as w:
                    w.setparams((channels,width,rate,0,'NONE',''));w.writeframes(bytes(n*channels*width))
                if valid:self.assertEqual(validate_wav(path)['sectors'],n//588)
                else:
                    with self.subTest(channels=channels,width=width,rate=rate,n=n),self.assertRaises(ValueError):validate_wav(path)

    def test_truncated_wav(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'audio.wav'
            with wave.open(str(path),'wb') as w:
                w.setparams((2,2,44100,0,'NONE',''));w.writeframes(bytes(588*4))
            path.write_bytes(path.read_bytes()[:-4])
            with self.assertRaises(ValueError):validate_wav(path)


if __name__=='__main__':unittest.main()
