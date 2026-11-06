"""Build/reopen native Aseprite aircraft and verify every exported index."""
import argparse,json,os,shutil,subprocess
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[2]/'assets/authored'
SCRIPTS=Path(__file__).resolve().parent

def exe():
    value=os.environ.get('ASEPRITE') or shutil.which('aseprite') or '/mnt/c/Program Files/Aseprite/Aseprite.exe'
    if not Path(value).is_file():raise RuntimeError('Installed/licensed Aseprite required; set ASEPRITE')
    return value

def path(p,program):
    p=str(Path(p).resolve())
    return subprocess.check_output(['wslpath','-w',p],text=True).strip() if program.lower().endswith('.exe') else p

def run(author=False):
    program=exe();(ROOT/'frames').mkdir(exist_ok=True)
    for script in (['author_aircraft.lua'] if author else [])+['export_aircraft.lua']:
        subprocess.run([program,'-b','--script-param','root='+path(ROOT,program),'--script',path(SCRIPTS/script,program)],check=True)
    models=json.loads((ROOT/'aircraft_models.json').read_text());report=[]
    sheet=Image.new('RGB',(7*192,3*216),(24,29,40));draw=ImageDraw.Draw(sheet)
    for j,m in enumerate(models):
        colors=m['palette'];pal=sum(colors,[])+[0]*720
        for i,pose in enumerate(m['frames']):
            stem=ROOT/'frames'/f"aircraft_{m['name']}__{pose['name']}"
            raw=np.array([[int(c,16) for c in row] for row in stem.with_suffix('.txt').read_text().splitlines()],np.uint8)
            with Image.open(stem.with_suffix('.png')) as source:
                if source.mode!='P' or source.size!=(48,48) or not np.array_equal(np.array(source),raw):raise ValueError(f'{stem}: export differs from Aseprite data')
                if source.getpalette()[:48]!=sum(colors,[]):raise ValueError(f'{stem}: embedded palette mismatch')
            if raw.max()>15:raise ValueError(f'{stem}: main layer palette overflow')
            im=Image.fromarray(raw,'P');im.putpalette(pal);im.info['transparency']=0;im.save(stem.with_suffix('.png'),transparency=0)
            with Image.open(stem.with_suffix('.png')) as check:
                if not np.array_equal(np.array(check),raw) or check.info.get('transparency')!=0:raise ValueError(f'{stem}: indexed round trip failed')
            if np.any(raw[0]) or np.any(raw[-1]) or np.any(raw[:,0]) or np.any(raw[:,-1]):raise ValueError(f'{stem}: clipped canvas edge')
            if pose['tag']=='bank':
                if raw[m['nose'][1],24]==0:raise ValueError(f'{stem}: nose/anchor missing')
                if not np.any(raw[17:35,:22]) or not np.any(raw[17:35,27:]):raise ValueError(f'{stem}: missing wing')
                rgba=im.convert('RGBA').resize((192,192),Image.Resampling.NEAREST);sheet.paste(rgba,(i*192,j*216+20),rgba);draw.text((i*192+8,j*216+3),f"{m['name']} {pose['angle']} deg",fill='white')
            report.append({'model':m['name'],'pose':pose['name'],'indexed_roundtrip':True,'pivot':pose['pivot']})
        for tag in ('bank','boost','damage','destruction','aerobatics'):
            ps=[p for p in m['frames'] if p['tag']==tag];frames=[]
            for p in ps:
                im=Image.open(ROOT/'frames'/f"aircraft_{m['name']}__{p['name']}.png");frames.append(im.resize((192,192),Image.Resampling.NEAREST))
            frames[0].save(ROOT/f'aircraft_{m["name"]}_{tag}.gif',save_all=True,append_images=frames[1:],duration=[p['duration_ms'] for p in ps],loop=0,transparency=0,disposal=2)
    sheet.save(ROOT/'aircraft-model-sheet-4x.png');(ROOT/'aircraft-check.json').write_text(json.dumps({'passed':True,'frames':report},indent=2)+'\n')
    print('Aircraft indexed round trip/palette/anchors/clipping: PASS (81 frames)')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--author',action='store_true');a=p.parse_args();run(a.author)
