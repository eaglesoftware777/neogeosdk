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
       "throw", "ko", "land", "bell", "counter", "select", "power",
       # the bonus count's tick and chime; the untouched win's sting; the fanfares
       # for a team swept by one fighter and for three untouched wins
       "tally", "tally_end", "perfect", "sweep", "legend")
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


from scipy.signal import butter, lfilter  # noqa: E402


def _band(x, lo, hi):
    """Band-pass (lo or hi may be None for a low- or high-pass)."""
    nyq = RATE_A / 2
    if lo and hi:
        b, a = butter(2, [lo / nyq, min(hi / nyq, .99)], "band")
    elif lo:
        b, a = butter(2, lo / nyq, "high")
    else:
        b, a = butter(2, min(hi / nyq, .99), "low")
    return lfilter(b, a, x)


def _decay(t, ms):
    return np.exp(-t / (ms / 1000.0))


def _drop(t, f0, f1, ms):
    """A sine whose pitch falls from f0 toward f1 with time constant ms."""
    f = f1 + (f0 - f1) * np.exp(-t / (ms / 1000.0))
    return np.sin(2 * np.pi * np.cumsum(f) / RATE_A)


def _room(x, rng, ms=90, mix=.22):
    """A short room tail: the sound convolved with a decaying burst of noise."""
    n = int(ms / 1000.0 * RATE_A)
    ir = rng.uniform(-1, 1, n) * np.exp(-np.arange(n) / (n / 4.0))
    ir = _band(ir, 300, 5000)
    wet = np.convolve(x, ir)[:len(x)]
    wet /= max(np.abs(wet).max(), 1e-6)
    return x + wet * mix * np.abs(x).max()


def _at(n, t0, x):
    """x placed t0 seconds into a buffer of n samples."""
    out = np.zeros(n)
    i = int(t0 * RATE_A)
    k = min(len(x), n - i)
    out[i:i + k] = x[:k]
    return out


def _brass(t, freqs, rng, bright=1.0):
    """A brass-like chord: detuned saws through a bright low-pass, a breath of noise."""
    out = np.zeros(len(t))
    for f in freqs:
        for d in (-.004, 0, .005):
            ph = (f * (1 + d) * t) % 1.0
            out += (2 * ph - 1) / (3 * len(freqs))
    out = _band(out, None, 2200 + 2600 * bright)
    return out + _band(rng.uniform(-1, 1, len(t)), 1500, 6000) * .04


def _bell(t, f, ms):
    """A struck bell: inharmonic partials ringing out."""
    return sum(np.sin(2 * np.pi * f * r * t) * a for r, a in ((1, .55), (2.01, .25), (2.76, .16), (5.40, .07))) * _decay(t, ms)


def _stab(n, t0, dur, freqs, rng, bright=1.0):
    """A fanfare chord at t0 for dur seconds: a quick swell and a clean release."""
    m = int(dur * RATE_A)
    tt = np.arange(m) / RATE_A
    shape = np.minimum(1, tt / .012) * np.where(tt > dur - .05, np.maximum(0, (dur - tt) / .05), 1)
    return _at(n, t0, _brass(tt, freqs, rng, bright) * shape)


