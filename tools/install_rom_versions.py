#!/usr/bin/env python3
"""
Install every ROM version in releases/rom-registry.json under the MAME test
folder (C:\\mame\\neogeosdk), each with its own launcher, plus the scripts that
verify them and boot-test them unattended:

  versions\\<set>\\<version>\\<mvs|aes>\\<game>.zip   the release's MAME set
  versions\\<set>\\<version>\\<mvs|aes>\\hash\\neogeo.xml  its own definition
  launchers\\<set>_<version>_<mode>.bat              play that version
  launchers\\<set>.bat, all_games_menu.bat           newest version, the menu
  verify_all_roms.bat     SHA-256 of every installed set, then MAME's own check
  run_all_tests.bat       boot every version unattended (tools\\boot_check.lua)
  generate_qa_report.bat  the results in reports\\QA_REPORT.txt
  manifests\\versions.json, manifests\\versions.sha256
  current\\roms, current\\bios  the newest set of each game and EagleBIOS

A current release file is installed as an NTFS hard link to dist/release (the
same volume), so it costs no space and can never differ from the release.
Builds that were later replaced under the same name come out of git history
with --history (about 310 MB). The SDK demo of v1.7.0 has no dist/release file:
--demo-zip installs the neogeosdk_v1.7.0_demo_777.zip release asset.

Only files this script wrote (marked "ngsdk-versions") are ever replaced; the
existing launchers, tests\\ and roms\\ are not touched.

    python3 tools/install_rom_versions.py [--target /mnt/c/mame/neogeosdk] [--history] [--demo-zip FILE]
"""

import argparse
import hashlib
import io
import json
import os
import re
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MARK = "ngsdk-versions"
# software list name -> folder and launcher names used in the test folder
SETS = {
    "skylance": ("skylance", "skylance"),
    "maiya": ("maiya", "maiya"),
    "maiyaluna": ("maiyaluna", "maiyaluna"),
    "nosignal": ("nosignal", "nosignal"),
    "shadowleague": ("shadowlf", "shadowlf"),
    "sevenwonders": ("7wonders", "seven_wonders"),
    "mahjong_demo": ("mahjongd", "mahjong"),
    "demo": ("sdkdemo", "sdkdemo"),
}
MODES = {"mame-mvs": "mvs", "mame-aes": "aes"}


def win(path):
    return subprocess.check_output(["wslpath", "-w", str(path)], text=True).strip()


def write_text(path, text, crlf=False):
    """Write a file of ours; refuse to replace one we did not write."""
    if path.exists() and MARK not in path.read_text(errors="replace"):
        raise SystemExit(f"install_rom_versions: {path} exists and was not written by this script; left alone")
    path.parent.mkdir(parents=True, exist_ok=True)
    data = text.replace("\r\n", "\n")
    path.write_bytes((data.replace("\n", "\r\n") if crlf else data).encode())


def link_or_write(dest, source=None, data=None):
    """A hard link to source when possible (no space used), else the bytes."""
    dest.parent.mkdir(parents=True, exist_ok=True)
    if dest.exists():
        same = source and os.path.samefile(dest, source)
        if same or (data is not None and dest.read_bytes() == data):
            return "kept"
        dest.unlink()
    if source:
        os.link(source, dest)
        return "linked"
    dest.write_bytes(data)
    return "written"


def label(version):
    return re.sub(r"[^0-9A-Za-z]+", "_", version).strip("_")


def blob(sha):
    return subprocess.run(["git", "-C", str(ROOT), "cat-file", "blob", sha], capture_output=True, check=True).stdout


