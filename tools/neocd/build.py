"""Build a native CD program and original diagnostic payloads, then author media."""
import argparse
import datetime
import importlib.util
import json
import math
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import wave

if __package__ in (None, ''):
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.neocd.build_disc import build as build_disc
from tools.neocd.formats import encode_spr

ROOT = Path(__file__).resolve().parents[2]
CD = ROOT / 'sdk/platform/cd'


def default_compiler():
    versions=('x-tools-v3-win','x-tools-v2-win','x-tools-v3','x-tools-v2','x-tools') if os.name=='nt' else (
        'x-tools-v3','x-tools-v2','x-tools','x-tools-v3-win','x-tools-v2-win')
    for version in versions:
        for suffix in (('.exe',) if os.name=='nt' else ('','.exe')):
            path = ROOT.parent / version / 'm68k-unknown-elf/bin' / ('m68k-unknown-elf-gcc'+suffix)
            if path.is_file():
                return str(path)
    return 'm68k-unknown-elf-gcc'


def module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def build_native(game, out, cc, plus=False, cdda=True, wlaz80='wla-z80', wlalink='wlalink'):
    if game == 'skylance':
        from tools.neocd.skylance import build_skylance
        return build_skylance(out,cc,plus,cdda,wlaz80,wlalink)
    if game == 'maiya':
        from tools.neocd.maiya import build_maiya
        return build_maiya(out,cc,plus,cdda,wlaz80,wlalink)
    if game != 'helloworld':
        raise ValueError('CD startup is currently enabled for helloworld only; '+game+
                         ' needs its own audited source/asset port, not a renamed cartridge')
    out = Path(out).resolve()
    if out.exists() and any(out.iterdir()):
        raise ValueError('Use a new CD build directory; previous outputs are preserved')
    out.mkdir(parents=True, exist_ok=True)
    obj, payload = out/'obj', out/'payload'
    obj.mkdir();payload.mkdir()
    cc = shutil.which(cc) or cc
    suffix = '.exe' if cc.endswith('.exe') else ''
    prefix = cc[:-len('gcc'+suffix)]
    flags = ['-m68000', '-O2', '-ffreestanding', '-fno-builtin', '-fno-common',
             '-ffunction-sections', '-fdata-sections', '-Wall', '-Wextra',
             '-I'+str(ROOT), '-I'+str(ROOT/'sdk'), '-I'+str(ROOT/'sdk/2d_engine'),
             '-I'+str(CD), '-DNG_PLATFORM_CD=1', '-DNG_CD_GAME_ID=0x0772',
             '-include', str(CD/'ng_cd_game.h')]
    sources = [CD/'ng_cd_boot.S',CD/'ng_cd_bios.S', *sorted(CD.glob('*.c')),
               ROOT/'sdk/2d_engine/ng_fix.c',ROOT/'games/helloworld/main.c']
    objects = []
    for source in sources:
        path = obj/(source.stem+'.o')
        compiler = cc
        lang = ['-std=gnu99'] if source.suffix == '.c' else []
        if plus and source.name in ('main.c', 'ng_fix.c'):
            compiler=prefix+'g++'+suffix
            lang=['-x','c++','-std=c++14','-fno-exceptions','-fno-rtti']
        subprocess.run([compiler,*flags,*lang,'-c',str(source),'-o',str(path)],check=True)
        objects.append(str(path))
    elf = out/'program.elf'
    subprocess.run([cc,'-m68000','-nostdlib','-Wl,--gc-sections','-Wl,-T,'+str(CD/'neocd.ld'),
                    '-Wl,-Map,'+str(out/'program.map'),*objects,'-lgcc','-o',str(elf)],check=True)
    subprocess.run([prefix+'objcopy'+suffix,'-O','binary',str(elf),str(payload/'HELLO.PRG')],check=True)
    font=module(ROOT/'bios/tools/gen_sfix.py','cd_original_font').build_rom()
    (payload/'FONT.FIX').write_bytes(font)
    tile=[[0]*16 for _ in range(16)]
    for y in range(2,14):
        for x in range(2,14):
            if abs(x-7)+abs(y-7)<8:
                tile[y][x]=1+(x+y)%14
    (payload/'BOOT.SPR').write_bytes(bytes(128)+encode_spr(tile))
    (payload/'SCENE2.SPR').write_bytes(encode_spr([row[::-1] for row in tile]))
    subprocess.run([wlaz80,'-o',str(obj/'sound.o'),str(CD/'ng_cd_sound.s')],check=True)
    link=obj/'sound.link'
    link.write_text('[objects]\n'+str(obj/'sound.o')+'\n')
    subprocess.run([wlalink,'-r',str(link),str(payload/'SOUND.Z80')],check=True)
    sys.path.insert(0,str(ROOT/'sound/tools'))
    encoder=module(ROOT/'sound/tools/adpcm_enc.py','cd_adpcm_encoder')
    samples=[int(12000*math.sin(2*math.pi*660*i/18518)*math.exp(-i/1000)) for i in range(8192)]
    pcm,_preroll=encoder.encode_adpcma(samples)
    if len(pcm)!=4096:
        raise ValueError('Diagnostic sample must fit the RAM driver sample table')
    (payload/'CUE.PCM').write_bytes(pcm)
    if cdda:
        audio=payload/'MUSIC.WAV'
        with wave.open(str(audio),'wb') as w:
            w.setnchannels(2);w.setsampwidth(2);w.setframerate(44100)
            frames=bytearray()
            for i in range(44100*4):
                envelope=min(1,i/2205,(44100*4-i)/2205)
                value=int(envelope*1800*(math.sin(2*math.pi*220*i/44100)+math.sin(2*math.pi*330*i/44100)))
                frames.extend(struct.pack('<hh',value,value))
            w.writeframes(frames)
    files=[dict(name=n,source='payload/'+n,group='boot') for n in
           ('FONT.FIX','BOOT.SPR','SOUND.Z80','CUE.PCM','HELLO.PRG')]
    files.append(dict(name='SCENE2.SPR',source='payload/SCENE2.SPR',group='scene2',bank=1))
    manifest=dict(format='neogeosdk-cd-1',game=game,volume='EAGLE_HELLO',
                  groups={'boot':{'depends':[]},'scene2':{'depends':['boot']}},files=files,
                  audio=['payload/MUSIC.WAV'] if cdda else [])
    manifest_path=out/'source-manifest.json'
    manifest_path.write_text(json.dumps(manifest,indent=2)+'\n')
    return build_disc(manifest_path,out/'disc',int(os.environ.get('SOURCE_DATE_EPOCH','946684800')))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--game',default='helloworld')
    p.add_argument('--out',type=Path,help='New/empty output directory; default: timestamped out/neocd/<game>/')
    p.add_argument('--cc',default=default_compiler())
    p.add_argument('--plus',action='store_true')
    p.add_argument('--no-cdda',action='store_true')
    p.add_argument('--wlaz80',default='wla-z80')
    p.add_argument('--wlalink',default='wlalink')
    a=p.parse_args()
    if a.out is None:
        stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
        a.out=ROOT/'out/neocd'/a.game/stamp
    result=build_native(a.game,a.out,a.cc,a.plus,not a.no_cdda,a.wlaz80,a.wlalink)
    print('CD build: '+str(a.out.resolve()))
    print(json.dumps(result['image_audit'],indent=2))


if __name__=='__main__':
    main()
