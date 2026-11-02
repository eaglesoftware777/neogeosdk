"""Shadow League's voices: who speaks, what they say, and the sample number of every line.

The effects (sound.py SFX) come first in the ADPCM-A bank, then the
announcer, then six shared voice types (attack, hurt, K.O. and power-up
grunts), then each fighter's own shouts: the names of their special moves
and their super. sound.py encodes the lines in this order; art.py emits the
tables that tell the game which sample goes with which move, so both read
the plan from here.

The recordings in assets/authored/voices/ come from voices.py (a neural
speech model with many speakers; see that file). Speakers are model ids.
"""
import json
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
VOICES = GAME / "assets/authored/voices"
NUM_SFX = 21          # sound.SFX, which comes first in the bank

ANNOUNCER_SPEAKER = 334
# The keys keep their sample numbers; the league's own words are in the texts.
ANNOUNCER = [("round1", "Round one!"), ("round2", "Round two!"), ("round3", "Round three!"),
             ("final", "Final round!"), ("fight", "Clash!"), ("ko", "Blackout!"), ("timeup", "Time up!"),
             ("perfect", "Untouched!"), ("youwin", "The night is yours!"), ("draw", "Draw game!"),
             ("sweep", "Clean sweep!"), ("legend", "Triple untouched! Legendary!")]

# Deep, middle and light men; lower and higher women; machines.
TYPE_SPEAKERS = [646, 121, 308, 863, 213, 438]
TYPE_ROBOT = [0, 0, 0, 0, 0, 1.0]
TYPE_LINES = [("attack", "Hyah!"), ("hurt", "Ugh!"), ("ko", "Aaaaaagh!"), ("power", "Haaaaaa!")]

CAST = {
    'aditya': {'speaker': 876, 'type': 2},
    'aisha': {'speaker': 475, 'type': 3},
    'aleksei': {'speaker': 851, 'type': 0},
    'amara': {'speaker': 890, 'type': 3},
    'arun': {'speaker': 132, 'type': 2},
    'atlas': {'speaker': 533, 'type': 5, 'robot': 1.0},
    'batu': {'speaker': 778, 'type': 0},
    'chenbao': {'speaker': 342, 'type': 1},
    'claire': {'speaker': 120, 'type': 4},
    'diego': {'speaker': 8, 'type': 2},
    'echo': {'speaker': 243, 'type': 5, 'robot': 1.0},
    'elena': {'speaker': 816, 'type': 3},
    'fahri': {'speaker': 736, 'type': 0},
    'hayato': {'speaker': 472, 'type': 2},
    'idris': {'speaker': 571, 'type': 1},
    'irina': {'speaker': 733, 'type': 3},
    'jack': {'speaker': 547, 'type': 1},
    'kaimana': {'speaker': 288, 'type': 0},
    'kenji': {'speaker': 182, 'type': 1},
    'kiran': {'speaker': 290, 'type': 4},
    'kofi': {'speaker': 813, 'type': 1},
    'leila': {'speaker': 387, 'type': 4},
    'linwei': {'speaker': 772, 'type': 2},
    'lotus': {'speaker': 487, 'type': 4},
    'lucas': {'speaker': 800, 'type': 1},
    'maia': {'speaker': 212, 'type': 3},
    'malik': {'speaker': 607, 'type': 0},
    'mason': {'speaker': 139, 'type': 0},
    'maya': {'speaker': 273, 'type': 4},
    'minjun': {'speaker': 477, 'type': 2},
    'mirage': {'speaker': 682, 'type': 3},
    'niran': {'speaker': 22, 'type': 2},
    'noah': {'speaker': 459, 'type': 1},
    'ryuhei': {'speaker': 867, 'type': 0},
    'sokha': {'speaker': 380, 'type': 2},
    'somchai': {'speaker': 899, 'type': 2},
    'thabo': {'speaker': 371, 'type': 1},
    'tomas': {'speaker': 400, 'type': 2},
    'tororojo': {'speaker': 479, 'type': 0},
    'viktor': {'speaker': 540, 'type': 0},
    'viper': {'speaker': 348, 'type': 4, 'robot': 0.5},
    'xiaomei': {'speaker': 675, 'type': 4},
    'zara': {'speaker': 610, 'type': 3},
    'zero': {'speaker': 812, 'type': 1},
}

