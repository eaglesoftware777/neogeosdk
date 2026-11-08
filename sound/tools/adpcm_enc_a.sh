#!/bin/sh
set -eu
PYTHON_BIN="${PYTHON:-python3}"
BASE="${GAME_SOUND:-..}/samples"
mkdir -p "$BASE/out_a"
for f in "$BASE"/out_16el_a/*.wav; do
    [ -e "$f" ] || continue
    # encoded already, from this file and by this encoder
    o="$BASE/out_a/$(basename "$f" .wav).adpcma"
    if [ "$o" -nt "$f" ] && [ "$o" -nt ./adpcm_enc.py ]; then continue; fi
    echo $f
    echo "$BASE/out_a/$(basename $f .wav).adpcma"
    "$PYTHON_BIN" ./adpcm_enc.py a "$f" "$BASE/out_a/$(basename "$f" .wav).adpcma"
done
