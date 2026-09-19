"""Synthesize the full custom audio bank for Maiya: Super Nature Girl.

    python3 games/maiya/tools/build_sound_bank.py

Produces:
1. 16 custom 16-bit mono WAV sound effects in games/maiya/sound/samples/in_wav_a:
   - 1.wav: Rose whip crack / slash
   - 2.wav: Rose thorn projectile throw
   - 3.wav: Slime squish / enemy splatter
   - 4.wav: Metal clank / armor deflect
   - 5.wav: Toxic acid spit
   - 6.wav: Syndicate robot laser pulse
   - 7.wav: Buzzsaw rev / chainsaw whine
   - 8.wav: Flame burst / fire breath
   - 9.wav: Ice crystal shatter
   - 10.wav: Arcade explosion
   - 11.wav: Seed / gem pickup chime
   - 12.wav: Life heart restore chime
   - 13.wav: Golden rose / secret art surge
   - 14.wav: Boss roar / mechanical screech
   - 15.wav: Wind dash whoosh / jump
   - 16.wav: Player hurt / impact

2. 8 melodic Shinobi-inspired MML tracks in games/maiya/sound/mml:
   - 0: Attract / Title ("Valley of the Rose")
   - 1: World 1 Greenroot Forest (Fast funky Shinobi groove, pentatonic lead)
   - 2: World 2 Mudriver Marsh (Murky atmospheric minor groove)
   - 3: World 3 Smokestack City (Heavy industrial mechanical rhythm)
   - 4: World 4 Burning Drylands (Intense fiery pulse)
   - 5: World 5 Frozen Sky Peaks (Crystalline frost melody)
   - 6: World 6 & 7 Black Core Citadel (Dark techno suspense)
   - 7: Boss Battle ("Wrath of the Syndicate" - rapid dramatic confrontation)
"""

import math
from pathlib import Path
import wave
import numpy as np

GAME = Path(__file__).resolve().parents[1]
SOUND_DIR = GAME / "sound"
WAV_A_DIR = SOUND_DIR / "samples/in_wav_a"
MML_DIR = SOUND_DIR / "mml"

RATE = 22050


def write_wav(path, data):
    data = np.clip(data, -1.0, 1.0)
    pcm = (data * 32000.0).astype("<i2")
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(pcm.tobytes())


def env(n, attack, decay):
    t = np.arange(n) / RATE
    e = np.minimum(1.0, t / max(attack, 1e-4))
    return e * np.exp(-t / decay)


def sfx_whip(duration=0.18):
    """Crisp whip crack and high speed whoosh."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    noise = np.random.uniform(-1.0, 1.0, n)
    whoosh = np.sin(2.0 * math.pi * (400.0 + 800.0 * np.exp(-t * 20.0)) * t)
    crack = np.where(t < 0.015, noise * 2.5, noise * 0.3)
    return (whoosh * 0.6 + crack * 0.8) * env(n, 0.001, 0.04)


def sfx_thorn(duration=0.15):
    """Spinning petal-thorn projectile whistle."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    freq = 1200.0 + 600.0 * np.sin(2.0 * math.pi * 35.0 * t) - t * 3000.0
    phase = 2.0 * math.pi * np.cumsum(freq) / RATE
    return np.sin(phase) * env(n, 0.002, 0.05) * 0.75


def sfx_squish(duration=0.22):
    """Slime / organic splatter."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    mod = np.sin(2.0 * math.pi * 45.0 * t)
    carrier = np.sin(2.0 * math.pi * (250.0 + 150.0 * mod) * t)
    noise = np.random.uniform(-0.6, 0.6, n) * np.exp(-t * 15.0)
    return (carrier * 0.7 + noise * 0.5) * env(n, 0.003, 0.07)


def sfx_clank(duration=0.25):
    """Sharp metal armor deflection."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    tones = (
        np.sin(2.0 * math.pi * 920.0 * t) * 0.5 +
        np.sin(2.0 * math.pi * 1440.0 * t) * 0.4 +
        np.sin(2.0 * math.pi * 2310.0 * t) * 0.3
    )
    noise = np.random.uniform(-0.8, 0.8, n) * np.exp(-t * 80.0)
    return (tones * 0.7 + noise * 0.8) * env(n, 0.001, 0.05)


