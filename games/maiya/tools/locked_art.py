"""Copy the approved V4 graphics without authoring, palette fitting or export."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys

GAME = Path(__file__).resolve().parents[1]
REFERENCE = GAME / 'artbox/v4_reference'
GENERATED = GAME / 'artbox/generated'
sys.path.insert(0,str(GAME/'tools'))
from maiya_posture import banks,recipe


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
    expected_banks=dict(zip(('780-c1.c1','780-c2.c2'),recipe()['graphics_sha256']))
    banks()  # Reconstruct and validate the finite permitted posture edits.
    for name, expected in lock['files'].items():
        leaf = Path(name).name
        if leaf in expected_banks: expected=dict(expected,sha256=expected_banks[leaf])
        if leaf != '780-s1.s1':
            verify_file(GENERATED / leaf, expected)
    return lock


def install():
    lock = verify_reference()
    GENERATED.mkdir(parents=True, exist_ok=True)
    roms = GAME.parents[1] / 'roms/maiya'
    roms.mkdir(parents=True, exist_ok=True)
    corrected=dict(zip(('780-c1.c1','780-c2.c2'),banks()))
    for name in lock['files']:
        source = GAME / name
        if source.name == '780-s1.s1':
            shutil.copyfile(source, roms / source.name)
        else:
            if source.name in corrected: (GENERATED/source.name).write_bytes(corrected[source.name])
            else: shutil.copyfile(source, GENERATED / source.name)
            if source.suffix in ('.c1', '.c2'):
                shutil.copyfile(GENERATED/source.name, roms / source.name)
    check_generated()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Verify only; never rewrite the reference')
    args = parser.parse_args()
    check_generated() if args.check else install()
    print('Artwork lock: PASS (V4 reference preserved; exact requested back-contour correction verified)')


if __name__ == '__main__':
    main()
