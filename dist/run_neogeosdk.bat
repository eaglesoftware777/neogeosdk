@echo off
setlocal
REM NeoGeo SDK - Release Launcher
REM Place neogeo.zip (BIOS) inside the roms\ folder before running.
REM Correct launch: mame neogeo -cart1 neogeosdk (NOT mame neogeosdk)
mame neogeo -cart1 neogeosdk ^
    -rompath "%~dp0roms" ^
    -hashpath "%~dp0hash_eagle;%~dp0hash" ^
    -bios unibios22 ^
    -window ^
    -console ^
    -verbose
endlocal
