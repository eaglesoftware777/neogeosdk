"""Copy approved voice WAVs to their stable game sample numbers before encoding."""
import shutil,wave
from pathlib import Path
import slf_voices

def main():
    folder=slf_voices.GAME/'sound/samples/in_wav_a';folder.mkdir(parents=True,exist_ok=True)
    entries=slf_voices.plan(slf_voices.load_fighters());changed=0
    for entry in entries:
        source=slf_voices.VOICES/(entry['key']+'.wav')
        with wave.open(str(source)) as w:
            if (w.getnchannels(),w.getsampwidth(),w.getframerate())!=(1,2,18519):raise ValueError(entry['key']+': expected mono PCM16, 18519 Hz')
            if not 1000<=w.getnframes()<=18519*3:raise ValueError(entry['key']+': invalid voice length')
        target=folder/(str(entry['number'])+'.wav')
        if not target.exists() or target.read_bytes()!=source.read_bytes():shutil.copy2(source,target);changed+=1
    print('Voice sample mapping:',len(entries),'stable indices;',changed,'WAVs updated')

if __name__=='__main__':main()
