"""Native CD smoke test with explicitly supplied stock or source-built firmware.

Firmware is mapped and compared byte for byte. No firmware downloads.
"""
import argparse
import csv
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

if __package__ in (None,''):
    sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from tools.neocd.audit_disc import audit_cue,read_iso
from tools.neocd.build import default_compiler

ROOT=Path(__file__).resolve().parents[2]
# Public MAME 0.264 identification metadata, not ROM content. Hashes apply to
# the word-reversed files used by that driver; normalize before comparing.
STOCK_SHA1={
    '235f4d1d74364415910f73c10ae5482d90b4274f':'top',
    '53bc1f283cdf00fa2efbb79f2e36d4c8038d743a':'front',
    '7bb26d1e5d1e930515219cb18bcde5b7b23e2eda':'cdz',
}


def firmware_bytes(path,mode):
    if mode not in ('stock','eagle'):
        raise ValueError('Unknown CD firmware mode')
    path=Path(path)
    if not path.is_file():
        raise ValueError('Stock CD boot is blocked: supply your own legitimate 512 KiB CD BIOS with --bios')
    raw=path.read_bytes()
    if len(raw)!=0x80000:
        raise ValueError('CD firmware must be 512 KiB; cartridge BIOS padding is not supported')
    if raw[:4] != bytes.fromhex('0010f300'):
        swapped=bytearray(raw)
        swapped[0::2],swapped[1::2]=raw[1::2],raw[0::2]
        raw=bytes(swapped)
    if raw[:4]!=bytes.fromhex('0010f300'):
        raise ValueError('Unrecognized CD reset stack; inspect the firmware rather than guessing byte order')
    is_eagle=raw[0x700:0x708]==b'EAGLECD\0'
    if mode=='stock' and is_eagle:
        raise ValueError('Original diagnostic firmware must not be counted as a stock BIOS')
    pc=int.from_bytes(raw[4:8],'big')
    if pc&1 or not 0xC00000<=pc<0xC80000:
        raise ValueError('Invalid CD firmware reset vector')
    if mode=='eagle':
        if (not is_eagle or raw[0x708:0x70E]!=bytes.fromhex('000100070007') or
                int.from_bytes(raw[0x70E:0x710],'big')>2 or
                int.from_bytes(raw[0x710:0x714],'big')&2==0):
            raise ValueError('Eagle firmware does not advertise native disc boot')
        return raw
    mame_order=bytearray(raw)
    mame_order[0::2],mame_order[1::2]=raw[1::2],raw[0::2]
    if hashlib.sha1(mame_order).hexdigest() not in STOCK_SHA1:
        raise ValueError('Unrecognized stock firmware revision; add its verified identity before testing it as stock')
    return raw


