#!/usr/bin/env python3
"""
Maiya & Luna sound: every effect and tune is synthesized here.

    python3 games/maiyaluna/tools/sound.py     writes sound/samples/in_wav_a/<n>.wav (effects)
                                               and in_wav_b/<n>.wav (music) for `make sound`

The tunes are original compositions for this game, rendered with the SDK's
instruments (sound/tools/ng_synth.py) by its sequencer (ng_songs.py): flute,
nylon strings, marimba, soft bells and string pads, as the valley sounds.
The order matches scenes/ml.h (ML_SFX_* is effect n, file n + 1) and the
world's music list (tools/worldlib.py music_list(): ML_MUS_* n is file n + 1,
from n = 4 on file n + 2, because ADPCM-B track 5 is the SDK's eye-catcher).
"""

from pathlib import Path
import sys

import numpy as np

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / "sound/tools"))
sys.path.insert(0, str(GAME / "tools"))
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


def chirp(f0, f1, seconds):
    n = n_of(seconds)
    f = np.geomspace(f0, f1, n)
    return np.sin(2 * np.pi * np.cumsum(f) / RATE)


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


def delay(x, seconds):
    return np.concatenate([np.zeros(n_of(seconds)), x])


def note(inst, name, seconds, vel=1.0):
    from ng_songs import note_midi
    return syn.INSTRUMENTS[inst](syn.hz(note_midi(name)), n_of(seconds), vel)


def knock(seconds=0.2, lo=200, hi=1200, tone=120):
    n = n_of(seconds)
    return mix(syn.bandpass(syn.noise(n), lo, hi) * syn.decay(n, 0.04), syn.tom(n, 0.7, tone) * 0.6)


