"""Back up Shadow League's installed files, then install verified V2 sets only."""
import argparse
import datetime
import hashlib
import json
import re
import shutil
import tempfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def batch(path,text):
    path.write_bytes(text.replace('\n','\r\n').encode('ascii'))


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--target',type=Path,default=Path('/mnt/c/mame/neogeosdk'))
    ap.add_argument('--source',type=Path,help='Verified platform package root containing aes/ and mvs/')
    ap.add_argument('--revision',default='team-battle-and-controls')
    args=ap.parse_args();installed=args.target.resolve()
    if not re.fullmatch(r'[a-z0-9][a-z0-9-]{0,63}',args.revision):
        ap.error('--revision must use lowercase letters, digits and hyphens')
    source=(args.source or ROOT/'dist/neo/shadowleague').resolve()
    for platform in ('mvs','aes'):
        assert (source/platform/'hash/neogeo.xml').is_file()
        assert len(list((source/platform/'roms/shadowleague').glob('781-*')))==6
    (installed/"staging").mkdir(parents=True,exist_ok=True)
    target=Path(tempfile.mkdtemp(prefix=f"shadowleague-v2-{args.revision}-",dir=installed/"staging"))
    stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S')
    backup=installed/'backups/shadowleague'/(stamp+'-before-'+args.revision)
    assert not backup.exists()
    tests=target/'tests/shadowleague'
    for platform in ('mvs','aes'):
        shutil.copytree(source/platform,tests/platform)
        # Preserve control bindings and soft DIPs, while leaving save states behind.
        old=installed/'tests/shadowleague'/platform
        if old.exists():
            for p in old.iterdir():
                if p.is_dir() and (p.name.startswith('cfg-') or p.name.startswith('nvram-')):
                    shutil.copytree(p,tests/platform/p.name,dirs_exist_ok=True)
    bios=tests/'bios'
    for board,names in {'neogeo':('sp-s2.sp1','sm1.sm1','sfix.sfix','000-lo.lo'),
                        'aes':('neo-epo.bin','000-lo.lo')}.items():
        (bios/board).mkdir(parents=True,exist_ok=True)
        for name in names:shutil.copy2(ROOT/'bios'/name,bios/board/name)
    shutil.copytree(source/'mvs/roms/shadowleague',target/'roms/shadowleague')
    (target/'hash/shadowleague').mkdir(parents=True,exist_ok=True)
    shutil.copy2(source/'mvs/hash/neogeo.xml',target/'hash/shadowleague/neogeo.xml')
    (target/'dist/release').mkdir(parents=True,exist_ok=True)
    if args.source is None:
        for p in (ROOT/'dist/release').glob('ShadowLeague*'):
            if '_v2' in p.name:shutil.copy2(p,target/'dist/release'/p.name)
    shutil.copytree(source,target/'dist/neo/shadowleague')
    batch(target/'run_shadowleague.bat',r'''@echo off
setlocal
set "HERE=%~dp0"
set "MODE=%~1"
if "%MODE%"=="" set "MODE=mvs"
set "SYSTEM=neogeo"
set "BIOS=euro"
if /i "%MODE%"=="aes" set "SYSTEM=aes"
if /i "%MODE%"=="aes" set "BIOS=asia"
set "MAME_EXE=%HERE%..\mame.exe"
if not exist "%MAME_EXE%" set "MAME_EXE=mame"
set "SCAN="
if /i "%~2"=="scanlines" set "SCAN=-hlsl_enable -scanline_alpha 0.25"
set "DATA=%HERE%tests\shadowleague\%MODE%"
set "ROMS=%HERE%tests\shadowleague\bios;%DATA%\roms"
set "MAME_DISPLAY=-skip_gameinfo -nowindow -keepaspect -video d3d"
if exist "%HERE%mame_display.bat" call "%HERE%mame_display.bat"
"%MAME_EXE%" %SYSTEM% -noreadconfig %MAME_DISPLAY% -nofilter -prescale 3 -rompath "%ROMS%" -hashpath "%DATA%\hash" -bios %BIOS% -cart1 shadowleague -cfg_directory "%DATA%\cfg-eagle" -nvram_directory "%DATA%\nvram-eagle" -state_directory "%DATA%\state-v2-__REVISION__" -noautosave -noautoframeskip -frameskip 0 %SCAN%
if errorlevel 1 pause
exit /b %ERRORLEVEL%
'''.replace('__REVISION__',args.revision))
    for platform in ('mvs','aes'):
        batch(target/f'run_shadowleague_{platform}.bat',f'@echo off\ncall "%~dp0run_shadowleague.bat" {platform}\n')
        batch(target/f'run_shadowleague_{platform}_scanlines.bat',f'@echo off\ncall "%~dp0run_shadowleague.bat" {platform} scanlines\n')
    # Recording helpers keep the same data locations and now resolve the new sets.
    for p in installed.glob('*shadowleague*.bat'):
        if not (target/p.name).exists():shutil.copy2(p,target/p.name)
    info={'version':'v2','revision':args.revision,'source':str(source),'installed_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
          'backup':str(backup.relative_to(installed)),'files':{}}
    for platform in ('mvs','aes'):
        for p in (tests/platform/'roms/shadowleague').glob('781-*'):
            original=source/platform/'roms/shadowleague'/p.name
            assert digest(original)==digest(p)
            info['files'][str(p.relative_to(target))]={'size':p.stat().st_size,'sha256':digest(p)}
    (target/'shadowleague-build.json').write_text(json.dumps(info,indent=2)+'\n')
    (target/'latest-shadowleague-backup.txt').write_text(str(backup.relative_to(installed))+'\n')
    (target/'SHADOWLEAGUE-TEST.txt').write_text(
        'SHADOW LEAGUE FIGHTING V2 - '+args.revision+'\n\n'
        'Launch run_shadowleague_mvs.bat or run_shadowleague_aes.bat.\n'
        'Modes: 1 vs 1, or 3 vs 3 with striker.\n'
        'Original showcase sprites: Kenji Arata, Jack Mercer, Amara Silva.\n'
        'The remaining roster retains its previous artwork. Arenas are preserved.\n'
        'Native display: 320 x 224. This is a local review candidate.\n'
        'The previous installation and launchers are in '+str(backup.relative_to(installed))+'\n')
    # All bytes have been staged and verified before the old install is moved.
    owned=[installed/'tests/shadowleague',installed/'roms/shadowleague',installed/'hash/shadowleague',
           installed/'dist/neo/shadowleague',installed/'shadowleague-build.json',installed/'latest-shadowleague-backup.txt',
           installed/'SHADOWLEAGUE-TEST.txt']
    owned+=list((installed/'dist/release').glob('ShadowLeague*'))
    owned+=list(installed.glob('*shadowleague*.bat'))
    fresh=[target/'tests/shadowleague',target/'roms/shadowleague',target/'hash/shadowleague',
           target/'dist/neo/shadowleague',target/'shadowleague-build.json',target/'latest-shadowleague-backup.txt',
           target/'SHADOWLEAGUE-TEST.txt']
    fresh+=list((target/'dist/release').glob('ShadowLeague*'))+list(target.glob('*shadowleague*.bat'))
    backup.mkdir(parents=True);moved=[];published=[]
    try:
        for old in owned:
            if old.exists():
                rel=old.relative_to(installed);dest=backup/rel;dest.parent.mkdir(parents=True,exist_ok=True)
                shutil.move(str(old),str(dest));moved.append(str(rel))
        for src in fresh:
            dest=installed/src.relative_to(target);dest.parent.mkdir(parents=True,exist_ok=True)
            shutil.move(str(src),str(dest));published.append(dest)
        for name,record in info['files'].items():assert digest(installed/name)==record['sha256']
    except Exception:
        # Put new files back in staging before restoring the previous installation.
        for dest in reversed(published):
            staging=target/dest.relative_to(installed);staging.parent.mkdir(parents=True,exist_ok=True)
            shutil.move(str(dest),str(staging))
        for name in reversed(moved):
            dest=installed/name;dest.parent.mkdir(parents=True,exist_ok=True)
            shutil.move(str(backup/name),str(dest))
        raise
    shutil.rmtree(target)
    (backup/'MOVED.json').write_text(json.dumps(moved,indent=2)+'\n')
    print(json.dumps({'installed':str(installed),'backup':str(backup),'roms_verified':len(info['files'])},indent=2))


if __name__=='__main__':main()
