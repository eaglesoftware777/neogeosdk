@echo off
setlocal EnableDelayedExpansion
REM ============================================================
REM  NeoGeo SDK - native Windows 11 installer (cmd.exe / .bat)
REM ============================================================
REM
REM  Uses `winget` for everything that has a winget package: Python,
REM  Git, MAME, srecord (via Chocolatey-style fallback), and the
REM  Microsoft C++ build tools (which include `make` via GnuWin or a
REM  vendored toolchain).  The m68k cross compiler is fetched from
REM  SysGCC's site since there is no official winget entry.
REM
REM  Idempotent: every install step checks first.
REM
REM  Usage (from an Administrator cmd.exe):
REM      install-windows.bat                         use defaults
REM      install-windows.bat C:\dev\neogeo           custom SDKHOME
REM
REM  Defaults:
REM      SDKHOME      %USERPROFILE%\neogeo
REM      m68k root    %SDKHOME%\x-tools-v2-win
REM      sdk repo     %SDKHOME%\neogeosdk
REM ============================================================

cd /d "%~dp0"

set "SDKHOME=%~1"
if "%SDKHOME%"=="" set "SDKHOME=%USERPROFILE%\neogeo"
set "M68K_ROOT=%SDKHOME%\x-tools-v2-win"
set "SDK_DIR=%SDKHOME%\neogeosdk"

echo.
echo [install] SDKHOME    = %SDKHOME%
echo [install] M68K_ROOT  = %M68K_ROOT%
echo [install] SDK_DIR    = %SDK_DIR%
echo.

REM ------------------------------------------------------------
REM 1. Administrator check
REM ------------------------------------------------------------
net session >nul 2>&1
if errorlevel 1 (
    echo [install] WARNING: not running as Administrator.
    echo [install] winget package installs may fail.  Re-run from an
    echo [install] elevated cmd.exe ^("Run as administrator"^) if so.
    echo.
)

REM ------------------------------------------------------------
REM 2. Verify winget is available
REM ------------------------------------------------------------
where winget >nul 2>&1
if errorlevel 1 (
    echo [install] ERROR: winget not found.  Install "App Installer" from
    echo [install] the Microsoft Store, then re-run this script.
    exit /b 1
)

REM ------------------------------------------------------------
REM 3. Base packages via winget
REM ------------------------------------------------------------
call :winget_install Python.Python.3.12      "Python 3.12"
call :winget_install Git.Git                 "Git"
call :winget_install GnuWin32.Make           "GNU Make"
call :winget_install MAMEDev.MAME            "MAME"
call :winget_install vhelin.wla-dx           "WLA-DX (wla-z80 + wlalink)"
call :winget_install srecord.srecord         "srecord"
call :winget_install ChrisAnt996.SoX         "SoX (optional)"

REM ------------------------------------------------------------
REM 4. Python dependencies
REM ------------------------------------------------------------
echo.
echo [install] installing Python packages via py -m pip ...
where py >nul 2>&1
if errorlevel 1 (
    echo [install] ERROR: py launcher not on PATH ^(install Python 3.x first^).
    exit /b 1
)
py -m pip install --upgrade pip
py -m pip install numpy pillow pypng
if errorlevel 1 (
    echo [install] ERROR: pip install failed.
    exit /b 2
)

REM ------------------------------------------------------------
REM 5. m68k cross compiler
REM ------------------------------------------------------------
REM The release ships x-tools-v2-win.tar containing m68k-elf-gcc.exe
REM and a sibling m68k-unknown-elf layout; the Win32 makefile detects
REM either.  We try the auto-download first; if no curl + tar.exe
REM available, we fall through to the SysGCC fallback below.
set "WIN_TC_URL=https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.3.0/x-tools-v2-win.tar"

if exist "%M68K_ROOT%\bin\m68k-elf-gcc.exe" goto :tc_done
if exist "%M68K_ROOT%\m68k-unknown-elf\bin\m68k-unknown-elf-gcc.exe" goto :tc_done

echo [install] m68k toolchain not at %M68K_ROOT%; attempting auto-fetch...
where curl >nul 2>&1
if errorlevel 1 (
    echo [install]   curl.exe not on PATH; skipping auto-fetch.
    goto :tc_fallback
)
where tar >nul 2>&1
if errorlevel 1 (
    echo [install]   tar.exe not on PATH ^(Win10 1803+/Win11 ships one^); skipping auto-fetch.
    goto :tc_fallback
)
if not exist "%SDKHOME%" mkdir "%SDKHOME%"
pushd "%SDKHOME%"
curl -fL -o x-tools-v2-win.tar "%WIN_TC_URL%"
if errorlevel 1 (
    echo [install]   download failed; trying SysGCC fallback.
    popd
    goto :tc_fallback
)
tar -xf x-tools-v2-win.tar
del /q x-tools-v2-win.tar 2>nul
popd

