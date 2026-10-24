#!/usr/bin/env python3
"""
Seven Wonders Puzzles sound: every effect and tune is synthesized here.

    python3 games/sevenwonders/tools/sound.py      writes sound/samples/in_wav_a/<n>.wav (effects)
                                                   and in_wav_b/<n>.wav (music) for `make sound`

The tunes are original compositions written for this game, rendered with the
SDK's instruments (sound/tools/ng_synth.py) by its sequencer (ng_songs.py).
The order matches scenes/sw.h: SW_SFX_* is effect n (file n + 1), SW_MUS_* is
track n (file n + 1). Track 5 is the SDK's reserved eye-catcher slot.
"""

from pathlib import Path
import sys

import numpy as np

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / "sound/tools"))
import ng_synth as syn  # noqa: E402
from ng_songs import L, P, S, Song, render_song, write_wav  # noqa: E402

RATE = syn.RATE
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


def meow(f_lo=520, f_hi=900, seconds=0.45, bright=1.0):
    """A cartoon meow: a buzzy voice whose pitch rises and falls, through moving formants."""
    n = n_of(seconds)
    t = np.linspace(0, 1, n)
    contour = f_lo + (f_hi - f_lo) * np.sin(np.pi * np.clip(t * 1.25, 0, 1)) ** 0.8
    ph = 2 * np.pi * np.cumsum(contour) / RATE
    src = sum(np.sin(k * ph) / k for k in range(1, 12)) + syn.noise(n) * 0.04
    out = np.zeros(n)
    hop = 256
    for i in range(0, n, hop):
        seg = slice(i, min(n, i + hop))
        tt = t[i]
        f1 = 650 + 400 * np.sin(np.pi * tt)            # "mi" opening to "ow"
        f2 = 1700 + 500 * np.sin(np.pi * min(1, tt * 1.4)) - 700 * tt
        part = syn.bandpass(src, f1 * 0.7, f1 * 1.4)[seg] + 0.6 * bright * syn.bandpass(src, f2 * 0.8, f2 * 1.25)[seg]
        out[seg] = part
    return out * syn.env_adsr(n, 0.03, 0.1, 0.8, 0.15)


