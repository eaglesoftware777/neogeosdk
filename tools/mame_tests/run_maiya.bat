@echo off
setlocal
set "HERE=%~dp0"
set "PLATFORM=%~1"
if "%PLATFORM%"=="" set "PLATFORM=mvs"
if /I "%PLATFORM%"=="mvs" goto mvs
if /I "%PLATFORM%"=="aes" goto aes
echo Usage: run_maiya.bat mvs ^| aes [BIOS]
exit /b 1
:mvs
set "PLATFORM=mvs"
set "DRIVER=neogeo"
set "BIOS=euro"
set CARD=
goto launch
:aes
set "PLATFORM=aes"
set "DRIVER=aes"
set "BIOS=asia"
rem A console keeps its saves on this memory card (made by the installer).
set CARD=
if exist "%HERE%tests\aes\memcard.bin" set CARD=-memc "%HERE%tests\aes\memcard.bin"
:launch
if not "%~2"=="" set "BIOS=%~2"
set "MAME_EXE=mame"
if exist "%HERE%..\mame.exe" set "MAME_EXE=%HERE%..\mame.exe"
if not exist "%HERE%..\mame.exe" if exist "%HERE%..\mame64.exe" set "MAME_EXE=%HERE%..\mame64.exe"
if not exist "%HERE%tests\%PLATFORM%\cfg" mkdir "%HERE%tests\%PLATFORM%\cfg"
if not exist "%HERE%tests\%PLATFORM%\nvram" mkdir "%HERE%tests\%PLATFORM%\nvram"
"%MAME_EXE%" %DRIVER% -rompath "%HERE%tests\%PLATFORM%\roms;%HERE%roms;%HERE%..\roms" -hashpath "%HERE%tests\%PLATFORM%\hash" -bios %BIOS% -cart1 maiya %CARD% -cfg_directory "%HERE%tests\%PLATFORM%\cfg" -nvram_directory "%HERE%tests\%PLATFORM%\nvram" -nofilter -window
exit /b %ERRORLEVEL%
