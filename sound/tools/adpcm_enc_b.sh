#!/bin/sh
PYTHON_BIN="${PYTHON:-python3}"
for f in ../samples/out_16el_b/*.wav; do
    [ -e "$f" ] || continue
    echo $f
    echo ../samples/out_b/$(basename $f .wav).adpcmb
    "$PYTHON_BIN" ./adpcm_enc.py b "$f" ../samples/out_b/$(basename "$f" .wav).adpcmb
done
