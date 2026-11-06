@echo off
setlocal
title Eagle Software - Maiya: Super Nature Girl Setup
cd /d "%~dp0"

echo ==========================================================
echo  Eagle Software - Maiya: Super Nature Girl Setup Wizard
echo ==========================================================
echo Starting Graphical Installer...

set "SCRIPT="
if exist "%~dp0tools\installer\install_maiya.py" set "SCRIPT=%~dp0tools\installer\install_maiya.py"
if "%SCRIPT%"=="" if exist "%~dp0install_maiya.py" set "SCRIPT=%~dp0install_maiya.py"
if "%SCRIPT%"=="" if exist "%~dp0installer\install_maiya.py" set "SCRIPT=%~dp0installer\install_maiya.py"

if "%SCRIPT%"=="" (
    echo [ERROR] Could not locate install_maiya.py.
    pause
    exit /b 1
)

where python >nul 2>nul
if %ERRORLEVEL% equ 0 (
    python "%SCRIPT%" %*
    goto end
)

where py >nul 2>nul
if %ERRORLEVEL% equ 0 (
    py "%SCRIPT%" %*
    goto end
)

echo [ERROR] Python 3 was not found on your system.
echo Please install Python 3 from https://www.python.org/ or the Microsoft Store.
pause

:end
