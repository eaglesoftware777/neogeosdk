#!/bin/sh
set -eu
PYTHON_BIN="${PYTHON:-python3}"
SOX_BIN="${SOX:-}"
SAMPLES_IN="${GAME_SOUND:-..}/samples/in_wav_a"
SAMPLES_OUT="${GAME_SOUND:-..}/samples/out_16el_a"
mkdir -p "$SAMPLES_OUT"

for fi in "$SAMPLES_IN"/*.wav; do
    [ -e "$fi" ] || continue
    # converted already, from this file and by this converter: a game with hundreds of
    # voice samples would otherwise convert them all on every build
    fo="$SAMPLES_OUT/$(basename "$fi" .wav).wav"
    if [ "$fo" -nt "$fi" ] && [ "$fo" -nt ./wav_to_raw_pcm.py ]; then continue; fi
    if [ -n "$SOX_BIN" ] && command -v "$SOX_BIN" >/dev/null 2>&1; then
        "$SOX_BIN" "$fi" -b 16 -c 1 -r 18500 -e signed-integer -t raw "$SAMPLES_OUT/$(basename "$fi" .wav).wav"
    else
        "$PYTHON_BIN" ./wav_to_raw_pcm.py "$fi" "$SAMPLES_OUT/$(basename "$fi" .wav).wav" --rate 18500
    fi
done