# Each fighter's own shouts, in their own speaker (after every older line, so
# those keep their numbers): a light and a heavy blow, a throw, being hurt,
# a knock-out. And their win: the first line of their win quote.
SHOUT_KINDS = ("attack", "heavy", "throw", "hurt", "ko")
SHOUTS = {
    'aditya': ('Hah!', 'Harimau!', 'Jatuh!', 'Ugh!', 'Aaaargh!'),
    'aisha': ('Tah!', 'Take that!', 'Down!', 'Agh!', 'Nooo!'),
    'aleksei': ('Hup!', 'Hraaah!', 'Over you go!', 'Ngh!', 'Aaargh...'),
    'amara': ('Eh!', 'Toma!', 'Vai!', 'Ai!', 'Aaaaah!'),
    'arun': ('Hah!', 'Haiyaa!', 'Fall!', 'Uff!', 'Aaaagh!'),
    'atlas': ('Engage.', 'Overload!', 'Grapple.', 'Damage.', 'System... failure.'),
    'batu': ('Hoh!', 'Hraaah!', 'Up you go!', 'Guh!', 'Uuuurgh!'),
    'chenbao': ('Hah!', 'Hiyah!', 'Down!', 'Ugh!', 'Aaaah!'),
    'claire': ('Hop!', 'Voilà!', 'Allez!', 'Ah!', 'Non...!'),
    'diego': ('Arriba!', 'Olé!', 'Vámonos!', 'Ay!', 'Noooo!'),
    'echo': ('Copy.', 'Execute!', 'Seize.', 'Error.', 'Shutting... down.'),
    'elena': ('Hup!', 'Haaah!', 'Pinned!', 'Agh!', 'Aaaaah!'),
    'fahri': ('Hoh!', 'Haydi!', 'Over!', 'Ugh!', 'Aaargh!'),
    'hayato': ('Hm!', 'Seiyah!', 'Gone.', 'Ngh.', 'Ooh...'),
    'idris': ('Yah!', 'Yallah!', 'Down you go!', 'Ugh!', 'Aaaagh!'),
    'irina': ('Hm.', 'Breathe!', 'Relax.', 'Ah!', 'Ohhh...'),
    'jack': ('Jab!', 'Knockout!', 'Get down!', 'Oof!', 'Noooo!'),
    'kaimana': ('Hoo!', 'Chee-hoo!', 'Up!', 'Ugh!', 'Aaaargh!'),
    'kenji': ('Ei!', 'Seiya!', 'Hah!', 'Guh!', 'Uwaaagh!'),
    'kiran': ('Oi!', 'Have it!', 'Down you go, mate!', 'Ow!', 'Noooo!'),
    'kofi': ('Hah!', 'Boom!', 'Down!', 'Ugh!', 'Aaaah!'),
    'leila': ('Yah!', 'Too slow!', 'Away!', 'Ah!', 'Nooo!'),
    'linwei': ('Hah!', 'Hiyah!', 'Throw!', 'Ugh!', 'Aaaah!'),
    'lotus': ('Tsk.', 'Vanish!', 'Gone.', 'Hk!', 'Ahhh...'),
    'lucas': ('Bora!', 'Tap out!', 'Gotcha!', 'Ugh!', 'Aaargh!'),
    'maia': ('Hup!', 'Freeze!', 'Down!', 'Ah!', 'Nooo!'),
    'malik': ('Hah!', 'Lion!', 'Into the sand!', 'Ugh!', 'Aaaargh!'),
    'mason': ('Hey!', 'Eat this!', 'Get over here!', 'Argh!', 'Graaagh!'),
    'maya': ('Hah!', 'Elbow!', 'Gotcha!', 'Ah!', 'Aaaah!'),
    'minjun': ('Hap!', 'Kiyap!', 'Hah!', 'Ugh!', 'Aaaagh!'),
    'mirage': ('Here.', 'Gone!', 'Fall.', 'Ah!', 'Fading...'),
    'niran': ('Hah!', 'Naga!', 'Fall!', 'Ugh!', 'Aaaagh!'),
    'noah': ('Osu!', 'Seiken!', 'Hah!', 'Guh!', 'Aaargh!'),
    'ryuhei': ('Hah!', 'Ippon!', 'Seoi!', 'Ugh!', 'Uwaaah!'),
    'sokha': ('Hah!', 'Elbow!', 'Down!', 'Ugh!', 'Aaaagh!'),
    'somchai': ('Hah!', 'Ha-ya!', 'Down!', 'Ugh!', 'Aaaargh!'),
    'thabo': ('Jab!', 'Lightning!', 'Down!', 'Oof!', 'Aaaagh!'),
    'tomas': ('Olé!', 'Toro!', 'Fuera!', 'Ay!', 'Noooo!'),
    'tororojo': ('Toro!', 'Rojo!', 'Body slam!', 'Gah!', 'Graaaagh!'),
    'viktor': ('Hup!', 'Hraaah!', 'Down!', 'Ngh!', 'Aaaargh!'),
    'viper': ('Fast.', 'Flash!', 'Caught.', 'Hit.', 'Offline...'),
    'xiaomei': ('Ha!', 'Hai!', 'Down!', 'Ah!', 'Aaaah!'),
    'zara': ('Hah!', 'Shabash!', 'To the clay!', 'Ugh!', 'Aaaagh!'),
    'zero': ('Hn.', 'Overload!', 'Gone.', 'Gh!', 'Nooo...'),
}
FIGHTER_VOICE_KINDS = ("win",) + SHOUT_KINDS


