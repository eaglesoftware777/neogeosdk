@echo off
setlocal
REM #######
REM #https://eaglesoftware.biz
REM #https://github.com/eaglesoftware777
REM #https://github.com/eaglesoftware777/neogeosdk
REM #######
REM NeoGeo SDK - Lua Console Launcher (Windows)
REM Place neogeo.zip (BIOS) inside roms\ before running.
REM Opens MAME with the Lua scripting console (interactive, no CPU debugger).
REM Do NOT combine -console with -debug — they conflict.

py "%~dp0hash_eagle\gen_hash.py"
if errorlevel 1 goto :eof

mame neogeo -cart1 neogeosdk ^
    -rompath "%~dp0roms" ^
    -hashpath "%~dp0hash_eagle;%~dp0hash" ^
    -bios unibios22 ^
    -window ^
    -pluginspath "C:\mame\plugins" ^
    -plugin console ^
    -console ^
    -verbose
endlocal
