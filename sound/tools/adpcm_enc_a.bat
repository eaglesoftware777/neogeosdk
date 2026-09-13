@echo off
if not defined PY set "PY=py"
if not defined GAME_SOUND set "GAME_SOUND=.."
set "SAMPLES_BASE=%GAME_SOUND%\samples"
if not exist "%SAMPLES_BASE%\out_a" mkdir "%SAMPLES_BASE%\out_a"
for %%f in ("%SAMPLES_BASE%\out_16el_a\*.wav") do (
    echo %%f
    echo %SAMPLES_BASE%\out_a\%%~nf.adpcma
    "%PY%" adpcm_enc.py a "%%f" "%SAMPLES_BASE%\out_a\%%~nf.adpcma"
    if errorlevel 1 exit /b 1
)
