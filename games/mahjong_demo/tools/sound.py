#!/usr/bin/env python3
"""
Mahjong Demo sound: every effect and tune is synthesized here.

    python3 games/mahjong_demo/tools/sound.py   writes sound/samples/in_wav_a/<n>.wav (effects)
                                                and in_wav_b/<n>.wav (music) for `make sound`

The tunes are short original loops rendered with the SDK's instruments
(sound/tools/ng_synth.py) by its sequencer (ng_songs.py). The order matches
scenes/mj.h: MJ_SFX_* is effect n (file n + 1), MJ_MUS_* is track n (file
n + 1, from MJ_MUS_RESULT on n + 2: 5.wav is the eye-catcher's).
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


def clack(seconds=0.09, lo=1800, hi=6500, body=900):
    """Two hard tiles meeting: a bright click over a short woody knock."""
    n = n_of(seconds)
    click = syn.bandpass(syn.noise(n), lo, hi) * syn.decay(n, 0.006)
    knock = np.sin(2 * np.pi * body * np.arange(n) / RATE) * syn.decay(n, 0.02) * 0.6
    return mix(click, knock)


def effects():
    fx = []
    fx.append(seq(note("steel", "E6", 0.07), note("steel", "B6", 0.3)))              # 0 coin
    fx.append(seq(*[note("marimba", m, 0.07) for m in ("C5", "E5", "G5")], note("marimba", "C6", 0.35)))  # 1 start
    fx.append(clack(0.04, 2500, 7000, 1400) * 0.6)                                     # 2 move
    fx.append(mix(clack(), 0.5 * np.concatenate([np.zeros(n_of(0.03)), clack(0.07, 1500, 5000, 700)])))   # 3 discard
    fx.append(clack(0.06, 1200, 4000, 600) * 0.7)                                      # 4 draw
    fx.append(seq(note("marimba", "G5", 0.08), note("marimba", "D6", 0.25)))           # 5 claim
    fx.append(seq(clack(), clack(), note("marimba", "G6", 0.3), gap=0.04))            # 6 kong
    fx.append(seq(*[note("marimba", m, 0.1) for m in ("C5", "E5", "G5", "C6", "E6")],
                  mix(note("steel", "G6", 0.9), note("marimba", "C6", 0.9, 0.7))))     # 7 win
    fx.append(seq(note("flute", "E5", 0.25), note("flute", "C5", 0.5)))               # 8 exhausted wall
    fx.append(seq(note("marimba", "C4", 0.08, 0.8), note("marimba", "B3", 0.18, 0.8)))  # 9 not allowed
    fx.append(mix(clack(0.1), 0.6 * note("steel", "A6", 0.25)))                        # 10 pair removed
    shuffle = np.zeros(n_of(0.6))
    rng = np.random.default_rng(11)
    for k in range(16):
        i = int(k * 0.035 * RATE + rng.integers(0, 300))
        c = clack(0.05, 1500, 6000, int(rng.integers(600, 1200))) * rng.uniform(0.4, 1.0)
        shuffle[i:i + len(c)] += c[:len(shuffle) - i]
    fx.append(shuffle)                                                                 # 11 shuffle
    fx.append(seq(note("steel", "E6", 0.06), note("steel", "A6", 0.2)))               # 12 hint
    fx.append(seq(*[note("marimba", m, 0.09) for m in ("G5", "C6", "E6", "G6")],
                  mix(note("steel", "C7", 0.8), note("marimba", "E6", 0.8, 0.6))))     # 13 cleared
    fx.append(seq(note("marimba", "E5", 0.06), note("marimba", "C5", 0.12)))          # 14 undo
    fx.append(clack(0.05, 3000, 8000, 1600) * 0.5)                                     # 15 select
    return fx


def songs():
    table = Song("table", 92, "G", "pent_major", [
        S(8, "G Em C D G Em Am,D G", "half", "half", [P("nylon", "strum", 4, 0.4), P("strings", "pad", 4, 0.3, "pad")],
          [L("flute", "B5 ~ D6 ~ E6 ~ D6 B5 | A5 ~ ~ ~ G5 ~ E5 ~ | E5 ~ G5 ~ A5 ~ B5 ~ | D6 ~ ~ ~ ~ ~ B5 ~ |"
                      "B5 ~ D6 ~ G6 ~ E6 D6 | E6 ~ ~ ~ D6 ~ B5 ~ | A5 ~ B5 ~ A5 ~ E5 ~ | G5 ~ ~ ~ - - - -", 0.45)]),
    ], {"bass": "pick_bass"}, seed=785)
    menu = Song("menu", 104, "D", "pent_major", [
        S(4, "D Bm G A", "funk", "octave", [P("marimba", "stab", 5, 0.4), P("strings", "pad", 4, 0.3, "pad")],
          [L("steel", "F#6 ~ A6 ~ B6 ~ A6 F#6 | E6 ~ ~ ~ D6 ~ B5 ~ | D6 ~ E6 ~ F#6 ~ A6 ~ | E6 ~ ~ ~ - - - -", 0.5)]),
    ], {"bass": "pick_bass"}, seed=786)
    sol = Song("solitaire", 84, "C", "pent_major", [
        S(8, "C Am F G C Am Dm,G C", "half", "half", [P("epiano", "comp", 4, 0.35), P("arp", "arp8", 5, 0.2, "arp")],
          [L("marimba", "E5 ~ G5 ~ A5 ~ ~ ~ | G5 ~ E5 ~ D5 ~ ~ ~ | C5 ~ D5 ~ E5 ~ G5 ~ | A5 ~ ~ ~ G5 ~ ~ ~ |"
                        "E5 ~ G5 ~ C6 ~ A5 ~ | G5 ~ ~ ~ E5 ~ D5 ~ | D5 ~ E5 ~ G5 ~ D5 ~ | C5 ~ ~ ~ - - - -", 0.45)]),
    ], {"bass": "fm_bass"}, seed=787)
    quiet = Song("quiet", 80, "A", "minor", [
        S(4, "Am F C G", "half", "half", [P("strings", "pad", 4, 0.35, "pad")],
          [L("flute", "E5 ~ ~ ~ C5 ~ A4 ~ | F5 ~ ~ ~ C5 ~ ~ ~ | G5 ~ E5 ~ C5 ~ ~ ~ | D5 ~ ~ ~ B4 ~ ~ ~", 0.4)]),
    ], {"bass": "fm_bass"}, seed=788)
    chime = Song("chime", 120, "C", "major", [
        S(2, "C G,C", None, None, [P("strings", "pad", 4, 0.4, "pad")],
          [L("steel", "C6 E6 G6 C7 ~ ~ ~ ~ | B6 ~ G6 ~ C7 ~ ~ ~", 0.6)], crash=False),
    ], {"bass": "pick_bass"}, loop=False, seed=789)
    result = Song("result", 120, "F", "major", [
        S(4, "F Bb C F", "funk", "octave", [P("marimba", "stab", 5, 0.4), P("epiano", "comp", 4, 0.3)],
          [L("steel", "A5 C6 F6 ~ C6 ~ A5 ~ | Bb5 ~ D6 ~ C6 ~ Bb5 ~ | A5 ~ G5 ~ E5 ~ C5 ~ | F5 ~ ~ ~ - - - -", 0.55)]),
    ], {"bass": "pick_bass"}, seed=790)
    return [menu, table, sol, quiet, chime, result]


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