def plan(registry, history, demo_zip):
    """[(entry dict)] for every MAME set to install."""
    out = []
    for b in registry["builds"]:
        mode = MODES.get(b.get("target"))
        game = b.get("game")
        if not mode or game not in SETS:
            continue
        current = b["status"] == "current"
        if not current and not history:
            continue
        folder = SETS[game][0]
        vdir = b["version"] if current else f"{b['version']}-build-{b['build_id']}"
        out.append({
            "game": game, "set": folder, "title": b["title"], "version": b["version"], "dir": vdir, "mode": mode,
            "build_id": b["build_id"], "status": b["status"], "source": b["path"], "content_no": b["content_no"],
            "blob": b.get("git_blob"), "sha256": b["archive"]["sha256"],
            "published": sorted({p["tag"] for p in b["published"]}) + b.get("external_matches", []),
            "pushed": b.get("pushed", False),
        })
    # Two archives of one build (same ROM bytes, other files changed) share a
    # folder: install the later one only, so the manifest has one sum per set.
    out = list({(e["set"], e["dir"], e["mode"]): e for e in out}.values())
    if demo_zip:
        data = Path(demo_zip).read_bytes()
        out.append({"game": "demo", "set": "sdkdemo", "title": "SDK Demo", "version": "v1.7.0", "dir": "v1.7.0", "mode": "mvs",
                    "build_id": None, "status": "current", "source": Path(demo_zip).name, "content_no": 1, "blob": None,
                    "sha256": hashlib.sha256(data).hexdigest(), "published": ["v1.7.0", "hbmame:sdkdemo"], "pushed": True,
                    "demo_data": data})
    return out


def install_set(target, e):
    vroot = target / "versions" / e["set"] / e["dir"] / e["mode"]
    zpath = vroot / f"{e['game']}.zip"
    if e.get("demo_data") is not None:
        how = link_or_write(zpath, data=e["demo_data"])
        z = zipfile.ZipFile(io.BytesIO(e["demo_data"]))
        xml = next(n for n in z.namelist() if n.endswith("/neogeo.xml"))
    elif e["status"] == "current":
        how = link_or_write(zpath, source=ROOT / e["source"])
        z = zipfile.ZipFile(zpath)
        xml = "hash/neogeo.xml"
    else:
        data = blob(e["blob"])
        if hashlib.sha256(data).hexdigest() != e["sha256"]:
            raise SystemExit(f"install_rom_versions: git blob {e['blob']} does not hash to the registry's {e['source']} #{e['content_no']}")
        how = link_or_write(zpath, data=data)
        z = zipfile.ZipFile(io.BytesIO(data))
        xml = "hash/neogeo.xml"
    link_or_write(vroot / "hash" / "neogeo.xml", data=z.read(xml))
    if hashlib.sha256(zpath.read_bytes()).hexdigest() != e["sha256"]:
        raise SystemExit(f"install_rom_versions: {zpath} does not match the registry")
    return how


