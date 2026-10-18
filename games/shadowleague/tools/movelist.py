"""Write every fighter's move list, from the same move data the cartridge is cooked from.

    python3 games/shadowleague/tools/movelist.py [OUTPUT]   (default games/shadowleague/MOVES.txt)
"""
import json
import sys
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
GAME = TOOLS.parent
sys.path.insert(0, str(TOOLS))
from fighter_cook import complete_moves  # noqa: E402

MOTION = {
    "qcf": "D, DF, F", "qcb": "D, DB, B", "dp": "F, D, DF", "rdp": "B, D, DB",
    "hcf": "B, DB, D, DF, F", "hcb": "F, DF, D, DB, B",
    "dqcf": "D, DF, F, D, DF, F", "dqcb": "D, DB, B, D, DB, B",
    "charge_bf": "hold B, then F", "charge_du": "hold D, then U",
    "360": "full circle", "720": "two full circles",
    "fwd": "F", "back": "B", "down_fwd": "DF", "dash": "F, F", "backdash": "B, B",
}
STANCE = {"stand": "standing", "crouch": "crouching", "air": "jumping"}

HEADER = """SHADOW LEAGUE FIGHTING - MOVE LIST
===================================

Controls: A light punch, B light kick, C heavy punch, D heavy kick.
Directions are for a fighter facing right: F forward, B back, U up,
D down, DF down-forward, DB down-back. Turn them round when facing left.
"A or C" means either button: the heavy one is stronger, the light one faster.

EVERY FIGHTER
  Walk / guard ...... F / B (hold B or DB to guard high or low)
  Run / back step ... F, F (hold F to keep running) / B, B
  Jump / short hop .. U, UF or UB / tap U briefly
  Long jump ......... UF out of a run
  Throw ............. close to the opponent, F or B + C or D
  Throw escape ...... C or D just as you are grabbed
  Evade roll ........ A + B (B + A + B rolls backwards)
  Blowback .......... C + D, a knockdown strike
  Guard cancel ...... while guarding: A + B rolls away (1 power stock),
                      C + D strikes back (half the spirit gauge)
  MAX power mode .... B + C (or A + B + C) with 1 power stock: for 12 seconds
                      hits do a quarter more damage, special moves cancel into
                      other specials and supers, and one super costs nothing
                      and hits half as hard again (that ends the mode)
  Super moves ....... cost 1 power stock (the gauge fills as you hit and get hit)

Buttons pressed together may land a frame or two apart. A special's
motion may be entered at an easy pace; finish it with the button.
Light normals chain into each other; most close normals cancel into
special and super moves when they connect.
"""


def buttons(group):
    names = sorted({m.get("buttons", "") for m in group}, key=lambda b: (len(b), b))
    return " or ".join(" + ".join(b) for b in names)


def notes(data, move):
    out = []
    level = move.get("level", "mid")
    if level in ("low", "overhead"):
        out.append(level)
    if "knockdown" in move.get("on_hit", "") or "launch" in move.get("on_hit", ""):
        out.append("knockdown")
    if any(len(f) > 2 and "invuln" in f[2] for f in data["anims"].get(move["anim"], [])[:3]):
        out.append("invincible start")
    if move.get("projectile"):
        out.append("projectile")
    if not move.get("damage") and move["kind"] == "special":
        out.append("counter stance")
    return f"  ({', '.join(out)})" if out else ""


def grouped(moves, kind):
    order, groups = [], {}
    for m in moves:
        if m["kind"] != kind:
            continue
        key = (m["name"], m.get("motion", "none"), m.get("stance", "stand"))
        if key not in groups:
            order.append(key)
            groups[key] = []
        groups[key].append(m)
    return [(key, groups[key]) for key in order]


def fighter_text(data, playable):
    moves = complete_moves(data)
    lines = [f"{data['name']}  -  {data.get('origin', '')}, {data.get('style', '')}",
             "  " + ("Playable" if playable else "Locked: shown on the select screen once the painted set is in")]

    def row(label, text):
        lines.append(f"    {label} {'.' * max(2, 30 - len(label))} {text}")

    supers = grouped(moves, "super")
    specials = grouped(moves, "special")
    if specials:
        lines.append("  Special moves")
        for (name, motion, _), group in specials:
            if not any(m.get("buttons") for m in group):
                continue                      # a counter's follow-up, not entered directly
            row(name, f"{MOTION.get(motion, motion)} + {buttons(group)}{notes(data, group[0])}")
    if supers:
        lines.append("  Super moves (1 power stock, or free in MAX mode)")
        for (name, motion, _), group in supers:
            row(name, f"{MOTION.get(motion, motion)} + {buttons(group)}{notes(data, group[0])}")
    commands = [g for g in grouped(moves, "command")]
    throws = grouped(moves, "throw")
    if commands or throws:
        lines.append("  Command moves and throws")
        for (name, motion, _), group in commands:
            lead = f"{MOTION[motion]} + " if motion != "none" else ""
            row(name, f"{lead}{buttons(group)}{notes(data, group[0])}")
        seen = set()
        for (name, _, _), group in throws:
            if name in seen:
                continue
            seen.add(name)
            row(name, "close, F or B + C or D")
    lines.append("  Normal attacks")
    for stance in ("stand", "crouch", "air"):
        parts = []
        for b in "ABCD":
            m = next((m for m in moves if m["kind"] == "normal" and m.get("buttons") == b and
                      m.get("stance", "stand").split("|")[0] == stance), None)
            if m:
                tag = " (low)" if m.get("level") == "low" else ""
                parts.append(f"{b} {m['name'].title()}{tag}")
        if parts:
            lines.append(f"    {STANCE[stance]:<10} " + " / ".join(parts))
    return "\n".join(lines)


def main():
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else GAME / "MOVES.txt"
    fighters = [json.loads(p.read_text()) for p in sorted((GAME / "fighters").glob("*.json"))]
    playable = {d["id"]: (GAME / "assets/authored/fighters" / d["id"] / "manifest.json").exists() for d in fighters}
    fighters.sort(key=lambda d: (not playable[d["id"]], d["name"]))
    body = [HEADER]
    count = sum(playable.values())
    body.append(f"FIGHTERS ({count} of {len(fighters)} playable now)\n" + "-" * 40 + "\n")
    for d in fighters:
        body.append(fighter_text(d, playable[d["id"]]) + "\n")
    out.write_text("\n".join(body).replace("\n", "\r\n"), encoding="ascii", newline="")
    print(f"{out}: {len(fighters)} fighters, {count} playable")


if __name__ == "__main__":
    main()
