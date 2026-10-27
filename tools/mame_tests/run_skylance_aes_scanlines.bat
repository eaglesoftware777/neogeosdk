@echo off
setlocal
set "MAME_SCANLINES=1"
call "%~dp0run_skylance.bat" aes %*
exit /b %ERRORLEVEL%
