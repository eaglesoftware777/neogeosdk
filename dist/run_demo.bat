@echo off
setlocal
REM NeoGeo SDK - Release Launcher (demo)
REM Place neogeo.zip (BIOS) inside the roms\ folder before running.
mame neogeo -cart1 demo ^
    -rompath "%~dp0roms" ^
    -hashpath "%~dp0hash_eagle\demo;%~dp0hash_eagle;%~dp0hash" ^
    -bios unibios22 ^
    -window ^
    -console ^
    -verbose
endlocal
