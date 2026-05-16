#!/bin/sh
PYTHON_BIN="${PYTHON:-python3}"
BASE="${GAME_SOUND:-..}/samples"
mkdir -p "$BASE/out_a"
for f in "$BASE"/out_16el_a/*.wav; do
    [ -e "$f" ] || continue
    echo $f
    echo "$BASE/out_a/$(basename $f .wav).adpcma"
    "$PYTHON_BIN" ./adpcm_enc.py a "$f" "$BASE/out_a/$(basename "$f" .wav).adpcma"
done
