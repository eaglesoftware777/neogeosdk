#!/usr/bin/env python3
"""
Write the Windows launchers of a MAME test folder (C:\\mame\\neogeosdk): play,
start at a level, the three video steps and MENU.bat, for every game
installed under its tests\\ folder.

    python3 tools/mame_bats.py --dest /mnt/c/mame/neogeosdk
    python3 tools/mame_bats.py --dest /mnt/c/mame/neogeosdk --describe nosignal mvs out/game

The launchers all call bin\\ngsdk.bat, so each one stays a single line:

    run_<game>_<mvs|aes>.bat             play (run_<game>.bat [mvs|aes] [sharp|crt|smooth])
    run_<game>_<mvs|aes>_scanlines.bat   play with the CRT shader
    run_<game>_<mvs|aes>_unibios.bat     play with UniBIOS 4.0
    1_record_<game>_<mode>_inp.bat       video step 1: play while MAME records the inputs
    2_export_<game>_<mode>_avi.bat       video step 2: replay them into a raw AVI
    3_save_<game>_<mode>_mp4.bat         video step 3: the AVI to a 1080p MP4 for YouTube
    levels\\<game>\\<mode>_<level>_NN.bat  start a game at that level

--describe records what the level launchers need from one installed build
(where a few variables of that build live, and the layout of its structures,
compiled from the build's own source with --src) in
tests\\<game>\\<mode>\\levels.json. Run it whenever a build of a game with
levels is installed; its level launchers are left out until it has been.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MARK = "ngsdk-bats"          # in every file this tool owns, so installers leave them alone
MODES = ("mvs", "aes")

# Each game's menu entry; "levels" says what its level launchers start at.
GAMES = {
    "maiya": {"title": "Maiya: Super Nature Girl", "levels": "mission"},
    "maiyaluna": {"title": "Maiya & Luna"},
    "nosignal": {"title": "No Signal Z Alpha", "levels": "stage"},
    "sevenwonders": {"title": "Seven Wonders Puzzles", "levels": "level"},
    "shadowleague": {"title": "Shadow League Fighting"},
    "skylance": {"title": "Sky Lance", "levels": "sector"},
    "mahjong_demo": {"title": "Mahjong Demo"},
}

# What --describe looks up. symbols: (source file or None, name); consts: C
# expressions compiled against the build's source after `include`.
PROBES = {
    "maiya": {
        "symbols": {"mg": ("maiya_game.c", "mg")},
        "include": ["games/maiya/scenes/maiya_game.c"],
        "consts": {
            "off_stage": "offsetof(MGState, stage)", "off_state": "offsetof(MGState, state)",
            "off_next_stage": "offsetof(MGState, next_stage)", "off_demo": "offsetof(MGState, demo)",
            "off_state_timer": "offsetof(MGState, state_timer)",
            "size_state": "sizeof(((MGState *)0)->state)",
            "size_state_timer": "sizeof(((MGState *)0)->state_timer)",
            "st_play": "MG_PLAY", "st_interlude": "MG_INTERLUDE", "count": "MG_LEVEL_COUNT",
        },
    },
    "nosignal": {
        "symbols": {"s_b": ("ns_game.c", "s_b"), "s_mode": ("ns_game.c", "s_mode"),
                    "s_stage": ("ns_game.c", "s_stage"), "ns_stages": (None, "ns_stages")},
        "include": ["games/nosignal/scenes/ns_game.c"],
        "consts": {
            "count": "NS_STAGES", "info_size": "sizeof(NSStageInfo)",
            "off_info_stage": "offsetof(NSStageInfo, stage)", "off_waves": "offsetof(NGBStage, waves)",
            "off_nwaves": "offsetof(NGBStage, nwaves)",
            "off_status": "offsetof(NGBeatEmUp, status)", "off_ev_head": "offsetof(NGBeatEmUp, ev_head)",
            "off_ev_count": "offsetof(NGBeatEmUp, ev_count)", "off_ev": "offsetof(NGBeatEmUp, ev)",
            "ev_size": "sizeof(NGBEvent)", "off_ev_type": "offsetof(NGBEvent, type)",
            "queue": "NGB_EVENT_QUEUE", "ev_stage_clear": "NGB_EV_STAGE_CLEAR",
            "playing": "NGB_PLAYING", "mode_story": "MODE_STORY",
        },
    },
    "sevenwonders": {
        "symbols": {"s_p": ("sw_game.c", "s_p"), "sw_levels": (None, "sw_levels")},
        "include": ["sdk/2d_engine/ng_puzzle.c", "games/sevenwonders/scenes/sw.h"],
        "consts": {
            "count": "SW_LEVELS", "level_size": "sizeof(NGPuzzleLevel)",
            "off_state": "offsetof(NGPuzzle, state)", "off_level": "offsetof(NGPuzzle, level)",
            "st_idle": "ST_IDLE", "st_won": "ST_WON",
            "off_ev_head": "offsetof(NGPuzzle, ev_head)", "off_ev_count": "offsetof(NGPuzzle, ev_count)",
            "off_ev": "offsetof(NGPuzzle, ev)", "ev_size": "sizeof(NGPuzzleEvent)",
            "off_ev_type": "offsetof(NGPuzzleEvent, type)", "off_ev_cell": "offsetof(NGPuzzleEvent, cell)",
            "off_ev_a": "offsetof(NGPuzzleEvent, a)", "queue": "NGP_EVENT_QUEUE",
            "ev_won": "NGP_EV_WON", "no_cell": "NGP_NO_CELL", "end_objectives": "NGP_END_OBJECTIVES",
        },
    },
    "skylance": {
        "symbols": {"s_stage": ("sky_sortie.c", "s_stage"), "s_player": ("sky_sortie.c", "s_player")},
        "include": ["games/skylance/scenes/sky_stage.h"],
        "consts": {"count": "SKY_STAGE_COUNT"},
    },
}


# ---------------------------------------------------------------- describe

def toolchain(src):
    for base in (src.parent / "x-tools-v3", ROOT.parent / "x-tools-v3", src.parent / "x-tools-v2", ROOT.parent / "x-tools-v2"):
        gcc = base / "m68k-unknown-elf/bin/m68k-unknown-elf-gcc"
        if gcc.is_file():
            return gcc.parent / "m68k-unknown-elf-"
    raise SystemExit("mame_bats: no m68k toolchain next to the source")


def elf_symbols(prefix, elf):
    """name -> {file: address}, from the ELF's symbol table (FILE entries name each local's source)."""
    out = subprocess.check_output([f"{prefix}readelf", "-sW", str(elf)], text=True)
    table, current = {}, None
    for line in out.splitlines():
        f = line.split()
        if len(f) < 8 or not f[0].endswith(":"):
            continue
        if f[3] == "FILE":
            current = f[7]
        elif f[3] in ("OBJECT", "NOTYPE", "FUNC"):
            table.setdefault(f[7], {})[current if f[4] == "LOCAL" else None] = int(f[1], 16)
    return table


def compile_flags(src, game, platform):
    """The build's own compile line for a source of the game, minus source and output."""
    cmd = ["make", "-B", "-n", f"GAME={game}", f"GAME_CFG_FILE=games/{game}/game.cfg", "p1"]
    if platform == "aes":
        cmd.insert(4, "PLATFORM=aes")
    out = subprocess.run(cmd, cwd=src, capture_output=True, text=True).stdout
    for part in re.split(r"\|\| exit \$\?;|\n", out):
        m = re.search(rf"(\S*m68k-unknown-elf-gcc) (-c .*?) games/{game}/\S+\.c -o \S+", part)
        if m:
            flags = m.group(2).split()
            return [f for f in flags if not f.startswith("-O")] + ["-O0"]
    raise SystemExit(f"mame_bats: no compile line for games/{game} in {src}")


