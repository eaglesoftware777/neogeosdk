@echo off
set "HERE=%~dp0"
"C:\mame\mame.exe" neogeo -noreadconfig -window -keepaspect -nofilter -prescale 3 -rompath "%HERE%bios\test_roms;%HERE%dist\candidates\shadowleague\rebirth-banked\mvs\roms" -hashpath "%HERE%dist\candidates\shadowleague\rebirth-banked\mvs\hash" -cart1 shadowleague -bios euro -cfg_directory "%HERE%games\shadowleague\build\play\mvs\cfg" -nvram_directory "%HERE%games\shadowleague\build\play\mvs\nvram" -state_directory "%HERE%games\shadowleague\build\play\mvs\states" -noautosave -skip_gameinfo
if errorlevel 1 pause
