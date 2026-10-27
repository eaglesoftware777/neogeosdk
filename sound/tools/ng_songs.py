"""A small sequencer over ng_synth's instruments: a game's songs become looping WAVs.

A song is sections of bars: a chord per bar (or two, "Am,F"), a drum pattern, a bass
pattern, comping parts and melodies written on an eighth-note grid ("E5 ~ - G5": note,
held, rest). Solo sections are improvised from the chords and the song's scale with a
fixed seed. Loops are rendered twice and the second pass kept, so reverb and decaying
notes carry over the loop point.

    from ng_songs import Song, S, L, P, render_song
    wav = render_song(Song("title", 120, "C", "major", [S(8, "C G Am F", "rock", "rock8", ...)], {"bass": "pick_bass"}))
"""
import sys
from pathlib import Path

import numpy as np
from scipy import signal

from ng_synth import DRUMS, INSTRUMENTS, RATE, compress, highpass, hz, reverb_ir

NOTE = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
QUALITY = {"": (0, 4, 7), "m": (0, 3, 7), "7": (0, 4, 7, 10), "m7": (0, 3, 7, 10), "maj7": (0, 4, 7, 11),
           "5": (0, 7), "sus4": (0, 5, 7), "sus2": (0, 2, 7), "dim": (0, 3, 6), "9": (0, 4, 7, 10, 14),
           "m9": (0, 3, 7, 10, 14), "add9": (0, 4, 7, 14), "7b9": (0, 4, 7, 10, 13)}
MODES = {"minor": (0, 2, 3, 5, 7, 8, 10), "major": (0, 2, 4, 5, 7, 9, 11), "dorian": (0, 2, 3, 5, 7, 9, 10),
         "mixolydian": (0, 2, 4, 5, 7, 9, 10), "harmonic": (0, 2, 3, 5, 7, 8, 11),
         "pent_minor": (0, 3, 5, 7, 10), "pent_major": (0, 2, 4, 7, 9), "in": (0, 1, 5, 7, 8)}


def note_midi(name):
    pc = NOTE[name[0]]
    rest = name[1:]
    while rest and rest[0] in "#b":
        pc += 1 if rest[0] == "#" else -1
        rest = rest[1:]
    return pc + 12 * (int(rest) + 1)


def parse_chord(sym):
    root = NOTE[sym[0]]
    rest = sym[1:]
    if rest[:1] in ("#", "b"):
        root += 1 if rest[0] == "#" else -1
        rest = rest[1:]
    return root % 12, QUALITY[rest]