def effects():
    fx = []
    # 0 coin
    fx.append(seq(note("steel", "B5", 0.08), note("steel", "E6", 0.3)))
    # 1 start: a rising flute phrase
    fx.append(seq(*[note("flute", m, 0.09) for m in ("G5", "C6", "E6")], note("flute", "G6", 0.5)))
    # 2 talk: a soft wooden blip
    fx.append(note("marimba", "A5", 0.07, 0.6))
    # 3 page: a paper flick
    n = n_of(0.08)
    fx.append(syn.bandpass(syn.noise(n), 2500, 8000) * np.sin(np.linspace(0, np.pi, n)) ** 2 * 0.6)
    # 4 pick up
    fx.append(mix(note("nylon", "E5", 0.2), 0.6 * delay(note("nylon", "B5", 0.25), 0.06)))
    # 5 a find (collectible): a little bell fanfare
    fx.append(seq(*[note("steel", m, 0.07) for m in ("C6", "E6", "G6")], mix(note("steel", "C7", 0.6), 0.5 * note("marimba", "C6", 0.6))))
    # 6 push: a stone scraping
    n = n_of(0.35)
    fx.append(syn.lowpass(syn.noise(n), 900) * syn.env_adsr(n, 0.05, 0.1, 0.7, 0.1) * 0.8)
    # 7 thud
    n = n_of(0.4)
    fx.append(mix(syn.kick(n, 1.0), syn.lowpass(syn.noise(n), 400) * syn.decay(n, 0.08) * 0.5))
    # 8 click: a pressure plate
    fx.append(mix(knock(0.1, 1500, 5000, 300) * 0.6, 0.4 * note("marimba", "E6", 0.08)))
    # 9 lever / a fitted gear: a heavy clunk
    fx.append(seq(knock(0.12, 300, 1600, 160), knock(0.18, 200, 900, 110), gap=0.03))
    # 10 gate: a wooden creak and a thump
    n = n_of(0.45)
    t = np.arange(n) / RATE
    creak = np.sin(2 * np.pi * (180 + 40 * np.sin(2 * np.pi * 9 * t)) * t) * syn.bandpass(syn.noise(n), 200, 1500) * 0.8
    fx.append(seq(creak * syn.env_adsr(n, 0.05, 0.1, 0.8, 0.1), knock(0.2, 150, 700, 90)))
    # 11 gears: a ratchet of teeth
    clicks = np.zeros(n_of(0.6))
    for k in range(12):
        i = int(k * 0.045 * RATE)
        c = knock(0.04, 2000, 7000, 400) * (0.9 - k * 0.04)
        clicks[i:i + len(c)] += c[:len(clicks) - i]
    fx.append(clicks)
    # 12 water: a rush
    n = n_of(0.9)
    fx.append(syn.bandpass(syn.noise(n), 500, 4000) * syn.env_adsr(n, 0.15, 0.2, 0.7, 0.4) * 0.7)
    # 13-15 the miners' chimes: low, middle, high
    for m in ("C5", "G5", "E6"):
        fx.append(mix(note("steel", m, 1.1), 0.4 * note("marimba", m, 0.6)))
    # 16 wrong: a dull double knock
    fx.append(seq(knock(0.12, 200, 800, 100), knock(0.16, 150, 600, 80), gap=0.05))
    # 17 solved: a bright arpeggio
    fx.append(seq(*[note("marimba", m, 0.08) for m in ("C5", "E5", "G5", "C6", "E6")], mix(note("steel", "G6", 0.8), note("flute", "C6", 0.8, 0.6))))
    # 18 hurt: an oof (a falling breathy tone)
    n = n_of(0.25)
    fx.append((chirp(520, 300, 0.25) * 0.6 + syn.bandpass(syn.noise(n), 400, 1800) * 0.3) * syn.decay(n, 0.1))
    # 19 whip: a crack
    n = n_of(0.16)
    fx.append(mix(chirp(2500, 600, 0.06) * syn.decay(n_of(0.06), 0.02) * 0.6, syn.bandpass(syn.noise(n), 2000, 9000) * syn.decay(n, 0.015)))
    # 20 calm: a relieved little rising tone
    fx.append(seq(note("flute", "E5", 0.12, 0.7), note("flute", "A5", 0.3, 0.7)))
    # 21 switch: two plucks
    fx.append(seq(note("nylon", "D5", 0.1), note("nylon", "A5", 0.22)))
    # 22 rest / save: a warm chord
    fx.append(mix(note("strings", "C5", 0.9, 0.6), note("strings", "E5", 0.9, 0.5), note("strings", "G5", 0.9, 0.5), note("steel", "C6", 0.7, 0.4)))
    # 23 menu
    fx.append(seq(note("marimba", "G5", 0.06), note("marimba", "D6", 0.15)))
    # 24 a secret: a mysterious little run
    fx.append(seq(*[note("steel", m, 0.07, 0.8) for m in ("A5", "C6", "E6", "G#6")], note("steel", "B6", 0.7)))
    # 25 step (unused while walking; kept for cut scenes)
    n = n_of(0.05)
    fx.append(syn.lowpass(syn.noise(n), 1200) * syn.decay(n, 0.015) * 0.5)
    return fx


