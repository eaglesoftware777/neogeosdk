#!/bin/sh
for f in ../samples/out_16el_b/*.wav; do
    [ -e "$f" ] || continue
    echo $f
    echo ../samples/out_b/$(basename $f .wav).adpcmb
    python3 ./adpcm_enc.py b "$f" ../samples/out_b/$(basename $f .wav).adpcmb
done
