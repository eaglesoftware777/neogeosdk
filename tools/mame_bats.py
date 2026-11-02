#!/usr/bin/env python3
"""
Write the Windows launchers of a MAME test folder (C:\\mame\\neogeosdk): play,
start at a level, the three video steps and MENU.bat, for every game
installed under its tests\\ folder, and its Neo Geo CD disc when it has one.

    python3 tools/mame_bats.py --dest /mnt/c/mame/neogeosdk
    python3 tools/mame_bats.py --dest /mnt/c/mame/neogeosdk --describe nosignal mvs out/game
    python3 tools/mame_bats.py --dest /mnt/c/mame/neogeosdk \\
        --cd maiya build/neocd/maiya/bincue/maiya.cue --cd-bios eagle_cd_cdz.rom

The launchers all call bin\\ngsdk.bat, so each one stays a single line:

    run_<game>_<mvs|aes>.bat             play (run_<game>.bat [mvs|aes] [sharp|crt|smooth])
    run_<game>_<mvs|aes>_scanlines.bat   play with the CRT shader
    run_<game>_<mvs|aes>_unibios.bat     play with UniBIOS 4.0
    1_record_<game>_<mode>_inp.bat       video step 1: play while MAME records the inputs
    2_export_<game>_<mode>_avi.bat       video step 2: replay them into a raw AVI
    3_save_<game>_<mode>_mp4.bat         video step 3: the AVI to a 1080p MP4 for YouTube
    levels\\<game>\\<mode>_<level>_NN.bat  start a game at that level
    run_<game>_cd_eagle.bat              the Neo Geo CD disc with EagleBIOS-CD
    run_<game>_cd_stock.bat              the same disc with your own stock CDZ firmware
    live\\live_<game>_<mvs|aes|cd_eagle|cd_stock>.bat
                                         play with the live log (live\\README-LIVE.txt)

--cd GAME CUE copies a disc (the .cue and the files it names, best the
single BIN/CUE of tools/neocd/bincue.py) to tests\\<game>\\cd\\<game>.cue.
--cd-bios takes EagleBIOS-CD as built (eagle_cd_cdz.rom, CPU byte order) and
puts it in MAME's order, with the SDK's own LO data, in
tests\\cd-bios\\eagle\\neocdz. The stock launchers read neocdz from MAME's own
roms folder; no stock firmware is copied or supplied.

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
rem   ngsdk.bat cd     GAME eagle^|stock [sharp^|crt^|smooth]  the game's Neo Geo CD disc
rem   ngsdk.bat live   GAME mvs^|aes^|cd-eagle^|cd-stock   play with the live log
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
if /i "%~1"=="cd" goto cd
if /i "%~1"=="live" goto live
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

rem -------------------------------------------------------------------- cd
rem The disc (tests\GAME\cd\GAME.cue) on a CDZ: eagle is EagleBIOS-CD from
rem tests\cd-bios\eagle, stock is the player's own neocdz in MAME's roms folder.
rem Each firmware keeps its own settings and saves.
:cd
call :cd_setup "%~2" "%~3" || goto failed
call :display "%~4"
call :cd_args
call :title "%GAME% Neo Geo CD, %FILTER% picture" "%BIOS%"
pushd "%MAMEDIR%"
"%MAME_EXE%" %ARGS% %MDISPLAY% %NGSDK_MAME_ARGS%
set "RC=%ERRORLEVEL%"
popd
goto done

rem ------------------------------------------------------------------ live
rem Play while live\ngsdk_live.lua writes live\sessions\<time>_<game>_<mode>\:
rem live.log (events, also added to live\events.log), stats.csv (the game's
rem frame rate every second), snap\, state\ (a save state at every M mark),
rem and session.inp with start\ (the settings and saves it began from), so
rem the session can be replayed exactly.
:live
set "KIND=%~3"
if /i "%KIND%"=="cd-eagle" goto live_cd
if /i "%KIND%"=="cd-stock" goto live_cd
call :setup "%~2" "%~3" "" || goto failed
call :args
set "LIVEMODE=%MODE%"
set "WATCH=%HERE%\live\watch\%GAME%_%MODE%.lua"
call :p1hash
goto live_go
:live_cd
call :cd_setup "%~2" "%KIND:~3%" || goto failed
call :cd_args
set "LIVEMODE=cd_%PROFILE%"
set "WATCH="
set "P1HASH="
:live_go
call :stamp
set "SESSION=%HERE%\live\sessions\%STAMP%_%GAME%_%LIVEMODE%"
mkdir "%SESSION%\start\cfg" "%SESSION%\start\nvram" "%SESSION%\snap" "%SESSION%\state"
if exist "%CFG%\" xcopy /e /i /q /y "%CFG%" "%SESSION%\start\cfg" >nul
if exist "%NVRAM%\" xcopy /e /i /q /y "%NVRAM%" "%SESSION%\start\nvram" >nul
if /i "%MODE%"=="aes" copy /y "%CARD%" "%SESSION%\start\memcard.bin" >nul
>"%SESSION%\session.txt" echo game=%GAME%
>>"%SESSION%\session.txt" echo mode=%LIVEMODE%
>>"%SESSION%\session.txt" echo p1=%P1HASH%
>>"%SESSION%\session.txt" echo disc=%DISC%
>>"%SESSION%\session.txt" echo date=%DATE% %TIME%
>>"%SESSION%\session.txt" echo args=%ARGS%
set "NGSDK_LIVE_DIR=%SESSION%"
set "NGSDK_LIVE_GAME=%GAME%"
set "NGSDK_LIVE_MODE=%LIVEMODE%"
set "NGSDK_LIVE_EVENTS=%HERE%\live\events.log"
set "NGSDK_LIVE_WATCH="
if defined WATCH if exist "%WATCH%" set "NGSDK_LIVE_WATCH=%WATCH%"
call :display ""
call :title "LIVE TEST: %GAME% %LIVEMODE%, %FILTER% picture" "%BIOS%"
echo   Play as usual. Press M whenever something is wrong: a mark saves a
echo   snapshot and a save state, and goes into the log with the time.
echo   The session goes to %SESSION%
pushd "%MAMEDIR%"
"%MAME_EXE%" %ARGS% %MDISPLAY% -snapshot_directory "%SESSION%\snap" -snapname %%i -state_directory "%SESSION%\state" -input_directory "%SESSION%" -record session.inp -autoboot_script "%HERE%\live\ngsdk_live.lua" %NGSDK_MAME_ARGS%
set "RC=%ERRORLEVEL%"
popd
echo.
echo   Session saved: %SESSION%
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

:cd_setup
set "GAME=%~1"
set "PROFILE=%~2"
if /i not "%PROFILE%"=="stock" set "PROFILE=eagle"
set "MODE=cd"
set "P1="
set "DISC=%HERE%\tests\%GAME%\cd\%GAME%.cue"
if not exist "%DISC%" goto cd_no_disc
set "DATA=%HERE%\tests\%GAME%\cd\%PROFILE%"
set "ROMS=%HERE%\tests\cd-bios\eagle"
set "BIOS=EagleBIOS-CD"
if /i "%PROFILE%"=="stock" set "ROMS=%MAMEDIR%\roms"
if /i "%PROFILE%"=="stock" set "BIOS=your own stock CDZ firmware, %MAMEDIR%\roms\neocdz"
if not exist "%ROMS%\neocdz\neocd.bin" goto cd_no_bios
if not exist "%ROMS%\neocdz\000-lo.lo" goto cd_no_bios
set "CFG=%DATA%\cfg"
set "NVRAM=%DATA%\nvram"
for %%D in (cfg nvram state) do if not exist "%DATA%\%%D\" mkdir "%DATA%\%%D"
exit /b 0

:cd_no_disc
echo.
echo   No Neo Geo CD disc for %GAME%: %DISC% is missing.
exit /b 1

:cd_no_bios
echo.
echo   The %PROFILE% CD firmware is missing: %ROMS%\neocdz needs neocd.bin and 000-lo.lo.
if /i "%PROFILE%"=="stock" echo   Put your own Neo Geo CDZ system ROM files there; none is supplied.
exit /b 1

:cd_args
set ARGS=neocdz -bios official -noreadconfig -skip_gameinfo -rompath "%ROMS%" -cdrom "%DISC%" -cfg_directory "%CFG%" -nvram_directory "%NVRAM%" -state_directory "%DATA%\state" -snapshot_directory "%SNAPDIR%" -noautosave
exit /b 0

:stamp
set "STAMP="
for /f "delims=" %%T in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss"') do set "STAMP=%%T"
if not defined STAMP set "STAMP=session%RANDOM%"
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


def menu(found, cds=()):
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
        if game in cds:
            opts.append(("Play the Neo Geo CD disc (EagleBIOS-CD)", f'call "%HERE%\\run_{game}_cd_eagle.bat"'))
            opts.append(("Play the Neo Geo CD disc (your stock CDZ firmware)", f'call "%HERE%\\run_{game}_cd_stock.bat"'))
        for mode in MODES:
            if mode in modes:
                opts.append((f"Play {mode.upper()} with the CRT picture", f'call "%HERE%\\run_{game}_{mode}_scanlines.bat"'))
        if has_levels:
            opts.append((f"Start at a {term}...", f"goto levels_{game}"))
        opts.append(("Make a video (3 steps)...", f"goto video_{game}"))
        for mode in MODES:
            if mode in modes:
                opts.append((f"Play {mode.upper()} with UniBIOS 4.0", f'call "%HERE%\\run_{game}_{mode}_unibios.bat"'))
        choice_keys = "123456789CDEFG"[:len(opts)]     # B is Back
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
  run_<game>_cd_eagle.bat                  the Neo Geo CD disc with EagleBIOS-CD
  run_<game>_cd_stock.bat                  the same disc with your own stock CDZ firmware
  live\live_<game>_<mode>.bat              play with a live log for debugging (live\README-LIVE.txt)
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

Neo Geo CD: a game's disc is tests\<game>\cd\<game>.cue with one .bin (the
same BIN/CUE an SD loader or a CD burner takes), its settings and saves in
tests\<game>\cd\eagle and tests\<game>\cd\stock. EagleBIOS-CD and the SDK's
own LO data are in tests\cd-bios\eagle\neocdz. The stock launchers read
neocd.bin and 000-lo.lo from MAME's own roms\neocdz folder: supply your own,
none is included. Loading takes about 30 seconds of disc reads.
"""


