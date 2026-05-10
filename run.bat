@echo off
setlocal
REM #######
REM #https://eaglesoftware.biz
REM #https://github.com/eaglesoftware777
REM #https://github.com/eaglesoftware777/neogeosdk
REM #######
REM NeoGeo SDK - MAME launcher (Windows)
REM Usage:  run.bat [--debug]

py "%~dp0hash_eagle\gen_hash.py"
if errorlevel 1 goto :eof

if /I "%1"=="--debug" (
    mame neogeo -cart1 neogeosdk ^
        -rompath "%~dp0roms" ^
        -hashpath "%~dp0hash_eagle;%~dp0hash" ^
        -bios unibios22 ^
        -window ^
        -waitvsync ^
        -debug -verbose
) else (
    mame neogeo -cart1 neogeosdk ^
        -rompath "%~dp0roms" ^
        -hashpath "%~dp0hash_eagle;%~dp0hash" ^
        -bios unibios22 ^
        -window ^
        -waitvsync
)
endlocal
