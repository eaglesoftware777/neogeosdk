@echo off
setlocal
set "HERE=%~dp0"
set "MODE=%~1"
if "%MODE%"=="" set "MODE=mvs"
set "FLAVOR=%~2"
if "%FLAVOR%"=="" set "FLAVOR=eagle"
set "SYSTEM=neogeo"
set "BIOS=euro"
if /i "%MODE%"=="aes" set "SYSTEM=aes"
if /i "%MODE%"=="aes" set "BIOS=asia"
if /i "%FLAVOR%"=="unibios40" set "BIOS=unibios40"
set "MAME_EXE=%HERE%..\mame.exe"
if not exist "%MAME_EXE%" set "MAME_EXE=mame"
set "DATA=%HERE%tests\skylance\%MODE%"
set "ROMS=%DATA%\roms;%HERE%roms;%HERE%..\roms"
if /i "%FLAVOR%"=="eagle" set "ROMS=%HERE%tests\skylance\bios;%ROMS%"
"%MAME_EXE%" %SYSTEM% -noreadconfig -rompath "%ROMS%" -hashpath "%DATA%\hash" -bios %BIOS% -cart1 skylance -cfg_directory "%DATA%\cfg-%FLAVOR%" -nvram_directory "%DATA%\nvram-%FLAVOR%" -nofilter -window -noautoframeskip -frameskip 0
if errorlevel 1 pause
exit /b %ERRORLEVEL%
