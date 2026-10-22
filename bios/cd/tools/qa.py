"""Isolated original-CD-firmware hello-screen probe, not a disc boot test."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--firmware', type=Path, required=True)
    p.add_argument('--model', choices=('front', 'top', 'cdz'), required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--mame', default=shutil.which('mame') or '/usr/games/mame')
    p.add_argument('--nm', default=str(ROOT.parent / 'x-tools-v3/m68k-unknown-elf/bin/m68k-unknown-elf-nm'))
    a = p.parse_args()
    out, firmware = a.out.resolve(), a.firmware.resolve()
    out.mkdir(parents=True, exist_ok=False)
    raw = (firmware / ('eagle_cd_' + a.model + '.rom')).read_bytes()
    if len(raw) != 0x80000 or raw[0x700:0x708] != b'EAGLECD\0':
        raise ValueError('Not a source-built Eagle CD diagnostic ROM')
    machine = 'neocdz' if a.model == 'cdz' else 'neocd'
    filename = {'cdz': 'neocd.bin', 'front': 'front-sp1.bin', 'top': 'top-sp1.bin'}[a.model]
    folder = out / 'roms' / machine
    folder.mkdir(parents=True)
    swapped = bytearray(raw)
    swapped[0::2], swapped[1::2] = raw[1::2], raw[0::2]
    (folder / filename).write_bytes(swapped)
    spec = importlib.util.spec_from_file_location('original_eagle_lo', ROOT / 'bios/tools/gen_lo.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    (folder / '000-lo.lo').write_bytes(module.build_rom())
    symbols = subprocess.check_output([a.nm, '-n', str(firmware / a.model / 'eagle_cd.elf')], text=True)
    symbols = {name: int(address, 16) for address, name in
               re.findall(r'^([0-9a-fA-F]+)\s+\w\s+(\w+)$', symbols, re.M)}
    config = dict(output=str(out), frames=symbols['eagle_cd_frames'], pad=symbols['eagle_cd_pad_seen'])
    script = out / 'probe.lua'
    script.write_text('local q=' + '{' + ','.join(k + '=' + json.dumps(v) for k, v in config.items()) + '}\n' + r'''
local m=manager.machine
local cpu=m.devices[':maincpu']
local mem=cpu.spaces['program']
local screen=m.screens[':screen']
local function register(names)
 for _,name in ipairs(names) do if cpu.state[name] then return cpu.state[name].value end end
 error('Emulator does not expose CPU state '..table.concat(names,'/'))
end
local log=assert(io.open(q.output..'/probe.csv','w'))
log:write('seconds,pc,frames,pad,sp\n')
local dumped=false
local function input(name,on)
 for _,port in pairs(m.ioport.ports) do
  if port.fields[name] then port.fields[name]:set_value(on and 1 or 0) end
 end
end
emu.register_frame_done(function()
 local t=m.time:as_double()
 input('P1 A',t>2 and t<2.25)
 input('P1 Right',t>3 and t<3.25)
 if t>1 and not dumped then
  local f=assert(io.open(q.output..'/loaded-firmware.bin','wb'))
  for start=0,0x7ffff,4096 do
   local bytes={}
   for i=0,4095 do bytes[#bytes+1]=string.char(mem:read_u8(0xc00000+start+i)) end
   f:write(table.concat(bytes))
  end
  f:close();dumped=true
  screen:snapshot(q.output..'/menu.png')
 end
 log:write(string.format('%.4f,%d,%d,%d,%d\n',t,register({'PC','CURPC'}),
  mem:read_u32(q.frames),mem:read_u16(q.pad),register({'SP','A7','CURSP'})))
 if t>=4 then
  screen:snapshot(q.output..'/input.png');log:close();m:exit()
 end
end)
''')
    env = os.environ.copy()
    env.pop('DISPLAY', None)
    env.pop('WAYLAND_DISPLAY', None)
    env.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', QT_QPA_PLATFORM='offscreen')
    command = [a.mame, machine, '-bios', 'official' if a.model == 'cdz' else a.model,
               '-noreadconfig', '-skip_gameinfo', '-video', 'none', '-sound', 'none',
               '-nothrottle', '-nonvram_save', '-seconds_to_run', '8',
               '-rompath', str(out / 'roms'), '-cfg_directory', str(out / 'cfg'),
               '-nvram_directory', str(out / 'nvram'), '-autoboot_delay', '0',
               '-autoboot_script', str(script)]
    with (out / 'mame.log').open('w') as log:
        subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=90)
    if (out / 'loaded-firmware.bin').read_bytes() != raw:
        raise ValueError('Emulator did not execute the exact original firmware bytes')
    import csv
    rows = list(csv.DictReader((out / 'probe.csv').open()))
    if not rows:
        raise ValueError('No firmware samples; inspect mame.log for scripting errors')
    last = rows[-1]
    if int(last['frames']) < 100 or int(last['pad']) & 0x18 != 0x18:
        raise ValueError('Firmware VBlank/input probe failed: ' + str(last))
    settled = [r for r in rows if float(r['seconds']) > 1]
    if any(int(b['frames']) < int(a['frames']) for a, b in zip(settled, settled[1:])):
        raise ValueError('Firmware reset during the probe (frame clock moved backward)')
    if any(not 0xC00000 <= int(r['pc']) < 0xC80000 or
           not 0x1FC000 <= int(r['sp']) <= 0x1FF000 for r in settled):
        raise ValueError('Firmware left its ROM/stack bounds')
    from PIL import Image
    screenshot = Image.open(out / 'menu.png').convert('RGB')
    if len(screenshot.getcolors(screenshot.width * screenshot.height)) < 2:
        raise ValueError('Firmware menu is blank')
    report = dict(passed=True, gate='F1 diagnostic only', model=a.model, command=command,
                  firmware_sha256=hashlib.sha256(raw).hexdigest(), frames=int(last['frames']),
                  pad_seen=int(last['pad']), disc_boot=False, hardware_tested=False)
    (out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
