"""Boot an unchanged V2 CD game with real firmware and ordinary controller input."""
import argparse,array,csv,hashlib,json,os,re,shutil,subprocess,wave
from pathlib import Path
from tools.neocd.qa import firmware_bytes
from tools.neocd.audit_disc import audit_cue,read_iso
from tools.neocd.build import default_compiler


def run(build,out,bios,lo,profile,journey,mame,cc,resume_from=None,media=None):
    build,out=Path(build).resolve(),Path(out).resolve();out.mkdir(parents=True,exist_ok=False)
    if resume_from:
        save=out/'nvram/neocdz';save.mkdir(parents=True);shutil.copyfile(resume_from,save/'saveram')
    bios=firmware_bytes(Path(bios),profile)
    roms=out/'roms/neocdz';roms.mkdir(parents=True)
    swapped=bytearray(bios);swapped[0::2],swapped[1::2]=bios[1::2],bios[0::2]
    (roms/'neocd.bin').write_bytes(swapped);shutil.copyfile(lo,roms/'000-lo.lo')
    prefix=cc[:-3];elf=build/'program.elf'
    text=subprocess.check_output([prefix+'nm','-n',str(elf)],text=True)
    symbols={n:int(v,16) for v,n in re.findall(r'^([0-9a-fA-F]+)\s+\w\s+(\w+)$',text,re.M)}
    fields={'room':'s_g.room','nparty':'s_g.nparty','px':'s_g.party[0].x','py':'s_g.party[0].y','hp':'s_g.party[0].hp'}
    cmd=[prefix+'gdb','-nx','-batch',str(elf)]
    for name,expr in fields.items():cmd+=['-ex',f'printf "{name}=%lu\\n", (unsigned long)&{expr}']
    symbols.update({k:int(v) for k,v in re.findall(r'^(\w+)=(\d+)$',subprocess.check_output(cmd,text=True),re.M)})
    program=read_iso((build/'disc/maiyaluna.iso').read_bytes())['ML.PRG']
    q={'out':str(out),'syms':symbols,'profile':profile,'journey':int(journey=='luna'),'program_bytes':len(program),'resume':bool(resume_from)}
    def lua(v):
        if isinstance(v,dict):return '{'+','.join('['+json.dumps(k)+']='+lua(x) for k,x in v.items())+'}'
        if isinstance(v,bool):return 'true' if v else 'false'
        return json.dumps(v)
    script=out/'probe.lua';script.write_text('local q='+lua(q)+'\n'+Path(__file__).with_name('qa_maiyaluna.lua').read_text())
    cmd=[mame,'neocdz','-noreadconfig','-skip_gameinfo','-rompath',str(out/'roms'),'-cdrom',str(media or build/'disc/maiyaluna.cue'),'-video','none','-sound','none','-nothrottle','-wavwrite',str(out/'audio.wav'),'-seconds_to_run','180','-autoboot_delay','0','-autoboot_script',str(script),'-cfg_directory',str(out/'cfg'),'-nvram_directory',str(out/'nvram')]
    env=os.environ.copy();env.pop('DISPLAY',None);env.pop('WAYLAND_DISPLAY',None);env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',QT_QPA_PLATFORM='offscreen')
    with (out/'mame.log').open('w') as log:subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,env=env,check=True,timeout=300)
    rows=list(csv.DictReader((out/'trace.csv').open()));assert rows and int(rows[-1]['clock'])>=4200,'Runtime did not finish'
    assert (out/'program.bin').read_bytes()==program[0x100:],'Loaded program differs'
    assert (out/'firmware.bin').read_bytes()==bios,'Mapped firmware differs'
    assert int(rows[-1]['pad'])&0x138==0x138,'Start/A/Left/Right input missing'
    assert int(rows[-1]['sfx'])>=5,'No meaningful SFX playback'
    assert int(rows[-1]['journey'])==q['journey'],'Wrong sister selected'
    active=[r for r in rows if int(r['clock'])>2200 and int(r['nparty'])>0]
    assert len({(r['px'],r['py']) for r in active})>5,'No actual exploration movement'
    if resume_from:
        previous=Path(resume_from).read_bytes();at=previous.index(b'SAVE\x07\x83\x02')+12
        expected_x=(previous[at+9]*16+8)*256
        expected_y=(previous[at+10]*16+8)*256
        assert any(500<int(r['clock'])<2200 and int(r['px'])==expected_x and int(r['py'])==expected_y for r in rows),'Cold boot did not restore the saved position'
    with wave.open(str(out/'audio.wav'),'rb') as w:samples=array.array('h',w.readframes(w.getnframes()))
    peak=max(map(abs,samples),default=0);assert peak>512,'Audio silent'
    checkpoint=(out/'checkpoint.bin').read_bytes()
    assert b'SAVE\x07\x83\x02' in checkpoint,'No V2 notebook checkpoint'
    persistent=False
    if profile=='stock':
        saved=(out/'nvram/neocdz/saveram').read_bytes()
        at=saved.find(b'SAVE\x07\x83\x02')
        assert at>=0 and b'MAIYA AND LUNA' in saved,'No persistent CD notebook save'
        assert saved[at+12+322]==0x80+q['journey'],'Saved journey differs'
        persistent=True
    report={'persistent_save_verified':persistent,'cold_reload':bool(resume_from),'passed':True,'profile':profile,'journey':journey,'runtime_vblanks':int(rows[-1]['clock']),'audio_peak':peak,'hardware_tested':False,'source_game_version':2,'iso_sha256':hashlib.sha256((build/'disc/maiyaluna.iso').read_bytes()).hexdigest(),'disc':audit_cue(build/'disc/maiyaluna.cue')}
    (out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='disc'}))


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build',required=True,type=Path);p.add_argument('--out',required=True,type=Path);p.add_argument('--bios',required=True,type=Path);p.add_argument('--lo',required=True,type=Path);p.add_argument('--profile',choices=('stock','eagle'),required=True);p.add_argument('--journey',choices=('maiya','luna'),default='maiya');p.add_argument('--mame',default='/usr/games/mame');p.add_argument('--cc',default=default_compiler());p.add_argument('--resume-from',type=Path);p.add_argument('--media',type=Path);a=p.parse_args();run(a.build,a.out,a.bios,a.lo,a.profile,a.journey,a.mame,a.cc,a.resume_from,a.media)


if __name__=='__main__':main()
