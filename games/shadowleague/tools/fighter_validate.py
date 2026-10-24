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
    if 'combat' in data:
        combat = data['combat']
        selectors = [m['anim'] + ':' + m.get('buttons', '') + ':' + m.get('motion', 'none') for m in data['moves']]
        routes = combat.get('routes', [])
        if not 1 <= len(routes) <= 32: err('combat needs 1..32 cancel routes')
        for route in routes:
            for key in ('from', 'to'):
                if selectors.count(route.get(key)) != 1: err('combat route has an unknown or ambiguous ' + key)
            ticks = route.get('ticks', [])
            if len(ticks) != 2 or not 0 <= ticks[0] <= ticks[1] <= 254: err('combat cancel window must be ordered ticks 0..254')
            conditions = set(route.get('on', '').split('|'))
            if conditions - {'hit', 'block', 'counter', 'whiff', 'max'} or not conditions - {'max'}:
                err('combat route has invalid contact conditions')
            if not 1 <= route.get('max_uses', 1) <= 3: err('combat repeat limit must be 1..3')
        for key, low, high in (('juggle_limit', 1, 12), ('relaunch_limit', 0, 3)):
            if not low <= combat.get(key, -1) <= high: err('combat ' + key + ' outside allowed range')
        if combat.get('assist'):
            if selectors.count(combat['assist']) != 1: err('combat assist is unknown or ambiguous')
            else:
                move = data['moves'][selectors.index(combat['assist'])]
                if move.get('projectile') or not move.get('limb'): err('combat assist must name an authored melee attack')
        throws = combat.get('throws', [])
        if len(throws) > 8: err('combat allows at most 8 throw anchors')
        throw_selectors = set(selectors)
        if any(m.get('kind') == 'throw' for m in data['moves']):
            for motion in ('fwd', 'back'):
                for btn in ('C', 'D'):
                    throw_selectors.add(f'throw:{btn}:{motion}')
        for th in throws:
            if th.get('move') not in throw_selectors:
                err('combat throw references unknown move ' + str(th.get('move')))
            if not 12 <= int(th.get('grip_x', 26)) <= 64: err('combat throw grip_x outside 12..64')
            if not 0 <= int(th.get('lift', 16)) <= 96: err('combat throw lift outside 0..96')
            if not 1 <= int(th.get('carry_tick', 8)) <= 32: err('combat throw carry_tick outside 1..32')
            if not 12 <= int(th.get('release_x', 26)) <= 80: err('combat throw release_x outside 12..80')
            if not 1 <= int(th.get('tech_ticks', 4)) <= 12: err('combat throw tech_ticks outside 1..12')
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
