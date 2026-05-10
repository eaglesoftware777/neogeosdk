@echo off
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set SDK_ROOT=%SCRIPT_DIR%..\..
set OUT_DIR=%SDK_ROOT%\out
set ROM_DIR=%SDK_ROOT%\roms\neogeosdk
set OBJ=%OUT_DIR%\driver.o
set OBJ_C=%OUT_DIR%\driver_c.o
set ASM=%OUT_DIR%\driver.gen.asm
set ASM_COMBINED=%OUT_DIR%\driver_combined.asm
set LINKFILE=%OUT_DIR%\m1.link
set M1ROM=%OUT_DIR%\777-m1.m1

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

if "%WLAZ80%"=="" set WLAZ80=wla-z80
if "%WLALINK%"=="" set WLALINK=wlalink
if "%PY%"=="" set PY=py
if "%USE_Z80C%"=="" set USE_Z80C=0
if "%LINK_C_DRIVER%"=="" set LINK_C_DRIVER=0
if "%Z80C_SRC%"=="" set Z80C_SRC=%SDK_ROOT%\sound\driver\driver.c

if not "%USE_Z80C%"=="1" goto assemble_asm

if "%HOSTCC%"=="" set HOSTCC=cc
if not "%Z80CC%"=="" if not exist "%Z80CC%" (
    echo Ignoring missing Z80CC=%Z80CC%
    set Z80CC=
)
if "%Z80CC%"=="" set Z80CC_ROOT=%SDK_ROOT%\z80c-special
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

echo Compiling %Z80C_SRC%
if "%USE_WSL_Z80CC%"=="1" (
    for %%I in ("%Z80C_SRC%") do set DRIVER_WIN=%%~fI
    for /f "usebackq delims=" %%I in (`wsl.exe wslpath -a "!Z80CC_ROOT!"`) do set Z80CC_ROOT_WSL=%%I
    for /f "usebackq delims=" %%I in (`wsl.exe wslpath -a "!DRIVER_WIN!"`) do set DRIVER_WSL=%%I
    wsl.exe /bin/bash -lc "make -C '!Z80CC_ROOT_WSL!' >/dev/null && '!Z80CC_ROOT_WSL!/build/z80cc' --target neogeo -DDRIVER_SPLIT_PRELUDE=1 -I'!DRIVER_WSL!/../' -S -o '!DRIVER_WSL!.asm' '!DRIVER_WSL!'"
    copy /Y "%Z80C_SRC%.asm" "%ASM%" >nul
) else (
    "%Z80CC%" --target neogeo -DDRIVER_SPLIT_PRELUDE=1 -I"%SDK_ROOT%\sound\driver" -S -o "%ASM%" "%Z80C_SRC%"
)
if errorlevel 1 exit /b 1
if "%LINK_C_DRIVER%"=="1" goto assemble_split
goto assemble_asm

:assemble_asm
echo Assembling %SDK_ROOT%\sound\m1\m1.asm
set ASM=%SDK_ROOT%\sound\m1\m1.asm

:assemble
echo Assembling %ASM%
%WLAZ80% -I "%SDK_ROOT%\sound\driver" -o "%OBJ%" "%ASM%"
if errorlevel 1 exit /b 1

echo [objects] > "%LINKFILE%"
echo %OBJ% >> "%LINKFILE%"
goto link_rom

:assemble_split
echo Linking experimental C driver core
%PY% "%SDK_ROOT%\sound\tools\combine_split_driver.py" "%SDK_ROOT%\sound\driver\driver_prelude.asm" "%ASM%" "%ASM_COMBINED%"
if errorlevel 1 exit /b 1
echo Assembling %ASM_COMBINED%
%WLAZ80% -I "%SDK_ROOT%\sound\driver" -o "%OBJ_C%" "%ASM_COMBINED%"
if errorlevel 1 exit /b 1

echo [objects] > "%LINKFILE%"
echo %OBJ_C% >> "%LINKFILE%"

:link_rom
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
copy /y "%M1ROM%" "%ROM_DIR%\777-m1.m1"
copy /y "%M1ROM%" "%ROM_DIR%\sm1.sm1"

if exist "%OBJ%" del "%OBJ%"
if exist "%OBJ_C%" del "%OBJ_C%"
if exist "%ASM_COMBINED%" del "%ASM_COMBINED%"
if exist "%ASM%" if "%USE_Z80C%"=="1" del "%ASM%"
del "%LINKFILE%"

echo Built %M1ROM%
