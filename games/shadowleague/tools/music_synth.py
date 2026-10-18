"""Instruments for Shadow League's music: drums, basses, keys, brass, guitars, leads and
regional voices, each a function that renders one note to a numpy array.

All synthesis: band-limited oscillators, two-operator FM, additive plucks and bells,
filtered noise. Mono, at RATE.
"""
import numpy as np
from scipy import signal

RATE = 22050
_rng = np.random.default_rng(1781)


def hz(midi):
    return 440.0 * 2 ** ((midi - 69) / 12.0)


def times(n):
    return np.arange(n) / RATE


def env_adsr(n, a, d, s, r):
    """Attack/decay/release in seconds, s the sustain level; the release ends the note."""
    e = np.full(n, s, dtype=np.float64)
    na, nd, nr = int(a * RATE), int(d * RATE), int(r * RATE)
    na = min(na, n)
    e[:na] = np.linspace(0, 1, na, endpoint=False) if na else e[:na]
    nd = min(nd, n - na)
    if nd > 0:
        e[na:na + nd] = np.linspace(1, s, nd)
    nr = min(nr, n)
    if nr > 0:
        e[n - nr:] *= np.linspace(1, 0, nr)
    return e


def decay(n, seconds):
    return np.exp(-times(n) / max(seconds, 1e-4))


def lowpass(x, cutoff, order=2):
    cutoff = min(cutoff, RATE * 0.45)
    sos = signal.butter(order, cutoff / (RATE / 2), "lowpass", output="sos")
    return signal.sosfilt(sos, x)


def highpass(x, cutoff, order=2):
    sos = signal.butter(order, cutoff / (RATE / 2), "highpass", output="sos")
    return signal.sosfilt(sos, x)


def bandpass(x, lo, hi, order=2):
    hi = min(hi, RATE * 0.45)
    sos = signal.butter(order, [lo / (RATE / 2), hi / (RATE / 2)], "bandpass", output="sos")
    return signal.sosfilt(sos, x)


def noise(n):
    return _rng.uniform(-1, 1, n)


def _polyblep(p, dt):
    out = np.zeros_like(p)
    a = p < dt
    x = p[a] / dt[a]
    out[a] = x + x - x * x - 1
    b = p > 1 - dt
    x = (p[b] - 1) / dt[b]
    out[b] = x * x + x + x + 1
    return out


def saw(freq, n, phase=0.0):
    f = np.broadcast_to(np.asarray(freq, dtype=np.float64), (n,))
    dt = np.clip(f / RATE, 1e-6, 0.45)
    p = (phase + np.cumsum(dt)) % 1.0
    return 2 * p - 1 - _polyblep(p, dt)


def square(freq, n, width=0.5):
    f = np.broadcast_to(np.asarray(freq, dtype=np.float64), (n,))
    dt = np.clip(f / RATE, 1e-6, 0.45)
    p = np.cumsum(dt) % 1.0
    q = (p + width) % 1.0
    return (2 * p - 1 - _polyblep(p, dt)) - (2 * q - 1 - _polyblep(q, dt))


def sine(freq, n, phase=0.0):
    f = np.broadcast_to(np.asarray(freq, dtype=np.float64), (n,))
    return np.sin(2 * np.pi * (np.cumsum(f) / RATE) + phase)


def vibrato(f0, n, depth=0.012, rate=5.5, delay=0.15):
    t = times(n)
    ramp = np.clip((t - delay) / 0.25, 0, 1)
    return f0 * (1 + depth * ramp * np.sin(2 * np.pi * rate * t))


def fm(f0, n, ratio, index_env, carrier_ratio=1.0):
    t = times(n)
    mod = np.sin(2 * np.pi * f0 * ratio * t) * index_env
    return np.sin(2 * np.pi * f0 * carrier_ratio * t + mod)


def additive(f0, n, partials):
    """partials: (ratio, amplitude, decay seconds)."""
    t = times(n)
    out = np.zeros(n)
    for ratio, amp, dec in partials:
        f = f0 * ratio
        if f >= RATE * 0.45:
            continue
        out += amp * np.sin(2 * np.pi * f * t) * np.exp(-t / dec)
    return out


