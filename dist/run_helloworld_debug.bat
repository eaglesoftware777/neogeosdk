@echo off
setlocal
REM NeoGeo SDK - Debug Release Launcher (helloworld)
REM Place neogeo.zip (BIOS) inside the roms\ folder before running.
mame neogeo -cart1 helloworld ^
    -rompath "%~dp0roms" ^
    -hashpath "%~dp0hash_eagle\helloworld;%~dp0hash_eagle;%~dp0hash" ^
    -bios unibios22 ^
    -window ^
    -console ^
    -verbose ^
    -debug
endlocal
