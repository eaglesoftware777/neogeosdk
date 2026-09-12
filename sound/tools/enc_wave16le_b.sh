#!/bin/sh
set -eu
PYTHON_BIN="${PYTHON:-python3}"
SOX_BIN="${SOX:-}"
RATE="${ADPCMB_RATE:-32000}"
SAMPLES_IN="${GAME_SOUND:-..}/samples/in_wav_b"
SAMPLES_OUT="${GAME_SOUND:-..}/samples/out_16el_b"
mkdir -p "$SAMPLES_OUT"

for fi in "$SAMPLES_IN"/*.wav; do
    [ -e "$fi" ] || continue
    if [ -n "$SOX_BIN" ] && command -v "$SOX_BIN" >/dev/null 2>&1; then
        "$SOX_BIN" "$fi" -b 16 -c 1 -r "$RATE" -e signed-integer -t raw "$SAMPLES_OUT/$(basename "$fi" .wav).wav"
        "$PYTHON_BIN" ./pcm_metadata.py "$SAMPLES_OUT/$(basename "$fi" .wav).wav" --rate "$RATE"
    else
        "$PYTHON_BIN" ./wav_to_raw_pcm.py "$fi" "$SAMPLES_OUT/$(basename "$fi" .wav).wav" --rate "$RATE"
    fi
done
