"""Stage a versioned MAME cartridge and per-scene Windows capture launchers."""

from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile
import zlib

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
WORK = GAME / "build/workspace"
TOOLS = ROOT / "tools"
sys.path.insert(0, str(TOOLS))
from patch_neosd_sound import read_regions

STAGES = (
    "emerald_forest", "sacred_falls", "coral_coast", "autumn_grove",
    "crystal_grotto", "world_tree", "rio_negro", "sunken_reef",
    "silver_cave", "golden_savanna", "sky_road", "smog_citadel",
)


def layout_text():
    gdb = next(ROOT.parent.glob("x-tools-v*/m68k-unknown-elf/bin/m68k-unknown-elf-gdb"))
    fields = {key: f"&mg.{key}" for key in
              ("stage", "state", "player", "demo", "next_stage", "state_timer")}
    command = [str(gdb), "-batch", str(WORK / "out/game")]
    for key, expression in fields.items():
        command += ["-ex", f'printf "{key}=%lu\\n", (unsigned long){expression}']
    values = dict(line.split("=", 1) for line in
                  subprocess.check_output(command, text=True).splitlines() if "=" in line)
    if set(values) != set(fields) or any(int(value) < 0x100000 for value in values.values()):
        raise ValueError("Rebuild with --quick to retain scene debug symbols")
    return "return {" + ",".join(f"{key}={int(value)}" for key, value in values.items()) + "}\n"


