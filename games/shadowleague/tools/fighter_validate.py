"""Reject invalid Shadow League fighter definitions before they are cooked.

    python3 games/shadowleague/tools/fighter_validate.py [fighters/*.json]
"""

import json
import sys
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
STANDARD = ("idle", "walk_f", "walk_b", "crouch", "jump", "guard", "guard_low", "hit", "hit_low",
            "fall", "down", "getup", "dash", "backdash", "thrown", "win", "lose", "intro", "dizzy")
KINDS = ("normal", "command", "special", "super", "throw", "evade", "guard_attack")
MOTIONS = ("none", "qcf", "qcb", "dp", "rdp", "hcf", "hcb", "dqcf", "dqcb", "charge_bf", "charge_du",
           "360", "720", "dash", "backdash", "fwd", "back", "down_fwd")
LEVELS = ("mid", "high", "low", "overhead", "throw", "unblockable")
STANCES = ("stand", "crouch", "air", "ground", "any", "stand|close", "stand|far")
FLAGS = ("active", "rehit", "spawn", "invuln", "airborne", "counter")
LIMBS = ("near_fist", "far_fist", "near_ankle", "far_ankle")
POSE_KEYS = ("base", "hip", "torso", "head", "near_fist", "far_fist", "near_foot", "far_foot",
             "near_elbow", "far_elbow", "near_knee", "far_knee", "fist", "tail")
BYTE_FIELDS = ("damage", "guard_damage", "chip", "hitstun", "blockstun", "pushback", "meter", "cost",
               "hitstop", "window", "range")


def validate(data, source="fighter"):
    errors = []

    def err(text):
        errors.append(f"{source}: {text}")

    for key in ("id", "name", "life", "physics", "push", "build", "materials", "palette", "poses", "anims", "moves", "cpu"):
        if key not in data:
            err(f"missing '{key}'")
    if errors:
        return errors
    poses = data["poses"]
    for name, pose in poses.items():
        for key in pose:
            if key not in POSE_KEYS:
                err(f"pose {name}: unknown key '{key}'")
        seen, cur = set(), name
        while "base" in poses.get(cur, {}):
            if cur in seen:
                err(f"pose {name}: base cycle")
                break
            seen.add(cur)
            cur = poses[cur]["base"]
            if cur not in poses:
                err(f"pose {name}: unknown base '{cur}'")
                break
    for need in ("hip", "torso", "head", "near_fist", "far_fist", "near_foot", "far_foot"):
        if need not in poses.get("stance", {}):
            err(f"pose stance must define '{need}'")
    anims = data["anims"]
    for name in STANDARD:
        if name not in anims:
            err(f"missing standard animation '{name}'")
    for name, frames in anims.items():
        if not frames:
            err(f"anim {name}: empty")
        for i, fr in enumerate(frames):
            if not isinstance(fr, list) or not 2 <= len(fr) <= 5:
                err(f"anim {name}[{i}]: expected [pose, time, flags, dx, dy]")
                continue
            if fr[0] not in poses:
                err(f"anim {name}[{i}]: unknown pose '{fr[0]}'")
            if not 1 <= int(fr[1]) <= 255:
                err(f"anim {name}[{i}]: time {fr[1]} outside 1..255")
            for flag in (fr[2].split("|") if len(fr) > 2 and fr[2] else []):
                if flag not in FLAGS:
                    err(f"anim {name}[{i}]: unknown flag '{flag}'")
            for v in fr[3:]:
                if not -128 <= int(v) <= 127:
                    err(f"anim {name}[{i}]: speed {v} outside int8")
    names = [m.get("name") for m in data["moves"]]
    if len(data["moves"]) > 255:
        err("more than 255 moves")
    for i, mv in enumerate(data["moves"]):
        tag = f"move {i} ({mv.get('name')})"
        if mv.get("anim") not in anims:
            err(f"{tag}: unknown anim '{mv.get('anim')}'")
        if mv.get("kind") not in KINDS:
            err(f"{tag}: unknown kind '{mv.get('kind')}'")
        if mv.get("motion", "none") not in MOTIONS:
            err(f"{tag}: unknown motion '{mv.get('motion')}'")
        if mv.get("level", "mid") not in LEVELS:
            err(f"{tag}: unknown level '{mv.get('level')}'")
        if mv.get("stance", "stand") not in STANCES:
            err(f"{tag}: unknown stance '{mv.get('stance')}'")
        if set(mv.get("buttons", "")) - set("ABCD"):
            err(f"{tag}: buttons must be letters A-D")
        if not mv.get("buttons") and not any(mv.get("name") == other.get("follow") for other in data["moves"]):
            err(f"{tag}: no buttons and no move follows into it")
        for key in BYTE_FIELDS:
            if not 0 <= int(mv.get(key, 0)) <= 255:
                err(f"{tag}: {key} outside 0..255")
        if "limb" in mv:
            if mv["limb"] not in LIMBS:
                err(f"{tag}: unknown limb '{mv['limb']}'")
            if mv.get("anim") in anims and not any(len(fr) > 2 and "active" in fr[2] for fr in anims[mv["anim"]]):
                err(f"{tag}: has a limb but its animation has no active frame")
        elif mv.get("kind") not in ("throw", "evade") and mv.get("damage") and not mv.get('projectile'):
            err(f"{tag}: deals damage but names no limb")
        if mv.get("follow") and mv["follow"] not in names:
            err(f"{tag}: follow '{mv['follow']}' is not a move")
        if mv.get("kind") == "throw" and not mv.get("range"):
            err(f"{tag}: a throw needs a range")
    for key, value in data["cpu"].items():
        if isinstance(value, str) and value not in names:
            err(f"cpu {key}: '{value}' is not a move")
    for slot in ("skin", "A", "B", "C"):
        if slot not in data["palette"]:
            err(f"palette: missing ramp '{slot}'")
    projectiles=data.get('projectiles',[])
    if len(projectiles)>4:err('more than four projectile definitions')
    for i,p in enumerate(projectiles):
        if p.get('anim','idle') not in anims:err(f'projectile {i}: unknown animation')
        if not 1<=p.get('speed',0)<=12:err(f'projectile {i}: invalid speed')
        if len(p.get('spawn',[]))!=2 or any(not -128<=v<=127 for v in p.get('spawn',[])):err(f'projectile {i}: invalid spawn')
        box=p.get('box',[])
        if len(box)!=4 or not (-128<=box[0]<=127 and -128<=box[1]<=127 and 1<=box[2]<=255 and 1<=box[3]<=255):err(f'projectile {i}: invalid box')
        for field in ('damage','chip','hitstun','blockstun','life'):
            if not 0<=p.get(field,0)<=255:err(f'projectile {i}: invalid {field}')
    for move in data['moves']:
        if move.get('projectile',0)>len(projectiles) or move.get('projectile',0)<0:err('move references an invalid projectile')
    return errors


def main(paths):
    paths = paths or sorted((GAME / "fighters").glob("*.json"))
    failures = []
    for path in paths:
        failures += validate(json.loads(Path(path).read_text()), Path(path).name)
    for line in failures:
        print(line)
    if failures:
        raise SystemExit(f"fighter_validate: {len(failures)} problem(s)")
    print(f"fighter_validate: {len(paths)} fighter(s) valid")


if __name__ == "__main__":
    main(sys.argv[1:])
