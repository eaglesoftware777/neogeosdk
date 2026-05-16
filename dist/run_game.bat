@echo off
setlocal
if "%~1"=="" (
    echo Usage: %~nx0 ^<game^>
    echo Example: %~nx0 demo
    exit /b 1
)
set GAME=%~1
if not exist "%~dp0hash_eagle\%GAME%\neogeo.xml" (
    echo ERROR: missing "%~dp0hash_eagle\%GAME%\neogeo.xml"
    exit /b 1
)
mame neogeo -cart1 %GAME% ^
    -rompath "%~dp0roms" ^
    -hashpath "%~dp0hash_eagle\%GAME%;%~dp0hash_eagle;%~dp0hash" ^
    -bios unibios22 ^
    -window ^
    -console ^
    -verbose
endlocal
