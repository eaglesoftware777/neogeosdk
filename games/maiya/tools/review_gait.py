"""Normal-input installed walk/run review for either heroine and platform.

Reads a fresh DWARF layout and the hardware's complete 20-tile group.
Selects Luna through the actual chooser; never writes game memory or
enables the pale-colour protection used in the longer stress inspection.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
import subprocess

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]


def windows(path):
    return subprocess.check_output(['wslpath', '-w', str(path.resolve())], text=True).strip()


def array(header, name):
    body = re.search(r'\b' + name + r'\[.*?\]\s*=\s*\{(.*?)\}', header, re.S).group(1)
    return [int(x, 0) for x in re.findall(r'0x[0-9A-Fa-f]+|\d+', body)]


def run(platform, hero, work, target, output):
    output.mkdir(parents=True, exist_ok=True)
    expressions = {'base': '(unsigned long)&mg', 'pick': '(unsigned long)&mg_chooser_pick'}
    for field in ('player', 'state', 'demo', 'veil', 'hurt', 'flash', 'hero_choice', 'facing'):
        expressions[field] = f'(unsigned long)&mg.{field}-(unsigned long)&mg'
    for field in ('sprite_tile', 'sprite_first', 'x', 'y', 'scale_x', 'scale_y'):
        expressions[field] = f'(unsigned long)&((NGCharacter*)0)->{field}'
    command = [str(ROOT.parent / 'x-tools-v3/m68k-unknown-elf/bin/m68k-unknown-elf-gdb'),
               '-batch', str(work / 'out/game')]
    for name, expression in expressions.items():
        command += ['-ex', f'printf "{name}=%lu\\n", {expression}']
    log = subprocess.check_output(command, text=True)
    layout = {k: int(v) for k, v in re.findall(r'^(\w+)=(\d+)$', log, re.M)}
    if set(layout) != set(expressions):
        raise ValueError('Incomplete fresh layout: ' + log)
    layout['output'] = windows(output).replace('\\', '/')
    layout['hero'] = 1 if hero == 'luna' else 0
    (output / 'layout.json').write_text(json.dumps(layout, indent=2) + '\n')
    config = '{' + ','.join(k + '=' + (json.dumps(v) if isinstance(v, str) else str(v))
                            for k, v in layout.items()) + '}'
    lua = 'local q=' + config + '\n' + r'''
local m=manager.machine
local mem=m.devices[':maincpu'].spaces['program']
local screen=m.screens[':screen']
local vram=emu.item(m.devices[':spritegen'].items['0/m_videoram'])
local file=assert(io.open(q.output..'/play.csv','w'))
file:write('time,phase,state,hero,tile,hardware_tile,x,y,veil,hurt,flash,facing,sx,sy')
for y=0,3 do for x=0,4 do file:write(',r'..y..'c'..x) end end
for i=1,15 do file:write(',pal'..i) end
file:write('\n')
local start,chooser=nil,nil
local seen={}
local startup=assert(io.open(q.output..'/startup.csv','w'))
startup:write('time,state,player,demo,title,pick,hero\n')
local lastsecond=-1
local function input(n,v)
 for _,p in pairs(m.ioport.ports) do if p.fields[n] then p.fields[n]:set_value(v and 1 or 0) end end
end
local function choose_title()
 local text='CHOOSE'
 for i=1,#text do
  -- ng_fix commits logical cells at FIXMAP + 2, including row 2.
  if vram:read(0x7002+(9+i)*32+2)%4096 ~= 0xD00+string.byte(text,i) then return false end
 end
 return true
end
emu.register_frame_done(function()
 local t=m.time:as_double()
 local p=mem:read_u32(q.base+q.player)
 local state=mem:read_u8(q.base+q.state)
 local active=p>=0x100000 and p<0x10efff and state==1 and mem:read_u8(q.base+q.demo)==0
 if not start then
  input('Coin 1',t>8 and t<8.3)
  local go=t>12 and t<16 and ((t-12)%2<0.12)
  input('1 Player Start',go);input('P1 Start',go)
  local title=choose_title()
  if title and not chooser then chooser=t end
  local second=math.floor(t)
  if second~=lastsecond then
   startup:write(string.format('%.4f,%d,%d,%d,%d,%d,%d\n',t,state,p,
    mem:read_u8(q.base+q.demo),title and 1 or 0,mem:read_u8(q.pick),mem:read_u8(q.base+q.hero_choice)))
   startup:flush();lastsecond=second
   if t>12 and second%4==0 then screen:snapshot(q.output..'/startup_'..second..'.png') end
  end
  local picked=mem:read_u8(q.pick)==q.hero
  -- The chooser deliberately polls away the opening inputs after its
  -- fade. A held direction from before that poll has no new edge.
  -- Release/repress after the fade, and stop as soon as Luna is picked.
  input('P1 Right',title and not picked and chooser and t-chooser>0.5 and
                    ((t-chooser-0.5)%0.5<0.08))
  input('P1 A',t>13 and t<19 and ((t-13)%0.75<0.12) and
                 (not title or (picked and chooser and t-chooser>0.45)))
  if t>20 and active then
   start=t
   local selected=assert(io.open(q.output..'/selection.json','w'))
   selected:write(string.format('{"chooser_seen":%s,"pick":%d,"hero":%d}',
    chooser and 'true' or 'false',mem:read_u8(q.pick),mem:read_u8(q.base+q.hero_choice)))
   selected:close()
   startup:close()
  end
  return
 end
 local r=t-start
 local walk=r>0.2 and r<2.5
 local run=r>2.7 and r<3.6
 input('Coin 1',false);input('1 Player Start',false);input('P1 Start',false)
 input('P1 A',false);input('P1 B',false);input('P1 Left',false)
 input('P1 Right',(walk and ((r-0.2)%0.4<0.16)) or run)
 if active then
  local tile=mem:read_u16(p+q.sprite_tile)
  local first=mem:read_u16(p+q.sprite_first)
  local hw=vram:read(first*64)
  file:write(string.format('%.4f,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d',t,
   walk and 'walk' or run and 'run' or 'settle',state,mem:read_u8(q.base+q.hero_choice),
   tile,hw,mem:read_u16(p+q.x),mem:read_u16(p+q.y),mem:read_u16(q.base+q.veil),
   mem:read_u8(q.base+q.hurt),mem:read_u8(q.base+q.flash),mem:read_u8(q.base+q.facing),
   mem:read_u8(p+q.scale_x),mem:read_u8(p+q.scale_y)))
  for y=0,3 do for x=0,4 do file:write(','..vram:read((first+x)*64+y*2)) end end
  for i=1,15 do file:write(','..mem:read_u16(0x400000+4*32+i*2)) end
  file:write('\n')
  if r>0.3 and not seen[hw] then screen:snapshot(q.output..'/gameplay_'..hw..'.png');seen[hw]=true end
 end
 if r>3.8 then file:close();m:exit() end
end)
'''
    script = output / 'boot.lua'
    script.write_text(lua)
    command = ['/mnt/c/mame/mame.exe', 'aes' if platform == 'aes' else 'neogeo',
               '-cart1', 'maiya', '-bios', 'asia' if platform == 'aes' else 'euro',
               '-noreadconfig', '-skip_gameinfo', '-video', 'none', '-sound', 'none',
               '-nothrottle', '-nonvram_save', '-seconds_to_run', '45',
               '-rompath', windows(target / 'tests' / platform / 'roms') + ';' +
               windows(target / 'roms') + ';C:\\mame\\roms',
               '-hashpath', windows(target / 'tests' / platform / 'hash'),
               '-cfg_directory', windows(output / 'cfg'), '-nvram_directory', windows(output / 'nvram'),
               '-autoboot_delay', '0', '-autoboot_script', windows(script)]
    with (output / 'mame.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=180)
    rows = list(csv.DictReader((output / 'play.csv').open()))
    selection_file = output / 'selection.json'
    assert selection_file.is_file(), 'Ordinary play never started; inspect startup.csv and snapshots'
    selection = json.loads(selection_file.read_text())
    assert selection['chooser_seen'], 'Real chooser was never observed'
    assert selection['pick'] == selection['hero'] == layout['hero'], 'Real chooser selection differs'
    assert len(rows) > 200, 'Incomplete normal-input gameplay'
    assert all(int(r['hero']) == layout['hero'] for r in rows), 'Actual chooser selected the wrong heroine'
    header = (GAME / 'artbox/generated/maiya_assets.h').read_text()
    tiles = array(header, 'mg_hero_tiles')
    observed = {int(r['hardware_tile']) for r in rows}
    for mode, ids in [('walk', tiles[60:68]), ('run', tiles[68:76])]:
        assert set(ids) <= observed, 'Missing installed hardware ' + mode + ' poses'
        if mode=='walk':
            assert set(ids)<={int(r['hardware_tile']) for r in rows if r['phase']=='walk'}, 'Incomplete walking phase'
        assert all((output / f'gameplay_{i}.png').is_file() for i in ids), 'Missing ' + mode + ' previews'
    gait = [r for r in rows if int(r['tile']) in tiles[60:76]]
    assert all(int(r['sx']) == 255 and int(r['sy']) == 255 for r in gait), 'Gait shrink/shimmer'
    for i, row in enumerate(rows):
        candidates = [row] + ([rows[i - 1]] if i else [])
        assert any(all(int(row[f'r{y}c{x}']) == int(pose['tile']) + y * 5 +
                       (4 - x if int(pose['facing']) else x)
                       for y in range(4) for x in range(5)) for pose in candidates), \
            f'Incomplete or mixed body tile group at sample {i}'
    expected = array(header, 'mg_hero_alt_pal' if hero == 'luna' else 'mg_hero_pal')
    normal = [r for r in rows if not any(int(r[n]) for n in ('veil', 'hurt', 'flash'))]
    assert len(normal) > 100, 'Insufficient ordinary-palette coverage'
    assert all(int(r[f'pal{i}']) == expected[i] for r in normal for i in range(1, 16)), \
        'Unexpected palette shadow/flicker in normal movement'
    roms = target / 'tests' / platform / 'roms/maiya'
    for part in ('c1', 'c2'):
        name = f'780-{part}.{part}'
        assert (roms / name).read_bytes() == (GAME / 'artbox/generated' / name).read_bytes(), 'Installed art stale'
    result = dict(passed=True, platform=platform, hero=hero, normal_inputs_only=True,
                  chooser=selection, gameplay_frames=len(rows), walk_poses=8, run_poses=8,
                  complete_tile_groups=len(rows), normal_palette_frames=len(normal),
                  full_gait_scale=True, hardware_pose_captures=True,
                  rom_sha256={p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                              for p in roms.iterdir() if p.is_file()}, target=str(target), output=str(output))
    (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(platform, hero, 'installed normal-input gait: PASS', len(rows), 'complete body groups')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', required=True, choices=['mvs', 'aes'])
    parser.add_argument('--hero', required=True, choices=['maiya', 'luna'])
    parser.add_argument('--workspace', required=True, type=Path)
    parser.add_argument('--target', type=Path, default=Path('/mnt/c/mame/neogeosdk'))
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    run(args.platform, args.hero, args.workspace.resolve(), args.target.resolve(), args.output.resolve())