def run(build,bios,mode,model,out,mame,nm):
    # Validate prerequisites before creating a test directory or copying firmware.
    raw=firmware_bytes(bios,mode)
    mame_order=bytearray(raw);mame_order[0::2],mame_order[1::2]=raw[1::2],raw[0::2]
    if mode=='stock' and STOCK_SHA1[hashlib.sha1(mame_order).hexdigest()]!=model:
        raise ValueError('Supplied stock firmware belongs to a different CD model')
    if mode=='eagle' and int.from_bytes(raw[0x70E:0x710],'big')!=('front','top','cdz').index(model):
        raise ValueError('Supplied Eagle firmware belongs to a different CD model')
    build=Path(build).resolve()
    metadata=json.loads((build/'source-manifest.json').read_text())
    if metadata.get('game')!='helloworld':
        raise ValueError('This probe knows the native helloworld contract only')
    cue=build/'disc/helloworld.cue'
    audit=audit_cue(cue)
    prg=read_iso((build/'disc/helloworld.iso').read_bytes())['HELLO.PRG']
    output=subprocess.check_output([nm,'-n',str(build/'program.elf')],text=True)
    symbols={name:int(address,16) for address,name in re.findall(r'^([0-9a-fA-F]+)\s+\w\s+(\w+)$',output,re.M)}
    out=Path(out).resolve();out.mkdir(parents=True,exist_ok=False)
    machine='neocdz' if model=='cdz' else 'neocd'
    folder=out/'roms'/machine;folder.mkdir(parents=True)
    name={'cdz':'neocd.bin','front':'front-sp1.bin','top':'top-sp1.bin'}[model]
    swapped=bytearray(raw);swapped[0::2],swapped[1::2]=raw[1::2],raw[0::2]
    (folder/name).write_bytes(swapped)
    spec=importlib.util.spec_from_file_location('cd_original_lo',ROOT/'bios/tools/gen_lo.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    (folder/'000-lo.lo').write_bytes(module.build_rom())
    values=dict(out=out.as_posix(),frames=symbols['ng_cd_frames'],pad=symbols['ng_cd_input_seen'],
                x=symbols['object_x'],length=len(prg))
    lua=out/'probe.lua'
    lua.write_text('local q={'+','.join(k+'='+json.dumps(v) for k,v in values.items())+'}\n'+r'''
local m=manager.machine
local mem=m.devices[':maincpu'].spaces['program']
local log=assert(io.open(q.out..'/probe.csv','w'))
log:write('seconds,frames,pad,x\n')
local function input(name,on)
 for _,port in pairs(m.ioport.ports) do
  if port.fields[name] then port.fields[name]:set_value(on and 1 or 0) end
 end
end
local captured=false
emu.register_frame_done(function()
 local t=m.time:as_double()
 input('P1 Start',t>2 and t<2.25)
 input('P1 Right',t>12 and t<12.3)
 input('P1 A',t>14 and t<14.2)
 log:write(string.format('%.4f,%d,%d,%d\n',t,mem:read_u32(q.frames),mem:read_u16(q.pad),mem:read_u16(q.x)))
 if t>10 and not captured then
  m.screens[':screen']:snapshot(q.out..'/before-input.png');captured=true
 end
 if t>=20 then
  local f=assert(io.open(q.out..'/loaded-program.bin','wb'))
  local bytes={}
  for i=0x100,q.length-1 do bytes[#bytes+1]=string.char(mem:read_u8(i)) end
  f:write(table.concat(bytes));f:close()
  m.screens[':screen']:snapshot(q.out..'/after-input.png')
  local b=assert(io.open(q.out..'/loaded-firmware.bin','wb'))
  for start=0,0x7ffff,4096 do
   local chunk={}
   for i=0,4095 do chunk[#chunk+1]=string.char(mem:read_u8(0xc00000+start+i)) end
   b:write(table.concat(chunk))
  end
  b:close();log:close();m:exit()
 end
end)
''')
    command=[mame,machine,'-bios','official' if model=='cdz' else model,
             '-noreadconfig','-skip_gameinfo','-rompath',str(out/'roms'),'-cdrom',str(cue),
             '-cfg_directory',str(out/'cfg'),'-nvram_directory',str(out/'nvram'),
             '-video','none','-sound','none','-wavwrite',str(out/'audio.wav'),
             '-nonvram_save','-nothrottle','-seconds_to_run','25',
             '-autoboot_delay','0','-autoboot_script',str(lua)]
    env=os.environ.copy();env.pop('DISPLAY',None);env.pop('WAYLAND_DISPLAY',None)
    env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',QT_QPA_PLATFORM='offscreen')
    with (out/'mame.log').open('w') as log:
        subprocess.run(command,env=env,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=180)
    rows=list(csv.DictReader((out/'probe.csv').open()))
    if not rows or int(rows[-1]['frames'])<120 or int(rows[-1]['pad'])&0x18!=0x18:
        raise ValueError('Native CD VBlank/input smoke test failed; inspect '+str(out))
    before=[int(r['x']) for r in rows if 11<float(r['seconds'])<12]
    after=[int(r['x']) for r in rows if 16<float(r['seconds'])<18]
    if not before or not after or max(after)<=max(before):
        raise ValueError('Input did not move the sprite')
    if (out/'loaded-program.bin').read_bytes()!=prg[0x100:]:
        raise ValueError('Disc program bytes differ from the native source-built PRG')
    if (out/'loaded-firmware.bin').read_bytes()!=raw:
        raise ValueError('Unexpected firmware was mapped into the emulator')
    from PIL import Image
    for name in ('before-input.png','after-input.png'):
        screenshot=Image.open(out/name).convert('RGB')
        if len(screenshot.getcolors(screenshot.width*screenshot.height))<3:
            raise ValueError('Native diagnostic screen is blank')
    report=dict(passed=True,gate='native '+mode+' smoke only',model=model,
                command=command,firmware_sha256=hashlib.sha256(raw).hexdigest(),
                image_sha256=audit['sha256'],frames=int(rows[-1]['frames']),
                second_scene=False,cdda_playback=False,save_persistence=False,hardware_tested=False)
    (out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build',type=Path,required=True)
    p.add_argument('--bios',type=Path,required=True)
    p.add_argument('--bios-mode',choices=('stock','eagle'),default='stock')
    p.add_argument('--model',choices=('front','top','cdz'),default='cdz')
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--mame',default=shutil.which('mame') or '/usr/games/mame')
    cc=default_compiler();suffix='.exe' if cc.endswith('.exe') else ''
    p.add_argument('--nm',default=cc[:-len('gcc'+suffix)]+'nm'+suffix)
    a=p.parse_args()
    if str(a.out) in ('.','') or str(a.build) in ('.',''):
        p.error('--out and --build must name explicit isolated directories')
    try:
        report=run(a.build,a.bios,a.bios_mode,a.model,a.out,a.mame,a.nm)
    except (ValueError,FileNotFoundError) as e:
        p.exit(2,str(e)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
