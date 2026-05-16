if "%GAME_ID%"=="" set GAME_ID=777
if "%GAME%"=="" set GAME=demo
if "%CROM_PAD_KB%"=="" set CROM_PAD_KB=8192
for %%i in ("%~dp0..") do set REPO_ROOT=%%~fi
set GAME_ARTBOX=%REPO_ROOT%\games\%GAME%\artbox
if "%SDKHOME%"=="" set SDKHOME=%~dp0..\..

cd /d "%GAME_ARTBOX%" || exit /b 1
py "%~dp0romtool.py" --dir "%GAME_ARTBOX%" /p 1c.c1 1p.c1 %CROM_PAD_KB% 0 || exit /b 1
py "%~dp0romtool.py" --dir "%GAME_ARTBOX%" /p 2c.c2 2p.c2 %CROM_PAD_KB% 0 || exit /b 1
py "%~dp0romtool.py" --dir "%GAME_ARTBOX%" /f 1p.c1 %GAME_ID%-c1.c1 || exit /b 1
py "%~dp0romtool.py" --dir "%GAME_ARTBOX%" /f 2p.c2 %GAME_ID%-c2.c2 || exit /b 1

if not exist "%REPO_ROOT%\roms\%GAME%" mkdir "%REPO_ROOT%\roms\%GAME%"
copy /Y "%GAME_ARTBOX%\%GAME_ID%-c1.c1" "%REPO_ROOT%\roms\%GAME%\%GAME_ID%-c1.c1"
copy /Y "%GAME_ARTBOX%\%GAME_ID%-c2.c2" "%REPO_ROOT%\roms\%GAME%\%GAME_ID%-c2.c2"
exit /b 0
