@echo off
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set SDK_ROOT=%SCRIPT_DIR%..\..
set OUT_DIR=%SDK_ROOT%\out
set ROM_DIR=%SDK_ROOT%\roms\ssideki
set OBJ=%OUT_DIR%\driver.o
set ASM=%OUT_DIR%\driver.gen.asm
set LINKFILE=%OUT_DIR%\m1.link
set M1ROM=%OUT_DIR%\052-m1.m1

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

if "%WLAZ80%"=="" set WLAZ80=wla-z80
if "%WLALINK%"=="" set WLALINK=wlalink
if "%HOSTCC%"=="" set HOSTCC=cc
if not "%Z80CC%"=="" if not exist "%Z80CC%" (
    echo Ignoring missing Z80CC=%Z80CC%
    set Z80CC=
)
if "%Z80CC%"=="" for %%I in ("%SDK_ROOT%\..\neogeo\z80c-special") do set Z80CC_ROOT=%%~fI
if "%Z80CC%"=="" set Z80CC=%Z80CC_ROOT%\build\z80cc.exe

if exist "%Z80CC%" goto have_z80cc
if "%Z80CC_ROOT%"=="" (
    echo Z80CC was set to "%Z80CC%" but the file was not found.
    exit /b 1
)

where "%HOSTCC%" >nul 2>nul
if errorlevel 1 goto try_wsl_z80cc

if not exist "%Z80CC_ROOT%\build" mkdir "%Z80CC_ROOT%\build"
echo Building %Z80CC%
"%HOSTCC%" -std=c11 -Wall -Wextra -O2 -o "%Z80CC%" "%Z80CC_ROOT%\src\main.c" "%Z80CC_ROOT%\src\lexer.c" "%Z80CC_ROOT%\src\parser.c" "%Z80CC_ROOT%\src\ast.c" "%Z80CC_ROOT%\src\codegen.c"
if errorlevel 1 exit /b 1
goto have_z80cc

:try_wsl_z80cc
where wsl >nul 2>nul
if errorlevel 1 (
    echo Could not find %Z80CC%, host compiler %HOSTCC%, or wsl.exe.
    exit /b 1
)
set USE_WSL_Z80CC=1

:have_z80cc

echo Compiling %SDK_ROOT%\sound\driver\driver.c
if "%USE_WSL_Z80CC%"=="1" (
    for %%I in ("%SDK_ROOT%\sound\driver\driver.c") do set DRIVER_WIN=%%~fI
    for /f "usebackq delims=" %%I in (`wsl.exe wslpath -a "!Z80CC_ROOT!"`) do set Z80CC_ROOT_WSL=%%I
    for /f "usebackq delims=" %%I in (`wsl.exe wslpath -a "!DRIVER_WIN!"`) do set DRIVER_WSL=%%I
    wsl.exe /bin/bash -lc "make -C '!Z80CC_ROOT_WSL!' >/dev/null && '!Z80CC_ROOT_WSL!/build/z80cc' '!DRIVER_WSL!'" > "%ASM%"
) else (
    "%Z80CC%" "%SDK_ROOT%\sound\driver\driver.c" > "%ASM%"
)
if errorlevel 1 exit /b 1

echo Assembling %ASM%
%WLAZ80% -I "%SDK_ROOT%\sound\driver" -o "%OBJ%" "%ASM%"
if errorlevel 1 exit /b 1

echo [objects] > "%LINKFILE%"
echo %OBJ% >> "%LINKFILE%"

echo Linking %M1ROM%
%WLALINK% -r "%LINKFILE%" "%M1ROM%"
if errorlevel 1 exit /b 1

REM Padding to 128KB using srec_cat if available
set SCAT=%SDK_ROOT%\win\srec_cat.exe
if exist "%SCAT%" (
    echo Padding %M1ROM% to 128KB
    "%SCAT%" "%M1ROM%" -binary -fill 0xFF 0x00000 0x20000 -o "%M1ROM%" -binary
)

if not exist "%ROM_DIR%" mkdir "%ROM_DIR%"
copy /y "%M1ROM%" "%ROM_DIR%\052-m1.m1"
copy /y "%M1ROM%" "%ROM_DIR%\sm1.sm1"

del "%OBJ%" "%LINKFILE%"

echo Built %M1ROM%
