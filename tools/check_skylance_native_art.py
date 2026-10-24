#!/usr/bin/env python3
"""Verify native Aseprite exports against actual byte-swapped C-ROM tiles."""
import argparse,json,re,sys
from pathlib import Path
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'artbox'))
from tile_codec import decode_image
from palette_banks import palette_words
ART=ROOT/'games/skylance/artbox/generated'
def check():
    assets=json.loads((ART/'assets.json').read_text());header=(ART/'sky_assets.h').read_text()
    words={int(a):[int(v.strip().removesuffix('u'),0) for v in body.split(',') if v.strip()] for a,body in re.findall(r'sky_asset_(\d+)_pal\[\] = \{([^}]+)\}',header)}
    lanes=[(ART/f'779-{p}.{p}').read_bytes() for p in ('c1','c2')];checked=[];last=-1
    for a in assets:
        if a['tile_base']<=last or a['tile_last']>=0xfffd or (a['tile_base']<=0x49ff and a['tile_last']>=0x4900):raise ValueError(f"{a['name']}: reserved/overlapping C tiles")
        last=a['tile_last']
        if a['bank_base']+a['palette_count']>128:raise ValueError(f"{a['name']}: palette bank overrun")
        if not a.get('native_source'):continue
        p=ROOT/'games/skylance'/a['native_source']
        with Image.open(p) as im:
            if im.mode!='P' or im.info.get('transparency')!=0:raise ValueError(f'{p}: transparent indexed source required')
            indices=np.array(im);colors=np.array(im.getpalette()[:48],np.uint8).reshape(16,3)
            if indices.max()>15 or im.size!=(a['width'],a['height']):raise ValueError(f'{p}: palette/canvas mismatch')
        if a['name']!='title' and any((indices[0].any(),indices[-1].any(),indices[:,0].any(),indices[:,-1].any())):raise ValueError(f'{p}: clipped mechanical contour')
        if a['name']=='title' and not np.all(indices):raise ValueError(f'{p}: title backdrop must cover the complete native canvas')
        decoded=decode_image(*(data[a['tile_base']*64:] for data in lanes),a['width'],a['height'],swapped=True)
        if not np.array_equal(decoded,indices):raise ValueError(f'{p}: actual C-ROM differs from authored pixels')
        expected=list(map(int,palette_words(colors)))*a['palette_count']
        if words[a['id']]!=expected:raise ValueError(f'{p}: hardware palette differs from source')
        if not a['pivot'] or not(0<=a['pivot'][0]<a['width'] and 0<=a['pivot'][1]<a['height']):raise ValueError(f'{p}: invalid root anchor')
        checked.append({'id':a['id'],'name':a['name'],'tile_base':a['tile_base'],'native_indices_exact':True,'palette_words_exact':True,'pivot':a['pivot']})
    report={'passed':True,'assets_checked':len(checked),'assets':checked,'tile_high_water':last}
    (ART/'native-rom-check.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f"Native C-ROM indices/palette/anchors/margins: PASS ({len(checked)} assets)")
    return report
if __name__=='__main__':
    try:check()
    except Exception as exc:print(str(exc),file=sys.stderr);sys.exit(1)
