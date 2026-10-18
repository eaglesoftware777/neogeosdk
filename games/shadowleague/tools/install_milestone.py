import argparse,datetime,hashlib,json,os,shutil
ap=argparse.ArgumentParser(description='Install only Shadow League game files; verify checksums and create no backups.')
ap.add_argument('--milestone',default='reviewed-candidate')
ap.add_argument('--version',default='1.7.7-WIP')
args=ap.parse_args()
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[3]/'dist/candidates/shadowleague/sdk177'
TARGET=Path('/mnt/c/mame/neogeosdk')
files={}
def place(src,dst):
 dst.parent.mkdir(parents=True,exist_ok=True)
 temp=dst.with_name(dst.name+'.sdk177-new')
 if temp.exists():temp.unlink()
 if src.suffix=='.neo':shutil.copy2(src,temp)
 else:
  try:os.link(src,temp)
  except OSError:shutil.copy2(src,temp)
 if hashlib.sha256(src.read_bytes()).digest()!=hashlib.sha256(temp.read_bytes()).digest():raise RuntimeError('Install checksum mismatch')
 os.replace(temp,dst)
 files[str(dst.relative_to(TARGET))]={'size':dst.stat().st_size,'sha256':hashlib.sha256(dst.read_bytes()).hexdigest()}
for platform in ['aes','mvs']:
 for folder in ['roms','hash']:
  for src in (SOURCE/platform/folder).rglob('*'):
   if src.is_file():place(src,TARGET/'tests/shadowleague/sdk177'/platform/folder/src.relative_to(SOURCE/platform/folder))
 place(SOURCE/f'shadowleague-{platform}.neo',TARGET/f'dist/neo/shadowleague/shadowleague-{platform}.neo')
for src in (SOURCE/'mvs/roms/shadowleague').iterdir():
 if src.is_file():place(src,TARGET/'roms/shadowleague'/src.name)
for src in (SOURCE/'mvs/hash').iterdir():
 if src.is_file():place(src,TARGET/'hash/shadowleague'/src.name)
launcher='''@echo off
setlocal
cd /d "%~dp0"
set "SLF_PLATFORM=%~1"
if /I not "%SLF_PLATFORM%"=="aes" set "SLF_PLATFORM=mvs"
set "SLF_SYSTEM=neogeo"
set "SLF_BIOS=euro"
if /I "%SLF_PLATFORM%"=="aes" set "SLF_SYSTEM=aes"
if /I "%SLF_PLATFORM%"=="aes" set "SLF_BIOS=asia"
"%~dp0..\\mame.exe" %SLF_SYSTEM% -noreadconfig -cart1 shadowleague -rompath "%~dp0tests\\shadowleague\\sdk177\\%SLF_PLATFORM%\\roms;%~dp0tests\\shadowleague\\bios" -hashpath "%~dp0tests\\shadowleague\\sdk177\\%SLF_PLATFORM%\\hash" -bios %SLF_BIOS% -nofilter -keepaspect -resolution 1280x960 -skip_gameinfo -noautosave -noautoframeskip -frameskip 0 -nvram_directory "%~dp0tests\\shadowleague\\sdk177\\%SLF_PLATFORM%\\nvram" -cfg_directory "%~dp0tests\\shadowleague\\sdk177\\%SLF_PLATFORM%\\cfg" %2 %3 %4 %5 %6 %7 %8 %9
endlocal
'''
(TARGET/'run_shadowleague.bat').write_bytes(launcher.replace('\n','\r\n').encode('ascii'))
(TARGET/'shadowleague-build.json').write_text(json.dumps({'version':args.version,'milestone':args.milestone,'source':str(SOURCE),'installed_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'backup':None,'files':files},indent=2)+'\n')
print('Installed Shadow League '+args.milestone+':',len(files),'verified files; no backups; other games unchanged')

for platform in ['aes','mvs']:
    text='@echo off\n'+'title Shadow League - '+args.milestone+' - '+args.version+'\n'+f'call "%~dp0run_shadowleague.bat" {platform}\n'
    (TARGET/f'START_SHADOW_LEAGUE_MILESTONE_{platform.upper()}.bat').write_bytes(text.replace('\n','\r\n').encode('ascii'))
(TARGET/'START_SHADOW_LEAGUE_MILESTONE.bat').write_bytes(('@echo off\ncall "%~dp0START_SHADOW_LEAGUE_MILESTONE_AES.bat"\n').replace('\n','\r\n').encode('ascii'))
if args.milestone == 'v1-hardware-test':
    (TARGET/'START_SHADOW_LEAGUE_V1.bat').write_bytes(('@echo off\ncall "%~dp0START_SHADOW_LEAGUE_MILESTONE_AES.bat"\n').replace('\n','\r\n').encode('ascii'))