def sfx_acid(duration=0.28):
    """Toxic sludge / acid spit."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    f = 320.0 * np.exp(-t * 6.0) + 120.0 * np.sin(2.0 * math.pi * 28.0 * t)
    phase = 2.0 * math.pi * np.cumsum(f) / RATE
    noise = np.random.uniform(-0.4, 0.4, n) * (1.0 - t / duration)
    return (np.sin(phase) * 0.6 + noise * 0.5) * env(n, 0.004, 0.08)


def sfx_laser(duration=0.16):
    """Blight syndicate drone pulse laser."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    f = 2400.0 * np.exp(-t * 18.0) + 180.0
    phase = 2.0 * math.pi * np.cumsum(f) / RATE
    sq = np.sign(np.sin(phase)) * 0.6 + 0.4 * np.sin(phase * 2.0)
    return sq * env(n, 0.002, 0.04) * 0.8


def sfx_buzzsaw(duration=0.35):
    """Buzzsaw rev / chainsaw blade whine."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    saw = 2.0 * ((t * 180.0) % 1.0) - 1.0
    saw2 = 2.0 * ((t * 360.0) % 1.0) - 1.0
    noise = np.random.uniform(-0.3, 0.3, n)
    return (saw * 0.5 + saw2 * 0.3 + noise * 0.2) * env(n, 0.01, 0.12) * 0.85


def sfx_flame(duration=0.42):
    """Fire breath / flame burst."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    noise = np.random.uniform(-1.0, 1.0, n)
    lp = np.zeros(n)
    acc = 0.0
    for i in range(n):
        acc += 0.08 * (noise[i] - acc)
        lp[i] = acc
    thump = np.sin(2.0 * math.pi * 85.0 * t) * np.exp(-t * 12.0)
    return (lp * 2.5 + thump * 0.5) * env(n, 0.005, 0.14) * 0.9


def sfx_ice(duration=0.32):
    """Crystal ice shatter."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    c1 = np.sin(2.0 * math.pi * 1840.0 * t) * env(n, 0.001, 0.05)
    c2 = np.sin(2.0 * math.pi * 2620.0 * t) * env(n, 0.002, 0.07)
    c3 = np.sin(2.0 * math.pi * 3520.0 * t) * env(n, 0.001, 0.09)
    noise = np.random.uniform(-0.5, 0.5, n) * env(n, 0.001, 0.04)
    return (c1 * 0.35 + c2 * 0.3 + c3 * 0.25 + noise * 0.3) * 0.9


def sfx_explosion(duration=0.65):
    """Deep thunderous arcade explosion."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    noise = np.random.uniform(-1.0, 1.0, n)
    lp = np.zeros(n)
    acc = 0.0
    for i in range(n):
        k = 0.25 * math.exp(-i / (RATE * 0.25)) + 0.02
        acc += k * (noise[i] - acc)
        lp[i] = acc
    sub = np.sin(2.0 * math.pi * (65.0 * np.exp(-t * 5.0) + 35.0) * t)
    return (lp * 2.8 * env(n, 0.002, 0.2) + sub * 0.8 * env(n, 0.002, 0.12)) * 0.85


def sfx_pickup(duration=0.28):
    """Crystal / seed pickup arpeggio."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    out = np.zeros(n)
    notes = ((0.0, 784.0), (0.06, 987.8), (0.12, 1174.7), (0.18, 1568.0))
    for start, freq in notes:
        i0 = int(start * RATE)
        seg = t[: n - i0]
        out[i0:] += np.sin(2.0 * math.pi * freq * seg) * env(n - i0, 0.002, 0.06)
    return out * 0.65


def sfx_heart(duration=0.45):
    """Life restore warm magic chime."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    out = np.zeros(n)
    notes = ((0.0, 523.3), (0.08, 659.3), (0.16, 784.0), (0.24, 1046.5))
    for start, freq in notes:
        i0 = int(start * RATE)
        seg = t[: n - i0]
        sine = np.sin(2.0 * math.pi * freq * seg)
        harm = 0.3 * np.sin(4.0 * math.pi * freq * seg)
        out[i0:] += (sine + harm) * env(n - i0, 0.004, 0.12)
    return out * 0.7


