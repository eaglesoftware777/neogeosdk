#!/usr/bin/env python3
"""The announcer, the officials and the terraces, by speech synthesis.

Run with an environment holding kokoro_onnx: voices.py MODEL_DIR [--names] (--names:
only the players' names, after a squad changes). It writes the
spoken lines into sound/samples/in_wav_a after tools/sfx.py's sounds, numbered as
scenes/cx_audio.c expects, through a stadium-tannoy chain: narrow band, a little
drive and a short slap of echo. The voices are synthetic; none is a real person's.
"""
import sys
import wave
from pathlib import Path

import numpy as np
from kokoro_onnx import Kokoro

OUT = Path(__file__).resolve().parents[1] / "sound" / "samples"
RATE = 22050
FIRST = 10            # 1-9 are tools/sfx.py's

# (text, voice, speed, pitch ratio, tannoy)
ANN, REF, COACH, PLAYER = "am_michael", "bm_george", "jm_kumo", "am_puck"
LINES = [
    ("It's a goal! Goal! Goal! Goal!", ANN, 1.1, 1.06, 1), ("What a goal!", ANN, 1.15, 1.06, 1), ("Oh, what a header!", ANN, 1.15, 1.06, 1),
    ("Amazing!", ANN, 1.1, 1.06, 1), ("Own goal! Oh no!", ANN, 1.15, 1.06, 1), ("Great save!", ANN, 1.15, 1.06, 1),
    ("So close!", ANN, 1.1, 1.06, 1), ("Off the post!", ANN, 1.15, 1.06, 1), ("Offside!", ANN, 1.1, 1.06, 1),
    ("Foul!", ANN, 1.0, 1.06, 1), ("Yellow card!", ANN, 1.1, 1.06, 1), ("Red card! He's off!", ANN, 1.15, 1.06, 1),
    ("Penalty!", ANN, 1.1, 1.06, 1), ("Free kick.", ANN, 1.1, 1.06, 1), ("Corner kick.", ANN, 1.1, 1.06, 1),
    ("Kick off!", ANN, 1.1, 1.06, 1), ("Half time.", ANN, 1.05, 1.06, 1), ("Full time!", ANN, 1.05, 1.06, 1),
    ("Extra time!", ANN, 1.1, 1.06, 1), ("Golden goal!", ANN, 1.15, 1.06, 1), ("Hand ball!", ANN, 1.1, 1.06, 1),
    ("Through the legs!", ANN, 1.2, 1.06, 1), ("Video review.", ANN, 1.05, 1.06, 1), ("Substitution.", ANN, 1.1, 1.06, 1),
    ("Referee! Are you blind?", PLAYER, 1.2, 1.12, 0), ("My leg! My leg!", PLAYER, 1.25, 1.14, 0), ("Oh, come on!", PLAYER, 1.15, 1.1, 0),
    ("No talking!", REF, 1.0, 1.02, 0), ("Get off my pitch!", REF, 1.05, 1.02, 0), ("Play on!", REF, 1.05, 1.02, 0),
    ("Run! Run! Run!", COACH, 1.2, 1.0, 0), ("Why? Why? Why?", COACH, 1.15, 1.0, 0), ("Yes! Yes! Yes!", COACH, 1.2, 1.0, 0),
]