def sfx(name, rng):
    """The effects, built the way arcade fighters build theirs: a sharp transient,
    a band of crunch, a body that drops in pitch, and a short room tail."""
    durations = {"coin": .35, "menu": .06, "start": .5, "hit_light": .14, "hit_heavy": .3, "block": .16,
                 "whoosh": .17, "special": .4, "super": .9, "throw": .32, "ko": 1.2, "land": .15, "bell": 1.0,
                 "counter": .35, "select": .3, "power": .8,
                 "tally": .05, "tally_end": .5, "perfect": 1.0, "sweep": 1.9, "legend": 2.6}
    n = int(durations[name] * RATE_A)
    t = np.arange(n) / RATE_A
    noise = rng.uniform(-1, 1, n)
    click = _band(noise, 2500, None) * _decay(t, 1.5)
    if name == "coin":
        s = np.where(t < .08, np.sin(2 * np.pi * 988 * t), np.sin(2 * np.pi * 1319 * t)) * env(n, .01, 1.5)
    elif name == "menu":
        s = np.sign(np.sin(2 * np.pi * 1200 * t)) * .5 * env(n, .02)
    elif name in ("start", "select"):
        f = np.where(t < durations[name] / 3, 660, np.where(t < 2 * durations[name] / 3, 880, 1320))
        s = np.sign(np.sin(2 * np.pi * np.cumsum(f) / RATE_A)) * .45 * env(n, .02, .7)
    elif name == "hit_light":            # a snapping slap
        s = (click * .9 + _band(noise, 900, 3800) * _decay(t, 22) * .9
             + _drop(t, 260, 110, 25) * _decay(t, 55) * .7)
        s = _room(np.tanh(s * 1.6), rng, 60, .15)
    elif name == "hit_heavy":            # a deep, crunching blow
        s = (click * 1.0 + _band(noise, 500, 2600) * _decay(t, 45) * .95
             + _drop(t, 170, 48, 40) * _decay(t, 150) * 1.0
             + _band(noise, None, 400) * _decay(t, 80) * .6)
        s = _room(np.tanh(s * 2.0), rng, 110, .22)
    elif name == "block":                # a hard clack on the guard
        partials = sum(np.sin(2 * np.pi * f * t) * a for f, a in ((1460, .5), (2390, .35), (3710, .2)))
        s = (click * .8 + partials * _decay(t, 28) + _drop(t, 220, 120, 20) * _decay(t, 40) * .5
             + _band(noise, 1500, 6000) * _decay(t, 12) * .5)
        s = _room(s, rng, 70, .18)
    elif name == "whoosh":               # air torn past: a filtered sweep up then down
        centre = 700 + 2200 * np.sin(np.pi * t / t[-1]) ** 1.5
        s = np.zeros(n)
        for k0 in range(0, n, 64):        # a sliding band, re-filtered every 64 samples
            seg = noise[k0:k0 + 64]
            c = centre[min(k0 + 32, n - 1)]
            s[k0:k0 + len(seg)] = _band(noise[max(0, k0 - 256):k0 + 64], c * .6, c * 1.5)[-len(seg):]
        s *= np.sin(np.pi * t / t[-1]) ** 2
    elif name == "land":                 # feet into the floor, a little grit
        s = (_drop(t, 120, 50, 30) * _decay(t, 70) + _band(noise, 200, 1400) * _decay(t, 35) * .6 + click * .3)
        s = np.tanh(s * 1.4)
    elif name == "throw":                # the grab, then the slam
        grab = _band(noise, 1200, 3500) * _decay(t, 25) * .7
        slam_t = t[:n - int(.1 * RATE_A)]
        slam = (_drop(slam_t, 140, 42, 45) * _decay(slam_t, 160) + _band(rng.uniform(-1, 1, len(slam_t)), 300, 2000)
                * _decay(slam_t, 60) * .8)
        s = grab + _at(n, .1, np.tanh(slam * 1.8))
        s = _room(s, rng, 120, .25)
    elif name == "special":              # energy gathering into a burst
        rise = _band(noise, 400, 4000) * np.sin(np.pi * np.minimum(t / (t[-1] * .7), 1)) ** 2 * .6
        tone = sum(np.sin(2 * np.pi * (f * (1 + 1.5 * t / t[-1])) * t) * a for f, a in ((220, .3), (331, .2), (442, .15)))
        burst = _at(n, t[-1] * .62, (click[:2000] * 1.2 + _drop(t[:2000], 300, 90, 30) * _decay(t[:2000], 60)))
        s = _room(rise + tone * env(n, .3, 1.2) + burst, rng, 120, .25)
    elif name == "super":                # the big one: a sub boom under a rising roar
        roar = _band(noise, 200, 3000) * env(n, .25, .8) * .7
        shimmer = (np.sin(2 * np.pi * np.cumsum(220 + 880 * t / t[-1]) / RATE_A) * .25
                   + np.sin(2 * np.pi * np.cumsum(330 + 1320 * t / t[-1]) / RATE_A) * .18) * env(n, .4, .7)
        boom = _drop(t, 90, 30, 120) * _decay(t, 400) * .9
        s = _room(np.tanh((roar + shimmer + boom + click) * 1.5), rng, 200, .3)
    elif name == "ko":                   # the finishing blow and a long fall-off
        s = (click * 1.2 + _band(noise, 400, 3000) * _decay(t, 70) + _drop(t, 120, 30, 80) * _decay(t, 450) * 1.1
             + _band(noise, None, 300) * _decay(t, 300) * .6)
        s = _room(np.tanh(s * 2.2), rng, 300, .35)
    elif name == "bell":                 # a ring bell: inharmonic partials, a slow ring-out
        s = sum(np.sin(2 * np.pi * f * t) * a for f, a in ((812, .5), (1622, .22), (2104, .18), (2963, .1), (4006, .06)))
        s = s * np.exp(-t * 2.6) + click * .4
    elif name == "power":                # MAX mode: a rising, crackling surge
        crackle = (rng.uniform(0, 1, n) > .985) * rng.uniform(-1, 1, n)
        crackle = _band(crackle, 1500, None) * 2.0
        s = (np.sin(2 * np.pi * np.cumsum(70 + 600 * (t / t[-1]) ** 1.5) / RATE_A) * .5
             + _band(noise, 300, 2500) * .35 + crackle) * env(n, .5, .6)
        s = _room(np.tanh(s * 1.4), rng, 160, .25)
    elif name == "counter":              # a bright strike that rings
        s = ((np.sin(2 * np.pi * 1760 * t) * .5 + np.sin(2 * np.pi * 2637 * t) * .3) * np.exp(-t * 8)
             + click * 1.0 + _band(noise, 800, 3500) * _decay(t, 30) * .7 + _drop(t, 240, 90, 25) * _decay(t, 60) * .6)
        s = _room(np.tanh(s * 1.4), rng, 140, .25)
    elif name == "tally":                # the counter's tick: a bright, dry blip
        s = np.sign(np.sin(2 * np.pi * 2350 * t)) * .5 * _decay(t, 9) + click * .25
    elif name == "tally_end":            # the line settles: a two-note bell, high and clear
        s = _bell(t, 1568, 160) * .7 + _at(n, .06, _bell(t, 2093, 220)) * .8 + click * .2
        s = _room(s, rng, 160, .2)
    elif name == "perfect":              # untouched: a quick climb to a ringing chord
        s = np.zeros(n)
        for k, f in enumerate((1047, 1319, 1568, 2093)):
            s += _at(n, k * .06, np.sign(np.sin(2 * np.pi * f * t)) * .22 * _decay(t, 70))
        shimmer = sum(np.sin(2 * np.pi * f * t) for f in (1047, 1319, 1568, 2093)) * .14
        s += _at(n, .24, shimmer * np.exp(-t * 3.2)) + _at(n, .24, _bell(t, 2637, 300) * .35)
        s = _room(s, rng, 220, .3)
    elif name == "sweep":                # a team swept away: three stabs and a held chord
        s = (_stab(n, 0.00, .14, (392, 494, 587), rng) + _stab(n, .18, .14, (392, 494, 587), rng)
             + _stab(n, .36, .14, (440, 554, 659), rng) + _stab(n, .56, 1.2, (523, 659, 784, 1047), rng, 1.3))
        crash = _band(rng.uniform(-1, 1, n), 3000, None) * np.exp(-np.maximum(t - .56, 0) * 3.5) * (t >= .56) * .35
        boom = _at(n, .56, _drop(t, 110, 40, 120) * _decay(t, 300) * .6)
        s = _room(np.tanh((s * 1.5 + crash + boom) * 1.2), rng, 260, .3)
    elif name == "legend":               # three untouched wins: a rising call, a choir of brass, bells
        rise = np.zeros(n)
        for k, f in enumerate((262, 330, 392, 523, 659, 784)):
            rise += _stab(n, k * .09, .11, (f, f * 1.5), rng, .8) * .8
        chord = _stab(n, .62, 1.9, (262, 392, 523, 659, 784, 1047), rng, 1.5)
        bells = _at(n, .62, _bell(t, 1047, 700) * .45) + _at(n, .9, _bell(t, 1568, 600) * .35) + _at(n, 1.2, _bell(t, 2093, 500) * .3)
        crash = _band(rng.uniform(-1, 1, n), 2500, None) * np.exp(-np.maximum(t - .62, 0) * 2.2) * (t >= .62) * .3
        boom = _at(n, .62, _drop(t, 90, 32, 160) * _decay(t, 500) * .7)
        s = _room(np.tanh((rise + chord * 1.4 + bells + crash + boom) * 1.1), rng, 320, .32)
    s = s / max(np.abs(s).max(), 1e-6) * 0.92
    return (s * 32000).astype(np.int16)


