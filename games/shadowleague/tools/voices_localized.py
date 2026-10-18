"""Render the localized combat pass with the licensed Kokoro voice library.

Run in the dedicated kokoro-venv; model downloads are stored on the Linux disk.
Model: https://huggingface.co/hexgrad/Kokoro-82M (Apache 2.0).
Voice data credits: Koniwa (CC BY 3.0), SIWIS (CC BY 4.0); see voice-sources-sdk177.md.
"""
import argparse,json
from pathlib import Path
import numpy as np
import unidic,unidic_lite
unidic.DICDIR=unidic_lite.DICDIR
from kokoro_onnx import Kokoro
from misaki import ja,zh
from voices import shout,write
from voice_locales import LOCALES
import slf_voices

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--models',type=Path,default=Path.home()/'.cache/slf-voice/kokoro-models')
    ap.add_argument('--only',nargs='*')
    a=ap.parse_args()
    import onnxruntime as ort
    options=ort.SessionOptions();options.intra_op_num_threads=2;options.inter_op_num_threads=1
    session=ort.InferenceSession(str(a.models/'kokoro-v1.0.onnx'),sess_options=options,providers=['CPUExecutionProvider'])
    k=Kokoro.from_session(session,str(a.models/'voices-v1.0.bin'))
    japanese=ja.JAG2P();chinese=zh.ZHG2P()
    result=[]
    for entry in slf_voices.plan(slf_voices.load_fighters()):
        who=entry.get('fighter');locale=LOCALES.get(who)
        if not locale or (a.only and who not in a.only):continue
        key=entry['key'].rsplit('_',1)[-1];text=locale['lines'][key]
        lang=locale['language'];phonetic=False
        if lang=='ja':text,_=japanese(text);phonetic=True
        elif lang=='zh':text,_=chinese(text);phonetic=True
        pcm,sr=k.create(text,voice=locale['voice'],speed=1.15,lang=lang if not phonetic else 'en-us',is_phonemes=phonetic)
        if not len(pcm) or not np.isfinite(pcm).all():raise ValueError(entry['key']+': invalid speech output')
        pcm=shout(np.asarray(pcm,dtype=np.float64),sr,entry)
        path=slf_voices.VOICES/(entry['key']+'.wav');write(path,pcm)
        item={'key':entry['key'],'fighter':who,'language':lang,'voice':locale['voice'],'text':locale['lines'][key],'duration':len(pcm)/18519,'peak':float(np.abs(pcm).max())}
        result.append(item);print(json.dumps(item,ensure_ascii=False),flush=True)
    (slf_voices.VOICES/'localized-recordings.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')

if __name__=='__main__':main()