LIVE_LUA = r"""-- ngsdk-bats: the live log of a test session, written by tools/mame_bats.py.
-- live\sessions\<session>\ gets live.log (events), stats.csv (one row a
-- second), snap\ and state\ (snapshot and save state at every M mark); every
-- event also goes to live\events.log, one file to follow during play.
local machine = manager.machine
local mem = machine.devices[":maincpu"].spaces["program"]
local dir = os.getenv("NGSDK_LIVE_DIR") or "."
local game = os.getenv("NGSDK_LIVE_GAME") or "game"
local mode = os.getenv("NGSDK_LIVE_MODE") or "?"
local FRAMES = 0x100020     -- waitVbl() counts the game's frames here (sdk/neogeolib.c)
local USER_MODE = 0x10FDAF  -- BIOS_USER_MODE: 0 start-up, 1 title/attract, 2 a game being played
local MODES = {[0] = "start-up", [1] = "title/attract", [2] = "game"}
NGSDK_LIVE = {}             -- the notifier subscriptions: a global, so they are never collected

local log = io.open(dir .. "\\live.log", "a")
local all = os.getenv("NGSDK_LIVE_EVENTS") and io.open(os.getenv("NGSDK_LIVE_EVENTS"), "a")
local csv = io.open(dir .. "\\stats.csv", "a")
csv:write("wall,t,game_fps,user_mode,stage,snapshot\n")

local watch
if os.getenv("NGSDK_LIVE_WATCH") then
    local chunk = loadfile(os.getenv("NGSDK_LIVE_WATCH"))
    local ok, w = pcall(chunk or error)
    if ok then watch = w end
end

local function clock()
    local s = math.floor(machine.time:as_double())
    return string.format("%s %3d:%02d", os.date("%H:%M:%S"), s // 60, s % 60)
end

local function event(kind, text)
    local line = string.format("%s %-6s %s", clock(), kind, text)
    if log then log:write(line, "\n") log:flush() end
    if all then all:write(game, " ", mode, " ", line, "\n") all:flush() end
end

local stage, stage_text = nil, "-"
local function read_stage()
    if not watch then return nil end
    local ok, s = pcall(watch.stage, mem)
    if ok then return s end
end

local shots = 0
local function snapshot()
    machine.video:snapshot()
    shots = shots + 1
    return string.format("snap\\%04d.png", shots - 1)
end

local vbl, last, sec_frames = 0, nil, nil
local seen, stall, stalled, stalls = false, 0, false, 0
local umode, fps, hist, norm = nil, 0, {}, nil
local slow, slow_min, slow_sum, slow_total, slow_shot, slow_first = 0, 999, 0, 0, -1000, 0
local played, marks, key_down, next_shot = 0, 0, false, 60
local key = machine.input:code_from_token("KEYCODE_M")
local test_mark = tonumber(os.getenv("NGSDK_LIVE_TEST_MARK") or "")

local function mark()
    marks = marks + 1
    local shot = snapshot()
    machine:save("mark_" .. marks)
    event("MARK", string.format("#%d  %s  stage %s, %d fps last second  (%s, state mark_%d)",
        marks, MODES[umode] or tostring(umode), stage_text, fps, shot, marks))
    machine:popmessage(string.format("Mark %d saved", marks))
end

local function second()
    local f = mem:read_u32(FRAMES)
    fps = sec_frames and ((f - sec_frames) & 0xFFFFFFFF) or 0
    if fps > 1000 then fps = 0 end
    sec_frames = f
    local shot = ""
    if umode == 2 then
        played = played + 1
        if played >= next_shot then shot = snapshot() next_shot = played + 60 end
    end
    csv:write(string.format("%s,%d,%d,%d,%s,%s\n", os.date("%H:%M:%S"),
        math.floor(machine.time:as_double()), fps, umode or -1, stage_text, shot))
    csv:flush()
    if not seen or fps == 0 then return end
    hist[fps] = (hist[fps] or 0) + 1
    if not norm or hist[fps] > hist[norm] then norm = fps end
    -- one slow second (a scene change, a load) is a DIP; two or more in a row
    -- are a SLOW stretch, with a snapshot at its start
    if norm and fps <= norm - 3 then
        slow, slow_sum, slow_total = slow + 1, slow_sum + fps, slow_total + 1
        if fps < slow_min then slow_min = fps end
        if slow == 1 then slow_first = fps end
        if slow == 2 then
            local where = ""
            if machine.time:as_double() - slow_shot > 15 then
                where = "  (" .. snapshot() .. ")"
                slow_shot = machine.time:as_double()
            end
            event("SLOW", string.format("%d, %d fps (normally %d)  %s, stage %s%s", slow_first, fps, norm,
                MODES[umode] or "?", stage_text, where))
        end
    elseif slow > 0 then
        if slow == 1 then
            event("DIP", string.format("%d fps for one second (normally %d)  %s, stage %s", slow_first, norm,
                MODES[umode] or "?", stage_text))
        else
            event("SLOW", string.format("over after %d s: lowest %d fps, average %d", slow, slow_min,
                slow_sum // slow))
        end
        slow, slow_min, slow_sum = 0, 999, 0
    end
end

local function frame()
    vbl = vbl + 1
    local um = mem:read_u8(USER_MODE)
    if um ~= umode then
        if umode == 2 then event("GAME", "the game ended: back to " .. (MODES[um] or tostring(um))) end
        if um == 2 then event("GAME", "a game started") end
        if um ~= 2 and umode ~= 2 then event("MODE", MODES[um] or tostring(um)) end
        umode = um
    end
    if um == 2 and watch then
        local s = read_stage()
        if s ~= stage then
            stage = s
            stage_text = s and ((watch.names and watch.names[s + 1]) and
                string.format("%d (%s)", s + 1, watch.names[s + 1]) or tostring(s + 1)) or "-"
            if s then event("STAGE", stage_text) end
        end
    end
    local f = mem:read_u32(FRAMES)
    if last and f ~= last then
        local step = (f - last) & 0xFFFFFFFF
        if step >= 1 and step <= 3 then seen = true end   -- counting, not RAM being set up
        if stalled then
            event("STALL", string.format("over: the game runs again after %.1f s", stall / 59.19))
            stalled = false
        end
        stall = 0
    elseif last then
        stall = stall + 1
        if seen and um ~= 0 and not stalled and stall == 120 then
            stalled, stalls = true, stalls + 1
            local shot = snapshot()
            machine:save("stall_" .. stalls)
            event("STALL", string.format("no game frame for 2 s: a hang, or a disc load on CD  %s, stage %s  (%s, state stall_%d)",
                MODES[um] or tostring(um), stage_text, shot, stalls))
        end
    end
    last = f
    if vbl % 60 == 0 then second() end
    local down = machine.input:code_pressed(key)
    if down and not key_down then mark() end
    key_down = down
    if test_mark and machine.time:as_double() >= test_mark then test_mark = nil mark() end
end

local resets = 0
event("START", string.format("%s %s, %s %s%s", game, mode, emu.app_name(), emu.app_version(),
    watch and "" or "  (no stage watch for this build)"))
NGSDK_LIVE[#NGSDK_LIVE + 1] = emu.add_machine_frame_notifier(frame)
NGSDK_LIVE[#NGSDK_LIVE + 1] = emu.add_machine_reset_notifier(function()
    resets = resets + 1
    if resets > 1 then event("RESET", "machine reset (F3, or the watchdog)") end
end)
NGSDK_LIVE[#NGSDK_LIVE + 1] = emu.add_machine_stop_notifier(function()
    if slow > 1 then event("SLOW", string.format("over after %d s: lowest %d fps", slow, slow_min)) end
    event("END", string.format("%d:%02d in games, %d slow seconds, %d stalls, %d marks%s",
        played // 60, played % 60, slow_total, stalls, marks,
        seen and "" or "  (this build never counted a game frame: no frame rates)"))
    if log then log:close() end
    if all then all:close() end
    csv:close()
end)
"""

