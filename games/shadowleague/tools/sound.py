"""Synthesize and encode Shadow League's effects (ADPCM-A) and music (ADPCM-B).

Everything is generated here from oscillators and noise: no recordings.

    python3 games/shadowleague/tools/sound.py
"""

import json
import sys
import wave
from pathlib import Path

import numpy as np

GAME = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(GAME.parents[1] / "sound/tools"))
from adpcm_enc import encode_adpcma, encode_adpcmb  # noqa: E402

RATE_A = 18519
RATE_B = 22050
SFX = ("coin", "menu", "start", "hit_light", "hit_heavy", "block", "whoosh", "special", "super",
       "throw", "ko", "land", "bell", "counter", "select")


def env(n, attack, decay_power=1.0):
    a = max(1, int(attack * n))
    e = np.ones(n)
    e[:a] = np.linspace(0, 1, a)
    e[a:] = np.linspace(1, 0, n - a) ** decay_power
    return e


def sweep(t, f0, f1):
    dur = max(t[-1], 1e-6)
    return np.sin(2 * np.pi * (f0 * t + (f1 - f0) * t * t / (2 * dur)))


def sfx(name, rng):
    durations = {"coin": .35, "menu": .06, "start": .5, "hit_light": .12, "hit_heavy": .26, "block": .14,
                 "whoosh": .16, "special": .38, "super": .9, "throw": .3, "ko": 1.2, "land": .14, "bell": 1.0,
                 "counter": .35, "select": .3}
    n = int(durations[name] * RATE_A)
    t = np.arange(n) / RATE_A
    noise = rng.uniform(-1, 1, n)
    lp = lambda x, k: np.convolve(x, np.ones(k) / k, mode="same")  # noqa: E731
    if name == "coin":
        s = np.where(t < .08, np.sin(2 * np.pi * 988 * t), np.sin(2 * np.pi * 1319 * t)) * env(n, .01, 1.5)
    elif name == "menu":
        s = np.sign(np.sin(2 * np.pi * 1200 * t)) * .5 * env(n, .02)
    elif name in ("start", "select"):
        f = np.where(t < durations[name] / 3, 660, np.where(t < 2 * durations[name] / 3, 880, 1320))
        s = np.sign(np.sin(2 * np.pi * np.cumsum(f) / RATE_A)) * .45 * env(n, .02, .7)
    elif name == "hit_light":
        s = (lp(noise, 3) * .8 + sweep(t, 420, 120) * .6) * env(n, .01, 3)
    elif name == "hit_heavy":
        s = (lp(noise, 5) * .9 * env(n, .005, 4) + sweep(t, 160, 45) * env(n, .01, 1.6)) * .95
    elif name == "block":
        s = (np.sin(2 * np.pi * 1700 * t) * .5 + np.sin(2 * np.pi * 2300 * t) * .3 + noise * .3) * env(n, .005, 4)
    elif name == "whoosh":
        s = lp(noise, 6) * np.sin(np.pi * t / t[-1]) ** 2 * .8
    elif name == "special":
        s = (lp(noise, 4) * np.sin(np.pi * t / t[-1]) * .6 + sweep(t, 220, 880) * .35 * env(n, .1))
    elif name == "super":
        s = (sweep(t, 110, 660) * .4 + sweep(t, 165, 990) * .3 + lp(noise, 8) * .4) * env(n, .2, .8)
    elif name == "throw":
        s = (sweep(t, 140, 60) * .8 + lp(noise, 7) * .4) * env(n, .02, 2)
    elif name == "ko":
        s = (sweep(t, 90, 30) * .9 * env(n, .005, 1.2) + lp(noise, 9) * .5 * env(n, .002, 6) +
             np.sin(2 * np.pi * 440 * t) * .2 * env(n, .05, 1))
    elif name == "land":
        s = (sweep(t, 110, 50) * .7 + lp(noise, 9) * .4) * env(n, .01, 3)
    elif name == "bell":
        s = sum(np.sin(2 * np.pi * f * t) * a for f, a in ((880, .5), (1320, .25), (2210, .15), (2960, .1))) * \
            np.exp(-t * 3.2)
    elif name == "counter":
        s = (np.sin(2 * np.pi * 1760 * t) * .5 + np.sin(2 * np.pi * 2637 * t) * .3) * np.exp(-t * 9) + \
            lp(noise, 2) * .3 * env(n, .005, 6)
    s = s / max(np.abs(s).max(), 1e-6) * 0.9
    return (s * 32000).astype(np.int16)


# ---------------------------------------------------------------- music
def note_hz(semitone):
    return 440.0 * 2 ** ((semitone - 9) / 12.0)  # semitone 0 = C4


def voice(kind, freq, n, rng):
    t = np.arange(n) / RATE_B
    if kind == "square":
        w = np.sign(np.sin(2 * np.pi * freq * t + 0.3 * np.sin(2 * np.pi * 5.5 * t))) * .5
        return w * env(n, .02, .6)
    if kind == "bass":
        ph = (freq * t) % 1.0
        return (2 * ph - 1) * .6 * env(n, .01, .4)
    if kind == "pluck":
        return np.sin(2 * np.pi * freq * t) * np.exp(-t * 7) + np.sin(2 * np.pi * 2 * freq * t) * .3 * np.exp(-t * 11)
    if kind == "kick":
        return sweep(t, 120, 40) * np.exp(-t * 18) * 1.2
    if kind == "snare":
        return (rng.uniform(-1, 1, n) * .7 + np.sin(2 * np.pi * 190 * t) * .4) * np.exp(-t * 22)
    if kind == "hat":
        x = rng.uniform(-1, 1, n)
        return (x - np.convolve(x, np.ones(3) / 3, mode="same")) * np.exp(-t * 60) * .6
    if kind == "taiko":
        return sweep(t, 90, 55) * np.exp(-t * 9) * 1.3
    raise ValueError(kind)


