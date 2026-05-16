if "%GAME_ID%"=="" set GAME_ID=777
if "%SDKHOME%"=="" set SDKHOME=%~dp0..\..
..\win\romtool_x64.exe /p 1c.c1 1p.c1 2048 0
..\win\romtool_x64.exe /p 2c.c2 2p.c2 2048 0
..\win\romtool_x64.exe /f 1p.c1 "%GAME_ID%-c1.c1"
..\win\romtool_x64.exe /f 2p.c2 "%GAME_ID%-c2.c2"
copy /Y "%GAME_ID%-c1.c1" "%SDKHOME%\neogeosdk\roms\neogeosdk"
copy /Y "%GAME_ID%-c2.c2" "%SDKHOME%\neogeosdk\roms\neogeosdk"