# Where each game keeps its stage (from levels.json): fn(mem) -> 0-based stage or nil.
LIVE_STAGE = {
    "maiya": "function(mem) return mem:read_u8({mg} + {off_stage}) end",
    "skylance": "function(mem) return mem:read_u8({s_stage}) end",
    "sevenwonders": """function(mem)
        local at = mem:read_u32({s_p} + {off_level}) - {sw_levels}
        if at >= 0 and at < {count} * {level_size} then return at // {level_size} end
    end""",
    "nosignal": """function(mem)
        if not W.of then
            W.of = {{}}
            for i = 0, {count} - 1 do
                local st = mem:read_u32({ns_stages} + i * {info_size} + {off_info_stage})
                W.of[mem:read_u32(st + {off_waves})] = i
            end
        end
        return W.of[mem:read_u32({s_stage} + {off_waves})]
    end""",
}


def live_watch(game, info):
    """The live log's stage watch for one build, or None."""
    if not info or game not in LIVE_STAGE:
        return None
    values = dict(info["symbols"])
    values.update(info["consts"])
    names = ", ".join('"' + n.replace("\\", "\\\\").replace('"', '\\"') + '"' for n in info["names"])
    return (f"-- {MARK}: where {GAMES[game]['title']} keeps its stage, for this build "
            f"(P1 {info.get('p1_sha256', '')[:16]}).\n"
            f"W = {{names = {{{names}}}}}\n"
            f"W.stage = {LIVE_STAGE[game].format(**values)}\n"
            f"return W\n")


