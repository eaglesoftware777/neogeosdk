@echo off
setlocal EnableDelayedExpansion
REM ============================================================
REM  NeoGeo SDK - WSL (Ubuntu) bootstrap from Windows
REM ============================================================
REM
REM  Bootstraps WSL2 + Ubuntu, then drops into the distro and runs
REM  install-ubuntu.sh to get the SDK to a buildable state.
REM
REM  Use this when:
REM    * You're on Windows 11 but want to build through WSL
REM      (Linux toolchain, Linux make, /mnt/c/... layout).
REM    * You already have WSL but it's a fresh distro.
REM
REM  Use install-windows.bat instead when you want to build natively
REM  with MakefileWin32.mak.
REM
REM  Usage (from an Administrator cmd.exe):
REM      install-wsl.bat                    Ubuntu (current LTS), default prefix
REM      install-wsl.bat Ubuntu-22.04       pin a specific distro
REM      install-wsl.bat Ubuntu /home/me/ng custom prefix inside the distro
REM ============================================================

set "DISTRO=%~1"
if "%DISTRO%"=="" set "DISTRO=Ubuntu"

set "PREFIX=%~2"

cd /d "%~dp0"

REM ------------------------------------------------------------
REM 1. Admin check
REM ------------------------------------------------------------
net session >nul 2>&1
if errorlevel 1 (
    echo [install] ERROR: WSL install requires Administrator.
    echo [install] Right-click cmd.exe -^> "Run as administrator" and retry.
    exit /b 1
)

REM ------------------------------------------------------------
REM 2. Enable + install WSL itself (idempotent)
REM ------------------------------------------------------------
echo [install] checking WSL ...
where wsl >nul 2>&1
if errorlevel 1 (
    echo [install] installing WSL feature ^(may require reboot^) ...
    wsl --install --no-distribution
    if errorlevel 1 (
        echo [install] ERROR: 'wsl --install' failed.
        exit /b 2
    )
    echo.
    echo [install] WSL feature installed.  A reboot may be required
    echo [install] before WSL becomes usable.  Reboot Windows, then
    echo [install] re-run this script to continue.
    exit /b 0
)

REM ------------------------------------------------------------
REM 3. Make sure the requested distro is installed
REM ------------------------------------------------------------
echo [install] checking for WSL distro: %DISTRO% ...
wsl -l -q 2>nul | findstr /I /C:"%DISTRO%" >nul
if errorlevel 1 (
    echo [install] installing %DISTRO% ...
    wsl --install -d %DISTRO%
    if errorlevel 1 (
        echo [install] ERROR: 'wsl --install -d %DISTRO%' failed.
        echo [install] List available distros with:  wsl --list --online
        exit /b 2
    )
    echo.
    echo [install] %DISTRO% installed.  When prompted, create a Unix
    echo [install] username + password, then exit the shell and
    echo [install] re-run this script to continue.
    exit /b 0
)

REM ------------------------------------------------------------
REM 4. Copy install-ubuntu.sh into the distro at /tmp and run it
REM ------------------------------------------------------------
set "WIN_SH=%~dp0install-ubuntu.sh"
if not exist "%WIN_SH%" (
    echo [install] ERROR: %WIN_SH% missing.  Re-clone the install/ dir.
    exit /b 1
)

REM Convert C:\path\to\install-ubuntu.sh -> /mnt/c/path/to/install-ubuntu.sh
set "WSL_SH=%WIN_SH:\=/%"
set "WSL_SH=%WSL_SH:C:=/mnt/c%"
set "WSL_SH=%WSL_SH:D:=/mnt/d%"
set "WSL_SH=%WSL_SH:E:=/mnt/e%"

echo.
echo [install] handing off to %DISTRO% to run install-ubuntu.sh ...
echo [install] You may be prompted for the WSL user's sudo password.
echo.

if "%PREFIX%"=="" (
    wsl -d %DISTRO% -- bash -lc "cp '%WSL_SH%' /tmp/install-ubuntu.sh && chmod +x /tmp/install-ubuntu.sh && /tmp/install-ubuntu.sh --yes"
) else (
    wsl -d %DISTRO% -- bash -lc "cp '%WSL_SH%' /tmp/install-ubuntu.sh && chmod +x /tmp/install-ubuntu.sh && /tmp/install-ubuntu.sh --prefix '%PREFIX%' --yes"
)
set "RC=%ERRORLEVEL%"

echo.
if "%RC%"=="0" (
    echo [install] WSL install complete.
    echo [install] To build, open the WSL shell:
    echo [install]     wsl -d %DISTRO%
    if "%PREFIX%"=="" (
        echo [install]     . ~/neogeo/setenv.sh
        echo [install]     cd ~/neogeo/neogeosdk
    ) else (
        echo [install]     . '%PREFIX%/setenv.sh'
        echo [install]     cd '%PREFIX%/neogeosdk'
    )
    echo [install]     make all ^&^& make test
) else (
    echo [install] install-ubuntu.sh inside WSL returned %RC%.
)
exit /b %RC%
