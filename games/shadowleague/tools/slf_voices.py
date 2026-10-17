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
NUM_SFX = 16          # sound.SFX, which comes first in the bank

ANNOUNCER_SPEAKER = 334
ANNOUNCER = [("round1", "Round one!"), ("round2", "Round two!"), ("round3", "Round three!"),
             ("final", "Final round!"), ("fight", "Fight!"), ("ko", "K.O.!"), ("timeup", "Time up!"),
             ("perfect", "Perfect!"), ("youwin", "You win!"), ("draw", "Draw game!")]

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
    return out


def load_fighters():
    return [json.loads(p.read_text()) for p in sorted((GAME / "fighters").glob("*.json"))]


def move_samples(data, entries):
    """Sample number for each of the fighter's moves (0: silent), by move name.
    A follow-up (KOKORO UKE STRIKE) shares its stance's shout."""
    names = {e["move"]: e["number"] for e in entries if e.get("fighter") == data["id"]}
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