def sfx_art_surge(duration=0.6):
    """Secret Art: Rose storm power surge."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    f = 440.0 + 800.0 * (t / duration) ** 2
    phase = 2.0 * math.pi * np.cumsum(f) / RATE
    sine = np.sin(phase) + 0.3 * np.sin(2.0 * phase)
    noise = np.random.uniform(-0.3, 0.3, n) * (t / duration)
    return (sine * 0.65 + noise * 0.4) * env(n, 0.02, 0.25) * 0.9


def sfx_boss_roar(duration=0.55):
    """Imposing boss roar / mechanical screech."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    f = 160.0 + 60.0 * np.sin(2.0 * math.pi * 18.0 * t) - 40.0 * t
    phase = 2.0 * math.pi * np.cumsum(f) / RATE
    sub = np.sin(phase) * 0.6
    dist = np.clip(sub * 2.0, -0.8, 0.8)
    noise = np.random.uniform(-0.4, 0.4, n) * env(n, 0.01, 0.2)
    return (dist + noise) * env(n, 0.01, 0.18) * 0.85


def sfx_dash(duration=0.22):
    """Wind dash whoosh."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    noise = np.random.uniform(-1.0, 1.0, n)
    lp = np.zeros(n)
    acc = 0.0
    for i in range(n):
        acc += 0.15 * (noise[i] - acc)
        lp[i] = acc
    whoosh = np.sin(2.0 * math.pi * (200.0 + 400.0 * np.exp(-t * 12.0)) * t)
    return (lp * 1.8 + whoosh * 0.5) * env(n, 0.005, 0.08) * 0.8


def sfx_hurt(duration=0.24):
    """Hero hurt / impact groan."""
    n = int(RATE * duration)
    t = np.arange(n) / RATE
    f = 280.0 * np.exp(-t * 16.0) + 90.0
    phase = 2.0 * math.pi * np.cumsum(f) / RATE
    thump = np.sin(phase) * env(n, 0.002, 0.06)
    noise = np.random.uniform(-0.5, 0.5, n) * env(n, 0.001, 0.04)
    return (thump * 0.7 + noise * 0.5) * 0.85


def build_sfx():
    WAV_A_DIR.mkdir(parents=True, exist_ok=True)
    sounds = [
        (1, sfx_whip),
        (2, sfx_thorn),
        (3, sfx_squish),
        (4, sfx_clank),
        (5, sfx_acid),
        (6, sfx_laser),
        (7, sfx_buzzsaw),
        (8, sfx_flame),
        (9, sfx_ice),
        (10, sfx_explosion),
        (11, sfx_pickup),
        (12, sfx_heart),
        (13, sfx_art_surge),
        (14, sfx_boss_roar),
        (15, sfx_dash),
        (16, sfx_hurt),
    ]
    for idx, fn in sounds:
        path = WAV_A_DIR / f"{idx}.wav"
        write_wav(path, fn())
    print(f"Generated {len(sounds)} custom 16-bit WAV SFX in {WAV_A_DIR}")


# Shinobi-inspired MML tracks for the 7 worlds, Title, and Boss
MML_TRACKS = {
    # Track 0: Attract / Title Screen ("Valley of the Rose")
    "0_mml_example_a.mml": """\
; Title / Attract Mode: Valley of the Rose
; Suspenseful melodic opening leading into heroic arcade theme
T26
@p1
@b0
V13
O5
L8

; Phrase 1: The Valley at Dawn
E G B > D < B G E4
< B > E G B G E < B4
; Phrase 2: The Blight Approaches
> C E G B G E C4
< A > C E A E C < A4
; Phrase 3: Maiya Steps Forward
> D F# A > C < A F# D4
< B > D F# B F# D < B4
; Phrase 4: The Heroic Resolve
> E G B > E < B G E4
< A > C E A > C4 R4
~
""",

    # Track 1: World 1 - Greenroot Forest (Fast funky Shinobi groove)
    "1_mml_example_b.mml": """\
; World 1: Greenroot Forest
; Iconic driving pentatonic bass and energetic arcade flute
T22
@p2
@b1
V14
O4
L8