# ------------------------------------------------------------- patterns
DRUM_PATTERNS = {
    "rock": {"K": "x.....x.x.......", "S": "....x.......x...", "h": "x.x.x.x.x.x.x.x."},
    "drive": {"K": "x.x...x.x.x...x.", "S": "....x.......x..g", "h": "xgxgxgxgxgxgxgxg"},
    "funk": {"K": "x..x..x...x..x..", "S": "....x..g.g..x..g", "h": "x.xgx.xgx.xgx.o."},
    "half": {"K": "x.......x.x.....", "S": "........x.......", "h": "x.x.x.x.x.x.x.x."},
    "gallop": {"K": "x..x..x.x..x..x.", "S": "....x.......x...", "t": "x.xxx.xxx.xxx.xx", "h": "x...x...x...x..."},
    "samba": {"U": "....x.......x...", "K": "x.......x.......", "a": "x.xx.x.xx.x.x.x.", "s": "xgxgxgxgxgxgxgxg",
              "S": "..g...g...g..g.g"},
    "afro": {"K": "x.....x...x.x...", "S": "....x.......x...", "a": "x.x.xx.x.x.xx.x.",
             "Q": "..x..xx...x..xx.", "s": "x.x.x.x.x.x.x.x."},
    "march": {"K": "x...x...x...x...", "S": "x.gxx.g.x.gxx.xx", "h": "x.x.x.x.x.x.x.x."},
    "techno": {"K": "x...x...x...x...", "C": "....x.......x...", "o": "..x...x...x...x.", "h": "xgxgxgxgxgxgxgxg"},
    "latin": {"K": "x..x..x.x..x..x.", "S": "....x.......x...", "Q": "..x.x.x...x.x.x.", "q": "......x.......x.",
              "b": "x.x.x.x.x.x.x.x."},
    "taiko": {"D": "x..x..x.x...x.x.", "K": "x.....x.x.......", "S": "....x.......x...", "h": "x.x.x.x.x.x.x.x."},
    "thai": {"K": "x..x..x.x.....x.", "S": "....x.......x...", "c": "..x...x...x...x.", "h": "x.x.x.x.x.x.x.x.",
             "Q": "x......x..x....."},
    "kulintang": {"K": "x..x....x.x.....", "S": "....x.......x...", "h": "x.xgx.xgx.xgx.xg", "b": "..x...x...x...x."},
    "shuffle": {"K": "x.......x.x.....", "S": "....x.......x...", "R": "x..x.xx..x.xx..x"},
    "intro": {"K": "x.......x.......", "h": "x.x.x.x.x.x.x.x."},
    "build": {"K": "x...x...x...x...", "S": "....x...x.x.xxxx"},
    "hits": {"K": "x.......x.......", "S": "x.......x.......", "X": "x..............."},
    "none": {},
}
FILLS = {"rock": {"S": "........x.x.xxxx", "T": "........x.......", "t": "..........x.....", "f": "............x..."},
         "toms": {"T": "........xx......", "t": "..........xx....", "f": "............xxxx"},
         "taiko": {"D": "........x.x.xxxx"}}

BASS_PATTERNS = {
    "rock8": "R ~ R ~ R ~ R ~ R ~ R ~ R ~ R ~",
    "drive": "R R R R R R R R R R R R R R R R",
    "funk": "R ~ - R' - R x - R - b7 - R' - 5 -",
    "funk2": "R ~ - - R' x R - - R b7 - 5 ~ R' -",
    "octave": "R - R' - R - R' - R - R' - R - R' -",
    "tumbao": "- - - 5 - - R ~ - - - 5 - - R ~",
    "gallop": "R - R R R - R R R - R R R - R R",
    "half": "R ~ ~ ~ ~ ~ R - - - 5 ~ R ~ - -",
    "afro": "R - - R - 5 - R - - b7 - 5 - R -",
    "samba": "R ~ - 5 R ~ - 5 R ~ - 5 R ~ - 5",
    "pulse": "- - R - - - R - - - R - - - R -",
    "techno": "R R' R R' R R' R R' R R' R R' R R' R R'",
    "whole": "R ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~",
    "march": "R ~ - - 5 ~ - - R ~ - - 5 ~ R -",
}
COMP_PATTERNS = {
    "pad": "x~~~~~~~~~~~~~~~", "half": "x~~~~~~~x~~~~~~~", "stab": "x......x..x.....", "stab2": "....x.......x...",
    "push": "x.....x.....x...", "chug": "x.x.x.x.x.x.x.x.", "chug16": "xxxxxxxxxxxxxxxx", "skank": "..x...x...x...x.",
    "strum": "x..x..x.x..x..x.", "comp": "x..x..x...x..x..", "hits": "x.......x.......", "last": "............x...",
    "arp16": "ARP16", "arp8": "ARP8", "updown": "UPDOWN",
}
RHYTHMS = ["x.x.x.x.", "x..xx.x.", "x~~.x.x.", "x.xx.x~~", "x~~~x.x.", "xx.x.x~~", "x~~x~~x.", "x.x~~~x."]


class Song:
    def __init__(self, name, bpm, key, mode, sections, inst, swing=0.0, loop=True, seed=1, master=1.0, gentle=False):
        self.name, self.bpm, self.key, self.mode = name, bpm, key, mode
        self.sections, self.inst, self.swing, self.loop, self.seed = sections, inst, swing, loop, seed
        self.step = 60.0 / bpm / 4
        self.master = master
        self.gentle = gentle          # a light touch of compression and no saturation: quiet, open songs

    def scale_notes(self, lo, hi):
        root = note_midi(self.key + "0") % 12
        return [m for m in range(lo, hi + 1) if (m - root) % 12 in MODES[self.mode]]


