"""Copy the approved V4 graphics without authoring, palette fitting or export."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil

GAME = Path(__file__).resolve().parents[1]
REFERENCE = GAME / 'artbox/v4_reference'
GENERATED = GAME / 'artbox/generated'


def verify_file(path, expected):
    data = path.read_bytes()
    if len(data) != expected['size'] or hashlib.sha256(data).hexdigest() != expected['sha256']:
        raise ValueError('Approved V4 artwork differs: ' + path.name)


def verify_reference():
    lock = json.loads((REFERENCE / 'lock.json').read_text())
    for name, expected in lock['files'].items():
        verify_file(GAME / name, expected)
    return lock


def check_generated():
    lock = verify_reference()
    for name, expected in lock['files'].items():
        leaf = Path(name).name
        if leaf != '780-s1.s1':
            verify_file(GENERATED / leaf, expected)
    return lock


def install():
    lock = verify_reference()
    GENERATED.mkdir(parents=True, exist_ok=True)
    roms = GAME.parents[1] / 'roms/maiya'
    roms.mkdir(parents=True, exist_ok=True)
    for name in lock['files']:
        source = GAME / name
        if source.name == '780-s1.s1':
            shutil.copyfile(source, roms / source.name)
        else:
            shutil.copyfile(source, GENERATED / source.name)
            if source.suffix in ('.c1', '.c2'):
                shutil.copyfile(source, roms / source.name)
    check_generated()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Verify only; never rewrite the reference')
    args = parser.parse_args()
    check_generated() if args.check else install()
    print('V4 artwork lock: PASS (graphics, FIX, palettes and tile metadata unchanged)')


if __name__ == '__main__':
    main()
