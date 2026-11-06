"""Export read-only route diagrams from the validated, current stage JSONs.

python3 games/maiya/tools/review_levels.py --out /path/to/review
These diagrams expose authored positions, not proven reachable player routes.
They contain no game artwork and never rewrite the compiled level header.
"""
import argparse
from html import escape
from pathlib import Path
import re

import levels


def diagram(data, ground):
    top = min([-64] + [p['y'] - 48 for p in data.get('platforms', [])])
    width = data['width']
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" '
           f'height="{ground - top + 112}" viewBox="0 {top} {width} {ground - top + 112}">',
           '<style>text{font:12px monospace;fill:#e8eff5} '
           '.grid{stroke:#344454;stroke-width:1}</style>',
           f'<rect x="0" y="{top}" width="{width}" height="{ground-top+112}" fill="#14212e"/>']

    def rect(x, y, w, h, color):
        out.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{color}"/>')

    def label(x, y, text):
        out.append(f'<text x="{x}" y="{y}">{escape(str(text))}</text>')

    for x in range(0, width, 320):
        out.append(f'<path d="M{x},{top}V{ground+40}" class="grid"/>')
        label(x + 4, ground + 60, x)
    rect(0, ground, width, 8, '#52735d')
    for i, p in enumerate(data.get('platforms', [])):
        # Same rounded 32px caps and four-pixel forgiveness as
        # maiya_collision.h; authored width remains separately visible.
        drawn = max(1, (p['w'] + 16) >> 5) * 32
        rect(p['x'] - 4, p['y'] - 2, drawn + 8, 4, '#e7c56c')
        rect(p['x'], p['y'], drawn, 8, '#76a68a')
        rect(p['x'], p['y'] + 8, p['w'], 2, '#becbd7')
        label(p['x'], p['y'] - 8, f'P{i}')
    for i, h in enumerate(data.get('hazards', [])):
        rect(h['x'], ground - 16, h['w'], 24, '#a64c59')
        label(h['x'], ground + 26, f"H{i}:{h['type']}")
    for i, e in enumerate(data.get('encounters', [])):
        rect(e['x'], ground - 28, 3, 28, '#ec925b')
        label(e['x'], ground + 42 + (i % 2) * 14, f"E{i}:{e['enemy']}")
    for kind, color in [('pickups', '#89cce0'), ('secrets', '#d4a5ed')]:
        for i, item in enumerate(data.get(kind, [])):
            # These coordinates are the icon's top-left, not its centre.
            # Show the same 32px box the level checker measures.
            out.append(f'<rect x="{item["x"]}" y="{item["y"]}" width="32" height="32" '
                       f'fill="{color}" fill-opacity="0.18" stroke="{color}"/>')
            rect(item['x'] + 14, item['y'] + 14, 4, 4, color)
            label(item['x'] + 34, item['y'] + 16, f'{kind[0].upper()}{i}')
    if data.get('key'):
        k = data['key']; rect(k['x']-4, k['y']-4, 8, 8, '#ffe380')
        label(k['x'] + 8, k['y'], 'KEY')
    gate = data.get('gate_x', 0)
    if gate:
        rect(gate, ground - 64, 4, 64, '#ffe380')
        label(gate + 8, ground - 54, 'GATE')
    out.append('</svg>')
    return '\n'.join(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    files, _, data = levels.load(with_data=True)
    header = (levels.GAME / 'scenes/maiya_levels.h').read_text()
    ground = int(re.search(r'^#define MG_GROUND_Y\s+(\d+)', header, re.M)[1])
    args.out.mkdir(parents=True, exist_ok=True)
    index = ['# Current Maiya route geometry', '',
             'Validated source positions; these are not reachability or playtest results.', '',
             f'Ground: {ground}px. Grid: 320px. Green: drawn ledges; gold: foot support;',
             'white: authored width; red: hazards; orange: encounter triggers;',
             'blue: pickups; purple: secrets (32px icon bounds, centre marked).',
             'Horizontal scrolling preserves world scale.', '']
    for path, d in zip(files, data):
        name = path.stem + '.svg'
        (args.out / name).write_text(diagram(d, ground), encoding='utf-8')
        index += [f"- [{d['name']}]({name}): {d['width']}px, {d['mechanic']}, "
                  f"{len(d.get('platforms', []))} ledges, {len(d.get('encounters', []))} encounters."]
    (args.out / 'README.md').write_text('\n'.join(index) + '\n', encoding='utf-8')
    print(f'Exported {len(files)} route diagrams to {args.out}')


if __name__ == '__main__':
    main()
