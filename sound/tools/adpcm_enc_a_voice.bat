@echo off
rem Voice 16-bit PCM -> ADPCM-A. Mirrors adpcm_enc_a.bat but operates on
rem samples\out_16el_a_voice -> samples\out_a_voice.
if not defined PY set "PY=py"
if not defined GAME_SOUND set "GAME_SOUND=.."
set "SAMPLES_BASE=%GAME_SOUND%\samples"
if not exist "%SAMPLES_BASE%\out_16el_a_voice" (
    echo [voice] %SAMPLES_BASE%\out_16el_a_voice not found -- skipping voice ADPCM-A stage
    exit /b 0
)
if not exist "%SAMPLES_BASE%\out_a_voice" mkdir "%SAMPLES_BASE%\out_a_voice"
for %%f in ("%SAMPLES_BASE%\out_16el_a_voice\*.wav") do (
    echo %%f
    echo %SAMPLES_BASE%\out_a_voice\%%~nf.adpcma
    "%PY%" adpcm_enc.py a "%%f" "%SAMPLES_BASE%\out_a_voice\%%~nf.adpcma"
    if errorlevel 1 exit /b 1
)