def bar_chords(section):
    out = []
    for token in section["chords"].split():
        halves = token.split(",")
        out.append([parse_chord(h) for h in halves])
    bars = section["bars"]
    return [out[i % len(out)] for i in range(bars)]


def chord_at(chords, bar, step):
    halves = chords[bar]
    return halves[0] if len(halves) == 1 or step < 8 else halves[1]


def voicing(chord, octave, top=None):
    root, ivs = chord
    base = root + 12 * (octave + 1)
    notes = [base + i for i in ivs]
    return [n if n < base + 12 or len(ivs) <= 3 else n - 12 for n in notes]


# ------------------------------------------------------------- events
class Track:
    def __init__(self, song, total_steps):
        self.song = song
        self.n = int((total_steps * song.step + 3.0) * RATE)
        self.buses = {}

    def time(self, step):
        s = self.song
        swing = s.swing * s.step if step % 2 == 1 else 0.0
        return step * s.step + swing

    def add(self, bus, start_step, length_steps, fn, *args, vel=1.0):
        t0 = self.time(start_step)
        n = max(1, int((self.time(start_step + length_steps) - t0) * RATE))
        key = (bus, fn, args, n, round(vel, 2))
        cache = _CACHE.setdefault(key, None)
        if cache is None:
            cache = fn(*args, n, vel) if args else fn(n, vel)
            _CACHE[key] = cache
        buf = self.buses.setdefault(bus, np.zeros(self.n))
        i = int(t0 * RATE)
        seg = cache[:max(0, self.n - i)]
        buf[i:i + len(seg)] += seg


_CACHE = {}


def drums(track, start, bars, name, fill=None, gain=1.0):
    pat = DRUM_PATTERNS[name]
    for bar in range(bars):
        lanes = dict(pat)
        if fill and bar == bars - 1:
            # the second half of the last bar: the fill instead of the groove (the kick stays)
            lanes = {k: row[:8] + (row[8:] if k == "K" else "." * 8) for k, row in pat.items()}
            for k, row in FILLS[fill].items():
                lanes[k] = lanes.get(k, "." * 16)[:8] + row[8:]
        for key, row in lanes.items():
            for i, c in enumerate(row):
                if c in "xg":
                    vel = (1.0 if c == "x" else 0.45) * (1.0 if i % 4 == 0 else 0.85) * gain
                    track.add("drums", start + bar * 16 + i, 2, DRUMS[key], vel=vel)


def bass(track, start, chords, name, inst, octave=1, gain=1.0):
    tokens = BASS_PATTERNS[name].split()
    fn = INSTRUMENTS[inst]
    for bar in range(len(chords)):
        i = 0
        while i < 16:
            tok = tokens[i % len(tokens)]
            if tok in ("-", "~"):
                i += 1
                continue
            length = 1
            while i + length < 16 and tokens[(i + length) % len(tokens)] == "~":
                length += 1
            root, ivs = chord_at(chords, bar, i)
            base = root + 12 * (octave + 1)
            if base < 28:
                base += 12
            third = ivs[1] if len(ivs) > 1 and ivs[1] in (3, 4) else 4
            seventh = next((v for v in ivs if v in (10, 11)), 10)
            pitch = {"R": base, "R'": base + 12, "5": base + 7, "3": base + third, "b7": base + seventh,
                     "x": base}[tok]
            vel = (0.55 if tok == "x" else 1.0) * gain
            ln = 0.6 if tok == "x" else length - 0.15
            track.add("bass", start + bar * 16 + i, ln, fn, hz(pitch), vel=vel)
            i += length


