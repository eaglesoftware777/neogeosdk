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

Piper voices (piper-tts, MIT), for the lines Kokoro has no voice for:
- Russian "dmitri" (medium), from the OHF-Voice voice-datasets collection, CC0.
- Ukrainian "mykyta" (high), from the ukrainian-tts-datasets collection, Apache License 2.0.
- French "upmc" (medium, speakers "pierre" and "jessica"), from the MaryTTS
  upmc-pierre-data corpus, CC BY-SA 4.0. The recordings made with it (Malik,
  Idris and Leila's localized lines) are shared under CC BY-SA 4.0 as well.

21 fighters speak their specials, super, heavy blow, throw and win in their own
language: Japanese (Kenji, Ryuhei, Hayato), Mandarin (Lin Wei, Chen Bao, Xiao
Mei), French (Claire, Aisha, Malik, Idris, Leila), Brazilian Portuguese (Amara,
Lucas), Spanish (Diego, Tomas, El Toro Rojo), Hindi (Arun, standing in for
Malayalam; Zara, for Urdu), British English (Kiran), Russian (Aleksei) and
Ukrainian (Viktor). Every other fighter speaks English: their language has no
voice here whose terms are clear (Irina and Fahri among them: the Russian
women's and Turkish voices on offer are unlicensed or non-commercial). The
short grunts -- the light blow's shout, being hurt, the knock-out -- are the
same kind for everyone, in each fighter's own speaker; the announcer is unchanged.

Rendering: tools/voices_localized.py (Kokoro) and tools/voices_localized_piper.py
(Piper); text/voice assignments: tools/voice_locales.py.
Recorded samples: mono 18519 Hz PCM16; final dynamics/equalization/reverb are
recorded in tools/voices.py. localized-recordings.json lists every output.
