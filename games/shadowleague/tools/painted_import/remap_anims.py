"""Point every Shadow League fighter's animations at the production pose set.

Timings, flags and movement of each animation are kept; only the drawings
change. Fighters missing crouching or jumping normals receive the standard
set (values from the karate template), so all 44 have A/B/C/D standing,
crouching and jumping attacks.

    python3 remap_anims.py WORKTREE
"""
import json
import sys
from pathlib import Path

SOURCE = Path(__file__).resolve().parent
sys.path.insert(0, str(SOURCE))
from roster import R  # noqa: E402

STANDARD = {
    'idle': [['stance', 10], ['idle2', 10], ['idle3', 10], ['idle2', 10]],
    'walk_f': [['walk1', 5], ['walk2', 5], ['walk3', 5], ['walk4', 5], ['walk5', 5], ['walk6', 5]],
    'walk_b': [['walk6', 6], ['walk5', 6], ['walk4', 6], ['walk3', 6], ['walk2', 6], ['walk1', 6]],
    'crouch': [['crouch', 6]],
    'jump': [['jump_up', 8], ['jump_apex', 10], ['jump_down', 255]],
    'guard': [['guard', 4]],
    'guard_low': [['guard_low', 4]],
    'hit': [['hit', 255]],
    'hit_low': [['hit_low', 255]],
    'fall': [['fall', 255]],
    'down': [['down', 255]],
    'getup': [['getup', 10], ['crouch', 8]],
    'dash': [['dash', 255]],
    'backdash': [['backdash', 255]],
    'thrown': [['hit_b', 255]],
    'win': [['win', 24], ['win_b', 255]],
    'lose': [['lose', 255]],
    'intro': [['intro', 40], ['stance', 255]],
    'dizzy': [['dizzy', 12], ['dizzy_b', 12]],
}
# (wind, strike, recover) per normal (stance, button)
NORMAL = {
    ('stand', 'A'): ('lp1', 'lp2', 'lp1'), ('stand', 'B'): ('hk1', 'lk2', 'stance'),
    ('stand', 'C'): ('hp1', 'hp2', 'hp1'), ('stand', 'D'): ('hk1', 'hk2', 'hk1'),
    ('crouch', 'A'): ('crouch', 'cr_lp', 'crouch'), ('crouch', 'B'): ('crouch', 'cr_lk', 'crouch'),
    ('crouch', 'C'): ('cr_hp1', 'cr_hp2', 'cr_hp1'), ('crouch', 'D'): ('crouch', 'cr_hk', 'crouch'),
    ('air', 'A'): ('jump_apex', 'j_p', 'j_p'), ('air', 'C'): ('jump_apex', 'j_p', 'j_p'),
    ('air', 'B'): ('jump_apex', 'j_k', 'j_k'), ('air', 'D'): ('jump_apex', 'j_k', 'j_k'),
}
TEMPLATE = {  # added normals, from the karate fighter's tuned values
    ('stand', 'A'): {"name": "JAB", "level": "mid", "damage": 24, "guard_damage": 6, "hitstun": 11, "blockstun": 8, "pushback": 6, "cancel": "chain|special|super", "meter": 4, "hitstop": 6, "hit_sound": "hit_light", "limb": "far_fist", "frames": [2, 3, 7]},
    ('stand', 'B'): {"name": "LIGHT KICK", "level": "mid", "damage": 30, "guard_damage": 8, "hitstun": 12, "blockstun": 9, "pushback": 8, "cancel": "chain|special|super", "meter": 5, "hitstop": 6, "hit_sound": "hit_light", "limb": "far_ankle", "frames": [3, 3, 9]},
    ('stand', 'C'): {"name": "HEAVY PUNCH", "level": "mid", "damage": 64, "guard_damage": 14, "hitstun": 18, "blockstun": 14, "pushback": 10, "cancel": "special|super", "meter": 10, "hitstop": 9, "hit_sound": "hit_heavy", "limb": "near_fist", "frames": [4, 4, 14]},
    ('stand', 'D'): {"name": "HEAVY KICK", "level": "mid", "damage": 70, "guard_damage": 14, "hitstun": 18, "blockstun": 14, "pushback": 12, "cancel": "special|super", "meter": 10, "hitstop": 9, "hit_sound": "hit_heavy", "limb": "near_ankle", "frames": [6, 4, 16]},
    ('crouch', 'A'): {"name": "CROUCH JAB", "level": "mid", "damage": 22, "guard_damage": 6, "hitstun": 11, "blockstun": 8, "pushback": 6, "cancel": "chain|special|super", "meter": 4, "hitstop": 6, "hit_sound": "hit_light", "limb": "far_fist", "frames": [2, 3, 7]},
    ('crouch', 'B'): {"name": "CROUCH KICK", "level": "low", "damage": 28, "guard_damage": 8, "hitstun": 12, "blockstun": 9, "pushback": 8, "cancel": "special|super", "meter": 5, "hitstop": 6, "hit_sound": "hit_light", "limb": "far_ankle", "frames": [4, 3, 9]},
    ('crouch', 'C'): {"name": "RISING STRIKE", "level": "mid", "damage": 70, "guard_damage": 14, "hitstun": 18, "blockstun": 14, "pushback": 10, "cancel": "special|super", "meter": 10, "hitstop": 9, "hit_sound": "hit_heavy", "limb": "near_fist", "frames": [4, 6, 14]},
    ('crouch', 'D'): {"name": "SWEEP", "level": "low", "damage": 70, "guard_damage": 14, "hitstun": 18, "blockstun": 14, "pushback": 12, "on_hit": "knockdown", "launch": [6, 14], "meter": 10, "hitstop": 9, "hit_sound": "hit_heavy", "limb": "near_ankle", "frames": [7, 4, 20]},
    ('air', 'A'): {"name": "JUMP JAB", "level": "overhead", "damage": 34, "guard_damage": 6, "hitstun": 12, "blockstun": 9, "pushback": 6, "meter": 5, "hitstop": 6, "hit_sound": "hit_light", "limb": "far_fist", "frames": [4]},
    ('air', 'B'): {"name": "JUMP KNEE", "level": "overhead", "damage": 40, "guard_damage": 8, "hitstun": 14, "blockstun": 10, "pushback": 6, "meter": 6, "hitstop": 7, "hit_sound": "hit_light", "limb": "far_ankle", "frames": [3]},
    ('air', 'C'): {"name": "JUMP PUNCH", "level": "overhead", "damage": 66, "guard_damage": 12, "hitstun": 18, "blockstun": 12, "pushback": 8, "meter": 8, "hitstop": 9, "hit_sound": "hit_heavy", "limb": "far_fist", "frames": [4]},
    ('air', 'D'): {"name": "FLYING KICK", "level": "overhead", "damage": 72, "guard_damage": 12, "hitstun": 18, "blockstun": 12, "pushback": 8, "meter": 8, "hitstop": 9, "hit_sound": "hit_heavy", "limb": "near_ankle", "frames": [5]},
}


