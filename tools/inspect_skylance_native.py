#!/usr/bin/env python3
"""Inspect staged AES/MVS cartridges, actual SCB priority and scanline occupancy."""
import argparse,csv,hashlib,json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
STAGE=Path('/mnt/c/mame/neogeosdk/staging/skylance-native-stage-one-20261010')
def wp(p):return subprocess.check_output(['wslpath','-w',str(p)],text=True).strip()
def run(platform,mode,duration,stage=0,pilot=0):
    source=STAGE/'tests/skylance'/platform
    out=STAGE/'qa'/((f'{platform}-stage-one-{mode}' if not stage else f'{platform}-stage-{stage+1}-{mode}')+(f'-pilot{pilot+1}' if pilot else ''));out.mkdir(parents=True,exist_ok=True);(out/'cfg').mkdir(exist_ok=True)
    symbols={name:int(address,16) for address,kind,name in re.findall(r'^([0-9a-f]+) ([a-zA-Z]) (\w+)$',(source/'symbols.txt').read_text(),re.M)}
    names='s_player s_stage s_energy s_invuln s_plane_pose s_camera s_pick s_resources s_frames sound_queue_head sound_queue_tail s_waves_left s_squad_left s_wave_timer s_lives ng_chars ngsg_high_n ngsg_norm_n ngsg_low_n ngsg_high ngsg_norm ngsg_low s_roll s_roll_maneuver s_loop_maneuver s_boss_health s_boss_hp_max s_targets_destroyed s_ground_page s_hud_face s_encounter s_radar_alert s_chaff_stock s_decoyed_missiles s_sortie_frames s_depth_strip_peak s_depth_shadow_drops'.split()
    fields={n:symbols[n] for n in names}
    config='local config={output='+json.dumps(wp(out).replace('\\','/'))+',symbols={'+','.join(f'{k}={v}' for k,v in fields.items())+'},pilot='+str(pilot)+',stage='+str(stage)+',layout={108,64,0,1,4,6,8,10,14,32,2,5,74,72,78,70},'+({'features':'features=true','quiet':'encounters=true','boss':'boss=true,features=false','radar':'encounters=true,radar_test=true','turrets':'boss=true,features=false,silence=true'}[mode])+'}\n'
    (out/'capture.lua').write_text(config+(ROOT/'games/skylance/tools/mame/native_inspect.lua').read_text())
    cmd=['/mnt/c/mame/mame.exe','aes' if platform=='aes' else 'neogeo','-cart1','skylance','-bios','asia' if platform=='aes' else 'euro','-noreadconfig','-skip_gameinfo','-video','none','-sound','none','-nothrottle','-nonvram_save','-seconds_to_run',str(duration),'-rompath',wp(source/'roms')+';'+wp(STAGE/'tests/skylance/bios')+';C:\\mame\\neogeosdk\\roms','-hashpath',wp(source/'hash'),'-cfg_directory',wp(out/'cfg'),'-autoboot_delay','0','-autoboot_script',wp(out/'capture.lua')]
    with(out/'mame.log').open('w')as log:subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
    rows=list(csv.DictReader((out/'state.csv').open()));depth=list(csv.DictReader((out/'depth.csv').open()));live=[r for r in rows if int(r['player'],16)!=0 and int(r['user_mode'])==2]
    live_times={r['seconds']for r in live};d=[r for r in depth if r['seconds']in live_times]
    if not live or not d:raise ValueError('No playable native state observed')
    peak=max(int(r['raw_scb_peak'])for r in d);duplicates=max(int(r['vram_duplicates'])for r in live)
    if peak>96:raise ValueError(f'Actual SCB scanline demand exceeded 96: {peak}')
    if duplicates:raise ValueError(f'Duplicate deferred VRAM groups: {duplicates}')
    if max(abs(float(r['camera_x']))for r in d)>12.001:raise ValueError('Camera escaped its bound')
    road=[r for r in d if int(r['road_x'])!=-999]
    # Each car against the ground page under it (one page may be mid-recycle above the field).
    if any(abs(int(r['road_x'])-(92+int(r['under_x'])))>1 for r in road):raise ValueError('Road traffic lost its terrain anchor')
    cloud=[r for r in d if int(r['cloud_slot'])>=96]
    if any(int(r['player_slot'])>=0 and int(r['player_slot'])>=int(r['cloud_slot'])for r in cloud):raise ValueError('Foreground cloud did not sort above player')
    if any(int(r['min_bullet_slot'])!=999 and int(r['min_bullet_slot'])<=int(r['cloud_slot'])for r in cloud):raise ValueError('Dangerous bolts sorted below cloud')
    video=game=0;missed=0
    for a,b in zip(d,d[1:]):
        if int(b['sortie_frame'])>int(a['sortie_frame']):
            dv=int(b['video_frame'])-int(a['video_frame']);dg=(int(b['sortie_frame'])-int(a['sortie_frame']))&65535
            video+=dv;game+=dg;missed+=max(0,dv-dg)
    result={'status':'passed','platform':platform,'inspection_mode':mode,'duration_seconds':duration,'actual_scb_peak':peak,'deferred_duplicates':duplicates,'camera_x_max':max(abs(float(r['camera_x']))for r in d),'shadow_samples':sum(int(r['shadow_groups'])>0 for r in d),'foreground_cloud_samples':len(cloud),'road_anchor_samples':len(road),'modules_max':max(int(r['modules'])for r in d),'wreck_samples':sum(int(r['wrecks'])>0 for r in d),'mission_targets':max(int(r['targets'])for r in live),'terrain_page_max':max(int(r['terrain_page'])for r in live),'simulation_updates':game,'video_intervals':video,'video_intervals_without_simulation_update':missed,'simulation_update_ratio':game/video if video else None,'inspection_seeds':'Invulnerability; features/quiet seed power 4 and extra cores; boss bypasses director/route for targeted inspection','rom_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest()for p in(source/'roms/skylance').iterdir()},'bios_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest()for p in(STAGE/'tests/skylance/bios').iterdir()if p.is_file()}}
    if mode=='turrets':
        shots=list(csv.DictReader((out/'turrets.csv').open()))
        sides={phase:{(int(r['dx'])>8)-(int(r['dx'])<-8) for r in shots if r['phase']==str(phase)} for phase in range(3)}
        # Both turrets fire, then only the right one, then the core alone.
        if sides!={0:{-1,1},1:{1},2:{0}}:raise ValueError(f'Turret fire does not follow the surviving turrets: {sides}')
        result['turret_fire_sides']={str(k):sorted(v) for k,v in sides.items()};result['turret_shots']=len(shots)
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if 'sha256'not in k},indent=2));return result
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--platform',choices=('aes','mvs'),required=True);p.add_argument('--mode',choices=('features','quiet','boss','radar','turrets'),default='features');p.add_argument('--duration',type=int,default=65);p.add_argument('--stage',type=int,default=0,help='0-based stage (0 = Stage 1)');p.add_argument('--pilot',type=int,default=0,choices=(0,1,2));a=p.parse_args();run(a.platform,a.mode,a.duration,a.stage,a.pilot)
