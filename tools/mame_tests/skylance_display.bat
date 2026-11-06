@echo off
set "MAME_DISPLAY=-skip_gameinfo -nowindow -keepaspect -filter -video d3d"
if not "%MAME_SCANLINES%"=="1" exit /b 0
for %%I in ("%MAME_EXE%") do set "MAME_ASSETS=%%~dpI"
if not exist "%MAME_ASSETS%bgfx\chains\crt-geom.json" exit /b 0
set "MAME_DISPLAY=-skip_gameinfo -nowindow -keepaspect -filter -video bgfx -bgfx_backend d3d11 -bgfx_path "%MAME_ASSETS%bgfx" -artpath "%MAME_ASSETS%artwork" -bgfx_screen_chains crt-geom"
exit /b 0
