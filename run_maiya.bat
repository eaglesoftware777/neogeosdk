@echo off
setlocal
REM Launch Maiya: Super Nature Girl from any directory or repository.
mame neogeo -cart1 maiya ^
    -rompath "%~dp0roms\maiya;%~dp0roms" ^
    -hashpath "%~dp0hash_eagle\maiya;%~dp0hash_eagle;%~dp0hash" ^
    -bios euro ^
    -window ^
    -waitvsync ^
    -nofilter
endlocal