def comp(track, start, chords, part):
    fn = INSTRUMENTS[part["inst"]]
    pat = COMP_PATTERNS[part["pat"]]
    octave, gain, bus = part.get("oct", 4), part.get("gain", 0.5), part.get("bus", "comp")
    power = part["inst"] in ("power", "mute", "drone", "berimbau")      # one note: the root
    for bar in range(len(chords)):
        if pat in ("ARP16", "ARP8", "UPDOWN"):
            stride = 1 if pat != "ARP8" else 2
            for i in range(0, 16, stride):
                notes = voicing(chord_at(chords, bar, i), octave) + [voicing(chord_at(chords, bar, i), octave)[0] + 12]
                k = i // stride
                if pat == "UPDOWN":
                    cyc = notes + notes[-2:0:-1]
                    note = cyc[k % len(cyc)]
                else:
                    note = notes[k % len(notes)]
                track.add(bus, start + bar * 16 + i, stride * 0.9, fn, hz(note), vel=gain * (1 if k % 2 == 0 else .8))
            continue
        i = 0
        while i < 16:
            if pat[i] != "x":
                i += 1
                continue
            length = 1
            while i + length < 16 and pat[i + length] == "~":
                length += 1
            chord = chord_at(chords, bar, i)
            if power:
                root = chord[0] + 12 * (octave + 1)
                if root > 52:
                    root -= 12
                track.add(bus, start + bar * 16 + i, length * 0.95 if length > 1 else 0.8, fn, hz(root), vel=gain)
            else:
                for j, note in enumerate(voicing(chord, octave)):
                    strum = j * 0.25 if part["inst"] in ("nylon", "clean") else 0
                    track.add(bus, start + bar * 16 + i + strum, max(0.7, length - 0.1), fn, hz(note), vel=gain)
            i += length


def melody(track, start, text, inst, gain=0.6, grid=8, transpose=0, bus="lead", harm=None, song=None):
    fn = INSTRUMENTS[inst]
    tokens = [t for t in text.replace("|", " ").split()]
    stride = 16 // grid
    i = 0
    while i < len(tokens):
        tok = tokens[i]
        if tok in ("-", "~"):
            i += 1
            continue
        length = 1
        while i + length < len(tokens) and tokens[i + length] == "~":
            length += 1
        pitch = note_midi(tok) + transpose
        vel = gain * (1.0 if (i * stride) % 8 == 0 else 0.88)
        track.add(bus, start + i * stride, length * stride - 0.2, fn, hz(pitch), vel=vel)
        if harm and song:
            scale = song.scale_notes(pitch - 14, pitch + 2)
            below = [m for m in scale if m < pitch]
            if len(below) >= abs(harm):
                track.add(bus, start + i * stride, length * stride - 0.2, fn, hz(below[harm]), vel=vel * 0.7)
        i += length


