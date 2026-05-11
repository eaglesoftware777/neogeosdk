@echo off
setlocal
REM #######
REM #https://eaglesoftware.biz
REM #https://github.com/eaglesoftware777
REM #https://github.com/eaglesoftware777/neogeosdk
REM #######
REM NeoGeo SDK - Debug Launcher (Windows)
REM Place neogeo.zip (BIOS) inside roms\ before running.
REM Opens MAME with the built-in debugger and verbose console output.

py "%~dp0hash_eagle\gen_hash.py"
if errorlevel 1 goto :eof

mame neogeo -cart1 neogeosdk ^
    -rompath "%~dp0roms" ^
    -hashpath "%~dp0hash_eagle;%~dp0hash" ^
    -bios unibios22 ^
    -window ^
    -console ^
    -verbose ^
    -debug
endlocal
