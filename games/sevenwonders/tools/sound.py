#!/usr/bin/env python3
"""
Seven Wonders Puzzles sound: every effect and tune is synthesized here.

    python3 games/sevenwonders/tools/sound.py      writes sound/samples/in_wav_a/<n>.wav (effects)
                                                   and in_wav_b/<n>.wav (music) for `make sound`

The tunes are original compositions written for this game, rendered with the
SDK's instruments (sound/tools/ng_synth.py) by its sequencer (ng_songs.py),
gently mixed and well below full scale; each effect has its own level, the
frequent small ones quiet, so nothing piles up into noise.
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


def soft_tick(f, seconds, tau):
    """A tiny sine pip: no noise, so nothing hisses."""
    n = n_of(seconds)
    t = np.arange(n) / RATE
    return np.sin(2 * np.pi * f * t) * syn.decay(n, tau)


def effects():
    """Every effect and how loud it sits (its peak): the small, frequent ones
    whisper, the rare rewards ring out. Tonal sounds only, no noise bursts,
    and every pitched note is a D, E, G or A: the notes all the tunes' keys
    (C, G, D, F major, A minor) share, so an effect never clashes with the
    music under it. Bells are the music box, never the clangy steel drum."""
    fx = []
    box = "music_box"
    # 0 coin
    fx.append((seq(note(box, "E6", 0.08), note(box, "A6", 0.3)), 0.4))
    # 1 move: a soft pip (A6)
    fx.append((soft_tick(1760, 0.035, 0.008), 0.16))
    # 2 start: a pentatonic run up
    fx.append((seq(*[note("marimba", m, 0.08) for m in ("D5", "E5", "G5", "A5")], note("marimba", "D6", 0.4)), 0.5))
    # 3 pick, a square filled: a round wooden pop
    fx.append((mix(note("marimba", "G5", 0.12), 0.35 * note("marimba", "D6", 0.1)), 0.3))
    # 4 swap: a soft rising boop
    fx.append((chirp(520, 820, 0.09) * np.sin(np.linspace(0, np.pi, n_of(0.09))) ** 1.5, 0.26))
    # 5 nope: a gentle "uh-oh" (E5, then D5 falling to A4)
    fx.append((seq(chirp(659, 640, 0.09) * syn.decay(n_of(0.09), 0.06),
                   chirp(587, 440, 0.16) * syn.decay(n_of(0.16), 0.09), gap=0.03), 0.34))
    # 6 match: an open G-D-A chord and a bell
    fx.append((mix(note("marimba", "G5", 0.3), note("marimba", "D6", 0.3, 0.75),
                   0.45 * np.concatenate([np.zeros(n_of(0.03)), note(box, "A6", 0.25)])), 0.4))
    # 7 cascade: higher, brighter
    fx.append((seq(*[note("marimba", m, 0.06) for m in ("E5", "G5", "A5")], note(box, "D6", 0.3)), 0.42))
    # 8 special made: a rising music-box sparkle
    fx.append((seq(*[note(box, m, 0.045, 0.8) for m in ("D6", "E6", "G6", "A6", "D7")], note(box, "E7", 0.35)), 0.4))
    # 9 blast: a round "pomf" and a low marimba D
    n = n_of(0.3)
    fx.append((mix(chirp(300, 90, 0.3) * syn.decay(n, 0.09), 0.6 * note("marimba", "D4", 0.3)), 0.46))
    # 10 crate: a wooden tok
    fx.append((mix(note("marimba", "A4", 0.16), 0.5 * note("marimba", "E5", 0.12)), 0.38))
    # 11 ice: a glassy tinkle
    fx.append((seq(note(box, "E7", 0.06, 0.8), note(box, "A7", 0.22, 0.6)), 0.28))
    # 12 fire: a soft fwip
    fx.append((chirp(880, 330, 0.12) * syn.decay(n_of(0.12), 0.05), 0.28))
    # 13 bounce: a little boing
    n = n_of(0.16)
    t = np.arange(n) / RATE
    fx.append((np.sin(2 * np.pi * (440 + 50 * np.sin(2 * np.pi * 26 * t)) * t) * syn.decay(n, 0.06), 0.22))
    # 14 pop: a bubble pop
    fx.append((chirp(440, 1480, 0.05) * syn.decay(n_of(0.05), 0.02), 0.3))
    # 15 drop: a falling whistle
    fx.append((chirp(1320, 440, 0.38) * syn.env_adsr(n_of(0.38), 0.01, 0.05, 0.8, 0.1), 0.22))
    # 16 win: a music-box jingle, up the pentatonic to a ringing chord (it plays
    # alone: the result screen silences the tune first)
    fx.append((seq(*[note(box, m, 0.09) for m in ("D6", "E6", "G6", "A6")],
                   mix(note(box, "D7", 0.8), 0.7 * note(box, "A6", 0.8), 0.6 * note(box, "G6", 0.8),
                       0.5 * note("marimba", "D5", 0.6))), 0.45))
    # 17 lose: a soft marimba fall, A G E D
    fx.append((seq(note("marimba", "A5", 0.22), note("marimba", "G5", 0.22), note("marimba", "E5", 0.22),
                   note("marimba", "D5", 0.6)), 0.4))
    # 18 meow, 19 mew (a kitten)
    fx.append((meow(), 0.3))
    fx.append((meow(820, 1300, 0.26, 1.2), 0.28))
    # 20 shuffle: a marimba rattle
    clicks = np.zeros(n_of(0.45))
    rng = np.random.default_rng(7)
    for k in range(12):
        i = int(k * 0.034 * RATE + rng.integers(0, 200))
        c = note("marimba", ["D6", "E6", "G6", "A5"][k % 4], 0.05, 0.5)
        clicks[i:i + len(c)] += c[:len(clicks) - i]
    fx.append((clicks, 0.3))
    # 21 tick: the quietest pip (D7)
    fx.append((soft_tick(2349, 0.025, 0.005), 0.12))
    # 22 star
    fx.append((mix(note(box, "A6", 0.45), 0.6 * np.concatenate([np.zeros(n_of(0.06)), note(box, "E7", 0.35)])), 0.38))
    # 23 rescue: a chime and a mew
    fx.append((seq(seq(*[note(box, m, 0.06) for m in ("E6", "G6", "A6")]), meow(820, 1300, 0.26, 1.2) * 0.8, gap=0.02), 0.38))
    return fx


def songs():
    """Eight tunes in a cute Japanese pop style: pentatonic (yonanuki) melodies on
    marimba, music-box bells and koto over the 'royal road' progression
    (IV V iii vi), light drums or none, mixed gently and well below full scale."""
    soft = {"bass": "pick_bass", "mix": {"drums": -7, "bass": -3, "comp": -2, "pad": -2}}
    calm = {"bass": "pick_bass", "mix": {"bass": -4, "comp": -2, "pad": -1}}
    # 1. Title: C major, bouncy and bright.
    title = Song("title", 112, "C", "major", [
        S(2, "F G", "intro", "whole", [P("strings", "pad", 4, 0.3, "pad")], crash=False, drum_gain=0.6),
        S(8, "F G Em Am F G C C", "half", "half", [P("koto", "strum", 4, 0.3), P("strings", "pad", 4, 0.25, "pad")],
          [L("marimba", "C6 ~ A5 ~ G5 ~ A5 ~ | G5 ~ E5 ~ D5 ~ E5 ~ | E5 ~ G5 ~ A5 ~ G5 E5 | A5 ~ ~ ~ - - E5 G5 |"
                        "A5 ~ C6 ~ D6 ~ C6 A5 | G5 ~ A5 ~ G5 ~ E5 D5 | E5 ~ D5 ~ C5 ~ D5 ~ | C5 ~ ~ ~ - - - -", 0.6)],
          crash=False, drum_gain=0.6, bass_gain=0.8),
        S(8, "Am F G C Am F G C", "half", "half", [P("marimba", "arp8", 5, 0.22, "arp"), P("strings", "pad", 4, 0.25, "pad")],
          [L("music_box", "E6 ~ ~ ~ D6 ~ C6 ~ | A5 ~ ~ ~ C6 ~ ~ ~ | D6 ~ E6 ~ D6 ~ C6 ~ | G5 ~ ~ ~ - - G5 A5 |"
                      "C6 ~ ~ ~ D6 ~ E6 ~ | A6 ~ ~ ~ G6 ~ E6 ~ | D6 ~ E6 ~ G6 ~ D6 ~ | C6 ~ ~ ~ - - - -", 0.55)],
          crash=False, drum_gain=0.6, bass_gain=0.8),
    ], soft, seed=711, master=0.8, gentle=True)
    # 2. Matching: G major, playful, a little swing.
    match = Song("match", 120, "G", "major", [
        S(8, "G Em C D G Em C,D G", "half", "half", [P("marimba", "skank", 5, 0.22), P("strings", "pad", 4, 0.22, "pad")],
          [L("music_box", "B5 ~ D6 ~ E6 ~ D6 B5 | G5 ~ A5 ~ B5 ~ ~ ~ | E6 ~ D6 ~ B5 ~ A5 ~ | A5 ~ ~ ~ - - D5 E5 |"
                      "G5 ~ A5 ~ B5 ~ D6 ~ | E6 ~ D6 ~ B5 ~ G5 ~ | A5 ~ B5 ~ A5 ~ G5 E5 | G5 ~ ~ ~ - - - -", 0.55)],
          crash=False, drum_gain=0.6, bass_gain=0.8),
        S(8, "C D Bm Em C D G G", "half", "half", [P("koto", "arp8", 4, 0.22, "arp"), P("strings", "pad", 4, 0.22, "pad")],
          [L("marimba", "E6 ~ G6 ~ E6 ~ D6 ~ | D6 ~ ~ ~ A5 ~ ~ ~ | B5 ~ D6 ~ F#6 ~ D6 ~ | E6 ~ ~ ~ B5 ~ ~ ~ |"
                        "C6 ~ E6 ~ G6 ~ E6 ~ | D6 ~ F#6 ~ A6 ~ F#6 ~ | G6 ~ D6 ~ B5 ~ D6 ~ | G6 ~ ~ ~ - - - -", 0.6)],
          crash=False, drum_gain=0.6, bass_gain=0.8),
    ], soft, swing=0.1, seed=712, master=0.8, gentle=True)
    # 3. Bubbles: D major, a light marimba tune over a marimba up-and-down.
    bubble = Song("bubble", 104, "D", "major", [
        S(8, "D Bm G A D Bm G,A D", "intro", "half", [P("marimba", "updown", 5, 0.2, "arp"), P("strings", "pad", 4, 0.25, "pad")],
          [L("marimba", "F#5 ~ A5 ~ B5 ~ A5 ~ | F#5 ~ ~ ~ D5 ~ ~ ~ | D5 ~ E5 ~ G5 ~ B5 ~ | A5 ~ ~ ~ - - - - |"
                      "D6 ~ ~ ~ B5 ~ A5 ~ | B5 ~ D6 ~ F#6 ~ ~ ~ | E6 ~ D6 ~ B5 ~ E6 ~ | D6 ~ ~ ~ - - - -", 0.55)],
          crash=False, drum_gain=0.55, bass_gain=0.75),
        S(8, "G A F#m Bm G A D D", "intro", "half", [P("koto", "arp8", 4, 0.2, "arp"), P("strings", "pad", 4, 0.25, "pad")],
          [L("music_box", "B5 ~ D6 ~ G6 ~ F#6 ~ | E6 ~ ~ ~ C#6 ~ A5 ~ | A5 ~ C#6 ~ F#6 ~ E6 ~ | D6 ~ ~ ~ B5 ~ ~ ~ |"
                      "B5 ~ D6 ~ E6 ~ G6 ~ | A6 ~ ~ ~ E6 ~ C#6 ~ | D6 ~ F#6 ~ A6 ~ F#6 ~ | D6 ~ ~ ~ - - - -", 0.5)],
          crash=False, drum_gain=0.55, bass_gain=0.75),
    ], soft, seed=713, master=0.8, gentle=True)
    # 4. The map, and the pictures: F major, calm music-box bells over a koto, no drums.
    world = Song("map", 88, "F", "major", [
        S(8, "F C Dm Am Bb F Gm,C F", None, "whole", [P("koto", "arp8", 4, 0.22, "arp"), P("strings", "pad", 4, 0.22, "pad")],
          [L("music_box", "A5 ~ C6 ~ D6 ~ C6 ~ | G5 ~ ~ ~ ~ ~ - - | F5 ~ A5 ~ D6 ~ C6 ~ | A5 ~ ~ ~ ~ ~ - - |"
                      "D6 ~ C6 ~ A5 ~ G5 ~ | A5 ~ C6 ~ F6 ~ ~ ~ | G5 ~ A5 ~ C6 ~ G5 ~ | F5 ~ ~ ~ ~ ~ - - |", 0.5)],
          crash=False, bass_gain=0.7),
        S(8, "Dm Am Bb C Dm Am Bb,C F", None, "whole", [P("marimba", "arp8", 5, 0.18, "arp"), P("strings", "pad", 4, 0.22, "pad")],
          [L("koto", "F5 ~ ~ ~ A5 ~ D6 ~ | C6 ~ ~ ~ A5 ~ ~ ~ | D6 ~ C6 ~ A5 ~ F5 ~ | G5 ~ ~ ~ ~ ~ - - |"
                     "A5 ~ ~ ~ D6 ~ F6 ~ | E6 ~ ~ ~ C6 ~ A5 ~ | D6 ~ F6 ~ E6 ~ C6 ~ | F6 ~ ~ ~ ~ ~ - - |", 0.55)],
          crash=False, bass_gain=0.7),
    ], calm, seed=714, master=0.75, gentle=True)
    # 5. The SDK's eye-catcher slot: a short chime.
    chime = Song("chime", 120, "C", "major", [
        S(2, "C G,C", None, None, [P("strings", "pad", 4, 0.35, "pad")],
          [L("music_box", "C6 E6 G6 C7 ~ ~ ~ ~ | B6 ~ G6 ~ C7 ~ ~ ~", 0.55)], crash=False),
    ], calm, loop=False, seed=715, master=0.8, gentle=True)
    # 6. A level won: a happy little loop (after the fanfare), no drums.
    won = Song("won", 126, "C", "major", [
        S(4, "F G C C", None, "half", [P("marimba", "stab", 5, 0.3), P("strings", "pad", 4, 0.22, "pad")],
          [L("music_box", "A5 C6 D6 ~ C6 ~ A5 ~ | G5 ~ D6 ~ C6 ~ A5 ~ | G5 ~ E6 ~ D6 ~ C6 ~ | C6 ~ ~ ~ - - - -", 0.55)],
          crash=False, drum_gain=0.55, bass_gain=0.75),
    ], soft, seed=716, master=0.75, gentle=True)
    # 7. Lost, continue, game over: A minor, a soft koto, no drums.
    lost = Song("lost", 80, "A", "minor", [
        S(8, "Am F C G Am F Dm,E Am", None, "whole", [P("koto", "arp8", 4, 0.2, "arp"), P("strings", "pad", 3, 0.25, "pad")],
          [L("koto", "E5 ~ ~ ~ C5 ~ A4 ~ | C5 ~ ~ ~ A4 ~ F4 ~ | G4 ~ C5 ~ E5 ~ ~ ~ | D5 ~ ~ ~ B4 ~ ~ ~ |"
                      "E5 ~ ~ ~ A5 ~ G5 ~ | F5 ~ ~ ~ C5 ~ A4 ~ | D5 ~ C5 ~ B4 ~ G#4 ~ | A4 ~ ~ ~ - - - -", 0.45)],
          crash=False, bass_gain=0.7),
    ], calm, seed=717, master=0.7, gentle=True)
    # 8. The finale: C major, the title's tunes together, a little grander.
    finale = Song("finale", 112, "C", "major", [
        S(8, "F G Em Am F G C C", "half", "half", [P("koto", "strum", 4, 0.28), P("strings", "pad", 4, 0.3, "pad")],
          [L("marimba", "C6 ~ A5 ~ G5 ~ A5 ~ | G5 ~ E5 ~ D5 ~ E5 ~ | E5 ~ G5 ~ A5 ~ G5 E5 | A5 ~ ~ ~ - - E5 G5 |"
                        "A5 ~ C6 ~ D6 ~ C6 A5 | G5 ~ A5 ~ G5 ~ E5 D5 | E5 ~ D5 ~ C5 ~ D5 ~ | C5 ~ ~ ~ - - - -", 0.6, harm=-2)],
          crash=False, drum_gain=0.65, bass_gain=0.8),
        S(8, "Am F G C Am F G C", "half", "half", [P("marimba", "arp8", 5, 0.22, "arp"), P("strings", "pad", 4, 0.3, "pad")],
          [L("music_box", "E6 ~ ~ ~ D6 ~ C6 ~ | A5 ~ ~ ~ C6 ~ ~ ~ | D6 ~ E6 ~ D6 ~ C6 ~ | G5 ~ ~ ~ - - G5 A5 |"
                      "C6 ~ ~ ~ D6 ~ E6 ~ | A6 ~ ~ ~ G6 ~ E6 ~ | D6 ~ E6 ~ G6 ~ D6 ~ | C6 ~ ~ ~ - - - -", 0.55, harm=-2)],
          crash=False, drum_gain=0.65, bass_gain=0.8),
    ], soft, seed=718, master=0.85, gentle=True)
    return [title, match, bubble, world, chime, won, lost, finale]


def main():
    for d in (OUT_A, OUT_B):
        d.mkdir(parents=True, exist_ok=True)
        for old in d.glob("*.wav"):
            old.unlink()
    for i, (x, peak) in enumerate(effects(), 1):
        write_wav(OUT_A / f"{i}.wav", norm(x, peak))
    total = 0.0
    for i, song in enumerate(songs(), 1):
        pcm = render_song(song)
        write_wav(OUT_B / f"{i}.wav", pcm)
        total += len(pcm) / RATE
        print(f"sound: track {i} {song.name}: {len(pcm) / RATE:.1f} s")
    print(f"sound: {len(effects())} effects, 8 tracks, {total:.0f} s of music")


if __name__ == "__main__":
    main()
