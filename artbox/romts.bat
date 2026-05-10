if "%SDKHOME%"=="" set SDKHOME=%~dp0..\..
..\win\romtool_x64.exe /p 1c.c1 1p.c1 2048 0
..\win\romtool_x64.exe /p 2c.c2 2p.c2 2048 0
..\win\romtool_x64.exe /f 1p.c1 777-c1.c1
..\win\romtool_x64.exe /f 2p.c2 777-c2.c2
copy /Y 777-c1.c1  %SDKHOME%\neogeosdk\roms\neogeosdk
copy /Y 777-c2.c2  %SDKHOME%\neogeosdk\roms\neogeosdk