def render(song, rng):
    bpm, bars = song["bpm"], song["bars"]
    step = 60.0 / bpm / 4                     # sixteenth notes
    total = int(bars * 16 * step * RATE_B)
    mix = np.zeros(total + RATE_B)
    for track in song["tracks"]:
        kind, gain, pattern = track["kind"], track["gain"], track["pattern"]
        steps = len(pattern)
        for bar in range(bars):
            prog = song.get("prog", [0])[bar % len(song.get("prog", [0]))]
            for i, cell in enumerate(pattern):
                if cell is None:
                    continue
                pos = int((bar * 16 * (steps // 16 or 1) + i) * step * 16 / steps * RATE_B) if steps != 16 else \
                    int((bar * 16 + i) * step * RATE_B)
                length = int(step * RATE_B * track.get("length", 2))
                freq = note_hz(cell + (prog if track.get("transpose") else 0)) if kind in ("square", "bass", "pluck") else 0
                mix[pos:pos + length] += voice(kind, freq, length, rng) * gain
    mix = mix[:total]
    mix = np.tanh(mix * 1.4) / np.tanh(1.4)
    return (mix / max(np.abs(mix).max(), 1e-6) * 0.85 * 32000).astype(np.int16)


R = None
SONGS = {
    # Title: driving, minor key.
    1: {"bpm": 138, "bars": 8, "prog": [0, 0, -4, -2],
        "tracks": [
            {"kind": "kick", "gain": .9, "pattern": [0, R, R, R, 0, R, R, R, 0, R, R, R, 0, R, R, R]},
            {"kind": "snare", "gain": .6, "pattern": [R, R, R, R, 0, R, R, R, R, R, R, R, 0, R, R, 0]},
            {"kind": "hat", "gain": .35, "pattern": [0, R, 0, R] * 4},
            {"kind": "bass", "gain": .55, "transpose": 1, "length": 2,
             "pattern": [-24, R, -24, -12, R, -24, R, -12, -24, R, -24, -12, R, -24, -10, -12]},
            {"kind": "square", "gain": .32, "transpose": 1, "length": 4,
             "pattern": [9, R, R, R, 12, R, 14, R, 16, R, R, R, 14, R, 12, R]},
        ]},
    # Arata Mountain Dojo: taiko and a pentatonic koto-like lead.
    2: {"bpm": 150, "bars": 16, "prog": [0, 0, 5, 3, 0, 0, 7, 5],
        "tracks": [
            {"kind": "taiko", "gain": 1.0, "pattern": [0, R, R, 0, R, R, 0, R, R, R, 0, R, 0, R, R, R]},
            {"kind": "snare", "gain": .45, "pattern": [R, R, R, R, 0, R, R, R, R, R, R, R, 0, R, R, R]},
            {"kind": "hat", "gain": .3, "pattern": [0, R, 0, 0] * 4},
            {"kind": "bass", "gain": .55, "transpose": 1, "length": 2,
             "pattern": [-24, R, -24, R, -17, R, -24, R, -24, R, -19, R, -17, R, -12, R]},
            {"kind": "pluck", "gain": .55, "transpose": 1, "length": 4,
             "pattern": [4, R, 5, R, 9, R, 11, R, 12, R, 11, 9, 5, R, 4, R]},
            {"kind": "square", "gain": .18, "transpose": 1, "length": 8,
             "pattern": [16, R, R, R, R, R, 17, R, 21, R, R, R, 17, R, R, R]},
        ]},
    # Victory jingle.
    3: {"bpm": 120, "bars": 2, "prog": [0],
        "tracks": [
            {"kind": "square", "gain": .4, "length": 3, "pattern": [0, R, 4, R, 7, R, 12, R, R, R, 7, R, 12, R, R, R]},
            {"kind": "bass", "gain": .5, "length": 6, "pattern": [-24, R, R, R, -20, R, R, R, -17, R, R, R, -12, R, R, R]},
            {"kind": "snare", "gain": .4, "pattern": [R, R, R, R, R, R, R, R, R, R, R, R, 0, 0, 0, 0]},
        ]},
}


def wav(path, pcm, rate):
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(np.asarray(pcm, "<i2").tobytes())


def main():
    rng = np.random.default_rng(781)
    base = GAME / "sound/samples"
    for sub in ("in_wav_a", "out_a", "in_wav_b", "out_b"):
        (base / sub).mkdir(parents=True, exist_ok=True)
    total = 0
    for i, name in enumerate(SFX, 1):
        pcm = sfx(name, rng)
        wav(base / "in_wav_a" / f"{i}.wav", pcm, RATE_A)
        data, _ = encode_adpcma(pcm.tolist())
        (base / "out_a" / f"{i}.adpcma").write_bytes(data)
        total += len(data)
    for i, song in SONGS.items():
        pcm = render(song, rng)
        wav(base / "in_wav_b" / f"{i}.wav", pcm, RATE_B)
        data = encode_adpcmb(pcm.tolist())
        data = data[0] if isinstance(data, tuple) else data
        if len(data) % 256:
            data = bytes(data) + bytes(256 - len(data) % 256)
        (base / "out_b" / f"{i}.adpcmb").write_bytes(data)
        (base / "out_b" / f"{i}.adpcmb.json").write_text(json.dumps({"sample_rate": RATE_B}, indent=2) + "\n")
        total += len(data)
    print(f"sound: {len(SFX)} effects, {len(SONGS)} tracks, {total} bytes encoded")


if __name__ == "__main__":
    main()
