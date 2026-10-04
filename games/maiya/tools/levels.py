#!/usr/bin/env python3
"""Build Maiya's stage tables from games/maiya/levels/*.json.

    python3 games/maiya/tools/levels.py          (the build runs it)
    python3 games/maiya/tools/levels.py --check  (validate only)
    python3 games/maiya/tools/levels.py --lint   (validate, then warn about design)

Each stage is one JSON file in games/maiya/levels/, taken in file-name order
(01_..., 02_...): the first file is mission 1. The script checks every file
and writes scenes/maiya_levels_data.h, the initialisers maiya_levels.h and
maiya_game.c build their tables from. The header is generated: edit the
JSON, not the header. The format is described in docs/levels.md.

A mistake is reported as file, field and entry, and nothing is written.
--lint goes on to what plays badly without breaking the game (see lint()):
--strict makes a warning fail too.
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
        s["climb"] = self.name(d, "climb", "stage", "decoration", "MG_D_")
        # How far above the screen the valley goes (a tall climb to an upper
        # tier); 0 for a valley all on one screen's height.
        s["upper"] = self.num(d, "upper", "stage", 0, 256) if "upper" in d else 0
        # What the gate asks for besides the key: "drums", every poison drum
        # on the road shut off (the works). Most stages ask nothing more.
        s["objective"] = self.choice(d, "objective", "stage", OBJECTIVES) if "objective" in d else 0
        top = -s["upper"]
        if self.get(d, "posted", "stage", str) == "none":
            s["posted"] = "0xFFu"
        else:
            s["posted"] = self.name(d, "posted", "stage", "enemy", "MG_E_")
        s["gate_x"] = self.num(d, "gate_x", "stage", 0, width)
        # What the gate stands in: a great tree, a cliff, a mountain... (a
        # stage with no gate has none).
        s["landmark"] = self.name(d, "landmark", "stage", "landmark", "MG_LM_") if s["gate_x"] else "0xFFu"
        key = self.get(d, "key", "stage", dict)
        s["key"] = (self.num(key, "x", "key", 0, width), self.num(key, "y", "key", top, 223))
        hide = self.get(d, "hideout", "stage", dict)
        s["hideout"] = (self.num(hide, "x", "hideout", 0, width), self.num(hide, "y", "hideout", top + 16, 192),
                        self.text(hide, "hint", "hideout"))
        g = self.get(d, "guardian", "stage", dict)
        s["guardian"] = self.text(g, "name", "guardian", 30)
        s["boss_style"] = self.name(g, "style", "guardian", "guardian", "MG_B_")
        s["boss_hp"] = self.num(g, "hp", "guardian", 1, 255)
        s["boss_hint"] = self.text(g, "hint", "guardian")
        s["taunt"] = self.text(g, "taunt", "guardian", 40)
        s["reply"] = self.text(g, "reply", "guardian", 40)
        art = self.get(d, "art", "stage", dict)
        s["art_kind"] = self.choice(art, "kind", "art", ART_KINDS)
        s["art_name"] = self.text(art, "name", "art", 30)
        s["art_words"] = self.text(art, "words", "art", 40)
        s["secret_hint"] = self.text(d, "secret_hint", "stage")
        sunboy = self.get(d, "sunboy", "stage", list)
        if len(sunboy) != 2:
            self.fail("sunboy", "needs two lines")
        s["sunboy"] = [self.text({"line": t}, "line", f"sunboy[{i}]") for i, t in enumerate(sunboy)]
        # What her work healed (the elder says it after the guardian falls),
        # and the elder's briefing before the stage: its pollution, its foe.
        for key in ("healed", "briefing"):
            lines = self.get(d, key, "stage", list)
            if len(lines) != 2:
                self.fail(key, "needs two lines")
            s[key] = [self.text({"line": t}, "line", f"{key}[{i}]") for i, t in enumerate(lines)]

        s["platforms"] = [(self.num(p, "x", w, 1, width), self.num(p, "y", w, top, 223), self.num(p, "w", w, 16, 1024))
                          for w, p in self.rows("platforms", "MG_PLATFORM_COUNT")]
        s["rotten"] = 0
        for i, (w, p) in enumerate(self.rows("platforms", "MG_PLATFORM_COUNT")):
            if p.get("rotten", False) not in (True, False):
                self.fail(f"{w}.rotten", "is true or false")
            if p.get("rotten", False):
                s["rotten"] |= 1 << i
        s["encounters"] = [(self.num(e, "x", w, 1, width), self.name(e, "enemy", w, "enemy", "MG_E_"))
                           for w, e in self.rows("encounters", "MG_ENCOUNTER_COUNT")]
        s["archers"] = [(self.num(a, "x", w, 1, width), self.num(a, "y", w, 0, 223))
                        for w, a in self.rows("archers", "MG_ARCHER_COUNT")]
        s["hazards"] = [(self.num(h, "x", w, 1, width), self.num(h, "w", w, 16, 512), self.name(h, "type", w, "hazard", "MG_H_"))
                        for w, h in self.rows("hazards", "MG_HAZARD_COUNT")]
        s["rescues"] = [(self.num(r, "x", w, 1, width), self.choice(r, "who", w, WHO))
                        for w, r in self.rows("rescues", "MG_RESCUE_COUNT")]
        s["secrets"] = [(self.num(r, "x", w, 1, width), self.num(r, "y", w, top, 223), self.choice(r, "type", w, SECRET))
                        for w, r in self.rows("secrets", "MG_SECRET_COUNT")]
        s["pickups"] = [(self.num(p, "x", w, 1, width), self.num(p, "y", w, top, 223), self.name(p, "kind", w, "pickup", "MG_K_"))
                        for w, p in self.rows("pickups", "MG_PICK_COUNT")]
        s["decor"] = [(self.num(p, "x", w, 1, width), self.num(p, "y", w, 0, 223), self.name(p, "kind", w, "decoration", "MG_D_"))
                      for w, p in self.rows("decor", "MG_DECOR_COUNT")]
        s["vines"] = [(self.num(v, "x", w, 1, width), self.num(v, "top", w, top, 223), self.num(v, "bottom", w, 0, 223))
                      for w, v in self.rows("vines", "MG_VINE_COUNT")]
        for w, v in self.rows("vines", "MG_VINE_COUNT"):
            if v["top"] >= v["bottom"]:
                self.fail(w, "top must be above bottom")
        s["npcs"] = [(self.num(n, "x", w, 1, width), self.choice(n, "who", w, WHO), self.text(n, "line", w))
                     for w, n in self.rows("npcs", "MG_NPC_COUNT")]
        # The spawn script: a flight of creatures when the view's right edge
        # reaches x, flying the named formation at height y.
        # The guardians she must beat again before the stage's own (a boss
        # rush), in order.
        rush = d.get("rush", [])
        if not isinstance(rush, list) or len(rush) > self.cap["MG_RUSH_COUNT"]:
            self.fail("rush", f"a list of at most {self.cap['MG_RUSH_COUNT']} guardian styles")
        s["rush"] = [self.name({"style": r}, "style", f"rush[{i}]", "guardian", "MG_B_") for i, r in enumerate(rush)]
        s["waves"] = [(self.num(v, "x", w, 1, width), self.name(v, "enemy", w, "enemy", "MG_E_"),
                       self.name(v, "form", w, "formation", "MG_FORM_"), self.num(v, "count", w, 1, 6),
                       self.num(v, "y", w, 16, 208))
                      for w, v in self.rows("waves", "MG_WAVE_COUNT")]
        self.rules(s)
        return s

    def rules(self, s):
        """How the game plays a stage: what it needs of the numbers."""
        width, gate = s["width"], s["gate_x"]
        flight = s["mechanic"] == "MG_M_FLIGHT"
        wx = [x for x, *_ in s["waves"]]
        for i in range(1, len(wx)):
            if wx[i] < wx[i - 1]:
                self.fail(f"waves[{i}].x", f"{wx[i]}: waves come in x order (after {wx[i - 1]})")
        if gate >= width - 320:
            self.fail("gate_x", f"{gate} leaves no arena: keep it under width - 320 ({width - 320})")
        if gate:
            # The gate is the last thing in the valley: its landmark stands
            # from gate - 80, the view ends with it, and nothing may reach
            # into it or lie past it; the road before it is clear of hazards.
            edge = gate - 80
            for key in ("platforms", "encounters", "archers", "rescues", "secrets", "pickups", "decor", "vines", "npcs"):
                for i, r in enumerate(self.data.get(key, [])):
                    end = r["x"] + r.get("w", 32 if key in ("pickups", "secrets", "decor", "vines") else 0)
                    if end > edge:
                        self.fail(f"{key}[{i}]", f"reaches x {end}, past {edge}: the gate ({gate}) is the "
                                  "valley's last thing, and its landmark stands from gate - 80")
            for i, r in enumerate(self.data.get("hazards", [])):
                if r["x"] + r["w"] > gate - 96:
                    self.fail(f"hazards[{i}]", f"ends at {r['x'] + r['w']}: keep the last 96 px before the gate clear")
            for key in ("key", "hideout"):
                if self.data[key]["x"] > edge:
                    self.fail(f"{key}.x", f"{self.data[key]['x']} is past {edge}, in the gate's landmark")
        for i, (x, y, w) in enumerate(s["platforms"]):
            if w % 16:
                self.fail(f"platforms[{i}].w", f"{w} is not a multiple of 16")
            # (The view rises at most `upper`: she stands 64 px down it on the top ledge.)
            if not (64 <= y <= 144 or (s["upper"] and -s["upper"] + 64 <= y < 64)):
                self.fail(f"platforms[{i}].y", f"{y}: a ledge is between 64 (high) and 144 (one jump up)"
                          + (f", or in the upper tier from {-s['upper'] + 64}" if s["upper"] else ""))
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
        if s["objective"] == OBJECTIVES.index("drums"):
            drums = [x for x, _, kind in s["hazards"] if kind == "MG_H_TOXIC"]
            if len(drums) != 3:
                self.fail("hazards", f"the drums objective needs three toxic drums, not {len(drums)}")
        if flight:
            # On the wing there is no road: no captives, key, gate, hideout
            # or climbs, and the guardian meets her at the end of the sky.
            for field in ("rescues", "vines", "archers", "hazards"):
                if s[field]:
                    self.fail(field, "a flight stage has none")
            if gate:
                self.fail("gate_x", "a flight stage has no gate (0)")
            return
        rx = [x for x, _ in s["rescues"]]
        if len(rx) != 4:
            self.fail("rescues", "needs four captives")
        if not (rx[0] < width // 2 < rx[3]) or rx != sorted(rx) or rx[3] >= gate:
            self.fail("rescues", "in x order, the first in the first half, the last in the second, all before the gate")
        hx, hy, _ = s["hideout"]
        if hy != 192 and not any(py == hy and px <= hx <= px + pw for px, py, pw in s["platforms"]):
            self.fail("hideout", f"({hx},{hy}) is on no ledge: y must be a ledge's y (or 192, the road), x within it")
        kx, ky = s["key"]
        if not any(py - 40 <= ky <= py - 16 and px - 16 <= kx <= px + pw for px, py, pw in s["platforms"]):
            self.fail("key", f"({kx},{ky}) is over no shelf: it should hang just above a ledge")
        for i, (x, top, bottom) in enumerate(s["vines"]):
            if not any(py == top and px <= x + 16 <= px + pw for px, py, pw in s["platforms"]):
                self.fail(f"vines[{i}]", f"its top ({x},{top}) meets no ledge")
            if bottom != 192 or bottom - top > (256 if s["upper"] else 128):
                self.fail(f"vines[{i}]", f"runs from the road (bottom 192) up at most {256 if s['upper'] else 128} px")

    def choice(self, obj, key, where, options):
        value = self.get(obj, key, where, str)
        if value not in options:
            self.fail(f"{where}.{key}", f"\"{value}\" is not one of {', '.join(options)}")
        return options.index(value)


# What a gate can ask for besides the key, in the order of MG_OBJ_* (maiya_game.c).
OBJECTIVES = ("none", "drums")

# The Secret Arts, in the order of the game's MG_ART_* (maiya_game.c): the
# rose storm she knows from the start, and four she learns in the valleys.
ART_KINDS = ("blossom", "rain", "sun", "frost", "gale")


# How she moves, for --lint (maiya_game.c): 8.8 pixels a frame.
RUN_SPEED, WALK_SPEED = 800, 512
JUMP_SPEED, RUN_JUMP_SPEED, LEAP_SPEED = 5 * 256 + 160, 6 * 256, 7 * 256 + 96
AIR_JUMP_SPEED = 5 * 256         # the sky lily's second jump
GRAVITY = {"MG_M_WATER": (34, 3 * 256)}          # (per frame, fastest fall); else (64, 6 * 256)
POOL_BLOCKS = 9          # ledge sprites (MG_LEDGE_BLOCKS)
VIEW = 320


def jump(mech, speed, vy0):
    """The arc of a jump from the ground: (frames, x, height) while she is
    in the air, fixed-point as the game steps it."""
    g, fall = GRAVITY.get(mech, (64, 6 * 256))
    vy, y, x, t, out = -vy0, 0, 0, 0, []
    while True:
        vy = min(vy + g, fall)
        y += vy
        x += speed
        t += 1
        out.append((t, x / 256, -y / 256))
        if y > 160 * 256 or t > 400:
            return out


def reach(mech, rise, run=True):
    """The longest gap (px) she can jump to land `rise` px higher (negative:
    lower), or None if the jump doesn't get that high."""
    arc = jump(mech, RUN_SPEED if run else WALK_SPEED, RUN_JUMP_SPEED if run else JUMP_SPEED)
    top = max(h for _, _, h in arc)
    if rise > top:
        return None
    best = 0
    for (t, x, h), (_, _, h0) in zip(arc[1:], arc):
        if h0 >= rise > h or (h <= rise and h0 <= rise and False):
            best = x
    return best if best else (arc[-1][1] if rise <= 0 else None)