def probe(src, game, platform, include, consts):
    prefix = toolchain(src)
    lines = ["#include <stddef.h>"] + [f'#include "{src / i}"' for i in include]
    lines.append('#define K(n, v) __asm__(".globl ngk_" #n "\\n.set ngk_" #n ", %c0" :: "i"((long)(v)))')
    lines.append("void ngk_probe(void);\nvoid ngk_probe(void)\n{")
    lines += [f"    K({k}, {v});" for k, v in consts.items()]
    lines.append("}")
    with tempfile.TemporaryDirectory() as tmp:
        c, o = Path(tmp) / "probe.c", Path(tmp) / "probe.o"
        c.write_text("\n".join(lines) + "\n")
        flags = compile_flags(src, game, platform)
        run = subprocess.run([f"{prefix}gcc", *flags, "-w", str(c), "-o", str(o)], cwd=src, capture_output=True, text=True)
        if run.returncode:
            raise SystemExit(f"mame_bats: probe for {game} did not compile:\n{run.stderr[-2000:]}")
        out = subprocess.check_output([f"{prefix}nm", str(o)], text=True)
    values = {}
    for line in out.split("\n"):
        f = line.split()
        if len(f) == 3 and f[2].startswith("ngk_") and f[2] != "ngk_probe":
            v = int(f[0], 16)
            values[f[2][4:]] = v - (1 << 32) if v >= 1 << 31 else v
    return values


def nice(name):
    """EMERALD FOREST -> Emerald Forest, keeping AI and the like."""
    return " ".join(w if w in ("AI", "II", "III", "IV", "VI", "VII") else w.capitalize() for w in name.split())


def level_names(src, game, count):
    """The levels' own names where the source has them."""
    names = []
    if game == "maiya":
        text = (src / "games/maiya/scenes/maiya_levels_data.h").read_text(errors="replace")
        names = [nice(n) for n in re.findall(r'/\* \d+_\w+\.json \*/ \{"([^"]+)"', text)]
    elif game == "nosignal":
        text = (src / "games/nosignal/scenes/ns_data.c").read_text(errors="replace")
        names = [nice(n) for n in re.findall(r'"STAGE \d+", "([^"]+)"', text)]
    elif game == "sevenwonders":
        text = (src / "games/sevenwonders/scenes/sw_levels_data.h").read_text(errors="replace")
        body = text[text.find("sw_chapters"):]
        chapters = [nice(n) for n in re.findall(r'\{ "([^"]+)", "', body)][:7]
        # four levels a chapter at first; later builds end each chapter with a picture
        per = re.search(r"#define SW_PER_CHAPTER\s+(\d+)", (src / "games/sevenwonders/scenes/sw.h").read_text(errors="replace"))
        per = int(per.group(1)) if per else 4
        names = [f"{c}, part {k + 1}" if k < 4 else f"{c}, picture" for c in chapters for k in range(per)]
    elif game == "skylance":
        names = [f"Sector {k + 1}" for k in range(count)]
    return names[:count] if len(names) >= count else [f"{GAMES[game]['levels'].title()} {k + 1}" for k in range(count)]


def describe(dest, game, platform, elf, src):
    if game not in PROBES:
        raise SystemExit(f"mame_bats: {game} has no level launchers")
    spec = PROBES[game]
    data = data_dir(dest, game, platform)
    if data is None:
        raise SystemExit(f"mame_bats: {game} {platform} is not installed in {dest}")
    prefix = toolchain(src)
    table = elf_symbols(prefix, elf)
    symbols = {}
    for key, (where, name) in spec["symbols"].items():
        found = table.get(name, {})
        if where in found:
            symbols[key] = found[where]
        elif None in found:
            symbols[key] = found[None]
        elif len(found) == 1:
            symbols[key] = next(iter(found.values()))
        else:
            raise SystemExit(f"mame_bats: {name} ({where}) not in {elf}")
    consts = probe(src, game, platform, spec["include"], spec["consts"])
    p1 = next((data / "roms" / game).glob("*-p1.p1"))
    info = {"game": game, "platform": platform, "p1_sha256": hashlib.sha256(p1.read_bytes()).hexdigest(),
            "symbols": symbols, "consts": consts, "names": level_names(src, game, consts["count"])}
    (data / "levels.json").write_text(json.dumps(info, indent=1) + "\n", encoding="ascii")
    print(f"mame_bats: {game} {platform}: {consts['count']} {GAMES[game]['levels']}s -> {data / 'levels.json'}")


# ---------------------------------------------------------------- the files