if exist "%M68K_ROOT%\bin\m68k-elf-gcc.exe" goto :tc_done
if exist "%M68K_ROOT%\m68k-unknown-elf\bin\m68k-unknown-elf-gcc.exe" goto :tc_done
echo [install]   extraction did not produce expected gcc; trying fallback.

:tc_fallback
if exist "C:\SysGCC\m68k-elf\bin\m68k-elf-gcc.exe" (
    echo [install] m68k toolchain found at C:\SysGCC\m68k-elf;
    echo [install] using M68K_ELF_ROOT=C:\SysGCC\m68k-elf as fallback.
    set "M68K_ELF_ROOT=C:\SysGCC\m68k-elf"
    goto :tc_done
)
echo.
echo [install] m68k cross compiler not found.  Install one of:
echo [install]   * SysGCC m68k-elf:  http://gnutoolchains.com/m68k-elf/
echo [install]   * extract release x-tools-v2-win.tar into %M68K_ROOT%:
echo [install]     %WIN_TC_URL%
echo [install] Then re-run this script ^(it is safe to re-run^).
set "TOOLCHAIN_MISSING=1"

:tc_done

REM ------------------------------------------------------------
REM 6. Clone the SDK (v1.3.x branch)
REM ------------------------------------------------------------
set "SDK_BRANCH=neo_universal_2d"
if not exist "%SDK_DIR%\.git" (
    if not exist "%SDKHOME%" mkdir "%SDKHOME%"
    echo [install] cloning neogeosdk ^(%SDK_BRANCH%^) into %SDK_DIR%
    git clone -b %SDK_BRANCH% https://github.com/eaglesoftware777/neogeosdk.git "%SDK_DIR%"
    if errorlevel 1 (
        echo [install] ERROR: git clone failed.
        exit /b 2
    )
) else (
    echo [install] neogeosdk repo already at %SDK_DIR%; skipping clone
)

REM ------------------------------------------------------------
REM 7. Persist SDKHOME for future shells (user-scope env var)
REM ------------------------------------------------------------
echo [install] persisting SDKHOME=%SDKHOME% to user environment
setx SDKHOME "%SDKHOME%" >nul

REM ------------------------------------------------------------
REM 8. Smoke test
REM ------------------------------------------------------------
echo.
echo [install] verifying installation:
call :need py
call :need git
call :need make
call :need wla-z80
call :need wlalink
call :need mame
call :need sox
call :need srec_cat
py -c "import numpy, PIL, png" 2>nul && echo   python deps  OK   || echo   python deps  MISSING
if exist "%M68K_ROOT%\bin\m68k-elf-gcc.exe" (
    echo   m68k-gcc     OK   ^(%M68K_ROOT%\bin\m68k-elf-gcc.exe^)
) else if defined M68K_ELF_ROOT (
    echo   m68k-gcc     OK   ^(%M68K_ELF_ROOT%\bin\m68k-elf-gcc.exe^)
) else (
    echo   m68k-gcc     MISSING
    set "TOOLCHAIN_MISSING=1"
)

echo.
if defined TOOLCHAIN_MISSING (
    echo [install] toolchain still missing; install m68k-elf and re-run.
    exit /b 2
)

echo [install] install complete.  Build the demo with:
echo.
echo     cd /d "%SDK_DIR%"
echo     make -f MakefileWin32.mak all
echo     make -f MakefileWin32.mak test
echo.
exit /b 0


REM ============================================================
REM Helpers
REM ============================================================
:winget_install
REM %1 = winget package id, %2 = friendly name
set "WGT_ID=%~1"
set "WGT_NAME=%~2"
echo.
echo [install] checking %WGT_NAME% (%WGT_ID%) ...
winget list --id %WGT_ID% --exact 2>nul | findstr /I /C:"%WGT_ID%" >nul
if errorlevel 1 (
    echo [install]   installing %WGT_NAME% ...
    winget install --id %WGT_ID% --silent --accept-source-agreements --accept-package-agreements
    if errorlevel 1 (
        echo [install]   WARN: winget install of %WGT_NAME% failed; install manually.
    )
) else (
    echo [install]   %WGT_NAME% already installed.
)
exit /b 0

:need
where %~1 >nul 2>&1
if errorlevel 1 (
    echo   %~1 MISSING
) else (
    echo   %~1 OK
)
exit /b 0
