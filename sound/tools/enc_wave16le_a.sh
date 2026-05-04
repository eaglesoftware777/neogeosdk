#!/bin/sh
for fi in ../samples/in_wav_a/*.wav; do
    [ -e "$fi" ] || continue
    sox "$fi" -b 16 -c 1 -r 18500 -e signed-integer -t raw ../samples/out_16el_a/$(basename $fi .wav).wav
done
