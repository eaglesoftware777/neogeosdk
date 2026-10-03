@echo off
setlocal
rem Maiya on AES with UniBIOS 4.0 (roms\aes\uni-bios_4_0.rom, placed by hand).
rem Its own settings and saves (tests\aes-unibios), apart from the EagleBIOS ones.
set "HERE=%~dp0"
set "MAME_EXE=mame"
if exist "%HERE%..\mame.exe" set "MAME_EXE=%HERE%..\mame.exe"
if not exist "%HERE%..\mame.exe" if exist "%HERE%..\mame64.exe" set "MAME_EXE=%HERE%..\mame64.exe"
if not exist "%HERE%roms\aes\uni-bios_4_0.rom" (
    echo UniBIOS 4.0 is not installed: put uni-bios_4_0.rom in %HERE%roms\aes
    pause
    exit /b 1
)
set "DATA=%HERE%tests\aes-unibios"
if not exist "%DATA%\cfg" mkdir "%DATA%\cfg"
if not exist "%DATA%\nvram" mkdir "%DATA%\nvram"
echo UniBIOS in-game menu, during play: hold Start+Select (keys 1+5)
echo   or Start+A+B+C (keys 1 + Left Ctrl + Left Alt + Space)
"%MAME_EXE%" aes -rompath "%HERE%tests\aes\roms;%HERE%roms;%HERE%..\roms" -hashpath "%HERE%tests\aes\hash" -bios unibios40 -cart1 maiya -cfg_directory "%DATA%\cfg" -nvram_directory "%DATA%\nvram" -nofilter -window
exit /b %ERRORLEVEL%
