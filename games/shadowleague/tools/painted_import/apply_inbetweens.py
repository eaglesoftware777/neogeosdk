"""Add painted in-between frames to fighters' moves without changing their timing.

    python3 apply_inbetweens.py SHEET_DIR ID [ID...]

SHEET_DIR/ID_s9.png: the guard stance, then the anticipation before each
special's start, the follow-through after each special's blow (special 1, 2,
3) and the super's last follow-through. SHEET_DIR/ID_s10.png: the stance,
then the heavy punch pulling back, the high kick folding down, the low kick
chambered, the sweep starting, the throw's slam, a landing and a second dash
stride. Paintings are kept at half size in assets/authored/sdk177/sources/
inbetween/, frames come in through import_extra.py, and each animation that
uses the old pictures gets the new ones by splitting a frame's time: a start
picture's time is shared with its anticipation, a recovery picture's with the
follow-through, so every move keeps its startup, active and recovery counts
(and its hit boxes, which ride on the active frames only).
"""
import json
import subprocess
import sys
from pathlib import Path

from PIL import Image

TOOLS = Path(__file__).resolve().parent
GAME = TOOLS.parents[1]
SOURCES = GAME / 'assets/authored/sdk177/sources/inbetween'
SHEETS = {
    's9': ['sp1_0', 'sp1_3', 'sp2_0', 'sp2_3', 'sp3_0', 'sp3_3', 'super_4'],
    's10': ['hp3', 'hk3', 'lk1', 'cr_hk0', 'throw_3', 'land', 'dash2'],
}


def keep_source(sheet, fid, tag):
    SOURCES.mkdir(parents=True, exist_ok=True)
    im = Image.open(sheet).convert('RGB')
    im = im.resize((im.width // 2, im.height // 2), Image.LANCZOS)
    out = SOURCES / f'{fid}_{tag}.png'
    im.save(out, optimize=True)
    return out


# Flags that act once, on entering the frame (ng_fight enter_frame): the
# airborne launch, a projectile and a fresh hit. Only the first half of a
# split frame keeps them, so a split never launches or fires twice.
ONCE = ('airborne', 'spawn', 'rehit')


def once_removed(rest):
    rest = list(rest)
    if rest and isinstance(rest[0], str):
        rest[0] = '|'.join(f for f in rest[0].split('|') if f and f not in ONCE)
    return rest


def split(frame, first_pose, share):
    """[pose, t, ...] -> first_pose for `share` of t (at least 1), then the original for the rest."""
    t = int(frame[1])
    if t < 2 or t == 255:
        return [frame]
    a = max(1, min(t - 1, round(t * share)))
    return [[first_pose, a] + list(frame[2:]), [frame[0], t - a] + once_removed(frame[2:])]


def split_after(frame, second_pose, share):
    """[pose, t, ...] -> the original for `share` of t, then second_pose for the rest."""
    t = int(frame[1])
    if t < 2 or t == 255:
        return [frame]
    a = max(1, min(t - 1, round(t * share)))
    return [[frame[0], a] + list(frame[2:]), [second_pose, t - a] + once_removed(frame[2:])]


def is_active(frame):
    return len(frame) > 2 and 'active' in str(frame[2])


def rework(anim, new):
    """The anim's frames with the in-betweens slotted in (timing unchanged)."""
    out = []
    n = len(anim)
    # A move that leaves the floor lands into the frame after the current one
    # (ng_fight land()): its recovery stays one frame, or landing would take longer.
    airborne = any(len(f) > 2 and 'airborne' in str(f[2]) for f in anim)
    for i, fr in enumerate(anim):
        pose = fr[0]
        prev_active = i > 0 and is_active(anim[i - 1])
        if i == 0 and not is_active(fr):
            # a start: anticipation first
            for base, ant in (('sp1_1', 'sp1_0'), ('sp2_1', 'sp2_0'), ('sp3_1', 'sp3_0')):
                if pose == base and ant in new:
                    out.extend(split(fr, ant, 0.5)); break
            else:
                if pose == 'hk1' and 'lk1' in new and n > 1 and anim[1][0] == 'lk2':
                    out.append(['lk1'] + list(fr[1:]))          # the low kick's own chamber
                elif pose == 'crouch' and 'cr_hk0' in new and n > 1 and anim[1][0] == 'cr_hk':
                    out.extend(split_after(fr, 'cr_hk0', 0.5))    # crouched, then the leg starts its arc
                else:
                    out.append(fr)
            continue
        if prev_active and not is_active(fr) and not airborne:
            # a recovery after the blow: the follow-through first
            done = False
            for blow, follow in (('sp1_2', 'sp1_3'), ('sp2_2', 'sp2_3'), ('sp3_2', 'sp3_3'), ('super_3', 'super_4'),
                                 ('hp2', 'hp3'), ('hk2', 'hk3'), ('lk2', 'lk1'), ('cr_hk', 'cr_hk0')):
                if anim[i - 1][0] == blow and follow in new:
                    out.extend(split(fr, follow, 0.6)); done = True; break
            if not done:
                out.append(fr)
            continue
        out.append(fr)
    return out


def apply(fid, sheet_dir):
    new = []
    for tag, names in SHEETS.items():
        sheet = Path(sheet_dir) / f'{fid}_{tag}.png'
        if not sheet.exists():
            print(f'{fid}: no {sheet.name}, skipped')
            continue
        src = keep_source(sheet, fid, tag)
        subprocess.run([sys.executable, str(TOOLS / 'import_extra.py'), fid, str(src), ','.join(names), '--replace'],
                       check=True)
        new.extend(names)
    if not new:
        return
    path = GAME / 'fighters' / f'{fid}.json'
    d = json.loads(path.read_text())
    for n in new:
        d['poses'].setdefault(n, {'base': 'stance'})
    changed = 0
    for name, anim in d['anims'].items():
        if name in ('win', 'lose', 'intro', 'idle', 'walk_f', 'walk_b'):
            continue
        before = sum(int(f[1]) for f in anim if int(f[1]) != 255)
        after_anim = rework(anim, set(new))
        after = sum(int(f[1]) for f in after_anim if int(f[1]) != 255)
        if before != after:
            raise ValueError(f'{fid}/{name}: timing changed {before} -> {after}')
        if after_anim != anim:
            d['anims'][name] = after_anim; changed += 1
    # the throw's slam: the hurl's frame shares its time with it
    if 'throw_3' in new and 'throw' in d['anims']:
        th = d['anims']['throw']
        if th and th[-1][0] == 'throw_2':
            d['anims']['throw'] = th[:-1] + split_after(th[-1], 'throw_3', 0.5)
            changed += 1
    # getting up ends through the landing crouch; the dash takes a second stride
    if 'land' in new and d['anims'].get('getup') and d['anims']['getup'][-1][0] == 'crouch':
        g = d['anims']['getup']
        d['anims']['getup'] = g[:-1] + split(g[-1], 'land', 0.5)
        changed += 1
    if 'dash2' in new and d['anims'].get('dash') == [['dash', 255]]:
        d['anims']['dash'] = [['dash', 6], ['dash2', 6]]   # (the dash loops: fighter_cook _loops)
        changed += 1
    path.write_text(json.dumps(d, indent=2) + '\n')
    print(fid, 'in-betweens applied,', changed, 'animations')


if __name__ == '__main__':
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    for fid in sys.argv[2:]:
        apply(fid, sys.argv[1])
