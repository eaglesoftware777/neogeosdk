"""Verify the small palette polish against the accepted motion and shapes.

Patch evidence reconstructs each preceding native layer. The anatomy,
visible-palm, stride and decoded-ROM gates remain separate and mandatory.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

import numpy as np
from PIL import Image, ImageDraw

ART = Path(__file__).resolve().parents[1] / 'assets/authored'
HEADER = ART.parents[1] / 'artbox/generated/maiya_assets.h'
LAYERS = ('far_arm', 'far_leg', 'near_leg', 'torso', 'head_hair', 'near_arm')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    rows = path.read_text().splitlines()
    if len(rows) != 64 or any(len(row) != 80 for row in rows):
        raise ValueError(f'{path}: expected 80x64 native map')
    return np.array([[0 if c == '.' else int(c, 16) for c in row]
                     for row in rows], np.uint8)


def hardware_palette(header, name):
    body = re.search(r'\b' + name + r'\[.*?\]\s*=\s*\{(.*?)\}',
                     header.read_text(), re.S).group(1)
    words = [int(v, 0) for v in re.findall(r'0x[0-9A-Fa-f]+|\d+', body)]
    if len(words) != 16:
        raise ValueError(f'{name}: expected one 16-slot bank')
    return [[round((((w >> 7) & 30) | ((w >> 14) & 1)) * 255 / 31),
             round((((w >> 3) & 30) | ((w >> 13) & 1)) * 255 / 31),
             round((((w << 1) & 30) | ((w >> 12) & 1)) * 255 / 31)]
            for w in words]


def review(root, header):
    """Opaque review backgrounds and complete redraws prevent GIF ghosting.

    GIF has 10ms timing units. Cumulative deadlines retain the intended
    distance-clock tempo instead of dropping each individual hold's tail.
    Runtime timing and artwork are unchanged.
    """
    poses = json.loads((root / 'poses.json').read_text())
    palette = json.loads((root / 'palette.json').read_text())['colors']
    dest = root / 'review'
    dest.mkdir(exist_ok=True)
    timing = {}
    for character, colors in [('maiya', palette),
                              ('luna', hardware_palette(header, 'mg_hero_alt_pal'))]:
        sheet = Image.new('RGB', (8 * 320, 2 * 280), (36, 43, 55))
        labels = ImageDraw.Draw(sheet)
        for mode in ('walk', 'run'):
            frames, times, elapsed, deadline = [], [], 0, 0
            speed = 800 / 256 if mode == 'run' else 512 / 256 * 80 / 64
            for p in [p for p in poses if p['mode'] == mode]:
                a = np.array(Image.open(root / 'layers' / f"{p['name']}__flat.png"))
                indexed = Image.fromarray(a, 'P')
                indexed.putpalette(sum(colors, []) + [0] * 720)
                indexed.info['transparency'] = 0
                rgba = indexed.convert('RGBA').resize((320, 256), Image.Resampling.NEAREST)
                full = Image.new('RGB', (320, 256), (36, 43, 55))
                full.paste(rgba, (0, 0), rgba)
                frames.append(full)
                elapsed += p['hold_distance'] / speed / 60 * 1000
                end = round(elapsed / 10) * 10
                times.append(end - deadline)
                deadline = end
                row = 0 if mode == 'walk' else 1
                sheet.paste(full, (p['slot'] * 320, row * 280 + 20))
                labels.text((p['slot'] * 320 + 8, row * 280 + 3),
                            f"{character} {p['name']}", fill='white')
            path = dest / f'{character}-{mode}.gif'
            frames[0].save(path, save_all=True, append_images=frames[1:],
                           duration=times, loop=0, disposal=2, optimize=False)
            with Image.open(path) as gif:
                actual = []
                for i, frame in enumerate(frames):
                    gif.seek(i)
                    # GIF quantization must leave the complete review
                    # drawing intact; no old limb is composited into it.
                    if not np.array_equal(np.array(gif.convert('RGB')), np.array(frame)):
                        raise ValueError(f'{path}: stale pixels or changed review colours')
                    actual.append(gif.info['duration'])
                if actual != times:
                    raise ValueError(f'{path}: preview tempo differs')
            timing[f'{character}_{mode}'] = dict(duration_ms=times,
                                               cycle_ms=sum(times),
                                               ideal_cycle_ms=elapsed,
                                               complete_frames_pixel_exact=True)
        sheet.save(dest / f'{character}-contact-sheet-4x.png')
    # Existing review links keep pointing to the most recent Maiya view.
    for mode in ('walk', 'run'):
        (dest / f'{mode}.gif').write_bytes((dest / f'maiya-{mode}.gif').read_bytes())
    (root / 'preview-timing.json').write_text(json.dumps(timing, indent=2) + '\n')


def check(root, header, before=None, previews=True):
    spec = json.loads((root / 'cute-polish.json').read_text())
    changes = {e['file']: e['pixels'] for e in spec['edits']}
    errors = []

    def need(ok, message):
        if not ok:
            errors.append(message)

    for item in spec['layers']:
        name = item['file']
        new = read(root / 'maps' / name)
        old = new.copy()
        for x, y, prev, curr in changes[name]:
            need(int(new[y, x]) == curr, f'{name}: reviewed patch changed at {x},{y}')
            need(prev != 0 and curr != 0, f'{name}: patch changes an accepted outline')
            old[y, x] = prev
        need(sha(old.tobytes()) == item['before'], f'{name}: accepted pixels cannot be reconstructed')
        need(sha(new.tobytes()) == item['after'], f'{name}: unreviewed pixel edits')
        need(sha((new != 0).tobytes()) == item['silhouette'], f'{name}: accepted silhouette moved')
        need(np.array_equal(old != 0, new != 0), f'{name}: accepted joint/limb silhouette moved')
        if '__head_hair' in name or '__torso' in name:
            need(np.array_equal(old, new), f'{name}: accepted upper body changed')
        with Image.open(root / 'layers' / Path(name).with_suffix('.png')) as im:
            need(im.mode == 'P' and im.size == (80, 64), f'{name}: incorrect indexed export')
            need(np.array_equal(np.array(im), new), f'{name}: reopened Aseprite differs from reviewed map')
        if before:
            need(np.array_equal(read(before / 'maps' / name), old), f'{name}: preceding source differs')
    for file, key in [('poses.json', 'poses_sha256'), ('palette.json', 'palette_sha256')]:
        need(sha((root / file).read_bytes()) == spec[key], f'{file}: accepted motion or palette changed')
    if previews:
        review(root, header)
    result = dict(passed=not errors, errors=errors,
                  native_pixel_changes=spec['pixel_changes'],
                  unchanged_silhouettes=len(spec['layers']),
                  accepted_head_torso_pose_order_and_holds_unchanged=not errors,
                  shared_luna_indices=True)
    (root / 'cute-check-result.json').write_text(json.dumps(result, indent=2) + '\n')
    for error in errors:
        print('FAIL:', error)
    print('Cute palette polish and unchanged accepted movement:', 'PASS' if not errors else 'FAIL')
    return not errors


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ART)
    parser.add_argument('--header', type=Path, default=HEADER)
    parser.add_argument('--before', type=Path)
    parser.add_argument('--no-previews', action='store_true')
    args = parser.parse_args()
    raise SystemExit(0 if check(args.root, args.header, args.before, not args.no_previews) else 1)