DISPATCH = r"""@echo off
rem ngsdk-versions: written by tools/install_rom_versions.py.
rem ngsdk-version.bat <game> <folder under versions\> <mvs|aes> [boot]
rem Plays one installed ROM version in MAME; with "boot" it runs the
rem unattended boot check instead (tools\boot_check.lua). Every run appends
rem the command and MAME's exit code to logs\<version>.log.
setlocal
for %%I in ("%~dp0..") do set "HERE=%%~fI"
for %%I in ("%HERE%\..") do set "MAMEDIR=%%~fI"
set "MAME_EXE=%MAMEDIR%\mame.exe"
if exist "%HERE%\bin\mame-path.txt" set /p MAME_EXE=<"%HERE%\bin\mame-path.txt"
if not exist "%MAME_EXE%" (
  echo   MAME was not found: %MAME_EXE%
  echo   Put mame.exe in %MAMEDIR% or its path in %HERE%\bin\mame-path.txt.
  exit /b 2
)
set "GAME=%~1"
set "MODE=%~3"
set "VDIR=%HERE%\versions\%~2\%MODE%"
set "TAG=%~2_%MODE%"
set "TAG=%TAG:\=_%"
if not exist "%VDIR%\%GAME%.zip" (
  echo   Not installed: %VDIR%\%GAME%.zip
  exit /b 2
)
set "SYSTEM=neogeo"
set "BIOS=euro"
set "MEMC="
if /i "%MODE%"=="aes" (
  set "SYSTEM=aes"
  set "BIOS=asia"
  if not exist "%VDIR%\memcard.bin" copy /y "%HERE%\bin\memcard-blank.bin" "%VDIR%\memcard.bin" >nul
)
if /i "%MODE%"=="aes" set MEMC=-memc "%VDIR%\memcard.bin"
if not exist "%VDIR%\cfg" mkdir "%VDIR%\cfg"
if not exist "%VDIR%\nvram" mkdir "%VDIR%\nvram"
if not exist "%HERE%\logs" mkdir "%HERE%\logs"
set "LOG=%HERE%\logs\%TAG%.log"
set ARGS=%SYSTEM% -noreadconfig -skip_gameinfo -rompath "%VDIR%;%HERE%\roms" -hashpath "%VDIR%\hash" -bios %BIOS% -cart1 %GAME% %MEMC% -cfg_directory "%VDIR%\cfg" -nvram_directory "%VDIR%\nvram" -noautosave
if /i "%~4"=="boot" goto boot
echo ==== %DATE% %TIME% play %*>>"%LOG%"
"%MAME_EXE%" %ARGS% -snapshot_directory "%HERE%\screenshots\%TAG%" -nowindow -keepaspect %NGSDK_MAME_ARGS% >>"%LOG%" 2>&1
set "RC=%ERRORLEVEL%"
echo exit %RC%>>"%LOG%"
if not "%RC%"=="0" (
  echo   MAME stopped with exit code %RC%; see %LOG%
  pause
)
exit /b %RC%

:boot
if not exist "%HERE%\reports\boot" mkdir "%HERE%\reports\boot"
if not exist "%HERE%\screenshots\%TAG%" mkdir "%HERE%\screenshots\%TAG%"
set "NGSDK_BOOT_REPORT=%HERE%\reports\boot\%TAG%.txt"
set "NGSDK_BOOT_SHOTS=%HERE%\screenshots\%TAG%"
set "NGSDK_BOOT_MODE=%MODE%"
echo ==== %DATE% %TIME% boot %*>>"%LOG%"
"%MAME_EXE%" %ARGS% -video none -sound none -nothrottle -seconds_to_run 40 -autoboot_script "%HERE%\tools\boot_check.lua" >>"%LOG%" 2>&1
set "RC=%ERRORLEVEL%"
echo exit %RC%>>"%LOG%"
echo mame_exit=%RC%>>"%NGSDK_BOOT_REPORT%"
echo   %TAG%: MAME exit %RC%
exit /b %RC%
"""

