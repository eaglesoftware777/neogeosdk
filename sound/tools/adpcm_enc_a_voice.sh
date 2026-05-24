#!/bin/sh
# Voice alphabet 16-bit PCM → ADPCM-A.  Mirrors adpcm_enc_a.sh but
# operates on samples/out_16el_a_voice → samples/out_a_voice.
PYTHON_BIN="${PYTHON:-python3}"
BASE="${GAME_SOUND:-..}/samples"
mkdir -p "$BASE/out_a_voice"

if [ ! -d "$BASE/out_16el_a_voice" ]; then
    echo "[voice] $BASE/out_16el_a_voice not found — skipping voice ADPCM-A stage"
    exit 0
fi

for f in "$BASE"/out_16el_a_voice/*.wav; do
    [ -e "$f" ] || continue
    echo "$f"
    echo "$BASE/out_a_voice/$(basename "$f" .wav).adpcma"
    "$PYTHON_BIN" ./adpcm_enc.py a "$f" "$BASE/out_a_voice/$(basename "$f" .wav).adpcma"
done
