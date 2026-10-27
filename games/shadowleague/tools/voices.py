"""Record Shadow League's voices into assets/authored/voices/ (18519 Hz, the ADPCM-A rate).

The speech comes from Piper with the LibriTTS-R multi-speaker English model
(dataset licence CC BY 4.0, credited in the game's notes); the lines and the
speakers are in slf_voices.py. Piper runs in its own environment:

    python3 games/shadowleague/tools/voices.py [--piper ~/.cache/slf-voice/venv/bin/python]
        [--model ~/.cache/slf-voice/models/en_US-libritts_r-medium.onnx] [--only KEY...]

Each line is trimmed, given a shouted edge (saturation, presence, a short
slap-back room), the announcer a larger hall, and the machines a ring-modulated
metallic tone. sound.py encodes the results.
"""
import argparse
import json
import subprocess
import tempfile
import wave
from pathlib import Path

import numpy as np
from scipy import signal

import slf_voices

TOOLS = Path(__file__).resolve().parent
RATE = 18519
HOME = Path.home() / ".cache/slf-voice"


def read(path):
    with wave.open(str(path)) as w:
        return np.frombuffer(w.readframes(w.getnframes()), "<i2").astype(np.float64) / 32768, w.getframerate()


def write(path, x):
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes((np.clip(x, -1, 1) * 32000).astype("<i2").tobytes())


def trim(x, sr, floor=0.03):
    on = np.nonzero(np.abs(x) > floor)[0]
    if not len(on):
        return x
    a, b = max(0, on[0] - int(0.01 * sr)), min(len(x), on[-1] + int(0.06 * sr))
    return x[a:b]


def room(x, sr, taps):
    y = np.concatenate([x, np.zeros(int(max(d for d, _ in taps) * sr) + 1)])
    for delay, gain in taps:
        d = int(delay * sr)
        y[d:d + len(x)] += x * gain
    return y


def shout(x, sr, item):
    x = trim(x, sr)
    b, a = signal.butter(2, 120 / (sr / 2), "highpass")
    x = signal.lfilter(b, a, x)
    # presence: lift 2-4 kHz so the line cuts through the music
    b, a = signal.butter(2, [1800 / (sr / 2), 4200 / (sr / 2)], "bandpass")
    x = x + signal.lfilter(b, a, x) * 0.6
    x = x / max(np.abs(x).max(), 1e-6)
    drive = 1.6 if item.get("announcer") else 2.4
    x = np.tanh(x * drive) / np.tanh(drive)
    if item.get("robot"):
        t = np.arange(len(x)) / sr
        metal = x * np.sin(2 * np.pi * 62 * t)
        comb = x.copy()
        d = int(0.0045 * sr)
        for i in range(d, len(comb)):
            comb[i] += comb[i - d] * 0.55
        comb /= max(np.abs(comb).max(), 1e-6)
        r = item["robot"]
        x = x * (1 - r) + (metal * 0.55 + comb * 0.45) * r
    if item.get("announcer"):
        x = room(x, sr, [(0.07, 0.32), (0.14, 0.2), (0.21, 0.12), (0.28, 0.07)])
    else:
        x = room(x, sr, [(0.045, 0.16), (0.09, 0.07)])
    fade = np.ones(len(x))
    n = int(0.004 * sr)
    fade[:n] = np.linspace(0, 1, n)
    m = int(0.05 * sr)
    fade[-m:] = np.linspace(1, 0, m)
    x = x * fade
    x = signal.resample_poly(x, RATE, sr)
    limit = int((2.2 if item.get("announcer") else 1.5) * RATE)
    if len(x) > limit:
        x = x[:limit] * np.r_[np.ones(limit - m), np.linspace(1, 0, m)][:limit]
    return x / max(np.abs(x).max(), 1e-6) * 0.97


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--piper", default=str(HOME / "venv/bin/python"))
    ap.add_argument("--model", default=str(HOME / "models/en_US-libritts_r-medium.onnx"))
    ap.add_argument("--only", nargs="*")
    args = ap.parse_args()
    entries = slf_voices.plan(slf_voices.load_fighters())
    if args.only:
        entries = [e for e in entries if e["key"] in args.only]
    slf_voices.VOICES.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="slf-voice-") as tmp:
        batch = Path(tmp) / "batch.json"
        batch.write_text(json.dumps([{"key": e["key"], "text": e["text"], "speaker": e["speaker"],
                                      "length": 0.95 if e.get("announcer") else 0.85} for e in entries]))
        subprocess.run([args.piper, "-I", str(TOOLS / "voices_tts.py"), args.model, str(batch), tmp], check=True)
        for e in entries:
            x, sr = read(Path(tmp) / f"{e['key']}.wav")
            write(slf_voices.VOICES / f"{e['key']}.wav", shout(x, sr, e))
    print(f"voices: {len(entries)} lines in {slf_voices.VOICES}")


if __name__ == "__main__":
    main()
