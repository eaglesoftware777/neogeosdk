@echo off
setlocal
REM NeoGeo SDK - Debug Release Launcher (maiya)
REM Place neogeo.zip (BIOS) inside the roms\ folder before running.
mame neogeo -cart1 maiya ^
    -rompath "%~dp0roms" ^
    -hashpath "%~dp0hash_eagle\maiya;%~dp0hash_eagle;%~dp0hash" ^
    -bios unibios22 ^
    -window ^
    -console ^
    -verbose ^
    -debug
endlocal