def tilt_bright(x, cutoff_dark, cutoff_bright, env):
    """A filter sweep without a time-varying filter: crossfade a dark and a bright copy."""
    dark = lowpass(x, cutoff_dark)
    bright = lowpass(x, cutoff_bright)
    return dark + (bright - dark) * env


# ----------------------------------------------------------------- drums
def kick(n, vel=1.0):
    n = max(n, int(0.38 * RATE))
    t = times(n)
    f = 48 + 120 * np.exp(-t / 0.035)
    body = np.sin(2 * np.pi * np.cumsum(f) / RATE) * np.exp(-t / 0.22)
    click = highpass(noise(n), 1500) * np.exp(-t / 0.004) * 0.5
    return np.tanh((body + click) * 1.6) * vel


def snare(n, vel=1.0):
    n = max(n, int(0.3 * RATE))
    t = times(n)
    body = (np.sin(2 * np.pi * 185 * t) * 0.6 + np.sin(2 * np.pi * 330 * t) * 0.3) * np.exp(-t / 0.07)
    rattle = bandpass(noise(n), 1500, 9000) * np.exp(-t / 0.13)
    return np.tanh((body + rattle * 1.1) * 1.3) * vel


def clap(n, vel=1.0):
    n = max(n, int(0.3 * RATE))
    t = times(n)
    burst = np.zeros(n)
    for k in range(3):
        s = int(k * 0.011 * RATE)
        burst[s:] += np.exp(-(t[:n - s]) / 0.006)
    burst += np.exp(-t / 0.12) * 0.6
    return bandpass(noise(n), 900, 5000) * burst * vel * 0.9


def hat(n, vel=1.0, open_=False):
    n = max(n, int((0.32 if open_ else 0.06) * RATE))
    t = times(n)
    metal = sum(square(f, n) for f in (4120, 5960, 7080, 8250)) * 0.25 + noise(n)
    return highpass(metal, 7000) * np.exp(-t / (0.16 if open_ else 0.028)) * vel * 0.55


def cymbal(n, vel=1.0, ride=False):
    n = max(n, int((0.9 if ride else 1.8) * RATE))
    t = times(n)
    metal = sum(square(f, n) for f in (3310, 4630, 5170, 6890, 7740, 8410)) * 0.2 + noise(n) * 0.9
    out = highpass(metal, 3500 if ride else 2500) * np.exp(-t / (0.35 if ride else 0.75))
    if ride:
        out += np.sin(2 * np.pi * 2600 * t) * np.exp(-t / 0.4) * 0.15
    return out * vel * 0.5


def tom(n, vel=1.0, pitch=110):
    n = max(n, int(0.4 * RATE))
    t = times(n)
    f = pitch * (1 + 0.6 * np.exp(-t / 0.03))
    return np.sin(2 * np.pi * np.cumsum(f) / RATE) * np.exp(-t / 0.18) * vel


def taiko(n, vel=1.0):
    n = max(n, int(0.8 * RATE))
    t = times(n)
    f = 52 + 40 * np.exp(-t / 0.05)
    body = np.sin(2 * np.pi * np.cumsum(f) / RATE) * np.exp(-t / 0.42)
    skin = lowpass(noise(n), 900) * np.exp(-t / 0.05) * 0.7
    return np.tanh((body + skin) * 1.8) * vel


def conga(n, vel=1.0, pitch=220):
    n = max(n, int(0.25 * RATE))
    t = times(n)
    f = pitch * (1 + 0.25 * np.exp(-t / 0.015))
    return (np.sin(2 * np.pi * np.cumsum(f) / RATE) * np.exp(-t / 0.11) +
            bandpass(noise(n), 1500, 5000) * np.exp(-t / 0.01) * 0.3) * vel


def surdo(n, vel=1.0):
    n = max(n, int(0.7 * RATE))
    t = times(n)
    f = 58 * (1 + 0.3 * np.exp(-t / 0.04))
    return np.sin(2 * np.pi * np.cumsum(f) / RATE) * np.exp(-t / 0.35) * vel * 1.1


