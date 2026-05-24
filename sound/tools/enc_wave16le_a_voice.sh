#!/bin/sh
# Voice alphabet WAV → 16-bit PCM (intermediate step before ADPCM-A
# encoding).  Mirrors enc_wave16le_a.sh but reads from samples/
# in_wav_a_voice (the alphabet WAVs a.wav..z.wav) and writes to
# samples/out_16el_a_voice.
PYTHON_BIN="${PYTHON:-python3}"
SOX_BIN="${SOX:-}"
SAMPLES_IN="${GAME_SOUND:-..}/samples/in_wav_a_voice"
SAMPLES_OUT="${GAME_SOUND:-..}/samples/out_16el_a_voice"
mkdir -p "$SAMPLES_OUT"

if [ ! -d "$SAMPLES_IN" ]; then
    echo "[voice] $SAMPLES_IN not found — skipping voice sample stage"
    exit 0
fi

for fi in "$SAMPLES_IN"/*.wav; do
    [ -e "$fi" ] || continue
    if [ -n "$SOX_BIN" ] && command -v "$SOX_BIN" >/dev/null 2>&1; then
        "$SOX_BIN" "$fi" -b 16 -c 1 -r 18500 -e signed-integer -t raw "$SAMPLES_OUT/$(basename "$fi" .wav).wav"
    else
        "$PYTHON_BIN" ./wav_to_raw_pcm.py "$fi" "$SAMPLES_OUT/$(basename "$fi" .wav).wav" --rate 18500
    fi
done
