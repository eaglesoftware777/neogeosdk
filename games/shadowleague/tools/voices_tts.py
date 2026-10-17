"""Speak a batch of lines with a multi-speaker Piper model (run by voices.py).

    PIPER_PYTHON voices_tts.py MODEL.onnx BATCH.json OUT_DIR

BATCH.json: [{"key", "text", "speaker"}]. Writes OUT_DIR/<key>.wav (22050 Hz).
"""
import json
import sys
import wave
from pathlib import Path

from piper import PiperVoice, SynthesisConfig


def main():
    model, batch, out = sys.argv[1], json.loads(Path(sys.argv[2]).read_text()), Path(sys.argv[3])
    voice = PiperVoice.load(model)
    for item in batch:
        # A little quicker and with more variation than reading aloud.
        config = SynthesisConfig(speaker_id=item["speaker"], length_scale=item.get("length", 0.85),
                                 noise_scale=0.9, noise_w_scale=1.0)
        with wave.open(str(out / f"{item['key']}.wav"), "wb") as w:
            voice.synthesize_wav(item["text"], w, config)


if __name__ == "__main__":
    main()
