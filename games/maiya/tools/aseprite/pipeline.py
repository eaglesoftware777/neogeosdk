"""Version-tested Aseprite CLI author/export; preserve and verify palette indices."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[2]/'assets/authored'
SCRIPTS=Path(__file__).resolve().parent
LAYERS=('far_arm','far_leg','near_leg','torso','head_hair','near_arm')
def aseprite():
    # ASEPRITE=builtin: author/reopen/export with ase_file.py instead of the
    # application (a machine without Aseprite); the files are the same.
    if os.environ.get('ASEPRITE')=='builtin':return 'builtin'
    exe=os.environ.get('ASEPRITE') or shutil.which('aseprite')
    if not exe:
        installed=Path('/mnt/c/Program Files/Aseprite/Aseprite.exe')
        if installed.is_file():exe=str(installed)
    if not exe:raise RuntimeError('Aseprite required; set ASEPRITE to the installed executable')
    return exe

def native_path(path,exe):
    path=str(Path(path).resolve())
    return subprocess.check_output(['wslpath','-w',path],text=True).strip() if exe.lower().endswith('.exe') else path

def run(exe,*args):
    subprocess.run([exe,'-b',*args],check=True)

def indices(path):
    rows=Path(path).read_text().splitlines()
    if len(rows)!=64 or any(len(row)!=80 for row in rows):raise ValueError(f'{path}: malformed pixel map')
    return np.array([[0 if c=='.' else int(c,16) for c in row] for row in rows],np.uint8)

def normalize(path,expected,colors):
    with Image.open(path) as im:
        if im.mode!='P':raise ValueError(f'{path}: Aseprite did not export indexed pixels')
        if not np.array_equal(np.array(im),expected):raise ValueError(f'{path}: indices differ from reopened Aseprite image')
        for i,c in enumerate(colors):
            if im.getpalette()[i*3:i*3+3]!=c:raise ValueError(f'{path}: palette slot {i} differs')
    out=Image.fromarray(expected,'P');out.putpalette(sum(colors,[])+[0]*720);out.save(path,transparency=0)
    with Image.open(path) as check:
        if check.mode!='P' or check.info.get('transparency')!=0 or not np.array_equal(np.array(check),expected):
            raise ValueError(f'{path}: postprocessing changed pixels/transparency')

def builtin_author(poses,colors):
    """build_walkrun.lua: the saved source from the text maps."""
    sys.path.insert(0,str(SCRIPTS));import ase_file
    frames=[]
    for p in poses:
        ref=indices(ROOT/'maps'/(('reference_idle0' if p['mode']=='stop' else 'reference_stride')+'.txt')).copy()
        ref[61,1:79]=14;ref[10,1:79]=14
        speed=800/256 if p['mode']=='run' else 512/256*80/64
        seconds=4/60 if p['mode']=='stop' else p['hold_distance']/speed/60
        frames.append((int(seconds*1000),[ref,*(indices(ROOT/'maps'/f"{p['name']}__{l}.txt") for l in LAYERS)]))
    first,last={},{}
    for i,p in enumerate(poses):first.setdefault(p['mode'],i);last[p['mode']]=i
    ase_file.write(ROOT/'maiya_walkrun.aseprite',80,64,colors,[('ref',False,False),*((l,True,True) for l in LAYERS)],
                   frames,[(m,first[m],last[m]) for m in first])

def builtin_export(poses,colors):
    """export_walkrun.lua and the --sheet/--data exports, from the saved source."""
    sys.path.insert(0,str(SCRIPTS));import ase_file
    doc=ase_file.read(ROOT/'maiya_walkrun.aseprite')
    if (doc['width'],doc['height'],doc['transparent'],len(doc['frames']))!=(80,64,0,len(poses)):
        raise ValueError('maiya_walkrun.aseprite: not the 80x64 indexed walk/run source')
    def save(image,name):
        out=Image.fromarray(image,'P');out.putpalette(sum(colors,[])+[0]*720);out.save(ROOT/'layers'/f'{name}.png',transparency=0)
        # Aseprite on Windows writes its text exports with CRLF line ends
        (ROOT/'layers'/f'{name}.txt').write_bytes(''.join(''.join('.' if v==0 else format(int(v),'X') for v in row)+'\r\n' for row in image).encode())
    flats=[]
    for i,p in enumerate(poses):
        flats.append(ase_file.flatten(doc,i));save(flats[-1],p['name']+'__flat')
        for index,layer in enumerate(doc['layers']):
            if layer['name']!='ref':save(doc['frames'][i]['cels'][index]['image'],p['name']+'__'+layer['name'])
    info=dict(width=80,height=64,indexed=True,transparentColor=0,colors=[c[:4] for c in doc['colors'][:16]],
              layers=[dict(name=l['name'],background=False,visible=l['visible'],editable=l['editable']) for l in doc['layers']],
              tags=[{'name':t['name'],'from':t['from']+1,'to':t['to']+1} for t in doc['tags']])
    (ROOT/'aseprite_source.json').write_text(json.dumps(info,sort_keys=True))
    for tag in doc['tags']:
        mode,span=tag['name'],range(tag['from'],tag['to']+1)
        sheet=Image.fromarray(np.concatenate([flats[i] for i in span],axis=1),'P');sheet.putpalette(sum(colors,[])+[0]*720)
        sheet.save(ROOT/f'maiya_{mode}.png',transparency=0)
        names=[f'maiya_walkrun #{mode}.aseprite'] if len(span)==1 else [f'maiya_walkrun #{mode} {k}.aseprite' for k in range(len(span))]
        frames=',\n'.join(f'   "{n}": {{\n    "frame": {{ "x": {k*80}, "y": 0, "w": 80, "h": 64 }},\n    "rotated": false,\n    "trimmed": false,\n'
                          f'    "spriteSourceSize": {{ "x": 0, "y": 0, "w": 80, "h": 64 }},\n    "sourceSize": {{ "w": 80, "h": 64 }},\n'
                          f'    "duration": {doc["frames"][i]["duration"]}\n   }}' for k,(n,i) in enumerate(zip(names,span)))
        (ROOT/f'{mode}_sheet.json').write_bytes(('{ "frames": {\n'+frames+'\n },\n "meta": {\n  "app": "games/maiya/tools/aseprite/ase_file.py",\n'
            '  "version": "ASEPRITE=builtin",\n  "image": "'+f'maiya_{mode}.png'+'",\n  "format": "I8",\n'
            f'  "size": {{ "w": {80*len(span)}, "h": 64 }},\n  "scale": "1"\n }}\n}}\n').replace('\n','\r\n').encode())

def export(author=False):
    exe=aseprite();(ROOT/'layers').mkdir(exist_ok=True)
    poses=json.loads((ROOT/'poses.json').read_text());colors=json.loads((ROOT/'palette.json').read_text())['colors']
    if exe=='builtin':
        if author:builtin_author(poses,colors)
        builtin_export(poses,colors)
    else:
        root=native_path(ROOT,exe)
        if author:run(exe,'--script-param','root='+root,'--script',native_path(SCRIPTS/'build_walkrun.lua',exe))
        run(exe,'--script-param','root='+root,'--script',native_path(SCRIPTS/'export_walkrun.lua',exe))
    for mode in ('walk','run','stop'):
        target=ROOT/f'maiya_{mode}.png'
        if exe!='builtin':
            run(exe,'--tag',mode,native_path(ROOT/'maiya_walkrun.aseprite',exe),'--sheet-type','horizontal',
                '--border-padding','0','--shape-padding','0','--inner-padding','0','--data',native_path(ROOT/f'{mode}_sheet.json',exe),'--sheet',native_path(target,exe))
        expected=np.concatenate([indices(ROOT/'layers'/f"{p['name']}__flat.txt") for p in poses if p['mode']==mode],axis=1)
        normalize(target,expected,colors)
    for p in poses:
        for layer in (*LAYERS,'flat'):
            stem=ROOT/'layers'/f"{p['name']}__{layer}"
            expected=indices(stem.with_suffix('.txt'))
            if layer!='flat' and not np.array_equal(expected,indices(ROOT/'maps'/f"{p['name']}__{layer}.txt")):
                raise ValueError(f'{stem}: saved source differs from authored map')
            normalize(stem.with_suffix('.png'),expected,colors)
    print('Aseprite reopen/export and indexed PNG round-trip: PASS')

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--author',action='store_true');args=parser.parse_args();export(args.author)
