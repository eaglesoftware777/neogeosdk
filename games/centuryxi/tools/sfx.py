#!/usr/bin/env python3
"""The match's sounds, synthesised here so every one is original.

Writes sound/samples/in_wav_a (ADPCM-A: 1.wav is playSFX(0)) and in_wav_b (ADPCM-B:
the crowd's loop). Run by hand when a sound changes; the WAVs are the source the
build encodes.
"""
import wave
from pathlib import Path

import numpy as np

RATE = 22050
OUT = Path(__file__).resolve().parents[1] / "sound" / "samples"
rng = np.random.default_rng(787)


def t_of(seconds):
    return np.arange(int(RATE * seconds)) / RATE


def env(t, attack, decay):
    return np.minimum(t / max(attack, 1e-4), 1.0) * np.exp(-t / decay)


def lowpass(x, cutoff):
    """One-pole low pass."""
    a = 1.0 - np.exp(-2 * np.pi * cutoff / RATE)
    y = np.empty_like(x)
    acc = 0.0
    for i, v in enumerate(x):
        acc += a * (v - acc)
        y[i] = acc
    return y


def highpass(x, cutoff):
    return x - lowpass(x, cutoff)


def band(x, lo, hi):
    return lowpass(highpass(x, lo), hi)


def kick(power=1.0):
    t = t_of(0.16)
    f = 70 + 150 * np.exp(-t / 0.018)
    body = np.sin(2 * np.pi * np.cumsum(f) / RATE) * env(t, 0.001, 0.045)
    click = band(rng.standard_normal(len(t)), 900, 5000) * np.exp(-t / 0.006) * 0.8
    return (body + click) * power


def whistle(length=0.32, gap=0.0):
    t = t_of(length)
    # a pea whistle: two close tones, the pea beating them at about 28 Hz
    pea = 0.5 + 0.5 * np.sin(2 * np.pi * 28 * t)
    f = 2850 + 110 * np.sin(2 * np.pi * 28 * t)
    x = (np.sin(2 * np.pi * np.cumsum(f) / RATE) + 0.5 * np.sin(2 * np.pi * np.cumsum(f * 1.06) / RATE)) * (0.55 + 0.45 * pea)
    x += band(rng.standard_normal(len(t)), 2000, 6000) * 0.12
    shape = np.minimum(t / 0.012, 1.0) * np.minimum((length - t) / 0.03, 1.0)
    return x * shape


def long_whistle():
    a = whistle(0.28)
    b = whistle(0.28)
    c = whistle(0.75)
    gap = np.zeros(int(RATE * 0.12))
    return np.concatenate((a, gap, b, gap, c))


def post():
    t = t_of(0.6)
    x = sum(np.sin(2 * np.pi * f * t) * np.exp(-t / d) * g for f, d, g in
            ((812, 0.22, 1.0), (1370, 0.15, 0.7), (2130, 0.09, 0.5), (3110, 0.05, 0.3)))
    return x + band(rng.standard_normal(len(t)), 800, 6000) * np.exp(-t / 0.01) * 0.6


def catch():
    t = t_of(0.18)
    return (np.sin(2 * np.pi * (120 + 60 * np.exp(-t / 0.02)) * t) * env(t, 0.001, 0.05)
            + band(rng.standard_normal(len(t)), 200, 1800) * np.exp(-t / 0.03) * 0.6)


def slide():
    t = t_of(0.42)
    x = band(rng.standard_normal(len(t)), 500, 3200)
    return x * np.sin(np.pi * np.clip(t / 0.42, 0, 1)) ** 1.5 * (0.7 + 0.3 * np.sin(2 * np.pi * 21 * t))


def crowd_noise(seconds, lo=260, hi=2400):
    """Many voices: noise, with slow swells so it never sounds like a hiss."""
    n = int(RATE * seconds)
    x = band(rng.standard_normal(n), lo, hi)
    t = np.arange(n) / RATE
    swell = np.zeros(n)
    for f in (0.31, 0.53, 0.87, 1.31, 2.1, 3.3):
        swell += np.sin(2 * np.pi * f * t + rng.uniform(0, 6.28)) / (1 + f)
    return x * (1 + 0.28 * swell)


def ooh():
    t = t_of(1.1)
    x = crowd_noise(1.1, 300, 1300)
    return x * np.sin(np.pi * np.clip(t / 1.1, 0, 1)) ** 0.8


def roar(seconds=3.2):
    t = t_of(seconds)
    x = crowd_noise(seconds, 220, 3400)
    return x * np.minimum(t / 0.12, 1.0) * np.minimum((seconds - t) / 1.2, 1.0) * 1.6


def ambience(seconds=8.0, fade=0.8):
    """The crowd under the match, made to loop without a seam."""
    n, f = int(RATE * seconds), int(RATE * fade)
    x = crowd_noise(seconds + fade, 240, 2000)
    k = np.linspace(0, 1, f)
    out = x[:n].copy()
    out[:f] = x[:f] * k + x[n:n + f] * (1 - k)
    return out


def save(path, x, peak=0.85):
    x = np.asarray(x, np.float64)
    x = x / max(1e-6, np.abs(x).max()) * peak
    data = (x * 32767).astype('<i2').tobytes()
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(data)


A = [kick(0.75), kick(1.0), whistle(), long_whistle(), post(), catch(), slide(), ooh(), roar()]
# playSFX numbers: see cx_audio.c
for i, x in enumerate(A, 1):
    save(OUT / "in_wav_a" / f"{i}.wav", x)
save(OUT / "in_wav_b" / "1.wav", ambience(), peak=0.7)
save(OUT / "in_wav_b" / "5.wav", ambience(6.0) * 1.0, peak=0.85)    # louder: a chance near goal
print("wrote", len(A), "samples and the crowd")
