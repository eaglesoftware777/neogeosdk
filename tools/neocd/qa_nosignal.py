"""Exercise the native No Signal CD with real firmware and controller inputs."""
import argparse
import array
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import wave

from tools.neocd.qa import firmware_bytes
from tools.neocd.audit_disc import audit_cue, read_iso
from tools.neocd.build import default_compiler


def run(build, out, bios, lo, profile, hero, mame, cc, media=None, startup=None):
    build, out = Path(build).resolve(), Path(out).resolve()
    metadata = json.loads((build/'source-manifest.json').read_text())
    media_audit = bool(metadata.get('media_audit'))
    out.mkdir(parents=True, exist_ok=False)
    firmware = firmware_bytes(Path(bios), profile)
    roms = out/'roms/neocdz'; roms.mkdir(parents=True)
    swapped = bytearray(firmware)
    swapped[0::2], swapped[1::2] = firmware[1::2], firmware[0::2]
    (roms/'neocd.bin').write_bytes(swapped)
    shutil.copyfile(lo, roms/'000-lo.lo')
    suffix = '.exe' if cc.endswith('.exe') else ''
    prefix = cc[:-len('gcc'+suffix)]; elf = build/'program.elf'
    text = subprocess.check_output([prefix+'nm'+suffix,'-n',str(elf)], text=True)
    symbols = {n:int(v,16) for v,n in re.findall(r'^([0-9a-fA-F]+)\s+\w\s+(\w+)$', text, re.M)}
    fields = {'tick':'s_b.tick','px':'s_b.f[0].x','py':'s_b.f[0].y','pz':'s_b.f[0].z',
              'hp':'s_b.f[0].hp','alive':'s_b.f[0].alive','state':'s_b.f[0].state'}
    if media_audit:
        fields = {}
    else:
        cmd = [prefix+'gdb'+suffix,'-nx','-batch',str(elf)]
        for name, expr in fields.items():
            cmd += ['-ex',f'printf "{name}=%lu:%u\\n", (unsigned long)&{expr}, (unsigned int)sizeof({expr})']
        data = subprocess.check_output(cmd, text=True)
        fields = {k:{'addr':int(a),'size':int(s)} for k,a,s in re.findall(r'^(\w+)=(\d+):(\d+)$',data,re.M)}
    program = read_iso((build/'disc/nosignal.iso').read_bytes())['NS.PRG']
    q = dict(out=str(out), syms=symbols, fields=fields, profile=profile,
             hero=int(hero), program_bytes=len(program), media_audit=media_audit,
             resident_bytes=metadata['graphics']['resident_tiles']*128,
             hero_bank=3, hero_offset=(metadata['graphics'].get('hero_base', 0x6000)-0x6000)*128,
             hero_bytes=max((h['size'] for h in metadata['graphics'].get('heroes', [])), default=0))
    def lua(v):
        if isinstance(v,dict):return '{'+','.join('['+json.dumps(k)+']='+lua(x) for k,x in v.items())+'}'
        if isinstance(v,bool):return 'true' if v else 'false'
        return json.dumps(v)
    script = out/'probe.lua'
    script.write_text('local q='+lua(q)+'\n'+Path(__file__).with_suffix('.lua').read_text()+
                      ('\n'+Path(startup).read_text() if startup else ''))
    cmd = [mame,'neocdz','-noreadconfig','-skip_gameinfo','-rompath',str(out/'roms'),
           '-cdrom',str(media or build/'disc/nosignal.cue'),'-video','none','-sound','none',
           '-nothrottle','-wavwrite',str(out/'audio.wav'),'-seconds_to_run','220',
           '-autoboot_delay','0','-autoboot_script',str(script),
           '-cfg_directory',str(out/'cfg'),'-nvram_directory',str(out/'nvram')]
    env = os.environ.copy()
    for k in ('DISPLAY','WAYLAND_DISPLAY','PULSE_SERVER'):env.pop(k,None)
    env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',QT_QPA_PLATFORM='offscreen')
    with (out/'mame.log').open('w') as log:
        subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT, env=env, check=True, timeout=360)
    assert '[LUA ERROR]' not in (out/'mame.log').read_text(), 'Emulator control script failed'
    rows = list(csv.DictReader((out/'trace.csv').open()))
    assert (out/'program.bin').read_bytes()==program[0x100:], 'Loaded program differs'
    assert (out/'firmware.bin').read_bytes()==firmware, 'Mapped firmware differs'
    assert max(int(r['errors']) for r in rows)==0, 'Background readback failed'
    if media_audit:
        assert max(int(r['loads']) for r in rows)==13, 'Not every scenery transfer completed'
        loaded=b''.join((out/f'resident{bank}.bin').read_bytes() for bank in range(4))
        expected=b''.join((build/'payload'/f'SPR{bank}.SPR').read_bytes() for bank in range(4))
        end=q['resident_bytes']
        assert loaded[0x8000:end]==expected[0x8000:end], 'Scenery loading changed resident fighter graphics'
        heroes = metadata['graphics'].get('heroes', [])
        if heroes:
            assert max(int(r['heroloads']) for r in rows)==len(heroes), 'Not every hero slot transfer completed'
            last = max(heroes, key=lambda h: h['id'])
            assert (out/'heroslot.bin').read_bytes()[:last['size']]==(build/'payload'/last['name']).read_bytes(), 'Hero slot differs'
        report = dict(passed=True, scope='separate scenery-transfer diagnostic', profile=profile,
                      backgrounds_verified=13, background_crc_errors=0,
                      hero_slots_verified=len(metadata['graphics'].get('heroes', [])),
                      resident_sprite_bytes_verified=end-0x8000, hardware_tested=False)
        (out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps(report))
        return report
    active = [r for r in rows if int(r['tick'])>0 and int(r['alive'])]
    assert active and len(active)>60, 'No sustained gameplay'
    assert (out/'program.bin').read_bytes()==program[0x100:], 'Loaded program differs'
    assert (out/'firmware.bin').read_bytes()==firmware, 'Mapped firmware differs'
    assert int(rows[-1]['pad'])&0x1FC==0x1FC, 'Start/buttons/directions missing'
    assert int(rows[-1]['sfx'])>=5, 'No meaningful SFX playback'
    assert {int(r['hero']) for r in active}=={int(hero)}, 'Wrong hero selected'
    assert len({(r['px'],r['py']) for r in active})>20, 'Player did not move'
    assert max(int(r['pz']) for r in active)>0, 'Jump did not execute'
    assert max(int(r['errors']) for r in rows)==0, 'Background readback failed'
    assert max(int(r['loads']) for r in rows)>=3, 'Scene overlays not exercised'
    for h in metadata['graphics'].get('heroes', []):
        if h['id']==int(hero):
            assert int(rows[-1]['heroslot'])==int(hero)+1, 'Played hero not in the hero slot'
            assert (out/'heroslot.bin').read_bytes()[:h['size']]==(build/'payload'/h['name']).read_bytes(), 'Hero slot differs'
    with wave.open(str(out/'audio.wav'),'rb') as w:
        samples = array.array('h',w.readframes(w.getnframes()))
    peak = max(map(abs,samples),default=0)
    assert peak>512, 'Captured audio is silent'
    music = sorted({int(r['music']) for r in rows})
    assert any(t>=2 for t in music), 'CDDA music not requested'
    report = dict(passed=True, profile=profile, hero=int(hero), hardware_tested=False,
                  source_game_version=metadata.get('version', 2), gameplay_samples=len(active), audio_peak=peak,
                  music_tracks=music, background_loads=max(int(r['loads']) for r in rows),
                  media_format='CHD' if media and Path(media).suffix.lower()=='.chd' else 'CUE',
                  media_sha256=hashlib.sha256(Path(media or build/'disc/nosignal.cue').read_bytes()).hexdigest(),
                  background_crc_errors=0, iso_sha256=hashlib.sha256((build/'disc/nosignal.iso').read_bytes()).hexdigest(),
                  firmware_sha256=hashlib.sha256(firmware).hexdigest(), disc=audit_cue(build/'disc/nosignal.cue'))
    (out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='disc'}))
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('build','out','bios','lo'):p.add_argument('--'+flag,type=Path,required=True)
    p.add_argument('--profile',choices=('stock','eagle'),required=True)
    p.add_argument('--hero',type=int,choices=(0,1,2),default=0)
    p.add_argument('--mame',default='/usr/games/mame');p.add_argument('--cc',default=default_compiler())
    p.add_argument('--media',type=Path)
    p.add_argument('--startup-script',type=Path)
    a=p.parse_args();run(a.build,a.out,a.bios,a.lo,a.profile,a.hero,a.mame,a.cc,a.media,a.startup_script)


if __name__=='__main__':main()
