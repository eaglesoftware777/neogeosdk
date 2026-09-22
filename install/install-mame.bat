@echo off
REM ============================================================
REM  NeoGeo SDK - install built games into a local MAME copy
REM ============================================================
REM
REM  Thin wrapper around tools\mame_launcher.py.  Prompts for the MAME
REM  installation folder the first time it runs (or reads it from
REM  %USERPROFILE%\.neogeosdk\config.json if already saved), then copies
REM  the requested game's dist zip into a self-contained neogeosdk\ folder
REM  inside that MAME install -- it never touches MAME's own rom
REM  collection or ini files.
REM
REM  Usage:
REM      install-mame.bat                  interactive: pick a game, install it
REM      install-mame.bat --game maiya     install one game
REM      install-mame.bat --game all       install every game already packaged
REM      install-mame.bat --mame-path DIR  set/override the saved MAME path
REM
REM  Build + package a game first with:
REM      make -f MakefileWin32.mak bios-package GAME=<name>
REM      (or: make -f MakefileWin32.mak dist-all, for every game)

cd /d "%~dp0\.."

where py >nul 2>&1
if errorlevel 1 (
    python tools\mame_launcher.py install %*
) else (
    py tools\mame_launcher.py install %*
)
