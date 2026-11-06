"""Reopen Aseprite production sources, require exact indexed round trips."""
import argparse,json,subprocess
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw
try:
    from .aircraft_pipeline import ROOT,SCRIPTS,exe,path
except ImportError:
    from aircraft_pipeline import ROOT,SCRIPTS,exe,path

def run(author=False):
    program=exe()
    if author:
        import sys
        sys.path.insert(0,str(SCRIPTS.parent))
        from title_pixels import build as build_title
        build_title()
        from unit_frames import run as build_units
        build_units()
    for script in (['author_stage_one.lua'] if author else [])+['export_production.lua']:
        subprocess.run([program,'-b','--script-param','root='+path(ROOT,program),'--script-param','scripts='+path(SCRIPTS,program),'--script',path(SCRIPTS/script,program)],check=True)
    assets=json.loads((ROOT/'production_assets.json').read_text());models={m['name']:m for m in json.loads((ROOT/'production_models.json').read_text())};report=[]
    sheet=Image.new('RGB',(6*224,4*240),(25,33,44));draw=ImageDraw.Draw(sheet);slot=0
    for a in assets:
        p=ROOT/a['png'];raw=np.array([[int(c,16) for c in r] for r in p.with_suffix('.txt').read_text().splitlines()],np.uint8)
        with Image.open(p) as im:
            if im.mode!='P' or not np.array_equal(np.array(im),raw):raise ValueError(f'{p}: indexed export mismatch')
            palette=im.getpalette()[:48]
            if palette!=sum(models[a['source'].removesuffix('.aseprite')]['palette'],[]):raise ValueError(f'{p}: saved palette mismatch')
        if a['kind']!='title' and any((raw[0].any(),raw[-1].any(),raw[:,0].any(),raw[:,-1].any())):raise ValueError(f'{p}: clipped hull')
        out=Image.fromarray(raw,'P');out.putpalette(palette+[0]*720);out.info['transparency']=0;out.save(p,transparency=0)
        with Image.open(p) as im:
            if not np.array_equal(np.array(im),raw) or im.info.get('transparency')!=0:raise ValueError(f'{p}: normalized export differs')
        if a['kind']=='title':out.resize((640,448),Image.Resampling.NEAREST).save(ROOT/'title-native-2x.png')
        elif a['frame']==1:
            rgba=out.convert('RGBA').resize((out.width*2,out.height*2),Image.Resampling.NEAREST);x=(slot%6)*224;y=(slot//6)*240+16;sheet.paste(rgba,(x,y),rgba);draw.text((x+4,y-14),a['name'],fill='white');slot+=1
        report.append({'asset':a['name'],'indexed_exact':True,'pivot':a['pivot']})
    # Strict shadow normalization keeps indices exact and leaves the shared gray bank unchanged.
    for p in (ROOT/'frames').glob('shadow_*.png'):
        with Image.open(p) as im:
            raw=np.array(im);palette=im.getpalette()[:48]
            if im.mode!='P' or raw.max()>15:raise ValueError(f'{p}: invalid shadow indices')
        out=Image.fromarray(raw,'P');out.putpalette(palette+[0]*720);out.save(p,transparency=0)
    sheet.save(ROOT/'stage-one-model-sheet-2x.png');(ROOT/'production-check.json').write_text(json.dumps({'passed':True,'assets':report},indent=2)+'\n')
    print(f'Native production palette/pixels/clipping: PASS ({len(assets)} frames)')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--author',action='store_true');a=p.parse_args();run(a.author)
