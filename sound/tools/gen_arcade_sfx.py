#!/usr/bin/env python3
"""Synthesise the shooter SFX bank additions (13..16) as 16-bit mono WAVs.

The four sounds are the arcade staples a vertical shooter needs and the
original twelve-sample bank did not have: a laser shot, an explosion, a
pick-up chime and a boss-warning siren.  They are generated rather than
recorded so the bank stays free of third-party material and can be
regenerated from this file alone.

    python3 sound/tools/gen_arcade_sfx.py games/demo/sound/samples/in_wav_a
"""

import math
import struct
import sys
import wave
from pathlib import Path

import numpy as np

RATE = 22050


def _write(path, data):
    data = np.clip(data, -1.0, 1.0)
    pcm = (data * 32000.0).astype("<i2")
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(pcm.tobytes())


def _env(n, attack, decay):
    t = np.arange(n) / RATE
    env = np.minimum(1.0, t / max(attack, 1e-4))
    return env * np.exp(-t / decay)


def laser(duration=0.14):
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    freq = 1900.0 * np.exp(-t * 14.0) + 240.0
    phase = 2.0 * math.pi * np.cumsum(freq) / RATE
    tone = np.sign(np.sin(phase)) * 0.55 + 0.45 * np.sin(2.0 * phase)
    return tone * _env(n, 0.002, 0.045) * 0.8


def explosion(duration=0.55):
    n = int(RATE * duration)
    rng = np.random.default_rng(7)
    noise = rng.uniform(-1.0, 1.0, n)
    # one-pole low-pass that opens with the blast and closes as it decays
    out = np.zeros(n)
    acc = 0.0
    for i in range(n):
        k = 0.35 * math.exp(-i / (RATE * 0.18)) + 0.03
        acc += k * (noise[i] - acc)
        out[i] = acc
    t = np.arange(n) / RATE
    thump = np.sin(2.0 * math.pi * (70.0 * np.exp(-t * 6.0) + 40.0) * t)
    return (out * 2.4 * _env(n, 0.003, 0.16) + thump * 0.6 * _env(n, 0.002, 0.09)) * 0.9


def pickup(duration=0.26):
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    out = np.zeros(n)
    for start, freq in ((0.0, 880.0), (0.07, 1108.0), (0.14, 1318.0)):
        i0 = int(start * RATE)
        seg = t[: n - i0]
        out[i0:] += np.sin(2.0 * math.pi * freq * seg) * _env(n - i0, 0.002, 0.06)
    return out * 0.6


def siren(duration=0.9):
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    freq = np.where(((t * 4.0) % 1.0) < 0.5, 620.0, 880.0)
    phase = 2.0 * math.pi * np.cumsum(freq) / RATE
    tone = np.sign(np.sin(phase)) * 0.5 + 0.5 * np.sin(phase)
    return tone * _env(n, 0.01, 0.9) * 0.7


def main():
    out_dir = Path(sys.argv[1] if len(sys.argv) > 1 else "games/demo/sound/samples/in_wav_a")
    out_dir.mkdir(parents=True, exist_ok=True)
    for index, fn in ((13, laser), (14, explosion), (15, pickup), (16, siren)):
        path = out_dir / f"{index}.wav"
        _write(path, fn())
        print(path)


if __name__ == "__main__":
    main()
