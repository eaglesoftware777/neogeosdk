"""Cook Shadow League fighter definitions into ng_fight ROM tables and frames.

Every pose an animation uses is drawn once (fighter_art), so a fighter's
sprite frames, hurtboxes and per-move hitboxes come from the same joints.

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
          "360": "NGF_MOTION_360", "dash": "NGF_MOTION_DASH", "backdash": "NGF_MOTION_BACKDASH",
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
        self.images, self.joints = [], []
        for name in used:
            image, joints = art.render(d["build"], d["materials"], self.poses[name])
            self.images.append(image)
            self.joints.append(joints)

    def palettes(self):
        base = {k: [tuple(c) for c in v] for k, v in self.data["palette"].items()}
        alt = dict(base)
        alt.update({k: [tuple(c) for c in v] for k, v in self.data.get("alt_palette", {}).items()})
        return art.ramp_colors(base), art.ramp_colors(alt)

    def hitbox(self, move):
        if "limb" not in move:
            return [0, 0, 0, 0]
        boxes = []
        for fr in self.data["anims"][move["anim"]]:
            if len(fr) > 2 and "active" in fr[2]:
                j = self.joints[self.sprite_names.index(fr[0])]
                boxes.append(art.hitbox(self.data["build"], j, move["limb"]))
        x0 = min(b[0] for b in boxes)
        y0 = min(b[1] for b in boxes)
        x1 = max(b[0] + b[2] for b in boxes)
        y1 = max(b[1] + b[3] for b in boxes)
        return [x0, y0, x1 - x0, y1 - y0]

    def c_tables(self, prefix):
        d = self.data
        lines = []
        frames, anims = [], []
        for name in self.anim_names:
            first = len(frames)
            for fr in d["anims"][name]:
                sprite = self.sprite_names.index(fr[0])
                fl = fr[2] if len(fr) > 2 else ""
                dx = int(fr[3]) if len(fr) > 3 else 0
                dy = int(fr[4]) if len(fr) > 4 else 0
                frames.append(f"{{{sprite}, {int(fr[1])}, {sprite}, {flags(fl, FLAG)}, {dx}, {dy}}}")
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
            boxes = art.hurtboxes(d["build"], j, attacking)
            if pose.get("hip", [0, 0])[1] > 36:          # lying down
                boxes = boxes[:1]
            cells = ", ".join(f"{{{b[0]}, {b[1]}, {b[2]}, {b[3]}}}" for b in boxes)
            hurt.append(f"{{{{{cells}}}, {len(boxes)}, 0}}")
        lines.append(f"static const NGFHurtSet {prefix}_hurt[] = {{\n    " + ",\n    ".join(hurt) + "\n};")
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
            f"{idx('special')}, {idx('rush')}, {idx('throw')}, {idx('super')}}}\n}};")
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
            rgb = pal[image]
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