; Funky Shinobi groove
E G A B > D < B A G
E E G E  A A B4
; High flute counter-melody
> E4 D8 < B8  A G E4
A B > D E  < B A G4
; Second measure: rapid step
< E8 E8 G8 E8  A8 A8 B4
> D8 E8 D8 < B8  A8 G8 E4
~
""",

    # Track 2: World 2 - Mudriver Marsh (Murky atmospheric groove)
    "2_mml_example_c.mml": """\
; World 2: Mudriver Marsh
; Atmospheric minor swamp tension with bubbling accents
T24
@p3
@b2
V12
O4
L8

D F G A  F D < A4
> D D F D  G G A4
< A# > D F G  F D < A#4
> C E G A#  A G E4
; Tension lift
D F A > D < A F D4
< G > C E G  E C < G4
< A# > D F A#  > D4 R4
~
""",

    # Track 3: World 3 - Smokestack City (Industrial factory drive)
    "3_mml_example_d.mml": """\
; World 3: Smokestack City
; Heavy mechanical pulse and driving synth rhythm
T20
@p4
@b3
V14
O4
L8

C C D# C  F F G4
C D# F G  G# G D# C
< G# > C D# G#  G D# C < G#
< A# > D F A#  > C4 R4
; Iron rhythm
C8 C8 D#8 C8  F8 F8 G4
> C8 < A#8 G#8 G8  F8 D#8 C4
~
""",

    # Track 4: World 4 - Burning Drylands (Volcanic fiery pulse)
    "4_mml_example_e.mml": """\
; World 4: Burning Drylands
; Fast rhythmic tempo with fiery brass stabs
T20
@p5
@b4
V13
O5
L8

D F# A > D < A F# D4
< A > D F# A  F# D < A4
< G > B D G  B G D < G
< A > C# E A  > C#4 R4
; Fire storm surge
> D8 D8 F#8 D8  A8 A8 > D4
< B8 A8 F#8 E8  D8 < B8 A4
~
""",

    # Track 5: World 5 - Frozen Sky Peaks (Glacial wind and crystal bells)
    "5_mml_example_f.mml": """\
; World 5: Frozen Sky Peaks
; Crystalline chimes and soaring azure mountain melody
T24
@p6
@b5
V13
O5
L8

A B > C E < B A G4
< E > A B > C < B A E4
F A > C D < A F D4
E G B > D < B G E4
; High wind glide
> A4 G8 E8  D C < A4
> C D E G  E D C4
~
""",

    # Track 6: World 6 & 7 - Black Core Citadel (Technological suspense)
    "6_mml_example_g.mml": """\
; World 6 & 7: Black Core Citadel
; High-stakes dark techno pulse, final fortress confrontation
T18
@p7
@b6
V14
O4
L8

C D# F# G  F# D# C < A#
> C C D# C  F# F# G4
< G# > C D# F#  D# C < G#4
< A# > D F A#  > D4 R4
; Deep reactor pulse
C8 C8 C8 D#8  F#8 F#8 G4
> D#8 C8 < A#8 G8  F#8 D#8 C4
~
""",

    # Track 7: Boss Battle ("Wrath of the Syndicate")
    "7_mml_example_h.mml": """\
; Boss Battle: Wrath of the Syndicate
; Furious arcade boss tempo, pounding battle drums, intense brass
T16
@p8
@b7
V15
O4
L8

E G A# B  A# G E < B
> E E G E  A# A# B4
> E8 E8 G8 E8  A#8 A#8 B4
> D8 C8 < B8 A#8  G8 F8 E4
; Dramatic clash
< A# > C# E G  E C# < A#4
< B > D# F# A  F# D# < B4
> E8 G8 A#8 > C#8  < B8 G8 E4
~
""",
}


def build_mml():
    MML_DIR.mkdir(parents=True, exist_ok=True)
    for fname, content in MML_TRACKS.items():
        (MML_DIR / fname).write_text(content, encoding="ascii")
    print(f"Generated {len(MML_TRACKS)} Shinobi-inspired MML tracks in {MML_DIR}")


def main():
    print("== Building Audio Bank for Maiya: Super Nature Girl ==")
    build_sfx()
    build_mml()
    print("Audio assets generated successfully.")


if __name__ == "__main__":
    main()
