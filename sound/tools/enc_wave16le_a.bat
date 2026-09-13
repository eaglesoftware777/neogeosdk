@echo off
setlocal
if not defined PY set "PY=py"
if not defined GAME_SOUND set "GAME_SOUND=.."
set "SAMPLES_IN=%GAME_SOUND%\samples\in_wav_a"
set "SAMPLES_OUT=%GAME_SOUND%\samples\out_16el_a"
if not exist "%SAMPLES_OUT%" mkdir "%SAMPLES_OUT%"
set "USE_SOX="
if defined SOX (
    where "%SOX%" >nul 2>nul && set "USE_SOX=1"
    if exist "%SOX%" set "USE_SOX=1"
)

for %%f in ("%SAMPLES_IN%\*.wav") do (
    echo Processing %%f
    if defined USE_SOX (
        "%SOX%" "%%f" -b 16 -c 1 -r 18500 -e signed-integer -t raw "%SAMPLES_OUT%\%%~nf.wav"
        if errorlevel 1 exit /b 1
        "%PY%" pcm_metadata.py "%SAMPLES_OUT%\%%~nf.wav" --rate 18500
    ) else (
        "%PY%" wav_to_raw_pcm.py "%%f" "%SAMPLES_OUT%\%%~nf.wav" --rate 18500
    )
    if errorlevel 1 exit /b 1
)
endlocal