def effects():
    fx = []
    # 0 coin
    fx.append(seq(note("steel", "B5", 0.09), note("steel", "E6", 0.35)))
    # 1 move: a soft tick
    fx.append(chirp(1500, 1200, 0.03) * syn.decay(n_of(0.03), 0.01))
    # 2 start
    fx.append(seq(*[note("marimba", m, 0.08) for m in ("C5", "E5", "G5")], note("marimba", "C6", 0.4)))
    # 3 pick
    fx.append(mix(note("nylon", "G5", 0.15), 0.5 * note("nylon", "D6", 0.15)))
    # 4 swap: a short whoosh
    n = n_of(0.14)
    fx.append(syn.bandpass(syn.noise(n), 900, 3500) * np.sin(np.linspace(0, np.pi, n)) ** 2)
    # 5 nope
    fx.append(seq(chirp(330, 300, 0.08, "square") * syn.decay(n_of(0.08), 0.05),
                  chirp(240, 210, 0.14, "square") * syn.decay(n_of(0.14), 0.08), gap=0.02))
    # 6 match
    fx.append(mix(note("marimba", "C5", 0.35), note("marimba", "E5", 0.35, 0.8), note("marimba", "G5", 0.35, 0.7),
                  0.4 * np.concatenate([np.zeros(n_of(0.03)), note("steel", "C7", 0.2)])))
    # 7 cascade: higher, brighter
    fx.append(seq(*[note("marimba", m, 0.06) for m in ("G5", "C6", "E6")], note("steel", "G6", 0.35)))
    # 8 special made: rising shimmer
    fx.append(seq(*[note("steel", m, 0.05, 0.8) for m in ("C6", "E6", "G6", "B6", "D7")], note("steel", "G7", 0.4)))
    # 9 blast: a burst and a thump
    n = n_of(0.45)
    fx.append(mix(syn.lowpass(syn.noise(n), 3500) * syn.decay(n, 0.12), syn.kick(n, 1.0) * 0.8))
    # 10 crate: a wooden knock
    n = n_of(0.22)
    fx.append(mix(syn.bandpass(syn.noise(n), 300, 1400) * syn.decay(n, 0.04), syn.tom(n, 0.8, 130) * 0.6))
    # 11 ice: a glassy crack
    n = n_of(0.3)
    fx.append(mix(syn.bandpass(syn.noise(n), 3000, 9000) * syn.decay(n, 0.03), note("steel", "E7", 0.3, 0.6)))
    # 12 fire: a fwip
    fx.append(chirp(900, 280, 0.13) * syn.decay(n_of(0.13), 0.06))
    # 13 bounce: a boing
    n = n_of(0.18)
    t = np.arange(n) / RATE
    fx.append(np.sin(2 * np.pi * (380 + 60 * np.sin(2 * np.pi * 28 * t)) * t) * syn.decay(n, 0.07))
    # 14 pop
    fx.append(mix(chirp(500, 1700, 0.07) * syn.decay(n_of(0.07), 0.03), 0.3 * syn.noise(n_of(0.07)) * syn.decay(n_of(0.07), 0.01)))
    # 15 drop: a falling whistle
    fx.append(chirp(1400, 380, 0.45) * syn.env_adsr(n_of(0.45), 0.01, 0.05, 0.8, 0.1) * 0.7)
    # 16 win: a fanfare
    fx.append(seq(*[note("marimba", m, 0.11) for m in ("C5", "E5", "G5", "C6", "E6")],
                  mix(note("steel", "G6", 0.8), note("marimba", "C6", 0.8, 0.7), note("marimba", "E6", 0.8, 0.6))))
    # 17 lose: a soft fall
    fx.append(seq(note("flute", "G5", 0.25), note("flute", "E5", 0.25), note("flute", "C5", 0.25), note("flute", "B4", 0.7)))
    # 18 meow, 19 mew (a kitten)
    fx.append(meow())
    fx.append(meow(820, 1300, 0.28, 1.2))
    # 20 shuffle: a rattle
    clicks = np.zeros(n_of(0.5))
    rng = np.random.default_rng(7)
    for k in range(14):
        i = int(k * 0.034 * RATE + rng.integers(0, 200))
        c = note("marimba", ["C6", "E6", "G6", "A5"][k % 4], 0.05, 0.5)
        clicks[i:i + len(c)] += c[:len(clicks) - i]
    fx.append(clicks)
    # 21 tick
    n = n_of(0.05)
    fx.append(syn.bandpass(syn.noise(n), 2000, 6000) * syn.decay(n, 0.008))
    # 22 star
    fx.append(mix(note("steel", "E7", 0.5), 0.6 * np.concatenate([np.zeros(n_of(0.06)), note("steel", "B7", 0.4)])))
    # 23 rescue: a chime and a mew
    fx.append(seq(seq(*[note("steel", m, 0.06) for m in ("E6", "G6", "B6")]), meow(820, 1300, 0.26, 1.2) * 0.8, gap=0.02))
    return fx


