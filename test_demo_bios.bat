@echo off
setlocal
echo === NeoGeo SDK - Experimental EagleBIOS Test Launcher (demo) ===

set "ROOT_DIR=%~dp0"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"

if not exist "%ROOT_DIR%\bios\test_roms\neogeo" mkdir "%ROOT_DIR%\bios\test_roms\neogeo"
if not exist "%ROOT_DIR%\bios\test_roms\aes" mkdir "%ROOT_DIR%\bios\test_roms\aes"

if exist "%ROOT_DIR%\bios\sp-s2.sp1" (
    copy /y "%ROOT_DIR%\bios\sp-s2.sp1" "%ROOT_DIR%\bios\test_roms\neogeo\" >nul
    copy /y "%ROOT_DIR%\bios\neo-epo.bin" "%ROOT_DIR%\bios\test_roms\aes\" >nul
    copy /y "%ROOT_DIR%\bios\sm1.sm1"   "%ROOT_DIR%\bios\test_roms\neogeo\" >nul
    copy /y "%ROOT_DIR%\bios\sm1.sm1"   "%ROOT_DIR%\bios\test_roms\aes\" >nul
    copy /y "%ROOT_DIR%\bios\sfix.sfix" "%ROOT_DIR%\bios\test_roms\neogeo\" >nul
    copy /y "%ROOT_DIR%\bios\sfix.sfix" "%ROOT_DIR%\bios\test_roms\aes\" >nul
    copy /y "%ROOT_DIR%\bios\000-lo.lo" "%ROOT_DIR%\bios\test_roms\neogeo\" >nul
    copy /y "%ROOT_DIR%\bios\000-lo.lo" "%ROOT_DIR%\bios\test_roms\aes\" >nul
) else (
    echo WARNING: EagleBIOS ROMs not found in bios\.
    echo Run: make -C bios
)

python "%ROOT_DIR%\hash_eagle\gen_hash.py" >nul 2>&1

echo Launching MAME with experimental EagleBIOS on demo...
mame neogeo ^
    -cart1 demo ^
    -rompath "%ROOT_DIR%\bios\test_roms;%ROOT_DIR%\roms" ^
    -hashpath "%ROOT_DIR%\hash_eagle\demo;%ROOT_DIR%\hash_eagle;%ROOT_DIR%\hash" ^
    -bios euro ^
    -window ^
    -waitvsync ^
    %*

endlocal
