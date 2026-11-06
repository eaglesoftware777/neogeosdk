"""Render the Piper half of the localized lines (voice_locales.py, engine 'piper').

    python3 games/shadowleague/tools/voices_localized_piper.py [--only ID...]
        [--piper ~/.cache/slf-voice/venv/bin/python] [--models ~/.cache/slf-voice/models]

Each model's lines are spoken in one batch by voices_tts.py in the Piper
environment, then shaped like every other line (voices.shout) and written to
assets/authored/voices/<key>.wav; localized-recordings.json gets their entries.
"""
import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
from voices import read, shout, write  # noqa: E402
from voice_locales import LOCALES  # noqa: E402
import slf_voices  # noqa: E402

HOME = Path.home() / '.cache/slf-voice'


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--piper', default=str(HOME / 'venv/bin/python'))
    ap.add_argument('--models', type=Path, default=HOME / 'models')
    ap.add_argument('--only', nargs='*')
    a = ap.parse_args()
    jobs = {}
    for entry in slf_voices.plan(slf_voices.load_fighters()):
        who = entry.get('fighter'); locale = LOCALES.get(who)
        if not locale or locale.get('engine') != 'piper' or (a.only and who not in a.only):
            continue
        key = entry['key'].rsplit('_', 1)[-1]
        if key in locale['lines']:
            jobs.setdefault(locale['model'], []).append((entry, locale, locale['lines'][key]))
    result = []
    with tempfile.TemporaryDirectory(prefix='slf-locale-') as tmp:
        for model, items in jobs.items():
            batch = Path(tmp) / f'{model}.json'
            batch.write_text(json.dumps([{'key': e['key'], 'text': text, 'speaker': loc.get('speaker')}
                                         for e, loc, text in items], ensure_ascii=False))
            subprocess.run([a.piper, '-I', str(TOOLS / 'voices_tts.py'), str(a.models / f'{model}.onnx'),
                            str(batch), tmp], check=True)
            for e, loc, text in items:
                x, sr = read(Path(tmp) / f"{e['key']}.wav")
                if not len(x) or not np.isfinite(x).all():
                    raise ValueError(e['key'] + ': invalid speech output')
                pcm = shout(x, sr, e)
                write(slf_voices.VOICES / f"{e['key']}.wav", pcm)
                item = {'key': e['key'], 'fighter': e['fighter'], 'language': loc['language'], 'voice': loc['voice'],
                        'text': text, 'duration': len(pcm) / 18519, 'peak': float(np.abs(pcm).max())}
                result.append(item)
                print(json.dumps(item, ensure_ascii=False), flush=True)
    path = slf_voices.VOICES / 'localized-recordings.json'
    keys = {r['key'] for r in result}
    old = json.loads(path.read_text()) if path.exists() else []
    merged = [r for r in old if r['key'] not in keys] + result
    path.write_text(json.dumps(sorted(merged, key=lambda r: r['key']), ensure_ascii=False, indent=2) + '\n')


if __name__ == '__main__':
    main()