def _master_voice(pcm):
    """Voices cleaned and pushed forward: low rumble off, a presence lift, gentle
    compression, then up to just under full scale. The words are untouched."""
    x = pcm.astype(np.float64) / 32768.0
    if not np.abs(x).max():
        return pcm
    x = _band(x, 90, None)
    x = x + _band(x, 2000, 4500) * .35
    level = np.abs(x)
    env_ = np.convolve(level, np.ones(64) / 64, mode="same")
    thr = np.percentile(env_, 90) * .6 + 1e-6
    gain = np.where(env_ > thr, (thr / np.maximum(env_, 1e-6)) ** .45, 1.0)
    x = x * gain
    x = x / max(np.abs(x).max(), 1e-6) * .95
    return (x * 32767).astype(np.int16)


def _master_music(pcm):
    """The songs: a soft-knee squeeze and a limiter for a fuller, louder mix."""
    x = pcm.astype(np.float64) / 32768.0
    peak = max(np.abs(x).max(), 1e-6)
    x = x / peak
    x = np.tanh(x * 1.6) / np.tanh(1.6)
    return (x * .95 * 32767).astype(np.int16)


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
            pcm = _master_voice(np.frombuffer(w.readframes(w.getnframes()), "<i2"))
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
        pcm = _master_music(render(song, rng))
        wav(base / "in_wav_b" / f"{i}.wav", pcm, RATE_B)
        data = encode_adpcmb(pcm.tolist())
        data = data[0] if isinstance(data, tuple) else data
        if len(data) % 256:
            data = bytes(data) + bytes(256 - len(data) % 256)
        (base / "out_b" / f"{i}.adpcmb").write_bytes(data)
        (base / "out_b" / f"{i}.adpcmb.json").write_text(json.dumps({"sample_rate": RATE_B}, indent=2) + "\n")
        total += len(data)
    # The driver's table takes 512 entries (samples past 255: command $10, playSFXWide).
    if last + len(SONGS) > 512:
        raise ValueError(f"{last} ADPCM-A and {len(SONGS)} ADPCM-B samples overflow the 512-entry table")
    print(f"sound: {len(SFX)} effects, {len(lines)} voice lines, {len(SONGS)} tracks, {total} bytes encoded")


if __name__ == "__main__":
    main()
