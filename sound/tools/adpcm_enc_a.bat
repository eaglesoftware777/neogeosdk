@echo off
if not defined PY set "PY=py"
for %%f in (..\samples\out_16el_a\*.wav) do (
    echo %%f
    echo ..\samples\out_a\%%~nf.adpcma
    "%PY%" adpcm_enc.py a "%%f" "..\samples\out_a\%%~nf.adpcma"
)
