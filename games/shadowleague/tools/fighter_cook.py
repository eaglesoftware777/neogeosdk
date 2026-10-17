"""Cook Shadow League fighter definitions into ng_fight ROM tables and frames.

Activated native fighters load completed PNG layers, exports and contact metadata.
Missing native poses fail instead of falling back to the legacy geometry renderer.
Other fighters retain the existing fighter_art drawing and joint metadata.

    python3 games/shadowleague/tools/fighter_cook.py --sheet   # contact sheets for review
"""

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

TOOLS = Path(__file__).resolve().parent
GAME = TOOLS.parent
sys.path.insert(0, str(TOOLS))
import fighter_art as art  # noqa: E402
from fighter_validate import STANDARD, validate  # noqa: E402

KIND = {"normal": "NGF_KIND_NORMAL", "command": "NGF_KIND_COMMAND", "special": "NGF_KIND_SPECIAL",
        "super": "NGF_KIND_SUPER", "throw": "NGF_KIND_THROW", "evade": "NGF_KIND_EVADE",
        "guard_attack": "NGF_KIND_GUARD_ATTACK"}
MOTION = {"none": "NGF_MOTION_NONE", "qcf": "NGF_MOTION_QCF", "qcb": "NGF_MOTION_QCB", "dp": "NGF_MOTION_DP",
          "rdp": "NGF_MOTION_RDP", "hcf": "NGF_MOTION_HCF", "hcb": "NGF_MOTION_HCB", "dqcf": "NGF_MOTION_DQCF",
          "dqcb": "NGF_MOTION_DQCB", "charge_bf": "NGF_MOTION_CHARGE_BF", "charge_du": "NGF_MOTION_CHARGE_DU",
          "360": "NGF_MOTION_360", "720": "NGF_MOTION_720", "dash": "NGF_MOTION_DASH", "backdash": "NGF_MOTION_BACKDASH",
          "fwd": "NGF_MOTION_FWD", "back": "NGF_MOTION_BACK", "down_fwd": "NGF_MOTION_DOWN_FWD"}
LEVEL = {"mid": "NGF_LV_MID", "high": "NGF_LV_HIGH", "low": "NGF_LV_LOW", "overhead": "NGF_LV_OVERHEAD",
         "throw": "NGF_LV_THROW", "unblockable": "NGF_LV_UNBLOCKABLE"}
STANCE = {"stand": "NGF_STANCE_STAND", "crouch": "NGF_STANCE_CROUCH", "air": "NGF_STANCE_AIR",
          "ground": "NGF_STANCE_GROUND", "any": "(NGF_STANCE_GROUND | NGF_STANCE_AIR)",
          "stand|close": "(NGF_STANCE_STAND | NGF_STANCE_CLOSE)", "stand|far": "(NGF_STANCE_STAND | NGF_STANCE_FAR)"}
FLAG = {"active": "NGF_FF_ACTIVE", "rehit": "NGF_FF_REHIT", "spawn": "NGF_FF_SPAWN", "invuln": "NGF_FF_INVULN",
        "airborne": "NGF_FF_AIRBORNE", "counter": "NGF_FF_COUNTER"}
ON_HIT = {"knockdown": "NGF_ON_KNOCKDOWN", "launch": "NGF_ON_LAUNCH", "wallbounce": "NGF_ON_WALLBOUNCE"}
CANCEL = {"chain": "NGF_CANCEL_CHAIN", "special": "NGF_CANCEL_SPECIAL", "super": "NGF_CANCEL_SUPER"}
SOUNDS = ("none", "hit_light", "hit_heavy", "block", "special", "super", "whoosh", "throw", "ko", "land")


