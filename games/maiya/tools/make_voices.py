#!/usr/bin/env python3
"""Speak Maiya's voice lines into games/maiya/sound/samples/in_wav_a_voice/.

    python3 games/maiya/tools/make_voices.py --model <en_US-ljspeech-high.onnx> \
        --luna-model <en_US-kristin-medium.onnx> [--missing]

Each line is spoken by the piper speech synthesiser with its LJ Speech
voice (trained on public-domain recordings) -- Luna's with the Kristin
voice (trained on LibriVox's public-domain recordings) -- then given its
speaker's colour with sox -- Maiya a little younger and brighter, the maiden younger
still, the elder deeper and slower, the spirit airy with an echo, Sunboy a
boy's pitch -- trimmed, levelled and written as 16-bit mono WAV. The build
encodes that folder as the ADPCM-A voice bank, after the sixteen effects:
the game plays line k as sample SOUND_SFX_COUNT + k, in the file-name order
below (see MG_VOICE_* in maiya_game.c). The WAVs are committed; this is
only needed to change a line. --missing speaks only the lines with no WAV
yet, so appending lines leaves the others byte for byte.
"""

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
OUT = GAME / "sound" / "samples" / "in_wav_a_voice"

# (file, speaker, words). The order is the sample order: keep it in step
# with MG_VOICE_* in maiya_game.c, and only ever append.
LINES = (
    ("v01_rise", "maiya", "Rising bloom!"),
    ("v02_surge", "maiya", "Rose blossom surge!"),
    ("v03_art", "maiya", "Nature, heal this land!"),
    ("v04_leap", "maiya", "Up we go!"),
    ("v05_lily", "maiya", "Light as air!"),
    ("v06_free", "maiya", "You're free now!"),
    ("v07_start", "maiya", "The valley called me!"),
    ("v08_retry", "maiya", "I'm not done yet!"),
    ("v09_win", "maiya", "The valley is healed!"),
    ("v10_elder", "elder", "Bless you, child of nature!"),
    ("v11_maiden", "maiden", "Thank you! You saved me!"),
    ("v12_spirit", "spirit", "The forest thanks you!"),
    ("v13_sunboy", "sunboy", "We did it! Thank you!"),
    # Luna says what Maiya says, in her own words and her own voice
    # (MG_VOICE_LUNA: the first nine lines again, in the same order).
    ("v14_luna_rise", "luna", "Bloom, rise up!"),
    ("v15_luna_surge", "luna", "Blossom rush!"),
    ("v16_luna_art", "luna", "Earth, make this valley whole!"),
    ("v17_luna_leap", "luna", "Here I go!"),
    ("v18_luna_lily", "luna", "Like a feather!"),
    ("v19_luna_free", "luna", "Go on, you're safe!"),
    ("v20_luna_start", "luna", "I heard the valley cry!"),
    ("v21_luna_retry", "luna", "Not over yet!"),
    ("v22_luna_win", "luna", "The land lives again!"),
)

# sox effects per speaker: pitch in cents, tempo, and colour.
SPEAKERS = {
    "maiya":  ["pitch", "250", "tempo", "1.08"],
    "maiden": ["pitch", "450", "tempo", "1.05"],
    "elder":  ["pitch", "-420", "tempo", "0.92", "bass", "4"],
    "spirit": ["pitch", "150", "tempo", "0.96", "reverb", "45", "echo", "0.8", "0.7", "60", "0.3"],
    "sunboy": ["pitch", "600", "tempo", "1.1"],
    "luna":   ["gain", "-6", "pitch", "120", "tempo", "1.04", "treble", "2"],
}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--model", required=True, help="the piper voice model (.onnx, its .onnx.json beside it)")
    ap.add_argument("--luna-model", required=True, help="Luna's piper voice model (.onnx)")
    ap.add_argument("--missing", action="store_true", help="speak only the lines with no WAV yet")
    ap.add_argument("--piper", default="piper", help="the piper command")
    args = ap.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        for name, speaker, words in LINES:
            if args.missing and (OUT / f"{name}.wav").is_file():
                continue
            raw = Path(tmp) / f"{name}.wav"
            model = args.luna_model if speaker == "luna" else args.model
            subprocess.run([args.piper, "--model", model, "--output_file", str(raw)],
                           input=words, text=True, check=True, capture_output=True)
            out = OUT / f"{name}.wav"
            subprocess.run(["sox", str(raw), "-b", "16", "-c", "1", str(out),
                            *SPEAKERS[speaker],
                            # trim the silence at both ends, then level it
                            "silence", "1", "0.02", "1%", "reverse", "silence", "1", "0.02", "1%", "reverse",
                            "gain", "-n", "-1", "rate", "22050"],
                           check=True)
            print(f"{out.relative_to(GAME.parents[1])}: {speaker}: {words}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
