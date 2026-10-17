@echo off
set "HERE=%~dp0"
"C:\mame\mame.exe" aes -noreadconfig -window -keepaspect -nofilter -prescale 3 -rompath "%HERE%bios\test_roms;%HERE%dist\candidates\shadowleague\rebirth-banked\aes\roms" -hashpath "%HERE%dist\candidates\shadowleague\rebirth-banked\aes\hash" -cart1 shadowleague -bios asia -cfg_directory "%HERE%games\shadowleague\build\play\aes\cfg" -nvram_directory "%HERE%games\shadowleague\build\play\aes\nvram" -state_directory "%HERE%games\shadowleague\build\play\aes\states" -noautosave -skip_gameinfo
if errorlevel 1 pause
