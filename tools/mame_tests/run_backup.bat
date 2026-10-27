@echo off
setlocal
set "HERE=%~dp0"
set "PLATFORM=%~1"
if "%PLATFORM%"=="" set "PLATFORM=mvs"
if /I "%PLATFORM%"=="mvs" (set "DRIVER=neogeo"& set "BIOS=euro") else if /I "%PLATFORM%"=="aes" (set "DRIVER=aes"& set "BIOS=asia") else (echo Usage: run_backup.bat mvs ^| aes [BIOS]& exit /b 1)
if not "%~2"=="" set "BIOS=%~2"
if not exist "%HERE%latest-backup.txt" (echo No backup found.& exit /b 1)
set /p "STAMP="<"%HERE%latest-backup.txt"
set "OLD=%HERE%backups\%STAMP%"
set "ROMPATH=%OLD%roms;%HERE%..\roms"
set "HASHPATH=%OLD%hash\maiya"
if exist "%OLD%tests\%PLATFORM%\hash\neogeo.xml" set "HASHPATH=%OLD%tests\%PLATFORM%\hash"
if exist "%OLD%compare-hash\neogeo.xml" set "HASHPATH=%OLD%compare-hash"
if exist "%OLD%tests\%PLATFORM%\roms" set "ROMPATH=%OLD%tests\%PLATFORM%\roms;%ROMPATH%"
set "MAME_EXE=%HERE%..\mame.exe"
if not exist "%MAME_EXE%" set "MAME_EXE=%HERE%..\mame64.exe"
if not exist "%HERE%compare-state\%STAMP%\%PLATFORM%\cfg" mkdir "%HERE%compare-state\%STAMP%\%PLATFORM%\cfg"
if not exist "%HERE%compare-state\%STAMP%\%PLATFORM%\nvram" mkdir "%HERE%compare-state\%STAMP%\%PLATFORM%\nvram"
echo Running backup %STAMP% - %PLATFORM%
"%MAME_EXE%" %DRIVER% -noreadconfig -rompath "%ROMPATH%" -hashpath "%HASHPATH%" -bios %BIOS% -cart1 maiya -cfg_directory "%HERE%compare-state\%STAMP%\%PLATFORM%\cfg" -nvram_directory "%HERE%compare-state\%STAMP%\%PLATFORM%\nvram" -nofilter -window
exit /b %ERRORLEVEL%
