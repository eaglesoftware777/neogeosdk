# Voice recordings

The announcer, the commentator, the referee, the coach, the players' shouts,
every nation's chant and every shirt name are speech synthesis, recorded by
`tools/voices.py` and processed there (crowd chants are layered and given
claps or drums).

Kokoro-82M model, Apache License 2.0:
https://huggingface.co/hexgrad/Kokoro-82M

Inference tool: kokoro-onnx (MIT):
https://github.com/thewh1teagle/kokoro-onnx

Voices used: am_adam, am_michael, am_puck, bm_george, em_alex, im_nicola,
pm_alex and jm_kumo.

Japanese voice jm_kumo (the coach) uses the Koniwa kumonoito reading, CC BY 3.0.
Attribution: Koniwa contributors, https://github.com/koniwa/koniwa ;
https://github.com/koniwa/koniwa/blob/master/source/tnc/tnc__kumonoito.txt
Full voice/model sources: https://huggingface.co/hexgrad/Kokoro-82M/blob/main/VOICES.md

Every name spoken is a made-up parody name: no real player, team or official
is named. Sound effects, crowd beds and music are synthesised by
`tools/sfx.py` and `tools/music.py`.