def batch_files(base):
    launch = base / "run_maiya_v3.bat"
    record = base / "record_maiya_v3.bat"
    launch.write_text("""@echo off
setlocal
set "HERE=%~dp0"
set "TEST=%HERE%render-v3-tests\\mvs"
set "MG_V3_MODE=%~1"
set "MG_V3_STAGE=%~2"
set "MG_V3_LAYOUT=%TEST%\\layout.lua"
set "MAME_EXE=%HERE%..\\mame.exe"
if not exist "%MAME_EXE%" set "MAME_EXE=mame"
"%MAME_EXE%" neogeo -noreadconfig -rompath "%TEST%\\roms" -hashpath "%TEST%\\hash" -bios euro -cart1 maiya -cfg_directory "%TEST%\\cfg" -nvram_directory "%TEST%\\nvram" -autoboot_delay 0 -autoboot_script "%TEST%\\v3_capture.lua" -noautoframeskip -frameskip 0 -throttle -nofilter -window -skip_gameinfo
exit /b %ERRORLEVEL%
""", encoding="ascii")
    record.write_text("""@echo off
setlocal
set "HERE=%~dp0"
set "TEST=%HERE%render-v3-tests\\mvs"
set "MG_V3_MODE=%~1"
set "MG_V3_STAGE=%~2"
set "MG_V3_LAYOUT=%TEST%\\layout.lua"
set "NAME=%MG_V3_MODE%_%MG_V3_STAGE%"
if not exist "%TEST%\\captures" mkdir "%TEST%\\captures"
if exist "%TEST%\\captures\\%NAME%.avi" (
    echo Capture already exists: %TEST%\\captures\\%NAME%.avi
    exit /b 1
)
set "MAME_EXE=%HERE%..\\mame.exe"
if not exist "%MAME_EXE%" set "MAME_EXE=mame"
"%MAME_EXE%" neogeo -noreadconfig -rompath "%TEST%\\roms" -hashpath "%TEST%\\hash" -bios euro -cart1 maiya -cfg_directory "%TEST%\\cfg" -nvram_directory "%TEST%\\nvram" -autoboot_delay 0 -autoboot_script "%TEST%\\v3_capture.lua" -noautoframeskip -frameskip 0 -throttle -nofilter -window -skip_gameinfo -aviwrite "%TEST%\\captures\\%NAME%.avi" -wavwrite "%TEST%\\captures\\%NAME%.wav"
if errorlevel 1 exit /b %ERRORLEVEL%
where ffmpeg >nul 2>nul
if errorlevel 1 (
    echo AVI and WAV saved; install ffmpeg to mux an MP4.
    exit /b 0
)
ffmpeg -hide_banner -loglevel error -i "%TEST%\\captures\\%NAME%.avi" -i "%TEST%\\captures\\%NAME%.wav" -map 0:v:0 -map 1:a:0 -vf "scale=1280:896:flags=neighbor,pad=1920:1080:320:92:black" -c:v libx264 -crf 16 -pix_fmt yuv420p -c:a aac -b:a 192k -shortest "%TEST%\\captures\\%NAME%.mp4"
exit /b %ERRORLEVEL%
""", encoding="ascii")
    for index, name in enumerate(STAGES):
        for verb in ("run", "record"):
            wrapper = base / f"{verb}_maiya_v3_{index + 1:02d}_{name}.bat"
            wrapper.write_text(f'@echo off\ncall "%~dp0{verb}_maiya_v3.bat" stage {index}\nexit /b %ERRORLEVEL%\n', encoding="ascii")
    for index in range(1, len(STAGES)):
        for verb in ("run", "record"):
            wrapper = base / f"{verb}_maiya_v3_transition_{index:02d}_to_{index + 1:02d}.bat"
            wrapper.write_text(f'@echo off\ncall "%~dp0{verb}_maiya_v3.bat" transition {index}\nexit /b %ERRORLEVEL%\n', encoding="ascii")
    for mode in ("attract", "title", "select", "intro", "bonus", "ending", "full"):
        for verb in ("run", "record"):
            wrapper = base / f"{verb}_maiya_v3_{mode}.bat"
            wrapper.write_text(f'@echo off\ncall "%~dp0{verb}_maiya_v3.bat" {mode} 0\nexit /b %ERRORLEVEL%\n', encoding="ascii")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mame-dir", type=Path, default=Path("/mnt/c/mame/neogeosdk"))
    parser.add_argument("--refresh", action="store_true", help="Refresh this v3 cartridge, keeping captures")
    parser.add_argument("--sound-base", type=Path,
                        default=ROOT / "dist/release/Maiya-WIP-NeoSD_MVS_SOUND_FIX_PROTOCOL_PRERELEASE_v1.neo")
    parser.add_argument("--bios-dir", type=Path, help="Local MAME-compatible Neo Geo BIOS files")
    args = parser.parse_args()
    base = args.mame_dir.resolve()
    target = base / "render-v3-tests/mvs"
    rom_target = target / "roms/maiya"
    if target.exists() and not args.refresh:
        parser.error(f"{target} exists; preserve previous captures")
    regions = read_regions(args.sound_base.read_bytes())
    if regions["m1"][0xC0:0xC4] != b"NGP2" or len(regions["v1"]) != 0x800000:
        parser.error("Sound base lacks the verified MVS protocol or aligned V1")
    rom_target.mkdir(parents=True, exist_ok=args.refresh)
    for name in ("p1.p1", "s1.s1", "c1.c1", "c2.c2"):
        shutil.copy2(WORK / "roms/maiya" / f"780-{name}", rom_target / f"780-{name}")
    for key in ("m1", "v1"):
        path = rom_target / f"780-{key}.{key}"
        path.write_bytes(regions[key])
        print(f"{path.name} {hashlib.sha256(regions[key]).hexdigest()}")
        (WORK / "roms/maiya" / path.name).write_bytes(regions[key])
    with zipfile.ZipFile(target / "roms/maiya.zip", "w", compression=zipfile.ZIP_STORED) as archive:
        for path in sorted(rom_target.glob("780-*")):
            archive.write(path, path.name)
    if args.bios_dir:
        expected = {"sp-s2.sp1": 0x9036D879, "sm1.sm1": 0x94416D67,
                    "000-lo.lo": 0x5A86CFF2, "sfix.sfix": 0xC2EA0CFD}
        with zipfile.ZipFile(target / "roms/neogeo.zip", "w", compression=zipfile.ZIP_STORED) as archive:
            for name, crc in expected.items():
                data = (args.bios_dir / name).read_bytes()
                if len(data) != 131072 or zlib.crc32(data) != crc:
                    parser.error(f"{name} does not match this MAME BIOS set")
                archive.writestr(name, data)
    subprocess.run([sys.executable, "hash_eagle/gen_hash.py"], cwd=WORK,
                   env=dict(os.environ, GAME="maiya", GAME_ID="780"), check=True)
    shutil.copy2(GAME / "tools/v3_capture.lua", target / "v3_capture.lua")
    (target / "layout.lua").write_text(layout_text())
    hash_dir = target / "hash"
    hash_dir.mkdir(exist_ok=args.refresh)
    source_hash = WORK / "hash_eagle/maiya/neogeo.xml"
    shutil.copy2(source_hash, hash_dir / "neogeo.xml")
    for directory in ("cfg", "nvram", "captures"):
        (target / directory).mkdir(exist_ok=args.refresh)
    batch_files(base)
    print(f"Staged {rom_target}")


if __name__ == "__main__":
    main()
