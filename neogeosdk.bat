@echo off
setlocal EnableDelayedExpansion
REM #######
REM #https://eaglesoftware.biz
REM #https://github.com/eaglesoftware777
REM #https://github.com/eaglesoftware777/neogeosdk
REM #######
REM NeoGeo SDK - Interactive build menu (Windows)

cd /d "%~dp0"

:menu
cls
echo =============================================
echo    NeoGeo SDK - Build Menu
echo =============================================
echo.
echo   -- FULL BUILD (start here) --
echo    1) make clean-all ^&^& make all               Full clean rebuild MVS  [recommended]
echo    2) make clean-all ^&^& make PLATFORM=aes all  Full clean rebuild AES
echo    3) make all                                   Build everything MVS (no clean)
echo    4) make PLATFORM=aes all                      Build everything AES (no clean)
echo.
echo   -- PROGRAM ROM --
echo    5) make p1                                    MVS P1 ROM only
echo    6) make PLATFORM=aes p1                       AES P1 ROM only
echo.
echo   -- GRAPHICS --
echo    7) make sfix                                  FIX / S ROM (tile graphics)
echo    8) make art                                   Sprite C ROMs
echo.
echo   -- SOUND --
echo    9) make sound                                 Full sound (samples + V ROM + M1)
echo   10) make m1rom                                 Z80 sound driver ROM only
echo   11) make vrom                                  V ROM (ADPCM sample pack)
echo   12) make mml                                   Compile SSG/MML music data
echo   13) make fm                                    Compile FM music data
echo   14) make fmpatches                             Compile FM patch bank
echo   15) make ssg                                   Compile standalone SSG data
echo.
echo   -- TEST / RUN --
echo   16) make test                                  Launch MAME (MVS, US BIOS)  [build first]
echo   17) make test-aes                              Launch MAME (AES)           [build first]
echo   18) make test-mvs                              Launch MAME (MVS, explicit) [build first]
echo   19) make test BIOS=^<name^>                     Launch MAME with specific BIOS (prompts)
echo   20) make bios-list                             Print all supported BIOS names
echo.
echo   -- PACKAGE --
echo   21) make dist                                  Build release package (dist\roms\neogeosdk.zip)
echo.
echo   -- CLEAN --
echo   22) make clean                                 Remove P1 ROM outputs
echo   23) make sound-clean                           Remove sound ROM outputs
echo   24) make art-clean                             Remove art outputs
echo   25) make clean-all                             Full clean (all generated files)
echo.
echo   -- DEBUG / TRACE (advanced) --
echo   26) make debug                                 Launch MAME with CPU debugger (MVS)
echo   27) make debug-aes                             Launch MAME with CPU debugger (AES)
echo   28) make mame-trace                            Trace m68k execution to dump\m68k_trace.txt
echo   29) make debug-build                           Debug build with map/sym artifacts
echo   30) make gdb-trace                             GDB trace dump to dump\gdb_trace.txt
echo   31) make gdb                                   Launch GDB interactive
echo   32) make gdb-remote                            GDB remote attach (MAME -debug)
echo   33) make dump                                  Disassemble and hex dump
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
if "%choice%"=="27" goto run27
if "%choice%"=="28" goto run28
if "%choice%"=="29" goto run29
if "%choice%"=="30" goto run30
if "%choice%"=="31" goto run31
if "%choice%"=="32" goto run32
if "%choice%"=="33" goto run33
if /i "%choice%"=="q" goto quit
echo Unknown choice: %choice%
goto pause_return

:run1
echo.
echo ^>^>^> make -f MakefileWin32.mak clean-all ^&^& make -f MakefileWin32.mak all
make -f MakefileWin32.mak clean-all
if errorlevel 1 goto done
make -f MakefileWin32.mak all
goto done

:run2
echo.
echo ^>^>^> make -f MakefileWin32.mak clean-all ^&^& make -f MakefileWin32.mak PLATFORM=aes all
make -f MakefileWin32.mak clean-all
if errorlevel 1 goto done
make -f MakefileWin32.mak PLATFORM=aes all
goto done

:run3
echo.
echo ^>^>^> make -f MakefileWin32.mak all
make -f MakefileWin32.mak all
goto done

:run4
echo.
echo ^>^>^> make -f MakefileWin32.mak PLATFORM=aes all
make -f MakefileWin32.mak PLATFORM=aes all
goto done