BOOT_LUA = r"""-- ngsdk-versions: written by tools/install_rom_versions.py.
-- Unattended boot check. Runs the cartridge for 30 emulated seconds: a coin
-- (MVS) and Start at 8 s, button A at 12 s, a snapshot at 6 s (title or
-- attract) and 26 s (after Start). Writes facts, not verdicts on gameplay:
-- whether each CPU kept executing, whether video memory and the palette
-- were written, and whether the Z80 was given sound commands.
local machine = manager.machine
local report = os.getenv('NGSDK_BOOT_REPORT') or 'boot_report.txt'
local shots = os.getenv('NGSDK_BOOT_SHOTS') or '.'
local mode = os.getenv('NGSDK_BOOT_MODE') or 'mvs'
local main = machine.devices[':maincpu']
local audio = machine.devices[':audiocpu']
local screen = machine.screens[':screen']
local memory = main.spaces['program']
local vram = emu.item(machine.devices[':spritegen'].items['0/m_videoram'])
local last_main, last_audio, last_wram, nmain, naudio = nil, nil, nil, 0, 0
local zram = audio.spaces['program']
local frame, shot6, shot26, done = 0, false, false, false

-- Newer MAME lists the buttons twice, as "P1 A" and "%p A": press both. The
-- AES pad is :ctrl1, the MVS panel :edge.
local joy = machine.ioport.ports[':edge:joy:JOY1'] and ':edge:joy:JOY1' or ':ctrl1:joy:JOY'
local start_port = machine.ioport.ports[':edge:joy:START'] and ':edge:joy:START' or ':ctrl1:joy:START_SELECT'
local start_name = start_port == ':edge:joy:START' and '1 Player Start' or 'P1 Start'
local function input(port, name, value)
    local p = machine.ioport.ports[port]
    if not p then return end
    for _, n in ipairs({name, (name:gsub('^P1 ([ABCD])$', '%%p %1'))}) do
        if p.fields[n] then p.fields[n]:set_value(value) end
    end
end

-- The 68000 calls its program counter CURPC in newer MAME, the Z80 PC.
local function pc(dev)
    for _, k in ipairs({'PC', 'CURPC'}) do
        local ok, v = pcall(function() return dev.state[k].value end)
        if ok and v then return v end
    end
end

local function finish(now)
    local sprites, colours = 0, 0
    for word = 0, 0x6fff, 2 do if vram:read(word) ~= 0 then sprites = sprites + 1 end end
    for word = 0, 4095 do if memory:read_u16(0x400000 + word * 2) ~= 0 then colours = colours + 1 end end
    local f = assert(io.open(report, 'w'))
    f:write(string.format('emulated_seconds=%.1f\nframes=%d\n', now, frame))
    f:write(string.format('main_cpu_changes=%d\naudio_cpu_ram_changes=%d\n', nmain, naudio))
    f:write(string.format('vram_words_written=%d\npalette_entries_set=%d\n', sprites, colours))
    f:write(string.format('main_cpu=%s\naudio_cpu=%s\nvideo=%s\n',
        nmain > 20 and 'RUNNING' or 'STALLED', naudio > 5 and 'RUNNING' or 'STALLED',
        (sprites > 0 and colours > 16) and 'WRITTEN' or 'BLANK'))
    f:write('snapshots=' .. shots .. '\n')
    f:write('gameplay=NOT ASSESSED (see the snapshots)\naudio_output=NOT ASSESSED (needs listening or a capture)\n')
    f:close()
    done = true
    machine:exit()
end

emu.register_frame_done(function()
    if done then return end
    frame = frame + 1
    local now = machine.time:as_double()
    -- every 8th frame: reading CPU state each frame slows MAME several times over
    if frame % 8 == 0 then
        -- a CPU runs when its program counter keeps changing between samples
        -- the Z80 driver idles in HALT between interrupts, so its sign of
        -- life is its work RAM ($F800-$F8FF) changing, not its PC
        -- the 68000 may sit in its vblank wait whenever a frame ends: its
        -- work RAM ($100000-$1001FF) changing counts as running too
        local a, b, w = pc(main), 0, 0
        for addr = 0xf800, 0xf8ff do b = (b * 31 + zram:read_u8(addr)) % 1000000007 end
        for addr = 0x100000, 0x1001fe, 2 do w = (w * 31 + memory:read_u16(addr)) % 1000000007 end
        if (a and last_main and a ~= last_main) or (last_wram and w ~= last_wram) then nmain = nmain + 1 end
        last_wram = w
        if last_audio and b ~= last_audio then naudio = naudio + 1 end
        last_main, last_audio = a, b
    end
    if mode ~= 'aes' then input(':AUDIO_COIN', 'Coin 1', (now > 8 and now < 8.3) and 1 or 0) end
    input(start_port, start_name, (now > 9 and now < 9.3) and 1 or 0)
    input(joy, 'P1 A', (now > 12 and now < 12.3) and 1 or 0)
    if now > 6 and not shot6 then screen:snapshot(shots .. '/boot_06s.png'); shot6 = true end
    if now > 26 and not shot26 then screen:snapshot(shots .. '/boot_26s.png'); shot26 = true end
    if now > 30 then finish(now) end
end)
"""


def bat_text(text):
    """Text that echo and rem print as written: cmd.exe would act on & | < > ^."""
    return re.sub(r"([&|<>^])", r"^\1", text)