LIVE_README = r"""Live test sessions
==================

live_<game>_mvs.bat, live_<game>_aes.bat, live_<game>_cd_eagle.bat and
live_<game>_cd_stock.bat play a game as usual (your settings, saves and
controls) while ngsdk_live.lua keeps a log for debugging.

Press M on the keyboard whenever something looks or plays wrong: a mark
saves a snapshot and a save state and writes the time, the stage and the
frame rate to the log ("Mark N saved" shows on screen).

Each session is a folder in sessions\ (date, time, game, mode):

  live.log      what happened: game start/end, stages, marks, slowdowns
                (the game's own frame rate under its normal rate), stalls
                (no game frame for 2 s: a hang, or loading on CD), resets
  stats.csv     the game's frame rate every second, with mode and stage
  snap\         snapshots: marks, slowdowns, stalls, one a minute in play
  state\        save states at the marks and stalls
  session.inp   every input, and start\ the settings and saves it began
                from, so the session can be replayed exactly on this build

events.log here collects the events of every session in one file.
"""


def cd_installed(dest):
    """The games with a disc in tests\\<game>\\cd."""
    return [g for g in GAMES if (Path(dest) / "tests" / g / "cd" / f"{g}.cue").is_file()]


def install_cd(dest, game, cue):
    """Copy a disc's .cue (as <game>.cue) and the files it names into tests\\<game>\\cd."""
    cue = Path(cue)
    names = re.findall(r'^\s*FILE\s+"([^"]+)"', cue.read_text(), re.M)
    if not names or any("/" in n or "\\" in n for n in names):
        raise SystemExit(f"mame_bats: {cue}: want a cue whose files lie beside it (tools/neocd/bincue.py)")
    out = Path(dest) / "tests" / game / "cd"
    out.mkdir(parents=True, exist_ok=True)
    for name in names:
        data = (cue.parent / name).read_bytes()
        (out / name).write_bytes(data)
    (out / f"{game}.cue").write_bytes(cue.read_bytes())
    print(f"mame_bats: {game} disc -> {out} ({', '.join(names)})")


