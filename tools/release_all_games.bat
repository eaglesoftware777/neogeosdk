@echo off
rem The release pipeline (tools/release_all_games.sh) run in WSL: builds every
rem game, checks it, installs the MAME test folder and writes
rem docs\qa\PIPELINE_RUN.md. It never publishes or replaces a released file.
rem   release_all_games.bat [--games "skylance nosignal"] [--quick]
setlocal
pushd "%~dp0.."
wsl.exe -e bash -lc "cd \"$(wslpath -a \"%CD%\")\" && tools/release_all_games.sh %*"
set "RC=%ERRORLEVEL%"
popd
echo.
if "%RC%"=="0" (echo   Every mandatory step passed.) else (echo   A mandatory step failed: see docs\qa\PIPELINE_RUN.md)
pause
exit /b %RC%