def crlf(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(text.replace("\r\n", "\n").replace("\n", "\r\n").encode("ascii"))


def data_dir(dest, game, mode):
    """Where a game's platform lives: tests\\<game>\\<mode>, or tests\\<mode> (Maiya's own installer)."""
    for d in (dest / "tests" / game / mode, dest / "tests" / mode):
        if (d / "roms" / game).is_dir() and any((d / "roms" / game).glob("*-p1.p1")):
            return d
    return None


ENGINE = r"""@echo off
rem ngsdk-bats: written by tools/mame_bats.py (NeoGeoSDK); change the tool, not this file.
rem
rem   ngsdk.bat play   GAME mvs^|aes [sharp^|crt^|smooth] [unibios]
rem   ngsdk.bat level  GAME mvs^|aes N [sharp^|crt^|smooth]   begin at level N
rem   ngsdk.bat record GAME mvs^|aes [N]   video step 1: play; MAME records the inputs
rem   ngsdk.bat avi    NAME                video step 2: replay inp\NAME.inp into an AVI
rem   ngsdk.bat mp4    NAME                video step 3: that AVI to a 1080p MP4
rem
rem settings\filter.txt (sharp, crt or smooth), settings\prescale.txt and
rem settings\video_dir.txt change the defaults; MENU.bat sets them.
rem settings\ffmpeg-path.txt, when there, names the FFmpeg for step 3.
rem NGSDK_MAME_ARGS, when set, is added to every MAME command line.
setlocal EnableExtensions
for %%I in ("%~dp0..") do set "HERE=%%~fI"
for %%I in ("%HERE%\..") do set "MAMEDIR=%%~fI"
set "MAME_EXE=%MAMEDIR%\mame.exe"
if exist "%HERE%\mame-path.txt" set /p MAME_EXE=<"%HERE%\mame-path.txt"
if not exist "%MAME_EXE%" goto no_mame
set "SNAPDIR=%HERE%\snap"
set "RC=0"
if /i "%~1"=="play" goto play
if /i "%~1"=="level" goto level
if /i "%~1"=="record" goto record
if /i "%~1"=="avi" goto avi
if /i "%~1"=="mp4" goto mp4
echo ngsdk.bat: unknown action "%~1"
set "RC=1"
goto done

rem ------------------------------------------------------------------ play
:play
set "UNI="
if /i "%~4"=="unibios" set "UNI=unibios"
if /i "%~5"=="unibios" set "UNI=unibios"
call :setup "%~2" "%~3" "%UNI%" || goto failed
set "LOOK=%~4"
if /i "%LOOK%"=="unibios" set "LOOK="
call :display "%LOOK%"
call :args
call :title "%GAME% %MODE%, %FILTER% picture" "%BIOS%"
pushd "%MAMEDIR%"
"%MAME_EXE%" %ARGS% %MDISPLAY% %NGSDK_MAME_ARGS%
set "RC=%ERRORLEVEL%"
popd
goto done

rem ----------------------------------------------------------------- level
:level
call :setup "%~2" "%~3" "" || goto failed
set "NGSDK_LEVEL=%~4"
if "%NGSDK_LEVEL%"=="" set "NGSDK_LEVEL=1"
set "LUA=%HERE%\levels\%GAME%_%MODE%.lua"
if not exist "%LUA%" goto no_levels
call :display "%~5"
call :args
call :title "%GAME% %MODE% from level %NGSDK_LEVEL%" "%BIOS%"
echo   Insert a coin and start as usual: the game begins at level %NGSDK_LEVEL%.
echo   The levels before it are skipped at full speed.
pushd "%MAMEDIR%"
"%MAME_EXE%" %ARGS% %MDISPLAY% -autoboot_script "%LUA%" %NGSDK_MAME_ARGS%
set "RC=%ERRORLEVEL%"
popd
goto done

rem ---------------------------------------------------------------- record
:record
call :setup "%~2" "%~3" "" || goto failed
set "NGSDK_LEVEL=%~4"
set "NAME=%GAME%_%MODE%"
set "LUA="
if not "%NGSDK_LEVEL%"=="" set "NAME=%GAME%_%MODE%_level%NGSDK_LEVEL%"
if not "%NGSDK_LEVEL%"=="" set "LUA=%HERE%\levels\%GAME%_%MODE%.lua"
if not "%LUA%"=="" if not exist "%LUA%" goto no_levels
if exist "%HERE%\inp\%NAME%.inp" call :record_over || goto done
rem what the game starts from (settings, saves, memory card) is kept with the
rem recording: the replay must begin from exactly the same state
set "START=%HERE%\inp\%NAME%"
if exist "%START%\" rd /s /q "%START%"
mkdir "%START%\cfg" "%START%\nvram"
if exist "%CFG%\" xcopy /e /i /q /y "%CFG%" "%START%\cfg" >nul
if exist "%NVRAM%\" xcopy /e /i /q /y "%NVRAM%" "%START%\nvram" >nul
copy /y "%CARD%" "%START%\memcard.bin" >nul
set "LUAARG="
if not "%LUA%"=="" copy /y "%LUA%" "%START%\level.lua" >nul
if not "%LUA%"=="" set LUAARG=-autoboot_script "%START%\level.lua"
call :p1hash
>"%START%\recording.txt" echo game=%GAME%
>>"%START%\recording.txt" echo mode=%MODE%
>>"%START%\recording.txt" echo level=%NGSDK_LEVEL%
>>"%START%\recording.txt" echo p1=%P1HASH%
>>"%START%\recording.txt" echo date=%DATE% %TIME%
call :display ""
call :args
call :title "VIDEO STEP 1 of 3: record %NAME%" "%BIOS%"
echo   Play as long as you like, then quit MAME (Esc). Your inputs go to
echo   %HERE%\inp\%NAME%.inp
pushd "%MAMEDIR%"
"%MAME_EXE%" %ARGS% %MDISPLAY% -input_directory "%HERE%\inp" -record "%NAME%.inp" %LUAARG% %NGSDK_MAME_ARGS%
set "RC=%ERRORLEVEL%"
popd
if not exist "%HERE%\inp\%NAME%.inp" goto no_recording
echo.
echo   Step 1 done: %HERE%\inp\%NAME%.inp
echo   Next: 2_export_%GAME%_%MODE%_avi.bat, or the menu's video steps.
goto done_pause

rem ------------------------------------------------------------------- avi
:avi
set "NAME=%~2"
set "START=%HERE%\inp\%NAME%"
if not exist "%HERE%\inp\%NAME%.inp" goto no_recording
set "REC_game="
set "REC_mode="
set "REC_level="
set "REC_p1="
set "PLAY=%HERE%\inp\%NAME%.replay"
if exist "%PLAY%\" rd /s /q "%PLAY%"
if not exist "%START%\recording.txt" goto avi_old
for /f "usebackq tokens=1,* delims==" %%A in ("%START%\recording.txt") do set "REC_%%A=%%B"
call :setup "%REC_game%" "%REC_mode%" "" || goto failed
call :p1hash
if not "%P1HASH%"=="%REC_p1%" echo   WARNING: the game was rebuilt since this recording; the replay may not match it.
set "NGSDK_LEVEL=%REC_level%"
xcopy /e /i /q /y "%START%" "%PLAY%" >nul
goto avi_play
:avi_old
rem a recording made by the earlier launchers (GAME_MODE.inp) kept no start
rem state: it replays from a copy of the game's settings and saves as they are now
if /i "%NAME:~-4%"=="_mvs" set "REC_mode=mvs"
if /i "%NAME:~-4%"=="_aes" set "REC_mode=aes"
if "%REC_mode%"=="" goto no_recording
set "REC_game=%NAME:~0,-4%"
call :setup "%REC_game%" "%REC_mode%" "" || goto failed
echo   NOTE: %NAME%.inp was made by the earlier launchers, which kept no start state.
echo   It replays from the game's settings and saves as they are now, so it can
echo   drift if they, or the game, changed since it was recorded.
mkdir "%PLAY%\cfg" "%PLAY%\nvram"
if exist "%CFG%\" xcopy /e /i /q /y "%CFG%" "%PLAY%\cfg" >nul
if exist "%NVRAM%\" xcopy /e /i /q /y "%NVRAM%" "%PLAY%\nvram" >nul
copy /y "%CARD%" "%PLAY%\memcard.bin" >nul
:avi_play
set "CFG=%PLAY%\cfg"
set "NVRAM=%PLAY%\nvram"
set "CARD=%PLAY%\memcard.bin"
set "LUAARG="
if exist "%PLAY%\level.lua" set LUAARG=-autoboot_script "%PLAY%\level.lua"
call :videodir
set "SNAPDIR=%VIDEODIR%"
if exist "%VIDEODIR%\%NAME%.avi" del /q "%VIDEODIR%\%NAME%.avi"
call :args
call :title "VIDEO STEP 2 of 3: replay %NAME% into an AVI" "%BIOS%"
echo   MAME replays the recording as fast as it can, silently, and closes at
echo   its end. The AVI keeps the game's own pixels (320x224) and its sound;
echo   step 3 scales the pixels.
rem MAME adds .avi to the name it is given
pushd "%MAMEDIR%"
"%MAME_EXE%" %ARGS% -window -video d3d -nowaitvsync -nothrottle -sound none -input_directory "%HERE%\inp" -playback "%NAME%.inp" -exit_after_playback -snapview native -snapsize auto -nosnapbilinear -aviwrite "%NAME%" %LUAARG% %NGSDK_MAME_ARGS%
set "RC=%ERRORLEVEL%"
popd
if not exist "%VIDEODIR%\%NAME%.avi" if exist "%VIDEODIR%\%NAME%" ren "%VIDEODIR%\%NAME%" "%NAME%.avi"
rd /s /q "%PLAY%" 2>nul
if not exist "%VIDEODIR%\%NAME%.avi" goto no_avi
echo.
echo   Step 2 done: %VIDEODIR%\%NAME%.avi
echo   Next: 3_save_%REC_game%_%REC_mode%_mp4.bat, or the menu's video steps.
goto done_pause

rem ------------------------------------------------------------------- mp4
:mp4
set "NAME=%~2"
call :videodir
if not exist "%VIDEODIR%\%NAME%.avi" goto no_avi
call :ffmpeg || goto failed
call :title "VIDEO STEP 3 of 3: %NAME%.avi to %NAME%.mp4" ""
pushd "%VIDEODIR%"
"%FFMPEG%" -i "%NAME%.avi" ^
  -vf "scale=1440:1080:flags=neighbor,pad=1920:1080:240:0:black" ^
  -c:v libx264 ^
  -preset slow ^
  -crf 16 ^
  -profile:v high ^
  -level 4.1 ^
  -pix_fmt yuv420p ^
  -movflags +faststart ^
  -c:a aac ^
  -b:a 192k ^
  -ar 48000 ^
  -ac 2 ^
  "%NAME%.mp4"
set "RC=%ERRORLEVEL%"
popd
if not "%RC%"=="0" goto done
echo.
echo   Step 3 done: %VIDEODIR%\%NAME%.mp4
choice /c YN /n /m "  Delete the AVI now to free its space? [Y/N] "
if errorlevel 2 goto done_pause
del /q "%VIDEODIR%\%NAME%.avi"
echo   Deleted %NAME%.avi
goto done_pause

rem -------------------------------------------------------------- helpers
:setup
set "GAME=%~1"
set "MODE=%~2"
if /i not "%MODE%"=="aes" set "MODE=mvs"
set "DATA=%HERE%\tests\%GAME%\%MODE%"
if not exist "%DATA%\roms\%GAME%\" if exist "%HERE%\tests\%MODE%\roms\%GAME%\" set "DATA=%HERE%\tests\%MODE%"
if not exist "%DATA%\roms\%GAME%\" goto not_installed
set "ROMS=%DATA%\roms;%HERE%\roms"
if exist "%HERE%\tests\%GAME%\bios\" set "ROMS=%HERE%\tests\%GAME%\bios;%DATA%\roms;%HERE%\roms"
set "SYSTEM=neogeo"
set "BIOS=euro"
if /i "%MODE%"=="aes" set "SYSTEM=aes"
if /i "%MODE%"=="aes" set "BIOS=asia"
if /i "%~3"=="unibios" set "BIOS=unibios40"
set "CFG=%DATA%\cfg"
set "NVRAM=%DATA%\nvram"
if not exist "%DATA%\state\" mkdir "%DATA%\state"
set "CARD=%HERE%\tests\aes\memcard.bin"
if not exist "%HERE%\tests\aes\" mkdir "%HERE%\tests\aes"
if not exist "%CARD%" copy /y "%~dp0memcard-blank.bin" "%CARD%" >nul
set "P1="
for %%F in ("%DATA%\roms\%GAME%\*-p1.p1") do set "P1=%%~fF"
exit /b 0

:not_installed
echo.
echo   %~1 is not installed for %MODE% in %HERE%\tests.
exit /b 1

:record_over
echo.
for %%F in ("%HERE%\inp\%NAME%.inp") do echo   There is already a recording %NAME%.inp (%%~tF).
choice /c YN /n /m "  Record over it? [Y/N] "
if errorlevel 2 exit /b 1
del /q "%HERE%\inp\%NAME%.inp"
exit /b 0

:args
set "MEMC="
if /i "%MODE%"=="aes" set MEMC=-memc "%CARD%"
set ARGS=%SYSTEM% -noreadconfig -skip_gameinfo -rompath "%ROMS%" -hashpath "%DATA%\hash" -bios %BIOS% -cart1 %GAME% %MEMC% -cfg_directory "%CFG%" -nvram_directory "%NVRAM%" -state_directory "%DATA%\state" -snapshot_directory "%SNAPDIR%" -noautosave
exit /b 0

:display
rem sharp : bilinear over a 4x nearest-neighbour picture (settings\prescale.txt):
rem         crisp pixels without the uneven widths of plain scaling
rem crt   : the crt-geom-deluxe shader (scanlines, mask, curvature)
rem smooth: the super-xBR shader (rounded edges)
set "FILTER=%~1"
if "%FILTER%"=="" if exist "%HERE%\settings\filter.txt" set /p FILTER=<"%HERE%\settings\filter.txt"
if /i not "%FILTER%"=="crt" if /i not "%FILTER%"=="smooth" set "FILTER=sharp"
set "PRESCALE=4"
if exist "%HERE%\settings\prescale.txt" set /p PRESCALE=<"%HERE%\settings\prescale.txt"
set "MDISPLAY=-video d3d -filter -prescale %PRESCALE%"
if /i "%FILTER%"=="crt" set MDISPLAY=-video bgfx -bgfx_backend d3d11 -bgfx_path "%MAMEDIR%\bgfx" -bgfx_screen_chains crt-geom-deluxe -filter
if /i "%FILTER%"=="smooth" set MDISPLAY=-video bgfx -bgfx_backend d3d11 -bgfx_path "%MAMEDIR%\bgfx" -bgfx_screen_chains xbr/super-xbr-6p -filter
set MDISPLAY=-nowindow -keepaspect -triplebuffer -waitvsync %MDISPLAY%
exit /b 0

:videodir
set "VIDEODIR="
if exist "%HERE%\settings\video_dir.txt" set /p VIDEODIR=<"%HERE%\settings\video_dir.txt"
if "%VIDEODIR%"=="" if exist "E:\" set "VIDEODIR=E:\NeoGeoVideos"
if "%VIDEODIR%"=="" set "VIDEODIR=%HERE%\video"
if not exist "%VIDEODIR%\" mkdir "%VIDEODIR%"
exit /b 0

:ffmpeg
rem the first FFmpeg that has libx264: settings\ffmpeg-path.txt, the PATH, C:\ffmpeg\bin
set "FFMPEG="
if exist "%HERE%\settings\ffmpeg-path.txt" set /p FFMPEG=<"%HERE%\settings\ffmpeg-path.txt"
if defined FFMPEG call :has_x264 "%FFMPEG%" && exit /b 0
set "FFMPEG="
for /f "delims=" %%X in ('where ffmpeg.exe 2^>nul') do if not defined FFMPEG call :has_x264 "%%X" && set "FFMPEG=%%X"
if defined FFMPEG exit /b 0
if exist "C:\ffmpeg\bin\ffmpeg.exe" call :has_x264 "C:\ffmpeg\bin\ffmpeg.exe" && set "FFMPEG=C:\ffmpeg\bin\ffmpeg.exe"
if defined FFMPEG exit /b 0
goto no_ffmpeg

:has_x264
"%~1" -hide_banner -encoders 2>nul | findstr /c:" libx264 " >nul
exit /b %ERRORLEVEL%

:p1hash
set "P1HASH="
if "%P1%"=="" exit /b 0
for /f "delims=" %%H in ('certutil -hashfile "%P1%" SHA256 ^| findstr /v ":"') do set "P1HASH=%%H"
if defined P1HASH set "P1HASH=%P1HASH: =%"
exit /b 0

:title
echo.
echo  ==================================================================
echo   %~1
if not "%~2"=="" echo   BIOS: %~2
echo  ==================================================================
exit /b 0

rem ---------------------------------------------------------------- errors
:no_mame
echo   MAME was not found: %MAME_EXE%
echo   Put mame.exe in %MAMEDIR% or its path in %HERE%\mame-path.txt.
set "RC=1"
goto done_pause
:no_levels
echo   No level launcher for %GAME% %MODE%: %HERE%\levels\%GAME%_%MODE%.lua is missing.
set "RC=1"
goto done_pause
:no_recording
echo   No recording %HERE%\inp\%NAME%.inp: run video step 1 first.
set "RC=1"
goto done_pause
:no_avi
echo   No AVI %VIDEODIR%\%NAME%.avi: run video step 2 first.
set "RC=1"
goto done_pause
:no_ffmpeg
echo   No FFmpeg with libx264 was found. Install a full build (choco install
echo   ffmpeg-full), or put its ffmpeg.exe path in %HERE%\settings\ffmpeg-path.txt.
exit /b 1
:failed
set "RC=1"
goto done_pause
:done
if not "%RC%"=="0" pause
exit /b %RC%
:done_pause
echo.
if not defined NGSDK_MENU pause
exit /b %RC%
"""

PRELUDE = r"""-- ngsdk-bats: {title} ({platform}) from a chosen {term}: NGSDK_LEVEL, from 1.
-- Written by tools/mame_bats.py for one build: the addresses below are that build's.
local cfg = {cfg}
local machine = manager.machine
local mem = machine.devices[":maincpu"].spaces["program"]
local want = (tonumber(os.getenv("NGSDK_LEVEL") or "") or 1) - 1
if want < 0 or want >= cfg.count then want = 0 end
NGSDK_KEEP = {{}}           -- frame callbacks and taps: a global, so they are never collected
local keep = NGSDK_KEEP
local frame, skipping = 0, false

local function say(text)
    print("[ngsdk] " .. text)
    machine:popmessage(text)
end

local function in_game()     -- BIOS_USER_MODE: 2 while a credit is being played
    return mem:read_u8(0x10FDAF) == 2
end

local button
for _, port in pairs(machine.ioport.ports) do
    if port.fields["P1 A"] then button = port.fields["P1 A"] end
end

-- Skipping runs unthrottled and taps A through cards and conversations.
local function skip(on)
    if on == skipping then return end
    skipping = on
    machine.video.throttled = not on
    if not on and button then button:clear_value() end
end

local function every_frame(fn)
    local wrapped = function()
        frame = frame + 1
        fn()
        if skipping and button then button:set_value((frame % 16 < 2) and 1 or 0) end
    end
    if emu.add_machine_frame_notifier then
        keep[#keep + 1] = emu.add_machine_frame_notifier(wrapped)
    else
        emu.register_frame_done(wrapped)
    end
end

-- fn(value) runs when the CPU writes the byte at addr; a number it returns is written instead.
local function on_byte_write(addr, fn)
    local word = addr - addr % 2
    local shift = (addr % 2 == 0) and 8 or 0
    keep[#keep + 1] = mem:install_write_tap(word, word + 1, "ngsdk-level", function(offset, data, mask)
        if ((mask >> shift) & 0xff) ~= 0xff then return end
        local r = fn((data >> shift) & 0xff)
        if r then return (data & ~(0xff << shift)) | ((r & 0xff) << shift) end
    end)
end

local function write(addr, size, value)
    if size == 2 then mem:write_u16(addr, value) else mem:write_u8(addr, value) end
end

if want == 0 then
    say("{title}: from the start")
    return
end
say(string.format("{title}: %s %d", "{term}", want + 1))
"""

LUA = {
    # A run begins with sky_run() clearing the sector: it is given the chosen one instead.
    "skylance": r"""
on_byte_write(cfg.s_stage, function(v)
    if v == 0 then
        say(string.format("Sky Lance: sector %d", want + 1))
        return want
    end
end)
""",
    # Mission 1 under way: the interlude is entered with the chosen mission next,
    # and its timer ends at once, so the game builds that mission itself.
    "maiya": r"""
local done = false
every_frame(function()
    if not in_game() then done = false return end
    if done then return end
    local mg = cfg.mg
    if mem:read_u8(mg + cfg.off_demo) ~= 0 or mem:read_u8(mg + cfg.off_stage) ~= 0 then return end
    if mem:read_u8(mg + cfg.off_state) ~= cfg.st_play then return end
    mem:write_u8(mg + cfg.off_next_stage, want)
    write(mg + cfg.off_state_timer, cfg.size_state_timer, 1)
    write(mg + cfg.off_state, cfg.size_state, cfg.st_interlude)
    done = true
    say(string.format("Maiya: mission %d", want + 1))
end)
""",
    # Each story stage before the chosen one is cleared as soon as it is set up:
    # the engine's own stage-clear event goes into its queue. A stage is told
    # by its waves, which stage_for() copies into s_stage at its start.
    "nosignal": r"""
local stage_of = {}
for i = 0, cfg.count - 1 do
    local st = mem:read_u32(cfg.ns_stages + i * cfg.info_size + cfg.off_info_stage)
    stage_of[mem:read_u32(st + cfg.off_waves)] = i
end
local fresh, reached, injected = false, false, false
on_byte_write(cfg.s_stage + cfg.off_nwaves, function()
    if in_game() and mem:read_u8(cfg.s_mode) == cfg.mode_story then fresh, injected = true, false end
end)
every_frame(function()
    if not in_game() then fresh, reached = false, false skip(false) return end
    if reached or not fresh then return end
    local stage = stage_of[mem:read_u32(cfg.s_stage + cfg.off_waves)]
    if stage == nil then return end
    if stage >= want then
        reached = true
        skip(false)
        say(string.format("No Signal: stage %d", stage + 1))
        return
    end
    skip(true)
    local b = cfg.s_b
    if injected or mem:read_u8(b + cfg.off_status) ~= cfg.playing then return end
    local head, count = mem:read_u8(b + cfg.off_ev_head), mem:read_u8(b + cfg.off_ev_count)
    if count >= cfg.queue then return end
    local e = b + cfg.off_ev + ((head + count) % cfg.queue) * cfg.ev_size
    for k = 0, cfg.ev_size - 1 do mem:write_u8(e + k, 0) end
    mem:write_u8(e + cfg.off_ev_type, cfg.ev_stage_clear)
    mem:write_u8(b + cfg.off_ev_count, count + 1)
    injected = true
end)
""",
    # Each level before the chosen one is won the moment its board is ready:
    # the engine's state says so, and its own WON event goes into its queue
    # (some builds end a level on the event alone). The board was never
    # played, so the result screen records nothing.
    "sevenwonders": r"""
local reached = false
every_frame(function()
    if not in_game() then reached = false skip(false) return end
    if reached then return end
    local p = cfg.s_p
    if mem:read_u8(p + cfg.off_state) ~= cfg.st_idle then return end
    local at = mem:read_u32(p + cfg.off_level) - cfg.sw_levels
    if at < 0 or at >= cfg.count * cfg.level_size then return end
    local level = at // cfg.level_size
    if level >= want then
        reached = true
        skip(false)
        say(string.format("Seven Wonders: level %d", level + 1))
        return
    end
    local head, count = mem:read_u8(p + cfg.off_ev_head), mem:read_u8(p + cfg.off_ev_count)
    if count >= cfg.queue then return end
    local e = p + cfg.off_ev + ((head + count) % cfg.queue) * cfg.ev_size
    for k = 0, cfg.ev_size - 1 do mem:write_u8(e + k, 0) end
    mem:write_u8(e + cfg.off_ev_type, cfg.ev_won)
    mem:write_u8(e + cfg.off_ev_cell, cfg.no_cell)
    mem:write_u8(e + cfg.off_ev_a, cfg.end_objectives)
    mem:write_u8(p + cfg.off_ev_count, count + 1)
    mem:write_u8(p + cfg.off_state, cfg.st_won)
    skip(true)
end)
""",
}


def lua_script(game, platform, info):
    cfg = dict(info["symbols"])
    cfg.update(info["consts"])
    table = "{" + ", ".join(f"{k} = {v}" for k, v in sorted(cfg.items())) + "}"
    term = GAMES[game]["levels"]
    return PRELUDE.format(title=GAMES[game]["title"], platform=platform.upper(), term=term, cfg=table) + LUA[game]


def one_liner(call, note=""):
    lines = ["@echo off", f"rem {MARK}: written by tools/mame_bats.py." + (f" {note}" if note else "")]
    return "\n".join(lines + [call]) + "\n"


def installed(dest):
    """game -> {mode: (data dir, level info or None)} for what tests\\ holds."""
    found = {}
    for game in GAMES:
        for mode in MODES:
            data = data_dir(dest, game, mode)
            if data is None:
                continue
            info = None
            lj = data / "levels.json"
            if game in PROBES and lj.is_file():
                info = json.loads(lj.read_text())
                p1 = next((data / "roms" / game).glob("*-p1.p1"))
                if hashlib.sha256(p1.read_bytes()).hexdigest() != info.get("p1_sha256"):
                    print(f"mame_bats: {game} {mode}: levels.json is for another build, run --describe; no level launchers")
                    info = None
            found.setdefault(game, {})[mode] = (data, info)
    return found


def esc(text):
    """Text to echo in MENU.bat, which runs with delayed expansion on."""
    for a, b in (("^", "^^"), ("&", "^&"), ("|", "^|"), ("<", "^<"), (">", "^>"), ("%", "%%"), ("!", "^^!")):
        text = text.replace(a, b)
    return text


def menu(found):
    out = [r"""@echo off
rem ngsdk-bats: written by tools/mame_bats.py (NeoGeoSDK); change the tool, not this file.
rem The menu of every launcher in this folder: games, levels, video steps, settings.
setlocal EnableExtensions EnableDelayedExpansion
set "HERE=%~dp0"
set "HERE=%HERE:~0,-1%"
set "NGSDK_MENU=1"
if not exist "%HERE%\settings\" mkdir "%HERE%\settings"

:main
call :settings
cls
echo.
echo   NEOGEOSDK GAMES IN MAME
echo   =======================
echo.
"""]
    keys = "1234567"
    order = [g for g in GAMES if g in found]
    for k, game in zip(keys, order):
        out.append(f"echo     {k}  {esc(GAMES[game]['title'])}\n")
    out.append(r"""echo.
echo     F  Picture: !FILTER!      (sharp, crt or smooth)
echo     V  Videos go to: !VIDEODIR!
echo     Q  Quit
echo.
""")
    out.append(f'choice /c {keys[:len(order)]}FVQ /n /m "  Choose: "\n')
    n = len(order)
    out.append(f"if errorlevel {n + 3} exit /b 0\n")
    out.append(f"if errorlevel {n + 2} goto set_videodir\n")
    out.append(f"if errorlevel {n + 1} goto set_filter\n")
    for i in range(n, 0, -1):
        out.append(f"if errorlevel {i} goto game_{order[i - 1]}\n")
    out.append("goto main\n\n")

    for game in order:
        modes = found[game]
        title = esc(GAMES[game]["title"])
        term = GAMES[game].get("levels")
        has_levels = term and any(info for _, info in modes.values())
        out.append(f":game_{game}\ncls\necho.\necho   {title.upper()}\necho.\n")
        opts = []
        for mode in MODES:
            if mode in modes:
                opts.append((f"Play {mode.upper()}" + (" (arcade)" if mode == "mvs" else " (console)"),
                             f'call "%HERE%\\run_{game}_{mode}.bat"'))
        for mode in MODES:
            if mode in modes:
                opts.append((f"Play {mode.upper()} with the CRT picture", f'call "%HERE%\\run_{game}_{mode}_scanlines.bat"'))
        if has_levels:
            opts.append((f"Start at a {term}...", f"goto levels_{game}"))
        opts.append(("Make a video (3 steps)...", f"goto video_{game}"))
        for mode in MODES:
            if mode in modes:
                opts.append((f"Play {mode.upper()} with UniBIOS 4.0", f'call "%HERE%\\run_{game}_{mode}_unibios.bat"'))
        choice_keys = "123456789"[:len(opts)]
        for k, (label, _) in zip(choice_keys, opts):
            out.append(f"echo     {k}  {label}\n")
        out.append("echo     B  Back\necho.\n")
        out.append(f'choice /c {choice_keys}B /n /m "  Choose: "\n')
        out.append(f"if errorlevel {len(opts) + 1} goto main\n")
        for i in range(len(opts), 0, -1):
            out.append(f"if errorlevel {i} goto g_{game}_{i}\n")
        out.append(f"goto game_{game}\n")
        for i, (_, action) in enumerate(opts, 1):
            back = "" if action.startswith("goto") else f"\ngoto game_{game}"
            out.append(f":g_{game}_{i}\n{action}{back}\n")
        out.append("\n")

        if has_levels:
            out.append(f":levels_{game}\ncls\necho.\necho   {title.upper()}: START AT A {term.upper()}\necho.\n")
            out.append(f"call :pick_mode {game}\nif \"!PICKED!\"==\"\" goto game_{game}\n")
            for mode in MODES:
                if mode in modes and modes[mode][1]:
                    out.append(f'if "!PICKED!"=="{mode}" goto levels_{game}_{mode}\n')
            out.append(f"echo   No level launchers for that one.\npause\ngoto game_{game}\n")
            for mode in MODES:
                if mode not in modes or not modes[mode][1]:
                    continue
                names = modes[mode][1]["names"]
                out.append(f":levels_{game}_{mode}\ncls\necho.\necho   {title.upper()} {mode.upper()}: START AT A {term.upper()}\necho.\n")
                for i, name in enumerate(names, 1):
                    out.append(f"echo    {i:2d}  {esc(name)}\n")
                out.append(f'echo.\nset "L="\nset /p "L=  {term.title()} number (Enter: back): "\n')
                out.append(f'if "!L!"=="" goto game_{game}\n')
                out.append(f'set /a "LN=!L!" 2>nul\nif !LN! lss 1 goto levels_{game}_{mode}\nif !LN! gtr {len(names)} goto levels_{game}_{mode}\n')
                out.append(f'call "%HERE%\\bin\\ngsdk.bat" level {game} {mode} !LN!\ngoto game_{game}\n')
            out.append("\n")

        out.append(f""":video_{game}
cls
echo.
echo   {title.upper()}: MAKE A VIDEO
echo.
echo     Step 1 records your inputs while you play. Step 2 replays them into
echo     a raw AVI in !VIDEODIR!. Step 3 makes the 1080p MP4 from it.
echo.
echo     1  Step 1: record a game from the start
""")
        vopts = [f"goto vrec_{game}"]
        if has_levels:
            out.append(f"echo     2  Step 1: record a game from a {term}\n")
            vopts.append(f"goto vrecl_{game}")
        k = len(vopts) + 1
        out.append(f"echo     {k}  Step 2: make the AVI of a recording\n")
        out.append(f"echo     {k + 1}  Step 3: make the MP4 of an AVI\n")
        out.append(f"echo     {k + 2}  Steps 2 and 3 for a recording\n")
        vopts += [f"set \"NEXT=avi\" & goto vpick_{game}", f"set \"NEXT=mp4\" & goto vpick_{game}",
                  f"set \"NEXT=both\" & goto vpick_{game}"]
        ck = "123456789"[:len(vopts)]
        out.append("echo     B  Back\necho.\n")
        out.append(f'choice /c {ck}B /n /m "  Choose: "\n')
        out.append(f"if errorlevel {len(vopts) + 1} goto game_{game}\n")
        for i in range(len(vopts), 0, -1):
            out.append(f"if errorlevel {i} goto v_{game}_{i}\n")
        out.append(f"goto video_{game}\n")
        for i, action in enumerate(vopts, 1):
            out.append(f":v_{game}_{i}\n{action}\n")
        out.append(f""":vrec_{game}
call :pick_mode {game}
if "!PICKED!"=="" goto video_{game}
call "%HERE%\\bin\\ngsdk.bat" record {game} !PICKED!
goto video_{game}
""")
        if has_levels:
            out.append(f""":vrecl_{game}
call :pick_mode {game}
if "!PICKED!"=="" goto video_{game}
set "L="
set /p "L=  {term.title()} number: "
if "!L!"=="" goto video_{game}
call "%HERE%\\bin\\ngsdk.bat" record {game} !PICKED! !L!
goto video_{game}
""")
        out.append(f""":vpick_{game}
call :pick_recording {game}
if "!REC!"=="" goto video_{game}
if not "!NEXT!"=="mp4" call "%HERE%\\bin\\ngsdk.bat" avi !REC!
if not "!NEXT!"=="avi" call "%HERE%\\bin\\ngsdk.bat" mp4 !REC!
goto video_{game}

""")

    out.append(r""":set_filter
cls
echo.
echo   PICTURE (play, levels and video step 1; the AVI always keeps the raw pixels)
echo.
echo     1  sharp   crisp, even pixels (bilinear over a 4x picture)
echo     2  crt     arcade monitor: scanlines, shadow mask, curvature
echo     3  smooth  super-xBR: rounded, painted edges
echo     4  sharper on a 1440p or 4K screen (sharp, 6x)
echo     B  Back
echo.
choice /c 1234B /n /m "  Choose: "
if errorlevel 5 goto main
if errorlevel 4 (>"%HERE%\settings\filter.txt" echo sharp& >"%HERE%\settings\prescale.txt" echo 6& goto main)
if errorlevel 3 (>"%HERE%\settings\filter.txt" echo smooth& goto main)
if errorlevel 2 (>"%HERE%\settings\filter.txt" echo crt& goto main)
>"%HERE%\settings\filter.txt" echo sharp
>"%HERE%\settings\prescale.txt" echo 4
goto main

:set_videodir
echo.
echo   Videos now go to !VIDEODIR!
set "D="
set /p "D=  New folder (Enter: keep it): "
if "!D!"=="" goto main
>"%HERE%\settings\video_dir.txt" echo !D!
goto main

:settings
set "FILTER=sharp"
if exist "%HERE%\settings\filter.txt" set /p FILTER=<"%HERE%\settings\filter.txt"
set "VIDEODIR="
if exist "%HERE%\settings\video_dir.txt" set /p VIDEODIR=<"%HERE%\settings\video_dir.txt"
if "!VIDEODIR!"=="" if exist "E:\" set "VIDEODIR=E:\NeoGeoVideos"
if "!VIDEODIR!"=="" set "VIDEODIR=%HERE%\video"
exit /b 0

:pick_mode
set "PICKED="
set "HAS_MVS="
set "HAS_AES="
if exist "%HERE%\run_%~1_mvs.bat" set "HAS_MVS=1"
if exist "%HERE%\run_%~1_aes.bat" set "HAS_AES=1"
if defined HAS_MVS if not defined HAS_AES (set "PICKED=mvs" & exit /b 0)
if defined HAS_AES if not defined HAS_MVS (set "PICKED=aes" & exit /b 0)
echo.
choice /c MAB /n /m "  M: MVS (arcade)   A: AES (console)   B: back  "
if errorlevel 3 exit /b 0
if errorlevel 2 (set "PICKED=aes" & exit /b 0)
set "PICKED=mvs"
exit /b 0

:pick_recording
set "REC="
set /a N=0
echo.
for %%F in ("%HERE%\inp\%~1_*.inp") do (
    set /a N+=1
    set "R_!N!=%%~nF"
    echo     !N!  %%~nF   ^(%%~tF^)
)
if !N!==0 (echo   No recordings of this game yet: run step 1 first. & pause & exit /b 0)
echo.
set "P="
set /p "P=  Recording number (Enter: back): "
if "!P!"=="" exit /b 0
set "REC=!R_%P%!"
exit /b 0
""")
    return "".join(out)


README = r"""NeoGeoSDK games in MAME
=======================

Start MENU.bat: every game, its levels, the video steps and the settings are
in it. Every launcher below can also be started on its own.

  run_<game>_mvs.bat, run_<game>_aes.bat   play (MVS arcade, AES console)
  run_<game>_<mode>_scanlines.bat          play with the CRT picture
  run_<game>_<mode>_unibios.bat            play with UniBIOS 4.0 instead of EagleBIOS
  levels\<game>\<mode>_<level>_NN.bat      begin a game at that level
  1_record_<game>_<mode>_inp.bat           video step 1: play while MAME records
  2_export_<game>_<mode>_avi.bat           video step 2: the recording to an AVI
  3_save_<game>_<mode>_mp4.bat             video step 3: the AVI to a 1080p MP4

The picture: sharp (default) is bilinear filtering over a 4x nearest-neighbour
picture, so pixels stay crisp and even; crt is the crt-geom-deluxe shader;
smooth is super-xBR. The menu's F changes it (settings\filter.txt).

Videos: step 1 keeps the game's settings, saves and memory card as they were
when it began (inp\<name>\), so step 2's replay starts from the same state.
Step 2 writes the game's own pixels (320x224) to <video folder>\<name>.avi, by
default E:\NeoGeoVideos. Step 3 scales them to 1440x1080 (nearest neighbour)
in a 1920x1080 frame, H.264 CRF 16 and AAC 192k: <name>.mp4, with the first
FFmpeg that has libx264 (settings\ffmpeg-path.txt, the PATH, C:\ffmpeg\bin).

Step 2 writes about 780 MB of AVI per minute of play; on a USB hard drive it
can take longer than the recording itself. A recording replays only on the
build it was made with: rebuilding the game ends it. Step 1 asks before it
records over a recording of the same name. Recordings made by the earlier
launchers (inp\<game>_<mode>.inp, no start state) still go through step 2:
they replay from a copy of the current settings and saves.

Levels: the level launchers skip the levels before the chosen one at full
speed (or, for Maiya, go straight to the mission). They use the installed
build's addresses (levels.json next to its ROMs) and are written again by
tools/mame_bats.py whenever a build is installed.

Each game's ROMs, settings and saves are in tests\<game>\<mvs|aes>\ (Maiya's
in tests\mvs and tests\aes), EagleBIOS in tests\<game>\bios\, the AES memory
card in tests\aes\memcard.bin. UniBIOS (menu only) is read from roms\neogeo
and roms\aes.
"""


def write_all(dest):
    dest = Path(dest)
    found = installed(dest)
    if not found:
        raise SystemExit(f"mame_bats: no games installed under {dest / 'tests'}")
    owned_file = dest / "bin" / "owned.txt"
    before = set(owned_file.read_text().split("\n")) - {""} if owned_file.is_file() else set()
    written = []

    def put(rel, text):
        crlf(dest / rel, text)
        written.append(rel)

    put("bin/ngsdk.bat", ENGINE)
    blank = dest / "bin" / "memcard-blank.bin"
    if not blank.is_file():
        blank.write_bytes(bytes(2048))
    for game, modes in found.items():
        put(f"run_{game}.bat", one_liner(f'call "%~dp0bin\\ngsdk.bat" play {game} %*', "run_<game>.bat [mvs|aes] [sharp|crt|smooth] [unibios]"))
        for mode, (data, info) in modes.items():
            put(f"run_{game}_{mode}.bat", one_liner(f'call "%~dp0bin\\ngsdk.bat" play {game} {mode} %*'))
            put(f"run_{game}_{mode}_scanlines.bat", one_liner(f'call "%~dp0bin\\ngsdk.bat" play {game} {mode} crt'))
            put(f"run_{game}_{mode}_unibios.bat", one_liner(f'call "%~dp0bin\\ngsdk.bat" play {game} {mode} %* unibios'))
            put(f"1_record_{game}_{mode}_inp.bat", one_liner(f'call "%~dp0bin\\ngsdk.bat" record {game} {mode} %*', "Video step 1."))
            put(f"2_export_{game}_{mode}_avi.bat", one_liner(f'call "%~dp0bin\\ngsdk.bat" avi {game}_{mode}', "Video step 2."))
            put(f"3_save_{game}_{mode}_mp4.bat", one_liner(f'call "%~dp0bin\\ngsdk.bat" mp4 {game}_{mode}', "Video step 3."))
            if info:
                put(f"levels/{game}_{mode}.lua", lua_script(game, mode, info))
                term = GAMES[game]["levels"]
                for i, name in enumerate(info["names"], 1):
                    slug = re.sub(r"[^a-z0-9]+", "_", name.lower()).strip("_")
                    put(f"levels/{game}/{mode}_{term}_{i:02d}_{slug}.bat",
                        one_liner(f'call "%~dp0..\\..\\bin\\ngsdk.bat" level {game} {mode} {i} %*', f"{GAMES[game]['title']}: {name}."))
    put("MENU.bat", menu(found))
    put("README-LAUNCHERS.txt", README)
    for stale in sorted(before - set(written)):
        p = dest / stale
        if p.is_file() and MARK in p.read_text(errors="replace")[:400]:
            p.unlink()
    for d in sorted((dest / "levels").glob("*"), reverse=True):
        if d.is_dir() and not any(d.iterdir()):
            d.rmdir()
    crlf(owned_file, "\n".join(sorted(written)) + "\n")
    (dest / "settings").mkdir(exist_ok=True)
    levels = sum(len(m[1]["names"]) for g in found.values() for m in g.values() if m[1])
    print(f"mame_bats: {len(written)} files in {dest} ({len(found)} games, {levels} level launchers)")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dest", required=True, type=Path, help="the MAME test folder (C:\\mame\\neogeosdk is /mnt/c/mame/neogeosdk)")
    ap.add_argument("--describe", nargs=3, metavar=("GAME", "PLATFORM", "ELF"),
                    help="record the level launchers' addresses of an installed build")
    ap.add_argument("--src", type=Path, default=ROOT, help="the source tree the build was made from (default: this one)")
    ap.add_argument("--no-write", action="store_true", help="only --describe")
    args = ap.parse_args(argv)
    if args.describe:
        game, platform, elf = args.describe
        describe(args.dest, game, platform, Path(elf), args.src.resolve())
    if not args.no_write:
        write_all(args.dest)
    return 0


if __name__ == "__main__":
    sys.exit(main())
