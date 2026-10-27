@echo off
setlocal
set /p STAMP=<"%~dp0latest-skylance-backup.txt"
set "HERE=%~dp0"
set "DATA=%HERE%backups\skylance\%STAMP%\tests\skylance\mvs"
"%HERE%..\mame.exe" neogeo -noreadconfig -rompath "%DATA%\roms;%HERE%tests\skylance\bios;%HERE%roms;%HERE%..\roms" -hashpath "%DATA%\hash" -bios euro -cart1 skylance -cfg_directory "%DATA%\cfg" -nvram_directory "%DATA%\nvram" -nofilter -window
if errorlevel 1 pause
exit /b %ERRORLEVEL%
