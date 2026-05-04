#!/bin/sh
for fi in ../samples/in_wav_b/*.wav; do
    [ -e "$fi" ] || continue
    sox "$fi" -b 16 -c 1 -r 16000 -e signed-integer -t raw ../samples/out_16el_b/$(basename $fi .wav).wav
done
