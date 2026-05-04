@echo off
for %%f in (..\samples\out_16el_b\*.wav) do (
    echo %%f
    echo ..\samples\out_b\%%~nf.adpcmb
    py adpcm_enc.py b "%%f" "..\samples\out_b\%%~nf.adpcmb"
)
