@echo off
setlocal
REM #######
REM #https://eaglesoftware.biz
REM #https://github.com/eaglesoftware777
REM #https://github.com/eaglesoftware777/neogeosdk
REM #######
REM NeoGeo SDK - Debug Launcher (Windows)
REM Place neogeo.zip (BIOS) inside roms\ before running.
REM Opens MAME with the built-in 68000 CPU debugger.
REM
REM NOTE: -console (Lua scripting console) conflicts with -debug (CPU debugger).
REM       Use run_neogeosdk_lua.bat if you need the Lua console without -debug.

py "%~dp0hash_eagle\gen_hash.py"
if errorlevel 1 goto :eof

mame neogeo -cart1 neogeosdk ^
    -rompath "%~dp0roms" ^
    -hashpath "%~dp0hash_eagle;%~dp0hash" ^
    -bios unibios22 ^
    -window ^
    -pluginspath "C:\mame\plugins" ^
    -verbose ^
    -debug
endlocal
