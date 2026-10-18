#!/usr/bin/env python3
"""
No Signal Z Alpha sound: effects and tunes are synthesized here; the voice
barks are short lines rendered once into sound/voices/ (see sound/CREDITS.txt).

    python3 games/nosignal/tools/sound.py      writes sound/samples/in_wav_a/<n>.wav (effects)
                                               and in_wav_b/<n>.wav (music) for `make sound`

The tunes are original compositions written for this game, rendered with the
SDK's instruments (sound/tools/ng_synth.py) by its sequencer (ng_songs.py).
The order matches scenes/ns.h: NS_SFX_* is effect n (file n + 1); NS_MUS_* is
track n (file n + 1, from NS_MUS_CLEAR on n + 2: 5.wav is the eye-catcher's).
Each stage has its own tune; the bosses share one theme, the last two a
second.
"""

from pathlib import Path
import sys
import wave

import numpy as np

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / "sound/tools"))
import ng_synth as syn  # noqa: E402
from ng_songs import L, P, S, Song, render_song, write_wav  # noqa: E402

RATE = syn.RATE
VOICES = GAME / "sound/voices"
OUT_A = GAME / "sound/samples/in_wav_a"
OUT_B = GAME / "sound/samples/in_wav_b"


def n_of(seconds):
    return int(seconds * RATE)


def norm(x, peak=0.8):
    x = np.asarray(x, dtype=np.float64)
    return (x / max(np.abs(x).max(), 1e-9) * peak * 32767).astype(np.int16)


def chirp(f0, f1, seconds, shape="sine"):
    n = n_of(seconds)
    f = np.geomspace(f0, f1, n)
    ph = 2 * np.pi * np.cumsum(f) / RATE
    if shape == "square":
        return np.sign(np.sin(ph)) * 0.6
    if shape == "saw":
        return ((ph / (2 * np.pi)) % 1.0) * 2 - 1
    return np.sin(ph)


def seq(*parts, gap=0.0):
    out = []
    for p in parts:
        out.append(p)
        if gap:
            out.append(np.zeros(n_of(gap)))
    return np.concatenate(out)


def mix(*parts):
    n = max(len(p) for p in parts)
    out = np.zeros(n)
    for p in parts:
        out[:len(p)] += p
    return out


def note(inst, name, seconds, vel=1.0):
    from ng_songs import note_midi
    return syn.INSTRUMENTS[inst](syn.hz(note_midi(name)), n_of(seconds), vel)


def hit(seconds, lo, hi, body_hz, snap=0.012, body=0.6):
    """A punch: a band of noise that snaps shut over a short low thump."""
    n = n_of(seconds)
    crack = syn.bandpass(syn.noise(n), lo, hi) * syn.decay(n, snap)
    thump = syn.tom(n, 1.0, body_hz) * body
    return mix(crack, thump)


def engine(seconds, f0, f1, rough=0.35):
    """A small motor: a buzzy pulse whose rate climbs from f0 to f1."""
    n = n_of(seconds)
    f = np.geomspace(f0, f1, n)
    ph = 2 * np.pi * np.cumsum(f) / RATE
    buzz = np.sign(np.sin(ph)) * 0.5 + np.sin(2 * ph) * 0.3
    grit = syn.bandpass(syn.noise(n), 200, 1800) * rough
    return syn.lowpass(buzz + grit * np.abs(np.sin(ph * 0.5)), 2600)


def voice(key, gain=1.0):
    """A rendered line from sound/voices, resampled to the effect rate."""
    with wave.open(str(VOICES / f"{key}.wav")) as w:
        rate = w.getframerate()
        pcm = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(float) / 32767
    if rate != RATE:
        pos = np.arange(0, len(pcm) - 1, rate / RATE)
        pcm = np.interp(pos, np.arange(len(pcm)), pcm)
    return syn.highpass(pcm, 120) * gain