def improvise(song, chords, seed, lo, hi, density=0.75):
    """An eighth-note line: chord tones on the strong beats, scale steps between."""
    rng = np.random.default_rng(seed)
    scale = song.scale_notes(lo, hi)
    out, prev = [], scale[len(scale) // 2]
    for bar in range(len(chords)):
        rhythm = RHYTHMS[rng.integers(len(RHYTHMS))] if rng.random() < density else "x~~~~~~."
        for k, c in enumerate(rhythm):
            if c != "x":
                out.append("~" if c == "~" else "-")
                continue
            root, ivs = chord_at(chords, bar, k * 2)
            tones = {(root + i) % 12 for i in ivs}
            cands = [m for m in scale if abs(m - prev) <= 7]
            if k % 2 == 0:
                cands = [m for m in cands if m % 12 in tones] or cands
            weights = np.array([1.0 / (1 + abs(m - prev)) for m in cands])
            if prev >= hi - 4:
                weights *= np.array([2.0 if m < prev else 0.5 for m in cands])
            if prev <= lo + 4:
                weights *= np.array([2.0 if m > prev else 0.5 for m in cands])
            prev = cands[rng.choice(len(cands), p=weights / weights.sum())]
            names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
            out.append(f"{names[prev % 12]}{prev // 12 - 1}")
    return " ".join(out)


# Bus levels (dB RMS while playing): the melody on top, then drums, bass, chords.
TARGET = {"drums": -14, "bass": -17, "comp": -21, "pad": -25, "arp": -23, "lead": -12}


def active_db(x, frame=1024):
    n = len(x) // frame
    rms = np.sqrt((x[:n * frame].reshape(n, frame) ** 2).mean(axis=1))
    on = rms[rms > rms.max() * 0.06]
    return 20 * np.log10(np.sqrt((on ** 2).mean()) + 1e-12) if len(on) else 0.0


def render_song(song):
    bars = sum(s["bars"] for s in song.sections)
    passes = 2 if song.loop else 1
    total = bars * 16 * passes
    track = Track(song, total)
    pos = 0
    for p in range(passes):
        for si, sec in enumerate(song.sections):
            chords = bar_chords(sec)
            if sec.get("drums"):
                drums(track, pos, sec["bars"], sec["drums"], sec.get("fill"), sec.get("drum_gain", 1.0))
            if sec.get("crash", True) and sec.get("drums") not in (None, "none"):
                track.add("drums", pos, 4, DRUMS["X"], vel=0.8)
            if sec.get("bass"):
                bass(track, pos, chords, sec["bass"], sec.get("bass_inst", song.inst["bass"]),
                     sec.get("bass_oct", 1), sec.get("bass_gain", 1.0))
            for part in sec.get("comp", []):
                comp(track, pos, chords, part)
            for line in sec.get("lines", []):
                text = line["text"]
                if text == "SOLO":
                    text = improvise(song, chords, song.seed * 100 + si + p * 0, note_midi(line.get("lo", "A4")),
                                     note_midi(line.get("hi", "E6")), line.get("density", 0.8))
                melody(track, pos, text, line["inst"], line.get("gain", 0.6), line.get("grid", 8),
                       line.get("transpose", 0), line.get("bus", "lead"), line.get("harm"), song)
            pos += sec["bars"] * 16
    sends = {"drums": 0.12, "bass": 0.0, "comp": 0.22, "pad": 0.4, "lead": 0.3, "arp": 0.25}
    offsets = song.inst.get("mix", {})
    dry = np.zeros(track.n)
    wet = np.zeros(track.n)
    for bus, buf in track.buses.items():
        gain = 10 ** ((TARGET[bus] + offsets.get(bus, 0) - active_db(buf)) / 20)
        dry += buf * gain
        wet += buf * gain * sends.get(bus, 0.2)
    wet = signal.fftconvolve(wet, reverb_ir(1.6))[:track.n] * 0.9
    mix = highpass(dry + wet, 32)
    if song.gentle:
        mix = compress(mix / max(np.abs(mix).max(), 1e-6), threshold=0.6, ratio=1.8)
    else:
        mix = compress(mix / max(np.abs(mix).max(), 1e-6), threshold=0.3, ratio=3.0)
        mix = np.tanh(mix * 1.6) / np.tanh(1.6)
    length = int(bars * 16 * song.step * RATE)
    if song.loop:
        mix = mix[length:2 * length]                                # the second pass: tails wrap around
    else:
        mix = mix[:length + int(1.5 * RATE)]
        mix[-int(0.5 * RATE):] *= np.linspace(1, 0, int(0.5 * RATE))
    return (mix / max(np.abs(mix).max(), 1e-6) * 0.89 * song.master * 32000).astype(np.int16)


# ------------------------------------------------------------- the songs
def S(bars, chords, drums=None, bass=None, comp=(), lines=(), fill=None, **kw):
    d = {"bars": bars, "chords": chords, "drums": drums, "bass": bass, "comp": list(comp), "lines": list(lines),
         "fill": fill}
    d.update(kw)
    return d


def L(inst, text, gain=0.6, **kw):
    d = {"inst": inst, "text": text, "gain": gain}
    d.update(kw)
    return d


def P(inst, pat, octave=4, gain=0.5, bus="comp"):
    return {"inst": inst, "pat": pat, "oct": octave, "gain": gain, "bus": bus}


def write_wav(path, samples, rate=RATE):
    """A mono 16-bit WAV, as the SDK's sample encoders read them."""
    import wave
    from pathlib import Path
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(np.asarray(samples, dtype=np.int16).tobytes())
