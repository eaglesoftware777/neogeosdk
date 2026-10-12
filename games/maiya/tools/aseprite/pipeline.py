"""Version-tested Aseprite CLI author/export; preserve and verify palette indices."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[2]/'assets/authored'
SCRIPTS=Path(__file__).resolve().parent
LAYERS=('far_arm','far_leg','near_leg','torso','head_hair','near_arm')
def aseprite():
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

def export(author=False):
    exe=aseprite();root=native_path(ROOT,exe);(ROOT/'layers').mkdir(exist_ok=True)
    if author:run(exe,'--script-param','root='+root,'--script',native_path(SCRIPTS/'build_walkrun.lua',exe))
    run(exe,'--script-param','root='+root,'--script',native_path(SCRIPTS/'export_walkrun.lua',exe))
    poses=json.loads((ROOT/'poses.json').read_text());colors=json.loads((ROOT/'palette.json').read_text())['colors']
    for mode in ('walk','run','stop'):
        target=ROOT/f'maiya_{mode}.png'
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