def effects():
    fx = []
    # 0 coin: a notification ping
    fx.append(seq(note("steel", "E6", 0.07), note("steel", "B6", 0.3)))
    # 1 start: an unlock jingle
    fx.append(seq(*[note("marimba", m, 0.06) for m in ("E5", "G5", "B5")], mix(note("steel", "E6", 0.4), note("marimba", "B6", 0.4, 0.6))))
    # 2 select: a swipe tick
    fx.append(mix(chirp(1800, 2600, 0.05) * syn.decay(n_of(0.05), 0.02), 0.3 * syn.noise(n_of(0.03)) * syn.decay(n_of(0.03), 0.006)))
    # 3 punch, 4 kick, 5 heavy
    fx.append(hit(0.14, 900, 4200, 170, 0.014, 0.7))
    fx.append(hit(0.18, 600, 3200, 120, 0.02, 0.9))
    fx.append(mix(hit(0.3, 400, 2600, 85, 0.035, 1.2), syn.kick(n_of(0.3), 1.0) * 0.6))
    # 6 whoosh
    n = n_of(0.2)
    fx.append(syn.bandpass(syn.noise(n), 700, 3800) * np.sin(np.linspace(0, np.pi, n)) ** 2)
    # 7 hurt: a short squelch
    fx.append(mix(chirp(520, 260, 0.16, "square") * syn.decay(n_of(0.16), 0.08) * 0.6, hit(0.12, 1000, 3000, 140, 0.01, 0.4)))
    # 8 fall: a body hitting pavement
    n = n_of(0.4)
    fx.append(mix(syn.kick(n, 1.0), syn.lowpass(syn.noise(n), 900) * syn.decay(n, 0.09) * 0.8,
                  np.concatenate([np.zeros(n_of(0.12)), syn.lowpass(syn.noise(n_of(0.2)), 700) * syn.decay(n_of(0.2), 0.05) * 0.5])))
    # 9 grab: cloth
    n = n_of(0.16)
    fx.append(syn.bandpass(syn.noise(n), 1500, 6000) * syn.decay(n, 0.05) * 0.8)
    # 10 throw: a heave and a whoosh
    n = n_of(0.32)
    fx.append(mix(chirp(240, 120, 0.12, "saw") * syn.decay(n_of(0.12), 0.06) * 0.5,
                  np.concatenate([np.zeros(n_of(0.06)), syn.bandpass(syn.noise(n_of(0.26)), 500, 3000) * np.sin(np.linspace(0, np.pi, n_of(0.26))) ** 2])))
    # 11 special: a power-up sweep into a hit
    fx.append(seq(mix(chirp(300, 2400, 0.35, "saw") * np.linspace(0.2, 1, n_of(0.35)) * 0.5, syn.bandpass(syn.noise(n_of(0.35)), 2000, 8000) * np.linspace(0, 0.6, n_of(0.35))),
                  hit(0.3, 500, 4000, 90, 0.04, 1.2)))
    # 12 break: wood splintering
    n = n_of(0.4)
    clatter = np.zeros(n)
    rng = np.random.default_rng(12)
    for k in range(9):
        i = int(rng.integers(0, n - n_of(0.08)))
        c = syn.bandpass(syn.noise(n_of(0.08)), 400, 2400) * syn.decay(n_of(0.08), 0.015)
        clatter[i:i + len(c)] += c * rng.uniform(0.4, 1.0)
    fx.append(mix(hit(0.2, 300, 1800, 110, 0.03, 0.8), clatter))
    # 13 pickup: a coin-like blip
    fx.append(seq(note("steel", "B5", 0.05), note("steel", "E6", 0.18)))
    # 14 food: a crunch and a happy blip
    n = n_of(0.12)
    crunch = syn.bandpass(syn.noise(n), 1200, 7000) * (np.abs(np.sin(np.linspace(0, 9 * np.pi, n)))) * syn.decay(n, 0.1)
    fx.append(seq(crunch, seq(*[note("marimba", m, 0.06) for m in ("C6", "E6", "G6")], note("marimba", "C7", 0.25))))
    # 15 scooter: a little electric motor winding up
    fx.append(engine(0.6, 60, 140, 0.15) * syn.env_adsr(n_of(0.6), 0.03, 0.1, 0.9, 0.15) * 0.8)
    # 16 crash
    n = n_of(0.6)
    fx.append(mix(syn.cymbal(n, 1.0) * 0.5, hit(0.4, 300, 5000, 80, 0.05, 1.1), syn.lowpass(syn.noise(n), 3000) * syn.decay(n, 0.15) * 0.6))
    # 17 phone: a thrown phone ringing as it flies
    tone = mix(chirp(1320, 1320, 0.25), chirp(1760, 1760, 0.25) * 0.6) * (np.sin(np.linspace(0, 16 * np.pi, n_of(0.25))) > 0)
    fx.append(tone * syn.decay(n_of(0.25), 0.2) * 0.7)
    # 18 bike: a motorbike revving
    fx.append(seq(engine(0.35, 40, 90, 0.5), engine(0.45, 90, 55, 0.5) * np.linspace(1, 0, n_of(0.45))))
    # 19 go: two bright beeps
    fx.append(seq(chirp(1400, 1400, 0.08, "square") * 0.5, chirp(1400, 1400, 0.08, "square") * 0.5, gap=0.05))
    # 20 ko: a deep impact with a ring
    n = n_of(0.7)
    fx.append(mix(syn.kick(n, 1.0) * 1.2, hit(0.35, 400, 3000, 70, 0.05, 1.0), note("steel", "E5", 0.7, 0.35)))
    # 21 text: a typing click
    n = n_of(0.03)
    fx.append(syn.bandpass(syn.noise(n), 2500, 7000) * syn.decay(n, 0.006))
    # 22 jump: a quick rising swish
    n = n_of(0.14)
    fx.append(mix(chirp(400, 900, 0.14) * syn.decay(n, 0.08) * 0.4, syn.bandpass(syn.noise(n), 1500, 5000) * np.sin(np.linspace(0, np.pi, n)) * 0.5))
    # 23 land: sneakers on asphalt
    n = n_of(0.1)
    fx.append(mix(syn.lowpass(syn.noise(n), 1500) * syn.decay(n, 0.025), syn.tom(n, 0.6, 90) * 0.4))
    # 24-35: each hero's four lines (bruh, no cap, you're cooked, massive W)
    for hero in ("jay", "zoe", "kai"):
        for k in range(4):
            fx.append(voice(f"{hero}{k}"))
    # 36 Rico: "fine. we ride!", 37 his laugh, 38 the announcer: "no signal."
    fx.append(voice("rico_ride"))
    fx.append(voice("rico_laugh"))
    fx.append(voice("nosignal"))
    # 39 zap: a stun baton
    n = n_of(0.22)
    buzz = np.sign(np.sin(2 * np.pi * 70 * np.arange(n) / RATE)) * syn.bandpass(syn.noise(n), 2000, 8000)
    fx.append(buzz * syn.decay(n, 0.12) * 0.8)
    # 40 laser
    fx.append(chirp(2400, 500, 0.25, "square") * syn.decay(n_of(0.25), 0.15) * 0.6)
    # 41 roar: a big growl
    n = n_of(0.8)
    growl = syn.lowpass(syn.noise(n), 600) * (1 + 0.6 * np.sin(2 * np.pi * 28 * np.arange(n) / RATE))
    fx.append(mix(growl * syn.env_adsr(n, 0.08, 0.2, 0.7, 0.3), chirp(110, 70, 0.8, "saw") * 0.4 * syn.env_adsr(n, 0.08, 0.2, 0.7, 0.3)))
    # 42 servo: a robot moving
    fx.append(seq(chirp(500, 900, 0.12, "square") * 0.35, chirp(900, 600, 0.1, "square") * 0.35))
    # 43 warble: alien tech
    n = n_of(0.5)
    t = np.arange(n) / RATE
    fx.append(np.sin(2 * np.pi * (600 + 300 * np.sin(2 * np.pi * 9 * t)) * t) * syn.env_adsr(n, 0.02, 0.1, 0.8, 0.2) * 0.6)
    # 44 alarm
    fx.append(seq(*[chirp(880, 880, 0.12, "square") * 0.45 for _ in range(3)], gap=0.06))
    # 45 gas: a sparkly puff
    n = n_of(0.5)
    sparkle = syn.bandpass(syn.noise(n), 3000, 9000) * np.abs(np.sin(np.linspace(0, 30 * np.pi, n)))
    fx.append(mix(syn.lowpass(syn.noise(n), 1200) * syn.env_adsr(n, 0.05, 0.1, 0.6, 0.3), sparkle * 0.4))
    # 46 stomp
    n = n_of(0.35)
    fx.append(mix(syn.kick(n, 1.0) * 1.2, syn.lowpass(syn.noise(n), 400) * syn.decay(n, 0.12)))
    return fx