# try_moves takes the first match, so moves whose input contains another's
# come first: supers, then specials by motion length, two-button moves before
# one-button ones, throws before the normals that share their button.
KIND_ORDER = {"super": 0, "special": 1, "guard_attack": 2, "evade": 3, "throw": 5, "command": 6, "normal": 7}
MOTION_ORDER = {"dqcf": 0, "dqcb": 0, "720": 1, "360": 2, "hcf": 3, "hcb": 3, "dp": 4, "rdp": 4, "qcf": 5, "qcb": 5,
                "charge_bf": 6, "charge_du": 6, "dash": 7, "backdash": 7, "down_fwd": 8, "fwd": 8, "back": 8, "none": 9}
# Normals without a cancel setting: lights chain, heavies cancel into
# specials, sweeps and jumping attacks do not.
DEFAULT_CANCEL = {("stand", "A"): "chain|special|super", ("stand", "B"): "chain|special|super",
                  ("stand", "C"): "special|super", ("stand", "D"): "special|super",
                  ("crouch", "A"): "chain|special|super", ("crouch", "B"): "special|super",
                  ("crouch", "C"): "special|super"}
BLOWBACK = {"name": "BLOWBACK", "kind": "command", "buttons": "CD", "stance": "stand", "level": "mid",
            "damage": 60, "guard_damage": 14, "hitstun": 20, "blockstun": 16, "pushback": 14,
            "on_hit": "knockdown", "launch": [10, 26], "cancel": "super", "meter": 8, "hitstop": 10,
            "sound": "whoosh", "hit_sound": "hit_heavy"}


def complete_moves(data):
    """The system moves every fighter has, then the moves in matching order."""
    moves = [dict(m) for m in data["moves"]]
    for m in moves:
        if m["kind"] == "normal" and "cancel" not in m:
            stance = m.get("stance", "stand").split("|")
            if "far" not in stance:
                m["cancel"] = DEFAULT_CANCEL.get((stance[0], m.get("buttons", "")[:1]), "")
    ground = [m for m in moves if m["kind"] in ("normal", "command") and m.get("stance", "stand").split("|")[0] != "air"]
    if not any(m.get("buttons") == "CD" for m in ground):
        heavy = next((m for m in ground if m["kind"] == "normal" and m.get("buttons") == "D" and
                      m.get("stance", "stand").split("|")[0] == "stand"), None)
        if heavy:
            moves.append(dict(BLOWBACK, anim=heavy["anim"], **({"limb": heavy["limb"]} if "limb" in heavy else {})))
    throws = [m for m in moves if m["kind"] == "throw"]
    if throws:
        base = throws[0]
        have = {(m.get("motion", "none"), m.get("buttons")) for m in throws}
        for motion in ("fwd", "back"):
            for button in ("C", "D"):
                if (motion, button) not in have:
                    moves.append(dict(base, motion=motion, buttons=button))
                    have.add((motion, button))
    order = sorted(range(len(moves)), key=lambda i: (
        KIND_ORDER[moves[i]["kind"]] - (2 if moves[i]["kind"] == "command" and len(moves[i].get("buttons", "")) > 1 else 0),
        MOTION_ORDER[moves[i].get("motion", "none")], -len(moves[i].get("buttons", "")), i))
    return [moves[i] for i in order]


# A limb's contact box covers only the fist or foot, which at close range is
# already past the other fighter's body: strikes reach back along the limb to
# this many pixels in front of the attacker, and rising strikes (uppercuts,
# flip kicks) down to chest height, so they also hit a standing opponent.
LIMB_INNER = 10
LIMB_CHEST = 80


def along_limb(box):
    x, y, w, h = (int(v) for v in box)
    if x > LIMB_INNER:
        w, x = x + w - LIMB_INNER, LIMB_INNER
    if y > LIMB_CHEST:
        h, y = y + h - LIMB_CHEST, LIMB_CHEST
    return [x, y, w, h]


def flags(text, table):
    parts = [table[p] for p in (text.split("|") if text else [])]
    return " | ".join(parts) if parts else "0"


def q8(value):
    return int(round(float(value) * 256))


