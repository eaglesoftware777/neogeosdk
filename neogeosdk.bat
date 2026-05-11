@echo off
setlocal EnableDelayedExpansion
REM #######
REM #https://eaglesoftware.biz
REM #https://github.com/eaglesoftware777
REM #https://github.com/eaglesoftware777/neogeosdk
REM #######
REM NeoGeo SDK - Interactive build menu (Windows)
REM Covers all make targets from first-use to most specialised.

cd /d "%~dp0"

:menu
cls
echo =============================================
echo    NeoGeo SDK - Build Menu
echo =============================================
echo.
echo   -- FULL BUILD (start here) --
echo    1) nmake clean-all ^&^& nmake all   Full clean rebuild  [recommended]
echo    2) nmake all                       Build everything (no clean)
echo.
echo   -- PROGRAM ROM --
echo    3) nmake p1                        68000 program ROM only
echo.
echo   -- GRAPHICS --
echo    4) nmake sfix                      FIX / S ROM (tile graphics)
echo    5) nmake art                       Sprite C ROMs
echo.
echo   -- SOUND --
echo    6) nmake sound                     Full sound (samples + V ROM + M1)
echo    7) nmake m1rom                     Z80 sound driver ROM only
echo    8) nmake vrom                      V ROM (ADPCM sample pack)
echo    9) nmake mml                       Compile SSG/MML music data
echo   10) nmake fm                        Compile FM music data
echo   11) nmake fmpatches                 Compile FM patch bank
echo   12) nmake ssg                       Compile standalone SSG data
echo.
echo   -- TEST / DEBUG --
echo   13) nmake test                      Build P1, update hash, launch MAME (MVS)
echo   14) nmake test-aes                  Launch MAME with UniBIOS (AES)
echo   15) nmake debug                     Launch MAME with CPU debugger
echo   16) nmake mame-trace                Trace m68k execution to dump\m68k_trace.txt
echo   17) nmake debug-build               Debug build with map/sym artifacts
echo   18) nmake gdb-trace                 GDB trace dump
echo   19) nmake gdb                       Launch GDB
echo   20) nmake gdb-remote                GDB remote (MAME -debug)
echo   21) nmake dump                      Disassemble and hex dump
echo.
echo   -- PACKAGE --
echo   22) nmake dist                      Build release package (dist\roms\neogeosdk.zip)
echo.
echo   -- CLEAN --
echo   23) nmake clean                     Remove program ROM outputs
echo   24) nmake sound-clean               Remove sound ROM outputs
echo   25) nmake art-clean                 Remove art outputs
echo   26) nmake clean-all                 Full clean (all generated files)
echo.
echo    q) Quit
echo.
set /p "choice=Enter choice: "

if "%choice%"=="1"  goto run1
if "%choice%"=="2"  goto run2
if "%choice%"=="3"  goto run3
if "%choice%"=="4"  goto run4
if "%choice%"=="5"  goto run5
if "%choice%"=="6"  goto run6
if "%choice%"=="7"  goto run7
if "%choice%"=="8"  goto run8
if "%choice%"=="9"  goto run9
if "%choice%"=="10" goto run10
if "%choice%"=="11" goto run11
if "%choice%"=="12" goto run12
if "%choice%"=="13" goto run13
if "%choice%"=="14" goto run14
if "%choice%"=="15" goto run15
if "%choice%"=="16" goto run16
if "%choice%"=="17" goto run17
if "%choice%"=="18" goto run18
if "%choice%"=="19" goto run19
if "%choice%"=="20" goto run20
if "%choice%"=="21" goto run21
if "%choice%"=="22" goto run22
if "%choice%"=="23" goto run23
if "%choice%"=="24" goto run24
if "%choice%"=="25" goto run25
if "%choice%"=="26" goto run26
if /i "%choice%"=="q" goto quit
echo Unknown choice: %choice%
goto pause_return

:run1
echo.
echo ^>^>^> nmake -f MakefileWin32.mak clean-all ^&^& nmake -f MakefileWin32.mak all
nmake -f MakefileWin32.mak clean-all
if errorlevel 1 goto done
nmake -f MakefileWin32.mak all
goto done

