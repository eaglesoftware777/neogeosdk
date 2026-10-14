@echo off
setlocal
set "MODE=%~1"
if "%MODE%"=="" set "MODE=mvs"
echo Starting current %MODE% build. Close MAME to start the saved version.
call "%~dp0run_maiya.bat" %MODE% %2
if errorlevel 1 exit /b %ERRORLEVEL%
echo Starting saved %MODE% build.
call "%~dp0run_backup.bat" %MODE% %2
exit /b %ERRORLEVEL%