class Fighter:
    def __init__(self, path):
        self.path = Path(path)
        self.data = json.loads(self.path.read_text())
        problems = validate(self.data, self.path.name)
        if problems:
            raise SystemExit("\n".join(problems))
        self.data["moves"] = complete_moves(self.data)
        d = self.data
        self.id = d["id"]
        self.poses = {name: art.resolve(d["poses"], name) for name in d["poses"]}
        anim_names = list(STANDARD) + [n for n in d["anims"] if n not in STANDARD]
        self.anim_names = anim_names
        used = []
        for name in anim_names:
            for fr in d["anims"][name]:
                if fr[0] not in used:
                    used.append(fr[0])
        self.sprite_names = used
        self.authored = None
        source = GAME / 'assets/authored/fighters' / self.id
        self.authored_folder = source
        self.images, self.joints = [], []
        if (source / 'manifest.json').exists():
            from native_art_io import fighter_source
            q = fighter_source(source, used)
            self.images, self.authored = q['images'], q['metadata']
            self.source_images, self.source_palette = q['source_images'], q['source_palette']
            self.authored_palette = self.source_palette[:16]
            self.tile_banks, self.bank_indices = q['tile_maps'], q['bank_indices']
            self.source_banks = q['banks']
            self.joints = [None] * len(used)
            return
        for name in used:
            image, joints = art.render(d["build"], d["materials"], self.poses[name])
            self.images.append(image)
            self.joints.append(joints)

    def palettes(self):
        if self.authored is not None:
            manifest = json.loads((self.authored_folder/'manifest.json').read_text())
            alternate = np.asarray(manifest['alt_palette'],dtype=np.uint8)
            if alternate.ndim!=2 or alternate.shape[1]!=3 or len(alternate)<=max(int(im.max()) for im in self.source_images):raise ValueError(self.id+': alternate palette omits used source colors')
            return self.authored_palette, alternate[:16]
        base = {k: [tuple(c) for c in v] if isinstance(v[0], (list, tuple)) else tuple(v) for k, v in self.data["palette"].items()}
        alt = dict(base)
        alt.update({k: [tuple(c) for c in v] if isinstance(v[0], (list, tuple)) else tuple(v) for k, v in self.data.get("alt_palette", {}).items()})
        return art.ramp_colors(base), art.ramp_colors(alt)

    def palette_banks(self):
        if self.authored is None:
            normal, alternate = self.palettes()
            return np.asarray([normal]), np.asarray([alternate])
        manifest = json.loads((self.authored_folder/'manifest.json').read_text())
        alternate = np.asarray(manifest['alt_palette'],dtype=np.uint8)
        if alternate.ndim!=2 or alternate.shape[1]!=3 or len(alternate)<=max(int(im.max()) for im in self.source_images):raise ValueError(self.id+': alternate palette omits used source colors')
        return self.source_banks, np.asarray([alternate[indices] for indices in self.bank_indices])

    def hitbox(self, move):
        if "limb" not in move:
            return [0, 0, 0, 0]
        boxes = []
        for fr in self.data["anims"][move["anim"]]:
            if len(fr) > 2 and "active" in fr[2]:
                i = self.sprite_names.index(fr[0])
                if self.authored is not None:
                    boxes.extend(along_limb(b) for b in self.authored[i]["hit"].get(move["limb"], []))
                else:
                    boxes.append(along_limb(art.hitbox(self.data["build"], self.joints[i], move["limb"])))
        if not boxes: return [0, 0, 0, 0]
        x0 = min(b[0] for b in boxes)
        y0 = min(b[1] for b in boxes)
        x1 = max(b[0] + b[2] for b in boxes)
        y1 = max(b[1] + b[3] for b in boxes)
        return [x0, y0, x1 - x0, y1 - y0]

    def c_tables(self, prefix):
        d = self.data
        lines = []
        frames, anims = [], []
        hit_sets = [()]
        frame_hits = []
        for name in self.anim_names:
            first = len(frames)
            for fr in d["anims"][name]:
                sprite = self.sprite_names.index(fr[0])
                fl = fr[2] if len(fr) > 2 else ""
                dx = int(fr[3]) if len(fr) > 3 else 0
                dy = int(fr[4]) if len(fr) > 4 else 0
                frames.append(f"{{{sprite}, {int(fr[1])}, {sprite}, {flags(fl, FLAG)}, {dx}, {dy}}}")
                limbs = sorted({m['limb'] for m in d['moves'] if m['anim']==name and 'limb' in m}) if 'active' in fl else []
                boxes = []
                for limb in limbs:
                    if self.authored is not None:
                        boxes.extend(along_limb(b) for b in self.authored[sprite]['hit'].get(limb, []))
                    else:
                        boxes.append(along_limb(art.hitbox(d['build'], self.joints[sprite], limb)))
                key = tuple(tuple(int(v) for v in box) for box in boxes)
                if len(key)>3:raise ValueError(self.id+'/'+name+': more than three frame contacts')
                if key not in hit_sets:hit_sets.append(key)
                if len(hit_sets)>256:raise ValueError(self.id+': contact set index overflow')
                frame_hits.append(hit_sets.index(key))
            count = len(d["anims"][name])
            last = d["anims"][name][-1]
            loop = "0xFF" if int(last[1]) == 255 or name in ("getup", "throw") or not self._loops(name) else "0"
            anims.append(f"{{{first}, {count}, {loop}}}")
        lines.append(f"static const NGFFrame {prefix}_frames[] = {{\n    " + ",\n    ".join(frames) + "\n};")
        lines.append(f"static const NGFAnim {prefix}_anims[] = {{\n    " + ",\n    ".join(anims) + "\n};")
        hurt = []
        for i, j in enumerate(self.joints):
            pose = self.poses[self.sprite_names[i]]
            attacking = self._attacking_limb(self.sprite_names[i])
            boxes = self.authored[i]["hurt"] if self.authored is not None else art.hurtboxes(d["build"], j, attacking)
            if self.authored is None and pose.get("hip", [0, 0])[1] > 36:          # lying down
                boxes = boxes[:1]
            cells = ", ".join(f"{{{b[0]}, {b[1]}, {b[2]}, {b[3]}}}" for b in boxes)
            hurt.append(f"{{{{{cells}}}, {len(boxes)}, 0}}")
        lines.append(f"static const NGFHurtSet {prefix}_hurt[] = {{\n    " + ",\n    ".join(hurt) + "\n};")
        hit_rows = []
        for boxes in hit_sets:
            cells = ', '.join('{'+', '.join(str(v) for v in b)+'}' for b in boxes) or '{0, 0, 0, 0}'
            hit_rows.append('{{'+cells+'}, '+str(len(boxes))+', 0}')
        lines.append(f"static const NGFHitSet {prefix}_hit_sets[] = {{\n    " + ",\n    ".join(hit_rows) + "\n};")
        lines.append(f"static const uint8_t {prefix}_frame_hits[] = {{" + ','.join(str(v) for v in frame_hits) + "};")
        names = [m["name"] for m in d["moves"]]
        moves = []
        for mv in d["moves"]:
            hb = self.hitbox(mv)
            launch = mv.get("launch", [0, 0])
            follow = names.index(mv["follow"]) + 1 if mv.get("follow") else 0
            buttons = " | ".join(f"NGF_BTN_{b}" for b in mv.get("buttons", "")) or "0"
            moves.append(
                "{" + ", ".join(str(v) for v in (
                    self.anim_names.index(mv["anim"]), KIND[mv["kind"]], MOTION[mv.get("motion", "none")], buttons,
                    STANCE[mv.get("stance", "stand")], LEVEL[mv.get("level", "mid")], mv.get("damage", 0),
                    mv.get("guard_damage", 0), mv.get("chip", 0), mv.get("hitstun", 0), mv.get("blockstun", 0),
                    mv.get("pushback", 0), flags(mv.get("on_hit", ""), ON_HIT), flags(mv.get("cancel", ""), CANCEL),
                    mv.get("meter", 0), mv.get("cost", 0), mv.get("hitstop", 0), mv.get("projectile", 0),
                    SOUNDS.index(mv.get("sound", "whoosh" if mv.get("damage") else "none")),
                    SOUNDS.index(mv.get("hit_sound", "hit_light")), launch[0], launch[1], mv.get("window", 0),
                    mv.get("range", 60), f"{{{hb[0]}, {hb[1]}, {hb[2]}, {hb[3]}}}", follow, 0)) + "}")
        lines.append(f"static const NGFMove {prefix}_moves[] = {{\n    " + ",\n    ".join(moves) + "\n};")
        ph = d["physics"]
        cpu = d["cpu"]

        def idx(key):
            return names.index(cpu[key]) if cpu.get(key) in names else 0xFF
        push = d["push"]
        lines.append(
            f"static const NGFFighterDef {prefix} = {{\n"
            f"    \"{d['name']}\", {d['life']}, {q8(ph['walk_fwd'])}, {q8(ph['walk_back'])}, {q8(ph['dash'])},\n"
            f"    {q8(ph['backdash'][0])}, {q8(ph['backdash'][1])}, {q8(ph['jump'][0])}, {q8(ph['jump'][1])}, {q8(ph['gravity'])},\n"
            f"    {push[0]}, {push[1]}, {push[2]}, {d['close_range']}, {d['guard_max']}, 0,\n"
            f"    {prefix}_anims, {prefix}_frames, {prefix}_hurt, {prefix}_moves, 0,\n"
            f"    {len(self.anim_names)}, {len(d['moves'])}, 0, {len(self.joints)},\n"
            f"    {{{cpu['aggression']}, {cpu['range']}, {cpu['jump']}, {idx('poke')}, {idx('sweep')}, {idx('anti_air')}, "
            f"{idx('special')}, {idx('rush')}, {idx('throw')}, {idx('super')}}},\n"
            f"    {prefix}_hit_sets, {prefix}_frame_hits\n}};")
        return "\n".join(lines)

    def _loops(self, name):
        return name in ("idle", "walk_f", "walk_b", "crouch", "guard", "guard_low", "dash", "dizzy", "intro") or \
            int(self.data["anims"][name][-1][1]) != 255 and name in ("idle",)

    def _attacking_limb(self, pose_name):
        for mv in self.data["moves"]:
            if "limb" not in mv:
                continue
            for fr in self.data["anims"][mv["anim"]]:
                if fr[0] == pose_name and len(fr) > 2 and "active" in fr[2]:
                    return mv["limb"]
        return None

    def sheet(self, path, scale=2):
        pal, _ = self.palettes()
        cols = 10
        rows = (len(self.images) + cols - 1) // cols
        out = np.zeros((rows * art.H, cols * art.W, 3), np.uint8)
        out[:] = (60, 64, 80)
        for i, image in enumerate(self.images):
            y, x = (i // cols) * art.H, (i % cols) * art.W
            rgb = self.source_palette[self.source_images[i]] if self.authored is not None else pal[image]
            mask = image > 0
            out[y:y + art.H, x:x + art.W][mask] = rgb[mask]
        Image.fromarray(out).resize((out.shape[1] * scale, out.shape[0] * scale), Image.NEAREST).save(path)


def load_all():
    return [Fighter(p) for p in sorted((GAME / "fighters").glob("*.json"))]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--sheet", action="store_true")
    args = ap.parse_args()
    out = GAME / "artbox/generated"
    out.mkdir(parents=True, exist_ok=True)
    for f in load_all():
        print(f"{f.id}: {len(f.images)} frames, {len(f.anim_names)} animations, {len(f.data['moves'])} moves")
        if args.sheet:
            f.sheet(out / f"{f.id}_sheet.png")


if __name__ == "__main__":
    main()