def lint(s, data):
    """What makes a stage play badly though the game takes it: a list of
    warnings. Ledges in x order, on the road's tier (y > 0).
      - repetition: three ledges in a row at one height; a run of heights
        coming round again (the staircase); one width for most ledges; one
        gap between ledges again and again
      - reach: a ledge she can't get to from the road, another ledge or a
        vine with a running jump (the high leap is said so); a pick-up, a
        secret or the key out of reach of where she can stand -- by a jump,
        the kneeling high leap from right under it, or, on a road with a
        sky lily, the lily's second jump (a lily and a gem are hung high on
        purpose)
      - hazards too close together to land between, or at the very start
      - creatures crowded on one screen, or none for a long way
      - more ledge blocks on one screen than the sprite pool holds (a block
        goes undrawn), or more than it holds with what hangs under them
      - the key, the hideout or a captive after the gate's landmark"""
    out = []
    mech = s["mechanic"]
    if mech == "MG_M_FLIGHT":
        return out
    width, gate = s["width"], s["gate_x"]
    plats = sorted((x, y, w) for x, y, w in s["platforms"])
    road = [p for p in plats if p[1] > 0]

    ys = [y for _, y, _ in road]
    for i in range(len(ys) - 2):
        if ys[i] == ys[i + 1] == ys[i + 2]:
            out.append(f"ledges at x {road[i][0]}..{road[i + 2][0]}: three in a row at height {ys[i]}")
    seen = {}
    for i in range(len(ys) - 2):
        key = tuple(y // 8 for y in ys[i:i + 3])
        seen.setdefault(key, []).append(road[i][0])
    for key, where in seen.items():
        if len(where) >= 3 and len(set(key)) > 1:
            out.append(f"the same three heights ({', '.join(str(k * 8) for k in key)}) come round "
                       f"{len(where)} times (x {', '.join(map(str, where))}): a staircase on repeat")
    if len(road) >= 6:
        widths = [w for _, _, w in road]
        common = max(set(widths), key=widths.count)
        if widths.count(common) * 2 > len(widths):
            out.append(f"{widths.count(common)} of {len(widths)} ledges are {common} px wide")
        gaps = [b[0] - a[0] for a, b in zip(road, road[1:])]
        run = 1
        for i in range(1, len(gaps)):
            run = run + 1 if abs(gaps[i] - gaps[i - 1]) <= 12 else 1
            if run == 4:
                out.append(f"ledges from x {road[i - 3][0]}: five {gaps[i]} px apart, like a metronome")

    # Reach: from the road (everywhere), vines' tops, and ledges reached.
    reached = set()
    surfaces = [(0, width, 192)]
    for x, top, _ in s["vines"]:
        for j, (px, py, pw) in enumerate(plats):
            if py == top and px <= x + 16 <= px + pw:
                reached.add(j)
    changed = True
    while changed:
        changed = False
        for j, (px, py, pw) in enumerate(plats):
            if j in reached:
                continue
            sources = surfaces + [plats[k][0:1] + (plats[k][0] + plats[k][2], plats[k][1]) for k in reached]
            for a0, a1, ay in sources:
                gap = max(0, px - a1, a0 - (px + pw))
                r = reach(mech, ay - py)
                if r is not None and gap <= r * 0.85:
                    reached.add(j)
                    changed = True
                    break
    for j, (px, py, pw) in enumerate(plats):
        if j not in reached:
            leap = any(ay - py <= max(h for _, _, h in jump(mech, 0, LEAP_SPEED)) and
                       max(0, px - a1, a0 - (px + pw)) <= 24
                       for a0, a1, ay in surfaces + [(plats[k][0], plats[k][0] + plats[k][2], plats[k][1]) for k in reached])
            out.append(f"ledge at x {px}, height {py}: out of a running jump's reach"
                       + (" (only the high leap gets there)" if leap else ""))
    stand = surfaces + [(plats[k][0], plats[k][0] + plats[k][2], plats[k][1]) for k in reached]

    def top(speed):
        return max(h for _, _, h in jump(mech, 0, speed))
    leap = top(LEAP_SPEED)
    lily = any(kind == "MG_K_LILY" for _, _, kind in s["pickups"])
    twice = top(RUN_JUMP_SPEED) + top(AIR_JUMP_SPEED)

    def reachable(x, y):
        # The pick-up test: her feet within 70 px under the art's top, 28
        # across its middle -- from where she stands, in a running jump off
        # its edge while she is high enough, the high leap from right under
        # it, or the lily's second jump.
        for a0, a1, ay in stand:
            if y > ay - 3:
                continue
            need = ay - (y + 69)
            d = 0 if need <= 0 else reach(mech, need)
            if d is not None and a0 - 28 - d <= x + 16 <= a1 + 28 + d:
                return True
            if need <= leap and a0 - 28 <= x + 16 <= a1 + 28:
                return True
            if lily and need <= twice and a0 - 88 <= x + 16 <= a1 + 88:
                return True
        return False
    for kind, rows in (("pick-up", s["pickups"]), ("secret", s["secrets"])):
        for x, y, _ in rows:
            if not reachable(x, y):
                out.append(f"{kind} at ({x},{y}): out of reach from anywhere she stands")
    kx, ky = s["key"]
    if not reachable(kx, ky):
        out.append(f"the key at ({kx},{ky}): out of reach")

    hz = sorted((x, w) for x, w, _ in s["hazards"])
    for (x0, w0), (x1, _) in zip(hz, hz[1:]):
        if 0 <= x1 - (x0 + w0) < 48:
            out.append(f"hazards at x {x0} and {x1}: {x1 - x0 - w0} px between, no room to land")
    for x, w in hz:
        if x < 200:
            out.append(f"hazard at x {x}: on the road she starts on")

    ex = [x for x, _ in s["encounters"]]
    for i, x in enumerate(ex):
        crowd = [e for e in ex if x <= e < x + VIEW]
        if len(crowd) > 3:
            out.append(f"creatures from x {x}: {len(crowd)} on one screen")
            break
    end = gate - 80 if gate else width
    for a, b in zip([200] + ex, ex + [end]):
        if b - a > 1100:
            out.append(f"no creature from x {a} to {b}")

    worst = (0, 0, 0)
    tiers = ([p for p in plats if p[1] > 0], [p for p in plats if p[1] <= 0])   # never on screen together
    for wx, tier in ((wx, tier) for tier in tiers for wx in range(0, width - VIEW + 1, 16)):
        tops = unders = 0
        for px, py, pw in tier:
            blocks = max(1, (pw + 16) // 32)
            if px + 32 * blocks < wx - 16 or px > wx + VIEW + 16:
                continue
            seen_blocks = sum(1 for k in range(blocks) if wx - 32 <= px + 32 * k <= wx + VIEW + 16)
            tops += seen_blocks
            unders += 1 if blocks >= 3 and seen_blocks else 0
        if (tops, tops + unders) > worst[:2]:
            worst = (tops, tops + unders, wx)
    if worst[0] > POOL_BLOCKS:
        out.append(f"the screen from x {worst[2]} has {worst[0]} ledge blocks: the pool draws {POOL_BLOCKS}")
    elif worst[1] > POOL_BLOCKS:
        out.append(f"the screen from x {worst[2]} has {worst[0]} ledge blocks and "
                   f"{worst[1] - worst[0]} undersides: one goes undrawn")

    if gate:
        for what, x in [("the key", kx), ("the hideout", s["hideout"][0])] + [("a captive", x) for x, _ in s["rescues"]]:
            if x >= gate - 80:
                out.append(f"{what} at x {x}: in the gate's landmark")
    return out


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
                    f"{tuples(s['secrets'], 3)}, 0x{s['rotten']:08X}u}}")
    out.append(table("MG_LEVELS_TABLE", rows))
    out.append(table("MG_DECOR_TABLE", [tuples(s["decor"], 3) for s in stages]))
    out.append(table("MG_VINES_TABLE", [tuples(s["vines"], 3) for s in stages]))
    out.append(table("MG_NPCS_TABLE", ["{" + ",".join(f"{{{x},{t},{c_string(line)}}}" for x, t, line in s["npcs"]) + "}"
                                       if s["npcs"] else "{{0}}" for s in stages]))
    out.append(table("MG_KEY_TABLE", [f"{{{s['key'][0]}, {s['key'][1]}}}" for s in stages]))
    out.append(table("MG_HIDEOUT_TABLE", [f"{{{s['hideout'][0]}, {s['hideout'][1]}, {c_string(s['hideout'][2])}}}"
                                          for s in stages]))
    out.append(table("MG_ART_NAME_TABLE", [c_string(s["art_name"]) for s in stages]))
    out.append(table("MG_ART_KIND_TABLE", [f"{s['art_kind']}u" for s in stages]))
    out.append(table("MG_ART_WORDS_TABLE", [c_string(s["art_words"]) for s in stages]))
    out.append(table("MG_SECRET_HINT_TABLE", [c_string(s["secret_hint"]) for s in stages]))
    out.append(table("MG_BOSS_HINT_TABLE", [c_string(s["boss_hint"]) for s in stages]))
    out.append(table("MG_HEALED_TABLE", [f"{{{c_string(s['healed'][0])}, {c_string(s['healed'][1])}}}" for s in stages]))
    out.append(table("MG_BRIEFING_TABLE", [f"{{{c_string(s['briefing'][0])}, {c_string(s['briefing'][1])}}}" for s in stages]))
    out.append(table("MG_SUNBOY_TABLE", [f"{{{c_string(s['sunboy'][0])}, {c_string(s['sunboy'][1])}}}" for s in stages]))
    out.append(table("MG_PICKS_TABLE", [tuples(s["pickups"], 3) for s in stages]))
    out.append(table("MG_BOSS_TAUNT_TABLE", [c_string(s["taunt"]) for s in stages]))
    out.append(table("MG_BOSS_REPLY_TABLE", [c_string(s["reply"]) for s in stages]))
    out.append(table("MG_STAGE_MECH_TABLE", [s["mechanic"] for s in stages]))
    out.append(table("MG_PIT_TABLE", [s["pit"] for s in stages]))
    out.append(table("MG_BLOCKS_TABLE", [s["blocks"] for s in stages]))
    out.append(table("MG_CLIMB_TABLE", [s["climb"] for s in stages]))
    out.append(table("MG_UPPER_TABLE", [str(s["upper"]) for s in stages]))
    out.append(table("MG_OBJECTIVE_TABLE", [f"{s['objective']}u" for s in stages]))
    out.append(table("MG_POSTED_TABLE", [s["posted"] for s in stages]))
    out.append(table("MG_WAVES_TABLE", [tuples(s["waves"], 5) for s in stages]))
    out.append(table("MG_RUSH_TABLE", ["{" + ",".join(s["rush"] + ["0xFFu"]) + "}" for s in stages]))
    out.append(table("MG_LANDMARK_TABLE", [s["landmark"] for s in stages]))
    out.append(table("MG_FLIGHT_TABLE", ["1" if s["mechanic"] == "MG_M_FLIGHT" else "0" for s in stages]))
    out.append("#endif")
    return "\n".join(out) + "\n"


def load(with_data=False):
    files = sorted(LEVELS.glob("*.json"))
    if not files:
        raise LevelError(f"no stage files in {LEVELS}")
    names = {
        "MG_E_": defines(GAME / "scenes" / "maiya_levels.h", "MG_E_"),
        "MG_H_": defines(GAME / "scenes" / "maiya_levels.h", "MG_H_"),
        "MG_B_": defines(GAME / "scenes" / "maiya_levels.h", "MG_B_"),
        "MG_FORM_": defines(GAME / "scenes" / "maiya_levels.h", "MG_FORM_"),
        "MG_M_": defines(GAME / "scenes" / "maiya_game.c", "MG_M_"),
        "MG_PIT_": defines(GAME / "scenes" / "maiya_game.c", "MG_PIT_"),
        "MG_BLOCKS_": defines(GAME / "scenes" / "maiya_game.c", "MG_BLOCKS_"),
        # From the art build; before it has run the names go unchecked
        # here and the compiler checks them instead.
        "MG_D_": defines(GAME / "artbox" / "generated" / "maiya_assets.h", "MG_D_"),
        "MG_K_": defines(GAME / "artbox" / "generated" / "maiya_assets.h", "MG_K_"),
        "MG_LM_": defines(GAME / "artbox" / "generated" / "maiya_assets.h", "MG_LM_"),
    }
    cap = caps()
    stages = [Stage(f, names, cap) for f in files]
    built = [st.build() for st in stages]
    return (files, built, [st.data for st in stages]) if with_data else (files, built)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="validate the stage files, write nothing")
    parser.add_argument("--lint", action="store_true", help="validate, then warn about how the stages play")
    parser.add_argument("--strict", action="store_true", help="with --lint: a warning fails")
    args = parser.parse_args()
    try:
        files, stages, data = load(with_data=True)
    except LevelError as e:
        print(f"levels: {e}", file=sys.stderr)
        return 1
    if args.lint:
        count = 0
        for f, s, d in zip(files, stages, data):
            for w in lint(s, d):
                print(f"levels: {f.name}: warning: {w}")
                count += 1
        print(f"levels: {len(stages)} stages, {count} warnings")
        return 1 if (args.strict and count) else 0
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
