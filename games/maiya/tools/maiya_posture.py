"""Exact permitted walk/run back-contour edits; approved V4 sources stay intact."""
from pathlib import Path
import hashlib
import json
import re
import sys
import numpy as np
GAME=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(GAME.parents[1]/'artbox'))
from tile_codec import decode_image,encode_image

def recipe():
    return json.loads((GAME/'assets/authored/posture_correction.json').read_text())

def correct_frame(frame,indices):
    out=indices.copy()
    for p in recipe()['frames']:
        if p['frame']!=frame:continue
        if not 60<=frame<76 or hashlib.sha256(indices.tobytes()).hexdigest()!=p['base_indices_sha256']:
            raise ValueError('Posture source frame differs')
        seen=set()
        for x,y,old,new in p['edits']:
            if not (36<=x<=39 and 28<=y<=34 and 0<=old<16 and new in (1,4,10)) or (x,y) in seen:
                raise ValueError('Posture edit exceeds permitted back contour')
            if int(out[y,x])!=old:raise ValueError('Posture pixel source differs')
            seen.add((x,y));out[y,x]=new
    return out

def banks():
    spec=recipe();base=[(GAME/'artbox'/f'780-{p}.{p}').read_bytes() for p in ('c1','c2')]
    if [hashlib.sha256(b).hexdigest() for b in base]!=spec['base_graphics_sha256']:
        raise ValueError('Approved posture bank source differs')
    header=(GAME/'artbox/v4_reference/maiya_assets.h').read_text()
    tiles=[int(v,0) for v in re.findall(r'0x[0-9a-fA-F]+|\d+',re.search(r'mg_hero_tiles\[.*?\].*?\{(.*?)\}',header,re.S)[1])]
    result=[bytearray(b) for b in base]
    for p in spec['frames']:
        start=tiles[p['frame']]*64
        a=decode_image(base[0][start:start+1280],base[1][start:start+1280],80,64,swapped=True)
        corrected=correct_frame(p['frame'],a)
        encoded=encode_image(corrected,20)
        for dst,data in zip(result,encoded):dst[start:start+1280]=np.frombuffer(data,np.uint8).reshape(-1,2)[:,::-1].tobytes()
    result=[bytes(b) for b in result]
    if [hashlib.sha256(b).hexdigest() for b in result]!=spec['graphics_sha256']:
        raise ValueError('Posture correction differs from the recorded pixels')
    return result
