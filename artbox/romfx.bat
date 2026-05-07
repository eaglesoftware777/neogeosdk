@echo off
if "%SDKHOME%"=="" set SDKHOME=%~dp0..\..
if not exist 052-s1.s1 (
    echo Missing 052-s1.s1. Run romdbfiximport.py and fixtiles.py first.
    exit /b 1
)
copy /Y 052-s1.s1 %SDKHOME%\neogeosdk\roms\ssideki\052-s1.s1 >nul
