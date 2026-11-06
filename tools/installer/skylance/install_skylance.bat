@echo off
rem Sky Lance v1 setup: see install_skylance.ps1. Optional: -Target <folder>
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0install_skylance.ps1" %*
exit /b %ERRORLEVEL%
