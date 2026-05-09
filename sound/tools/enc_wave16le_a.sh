#!/bin/sh
PYTHON_BIN="${PYTHON:-python3}"
SOX_BIN="${SOX:-}"

for fi in ../samples/in_wav_a/*.wav; do
    [ -e "$fi" ] || continue
    if [ -n "$SOX_BIN" ] && command -v "$SOX_BIN" >/dev/null 2>&1; then
        "$SOX_BIN" "$fi" -b 16 -c 1 -r 18500 -e signed-integer -t raw ../samples/out_16el_a/$(basename "$fi" .wav).wav
    else
        "$PYTHON_BIN" ./wav_to_raw_pcm.py "$fi" ../samples/out_16el_a/$(basename "$fi" .wav).wav --rate 18500
    fi
done
