"""Run isolated AES/MVS gait and close-range collision inspections with fresh DWARF."""
import argparse
import csv
import hashlib
import json
import re
import shutil
import subprocess
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
WORK = GAME/'build/workspace'
TOOLCHAIN = ROOT.parent/'x-tools-v3/m68k-unknown-elf/bin'


def windows(path):
    return subprocess.check_output(['wslpath','-w',str(path.resolve())],text=True).strip()


def run(platform, scenario, work, output=None):
    output = output or GAME/'build/feedback-20261011/native'/platform/scenario
    output.mkdir(parents=True,exist_ok=True)
    expressions = {'base':'&mg','enemy_count':'sizeof(mg.enemies)/sizeof(MGEnemy)',
                   'shot_count':'sizeof(mg.shots)/sizeof(MGShot)'}
    fields = ('state player demo tick walk_distance gait gait_stop facing cast attack plant fire '
              'veil hurt shots run_hold crouch_timer lives next_stage state_timer stage airborne '
              'enemies on_ledge rising rise_wait dp_step dp_timer previous_joy combo_buffer '
              'combo_timer drop sitting player_prev_y').split()
    expressions.update({n:f'(unsigned long)&mg.{n}-(unsigned long)&mg' for n in fields})
    expressions.update({'fire_held':'(unsigned long)&mg.fire.held-(unsigned long)&mg',
                        'fire_wait':'(unsigned long)&mg.fire.wait-(unsigned long)&mg',
                        'fire_queued':'(unsigned long)&mg.fire.queued-(unsigned long)&mg',
                        'cam_x':'(unsigned long)&mg.camera.x-(unsigned long)&mg',
                        'cam_y':'(unsigned long)&mg.cam_y-(unsigned long)&mg',
                        'swift':'(unsigned long)&mg.swift-(unsigned long)&mg',
                        'shot_size':'sizeof(MGShot)','enemy_size':'sizeof(MGEnemy)'})
    for struct,prefix,names in (
        ('NGCharacter','char','x y x_fp y_fp vx_fp vy_fp sprite_tile sprite_offset_x sprite_offset_y scale_x scale_y visible hp max_hp body_y body_h active'),
        ('MGShot','shot','life hostile'),('MGEnemy','enemy','body type hurt mood')):
        for name in names.split():
            alias={'sprite_tile':'tile','sprite_offset_x':'ox','sprite_offset_y':'oy',
                   'scale_x':'sx','scale_y':'sy'}.get(name,name)
            expressions[f'{prefix}_{alias}']=f'&(({struct}*)0)->{name}'
    command=[str(TOOLCHAIN/'m68k-unknown-elf-gdb'),'-batch',str(work/'out/game')]
    for name,expr in expressions.items():
        command+=['-ex',f'printf "{name}=%lu\\n", (unsigned long)({expr})']
    result=subprocess.run(command,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,check=True)
    values={k:int(v) for k,v in re.findall(r'^(\w+)=(\d+)$',result.stdout,re.M)}
    if set(values)!=set(expressions):
        raise ValueError('Missing DWARF layout fields: '+str(set(expressions)-set(values))+'\n'+result.stdout)
    base=values.pop('base');enemy_count=values.pop('enemy_count');shot_count=values.pop('shot_count')
    config={'base':base,'offsets':values,'output':windows(output).replace('\\','/'),
            'enemy_count':enemy_count,'shot_count':shot_count}
    script='local qa='+('{base='+str(base)+',output='+json.dumps(config['output'])+
            ',enemy_count='+str(enemy_count)+',shot_count='+str(shot_count)+',offsets={'+
            ','.join(f'{k}={v}' for k,v in values.items())+'}}\n')
    script+=(GAME/'tools'/f'feedback_{scenario}.lua').read_text()
    (output/'capture.lua').write_text(script)
    (output/'layout.json').write_text(json.dumps(config,indent=2)+'\n')
    roms=output/'roms/maiya';roms.mkdir(parents=True,exist_ok=True)
    for p in (work/'roms/maiya').iterdir():
        if p.is_file():shutil.copy2(p,roms/p.name)
    hashdir=output/'hash';hashdir.mkdir(exist_ok=True)
    shutil.copy2(work/'hash_eagle/maiya/neogeo.xml',hashdir/'neogeo.xml')
    driver='aes' if platform=='aes' else 'neogeo'
    command=['/mnt/c/mame/mame.exe',driver,'-cart1','maiya','-bios','asia' if platform=='aes' else 'euro',
             '-noreadconfig','-skip_gameinfo','-video','none','-sound','none','-nothrottle','-nonvram_save',
             '-seconds_to_run','45','-rompath',windows(output/'roms')+';'+windows(ROOT/'bios/test_roms')+
             ';C:\\mame\\neogeosdk\\roms','-hashpath',windows(hashdir),'-cfg_directory',windows(output/'cfg'),
             '-autoboot_delay','0','-autoboot_script',windows(output/'capture.lua')]
    with (output/'mame.log').open('w') as log:
        subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=180)
    summary={'platform':platform,'scenario':scenario,'seeded_inspection':True,
             'rom_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in roms.iterdir()}}
    if scenario=='combat':
        rows=list(csv.DictReader((output/'combat.csv').open()))
        groups={n:[r for r in rows if r['phase']==n] for n in
                ['below','same','kneel_below','rise','left_edge','left_outside','right_edge','right_outside']}
        assert all(groups.values()),'Incomplete native scenario coverage'
        for n in ['below','kneel_below']:
            assert all(int(r['target_active'])==1 and int(r['target_pointer'])!=0
                       for r in groups[n]),n+': target was not a live encounter'
            assert all(int(r['enemy_hp'])==3 for r in groups[n]),n+': enemy was hit from the upper tier'
            assert any(int(r['cast']) for r in groups[n]),n+': no attempted attack observed'
        assert min(int(r['enemy_hp']) for r in groups['same'])<3,'Ground whip did not damage its target'
        assert any(int(r['rising']) for r in groups['rise']),'Rising Bloom did not trigger'
        assert min(int(r['enemy_hp']) for r in groups['rise'])<3,'Rising Bloom did not damage its target'
        for n in ['left_edge','right_edge']:
            settled=[r for r in groups[n] if 2<=int(r['age'])<=10]
            assert all(int(r['on_ledge'])==1 and int(r['y'])==128 for r in settled),n+': boot edge did not land'
        for n in ['left_outside','right_outside']:
            assert any(int(r['y'])>128 and int(r['on_ledge'])==0 for r in groups[n]),n+': support extended past its limit'
        summary['checks']=['high-platform standing/kneeling whip blocked','same-height whip damages',
                           'Rising Bloom triggers and damages','both drawn ledge edges catch the boot',
                           'beyond both four-pixel margins falls']
    else:
        rows=list(csv.DictReader((output/'state.csv').open()))
        header=(GAME/'artbox/generated/maiya_assets.h').read_text()
        body=re.search(r'mg_hero_tiles\[.*?\]\s*=\s*\{(.*?)\}',header,re.S).group(1)
        tiles=[int(x,0) for x in re.findall(r'0x[0-9a-fA-F]+|\d+',body)]
        gait=[r for r in rows if int(r['tile']) in tiles[60:76]]
        assert gait,'No gait observed'
        assert {int(r['tile']) for r in gait}==set(tiles[60:76]),'Not every accepted walk/run pose was observed'
        assert all(int(r['sx'])==255 and int(r['sy'])==255 for r in gait),'Gait hardware scale changed'
        # frame_done samples the CPU's new pose before its queued VBlank
        # upload. Hardware may still show the previous complete frame.
        # Facing left starts at column 4, not the frame's base tile.
        missing=[];latency=0
        for i,r in enumerate(rows):
            candidates=[r]+([rows[i-1]] if i else [])
            if (int(r['scb3'])&63)==0 or not any(
                int(r['scb1'])==int(c['tile'])+(4 if int(c['facing']) else 0)
                for c in candidates):
                missing.append(r)
            if int(r['scb1'])!=int(r['tile'])+(4 if int(r['facing']) else 0):latency+=1
        assert not missing,f'{len(missing)} frames with hero missing/stale in hardware VRAM'
        hardware=list(csv.DictReader((output/'tiles.csv').open()))
        assert len(hardware)==len(rows),'Hardware/CPU samples lost synchronization'
        for i,(r,h) in enumerate(zip(rows,hardware)):
            candidates=[r]+([rows[i-1]] if i else [])
            assert any(all(int(h[f'r{y}c{x}'])==int(c['tile'])+y*5+(4-x if int(c['facing']) else x)
                           for y in range(4) for x in range(5)) for c in candidates),\
                   f'Partially rewritten or unrelated hero tile group at sample {i}'
        for phase in ['fire_held','fire_held_left']:
            group=[r for r in rows if r['phase']==phase]
            assert any(int(r['cast']) or int(r['attack']) for r in group),phase+': no attack'
            assert any(abs(int(r['vx']))>0 and int(r['tile']) in tiles[60:76] for r in group),phase+': no steps between attacks'
        summary.update(gait_updates=len(gait),hardware_missing_frames=len(missing),
                       complete_hardware_frame_samples=len(hardware),queued_frame_samples=latency,
                       checks=['all 16 gait poses','full hardware scale','no blank/stale player strips',
                               'all 20 tiles form one complete current/queued frame',
                               'held fire steps and attacks in both directions'])
    summary['passed']=True
    (output/'result.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(platform,scenario,'PASS',summary.get('gait_updates',''),output)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform',choices=['aes','mvs'],required=True)
    parser.add_argument('--scenario',choices=['gait','combat'],required=True)
    parser.add_argument('--workspace',type=Path,default=WORK)
    parser.add_argument('--output',type=Path,help='Keep this inspection in its own evidence directory')
    args=parser.parse_args();run(args.platform,args.scenario,args.workspace.resolve(),
                               args.output.resolve() if args.output else None)
