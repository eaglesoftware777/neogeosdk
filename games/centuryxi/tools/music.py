#!/usr/bin/env python3
"""Century XI's music: original tunes in the spirit of 1994 arcade football, played by
the SDK's instruments (sound/tools/ng_synth.py, ng_songs.py) into ADPCM-B beds.

During a match there is no music, as on the television of the day: the crowd has the
ADPCM-B channel and the announcer and effects ride on ADPCM-A. The tunes are for
the screens round the match. Numbers: see scenes/cx_audio.c.
"""
from pathlib import Path
import sys

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / "sound/tools"))
from ng_songs import L, P, S, Song, render_song, write_wav  # noqa: E402

OUT_A = GAME / "sound/samples/in_wav_a"
OUT_B = GAME / "sound/samples/in_wav_b"

ROCK = {"bass": "pick_bass"}
FUNK = {"bass": "slap_bass"}
SYNTH = {"bass": "synth_bass"}


def title():
    a = ("C5 ~ C5 D5 E5 ~ G5 ~ | G5 ~ ~ ~ E5 D5 C5 ~ | A5 ~ A5 G5 E5 ~ C5 ~ | F5 ~ ~ ~ - - E5 F5 |"
         "G5 ~ G5 A5 G5 ~ E5 ~ | D5 ~ E5 ~ G5 ~ B5 ~ | C6 ~ B5 ~ A5 ~ G5 ~ | C6 ~ ~ ~ - - - -")
    b = ("A5 ~ G5 ~ F5 ~ A5 ~ | G5 ~ F5 ~ E5 ~ G5 ~ | E5 ~ D5 ~ E5 ~ G5 ~ | A5 ~ ~ ~ C6 ~ A5 ~ |"
         "F5 ~ G5 ~ A5 ~ C6 ~ | D6 ~ C6 ~ B5 ~ G5 ~ | C6 ~ D6 ~ E6 ~ D6 ~ | C6 ~ ~ ~ G5 G5 A5 B5")
    return Song("title", 136, "C", "major", [
        S(2, "F G", "build", "whole", [P("brass", "stab", 4, 0.4)], fill="rock"),
        S(8, "C G Am F C G F,G C", "rock", "rock8", [P("power", "push", 3, 0.3), P("brass", "stab2", 4, 0.28)],
          [L("trumpet", a, 0.62)]),
        S(8, "F G Em Am F G C C", "drive", "octave", [P("power", "chug", 3, 0.28), P("strings", "pad", 4, 0.25, "pad")],
          [L("trumpet", b, 0.62)], fill="rock"),
    ], ROCK, seed=1994, master=0.85)


def menu():
    a = ("G5 ~ - G5 A5 ~ B5 ~ | D6 ~ B5 ~ A5 ~ G5 ~ | F5 ~ - F5 G5 ~ A5 ~ | C6 ~ A5 ~ G5 ~ F5 ~ |"
         "G5 ~ - G5 A5 ~ B5 ~ | D6 ~ E6 ~ D6 ~ B5 ~ | A5 ~ G5 ~ A5 ~ B5 ~ | G5 ~ ~ ~ - - - -")
    return Song("menu", 118, "G", "mixolydian", [
        S(8, "G G F F G G Am,D G", "funk", "funk", [P("epiano", "skank", 4, 0.28), P("clean", "push", 4, 0.2)],
          [L("sax", a, 0.5)]),
        S(8, "C D G Em C D Am,D G", "funk", "funk2", [P("epiano", "comp", 4, 0.28), P("strings", "pad", 4, 0.2, "pad")],
          [L("organ", a, 0.4, transpose=12)]),
    ], FUNK, swing=0.08, seed=1995, master=0.8)


def penalty():
    a = ("A4 ~ ~ ~ - - - - | - - - - C5 ~ B4 ~ | A4 ~ ~ ~ - - - - | - - - - E5 ~ D5 ~ |"
         "A4 ~ ~ ~ - - - - | - - - - C5 ~ D5 ~ | E5 ~ ~ ~ ~ ~ ~ ~ | E5 ~ ~ ~ ~ ~ ~ ~")
    return Song("penalty", 92, "A", "minor", [
        S(8, "Am Am F F Am Am E E", "taiko", "pulse", [P("strings", "pad", 3, 0.32, "pad")], [L("mute", a, 0.34)], crash=False),
    ], SYNTH, seed=1996, master=0.75)


def final():
    a = ("D5 ~ ~ ~ A5 ~ ~ ~ | F#5 ~ E5 ~ D5 ~ ~ ~ | G5 ~ ~ ~ B5 ~ ~ ~ | A5 ~ G5 ~ F#5 ~ E5 ~ |"
         "D5 ~ F#5 ~ A5 ~ D6 ~ | C#6 ~ B5 ~ A5 ~ F#5 ~ | G5 ~ A5 ~ B5 ~ C#6 ~ | D6 ~ ~ ~ ~ ~ - -")
    return Song("final", 100, "D", "major", [
        S(8, "D A Bm G D A G,A D", "march", "march", [P("strings", "pad", 4, 0.3, "pad"), P("choir", "pad", 4, 0.22, "pad")],
          [L("brass", a, 0.6)], fill="rock"),
    ], ROCK, seed=2030, master=0.82)


def champion():
    a = ("C6 ~ C6 C6 D6 ~ E6 ~ | G6 ~ ~ ~ E6 ~ C6 ~ | F6 ~ F6 F6 E6 ~ D6 ~ | C6 ~ ~ ~ - - G5 A5 |"
         "C6 ~ C6 C6 D6 ~ E6 ~ | G6 ~ E6 ~ G6 ~ A6 ~ | G6 ~ F6 ~ E6 ~ D6 ~ | C6 ~ ~ ~ ~ ~ - -")
    return Song("champion", 142, "C", "major", [
        S(8, "C C F C C Am D,G C", "samba", "samba", [P("brass", "stab", 4, 0.34), P("nylon", "strum", 4, 0.22)],
          [L("trumpet", a, 0.6)]),
    ], ROCK, seed=1990, master=0.85)


def jingle():
    a = "G5 G5 G5 C6 ~ ~ E6 ~ | G6 ~ ~ ~ ~ ~ ~ ~"
    return Song("jingle", 150, "C", "major", [
        S(2, "C G", "rock", "rock8", [P("brass", "stab", 4, 0.4)], [L("trumpet", a, 0.7)]),
    ], ROCK, seed=7, master=0.85)


def main():
    for n, song in ((2, title()), (3, menu()), (4, penalty()), (6, final()), (7, champion())):
        write_wav(OUT_B / f"{n}.wav", render_song(song))
        print("B", n, song.name)
    write_wav(OUT_A / "45.wav", render_song(jingle()))


if __name__ == "__main__":
    main()
