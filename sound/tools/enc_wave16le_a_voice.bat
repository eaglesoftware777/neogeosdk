@echo off
rem Voice WAV -> 16-bit PCM (intermediate step before ADPCM-A encoding).
rem Mirrors enc_wave16le_a.bat but reads from samples\in_wav_a_voice and
rem writes to samples\out_16el_a_voice.
setlocal
if not defined PY set "PY=py"
if not defined GAME_SOUND set "GAME_SOUND=.."
set "SAMPLES_IN=%GAME_SOUND%\samples\in_wav_a_voice"
set "SAMPLES_OUT=%GAME_SOUND%\samples\out_16el_a_voice"
if not exist "%SAMPLES_IN%" (
    echo [voice] %SAMPLES_IN% not found -- skipping voice sample stage
    endlocal & exit /b 0
)
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
    ) else (
        "%PY%" wav_to_raw_pcm.py "%%f" "%SAMPLES_OUT%\%%~nf.wav" --rate 18500
    )
)
endlocal
