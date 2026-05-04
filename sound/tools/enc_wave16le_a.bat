@echo off
for %%f in (..\samples\in_wav_a\*.wav) do (
    echo Processing %%f
    sox "%%f" -b 16 -c 1 -r 18500 -e signed-integer -t raw "..\samples\out_16el_a\%%~nf.wav"
)
