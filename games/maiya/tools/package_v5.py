"""Build separate V5 test cartridges; optionally refresh only Maiya in MAME.

The destination must be new. Released cartridges and launch settings are
preserved; a legacy ROM search path is corrected when needed. Both program
variants are built before an installed ROM is replaced.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
WORK = GAME / 'build/workspace'
sys.path.insert(0, str(ROOT / 'tools'))
from build_maiya_mame import PARTS, verify_image, verify_set
from pack_neosd import build_image
from pack_darksoft import build as darksoft, unpack
from pack_backbit import build as backbit


def archive(path, files):
    with zipfile.ZipFile(path, 'x', zipfile.ZIP_DEFLATED) as dest:
        for name, data in files.items():
            dest.writestr(name, data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--install', type=Path)
    args = parser.parse_args()
    dest = args.out.resolve()
    dest.mkdir(parents=True, exist_ok=False)
    manifest = {'version': 'v5-test', 'hardware_approval': 'pending', 'roms': {}}
    for mode in ('mvs', 'aes'):
        with (dest / (mode + '-build.log')).open('w') as log:
            subprocess.run([sys.executable, str(GAME / 'tools/build.py'),
                            '--platform', mode, '--debug'], cwd=ROOT,
                           stdout=log, stderr=subprocess.STDOUT, check=True)
        folder = dest / 'tests' / mode
        roms = folder / 'roms/maiya'
        roms.mkdir(parents=True)
        (folder / 'hash').mkdir()
        for part in PARTS:
            name = f'780-{part}.{part}'
            shutil.copy2(WORK / 'roms/maiya' / name, roms / name)
        shutil.copy2(WORK / 'hash_eagle/maiya/neogeo.xml', folder / 'hash/neogeo.xml')
        shutil.copy2(WORK / 'out/game', folder / 'game.elf')
        verify_set(ROOT, roms, folder / 'hash/neogeo.xml')
        parts = {p: (roms / f'780-{p}.{p}').read_bytes() for p in PARTS}
        image = build_image(parts, 'Maiya: Super Nature Girl V5', 'Eagle Software', 2026, 0, 0x780)
        verify_image(image, parts)
        (folder / f'Maiya-{mode.upper()}.neo').write_bytes(image)
        (dest / f'Maiya-NeoSD_{mode.upper()}_v5_TEST.neo').write_bytes(image)
        archive(dest / f'Maiya-MAME-{mode.upper()}_v5_TEST.zip',
                {f'780-{p}.{p}': data for p, data in parts.items()})
        shutil.copy2(dest / f'Maiya-MAME-{mode.upper()}_v5_TEST.zip', folder / 'roms/maiya.zip')
        ds = darksoft(roms, '780', mode.upper(), 'Maiya V5')
        # The portable manifest describes the archive, not the build host.
        dm = json.loads(ds['MANIFEST.json'])
        dm['source'] = 'maiya/780-*'
        ds['MANIFEST.json'] = (json.dumps(dm, indent=2) + '\n').encode()
        restored = unpack(ds, '780')
        assert all(restored[f'780-{p}.{p}'] == data for p, data in parts.items())
        archive(dest / f'Maiya-Darksoft-{mode.upper()}_v5_TEST.zip',
                {'maiya/' + name: data for name, data in ds.items()})
        if mode == 'mvs':
            archive(dest / 'Maiya-BackBit-MVS_v5_TEST.zip',
                    backbit(parts, 'maiya', 'Maiya V5', 'MVS'))
        manifest['roms'][mode] = {f'780-{p}.{p}': hashlib.sha256(data).hexdigest()
                                  for p, data in parts.items()}
        print('Built and round-trip checked ' + mode.upper(), flush=True)
    (dest / 'maiya-v5-build.json').write_text(json.dumps(manifest, indent=2) + '\n')
    if args.install:
        target = args.install.resolve()
        # No broad directory replacement: other games and configuration survive.
        for mode in ('mvs', 'aes'):
            source = dest / 'tests' / mode
            installed = target / 'tests' / mode
            shutil.copytree(source / 'roms/maiya', installed / 'roms/maiya', dirs_exist_ok=True)
            # MAME can prefer an archive over a same-named loose folder.
            shutil.copy2(source / 'roms/maiya.zip', installed / 'roms/maiya.zip')
            (installed / 'hash').mkdir(parents=True, exist_ok=True)
            shutil.copy2(source / 'hash/neogeo.xml', installed / 'hash/neogeo.xml')
            for name in ('game.elf', f'Maiya-{mode.upper()}.neo'):
                shutil.copy2(source / name, installed / name)
            verify_set(ROOT, installed / 'roms/maiya', installed / 'hash/neogeo.xml')
        shutil.copytree(dest / 'tests/mvs/roms/maiya', target / 'roms/maiya', dirs_exist_ok=True)
        shutil.copy2(dest / 'tests/mvs/roms/maiya.zip', target / 'roms/maiya.zip')
        (target / 'hash/maiya').mkdir(parents=True, exist_ok=True)
        shutil.copy2(dest / 'tests/mvs/hash/neogeo.xml', target / 'hash/maiya/neogeo.xml')
        shutil.copy2(dest / 'maiya-v5-build.json', target / 'maiya-v5-build.json')
        launcher = target / 'run_maiya.bat'
        if launcher.is_file():
            data = launcher.read_bytes()
            old = b'set "ROMS=%HERE%roms;%HERE%..\\roms"'
            new = b'set "ROMS=%DATA%\\roms;%HERE%roms;%HERE%..\\roms"'
            if old in data:
                # Older launchers share an MVS search path in AES mode.
                # Change that one ROM path, preserving every other byte.
                launcher.write_bytes(data.replace(old, new, 1))
        print('Refreshed Maiya ROMs; other games and launcher settings preserved', flush=True)


if __name__ == '__main__':
    main()
