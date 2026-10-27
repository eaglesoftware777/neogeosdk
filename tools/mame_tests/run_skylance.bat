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
if exist "%HERE%mame-path.txt" set /p MAME_EXE=<"%HERE%mame-path.txt"
set "DATA=%HERE%tests\skylance\%MODE%"
set "ROMS=%DATA%\roms;%HERE%roms;%HERE%..\roms"
if /i "%FLAVOR%"=="eagle" set "ROMS=%HERE%tests\skylance\bios;%ROMS%"
set "MAME_DISPLAY=-skip_gameinfo -nowindow -keepaspect -filter -video d3d"
if exist "%HERE%skylance_display.bat" call "%HERE%skylance_display.bat"
if exist "%HERE%mame_display.bat" call "%HERE%mame_display.bat"
"%MAME_EXE%" %SYSTEM% -noreadconfig %MAME_DISPLAY% -rompath "%ROMS%" -hashpath "%DATA%\hash" -bios %BIOS% -cart1 skylance -cfg_directory "%DATA%\cfg-%FLAVOR%" -nvram_directory "%DATA%\nvram-%FLAVOR%" -noautoframeskip -frameskip 0
if errorlevel 1 pause
exit /b %ERRORLEVEL%