def songs():
    # 0 title: the valley theme, D major, flute over nylon and strings.
    title = Song("title", 96, "D", "major", [
        S(2, "D A", "intro", "half", [P("strings", "pad", 4, 0.4, "pad")], crash=True),
        S(8, "D A Bm G D A G,A D", "half", "half", [P("nylon", "strum", 4, 0.45), P("strings", "pad", 4, 0.35, "pad")],
          [L("flute", "F#5 ~ A5 ~ D6 ~ C#6 B5 | A5 ~ ~ ~ E5 ~ - - | F#5 ~ B5 ~ D6 ~ E6 ~ | D6 ~ B5 ~ G5 ~ - - |"
                      "A5 ~ D6 ~ F#6 ~ E6 D6 | C#6 ~ ~ ~ A5 ~ - - | B5 ~ A5 ~ G5 ~ E5 ~ | D5 ~ ~ ~ - - - -", 0.55)], fill="toms"),
        S(8, "G A F#m Bm G A D D", "half", "octave", [P("marimba", "arp8", 5, 0.3, "arp"), P("strings", "pad", 4, 0.35, "pad")],
          [L("fiddle", "B5 ~ ~ ~ D6 ~ B5 ~ | C#6 ~ ~ ~ A5 ~ E5 ~ | F#5 ~ A5 ~ C#6 ~ A5 ~ | B5 ~ ~ ~ ~ ~ F#5 ~ |"
                       "G5 ~ B5 ~ D6 ~ G6 ~ | F#6 ~ E6 ~ C#6 ~ A5 ~ | D6 ~ ~ ~ A5 ~ F#5 ~ | D6 ~ ~ ~ - - - -", 0.5)]),
    ], {"bass": "pick_bass"}, seed=811)
    # 1 restored: the theme again, brighter, with bells.
    restored = Song("restored", 104, "D", "major", [
        S(8, "D G A D Bm G A D", "half", "rock8", [P("strings", "pad", 4, 0.45, "pad"), P("nylon", "strum", 4, 0.4)],
          [L("steel", "F#6 ~ E6 ~ D6 ~ A5 ~ | B5 ~ D6 ~ G6 ~ - - | A6 ~ G6 ~ F#6 ~ E6 ~ | D6 ~ ~ ~ - - - - |"
                      "F#6 ~ A6 ~ B6 ~ A6 ~ | G6 ~ F#6 ~ E6 ~ D6 ~ | E6 ~ F#6 ~ G6 ~ E6 ~ | D6 ~ ~ ~ - - - -", 0.55, harm=-3)],
          fill="toms"),
    ], {"bass": "pick_bass"}, seed=812)
    # 2 chapter: a short, wondering introduction (A minor to C).
    chapter = Song("chapter", 84, "A", "minor", [
        S(8, "Am F C G Am F Dm,E Am", "half", "half", [P("strings", "pad", 4, 0.45, "pad"), P("arp", "updown", 5, 0.22, "arp")],
          [L("flute", "E5 ~ ~ ~ A5 ~ C6 ~ | B5 ~ ~ ~ A5 ~ F5 ~ | G5 ~ ~ ~ C6 ~ E6 ~ | D6 ~ ~ ~ B5 ~ ~ ~ |"
                      "C6 ~ ~ ~ E6 ~ A6 ~ | G6 ~ ~ ~ F6 ~ C6 ~ | D6 ~ C6 ~ B5 ~ G#5 ~ | A5 ~ ~ ~ - - - -", 0.5)]),
    ], {"bass": "fm_bass"}, seed=813)
    # 3 danger: a tense ostinato for machines and storms (D minor).
    danger = Song("danger", 128, "D", "minor", [
        S(8, "Dm Bb C Dm Dm Bb Gm,A Dm", "drive", "octave", [P("strings", "pad", 3, 0.4, "pad"), P("marimba", "arp16", 4, 0.3, "arp")],
          [L("fiddle", "D5 ~ F5 ~ A5 ~ G5 F5 | D5 ~ ~ ~ Bb4 ~ - - | C5 ~ E5 ~ G5 ~ F5 E5 | D5 ~ ~ ~ - - - - |"
                       "A5 ~ G5 ~ F5 ~ E5 ~ | F5 ~ D5 ~ Bb4 ~ - - | G5 ~ F5 ~ E5 ~ C#5 ~ | D5 ~ ~ ~ - - - -", 0.5)], fill="toms"),
    ], {"bass": "synth_bass"}, seed=814)
    # (track 5 is the eye-catcher's chime)
    chime = Song("chime", 110, "D", "major", [
        S(2, "D A,D", None, None, [P("strings", "pad", 4, 0.4, "pad")],
          [L("steel", "D6 F#6 A6 D7 ~ ~ ~ ~ | C#7 ~ A6 ~ D7 ~ ~ ~", 0.6)], crash=False),
    ], {"bass": "pick_bass"}, loop=False, seed=815)
    # 4 secret: a hushed discovery (E dorian).
    secret = Song("secret", 80, "E", "dorian", [
        S(8, "Em A Em A C D Em Em", "none", "half", [P("strings", "pad", 4, 0.4, "pad"), P("arp", "updown", 5, 0.25, "arp")],
          [L("steel", "B5 ~ ~ ~ E6 ~ ~ ~ | C#6 ~ ~ ~ A5 ~ ~ ~ | G5 ~ B5 ~ E6 ~ ~ ~ | F#6 ~ ~ ~ C#6 ~ ~ ~ |"
                      "E6 ~ ~ ~ G6 ~ ~ ~ | F#6 ~ ~ ~ D6 ~ A5 ~ | B5 ~ ~ ~ ~ ~ ~ ~ | - - - - - - - -", 0.45)]),
    ], {"bass": "fm_bass"}, seed=816)
    # 5 ending: warm and full (G major).
    ending = Song("ending", 100, "G", "major", [
        S(8, "G D Em C G D C,D G", "half", "rock8", [P("strings", "pad", 4, 0.45, "pad"), P("nylon", "strum", 4, 0.4)],
          [L("flute", "B5 ~ D6 ~ G6 ~ F#6 E6 | D6 ~ ~ ~ A5 ~ - - | B5 ~ E6 ~ G6 ~ A6 ~ | G6 ~ E6 ~ C6 ~ - - |"
                      "D6 ~ G6 ~ B6 ~ A6 G6 | F#6 ~ ~ ~ D6 ~ - - | E6 ~ D6 ~ C6 ~ A5 ~ | G5 ~ ~ ~ - - - -", 0.55, harm=-3)],
          fill="toms"),
    ], {"bass": "pick_bass"}, seed=817)
    # 6 village: a morning dance (G major, 6/8 feel in eighths).
    village = Song("village", 112, "G", "major", [
        S(8, "G C G D G C D G", "half", "octave", [P("nylon", "strum", 4, 0.45), P("marimba", "skank", 5, 0.3)],
          [L("flute", "D5 G5 B5 ~ A5 G5 E5 ~ | G5 ~ C6 ~ B5 A5 G5 ~ | B5 A5 G5 ~ D5 ~ E5 F#5 | A5 ~ D6 ~ C6 B5 A5 ~ |"
                      "D5 G5 B5 ~ D6 B5 G5 ~ | C6 ~ E6 ~ D6 C6 B5 ~ | B5 ~ A5 ~ F#5 ~ A5 ~ | G5 ~ ~ ~ - - - -", 0.55)], fill="toms"),
        S(8, "Em C G D Em C D G", "half", "half", [P("nylon", "comp", 4, 0.4), P("strings", "pad", 4, 0.3, "pad")],
          [L("marimba", "B5 ~ G5 ~ E5 ~ G5 ~ | C6 ~ ~ ~ G5 ~ E5 ~ | D6 ~ B5 ~ G5 ~ B5 ~ | A5 ~ ~ ~ F#5 ~ D5 ~ |"
                        "E5 ~ G5 ~ B5 ~ E6 ~ | E6 ~ C6 ~ G5 ~ C6 ~ | D6 ~ C6 ~ B5 ~ A5 ~ | G5 ~ ~ ~ - - - -", 0.55)]),
    ], {"bass": "pick_bass"}, seed=818)
    # 7 mill: wood and water, a steady working tune (C mixolydian).
    mill = Song("mill", 100, "C", "mixolydian", [
        S(8, "C Bb F C C Bb F,G C", "march", "octave", [P("marimba", "arp8", 4, 0.35, "arp"), P("nylon", "comp", 4, 0.35)],
          [L("fiddle", "G5 ~ E5 ~ C5 ~ E5 G5 | Bb5 ~ ~ ~ F5 ~ - - | A5 ~ C6 ~ F6 ~ E6 D6 | C6 ~ ~ ~ G5 ~ - - |"
                       "E6 ~ D6 ~ C6 ~ Bb5 ~ | D6 ~ ~ ~ Bb5 ~ F5 ~ | A5 ~ G5 ~ F5 ~ D5 ~ | C5 ~ ~ ~ - - - -", 0.5)], fill="toms"),
    ], {"bass": "pick_bass"}, seed=819)
    # 8 forest: dappled light, koto and flute (A minor pentatonic colour).
    forest = Song("forest", 92, "A", "minor", [
        S(8, "Am G F G Am G F,E Am", "half", "half", [P("koto", "arp8", 5, 0.3, "arp"), P("strings", "pad", 4, 0.35, "pad")],
          [L("flute", "E5 ~ G5 ~ A5 ~ C6 ~ | D6 ~ ~ ~ B5 ~ G5 ~ | A5 ~ C6 ~ F6 ~ E6 ~ | D6 ~ ~ ~ ~ ~ B5 ~ |"
                      "C6 ~ E6 ~ A6 ~ G6 ~ | E6 ~ D6 ~ B5 ~ G5 ~ | A5 ~ G5 ~ F5 ~ E5 ~ | A5 ~ ~ ~ - - - -", 0.5)]),
        S(8, "F C G Am F C E E", "half", "octave", [P("nylon", "strum", 4, 0.4), P("arp", "updown", 5, 0.2, "arp")],
          [L("koto", "A5 ~ C6 ~ F6 ~ ~ ~ | E6 ~ C6 ~ G5 ~ ~ ~ | B5 ~ D6 ~ G6 ~ ~ ~ | E6 ~ ~ ~ C6 ~ A5 ~ |"
                     "F5 ~ A5 ~ C6 ~ F6 ~ | G6 ~ E6 ~ C6 ~ G5 ~ | G#5 ~ B5 ~ E6 ~ ~ ~ | E6 ~ ~ ~ - - - -", 0.5)]),
    ], {"bass": "fm_bass"}, seed=820)
    # 9 cave: drips and echoes (E minor, slow).
    cave = Song("cave", 72, "E", "minor", [
        S(8, "Em C Am B Em C Am,B Em", "none", "half", [P("strings", "pad", 3, 0.45, "pad"), P("marimba", "updown", 5, 0.22, "arp")],
          [L("steel", "B5 ~ ~ ~ ~ ~ G5 ~ | E6 ~ ~ ~ C6 ~ ~ ~ | A5 ~ ~ ~ C6 ~ E6 ~ | D#6 ~ ~ ~ ~ ~ B5 ~ |"
                      "G5 ~ ~ ~ B5 ~ E6 ~ | G6 ~ ~ ~ E6 ~ C6 ~ | A5 ~ C6 ~ B5 ~ F#5 ~ | E5 ~ ~ ~ - - - -", 0.45)]),
    ], {"bass": "fm_bass"}, seed=821)
    # 10 labyrinth: green shade and old stones (E dorian), koto and shakuhachi.
    labyrinth = Song("labyrinth", 88, "E", "dorian", [
        S(8, "Em A Em D C G A,B Em", "half", "half", [P("koto", "arp8", 5, 0.3, "arp"), P("strings", "pad", 4, 0.35, "pad")],
          [L("shakuhachi", "B4 ~ E5 ~ F#5 ~ G5 ~ | A5 ~ ~ ~ F#5 ~ E5 ~ | B4 ~ ~ ~ D5 ~ E5 ~ | F#5 ~ ~ ~ D5 ~ - - |"
                           "G5 ~ F#5 ~ E5 ~ D5 ~ | B4 ~ ~ ~ G4 ~ - - | A4 ~ C#5 ~ E5 ~ F#5 ~ | E5 ~ ~ ~ - - - -", 0.5)]),
        S(8, "C D Em Em C D B B", "half", "octave", [P("nylon", "strum", 4, 0.35), P("arp", "updown", 5, 0.2, "arp")],
          [L("koto", "G5 ~ E5 ~ C5 ~ E5 ~ | F#5 ~ A5 ~ D6 ~ ~ ~ | B5 ~ G5 ~ E5 ~ G5 ~ | B5 ~ ~ ~ ~ ~ - - |"
                     "E6 ~ D6 ~ C6 ~ G5 ~ | A5 ~ D6 ~ F#6 ~ ~ ~ | D#6 ~ ~ ~ B5 ~ F#5 ~ | B5 ~ ~ ~ - - - -", 0.5)]),
    ], {"bass": "fm_bass"}, seed=822)
    # 11 coast: sea air and sails (F major), steel drum and flute.
    coast = Song("coast", 108, "F", "major", [
        S(8, "F C Dm Bb F C Bb,C F", "half", "octave", [P("marimba", "skank", 5, 0.3), P("nylon", "strum", 4, 0.35)],
          [L("flute", "C6 ~ A5 ~ F5 ~ A5 C6 | E6 ~ D6 ~ C6 ~ - - | D6 ~ F6 ~ E6 D6 C6 ~ | Bb5 ~ ~ ~ G5 ~ - - |"
                      "A5 ~ C6 ~ F6 ~ E6 ~ | G6 ~ E6 ~ C6 ~ - - | D6 ~ C6 ~ Bb5 ~ G5 ~ | F5 ~ ~ ~ - - - -", 0.55)], fill="toms"),
        S(8, "Bb F Gm C Bb F C C", "half", "half", [P("steel", "arp8", 5, 0.25, "arp"), P("strings", "pad", 4, 0.3, "pad")],
          [L("steel", "D6 ~ ~ ~ F6 ~ D6 ~ | C6 ~ ~ ~ A5 ~ F5 ~ | G5 ~ Bb5 ~ D6 ~ G6 ~ | E6 ~ ~ ~ C6 ~ - - |"
                      "D6 ~ F6 ~ Bb6 ~ A6 ~ | A6 ~ F6 ~ C6 ~ A5 ~ | G5 ~ C6 ~ E6 ~ G6 ~ | C6 ~ ~ ~ - - - -", 0.5)]),
    ], {"bass": "pick_bass"}, seed=823)
    # 12 lighthouse: the clockwork tower (B minor), marimba ticking under the erhu.
    lighthouse = Song("lighthouse", 92, "B", "minor", [
        S(8, "Bm G D A Bm G Em,F# Bm", "half", "half", [P("marimba", "arp16", 4, 0.25, "arp"), P("strings", "pad", 4, 0.35, "pad")],
          [L("erhu", "F#5 ~ ~ ~ B5 ~ A5 ~ | G5 ~ ~ ~ D5 ~ ~ ~ | F#5 ~ A5 ~ D6 ~ C#6 ~ | A5 ~ ~ ~ E5 ~ - - |"
                     "D6 ~ C#6 ~ B5 ~ F#5 ~ | G5 ~ B5 ~ D6 ~ ~ ~ | E6 ~ D6 ~ C#6 ~ A#5 ~ | B5 ~ ~ ~ - - - -", 0.5)]),
    ], {"bass": "fm_bass"}, seed=824)
    # 13 orchard: a harvest dance (C major), balalaika and reed.
    orchard = Song("orchard", 116, "C", "major", [
        S(8, "C F G C Am F G C", "half", "octave", [P("balalaika", "strum", 4, 0.4), P("marimba", "skank", 5, 0.25)],
          [L("reed", "E5 G5 C6 ~ B5 A5 G5 ~ | A5 ~ F5 ~ A5 C6 ~ - | B5 ~ D6 ~ G5 ~ B5 ~ | C6 ~ ~ ~ G5 E5 - - |"
                     "A5 ~ C6 ~ E6 ~ C6 ~ | F5 ~ A5 ~ C6 ~ A5 ~ | G5 ~ B5 ~ D6 ~ B5 G5 | C6 ~ ~ ~ - - - -", 0.55)], fill="toms"),
        S(8, "F G Em Am F G C C", "half", "half", [P("nylon", "comp", 4, 0.35), P("strings", "pad", 4, 0.3, "pad")],
          [L("fiddle", "A5 ~ C6 ~ F6 ~ E6 ~ | D6 ~ ~ ~ B5 ~ G5 ~ | G5 ~ B5 ~ E6 ~ D6 ~ | C6 ~ ~ ~ A5 ~ E5 ~ |"
                       "F5 ~ A5 ~ C6 ~ F6 ~ | G6 ~ F6 ~ D6 ~ B5 ~ | C6 ~ E6 ~ G6 ~ E6 ~ | C6 ~ ~ ~ - - - -", 0.5)]),
    ], {"bass": "pick_bass"}, seed=825)
    # 14 grotto: crystal drops in the dark (C# minor), steel and choir.
    grotto = Song("grotto", 70, "C#", "minor", [
        S(8, "C#m A E B C#m A F#m,G# C#m", "none", "half", [P("choir", "pad", 4, 0.35, "pad"), P("kulintang", "updown", 5, 0.2, "arp")],
          [L("steel", "G#5 ~ ~ ~ C#6 ~ ~ ~ | E6 ~ ~ ~ B5 ~ ~ ~ | G#5 ~ B5 ~ E6 ~ ~ ~ | D#6 ~ ~ ~ F#5 ~ ~ ~ |"
                      "E5 ~ G#5 ~ C#6 ~ E6 ~ | C#6 ~ ~ ~ A5 ~ ~ ~ | A5 ~ G#5 ~ F#5 ~ C6 ~ | C#6 ~ ~ ~ - - - -", 0.45)]),
    ], {"bass": "fm_bass"}, seed=826)
    # 15 worldtree: the great tree and the open sky (E flat major), strings, choir and flute.
    worldtree = Song("worldtree", 84, "Eb", "major", [
        S(8, "Eb Bb Cm Ab Eb Bb Ab,Bb Eb", "half", "whole", [P("strings", "pad", 4, 0.45, "pad"), P("choir", "half", 4, 0.25, "pad")],
          [L("flute", "G5 ~ Bb5 ~ Eb6 ~ ~ ~ | D6 ~ ~ ~ Bb5 ~ F5 ~ | G5 ~ C6 ~ Eb6 ~ D6 C6 | Ab5 ~ ~ ~ ~ ~ - - |"
                      "Bb5 ~ Eb6 ~ G6 ~ F6 ~ | F6 ~ D6 ~ Bb5 ~ ~ ~ | C6 ~ Bb5 ~ Ab5 ~ F5 ~ | Eb5 ~ ~ ~ - - - -", 0.55)]),
    ], {"bass": "pick_bass"}, seed=827)
    # 16 works: the old hydraulic halls, urgent but hopeful (A minor), organ and brass.
    works = Song("works", 120, "A", "minor", [
        S(8, "Am F G Am Am F G,E Am", "drive", "octave", [P("organ", "stab", 4, 0.35), P("strings", "pad", 4, 0.3, "pad")],
          [L("brass", "A4 ~ C5 ~ E5 ~ A5 ~ | F5 ~ ~ ~ C5 ~ - - | G5 ~ B4 ~ D5 ~ G5 ~ | A5 ~ ~ ~ E5 ~ - - |"
                      "C6 ~ B5 ~ A5 ~ E5 ~ | F5 ~ A5 ~ C6 ~ - - | B5 ~ D6 ~ G#5 ~ B5 ~ | A5 ~ ~ ~ - - - -", 0.5)], fill="toms"),
    ], {"bass": "synth_bass"}, seed=828)
    by_name = {s.name: s for s in (title, restored, chapter, danger, secret, ending, village, mill, forest, cave,
                                   labyrinth, coast, lighthouse, orchard, grotto, worldtree, works)}
    # in ML_MUS order (the world's music list), the eye-catcher's chime as file 5
    import world as world_tool
    order = world_tool.load().music_list()
    missing = [n for n in order if n not in by_name]
    if missing:
        raise SystemExit(f"sound: no tune for {missing}")
    out = [by_name[n] for n in order]
    return out[:4] + [chime] + out[4:]


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
