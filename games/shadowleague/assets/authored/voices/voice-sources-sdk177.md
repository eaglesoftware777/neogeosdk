# Localized combat recordings

Kokoro-82M model, Apache License 2.0:
https://huggingface.co/hexgrad/Kokoro-82M

Inference tool: kokoro-onnx (MIT):
https://github.com/thewh1teagle/kokoro-onnx

Japanese voice jm_kumo uses the Koniwa kumonoito reading, CC BY 3.0.
Attribution: Koniwa contributors, https://github.com/koniwa/koniwa ;
https://github.com/koniwa/koniwa/blob/master/source/tnc/tnc__kumonoito.txt
French voice ff_siwis uses the SIWIS French Speech Synthesis Database, CC BY 4.0.
Attribution: University of Edinburgh SIWIS contributors,
https://datashare.ed.ac.uk/handle/10283/2353
Full voice/model sources: https://huggingface.co/hexgrad/Kokoro-82M/blob/main/VOICES.md

These original short combat lines replace the existing sample keys for 15 fighters.
Japanese, French, Brazilian Portuguese, Spanish, Mandarin, Hindi and British
English are included. Hindi is used for Arun in this first localization pass;
this does not establish a Malayalam performance. Other cast members retain
previous English or mechanical recordings; full native-language coverage and
native-speaker direction remain unfinished. Male/female/robot shared grunts
and the announcer are unchanged.

Rendering: tools/voices_localized.py; text/voice assignments: tools/voice_locales.py.
Recorded samples: mono 18519 Hz PCM16; final dynamics/equalization/reverb are
recorded in tools/voices.py. localized-recordings.json lists every output.