def menu_bat(entries):
    lines = ["@echo off", f"rem {MARK}: written by tools/install_rom_versions.py.",
             "rem Every installed ROM version. Current release sets are supported in MAME;",
             "rem replaced builds (same name, older bytes) are kept for comparisons.",
             "setlocal", ":menu", "cls", "echo.", "echo   NeoGeoSDK ROM versions in MAME (EagleBIOS)", "echo."]
    for n, e in enumerate(entries, 1):
        kind = "release" if e["status"] == "current" else "replaced build " + (e["build_id"] or "")
        where = ", ".join(e["published"]) or ("public repo" if e["pushed"] else "local only")
        lines.append(bat_text(f"echo   {n:3d}  {e['title'][:26]:26} {e['version']:8} {e['mode'].upper():3}  {kind:28} {where[:40]}"))
    lines += ["echo.", "echo   NeoSD, Darksoft and BackBit packages are for real hardware: see README_TESTING.txt.",
              "echo.", 'set "N="', 'set /p N=  Number to play (Enter quits): ', 'if "%N%"=="" exit /b 0']
    for n, e in enumerate(entries, 1):
        lines.append(f'if "%N%"=="{n}" call "%~dp0{e["launcher"]}"')
    lines += ["goto menu", ""]
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--target", type=Path, default=Path("/mnt/c/mame/neogeosdk"))
    ap.add_argument("--history", action="store_true", help="also install builds replaced under the same name (from git)")
    ap.add_argument("--demo-zip", type=Path, help="the neogeosdk_v1.7.0_demo_777.zip release asset")
    args = ap.parse_args()
    t = args.target
    if not (t / "bin" / "ngsdk.bat").is_file():
        raise SystemExit(f"install_rom_versions: {t} is not the MAME test folder (bin\\ngsdk.bat missing)")
    registry = json.loads((ROOT / "releases" / "rom-registry.json").read_text())
    entries = plan(registry, args.history, args.demo_zip)
    order = {g: i for i, g in enumerate(SETS)}
    entries.sort(key=lambda e: (order[e["game"]], e["status"] != "current",
                                [int(n) for n in re.findall(r"\d+", e["version"])], e["dir"], e["mode"]))

    counts = {}
    for e in entries:
        how = install_set(t, e)
        counts[how] = counts.get(how, 0) + 1
        e["launcher"] = f"{e['set']}_{label(e['dir'])}_{e['mode']}.bat"
        write_text(t / "launchers" / e["launcher"],
                   f"@echo off\nrem {MARK}: written by tools/install_rom_versions.py.\n"
                   + bat_text(f"rem {e['title']} {e['version']} ({e['mode'].upper()}), build {e['build_id']}, from {e['source']}"
                              f"{'' if e['status'] == 'current' else ' content ' + str(e['content_no'])}.") + "\n"
                   f"call \"%~dp0..\\bin\\ngsdk-version.bat\" {e['game']} \"{e['set']}\\{e['dir']}\" {e['mode']} %*\n", crlf=True)

    write_text(t / "bin" / "ngsdk-version.bat", DISPATCH, crlf=True)
    write_text(t / "tools" / "boot_check.lua", BOOT_LUA)
    write_text(t / "launchers" / "all_games_menu.bat", menu_bat(entries), crlf=True)
    for game, (folder, short) in SETS.items():
        newest = [e for e in entries if e["game"] == game and e["status"] == "current" and e["mode"] == "mvs"]
        if newest:
            e = newest[-1]
            write_text(t / "launchers" / f"{short}.bat",
                       f"@echo off\nrem {MARK}: written by tools/install_rom_versions.py.\n"
                       + bat_text(f"rem The newest installed {e['title']} release, {e['version']} MVS.") + "\n"
                       f"call \"%~dp0{e['launcher']}\" %*\n", crlf=True)
            link_or_write(t / "current" / "roms" / f"{game}.zip", source=t / "versions" / e["set"] / e["dir"] / "mvs" / f"{game}.zip")
    for sub in ("neogeo", "aes"):
        for f in sorted((t / "roms" / sub).glob("*")):
            if f.is_file() and not f.name.startswith("uni-bios"):
                link_or_write(t / "current" / "bios" / sub / f.name, source=f)
    for d in ("screenshots", "reports", "logs", "current/screenshots"):
        (t / d).mkdir(parents=True, exist_ok=True)

    manifest = [{k: e[k] for k in ("title", "game", "set", "version", "dir", "mode", "build_id", "status", "source",
                                   "content_no", "sha256", "published", "pushed", "launcher")} for e in entries]
    write_text(t / "manifests" / "versions.json", json.dumps({"note": f"{MARK}: written by tools/install_rom_versions.py "
                                                              "from releases/rom-registry.json", "versions": manifest}, indent=1) + "\n")
    write_text(t / "manifests" / "versions.sha256",
               f"# {MARK}: SHA-256 of every installed set, relative to this folder's parent\n" +
               "".join(f"{e['sha256']}  versions\\{e['set']}\\{e['dir']}\\{e['mode']}\\{e['game']}.zip\n" for e in entries))

    verify = [f"@echo off", f"rem {MARK}: written by tools/install_rom_versions.py.",
              "rem 1) SHA-256 of every installed set against manifests\\versions.sha256;",
              "rem 2) MAME's own check of each set against its hash\\neogeo.xml.",
              "rem MAME reserves a lot of memory: close other programs if Windows is short of it.",
              "setlocal", 'cd /d "%~dp0"', 'if not exist reports mkdir reports', 'set "OUT=%~dp0reports\\verify_roms.txt"',
              'echo NeoGeoSDK ROM verification %DATE% %TIME%>"%OUT%"',
              "powershell -NoProfile -ExecutionPolicy Bypass -Command \"$bad=0; Get-Content 'manifests\\versions.sha256' | "
              "Where-Object { $_ -and -not $_.StartsWith('#') } | ForEach-Object { $h,$p = $_ -split '  ',2; "
              "if (-not (Test-Path $p)) { 'MISSING  ' + $p; $bad++ } elseif ((Get-FileHash -Algorithm SHA256 $p).Hash.ToLower() -ne $h) "
              "{ 'CHANGED  ' + $p; $bad++ } else { 'OK       ' + $p } }; 'sha256 failures: ' + $bad; exit $bad\" >>\"%OUT%\"",
              'set "SHA_RC=%ERRORLEVEL%"',
              'for %%I in ("%~dp0..") do set "MAMEDIR=%%~fI"', 'set "MAME_EXE=%MAMEDIR%\\mame.exe"',
              'if exist "%~dp0bin\\mame-path.txt" set /p MAME_EXE=<"%~dp0bin\\mame-path.txt"']
    for e in entries:
        v = f"%~dp0versions\\{e['set']}\\{e['dir']}\\{e['mode']}"
        verify.append(f'echo [{e["set"]} {e["dir"]} {e["mode"]}]>>"%OUT%"')
        verify.append(f'"%MAME_EXE%" -rompath "{v};%~dp0roms" -hashpath "{v}\\hash" -verifysoftlist neogeo >>"%OUT%" 2>&1')
    verify += ['type "%OUT%"', 'echo.', 'echo   Written to %OUT%', 'if not "%SHA_RC%"=="0" (echo   SHA-256 FAILURES: %SHA_RC%) else (echo   Every SHA-256 matches.)',
               'pause', 'exit /b %SHA_RC%', '']
    write_text(t / "verify_all_roms.bat", "\n".join(verify), crlf=True)

    tests = [f"@echo off", f"rem {MARK}: written by tools/install_rom_versions.py.",
             "rem Boots every installed version unattended, one at a time (about 30 s each):",
             "rem reports\\boot\\<version>.txt and screenshots\\<version>\\boot_06s.png, boot_26s.png.",
             "rem Close other programs first: each MAME run reserves a lot of memory.", "setlocal"]
    for e in entries:
        tests.append(f'call "%~dp0launchers\\{e["launcher"]}" boot')
    tests += ['call "%~dp0generate_qa_report.bat" nopause', 'pause', '']
    write_text(t / "run_all_tests.bat", "\n".join(tests), crlf=True)

    report = [f"@echo off", f"rem {MARK}: written by tools/install_rom_versions.py.",
              "rem Collects reports\\boot\\*.txt and reports\\verify_roms.txt into reports\\QA_REPORT.txt.",
              "setlocal", 'cd /d "%~dp0"',
              "powershell -NoProfile -ExecutionPolicy Bypass -Command \"$o=@('NeoGeoSDK QA report ' + (Get-Date -Format s), ''); "
              "$rows = Get-ChildItem reports\\boot\\*.txt -ErrorAction SilentlyContinue | Sort-Object Name | ForEach-Object { $k=@{}; "
              "Get-Content $_ | ForEach-Object { $a,$b = $_ -split '=',2; $k[$a]=$b }; "
              "'{0,-40} exit {1,-3} main {2,-8} z80 {3,-8} video {4,-8}' -f $_.BaseName,$k['mame_exit'],$k['main_cpu'],$k['audio_cpu'],$k['video'] }; "
              "$o += 'Boot checks (gameplay and audio output need a person: see the snapshots):'; $o += $rows; $o += ''; "
              "if (Test-Path reports\\verify_roms.txt) { $o += 'ROM verification:'; $o += (Get-Content reports\\verify_roms.txt | "
              "Where-Object { $_ -match 'sha256 failures|is good|is bad|not found|CHANGED|MISSING' }) }; "
              "$o | Set-Content -Encoding ascii reports\\QA_REPORT.txt; $o\"",
              'echo.', 'echo   Written to %~dp0reports\\QA_REPORT.txt', 'if /i not "%~1"=="nopause" pause', '']
    write_text(t / "generate_qa_report.bat", "\n".join(report), crlf=True)

    readme = f"""NeoGeoSDK ROM versions: testing  ({MARK}: written by tools/install_rom_versions.py)
==========================================================================================

Every ROM version recorded in the SDK's release registry (releases/rom-registry.json)
is installed here with its own launcher. The existing launchers (MENU.bat, run_*.bat,
levels\\, tests\\) are unchanged: they play the latest builds of the SDK tree.

  launchers\\all_games_menu.bat   pick any version
  launchers\\<set>.bat            the newest release of a game (MVS)
  launchers\\<set>_<version>_<mvs|aes>.bat   one version; add "boot" for the unattended check
  verify_all_roms.bat            SHA-256 of every set, then MAME's check of each
  run_all_tests.bat              boots every version unattended, then writes the report
  generate_qa_report.bat         reports\\QA_REPORT.txt from the last runs

Folders
  versions\\<set>\\<version>\\<mvs|aes>\\   the release's MAME set (<game>.zip) and its hash\\neogeo.xml
  versions\\<set>\\<version>-build-<id>\\  a build later replaced under the same name (kept for comparison)
  current\\roms, current\\bios             the newest set of each game; EagleBIOS
  manifests\\versions.json, .sha256       what is installed, from the registry
  reports\\, screenshots\\, logs\\           results; logs\\<version>.log has every run and MAME's exit code

What the checks do and do not show
  The boot check runs 30 emulated seconds with a coin, Start and button A, and records
  whether the 68000 and the Z80 kept executing, whether video memory and the palette
  were written, and two snapshots. It does not judge gameplay or sound: look at the
  snapshots and play. A version that boots in MAME is not thereby tested on hardware.

  Sets are MVS and AES MAME builds with EagleBIOS (from roms\\neogeo, roms\\aes). MAME
  warns that the BIOS checksums differ from SNK's: expected. NeoSD (.neo), Darksoft and
  BackBit packages in dist\\release are for real cartridges and are not launched here.

Set names: skylance, maiya, maiyaluna, nosignal, shadowlf (software list shadowleague),
7wonders (sevenwonders), mahjongd (mahjong_demo), sdkdemo (demo).

Installed: {len(entries)} sets ({sum(1 for e in entries if e['status'] == 'current')} current releases,
{sum(1 for e in entries if e['status'] != 'current')} replaced builds).
"""
    write_text(t / "README_TESTING.txt", readme, crlf=True)
    print(f"installed {len(entries)} sets into {t} ({', '.join(f'{v} {k}' for k, v in sorted(counts.items()))})")


if __name__ == "__main__":
    main()