:run5
echo.
echo ^>^>^> make -f MakefileWin32.mak p1
make -f MakefileWin32.mak p1
goto done

:run6
echo.
echo ^>^>^> make -f MakefileWin32.mak PLATFORM=aes p1
make -f MakefileWin32.mak PLATFORM=aes p1
goto done

:run7
echo.
echo ^>^>^> make -f MakefileWin32.mak sfix
make -f MakefileWin32.mak sfix
goto done

:run8
echo.
echo ^>^>^> make -f MakefileWin32.mak art
make -f MakefileWin32.mak art
goto done

:run9
echo.
echo ^>^>^> make -f MakefileWin32.mak sound
make -f MakefileWin32.mak sound
goto done

:run10
echo.
echo ^>^>^> make -f MakefileWin32.mak m1rom
make -f MakefileWin32.mak m1rom
goto done

:run11
echo.
echo ^>^>^> make -f MakefileWin32.mak vrom
make -f MakefileWin32.mak vrom
goto done

:run12
echo.
echo ^>^>^> make -f MakefileWin32.mak mml
make -f MakefileWin32.mak mml
goto done

:run13
echo.
echo ^>^>^> make -f MakefileWin32.mak fm
make -f MakefileWin32.mak fm
goto done

:run14
echo.
echo ^>^>^> make -f MakefileWin32.mak fmpatches
make -f MakefileWin32.mak fmpatches
goto done

:run15
echo.
echo ^>^>^> make -f MakefileWin32.mak ssg
make -f MakefileWin32.mak ssg
goto done

:run16
echo.
echo ^>^>^> make -f MakefileWin32.mak test
make -f MakefileWin32.mak test
goto done

:run17
echo.
echo ^>^>^> make -f MakefileWin32.mak test-aes
make -f MakefileWin32.mak test-aes
goto done

:run18
echo.
echo ^>^>^> make -f MakefileWin32.mak test-mvs
make -f MakefileWin32.mak test-mvs
goto done

:run19
echo.
make -f MakefileWin32.mak bios-list
echo.
set /p "biosname=Enter BIOS name: "
echo.
echo ^>^>^> make -f MakefileWin32.mak test BIOS=%biosname%
make -f MakefileWin32.mak test BIOS=%biosname%
goto done

:run20
echo.
echo ^>^>^> make -f MakefileWin32.mak bios-list
make -f MakefileWin32.mak bios-list
goto done

:run21
echo.
echo ^>^>^> make -f MakefileWin32.mak dist
make -f MakefileWin32.mak dist
goto done

:run22
echo.
echo ^>^>^> make -f MakefileWin32.mak clean
make -f MakefileWin32.mak clean
goto done

:run23
echo.
echo ^>^>^> make -f MakefileWin32.mak sound-clean
make -f MakefileWin32.mak sound-clean
goto done

:run24
echo.
echo ^>^>^> make -f MakefileWin32.mak art-clean
make -f MakefileWin32.mak art-clean
goto done

:run25
echo.
echo ^>^>^> make -f MakefileWin32.mak clean-all
make -f MakefileWin32.mak clean-all
goto done

:run26
echo.
echo ^>^>^> make -f MakefileWin32.mak debug
make -f MakefileWin32.mak debug
goto done

:run27
echo.
echo ^>^>^> make -f MakefileWin32.mak debug-aes
make -f MakefileWin32.mak debug-aes
goto done

:run28
echo.
echo ^>^>^> make -f MakefileWin32.mak mame-trace
make -f MakefileWin32.mak mame-trace
goto done

:run29
echo.
echo ^>^>^> make -f MakefileWin32.mak debug-build
make -f MakefileWin32.mak debug-build
goto done

:run30
echo.
echo ^>^>^> make -f MakefileWin32.mak gdb-trace
make -f MakefileWin32.mak gdb-trace
goto done

:run31
echo.
echo ^>^>^> make -f MakefileWin32.mak gdb
make -f MakefileWin32.mak gdb
goto done

:run32
echo.
echo ^>^>^> make -f MakefileWin32.mak gdb-remote
make -f MakefileWin32.mak gdb-remote
goto done

:run33
echo.
echo ^>^>^> make -f MakefileWin32.mak dump
make -f MakefileWin32.mak dump
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
