"""Package a tested native No Signal CD without distributing system firmware."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import zipfile

from tools.neocd.audit_disc import audit_cue
from tools.neocd.build import default_compiler
from tools.neocd.nosignal import verify_v2

ROOT=Path(__file__).resolve().parents[2]


def package(build,out,qa,cc):
    build,out,qa=Path(build).resolve(),Path(out).resolve(),Path(qa).resolve()
    meta=json.loads((build/'source-manifest.json').read_text())
    if meta.get('game')!='nosignal' or meta.get('media_audit'):
        raise ValueError('Only the playable native No Signal CD may be released')
    verify_v2()
    for name,expected in meta['source_sha256'].items():
        if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=expected:
            raise ValueError('Source changed after the tested CD build: '+name)
    source=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    if subprocess.run(['git','cat-file','-e',source+':tools/neocd/nosignal.py'],cwd=ROOT,capture_output=True).returncode:
        raise ValueError('Commit the native port source before packaging it')
    reports=[]
    for profile in ('stock','eagle'):
        for hero in range(3):
            report=json.loads((qa/f'qa-release-{profile}-{hero}/result.json').read_text())
            if not report.get('passed') or report['iso_sha256']!=hashlib.sha256((build/'disc/nosignal.iso').read_bytes()).hexdigest():
                raise ValueError('Missing or mismatched gameplay QA')
            reports.append({k:v for k,v in report.items() if k!='disc'})
        tour=json.loads((qa/f'qa-resident-{profile}/result.json').read_text())
        if not tour.get('passed') or tour.get('backgrounds_verified')!=13 or not tour.get('resident_sprite_bytes_verified'):
            raise ValueError('Missing resident/scenery readback QA')
        compressed=json.loads((qa/f'qa-chd-final-{profile}/result.json').read_text())
        if (not compressed.get('passed') or compressed.get('media_format')!='CHD' or
                compressed.get('media_sha256')!=hashlib.sha256((build/'nosignal.chd').read_bytes()).hexdigest()):
            raise ValueError('Missing or mismatched CHD boot QA')
    audit_cue(build/'disc/nosignal.cue')
    out.mkdir(parents=True,exist_ok=True)
    dest=out/'NoSignal-NeoGeoCD_v2.zip'
    if dest.exists():raise ValueError('Existing release stays unchanged')
    stage=out/'package';stage.mkdir(exist_ok=False)
    (stage/'disc').mkdir()
    shutil.copy2(build/'nosignal.chd',stage/'disc/nosignal.chd')
    suffix='.exe' if cc.endswith('.exe') else '';prefix=cc[:-len('gcc'+suffix)]
    symbols=subprocess.check_output([prefix+'nm'+suffix,'-n',str(build/'program.elf')],text=True)
    address=int(re.search(r'^([0-9a-fA-F]+)\s+\w\s+ng_cd_frames$',symbols,re.M)[1],16)
    (stage/'boot.lua').write_text((ROOT/'games/nosignal/cd/boot.lua.in').read_text().replace('@FRAMES_ADDR@',str(address)))
    shutil.copy2(ROOT/'LICENSE',stage/'LICENSE')
    shutil.copy2(ROOT/'games/nosignal/sound/CREDITS.txt',stage/'SOUND-CREDITS.txt')
    readme='''No Signal Z Alpha V2 - Native Neo Geo CD

Original single-player beat-'em-up: Jay, Zoe or Kai, eight stages, console
menus and the existing V2 moves, graphics, effects, voices and music.

This package contains the complete disc/nosignal.chd, including the data
track and all 18 audio tracks. Select it directly in MAME. To produce a
burnable mixed-mode CUE/BIN image, use extract_disc.bat or run:
  chdman extractcd -i disc/nosignal.chd -o extracted/nosignal.cue
Create the extracted directory first. Burn the resulting CUE with a suitable
mixed-mode CD writer. Physical hardware has not been tested. The source
build also produces the full ISO/CUE/WAV set.

Windows MAME:
1. Supply your own legitimate system ROM and LO data. They are not bundled.
2. Put the stock CDZ neocd.bin and 000-lo.lo in roms-stock/neocdz/.
   For EagleBIOS-CDZ, use the equivalent files in roms-eagle/neocdz/.
   EagleBIOS CD firmware is distributed separately with SDK 1.7.7.
3. Set MAME_EXE to your MAME executable, or place MAME on PATH.
4. Use run_stock.bat or run_eagle.bat. Configurations and saves are separate.
   The drive-menu helper releases forced controls before the game starts.

Controls: stick moves, double-tap runs; A attacks, Down+A sweeps;
B jumps (A airborne kicks); C grabs/throws; D uses the special.
Walk over pickups or onto the scooter; B leaves the scooter. START pauses.

Native MAME CDZ testing passed for all three heroes on both BIOS profiles,
including controller actions, gameplay, effects, music, exact program and
firmware readback, all 13 scenery transfers and resident sprite integrity.
CHD integrity and boot were checked. This is emulator evidence, not physical
hardware certification. Front/top models, full campaigns and power-cycle
save persistence remain unverified. Session progress/unlocks are retained.

Source and build instructions: games/nosignal/cd/README.md in NeoGeoSDK.
'''
    readme+='\nSource commit: '+source+'\nhttps://github.com/eaglesoftware777/neogeosdk/commit/'+source+'\n'
    (stage/'README.txt').write_text(readme)
    for profile in ('stock','eagle'):
        (stage/f'roms-{profile}/neocdz').mkdir(parents=True)
        (stage/f'roms-{profile}/neocdz/PLACE-YOUR-OWN-ROM-FILES.txt').write_text('Supply neocd.bin and 000-lo.lo for this BIOS profile. No system ROM is bundled.\n')
        bat=r'''@echo off
setlocal
set "HERE=%~dp0"
if not defined MAME_EXE set "MAME_EXE=mame"
if not exist "%HERE%roms-PROFILE\neocdz\neocd.bin" (
 echo Supply your own PROFILE CDZ firmware in roms-PROFILE\neocdz.
 pause
 exit /b 1
)
if not exist "%HERE%roms-PROFILE\neocdz\000-lo.lo" (
 echo Supply the required LO data in roms-PROFILE\neocdz.
 pause
 exit /b 1
)
for %%D in (cfg nvram state) do if not exist "%HERE%state\PROFILE\%%D" mkdir "%HERE%state\PROFILE\%%D"
"%MAME_EXE%" neocdz -bios official -noreadconfig %MAME_DISPLAY% -rompath "%HERE%roms-PROFILE" -cdrom "%HERE%disc\nosignal.chd" -autoboot_delay 0 -autoboot_script "%HERE%boot.lua" -cfg_directory "%HERE%state\PROFILE\cfg" -nvram_directory "%HERE%state\PROFILE\nvram" -state_directory "%HERE%state\PROFILE\state" -noautosave
if errorlevel 1 pause
'''.replace('PROFILE',profile)
        (stage/f'run_{profile}.bat').write_bytes(bat.replace('\n','\r\n').encode('ascii'))
    # Publish portable evidence, never local command lines, paths or firmware.
    report=dict(game='No Signal Z Alpha',version=2,platform='Neo Geo CD',source_commit=source,
                bios_profiles=['stock CDZ','EagleBIOS CDZ'],hardware_tested=False,
                geometry_and_palette_preserved=True,gameplay=reports,
                resident_sprite_bytes=3556352,backgrounds_verified=13,
                source_sha256=meta['source_sha256'])
    (stage/'VALIDATION.json').write_text(json.dumps(report,indent=2)+'\n')
    extract='''@echo off
setlocal
cd /d "%~dp0"
if not defined CHDMAN set "CHDMAN=chdman"
if not exist extracted mkdir extracted
"%CHDMAN%" extractcd -i disc\\nosignal.chd -o extracted\\nosignal.cue
if errorlevel 1 pause
'''
    (stage/'extract_disc.bat').write_bytes(extract.replace('\n','\r\n').encode('ascii'))
    entries=sorted(p for p in stage.rglob('*') if p.is_file())
    sums=''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.relative_to(stage).as_posix()+'\n' for p in entries)
    (stage/'checksums.sha256').write_text(sums)
    with zipfile.ZipFile(dest,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for p in sorted(stage.rglob('*')):
            if p.is_file():z.write(p,p.relative_to(stage).as_posix())
    (out/'NoSignal-NeoGeoCD-README_v2.txt').write_text(readme)
    (out/'NoSignal-NeoGeoCD-SHA256_v2.txt').write_text(hashlib.sha256(dest.read_bytes()).hexdigest()+'  '+dest.name+'\n')
    print(dest.name, dest.stat().st_size)
    return dest


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('build','out','qa'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--cc',default=default_compiler());a=p.parse_args()
    package(a.build,a.out,a.qa,a.cc)


if __name__=='__main__':main()
