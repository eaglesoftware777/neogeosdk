#!/bin/sh
for f in ../samples/out_16el_a/*.wav; do
    [ -e "$f" ] || continue
    echo $f
    echo ../samples/out_a/$(basename $f .wav).adpcma
    python3 ./adpcm_enc.py a "$f" ../samples/out_a/$(basename $f .wav).adpcma
done
