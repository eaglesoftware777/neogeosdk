@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0backup_current.ps1"
exit /b %ERRORLEVEL%