def cowbell(n, vel=1.0):
    n = max(n, int(0.3 * RATE))
    t = times(n)
    x = square(587, n) * 0.5 + square(845, n) * 0.5
    return bandpass(x, 500, 3000) * np.exp(-t / 0.09) * vel * 0.6


def bell(n, vel=1.0):
    """Agogo / gankogui: a bright metal bell."""
    n = max(n, int(0.35 * RATE))
    return additive(1240, n, [(1, 0.7, 0.18), (2.76, 0.3, 0.08), (5.4, 0.15, 0.04)]) * vel * 0.6


def shaker(n, vel=1.0):
    n = max(n, int(0.08 * RATE))
    t = times(n)
    e = np.minimum(t / 0.01, 1) * np.exp(-t / 0.03)
    return bandpass(noise(n), 4000, 9500) * e * vel * 0.5


def gong(n, vel=1.0):
    n = max(n, int(2.5 * RATE))
    t = times(n)
    x = additive(88, n, [(1, 0.6, 1.6), (1.48, 0.4, 1.2), (2.12, 0.3, 0.9), (2.9, 0.25, 0.7), (4.2, 0.2, 0.5)])
    x *= 1 + 0.2 * np.sin(2 * np.pi * 4.2 * t)
    return x * np.minimum(t / 0.02, 1) * vel * 0.7


def ching(n, vel=1.0):
    n = max(n, int(0.5 * RATE))
    return additive(2350, n, [(1, 0.5, 0.25), (1.53, 0.3, 0.2), (2.4, 0.2, 0.15)]) * vel * 0.4


DRUMS = {"K": kick, "S": snare, "C": clap, "h": hat, "o": lambda n, v: hat(n, v, True),
         "X": cymbal, "R": lambda n, v: cymbal(n, v, True), "T": lambda n, v: tom(n, v, 150),
         "t": lambda n, v: tom(n, v, 100), "f": lambda n, v: tom(n, v, 72), "D": taiko,
         "q": lambda n, v: conga(n, v, 330), "Q": lambda n, v: conga(n, v, 220), "U": surdo,
         "b": cowbell, "a": bell, "s": shaker, "G": gong, "c": ching}


# ----------------------------------------------------------------- pitched
def fm_bass(f0, n, vel=1.0):
    idx = 2.8 * decay(n, 0.09) + 0.4
    x = fm(f0, n, 1.0, idx) * 0.8 + sine(f0 / 2, n) * 0.35
    return np.tanh(x * 1.4) * env_adsr(n, 0.004, 0.25, 0.6, 0.03) * vel


def slap_bass(f0, n, vel=1.0):
    idx = 5.0 * decay(n, 0.05) + 0.6
    x = fm(f0, n, 1.0, idx) * 0.7 + fm(f0, n, 3.0, 1.2 * decay(n, 0.03)) * 0.3
    pop = highpass(noise(n), 2000) * decay(n, 0.006) * 0.4
    return np.tanh((x + pop) * 1.6) * env_adsr(n, 0.002, 0.18, 0.45, 0.03) * vel


def synth_bass(f0, n, vel=1.0):
    x = saw(f0, n) * 0.6 + square(f0 / 2, n) * 0.4
    x = tilt_bright(x, 300, 2400, decay(n, 0.12))
    return np.tanh(x * 1.5) * env_adsr(n, 0.003, 0.2, 0.7, 0.03) * vel


def pick_bass(f0, n, vel=1.0):
    x = additive(f0, n, [(1, 1, 0.8), (2, 0.5, 0.4), (3, 0.35, 0.25), (4, 0.2, 0.15), (5, 0.12, 0.1)])
    x += highpass(noise(n), 1200) * decay(n, 0.008) * 0.3
    return np.tanh(x * 1.2) * env_adsr(n, 0.002, 0.1, 0.8, 0.03) * vel


def epiano(f0, n, vel=1.0):
    n2 = max(n, int(0.6 * RATE))
    idx = 1.8 * decay(n2, 0.4) * vel + 0.2
    x = fm(f0, n2, 1.0, idx) + fm(f0, n2, 14.0, 0.6 * decay(n2, 0.02)) * 0.15 * decay(n2, 0.05)
    x *= decay(n2, 1.4) * env_adsr(n2, 0.002, 0, 1, 0.05)
    return x * vel