def songs():
    # 1. Title: C major, a bright marimba tune over light drums.
    title = Song("title", 118, "C", "major", [
        S(2, "C G", "intro", "half", [P("strings", "pad", 4, 0.4, "pad")], crash=True),
        S(8, "C Am F G C Am Dm,G C", "half", "rock8", [P("nylon", "strum", 4, 0.45), P("strings", "pad", 4, 0.35, "pad")],
          [L("marimba", "E5 ~ G5 ~ C6 ~ B5 A5 | G5 ~ E5 ~ C5 ~ - - | F5 ~ A5 ~ C6 ~ A5 F5 | G5 ~ ~ ~ D5 ~ - - |"
                        "E5 ~ G5 ~ C6 ~ D6 E6 | A5 ~ ~ ~ G5 ~ E5 ~ | F5 ~ E5 D5 G5 ~ B4 ~ | C5 ~ ~ ~ - - - -", 0.65)],
          fill="rock"),
        S(8, "F G Em Am F G C C", "funk", "octave", [P("epiano", "comp", 4, 0.45), P("arp", "arp8", 5, 0.25, "arp")],
          [L("flute", "A5 ~ ~ ~ C6 ~ A5 ~ | B5 ~ ~ ~ G5 ~ D5 ~ | E5 ~ G5 ~ B5 ~ G5 ~ | A5 ~ ~ ~ ~ ~ E5 ~ |"
                      "F5 ~ A5 ~ C6 ~ F6 ~ | E6 ~ D6 ~ B5 ~ G5 ~ | C6 ~ ~ ~ ~ ~ G5 ~ | C6 ~ ~ ~ - - - -", 0.55)],
          fill="toms"),
    ], {"bass": "pick_bass"}, seed=701)
    # 2. Match: F major, a bouncy steel-drum groove.
    match = Song("match", 124, "F", "major", [
        S(8, "F Dm Bb C F Dm Gm,C F", "funk", "funk", [P("epiano", "stab", 4, 0.45), P("marimba", "skank", 5, 0.35)],
          [L("steel", "A5 - C6 - A5 G5 F5 - | D5 ~ F5 ~ A5 ~ - - | Bb5 - D6 - C6 Bb5 A5 - | G5 ~ ~ ~ C5 ~ - - |"
                      "A5 - C6 - F6 E6 D6 - | C6 ~ A5 ~ F5 ~ D5 ~ | G5 ~ Bb5 ~ A5 ~ G5 ~ | F5 ~ ~ ~ - - - -", 0.6)], fill="rock"),
        S(8, "Bb C Am Dm Bb C F F", "funk", "funk2", [P("nylon", "comp", 4, 0.4), P("strings", "pad", 4, 0.3, "pad")],
          [L("marimba", "D6 ~ C6 ~ Bb5 ~ A5 ~ | G5 ~ A5 ~ Bb5 ~ C6 ~ | E6 ~ ~ ~ C6 ~ A5 ~ | D6 ~ ~ ~ A5 ~ F5 ~ |"
                        "D6 ~ C6 ~ Bb5 ~ D6 ~ | E6 ~ F6 ~ G6 ~ E6 ~ | F6 ~ ~ ~ C6 ~ A5 ~ | F5 ~ ~ ~ - - - -", 0.6)], fill="toms"),
    ], {"bass": "synth_bass"}, swing=0.12, seed=702)
    # 3. Bubbles: G major, an airy flute over arpeggios.
    bubble = Song("bubble", 108, "G", "major", [
        S(8, "G Em C D G Em Am,D G", "half", "half", [P("arp", "updown", 5, 0.3, "arp"), P("strings", "pad", 4, 0.35, "pad")],
          [L("flute", "B5 ~ ~ ~ D6 ~ B5 ~ | G5 ~ ~ ~ E5 ~ ~ ~ | E5 ~ G5 ~ C6 ~ E6 ~ | D6 ~ ~ ~ ~ ~ A5 ~ |"
                      "B5 ~ ~ ~ D6 ~ G6 ~ | E6 ~ ~ ~ B5 ~ G5 ~ | A5 ~ C6 ~ B5 ~ A5 ~ | G5 ~ ~ ~ - - - -", 0.55)], fill="toms"),
        S(8, "C D Bm Em C D G G", "half", "octave", [P("nylon", "strum", 4, 0.4), P("marimba", "arp8", 5, 0.3, "arp")],
          [L("steel", "E6 ~ D6 ~ C6 ~ B5 ~ | A5 ~ B5 ~ C6 ~ D6 ~ | F#6 ~ ~ ~ D6 ~ B5 ~ | G6 ~ ~ ~ E6 ~ B5 ~ |"
                      "E6 ~ D6 ~ C6 ~ E6 ~ | F#6 ~ G6 ~ A6 ~ F#6 ~ | G6 ~ ~ ~ D6 ~ B5 ~ | G5 ~ ~ ~ - - - -", 0.55)]),
    ], {"bass": "fm_bass"}, seed=703)
    # 4. The map: D major, a gentle walk.
    world = Song("map", 96, "D", "major", [
        S(8, "D Bm G A D Bm Em,A D", "half", "half", [P("nylon", "strum", 4, 0.45), P("strings", "pad", 4, 0.35, "pad")],
          [L("flute", "F#5 ~ A5 ~ D6 ~ ~ ~ | B5 ~ A5 ~ F#5 ~ ~ ~ | G5 ~ B5 ~ D6 ~ B5 ~ | A5 ~ ~ ~ E5 ~ ~ ~ |"
                      "F#5 ~ A5 ~ D6 ~ E6 ~ | F#6 ~ ~ ~ D6 ~ B5 ~ | G5 ~ F#5 ~ E5 ~ A5 ~ | D5 ~ ~ ~ - - - -", 0.5)]),
    ], {"bass": "pick_bass"}, seed=704)
    # 5. The SDK's eye-catcher slot: a short chime.
    chime = Song("chime", 120, "C", "major", [
        S(2, "C G,C", None, None, [P("strings", "pad", 4, 0.4, "pad")],
          [L("steel", "C6 E6 G6 C7 ~ ~ ~ ~ | B6 ~ G6 ~ C7 ~ ~ ~", 0.6)], crash=False),
    ], {"bass": "pick_bass"}, loop=False, seed=705)
    # 6. A level won: a happy loop.
    won = Song("won", 132, "C", "major", [
        S(4, "C F G C", "funk", "octave", [P("marimba", "stab", 5, 0.45), P("strings", "pad", 4, 0.3, "pad")],
          [L("steel", "C6 E6 G6 ~ E6 ~ C6 ~ | F6 ~ A6 ~ G6 ~ F6 ~ | E6 ~ D6 ~ B5 ~ G5 ~ | C6 ~ ~ ~ - - - -", 0.6, harm=-2)]),
    ], {"bass": "pick_bass"}, seed=706)
    # 7. Lost, continue, game over: A minor, soft.
    lost = Song("lost", 84, "A", "minor", [
        S(8, "Am F C G Am F Dm,E Am", "half", "half", [P("epiano", "comp", 4, 0.4), P("strings", "pad", 3, 0.35, "pad")],
          [L("flute", "E5 ~ ~ ~ C5 ~ A4 ~ | C5 ~ ~ ~ A4 ~ F4 ~ | G4 ~ C5 ~ E5 ~ ~ ~ | D5 ~ ~ ~ B4 ~ ~ ~ |"
                      "E5 ~ ~ ~ A5 ~ G5 ~ | F5 ~ ~ ~ C5 ~ A4 ~ | D5 ~ C5 ~ B4 ~ G#4 ~ | A4 ~ ~ ~ - - - -", 0.45)]),
    ], {"bass": "fm_bass"}, seed=707)
    # 8. The finale: C major, everyone together.
    finale = Song("finale", 112, "C", "major", [
        S(8, "C G Am Em F C Dm G", "half", "rock8", [P("strings", "pad", 4, 0.45, "pad"), P("nylon", "strum", 4, 0.4)],
          [L("marimba", "G5 ~ C6 ~ E6 ~ D6 C6 | B5 ~ G5 ~ D6 ~ - - | C6 ~ E6 ~ A6 ~ G6 E6 | G6 ~ ~ ~ E6 ~ - - |"
                        "F6 ~ E6 ~ D6 ~ C6 ~ | E6 ~ ~ ~ G6 ~ C6 ~ | D6 ~ E6 ~ F6 ~ D6 ~ | G6 ~ ~ ~ - - - -", 0.6, harm=-2)],
          fill="rock"),
        S(8, "F G Em Am Dm G C C", "funk", "octave", [P("epiano", "comp", 4, 0.4), P("arp", "arp16", 5, 0.25, "arp")],
          [L("steel", "A6 ~ G6 ~ F6 ~ E6 ~ | D6 ~ E6 ~ F6 ~ G6 ~ | B6 ~ ~ ~ G6 ~ E6 ~ | C7 ~ ~ ~ A6 ~ E6 ~ |"
                      "F6 ~ A6 ~ D7 ~ C7 ~ | B6 ~ G6 ~ D6 ~ B5 ~ | C7 ~ ~ ~ G6 ~ E6 ~ | C6 ~ ~ ~ - - - -", 0.55)], fill="toms"),
    ], {"bass": "pick_bass"}, seed=708)
    return [title, match, bubble, world, chime, won, lost, finale]


def main():
    for d in (OUT_A, OUT_B):
        d.mkdir(parents=True, exist_ok=True)
        for old in d.glob("*.wav"):
            old.unlink()
    for i, x in enumerate(effects(), 1):
        write_wav(OUT_A / f"{i}.wav", norm(x, 0.85))
    total = 0.0
    for i, song in enumerate(songs(), 1):
        pcm = render_song(song)
        write_wav(OUT_B / f"{i}.wav", pcm)
        total += len(pcm) / RATE
        print(f"sound: track {i} {song.name}: {len(pcm) / RATE:.1f} s")
    print(f"sound: {len(effects())} effects, 8 tracks, {total:.0f} s of music")


if __name__ == "__main__":
    main()