def fighter_lines(data):
    """(key, text, move names) for each special (up to three) and the first super."""
    lines, specials = [], []
    for m in data["moves"]:
        if m["kind"] == "special" and m.get("buttons") and m["name"] not in specials:
            specials.append(m["name"])
    for n, name in enumerate(specials[:3], 1):
        lines.append((f"sp{n}", name.title() + "!", name))
    supers = [m["name"] for m in data["moves"] if m["kind"] == "super"]
    if supers:
        lines.append(("super", supers[0].title() + "!", supers[0]))
    return lines


def plan(fighter_data):
    """Every voice line in bank order: dicts with number, key, text, speaker, robot."""
    out, number = [], NUM_SFX + 1
    for key, text in ANNOUNCER:
        out.append({"number": number, "key": f"ann_{key}", "text": text, "speaker": ANNOUNCER_SPEAKER,
                    "robot": 0, "announcer": True})
        number += 1
    for t, spk in enumerate(TYPE_SPEAKERS):
        for key, text in TYPE_LINES:
            out.append({"number": number, "key": f"type{t}_{key}", "text": text, "speaker": spk,
                        "robot": TYPE_ROBOT[t]})
            number += 1
    for data in sorted(fighter_data, key=lambda d: d["id"]):
        cast = CAST[data["id"]]
        for key, text, move in fighter_lines(data):
            out.append({"number": number, "key": f"{data['id']}_{key}", "text": text, "move": move,
                        "fighter": data["id"], "speaker": cast["speaker"], "robot": cast.get("robot", 0)})
            number += 1
    quotes = json.loads((GAME / "assets/authored/win_quotes.json").read_text())
    for data in sorted(fighter_data, key=lambda d: d["id"]):
        fid, cast = data["id"], CAST[data["id"]]
        texts = (win_text(quotes.get(fid)),) + tuple(SHOUTS[fid])
        for kind, text in zip(FIGHTER_VOICE_KINDS, texts):
            out.append({"number": number, "key": f"{fid}_v_{kind}", "text": text, "fighter": fid,
                        "voice": kind, "speaker": cast["speaker"], "robot": cast.get("robot", 0)})
            number += 1
    return out


def win_text(quote):
    """The spoken win: the quote's first line, as a sentence -- a line that
    stops mid-sentence takes the next line's words up to the sentence's end."""
    lines = [l.strip() for l in (quote or ["Victory."])]
    line = lines[0]
    if line[-1:] not in ".!?" and len(lines) > 1:
        rest = lines[1]
        cut = min((rest.find(c) for c in ".!?" if c in rest), default=-1)
        line = f"{line} {rest[:cut + 1] if cut >= 0 else rest}"
    if line.isupper():
        line = line.capitalize()
    return line if line[-1:] in ".!?" else line + "."


def fighter_voices(fighter_id, entries):
    """Sample numbers of a fighter's win, attack, heavy, throw, hurt and K.O. lines."""
    own = {e["voice"]: e["number"] for e in entries if e.get("fighter") == fighter_id and e.get("voice")}
    return [own[k] for k in FIGHTER_VOICE_KINDS]


def load_fighters():
    return [json.loads(p.read_text()) for p in sorted((GAME / "fighters").glob("*.json"))]


def move_samples(data, entries):
    """Sample number for each of the fighter's moves (0: silent), by move name.
    A follow-up (KOKORO UKE STRIKE) shares its stance's shout."""
    names = {e["move"]: e["number"] for e in entries if e.get("fighter") == data["id"] and e.get("move")}
    out = []
    for m in data["moves"]:
        n = names.get(m["name"], 0)
        if not n and m["kind"] == "special":
            n = next((v for k, v in names.items() if m["name"].startswith(k)), 0)
        out.append(n)
    return out


def type_base(entries):
    return next(e["number"] for e in entries if e["key"] == "type0_attack")


def announcer_base(entries):
    return next(e["number"] for e in entries if e["key"] == "ann_round1")