def organ(f0, n, vel=1.0):
    t = times(n)
    x = sum(a * np.sin(2 * np.pi * f0 * h * t) for h, a in ((0.5, .5), (1, .8), (2, .55), (3, .35), (4, .3), (6, .15)))
    x += np.sin(2 * np.pi * f0 * 3 * t) * 0.4 * decay(n, 0.08)     # key click / percussion
    x *= 1 + 0.12 * np.sin(2 * np.pi * 6.2 * t)                     # rotating speaker
    return x * env_adsr(n, 0.005, 0, 1, 0.04) * vel * 0.4


def brass(f0, n, vel=1.0):
    f = f0 * (1 - 0.02 * decay(n, 0.04))                           # scoop up into the note
    f = vibrato(f, n, 0.006, 5.2, 0.25)
    x = saw(f, n) + saw(f * 1.004, n) * 0.8 + saw(f * 0.996, n) * 0.8
    x = tilt_bright(x, 900, 3800 + 1500 * vel, env_adsr(n, 0.04, 0.2, 0.6, 0.05))
    return np.tanh(x * 0.9) * env_adsr(n, 0.025, 0.1, 0.85, 0.06) * vel * 0.5


def stab(f0, n, vel=1.0):
    """A short brass hit."""
    n = min(n, int(0.22 * RATE))
    return brass(f0, n, vel) * decay(n, 0.12) * 1.3


def strings(f0, n, vel=1.0):
    x = sum(saw(vibrato(f0 * d, n, 0.004, 5.0, 0.3), n) for d in (0.993, 0.998, 1.0, 1.003, 1.007)) / 3.0
    return lowpass(x, 2600) * env_adsr(n, 0.25, 0.3, 0.85, 0.3) * vel * 0.45


def choir(f0, n, vel=1.0):
    x = sum(saw(vibrato(f0 * d, n, 0.006, 4.6, 0.2), n) for d in (0.995, 1.0, 1.005)) / 2
    y = bandpass(x, 500, 1100) * 1.2 + bandpass(x, 2200, 3000) * 0.5           # an "ah" formant
    return y * env_adsr(n, 0.35, 0.3, 0.9, 0.4) * vel * 0.6


def guitar_power(f0, n, vel=1.0, mute=False):
    """A distorted power chord (root, fifth, octave)."""
    x = saw(f0, n) + saw(f0 * 1.4983, n) * 0.8 + saw(f0 * 2, n) * 0.6
    x = lowpass(x, 3000)
    x = np.tanh(x * 6) * 0.6
    x = bandpass(x, 90, 4800)
    e = env_adsr(n, 0.003, 0.08 if mute else 0.3, 0.25 if mute else 0.8, 0.04)
    return x * e * vel


def guitar_lead(f0, n, vel=1.0):
    f = vibrato(f0, n, 0.012, 5.8, 0.18)
    x = saw(f, n) * 0.8 + square(f, n, 0.3) * 0.3
    x = np.tanh(lowpass(x, 3500) * 5) * 0.6
    return bandpass(x, 150, 5200) * env_adsr(n, 0.008, 0.2, 0.85, 0.06) * vel


def clean_guitar(f0, n, vel=1.0):
    n2 = max(n, int(0.5 * RATE))
    x = additive(f0, n2, [(k, 1 / k ** 1.1, 1.2 / (1 + 0.5 * k)) for k in range(1, 12)])
    x += highpass(noise(n2), 2500) * decay(n2, 0.004) * 0.3
    return x * env_adsr(n2, 0.001, 0, 1, 0.05) * vel * 0.6


def nylon(f0, n, vel=1.0):
    n2 = max(n, int(0.7 * RATE))
    x = additive(f0, n2, [(k, 1 / k ** 1.4, 1.5 / (1 + 0.7 * k)) for k in range(1, 10)])
    return lowpass(x, 3200) * env_adsr(n2, 0.002, 0, 1, 0.05) * vel * 0.7


def koto(f0, n, vel=1.0):
    n2 = max(n, int(0.9 * RATE))
    x = additive(f0, n2, [(k, 1 / k ** 0.9, 1.4 / (1 + 0.9 * k)) for k in range(1, 14)])
    x += bandpass(noise(n2), 2000, 7000) * decay(n2, 0.006) * 0.4
    return x * vel * 0.55


