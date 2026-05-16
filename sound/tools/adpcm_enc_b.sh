#!/bin/sh
PYTHON_BIN="${PYTHON:-python3}"
BASE="${GAME_SOUND:-..}/samples"
mkdir -p "$BASE/out_b"
for f in "$BASE"/out_16el_b/*.wav; do
    [ -e "$f" ] || continue
    echo $f
    echo "$BASE/out_b/$(basename $f .wav).adpcmb"
    "$PYTHON_BIN" ./adpcm_enc.py b "$f" "$BASE/out_b/$(basename "$f" .wav).adpcmb"
done
