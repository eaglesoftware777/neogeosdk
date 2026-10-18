"""Package the reviewed candidate without overwriting any released cartridge.

Every output is decoded and compared against the candidate's six source ROMs.
This checks packaging integrity; physical cartridge testing remains separate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools'))
import pack_backbit
import pack_darksoft
import pack_neosd

PARTS = ('p1', 's1', 'm1', 'v1', 'c1', 'c2')


def verify_neo(data, original):
    if data[:4] != b'NEO\x01':
        raise ValueError('Invalid NeoSD header')
    p, s, m, v, v2, c = struct.unpack_from('<6I', data, 4)
    if v2 or len(data) != 4096 + p + s + m + v + c:
        raise ValueError('Invalid NeoSD region offsets')
    cursor = 4096
    for part, size in zip(PARTS[:4], (p, s, m, v)):
        region = data[cursor:cursor + size]
        if region[:len(original[part])] != original[part] or set(region[len(original[part]):]) - {255}:
            raise ValueError('NeoSD data/padding mismatch: ' + part)
        cursor += size
    interleaved = data[cursor:]
    for lane, part in enumerate(('c1', 'c2')):
        region = interleaved[lane::2]
        if region[:len(original[part])] != original[part] or set(region[len(original[part]):]) - {255}:
            raise ValueError('NeoSD lane mismatch: ' + part)


def write_zip(path, files):
    with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED) as archive:
        for name, data in sorted(files.items()):
            archive.writestr(name, data)
    with zipfile.ZipFile(path) as archive:
        if archive.testzip() is not None:
            raise ValueError('ZIP integrity failure: ' + str(path))
        return {name: archive.read(name) for name in archive.namelist()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', type=Path, default=ROOT / 'dist/candidates/shadowleague/sdk177')
    parser.add_argument('--out', type=Path)
    args = parser.parse_args()
    output = args.out or args.candidate / 'hardware-v1'
    output.mkdir(parents=True, exist_ok=True)
    checks = {}
    for platform in ('aes', 'mvs'):
        directory = args.candidate / platform / 'roms/shadowleague'
        roms = {part: (directory / f'781-{part}.{part}').read_bytes() for part in PARTS}
        if len(roms['p1']) % 0x100000 or len(roms['c1']) != len(roms['c2']):
            raise ValueError('Unaligned program banks or unequal sprite lanes')
        suffix = platform.upper() + '_v1'
        neo = pack_neosd.build_image(roms, 'Shadow League Fighting V1', 'Eagle Software', 2026, 5, 0x781)
        neo_path = output / ('ShadowLeague-NeoSD-' + suffix + '.neo')
        neo_path.write_bytes(neo)
        verify_neo(neo_path.read_bytes(), roms)
        mame_files = {f'shadowleague/781-{part}.{part}': roms[part] for part in PARTS}
        for xml in (args.candidate / platform / 'hash').glob('*.xml'):
            mame_files['hash/' + xml.name] = xml.read_bytes()
        zipped = write_zip(output / ('ShadowLeague-MAME-' + suffix + '.zip'), mame_files)
        if zipped != mame_files:
            raise ValueError('MAME ZIP changed ROM bytes')
        dark = pack_darksoft.build(directory, '781', platform.upper(), 'Shadow League Fighting V1')
        folder = 'ShadowLeague_' + platform.upper()
        dark_path = output / ('ShadowLeague-Darksoft-' + suffix + '.zip')
        write_zip(dark_path, {folder + '/' + name: data for name, data in dark.items()})
        restored = pack_darksoft.unpack(pack_darksoft.read_folder(dark_path), '781')
        if any(restored[f'781-{part}.{part}'] != roms[part] for part in PARTS):
            raise ValueError('Darksoft decode changed ROM bytes')
        if platform == 'mvs':
            back = pack_backbit.build(roms, 'shadowleague', 'Shadow League Fighting V1', 'MVS')
            restored = write_zip(output / ('ShadowLeague-BackBit-' + suffix + '.zip'), back)
            if any(restored[f'shadowleague/shadowleague.{part}'] != roms[part] for part in PARTS):
                raise ValueError('BackBit decode changed ROM bytes')
        checks[platform] = {'all_six_roms_roundtrip': True,
                            'roms': {part: {'size': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
                                     for part, data in roms.items()}}
    (output / 'README.txt').write_text(
        'SHADOW LEAGUE FIGHTING V1 -- SDK fighting component 1.7.7\n\n'
        'Hardware-test candidate; emulator QA is recorded separately.\n'
        'Physical NeoSD, Darksoft and BackBit tests have not yet been performed.\n'
        'Choose the package matching your cartridge and AES/MVS system.\n'
        'BackBit package is MVS only. No system BIOS is included.\n'
        'MAME packages require the matching candidate software-list XML.\n\n'
        'Difficulty: seven levels, default 3. MVS: SOFTWARE DIP.\n'
        'AES: OPTIONS; TRAINING includes a command list and dummy controls.\n'
        'A/B light strikes; C/D heavy strikes; close forward/back+C/D throws.\n'
        '1 versus 1 and team 3 versus 3 with striker support.\n\n'
        'Known follow-up work: arena art/depth, animation and movement polish,\n'
        'reported occasional slowdown/flicker, complete native-language voices.\n'
        'Technical art checks do not establish independent visual approval.\n', encoding='utf-8')
    files = {path.name: {'size': path.stat().st_size, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
             for path in sorted(output.iterdir()) if path.is_file() and path.name != 'MANIFEST.json'}
    (output / 'MANIFEST.json').write_text(json.dumps({'version': 'V1', 'sdk_fighting': '1.7.7',
        'physical_hardware_tested': False, 'roundtrip': checks, 'files': files}, indent=2) + '\n')
    print(f'V1: {len(files)} files; all AES/MVS payloads round-trip exactly: {output}')


if __name__ == '__main__':
    main()