FIRST_CHANT = 46      # one per nation, in tools/nations.py's order; 45 is the goal fanfare
# (what the terraces sing, voice, language, the beat under it: None, "clap", "drum")
CHANTS = [
    ("Sambalia! Sambalia! Sambalia!", "pm_alex", "pt-br", "drum"), ("Vamos, vamos, Argentango!", "em_alex", "es", "drum"),
    ("Mateguay! Mateguay! Mateguay!", "em_alex", "es", None), ("Coffeelombia! Coffeelombia!", "em_alex", "es", "drum"),
    ("Mariachia! Mariachia! Ra ra ra!", "em_alex", "es", "clap"), ("Burgerland! Burgerland!", "am_adam", "en-us", None),
    ("Go Mapledonia go! Go go go!", "am_adam", "en-us", None), ("Pura vida! Puravidia!", "em_alex", "es", "drum"),
    ("Allez Baguettia! Allez!", "em_alex", "fr-fr", "clap"), ("Wurstland! Wurstland!", "am_adam", "de", "clap"),
    ("Forza Pastalia! Forza!", "im_nicola", "it", None), ("Paellania, ole, ole, ole!", "em_alex", "es", None),
    ("Tealand! Tealand! Tealand!", "bm_george", "en-gb", None), ("Bacalandia! Bacalandia!", "pm_alex", "pt-br", "clap"),
    ("Hup Tuliplands hup! Hup Tuliplands hup!", "am_adam", "nl", "clap"), ("Allez Wafflegium! Allez!", "em_alex", "fr-fr", "clap"),
    ("Cravatia! Cravatia!", "am_adam", "hr", "clap"), ("Heja Meatballia! Heja!", "am_adam", "sv", None),
    ("Hai Carpathia! Hai!", "im_nicola", "ro", "clap"), ("Heia Fjordway! Heia!", "am_adam", "nb", None),
    ("Mintorocco! Mintorocco!", "am_adam", "en-us", "drum"), ("Pyramidia! Pyramidia!", "am_adam", "en-us", "drum"),
    ("Jollofia! Jollofia! Jollof power!", "am_adam", "en-us", "drum"), ("Allez Senegoal! Allez!", "em_alex", "fr-fr", "drum"),
    ("Cameroar! Roar! Roar!", "em_alex", "fr-fr", "drum"), ("Chocoghana! Chocoghana!", "am_adam", "en-us", "drum"),
    ("Sushipon! Sushipon! Sushipon!", "jm_kumo", "en-us", "drum"), ("Kimchirea! Kimchirea!", "jm_kumo", "en-us", "korea"),
    ("Kangaroolia! Oi! Oi! Oi!", "am_adam", "en-us", None), ("Ya Palmarabia! Ya!", "am_adam", "en-us", "drum"),
    ("Persicatia! Meow! Meow!", "am_adam", "en-us", "drum"), ("Pearlatar! Pearlatar!", "am_adam", "en-us", "drum"),
]
# the sixteen sides added for 48: recorded after the first names (--new), as cx_chant_voice() expects
CHANTS2 = [
    ("Silkina! Jia you! Jia you! Silkina!", "am_adam", "en-us", "drum"), ("Hyggemark! Hyggemark! Hyggemark!", "am_adam", "da", "clap"),
    ("Hopp Fonduzerland! Hopp! Hopp!", "am_adam", "de", "clap"), ("Pieroland! Pieroland! Gola! Gola!", "am_adam", "pl", "clap"),
    ("Kajmakia! Kajmakia! Kajmakia!", "am_adam", "sr", "drum"), ("Delightiye! Delightiye! Hey! Hey! Hey!", "am_adam", "tr", "drum"),
    ("Tartanland! Tartanland! Tartanland!", "bm_george", "en-gb", None), ("Do toho Crystalia! Do toho!", "am_adam", "cs", "clap"),
    ("Rosegaria! Rosegaria! Rosegaria!", "am_adam", "bg", "clap"), ("Fetallas! Fetallas! Ole! Ole!", "am_adam", "el", "clap"),
    ("Con! Con! Con! Do! Do! Do! Condoria!", "em_alex", "es", "drum"), ("Arriba Llamaru! Arriba!", "em_alex", "es", "drum"),
    ("Vamos Harpaguay! Vamos!", "em_alex", "es", "drum"), ("Si se puede! Equatoria! Si se puede!", "em_alex", "es", "drum"),
    ("Allez les Elephantoire! Allez!", "em_alex", "fr-fr", "drum"), ("Un, deux, trois! Fennecia! Un, deux, trois!", "em_alex", "fr-fr", "drum"),
]


def resample(x, ratio):
    n = int(len(x) / ratio)
    return np.interp(np.arange(n) * ratio, np.arange(len(x)), x)


def onepole(x, cutoff, rate):
    a = 1.0 - np.exp(-2 * np.pi * cutoff / rate)
    y = np.empty_like(x)
    acc = 0.0
    for i, v in enumerate(x):
        acc += a * (v - acc)
        y[i] = acc
    return y


def tannoy(x, rate):
    x = x - onepole(x, 260, rate)
    x = onepole(x, 3800, rate)
    x = np.tanh(x * 3.0)
    for ms, g in ((110, 0.32), (230, 0.18), (360, 0.09)):
        d = int(rate * ms / 1000)
        y = np.concatenate((x, np.zeros(d)))
        y[d:] += x * g
        x = y
    return x


