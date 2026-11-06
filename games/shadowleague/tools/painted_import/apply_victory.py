"""Give fighters their own victory animation from a painted victory sheet.

    python3 apply_victory.py SHEET_DIR ID [ID...]

SHEET_DIR/ID_s8.png is a sheet of 8 figures: the guard stance (the scale
key), then the victory in seven figures, the last two a small loop (a breath,
a nod). The painting is kept, at half size, in
assets/authored/sdk177/sources/victory/ (the reduction never needs more),
its seven figures become frames win1..win7 in the fighter's own colours
(import_extra.py), and the fighter's "win" animation plays them: five once,
then the last two in turn for as long as the win lasts.
"""
import json
import subprocess
import sys
from pathlib import Path

from PIL import Image

TOOLS = Path(__file__).resolve().parent
GAME = TOOLS.parents[1]
SOURCES = GAME / 'assets/authored/sdk177/sources/victory'
NAMES = [f'win{i}' for i in range(1, 8)]
TIMES = [6, 7, 8, 10, 10, 18, 18]          # frames each; the last two loop
LOOP_FROM = 5


def keep_source(sheet, fid):
    SOURCES.mkdir(parents=True, exist_ok=True)
    im = Image.open(sheet).convert('RGB')
    im = im.resize((im.width // 2, im.height // 2), Image.LANCZOS)
    out = SOURCES / f'{fid}_s8.png'
    im.save(out, optimize=True)        # (no text chunks: PIL writes none unless asked)
    return out


def apply(fid, sheet_dir):
    sheet = Path(sheet_dir) / f'{fid}_s8.png'
    if not sheet.exists():
        raise SystemExit(f'{fid}: no {sheet}')
    src = keep_source(sheet, fid)
    subprocess.run([sys.executable, str(TOOLS / 'import_extra.py'), fid, str(src), ','.join(NAMES), '--replace'], check=True)
    path = GAME / 'fighters' / f'{fid}.json'
    d = json.loads(path.read_text())
    for n in NAMES:
        d['poses'].setdefault(n, {'base': 'stance'})
    d['anims']['win'] = [[n, t] for n, t in zip(NAMES, TIMES)]
    d.setdefault('anim_loops', {})['win'] = LOOP_FROM
    path.write_text(json.dumps(d, indent=2) + '\n')
    print(fid, 'victory applied')


if __name__ == '__main__':
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    for fid in sys.argv[2:]:
        apply(fid, sys.argv[1])