:run2
echo.
echo ^>^>^> nmake -f MakefileWin32.mak all
nmake -f MakefileWin32.mak all
goto done

:run3
echo.
echo ^>^>^> nmake -f MakefileWin32.mak p1
nmake -f MakefileWin32.mak p1
goto done

:run4
echo.
echo ^>^>^> nmake -f MakefileWin32.mak sfix
nmake -f MakefileWin32.mak sfix
goto done

:run5
echo.
echo ^>^>^> nmake -f MakefileWin32.mak art
nmake -f MakefileWin32.mak art
goto done

:run6
echo.
echo ^>^>^> nmake -f MakefileWin32.mak sound
nmake -f MakefileWin32.mak sound
goto done

:run7
echo.
echo ^>^>^> nmake -f MakefileWin32.mak m1rom
nmake -f MakefileWin32.mak m1rom
goto done

:run8
echo.
echo ^>^>^> nmake -f MakefileWin32.mak vrom
nmake -f MakefileWin32.mak vrom
goto done

:run9
echo.
echo ^>^>^> nmake -f MakefileWin32.mak mml
nmake -f MakefileWin32.mak mml
goto done

:run10
echo.
echo ^>^>^> nmake -f MakefileWin32.mak fm
nmake -f MakefileWin32.mak fm
goto done

:run11
echo.
echo ^>^>^> nmake -f MakefileWin32.mak fmpatches
nmake -f MakefileWin32.mak fmpatches
goto done

:run12
echo.
echo ^>^>^> nmake -f MakefileWin32.mak ssg
nmake -f MakefileWin32.mak ssg
goto done

:run13
echo.
echo ^>^>^> nmake -f MakefileWin32.mak test
nmake -f MakefileWin32.mak test
goto done

:run14
echo.
echo ^>^>^> nmake -f MakefileWin32.mak test-aes
nmake -f MakefileWin32.mak test-aes
goto done

:run15
echo.
echo ^>^>^> nmake -f MakefileWin32.mak debug
nmake -f MakefileWin32.mak debug
goto done

:run16
echo.
echo ^>^>^> nmake -f MakefileWin32.mak mame-trace
nmake -f MakefileWin32.mak mame-trace
goto done

:run17
echo.
echo ^>^>^> nmake -f MakefileWin32.mak debug-build
nmake -f MakefileWin32.mak debug-build
goto done

:run18
echo.
echo ^>^>^> nmake -f MakefileWin32.mak gdb-trace
nmake -f MakefileWin32.mak gdb-trace
goto done

:run19
echo.
echo ^>^>^> nmake -f MakefileWin32.mak gdb
nmake -f MakefileWin32.mak gdb
goto done

:run20
echo.
echo ^>^>^> nmake -f MakefileWin32.mak gdb-remote
nmake -f MakefileWin32.mak gdb-remote
goto done

:run21
echo.
echo ^>^>^> nmake -f MakefileWin32.mak dump
nmake -f MakefileWin32.mak dump
goto done

:run22
echo.
echo ^>^>^> nmake -f MakefileWin32.mak dist
nmake -f MakefileWin32.mak dist
goto done

:run23
echo.
echo ^>^>^> nmake -f MakefileWin32.mak clean
nmake -f MakefileWin32.mak clean
goto done

:run24
echo.
echo ^>^>^> nmake -f MakefileWin32.mak sound-clean
nmake -f MakefileWin32.mak sound-clean
goto done

:run25
echo.
echo ^>^>^> nmake -f MakefileWin32.mak art-clean
nmake -f MakefileWin32.mak art-clean
goto done

:run26
echo.
echo ^>^>^> nmake -f MakefileWin32.mak clean-all
nmake -f MakefileWin32.mak clean-all
goto done

:done
echo.
if errorlevel 1 (echo Exit code: %errorlevel%) else (echo Done.)

:pause_return
echo.
pause
goto menu

:quit
echo Bye.
endlocal
