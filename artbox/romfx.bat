@echo off
setlocal
set SCRIPT_DIR=%~dp0
set REPO_ROOT=%SCRIPT_DIR%..
if not exist 777-s1.s1 (
    echo Missing 777-s1.s1. Run romdbfiximport.py and fixtiles.py first.
    exit /b 1
)
if not exist "%REPO_ROOT%\roms\neogeosdk" mkdir "%REPO_ROOT%\roms\neogeosdk"
copy /Y 777-s1.s1 "%REPO_ROOT%\roms\neogeosdk\777-s1.s1" >nul
endlocal
