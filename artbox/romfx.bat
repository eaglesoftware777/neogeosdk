@echo off
setlocal
if "%GAME_ID%"=="" set GAME_ID=777
if "%GAME%"=="" set GAME=demo
for %%i in ("%~dp0..") do set REPO_ROOT=%%~fi
if not exist "%GAME_ID%-s1.s1" (
    echo Missing %GAME_ID%-s1.s1. Run romdbfiximport.py and fixtiles.py first.
    exit /b 1
)
if not exist "%REPO_ROOT%\roms\%GAME%" mkdir "%REPO_ROOT%\roms\%GAME%"
copy /Y "%GAME_ID%-s1.s1" "%REPO_ROOT%\roms\%GAME%\%GAME_ID%-s1.s1" >nul
endlocal