def install_cd_bios(dest, rom):
    """EagleBIOS-CD (CPU byte order) to MAME's word order, with the SDK's LO data."""
    data = Path(rom).read_bytes()
    if len(data) != 0x80000:
        raise SystemExit(f"mame_bats: {rom}: EagleBIOS-CD is 512 KiB")
    swapped = bytearray(len(data))
    swapped[0::2], swapped[1::2] = data[1::2], data[0::2]
    out = Path(dest) / "tests" / "cd-bios" / "eagle" / "neocdz"
    out.mkdir(parents=True, exist_ok=True)
    (out / "neocd.bin").write_bytes(bytes(swapped))
    (out / "000-lo.lo").write_bytes((ROOT / "bios" / "000-lo.lo").read_bytes())
    print(f"mame_bats: EagleBIOS-CD -> {out} (neocd.bin sha256 {hashlib.sha256(swapped).hexdigest()[:16]})")


def write_all(dest):
    dest = Path(dest)
    found = installed(dest)
    cds = cd_installed(dest)
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
    for game in cds:
        put(f"run_{game}_cd_eagle.bat", one_liner(f'call "%~dp0bin\\ngsdk.bat" cd {game} eagle %*', "Neo Geo CD, EagleBIOS-CD."))
        put(f"run_{game}_cd_stock.bat", one_liner(f'call "%~dp0bin\\ngsdk.bat" cd {game} stock %*', "Neo Geo CD, your stock CDZ firmware."))
    put("live/ngsdk_live.lua", LIVE_LUA)
    put("live/README-LIVE.txt", LIVE_README)
    for game, modes in found.items():
        for mode, (data, info) in modes.items():
            put(f"live/live_{game}_{mode}.bat",
                one_liner(f'call "%~dp0..\\bin\\ngsdk.bat" live {game} {mode} %*', "Play with the live log."))
            watch = live_watch(game, info)
            if watch:
                put(f"live/watch/{game}_{mode}.lua", watch)
    for game in cds:
        for profile in ("eagle", "stock"):
            put(f"live/live_{game}_cd_{profile}.bat",
                one_liner(f'call "%~dp0..\\bin\\ngsdk.bat" live {game} cd-{profile} %*', "Play with the live log."))
    put("MENU.bat", menu(found, cds))
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
    print(f"mame_bats: {len(written)} files in {dest} ({len(found)} games, {levels} level launchers, "
          f"{len(cds)} CD discs)")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dest", required=True, type=Path, help="the MAME test folder (C:\\mame\\neogeosdk is /mnt/c/mame/neogeosdk)")
    ap.add_argument("--describe", nargs=3, metavar=("GAME", "PLATFORM", "ELF"),
                    help="record the level launchers' addresses of an installed build")
    ap.add_argument("--src", type=Path, default=ROOT, help="the source tree the build was made from (default: this one)")
    ap.add_argument("--cd", nargs=2, action="append", default=[], metavar=("GAME", "CUE"),
                    help="install a game's Neo Geo CD disc (repeatable)")
    ap.add_argument("--cd-bios", type=Path, metavar="ROM", help="install EagleBIOS-CD (eagle_cd_cdz.rom)")
    ap.add_argument("--no-write", action="store_true", help="only --describe")
    args = ap.parse_args(argv)
    for game, cue in args.cd:
        if game not in GAMES:
            raise SystemExit(f"mame_bats: unknown game {game}")
        install_cd(args.dest, game, cue)
    if args.cd_bios:
        install_cd_bios(args.dest, args.cd_bios)
    if args.describe:
        game, platform, elf = args.describe
        describe(args.dest, game, platform, Path(elf), args.src.resolve())
    if not args.no_write:
        write_all(args.dest)
    return 0


if __name__ == "__main__":
    sys.exit(main())
