@echo off
if not defined PY set "PY=py"
for %%f in (..\samples\out_16el_b\*.wav) do (
    echo %%f
    echo ..\samples\out_b\%%~nf.adpcmb
    "%PY%" adpcm_enc.py b "%%f" "..\samples\out_b\%%~nf.adpcmb"
)
