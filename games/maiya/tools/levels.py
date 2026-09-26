#!/usr/bin/env python3
"""Build Maiya's stage tables from games/maiya/levels/*.json.

    python3 games/maiya/tools/levels.py          (the build runs it)
    python3 games/maiya/tools/levels.py --check  (validate only)

Each stage is one JSON file in games/maiya/levels/, taken in file-name order
(01_..., 02_...): the first file is mission 1. The script checks every file
and writes scenes/maiya_levels_data.h, the initialisers maiya_levels.h and
maiya_game.c build their tables from. The header is generated: edit the
JSON, not the header. The format is described in docs/levels.md.

A mistake is reported as file, field and entry, and nothing is written.
"""

import argparse
import json
import re
import sys
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
LEVELS = GAME / "levels"
OUT = GAME / "scenes" / "maiya_levels_data.h"

WHO = ("elder", "maiden", "spirit", "sunboy")   # rescue and villager art, in order
SECRET = ("rose", "gem", "chest")


class LevelError(Exception):
    pass


def defines(path, prefix):
    """The names (lowercase, prefix dropped) a header #defines or enumerates."""
    if not path.is_file():
        return None
    text = path.read_text(encoding="utf-8")
    names = re.findall(r"#define\s+" + prefix + r"(\w+)\s", text)
    names += re.findall(r"\b" + prefix + r"(\w+)\s*=", text)
    return {n.lower() for n in names}


def caps():
    """The table sizes maiya_levels.h declares."""
    text = (GAME / "scenes" / "maiya_levels.h").read_text(encoding="utf-8")
    found = dict(re.findall(r"#define\s+(MG_\w+_COUNT)\s+(\d+)", text))
    return {k: int(v) for k, v in found.items()}


