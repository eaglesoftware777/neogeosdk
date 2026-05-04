@echo off
for %%f in (..\samples\out_16el_a\*.wav) do (
    echo %%f
    echo ..\samples\out_a\%%~nf.adpcma
    py adpcm_enc.py a "%%f" "..\samples\out_a\%%~nf.adpcma"
)
