"""Synthesize and encode Shadow League's effects (ADPCM-A) and music (ADPCM-B),
and encode the voices (ADPCM-A, recorded by voices.py, listed in slf_voices.py).

Effects and music are generated here from oscillators and noise.

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
import slf_voices  # noqa: E402

RATE_A = 18519
RATE_B = 22050
assert RATE_B == __import__("music_synth").RATE
SFX = ("coin", "menu", "start", "hit_light", "hit_heavy", "block", "whoosh", "special", "super",
       "throw", "ko", "land", "bell", "counter", "select", "power")
assert len(SFX) == slf_voices.NUM_SFX


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
                 "counter": .35, "select": .3, "power": .8}
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
    elif name == "power":                       # MAX mode: a rising, crackling surge
        s = (sweep(t, 80, 640) * .5 + sweep(t, 120, 960) * .3 + lp(noise, 3) * .45 * np.sin(np.pi * t / t[-1])) * \
            env(n, .55, .6)
    elif name == "counter":
        s = (np.sin(2 * np.pi * 1760 * t) * .5 + np.sin(2 * np.pi * 2637 * t) * .3) * np.exp(-t * 9) + \
            lp(noise, 2) * .3 * env(n, .005, 6)
    s = s / max(np.abs(s).max(), 1e-6) * 0.9
    return (s * 32000).astype(np.int16)


# ---------------------------------------------------------------- music
# The songs live in music.py (title, select, victory, continue, then one per arena).
import music  # noqa: E402

SONGS = {i: song for i, song in enumerate(music.SONGS, 1)}


def render(song, rng):
    return music.render_song(song)


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
    lines = slf_voices.plan(slf_voices.load_fighters())
    for line in lines:
        with wave.open(str(slf_voices.VOICES / f"{line['key']}.wav")) as w:
            if w.getframerate() != RATE_A:
                raise ValueError(f"{line['key']}: voices are recorded at {RATE_A} Hz")
            pcm = np.frombuffer(w.readframes(w.getnframes()), "<i2")
        wav(base / "in_wav_a" / f"{line['number']}.wav", pcm, RATE_A)
        data, _ = encode_adpcma(pcm.tolist())
        (base / "out_a" / f"{line['number']}.adpcma").write_bytes(data)
        total += len(data)
    last = lines[-1]["number"] if lines else len(SFX)
    for sub, ext in (("in_wav_a", "wav"), ("out_a", "adpcma")):
        for stale in (base / sub).glob(f"*.{ext}"):
            if stale.stem.isdigit() and int(stale.stem) > last:
                stale.unlink()
    for stale in (base / "out_b").glob("*.adpcmb*"):
        stale.unlink()
    for stale in (base / "in_wav_b").glob("*.wav"):
        stale.unlink()
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
    if last + len(SONGS) > 256:
        raise ValueError(f"{last} ADPCM-A and {len(SONGS)} ADPCM-B samples overflow the 256-entry table")
    print(f"sound: {len(SFX)} effects, {len(lines)} voice lines, {len(SONGS)} tracks, {total} bytes encoded")


if __name__ == "__main__":
    main()
