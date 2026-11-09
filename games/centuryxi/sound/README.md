FM loops and patches live in `fm/`: track 0 the match, 1 the goal sting, 2 the title, 3 the champions. SSG tracks are the menu's blips.

`samples/in_wav_a` holds the match's ADPCM-A sounds and `in_wav_b` the crowd's loop. `tools/sfx.py` synthesises all of them, so run it when one changes; the WAVs are what the build encodes. `1.wav` is sample 0, in the order of the `SFX_` list in `scenes/cx_audio.c`.