def balalaika(f0, n, vel=1.0):
    """Tremolo picking: repeated bright plucks through the note."""
    out = np.zeros(n)
    step = int(0.055 * RATE)
    one = additive(f0, step * 3, [(k, 1 / k, 0.4 / (1 + 0.6 * k)) for k in range(1, 10)])
    for s in range(0, n, step):
        seg = one[:n - s] * (0.75 + 0.25 * ((s // step) % 2))
        out[s:s + len(seg)] += seg
    return out * env_adsr(n, 0.005, 0, 1, 0.05) * vel * 0.35


def steel_drum(f0, n, vel=1.0):
    n2 = max(n, int(0.6 * RATE))
    return additive(f0, n2, [(1, 1, 0.5), (2, 0.5, 0.35), (3, 0.3, 0.2), (3.98, 0.25, 0.15)]) * vel * 0.6


def marimba(f0, n, vel=1.0):
    n2 = max(n, int(0.5 * RATE))
    return additive(f0, n2, [(1, 1, 0.35), (4, 0.35, 0.06), (9.9, 0.1, 0.02)]) * vel * 0.8


def kulintang(f0, n, vel=1.0):
    n2 = max(n, int(0.8 * RATE))
    t = times(n2)
    x = additive(f0, n2, [(1, 1, 0.6), (2.41, 0.4, 0.3), (3.92, 0.2, 0.2)])
    return x * (1 + 0.15 * np.sin(2 * np.pi * 6 * t)) * vel * 0.6


def flute(f0, n, vel=1.0, breathy=0.25):
    f = vibrato(f0, n, 0.01, 5.0, 0.2)
    x = sine(f, n) + sine(f * 2, n) * 0.15
    x += bandpass(noise(n), 1200, 5000) * breathy * env_adsr(n, 0.02, 0.1, 0.4, 0.05)
    chiff = bandpass(noise(n), 2000, 8000) * decay(n, 0.02) * 0.4
    return (x + chiff) * env_adsr(n, 0.03, 0.1, 0.9, 0.06) * vel * 0.6


def shakuhachi(f0, n, vel=1.0):
    return flute(f0, n, vel, breathy=0.6)


def erhu(f0, n, vel=1.0):
    f = f0 * (1 - 0.03 * decay(n, 0.06))                           # slide up into the note
    f = vibrato(f, n, 0.02, 6.0, 0.12)
    x = saw(f, n)
    x = bandpass(x, 400, 3500) + bandpass(x, 900, 1400) * 0.8
    return x * env_adsr(n, 0.04, 0.1, 0.9, 0.08) * vel * 0.5


def reed(f0, n, vel=1.0):
    """A nasal double reed (Thai pi)."""
    f = vibrato(f0, n, 0.015, 6.4, 0.1)
    x = square(f, n, 0.35) * 0.6 + saw(f, n) * 0.4
    x = bandpass(x, 700, 2600) * 1.5 + bandpass(x, 1100, 1500)
    return np.tanh(x) * env_adsr(n, 0.02, 0.1, 0.9, 0.05) * vel * 0.45


def fiddle_low(f0, n, vel=1.0):
    """A horse-head fiddle: a bowed, rough, cello-like voice."""
    f = vibrato(f0, n, 0.014, 5.4, 0.15)
    x = saw(f, n) + saw(f * 1.003, n) * 0.6
    x = bandpass(x, 150, 2200)
    bow = bandpass(noise(n), 800, 3000) * 0.08
    return np.tanh((x + bow) * 1.2) * env_adsr(n, 0.06, 0.1, 0.9, 0.1) * vel * 0.45


def trumpet(f0, n, vel=1.0):
    f = f0 * (1 - 0.025 * decay(n, 0.03))
    f = vibrato(f, n, 0.008, 5.6, 0.22)
    x = saw(f, n)
    x = tilt_bright(x, 1200, 5000, env_adsr(n, 0.03, 0.15, 0.7, 0.05))
    return np.tanh(x * 1.3) * env_adsr(n, 0.02, 0.1, 0.9, 0.05) * vel * 0.45


def sax(f0, n, vel=1.0):
    f = vibrato(f0, n, 0.012, 5.0, 0.2)
    x = saw(f, n) * 0.7 + square(f, n, 0.4) * 0.3
    x = bandpass(x, 250, 3200) + bandpass(x, 1200, 1800) * 0.6
    return np.tanh(x * 1.4) * env_adsr(n, 0.03, 0.1, 0.9, 0.05) * vel * 0.45


def lead_synth(f0, n, vel=1.0):
    f = vibrato(f0, n, 0.01, 5.6, 0.2)
    x = saw(f, n) + saw(f * 1.006, n) * 0.7 + square(f * 0.5, n) * 0.3
    x = tilt_bright(x, 1500, 6000, env_adsr(n, 0.01, 0.3, 0.6, 0.05))
    return x * env_adsr(n, 0.01, 0.1, 0.85, 0.05) * vel * 0.4


def arp_synth(f0, n, vel=1.0):
    n = min(n, int(0.2 * RATE))
    x = square(f0, n, 0.25) * 0.6 + saw(f0 * 2, n) * 0.2
    return tilt_bright(x, 800, 5000, decay(n, 0.06)) * decay(n, 0.12) * vel * 0.5


def berimbau(f0, n, vel=1.0):
    n2 = max(n, int(0.6 * RATE))
    t = times(n2)
    f = f0 * (1 + 0.04 * np.sin(2 * np.pi * 3 * t) * np.minimum(t / 0.1, 1))     # the gourd's wah
    x = sine(f, n2) + sine(f * 2.02, n2) * 0.4 + sine(f * 3.01, n2) * 0.2
    x = bandpass(x, 150, 2500) * decay(n2, 0.45)
    return x * vel * 0.7


def drone(f0, n, vel=1.0):
    """A low throat-singing drone: a buzzing fundamental with a whistled overtone."""
    t = times(n)
    x = saw(f0, n)
    y = bandpass(x, 180, 600) + bandpass(x, f0 * 8 * 0.97, f0 * 8 * 1.03) * 3.0 * (0.6 + 0.4 * np.sin(2 * np.pi * 0.3 * t))
    return y * env_adsr(n, 0.4, 0.2, 0.9, 0.4) * vel * 0.5


INSTRUMENTS = {
    "fm_bass": fm_bass, "slap_bass": slap_bass, "synth_bass": synth_bass, "pick_bass": pick_bass,
    "epiano": epiano, "organ": organ, "brass": brass, "stab": stab, "strings": strings, "choir": choir,
    "power": guitar_power, "mute": lambda f, n, v=1.0: guitar_power(f, n, v, True), "guitar_lead": guitar_lead,
    "clean": clean_guitar, "nylon": nylon, "koto": koto, "balalaika": balalaika, "steel": steel_drum,
    "marimba": marimba, "kulintang": kulintang, "flute": flute, "shakuhachi": shakuhachi, "erhu": erhu,
    "reed": reed, "fiddle": fiddle_low, "trumpet": trumpet, "sax": sax, "lead": lead_synth, "arp": arp_synth,
    "berimbau": berimbau, "drone": drone,
}


def reverb_ir(seconds=1.4, damp=4500):
    n = int(seconds * RATE)
    t = times(n)
    ir = noise(n) * np.exp(-t / (seconds / 5.5))
    ir = lowpass(ir, damp)
    ir[:int(0.012 * RATE)] = 0                                     # pre-delay
    return ir / np.sqrt((ir ** 2).sum())


def compress(x, threshold=0.35, ratio=3.0, attack=0.005, release=0.12):
    level = np.abs(x)
    a = np.exp(-1 / (attack * RATE))
    r = np.exp(-1 / (release * RATE))
    # one-pole follower with separate attack/release, approximated in two passes
    env = signal.lfilter([1 - r], [1, -r], level)
    env = np.maximum(env, signal.lfilter([1 - a], [1, -a], level))
    gain = np.ones_like(x)
    over = env > threshold
    gain[over] = (threshold + (env[over] - threshold) / ratio) / env[over]
    return x * gain
