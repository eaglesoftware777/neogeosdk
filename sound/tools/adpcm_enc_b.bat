@echo off
if not defined PY set "PY=py"
if not defined GAME_SOUND set "GAME_SOUND=.."
set "SAMPLES_BASE=%GAME_SOUND%\samples"
if not exist "%SAMPLES_BASE%\out_b" mkdir "%SAMPLES_BASE%\out_b"
for %%f in ("%SAMPLES_BASE%\out_16el_b\*.wav") do (
    echo %%f
    echo %SAMPLES_BASE%\out_b\%%~nf.adpcmb
    "%PY%" adpcm_enc.py b "%%f" "%SAMPLES_BASE%\out_b\%%~nf.adpcmb"
    if errorlevel 1 exit /b 1
)
