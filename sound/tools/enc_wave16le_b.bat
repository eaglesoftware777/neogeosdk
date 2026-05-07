@echo off
setlocal
if not defined PY set "PY=py"
if not defined SOX set "SOX=sox"
set "USE_SOX="
where "%SOX%" >nul 2>nul && set "USE_SOX=1"
if exist "%SOX%" set "USE_SOX=1"

for %%f in (..\samples\in_wav_b\*.wav) do (
    echo Processing %%f
    if defined USE_SOX (
        "%SOX%" "%%f" -b 16 -c 1 -r 16000 -e signed-integer -t raw "..\samples\out_16el_b\%%~nf.wav"
    ) else (
        "%PY%" wav_to_raw_pcm.py "%%f" "..\samples\out_16el_b\%%~nf.wav" --rate 16000
    )
)
endlocal
