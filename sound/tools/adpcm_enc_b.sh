#!/bin/sh
for f in ../samples/out_16el_b/*.wav; do
    [ -e "$f" ] || continue
    echo $f
    echo ../samples/out_b/$(basename $f .wav).wav
    ./adpcm_enc a "$f" ../samples/out_b/$(basename $f .wav).adpcmb
done
