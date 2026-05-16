if "%GAME_ID%"=="" set GAME_ID=777
if "%GAME%"=="" set GAME=demo
for %%i in ("%~dp0..") do set REPO_ROOT=%%~fi
..\win\romtool_x64.exe /p 1c.c1 1p.c1 2048 0
..\win\romtool_x64.exe /p 2c.c2 2p.c2 2048 0
..\win\romtool_x64.exe /f 1p.c1 "%GAME_ID%-c1.c1"
..\win\romtool_x64.exe /f 2p.c2 "%GAME_ID%-c2.c2"
if not exist "%REPO_ROOT%\roms\%GAME%" mkdir "%REPO_ROOT%\roms\%GAME%"
copy /Y "%GAME_ID%-c1.c1" "%REPO_ROOT%\roms\%GAME%\%GAME_ID%-c1.c1"
copy /Y "%GAME_ID%-c2.c2" "%REPO_ROOT%\roms\%GAME%\%GAME_ID%-c2.c2"