class Stage:
    def __init__(self, path, names, cap):
        self.path = path
        self.names = names
        self.cap = cap
        try:
            self.data = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as e:
            raise LevelError(f"{path.name}: line {e.lineno}: {e.msg}")

    def fail(self, where, message):
        raise LevelError(f"{self.path.name}: {where}: {message}")

    def get(self, obj, key, where, kind):
        if not isinstance(obj, dict) or key not in obj:
            self.fail(where, f"missing \"{key}\"")
        value = obj[key]
        if kind is int and (not isinstance(value, int) or isinstance(value, bool)):
            self.fail(f"{where}.{key}", f"must be a whole number, not {value!r}")
        if kind is str and not isinstance(value, str):
            self.fail(f"{where}.{key}", f"must be a string, not {value!r}")
        return value

    def num(self, obj, key, where, lo, hi):
        value = self.get(obj, key, where, int)
        if not lo <= value <= hi:
            self.fail(f"{where}.{key}", f"{value} is outside {lo}..{hi}")
        return value

    def text(self, obj, key, where, longest=38):
        value = self.get(obj, key, where, str)
        if len(value) > longest:
            self.fail(f"{where}.{key}", f"{len(value)} characters, the screen takes {longest}")
        if not all(" " <= ch <= "~" for ch in value):
            self.fail(f"{where}.{key}", "only plain ASCII prints")
        return value

    def name(self, obj, key, where, kind, prefix):
        value = self.get(obj, key, where, str)
        known = self.names.get(prefix)
        if known is not None and value not in known:
            self.fail(f"{where}.{key}", f"unknown {kind} \"{value}\" (known: {', '.join(sorted(known))})")
        return f"{prefix}{value.upper()}"

    def rows(self, key, cap_name):
        rows = self.data.get(key, [])
        if not isinstance(rows, list):
            self.fail(key, "must be a list")
        if len(rows) > self.cap[cap_name]:
            self.fail(key, f"{len(rows)} entries, the table holds {self.cap[cap_name]}")
        return [(f"{key}[{i}]", row) for i, row in enumerate(rows)]

    def build(self):
        d = self.data
        s = {}
        s["name"] = self.text(d, "name", "stage", 30)
        s["width"] = self.num(d, "width", "stage", 640, 32000)
        width = s["width"]
        s["background"] = self.num(d, "background", "stage", 0, 255)
        s["music"] = self.num(d, "music", "stage", 0, 255)
        s["mechanic"] = self.name(d, "mechanic", "stage", "mechanic", "MG_M_")
        s["pit"] = self.name(d, "pit", "stage", "pit", "MG_PIT_")
        s["blocks"] = self.name(d, "blocks", "stage", "ledge set", "MG_BLOCKS_")
        if self.get(d, "posted", "stage", str) == "none":
            s["posted"] = "0xFFu"
        else:
            s["posted"] = self.name(d, "posted", "stage", "enemy", "MG_E_")
        s["gate_x"] = self.num(d, "gate_x", "stage", 0, width)
        key = self.get(d, "key", "stage", dict)
        s["key"] = (self.num(key, "x", "key", 0, width), self.num(key, "y", "key", 0, 223))
        g = self.get(d, "guardian", "stage", dict)
        s["guardian"] = self.text(g, "name", "guardian", 30)
        s["boss_style"] = self.name(g, "style", "guardian", "guardian", "MG_B_")
        s["boss_hp"] = self.num(g, "hp", "guardian", 1, 255)
        s["boss_hint"] = self.text(g, "hint", "guardian")
        s["taunt"] = self.text(g, "taunt", "guardian", 40)
        s["reply"] = self.text(g, "reply", "guardian", 40)
        art = self.get(d, "art", "stage", dict)
        s["art_name"] = self.text(art, "name", "art", 30)
        s["art_words"] = self.text(art, "words", "art", 40)
        s["secret_hint"] = self.text(d, "secret_hint", "stage")
        sunboy = self.get(d, "sunboy", "stage", list)
        if len(sunboy) != 2:
            self.fail("sunboy", "needs two lines")
        s["sunboy"] = [self.text({"line": t}, "line", f"sunboy[{i}]") for i, t in enumerate(sunboy)]

        s["platforms"] = [(self.num(p, "x", w, 1, width), self.num(p, "y", w, 0, 223), self.num(p, "w", w, 16, 1024))
                          for w, p in self.rows("platforms", "MG_PLATFORM_COUNT")]
        s["encounters"] = [(self.num(e, "x", w, 1, width), self.name(e, "enemy", w, "enemy", "MG_E_"))
                           for w, e in self.rows("encounters", "MG_ENCOUNTER_COUNT")]
        s["archers"] = [(self.num(a, "x", w, 1, width), self.num(a, "y", w, 0, 223))
                        for w, a in self.rows("archers", "MG_ARCHER_COUNT")]
        s["hazards"] = [(self.num(h, "x", w, 1, width), self.num(h, "w", w, 16, 512), self.name(h, "type", w, "hazard", "MG_H_"))
                        for w, h in self.rows("hazards", "MG_HAZARD_COUNT")]
        s["rescues"] = [(self.num(r, "x", w, 1, width), self.choice(r, "who", w, WHO))
                        for w, r in self.rows("rescues", "MG_RESCUE_COUNT")]
        s["secrets"] = [(self.num(r, "x", w, 1, width), self.num(r, "y", w, 0, 223), self.choice(r, "type", w, SECRET))
                        for w, r in self.rows("secrets", "MG_SECRET_COUNT")]
        s["pickups"] = [(self.num(p, "x", w, 1, width), self.num(p, "y", w, 0, 223), self.name(p, "kind", w, "pickup", "MG_K_"))
                        for w, p in self.rows("pickups", "MG_PICK_COUNT")]
        s["decor"] = [(self.num(p, "x", w, 1, width), self.num(p, "y", w, 0, 223), self.name(p, "kind", w, "decoration", "MG_D_"))
                      for w, p in self.rows("decor", "MG_DECOR_COUNT")]
        s["vines"] = [(self.num(v, "x", w, 1, width), self.num(v, "top", w, 0, 223), self.num(v, "bottom", w, 0, 223))
                      for w, v in self.rows("vines", "MG_VINE_COUNT")]
        for w, v in self.rows("vines", "MG_VINE_COUNT"):
            if v["top"] >= v["bottom"]:
                self.fail(w, "top must be above bottom")
        s["npcs"] = [(self.num(n, "x", w, 1, width), self.choice(n, "who", w, WHO), self.text(n, "line", w))
                     for w, n in self.rows("npcs", "MG_NPC_COUNT")]
        self.rules(s)
        return s

    def rules(self, s):
        """How the game plays a stage: what it needs of the numbers."""
        width, gate = s["width"], s["gate_x"]
        if gate >= width - 320:
            self.fail("gate_x", f"{gate} leaves no arena: keep it under width - 320 ({width - 320})")
        for i, (x, y, w) in enumerate(s["platforms"]):
            if w % 16:
                self.fail(f"platforms[{i}].w", f"{w} is not a multiple of 16")
            if not 64 <= y <= 144:
                self.fail(f"platforms[{i}].y", f"{y}: a ledge is between 64 (high) and 144 (one jump up)")
            if x + w >= width:
                self.fail(f"platforms[{i}]", "runs off the end of the stage")
        xs = [x for x, _ in s["encounters"]]
        for i in range(1, len(xs)):
            if xs[i] <= xs[i - 1]:
                self.fail(f"encounters[{i}].x", f"{xs[i]}: encounters are met in x order (after {xs[i - 1]})")
        for i, (x, y) in enumerate(s["archers"]):
            if not any(py == y and px <= x <= px + pw for px, py, pw in s["platforms"]):
                self.fail(f"archers[{i}]", f"({x},{y}) stands on no ledge: y must be a ledge's y, x within it")
        for i, (x, w, kind) in enumerate(s["hazards"]):
            if w % 16 or w > 64:
                self.fail(f"hazards[{i}].w", f"{w}: a multiple of 16, at most 64")
            if x + w >= width - 320:
                self.fail(f"hazards[{i}]", "reaches into the arena (keep it under width - 320)")
        rx = [x for x, _ in s["rescues"]]
        if len(rx) != 4:
            self.fail("rescues", "needs four captives")
        if not (rx[0] < width // 2 < rx[3]) or rx != sorted(rx) or rx[3] >= gate:
            self.fail("rescues", "in x order, the first in the first half, the last in the second, all before the gate")
        for i, (x, top, bottom) in enumerate(s["vines"]):
            if bottom != 192 or bottom - top > 128:
                self.fail(f"vines[{i}]", "runs from the road (bottom 192) up at most 128 px")

    def choice(self, obj, key, where, options):
        value = self.get(obj, key, where, str)
        if value not in options:
            self.fail(f"{where}.{key}", f"\"{value}\" is not one of {', '.join(options)}")
        return options.index(value)


def c_string(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def table(name, rows):
    """A #define whose body is the brace-enclosed initialiser `rows`."""
    lines = [f"#define {name} {{"] + [f"    {row}," for row in rows] + ["}"]
    return " \\\n".join(lines) + "\n"


def tuples(items, width):
    return "{" + ",".join("{" + ",".join(str(v) for v in item) + "}" for item in items) + "}" if items else "{" + "{0}" + "}"


def render(stages, files):
    out = ["/*",
           " * Generated by games/maiya/tools/levels.py from the stage files in",
           " * games/maiya/levels/.",
           " * Do not edit: change the JSON and rebuild.",
           " */",
           "#ifndef MAIYA_LEVELS_DATA_H",
           "#define MAIYA_LEVELS_DATA_H",
           "",
           f"#define MG_LEVEL_COUNT {len(stages)}",
           ""]
    rows = []
    for f, s in zip(files, stages):
        rows.append(f"/* {f} */ {{{c_string(s['name'])}, {c_string(s['guardian'])}, {s['width']}, "
                    f"{s['background']}, {s['music']}, {s['boss_style']}, {s['boss_hp']}, {s['gate_x']}, "
                    f"{tuples(s['platforms'], 3)}, {tuples(s['encounters'], 2)}, {tuples(s['archers'], 2)}, "
                    f"{tuples(s['hazards'], 3)}, "
                    f"{{{','.join(str(x) for x, _ in s['rescues']) or '0'}}}, "
                    f"{{{','.join(str(t) for _, t in s['rescues']) or '0'}}}, "
                    f"{tuples(s['secrets'], 3)}}}")
    out.append(table("MG_LEVELS_TABLE", rows))
    out.append(table("MG_DECOR_TABLE", [tuples(s["decor"], 3) for s in stages]))
    out.append(table("MG_VINES_TABLE", [tuples(s["vines"], 3) for s in stages]))
    out.append(table("MG_NPCS_TABLE", ["{" + ",".join(f"{{{x},{t},{c_string(line)}}}" for x, t, line in s["npcs"]) + "}"
                                       if s["npcs"] else "{{0}}" for s in stages]))
    out.append(table("MG_KEY_TABLE", [f"{{{s['key'][0]}, {s['key'][1]}}}" for s in stages]))
    out.append(table("MG_ART_NAME_TABLE", [c_string(s["art_name"]) for s in stages]))
    out.append(table("MG_ART_WORDS_TABLE", [c_string(s["art_words"]) for s in stages]))
    out.append(table("MG_SECRET_HINT_TABLE", [c_string(s["secret_hint"]) for s in stages]))
    out.append(table("MG_BOSS_HINT_TABLE", [c_string(s["boss_hint"]) for s in stages]))
    out.append(table("MG_SUNBOY_TABLE", [f"{{{c_string(s['sunboy'][0])}, {c_string(s['sunboy'][1])}}}" for s in stages]))
    out.append(table("MG_PICKS_TABLE", [tuples(s["pickups"], 3) for s in stages]))
    out.append(table("MG_BOSS_TAUNT_TABLE", [c_string(s["taunt"]) for s in stages]))
    out.append(table("MG_BOSS_REPLY_TABLE", [c_string(s["reply"]) for s in stages]))
    out.append(table("MG_STAGE_MECH_TABLE", [s["mechanic"] for s in stages]))
    out.append(table("MG_PIT_TABLE", [s["pit"] for s in stages]))
    out.append(table("MG_BLOCKS_TABLE", [s["blocks"] for s in stages]))
    out.append(table("MG_POSTED_TABLE", [s["posted"] for s in stages]))
    out.append("#endif")
    return "\n".join(out) + "\n"


def load():
    files = sorted(LEVELS.glob("*.json"))
    if not files:
        raise LevelError(f"no stage files in {LEVELS}")
    names = {
        "MG_E_": defines(GAME / "scenes" / "maiya_levels.h", "MG_E_"),
        "MG_H_": defines(GAME / "scenes" / "maiya_levels.h", "MG_H_"),
        "MG_B_": defines(GAME / "scenes" / "maiya_levels.h", "MG_B_"),
        "MG_M_": defines(GAME / "scenes" / "maiya_game.c", "MG_M_"),
        "MG_PIT_": defines(GAME / "scenes" / "maiya_game.c", "MG_PIT_"),
        "MG_BLOCKS_": defines(GAME / "scenes" / "maiya_game.c", "MG_BLOCKS_"),
        # From the art build; before it has run the names go unchecked
        # here and the compiler checks them instead.
        "MG_D_": defines(GAME / "artbox" / "generated" / "maiya_assets.h", "MG_D_"),
        "MG_K_": defines(GAME / "artbox" / "generated" / "maiya_assets.h", "MG_K_"),
    }
    cap = caps()
    return files, [Stage(f, names, cap).build() for f in files]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="validate the stage files, write nothing")
    args = parser.parse_args()
    try:
        files, stages = load()
    except LevelError as e:
        print(f"levels: {e}", file=sys.stderr)
        return 1
    text = render(stages, [f.name for f in files])
    if args.check:
        print(f"levels: {len(stages)} stages OK")
        return 0
    if not OUT.is_file() or OUT.read_text(encoding="utf-8") != text:
        OUT.write_text(text, encoding="utf-8", newline="\n")
        print(f"levels: {len(stages)} stages -> {OUT.relative_to(GAME.parents[1])}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
