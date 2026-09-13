@echo off
setlocal
if not defined PY set "PY=py"
if not defined ADPCMB_RATE set "ADPCMB_RATE=32000"
if not defined GAME_SOUND set "GAME_SOUND=.."
set "SAMPLES_IN=%GAME_SOUND%\samples\in_wav_b"
set "SAMPLES_OUT=%GAME_SOUND%\samples\out_16el_b"
if not exist "%SAMPLES_OUT%" mkdir "%SAMPLES_OUT%"
set "USE_SOX="
if defined SOX (
    where "%SOX%" >nul 2>nul && set "USE_SOX=1"
    if exist "%SOX%" set "USE_SOX=1"
)

for %%f in ("%SAMPLES_IN%\*.wav") do (
    echo Processing %%f
    if defined USE_SOX (
        "%SOX%" "%%f" -b 16 -c 1 -r %ADPCMB_RATE% -e signed-integer -t raw "%SAMPLES_OUT%\%%~nf.wav"
        if errorlevel 1 exit /b 1
        "%PY%" pcm_metadata.py "%SAMPLES_OUT%\%%~nf.wav" --rate %ADPCMB_RATE%
    ) else (
        "%PY%" wav_to_raw_pcm.py "%%f" "%SAMPLES_OUT%\%%~nf.wav" --rate %ADPCMB_RATE%
    )
    if errorlevel 1 exit /b 1
)
endlocal