def roles(frames):
    """Split an animation into wind-up, active and recovery frame indices."""
    act = [i for i, f in enumerate(frames) if len(f) > 2 and 'active' in f[2]]
    if not act:
        return list(range(len(frames))), [], []
    return list(range(act[0])), act, list(range(act[-1] + 1, len(frames)))


def assign(frames, wind, strike, recover, strike2=None):
    w, a, r = roles(frames)
    out = [list(f) for f in frames]
    for i in w:
        out[i][0] = wind
    for n, i in enumerate(a):
        out[i][0] = strike2 if (strike2 and n % 2) else strike
    for i in r:
        # recovery frames that carry movement keep the strike pose (lunge follow-through)
        out[i][0] = strike if len(out[i]) > 3 and any(int(v) for v in out[i][3:]) else recover
    return out


def special_slot(c, name, moves=()):
    for k, (sp_name, *_rest) in enumerate(c['sp']):
        caps = ' '.join(w for w in sp_name.split() if w.upper() == w)
        if sp_name.upper().startswith(name.upper()) or name.upper() in sp_name.upper() or \
                (caps and name.upper().startswith(caps)):
            return k + 1
    leader = next((m for m in moves if m.get('follow') == name), None)
    return special_slot(c, leader['name']) if leader else None


def remap(path):
    d = json.loads(path.read_text())
    c = R[d['id']]
    anims, moves = d['anims'], d['moves']
    users = {}
    for m in moves:
        users.setdefault(m['anim'], []).append(m)
    new = {}
    for name, frames in anims.items():
        if name in STANDARD:
            new[name] = [list(f) for f in STANDARD[name]]
            continue
        ms = users.get(name, [])
        kinds = {m['kind'] for m in ms}
        if not ms:
            # follow-up animations (counter strikes, haymakers) belong to the special that leads into them
            leader = next((m for m in moves if m.get('follow') and any(x['anim'] == name for x in moves if x['name'] == m['follow'])), None)
            slot = special_slot(c, leader['name']) if leader else 3
            new[name] = assign(frames, f'sp{slot or 3}_1', f'sp{slot or 3}_2', f'sp{slot or 3}_1')
            continue
        m = ms[0]
        if 'normal' in kinds:
            st = m.get('stance', 'stand').split('|')[0]
            st = 'stand' if st not in ('crouch', 'air') else st
            wind, strike, rec = NORMAL[(st, m['buttons'][0])]
            new[name] = assign(frames, wind, strike, rec)
        elif 'super' in kinds:
            new[name] = assign(frames, 'super_1', 'super_2', 'hp1', strike2='super_3')
        elif 'throw' in kinds:
            out = [list(f) for f in frames]
            for i, f in enumerate(out):
                f[0] = 'throw_1' if i == 0 else 'throw_2'
            new[name] = out
        elif 'guard_attack' in kinds:
            new[name] = assign(frames, 'guard', 'gattack', 'guard')
        elif 'evade' in kinds:
            out = [list(f) for f in frames]
            for f in out:
                f[0] = 'roll' if len(f) > 2 and 'invuln' in f[2] else 'crouch'
            new[name] = out
        elif 'command' in kinds:
            new[name] = assign(frames, 'hk1', 'hk2', 'hk1')
        elif 'special' in kinds and any(m.get('projectile') for m in ms):
            new[name] = [['sp1_1'] + list(f[1:]) if i == 0 else ['sp1_2'] + list(f[1:]) for i, f in enumerate(frames)]
        elif 'special' in kinds:
            slot = special_slot(c, m['name'], moves)
            if slot is None:
                raise SystemExit(f"{d['id']}: no pose notes for special {m['name']}")
            if not any(len(f) > 2 and 'active' in f[2] for f in frames):
                # parries/stances: hold the wind-up drawing throughout
                new[name] = [[f'sp{slot}_1'] + list(f[1:]) for f in frames]
            else:
                new[name] = assign(frames, f'sp{slot}_1', f'sp{slot}_2', f'sp{slot}_1')
        else:
            raise SystemExit(f"{d['id']}: unclassified animation {name}")
    # complete the normal set
    have = {(m.get('stance', 'stand').split('|')[0], m['buttons'][0]) for m in moves
            if m['kind'] == 'normal' and m.get('buttons')}
    for key, t in TEMPLATE.items():
        if key in have:
            continue
        st, b = key
        anim = f'n_{st}_{b.lower()}'
        wind, strike, rec = NORMAL[key]
        fr = t['frames']
        if st == 'air':
            new[anim] = [[wind, 3], [strike, 255, 'active']]
        else:
            new[anim] = [[wind, fr[0]], [strike, fr[1], 'active'], [rec, fr[2]]]
        mv = {k: v for k, v in t.items() if k != 'frames'}
        mv.update(anim=anim, kind='normal', buttons=b, stance=st)
        moves.append(mv)
    d['anims'] = new
    used = {f[0] for frames in new.values() for f in frames}
    for p in sorted(used):
        d['poses'].setdefault(p, {'base': 'stance'})
    path.write_text(json.dumps(d, indent=2) + '\n')
    return d['id'], len(used), len(new), len(moves)


if __name__ == '__main__':
    root = Path(sys.argv[1]) / 'games/shadowleague/fighters'
    only = set(sys.argv[2:])
    for p in sorted(root.glob('*.json')):
        if only and p.stem not in only:
            continue
        print(*remap(p))
