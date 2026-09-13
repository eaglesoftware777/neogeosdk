@echo off
call adpcm_enc_a.bat
if errorlevel 1 exit /b 1
call adpcm_enc_a_voice.bat
if errorlevel 1 exit /b 1
call adpcm_enc_b.bat
if errorlevel 1 exit /b 1