def songs():
    # 1. Title: E dorian street funk, slap bass and a sax hook.
    title = Song("title", 112, "E", "dorian", [
        S(2, "Em7 A7", "intro", "funk", [P("epiano", "stab", 4, 0.4)], crash=True),
        S(8, "Em7 A7 Em7 A7 Gmaj7 F#m7 Em7 A7", "funk", "funk", [P("epiano", "comp", 4, 0.45), P("power", "stab2", 3, 0.3)],
          [L("sax", "E5 ~ G5 ~ A5 ~ - B5 | - - A5 G5 E5 ~ - - | E5 ~ G5 ~ A5 ~ D6 ~ | B5 ~ ~ ~ - - A5 ~ |"
                    "B5 ~ D6 ~ F#6 ~ E6 D6 | C#6 ~ A5 ~ - - F#5 ~ | G5 ~ A5 ~ B5 ~ G5 ~ | E5 ~ ~ ~ - - - -", 0.6)], fill="rock"),
        S(8, "Cmaj7 Bm7 Am7 D7 Cmaj7 Bm7 Am7 B7", "drive", "octave", [P("brass", "stab", 4, 0.45), P("arp", "arp8", 5, 0.22, "arp")],
          [L("brass", "G5 - G5 - A5 ~ B5 ~ | D6 ~ B5 ~ A5 ~ - - | C6 - C6 - B5 ~ A5 ~ | F#5 ~ ~ ~ D5 ~ - - |"
                      "G5 - G5 - A5 ~ B5 ~ | D6 ~ E6 ~ F#6 ~ D6 ~ | E6 ~ D6 ~ C6 ~ A5 ~ | B5 ~ ~ ~ D#6 ~ - -", 0.55)], fill="toms"),
    ], {"bass": "slap_bass"}, swing=0.1, seed=841)
    # 2. Hero select: a short, bouncy loop.
    select = Song("select", 120, "A", "minor", [
        S(4, "Am7 D7 Fmaj7 E7", "funk", "funk2", [P("clean", "skank", 4, 0.4), P("epiano", "stab", 4, 0.35)],
          [L("lead", "A5 ~ C6 ~ E6 ~ D6 C6 | D6 ~ ~ ~ A5 ~ - - | C6 ~ A5 ~ F5 ~ A5 ~ | G#5 ~ ~ ~ B5 ~ - -", 0.5)]),
        S(4, "Am7 D7 Fmaj7 E7", "funk", "funk2", [P("clean", "skank", 4, 0.4), P("brass", "stab", 4, 0.35)],
          [L("lead", "E6 ~ D6 ~ C6 ~ A5 ~ | F#6 ~ ~ ~ D6 ~ - - | A6 ~ G6 ~ F6 ~ E6 ~ | E6 ~ ~ ~ - - - -", 0.5)]),
    ], {"bass": "synth_bass"}, swing=0.08, seed=842)
    # 3. Stage 1, the hood: D minor, a driving rock-funk groove.
    stage1 = Song("hood", 128, "D", "minor", [
        S(2, "Dm C", "build", "drive", [P("mute", "chug", 3, 0.35)], crash=True),
        S(8, "Dm Bb C Dm Dm Bb Gm A", "drive", "funk", [P("power", "push", 3, 0.4), P("organ", "skank", 4, 0.25)],
          [L("lead", "D5 ~ F5 ~ A5 ~ - G5 | F5 ~ D5 ~ - - Bb4 ~ | C5 ~ E5 ~ G5 ~ F5 E5 | D5 ~ ~ ~ - - A4 ~ |"
                     "D5 ~ F5 ~ A5 ~ C6 ~ | D6 ~ ~ ~ Bb5 ~ G5 ~ | A5 ~ G5 ~ F5 ~ E5 ~ | C#5 ~ ~ ~ E5 ~ - -", 0.6)], fill="rock"),
        S(8, "Gm Dm Bb F Gm Dm Bb A", "rock", "octave", [P("power", "chug", 3, 0.35), P("brass", "stab2", 4, 0.4)],
          [L("guitar_lead", "G5 ~ Bb5 ~ D6 ~ C6 Bb5 | A5 ~ ~ ~ F5 ~ D5 ~ | F5 ~ ~ ~ D6 ~ C6 ~ | A5 ~ ~ ~ - - F5 ~ |"
                            "G5 ~ Bb5 ~ D6 ~ F6 ~ | E6 ~ D6 ~ A5 ~ ~ ~ | Bb5 ~ C6 ~ D6 ~ Bb5 ~ | A5 ~ ~ ~ C#6 ~ E6 ~", 0.55)], fill="toms"),
    ], {"bass": "pick_bass"}, seed=843)
    # 4. Boss: C minor, faster, synth and power chords.
    boss = Song("boss", 142, "C", "minor", [
        S(2, "Cm Cm", "hits", "whole", [P("strings", "pad", 3, 0.4, "pad")], crash=True),
        S(8, "Cm Ab Bb Cm Cm Ab Fm G", "techno", "techno", [P("power", "chug16", 3, 0.3), P("stab", "push", 4, 0.35)],
          [L("lead", "C6 ~ - C6 Eb6 ~ G6 ~ | Ab6 ~ G6 ~ Eb6 ~ - - | F6 ~ - F6 D6 ~ Bb5 ~ | C6 ~ ~ ~ G5 ~ - - |"
                     "C6 ~ - C6 Eb6 ~ G6 ~ | C7 ~ ~ ~ Ab6 ~ - - | F6 ~ Eb6 ~ D6 ~ C6 ~ | B5 ~ ~ ~ D6 ~ G6 ~", 0.6)], fill="rock"),
        S(4, "Ab Bb G Cm", "drive", "drive", [P("brass", "chug", 4, 0.35), P("arp", "arp16", 5, 0.2, "arp")],
          [L("guitar_lead", "Eb6 ~ F6 ~ G6 ~ Ab6 ~ | F6 ~ ~ ~ D6 ~ Bb5 ~ | B5 ~ D6 ~ G6 ~ F6 ~ | Eb6 ~ ~ ~ C6 ~ - -", 0.55)], fill="toms"),
    ], {"bass": "synth_bass"}, seed=844)
    # 5. The SDK's eye-catcher slot: a short chime.
    chime = Song("chime", 120, "C", "major", [
        S(2, "C G,C", None, None, [P("strings", "pad", 4, 0.4, "pad")],
          [L("steel", "C6 E6 G6 C7 ~ ~ ~ ~ | B6 ~ G6 ~ C7 ~ ~ ~", 0.6)], crash=False),
    ], {"bass": "pick_bass"}, loop=False, seed=845)
    # 6. Stage clear: a brass fanfare loop.
    clear = Song("clear", 132, "F", "major", [
        S(4, "F Bb C F", "funk", "octave", [P("brass", "stab", 4, 0.45), P("epiano", "comp", 4, 0.3)],
          [L("trumpet", "F5 A5 C6 ~ A5 ~ F5 ~ | Bb5 ~ D6 ~ C6 ~ Bb5 ~ | A5 ~ G5 ~ E5 ~ C5 ~ | F5 ~ ~ ~ - - - -", 0.6, harm=-2)]),
    ], {"bass": "slap_bass"}, seed=846)
    # 7. Connection lost: a slow, sad sax.
    over = Song("lost", 76, "A", "minor", [
        S(8, "Am7 Dm7 G7 Cmaj7 Fmaj7 Bdim E7 Am7", "half", "half", [P("epiano", "comp", 4, 0.4), P("strings", "pad", 3, 0.3, "pad")],
          [L("sax", "E5 ~ ~ ~ C5 ~ A4 ~ | F5 ~ ~ ~ D5 ~ ~ ~ | D5 ~ B4 ~ G4 ~ ~ ~ | E5 ~ ~ ~ ~ ~ ~ ~ |"
                    "C5 ~ A4 ~ F4 ~ ~ ~ | D5 ~ ~ ~ B4 ~ ~ ~ | G#4 ~ B4 ~ D5 ~ ~ ~ | A4 ~ ~ ~ - - - -", 0.45)]),
    ], {"bass": "fm_bass"}, seed=847)
    # 8. Scenes: light, sly electric piano funk.
    scene = Song("scene", 100, "G", "dorian", [
        S(8, "Gm7 C7 Gm7 C7 Ebmaj7 D7 Gm7 C7", "funk", "funk2", [P("epiano", "comp", 4, 0.45), P("clean", "skank", 4, 0.25)],
          [L("flute", "D5 ~ F5 ~ G5 ~ - - | E5 ~ ~ ~ C5 ~ - - | D5 ~ F5 ~ Bb5 ~ A5 ~ | G5 ~ ~ ~ - - - - |"
                      "G5 ~ Bb5 ~ D6 ~ C6 ~ | A5 ~ F#5 ~ D5 ~ - - | F5 ~ G5 ~ A5 ~ F5 ~ | G5 ~ ~ ~ - - - -", 0.45)]),
    ], {"bass": "pick_bass"}, swing=0.12, seed=848)
    # 9. The ending: everyone home, phone in hand.
    ending = Song("ending", 108, "D", "major", [
        S(8, "D A Bm G D A G,A D", "half", "rock8", [P("clean", "strum", 4, 0.4), P("strings", "pad", 4, 0.35, "pad")],
          [L("sax", "F#5 ~ A5 ~ D6 ~ C#6 B5 | A5 ~ ~ ~ E5 ~ - - | F#5 ~ B5 ~ D6 ~ E6 ~ | D6 ~ ~ ~ B5 ~ - - |"
                    "F#5 ~ A5 ~ D6 ~ F#6 ~ | E6 ~ ~ ~ C#6 ~ A5 ~ | B5 ~ A5 ~ G5 ~ E5 ~ | D5 ~ ~ ~ - - - -", 0.55)], fill="rock"),
        S(8, "G A F#m Bm G A D D", "funk", "octave", [P("brass", "stab", 4, 0.4), P("epiano", "comp", 4, 0.3)],
          [L("trumpet", "B5 ~ A5 ~ G5 ~ F#5 ~ | E5 ~ F#5 ~ G5 ~ A5 ~ | C#6 ~ ~ ~ A5 ~ F#5 ~ | D6 ~ ~ ~ B5 ~ F#5 ~ |"
                        "B5 ~ A5 ~ G5 ~ B5 ~ | C#6 ~ D6 ~ E6 ~ C#6 ~ | D6 ~ ~ ~ A5 ~ F#5 ~ | D5 ~ ~ ~ - - - -", 0.55)], fill="toms"),
    ], {"bass": "slap_bass"}, seed=849)
    # 10. Stage 2, the underpass: A minor synthwave.
    s2 = Song("underpass", 118, "A", "minor", [
        S(2, "Am F", "build", "drive", [P("strings", "pad", 3, 0.4, "pad")], crash=True),
        S(8, "Am F C G Am F Dm E", "drive", "drive", [P("arp", "arp16", 5, 0.3, "arp"), P("strings", "pad", 4, 0.3, "pad")],
          [L("lead", "E5 ~ A5 ~ C6 ~ B5 A5 | F5 ~ ~ ~ A5 ~ C6 ~ | G5 ~ E5 ~ C5 ~ E5 G5 | D5 ~ ~ ~ G5 ~ B5 ~ |"
                     "E5 ~ A5 ~ E6 ~ D6 C6 | A5 ~ ~ ~ F5 ~ A5 ~ | D6 ~ C6 ~ A5 ~ F5 ~ | G#5 ~ ~ ~ B5 ~ E6 ~", 0.6)], fill="rock"),
    ], {"bass": "synth_bass"}, seed=850)
    # 11. Stage 3, the outbreak: E harmonic minor, tense funk.
    s3 = Song("outbreak", 124, "E", "harmonic", [
        S(8, "Em C Am B Em C F#m,B Em", "funk", "funk", [P("organ", "stab", 4, 0.35), P("clean", "skank", 4, 0.3)],
          [L("sax", "E5 ~ G5 ~ B5 ~ C6 B5 | A5 ~ ~ ~ E5 ~ - - | C5 ~ E5 ~ A5 ~ G5 F#5 | D#5 ~ ~ ~ B4 ~ - - |"
                    "E5 ~ G5 ~ B5 ~ E6 ~ | D#6 ~ ~ ~ C6 ~ B5 ~ | A5 ~ F#5 ~ D#5 ~ B4 ~ | E5 ~ ~ ~ - - - -", 0.55)], fill="toms"),
    ], {"bass": "slap_bass"}, swing=0.08, seed=851)
    # 12. Stage 4, the lab: F minor, industrial electronics.
    s4 = Song("lab", 132, "F", "minor", [
        S(2, "Fm Fm", "techno", "techno", [P("stab", "push", 4, 0.3)], crash=True),
        S(8, "Fm Db Ab Eb Fm Db Bbm C", "techno", "techno", [P("arp", "arp16", 5, 0.3, "arp"), P("stab", "chug", 4, 0.25)],
          [L("lead", "F5 ~ - F5 Ab5 ~ C6 ~ | Db6 ~ C6 ~ Ab5 ~ - - | Eb6 ~ - Eb6 C6 ~ Ab5 ~ | Bb5 ~ ~ ~ G5 ~ - - |"
                     "F5 ~ - F5 Ab5 ~ F6 ~ | Eb6 ~ Db6 ~ C6 ~ - - | Bb5 ~ C6 ~ Db6 ~ Bb5 ~ | C6 ~ ~ ~ E5 ~ G5 ~", 0.55)], fill="rock"),
    ], {"bass": "synth_bass"}, seed=852)
    # 13. Stage 5, concrete jungle: G mixolydian rock with drums from everywhere.
    s5 = Song("jungle", 126, "G", "mixolydian", [
        S(8, "G F C G G F C D", "afro", "afro", [P("power", "push", 3, 0.35), P("brass", "stab2", 4, 0.35)],
          [L("guitar_lead", "G5 ~ B5 ~ D6 ~ F6 ~ | E6 ~ C6 ~ A5 ~ - - | G5 ~ C6 ~ E6 ~ D6 C6 | B5 ~ ~ ~ G5 ~ - - |"
                            "G5 ~ B5 ~ D6 ~ G6 ~ | F6 ~ E6 ~ C6 ~ - - | E6 ~ D6 ~ C6 ~ A5 ~ | D6 ~ ~ ~ F#6 ~ - -", 0.55)], fill="toms"),
    ], {"bass": "pick_bass"}, seed=853)
    # 14. Stage 6, the machine city: D minor industrial techno.
    s6 = Song("machines", 136, "D", "minor", [
        S(8, "Dm Dm Bb C Dm Dm Gm A", "techno", "techno", [P("stab", "chug16", 3, 0.25), P("organ", "stab2", 4, 0.3)],
          [L("lead", "D6 ~ D6 ~ F6 ~ D6 ~ | A5 ~ - - C6 ~ D6 ~ | Bb5 ~ D6 ~ F6 ~ E6 D6 | C6 ~ ~ ~ E6 ~ - - |"
                     "D6 ~ D6 ~ F6 ~ A6 ~ | G6 ~ F6 ~ E6 ~ D6 ~ | Bb5 ~ ~ ~ D6 ~ G6 ~ | C#6 ~ ~ ~ E6 ~ A5 ~", 0.55)], fill="rock"),
    ], {"bass": "synth_bass"}, seed=854)
    # 15. Stage 7, the rogue AI: C minor, dark synths and a choir.
    s7 = Song("tower", 110, "C", "minor", [
        S(8, "Cm Ab Eb Bb Cm Ab Fm G", "half", "half", [P("choir", "pad", 4, 0.35, "pad"), P("arp", "arp8", 5, 0.25, "arp")],
          [L("lead", "G5 ~ ~ ~ Eb5 ~ C5 ~ | Ab5 ~ ~ ~ C6 ~ Eb6 ~ | G5 ~ Bb5 ~ Eb6 ~ D6 ~ | Bb5 ~ ~ ~ F5 ~ - - |"
                     "G5 ~ ~ ~ C6 ~ Eb6 ~ | Ab6 ~ G6 ~ Eb6 ~ C6 ~ | F6 ~ Eb6 ~ D6 ~ C6 ~ | B5 ~ ~ ~ D6 ~ G5 ~", 0.5)]),
    ], {"bass": "fm_bass"}, seed=855)
    # 16. Stage 8, beyond Earth: E dorian, wide and strange.
    s8 = Song("beyond", 100, "E", "dorian", [
        S(8, "Em7 A7 Cmaj7 D Em7 A7 Cmaj7 B7", "half", "half", [P("choir", "pad", 4, 0.35, "pad"), P("arp", "updown", 5, 0.3, "arp")],
          [L("flute", "B5 ~ ~ ~ E6 ~ D6 ~ | C#6 ~ ~ ~ A5 ~ ~ ~ | G5 ~ B5 ~ E6 ~ G6 ~ | F#6 ~ ~ ~ D6 ~ ~ ~ |"
                      "B5 ~ ~ ~ E6 ~ F#6 ~ | G6 ~ ~ ~ E6 ~ C#6 ~ | E6 ~ D6 ~ C6 ~ B5 ~ | D#6 ~ ~ ~ B5 ~ ~ ~", 0.5)]),
    ], {"bass": "fm_bass"}, seed=856)
    # 17. The last two bosses: B harmonic minor, everything at once.
    final = Song("final", 150, "B", "harmonic", [
        S(2, "Bm Bm", "hits", "whole", [P("choir", "pad", 4, 0.4, "pad")], crash=True),
        S(8, "Bm G Em F# Bm G C#m,F# Bm", "drive", "drive", [P("power", "chug16", 3, 0.3), P("choir", "pad", 4, 0.3, "pad")],
          [L("lead", "B5 ~ D6 ~ F#6 ~ E6 D6 | D6 ~ B5 ~ G5 ~ - - | E5 ~ G5 ~ B5 ~ C#6 D6 | A#5 ~ ~ ~ F#5 ~ - - |"
                     "B5 ~ D6 ~ F#6 ~ B6 ~ | A6 ~ G6 ~ F#6 ~ D6 ~ | E6 ~ D6 ~ C#6 ~ A#5 ~ | B5 ~ ~ ~ - - - -", 0.6)], fill="rock"),
    ], {"bass": "synth_bass"}, seed=857)
    # 18. Credits: the title's groove, home again.
    credits = Song("credits", 112, "D", "major", [
        S(8, "Dmaj7 Bm7 Em7 A7 Dmaj7 Bm7 Em7,A7 D", "funk", "funk", [P("epiano", "comp", 4, 0.4), P("brass", "stab2", 4, 0.3)],
          [L("sax", "F#5 ~ A5 ~ B5 ~ - C#6 | - - B5 A5 F#5 ~ - - | G5 ~ B5 ~ D6 ~ E6 ~ | C#6 ~ ~ ~ - - A5 ~ |"
                    "F#5 ~ A5 ~ D6 ~ E6 F#6 | E6 ~ D6 ~ B5 ~ - - | G5 ~ A5 ~ B5 ~ C#6 ~ | D6 ~ ~ ~ - - - -", 0.55)], fill="rock"),
    ], {"bass": "slap_bass"}, swing=0.1, seed=858)
    return [title, select, stage1, boss, chime, clear, over, scene, ending, s2, s3, s4, s5, s6, s7, s8, final, credits]


def main():
    for d in (OUT_A, OUT_B):
        d.mkdir(parents=True, exist_ok=True)
        for old in d.glob("*.wav"):
            old.unlink()
    fx = effects()
    for i, x in enumerate(fx, 1):
        write_wav(OUT_A / f"{i}.wav", norm(x, 0.85))
    total = 0.0
    for i, song in enumerate(songs(), 1):
        pcm = render_song(song)
        write_wav(OUT_B / f"{i}.wav", pcm)
        total += len(pcm) / RATE
        print(f"sound: track {i} {song.name}: {len(pcm) / RATE:.1f} s")
    print(f"sound: {len(fx)} effects, {len(songs())} tracks, {total:.0f} s of music")


if __name__ == "__main__":
    main()