def crowd(k, text, voice, lang, rng, beat):
    """Thousands of voices, from one: copies a little apart in pitch and time, the terrace's
    roar under them, and the drums or claps that side's fans bring."""
    s, r = k.create(text, voice=voice, speed=0.88, lang=lang)
    x = trim(np.asarray(s, np.float64), r)
    n = int(len(x) / (r / RATE)) + RATE // 2
    mix = np.zeros(n)
    for _ in range(24):
        y = resample(x, r / RATE * rng.uniform(0.84, 1.12))
        at = rng.integers(0, RATE // 7)
        m = min(len(y), n - at)
        mix[at:at + m] += y[:m] * rng.uniform(0.5, 1.0)
    mix = onepole(mix, 3200, RATE) - onepole(mix, 180, RATE)
    mix = mix / max(1e-6, np.abs(mix).max())
    noise = onepole(rng.standard_normal(n), 2000, RATE) * 0.1
    out = mix + noise
    t = np.arange(n) / RATE
    if beat in ("clap", "korea"):
        # hand claps: a burst of noise on the beat; Korea's five-beat answer after the words
        hits = (0.0, 0.5, 1.0, 1.5, 2.0, 2.5) if beat == "clap" else (1.3, 1.5, 1.7, 2.05, 2.25)
        for h in hits:
            i0 = int(h * RATE)
            if i0 >= n:
                continue
            m = min(int(0.06 * RATE), n - i0)
            out[i0:i0 + m] += rng.standard_normal(m) * np.exp(-np.arange(m) / (0.012 * RATE)) * 0.55
    if beat == "drum":
        # the big drum, on the beat
        for h in np.arange(0.0, n / RATE, 0.42):
            i0 = int(h * RATE)
            m = min(int(0.18 * RATE), n - i0)
            tt = np.arange(m) / RATE
            out[i0:i0 + m] += np.sin(2 * np.pi * (70 + 60 * np.exp(-tt / 0.02)) * tt) * np.exp(-tt / 0.06) * 0.7
    fade = np.minimum(1.0, (n / RATE - t) / 0.3)
    return np.tanh(out * 1.3) * np.minimum(t / 0.05, 1.0) * fade


def save(path, x, peak=0.9):
    x = np.asarray(x, np.float64)
    x = x / max(1e-6, np.abs(x).max()) * peak
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes((x * 32767).astype('<i2').tobytes())


def trim(x, rate):
    e = np.abs(x)
    keep = np.nonzero(e > e.max() * 0.02)[0]
    return x[max(0, keep[0] - rate // 100):keep[-1] + rate // 12]


# the commentator on the play (sample files 80 on): the game says them at the moments
COMMENTARY = [("Shot at goal!", 1.1), ("Great passing!", 1.1), ("Up for the header!", 1.1), ("What a strike!", 1.15),
              ("A long ball forward!", 1.1), ("Through ball!", 1.15), ("One two!", 1.15), ("Great tackle!", 1.15),
              ("He's through!", 1.2), ("Danger here!", 1.15)]


def whistling(rng, seconds=1.6):
    """A stand whistling: a hundred two-finger whistles, each its own pitch and length,
    over the crowd's noise."""
    n = int(RATE * seconds)
    t = np.arange(n) / RATE
    x = np.zeros(n)
    for _ in range(90):
        f = rng.uniform(1900, 3400)
        start = rng.uniform(0, seconds * 0.5)
        dur = rng.uniform(0.3, 1.1)
        vib = rng.uniform(4, 9)
        env = np.clip((t - start) / 0.04, 0, 1) * np.clip((start + dur - t) / 0.08, 0, 1)
        x += env * np.sin(2 * np.pi * f * t + 3 * np.sin(2 * np.pi * vib * t)) * rng.uniform(0.3, 1.0)
    x += rng.normal(0, 0.25, n) * np.clip(1 - t / seconds, 0.2, 1)
    return x / max(1e-6, np.abs(x).max())


def say_name(k, name):
    """A man's shirt name, called the way a commentator calls a man on the ball: a
    little lift, quick, through the same tannoy as the rest of his lines."""
    c, r = k.create(name.capitalize() + "!", voice=ANN, speed=1.25, lang="en-us")
    x = resample(trim(np.asarray(c, np.float64), r), r / RATE * 1.06)
    # the tannoy's band and drive, but one short slap of echo: a name is said many
    # times a match and the V ROM holds every one of them
    x = x - onepole(x, 260, RATE)
    x = np.tanh(onepole(x, 3800, RATE) * 3.0)
    d = int(RATE * 0.07)
    y = np.concatenate((x, np.zeros(d)))
    y[d:] += x * 0.2
    return y


def names(k):
    """The first names, after the commentator's calls: file NAME_FIRST + i for
    nations.VOICE_V1[i], as scenes/cx_audio.c expects (the rest: new())."""
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import nations
    for i, name in enumerate(nations.VOICE_V1):
        save(OUT / "in_wav_a" / f"{NAME_FIRST + i}.wav", say_name(k, name))
    return len(nations.VOICE_V1)


NAME_FIRST = 90       # after the ten commentary calls (80-89)


def new(k):
    """The 48 sides' additions, after the first names: their chants, then the names they
    brought (nations.voice_names() past the first recordings)."""
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import nations
    first = NAME_FIRST + len(nations.VOICE_V1)
    assert len(CHANTS2) == len(nations.NATIONS) - nations.CHANTS_V1
    rng = np.random.default_rng(4848)
    for i, (text, voice, lang, beat) in enumerate(CHANTS2):
        save(OUT / "in_wav_a" / f"{first + i}.wav", crowd(k, text, voice, lang, rng, beat), peak=0.72)
    names_ = nations.voice_names()[len(nations.VOICE_V1):]
    for j, name in enumerate(names_):
        save(OUT / "in_wav_a" / f"{first + len(CHANTS2) + j}.wav", say_name(k, name))
    return len(CHANTS2), len(names_)


def main():
    base = sys.argv[1]
    k = Kokoro(f"{base}/kokoro-v1.0.onnx", f"{base}/voices-v1.0.bin")
    if "--names" in sys.argv[2:]:
        print("wrote", names(k), "names")
        return
    if "--new" in sys.argv[2:]:
        print("wrote %d chants and %d names" % new(k))
        return
    for i, (text, voice, speed, pitch, pa) in enumerate(LINES):
        s, r = k.create(text, voice=voice, speed=speed, lang="en-us")
        x = trim(np.asarray(s, np.float64), r)
        x = resample(x, r / RATE * pitch)
        if pa:
            x = tannoy(x, RATE)
        else:
            x = np.tanh(x / max(1e-6, np.abs(x).max()) * 1.6)
        save(OUT / "in_wav_a" / f"{FIRST + i}.wav", x)
    # the terraces: each nation's own chant, sung by a crowd of slightly different voices
    rng = np.random.default_rng(787)
    for i, (text, voice, lang, beat) in enumerate(CHANTS):
        x = crowd(k, text, voice, lang, rng, beat)
        save(OUT / "in_wav_a" / f"{FIRST_CHANT + i}.wav", x, peak=0.72)
    for i, (text, voice, lang) in enumerate((("Oh lay, oh lay, oh lay, oh lay!", "em_alex", "es"), ("Sushipon! Sushipon! Sushipon!", "jm_kumo", "en-us"))):
        save(OUT / "in_wav_a" / f"{FIRST + len(LINES) + i}.wav", crowd(k, text, voice, lang, rng, None), peak=0.7)
    # the crowd's other voices, after the chants: a boo for a dive or a nasty foul, and
    # the high whistling of a stand that thinks the referee or the other side is wrong
    nxt = FIRST_CHANT + len(CHANTS)
    save(OUT / "in_wav_a" / f"{nxt}.wav", crowd(k, "Booooo! Booooo!", "am_adam", "en-us", rng, None), peak=0.7)
    save(OUT / "in_wav_a" / f"{nxt + 1}.wav", whistling(rng), peak=0.6)
    # the commentator's calls on the play itself, after the crowd's sounds
    for j, (text, speed) in enumerate(COMMENTARY):
        c, r = k.create(text, voice=ANN, speed=speed, lang="en-us")
        x = trim(np.asarray(c, np.float64), r)
        x = tannoy(resample(x, r / RATE * 1.06), RATE)
        save(OUT / "in_wav_a" / f"{nxt + 2 + j}.wav", x)
    assert nxt + 2 + len(COMMENTARY) == NAME_FIRST
    print("wrote", len(LINES), "lines,", len(CHANTS) + 4, "crowd sounds and", names(k), "names")


if __name__ == "__main__":
    main()
