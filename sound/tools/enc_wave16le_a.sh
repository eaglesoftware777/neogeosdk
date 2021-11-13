#!/bin/sh
#for fi in ../samples/in_wav_a/*.wav; do
#    [ -e "$fi" ] || continue
#    sox "$fi" -b 16  ../samples/out_sr_a/$(basename $fi .wav).wav
#done
for fi in ../samples/in_wav_a/*.wav; do
    [ -e "$fi" ] || continue
    cp "$fi"  ../samples/out_sr_a/$(basename $fi .wav).wav
done
for f in ../samples/out_sr_a/*.wav; do
    [ -e "$f" ] || continue
    ffmpeg -y -i "$f" -f s16le -acodec pcm_s16le  -ar 16000 ../samples/out_16el_a/$(basename $f .wav).wav
done
