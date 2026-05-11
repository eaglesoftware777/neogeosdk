@echo off
setlocal
REM #######
REM #https://eaglesoftware.biz
REM #https://github.com/eaglesoftware777
REM #https://github.com/eaglesoftware777/neogeosdk
REM #######
REM NeoGeo SDK - M68K Trace Launcher (Windows)
REM Runs MAME with the 68000 execution tracer.
REM On crash the last instructions are in dump\m68k_trace.txt.
REM
REM After the crash: open dump\m68k_trace.txt and check the last 30 lines.
REM Cross-reference PCs with:  make debug-build  (writes dump\game.sym + game.debug.dump)

py "%~dp0hash_eagle\gen_hash.py"
if errorlevel 1 goto :eof

if not exist "%~dp0dump" mkdir "%~dp0dump"

mame neogeo -cart1 neogeosdk ^
    -rompath "%~dp0roms" ^
    -hashpath "%~dp0hash_eagle;%~dp0hash" ^
    -bios unibios22 ^
    -window ^
    -verbose ^
    -debug ^
    -debugscript "%~dp0dump\mame_trace.mds"

echo.
echo Trace written to: %~dp0dump\m68k_trace.txt
echo Cross-ref symbols: %~dp0dump\game.sym
endlocal
