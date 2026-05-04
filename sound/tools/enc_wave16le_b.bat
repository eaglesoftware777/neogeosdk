@echo off
for %%f in (..\samples\in_wav_b\*.wav) do (
    echo Processing %%f
    sox "%%f" -b 16 -c 1 -r 16000 -e signed-integer -t raw "..\samples\out_16el_b\%%~nf.wav"
)
