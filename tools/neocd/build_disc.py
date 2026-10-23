"""Author a manifest-driven native CD disc; does not read cartridge P-ROMs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import sys

if __package__ in (None, ''):
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.neocd.formats import Entry, encode_ipl, filename, validate_prg, validate_wav
from tools.neocd.iso9660 import build_iso
from tools.neocd.audit_disc import audit_cue

# Firmware recognition markers, not ownership metadata for the homebrew.
# These exact format strings are required by the stock CD system ROM.
RECOGNITION_FILES = {
    'CPY.TXT': b'Copyright by SNK corporation',
    'BIB.TXT': b'Bibliographiced by SNK corporation',
    'ABS.TXT': b'Abstracted by SNK corporation',
}


def source_file(folder, name):
    if not isinstance(name, str):
        raise ValueError('Source must be a relative package path')
    p = (folder / name).resolve()
    if Path(name).is_absolute() or not p.is_relative_to(folder.resolve()) or not p.is_file():
        raise ValueError('Asset path is missing or escapes its source package: ' + name)
    return p


def validate_manifest(manifest, folder):
    if not isinstance(manifest, dict) or manifest.get('format') != 'neogeosdk-cd-1':
        raise ValueError('Unknown CD manifest version')
    groups = manifest.get('groups', {})
    if not isinstance(groups, dict) or 'boot' not in groups:
        raise ValueError('CD manifest needs an explicit boot group')
    for name, definition in groups.items():
        if not isinstance(name, str) or not isinstance(definition, dict):
            raise ValueError('Invalid asset group definition')
    visiting, closures = set(), {}

    def closure(name):
        if name in closures:
            return closures[name]
        if name in visiting or name not in groups:
            raise ValueError('Missing or cyclic asset group dependency')
        visiting.add(name)
        result = {name}
        dependencies = groups[name].get('depends', [])
        if (not isinstance(dependencies, list) or
                any(not isinstance(d, str) for d in dependencies) or
                len(dependencies) != len(set(dependencies))):
            raise ValueError('Invalid asset dependency list')
        for dependency in dependencies:
            result |= closure(dependency)
        visiting.remove(name)
        closures[name] = result
        return result

    for group in groups:
        if not re.fullmatch(r'[a-z][a-z0-9_]{0,31}', group):
            raise ValueError('Invalid asset group name')
        closure(group)
        if group != 'boot' and 'boot' not in closures[group]:
            raise ValueError('Every scene must depend on pinned boot resources')
    records, names = [], set()
    files = manifest.get('files', [])
    if not isinstance(files, list):
        raise ValueError('Manifest files must be a list')
    for record in files:
        if not isinstance(record, dict) or not {'name', 'source', 'group'} <= record.keys():
            raise ValueError('File record needs name, source and group')
        name = filename(record['name'])
        if name in names or name == 'IPL.TXT' or name in RECOGNITION_FILES:
            raise ValueError('Duplicate/reserved CD filename')
        names.add(name)
        group = record['group']
        if not isinstance(group, str) or group not in groups:
            raise ValueError('Asset references unknown group')
        data = source_file(folder, record['source']).read_bytes()
        entry = Entry(name, record.get('bank', 0), record.get('offset', 0))
        start, end = entry.validate(len(data))
        digest = hashlib.sha256(data).hexdigest()
        if 'size' in record and (type(record['size']) is not int or record['size'] != len(data)):
            raise ValueError('Asset size disagrees with manifest')
        if 'sha256' in record and record['sha256'] != digest:
            raise ValueError('Asset checksum disagrees with manifest')
        records.append(dict(entry=entry, group=group, data=data, start=start, end=end, sha256=digest))
    if not 1 <= len(records) <= 508:
        raise ValueError('Package file count exceeds conservative CD limits')
    for group, active in closures.items():
        occupants = {}
        for record in records:
            if record['group'] not in active:
                continue
            e = record['entry']
            for a, b in occupants.setdefault(e.zone, []):
                if record['start'] < b and a < record['end']:
                    raise ValueError(f'{group}: simultaneously resident {e.zone} assets overlap')
            occupants[e.zone].append((record['start'], record['end']))
    boot = [r for r in records if r['group'] == 'boot']
    if not 1 <= len(boot) <= 32:
        raise ValueError('Initial load exceeds32 file descriptors')
    resident = [r for r in boot if r['entry'].zone == 'PRG' and r['entry'].offset == 0]
    if len(resident) != 1:
        raise ValueError('One resident native PRG at offset0 is required')
    if sum(r['entry'].zone == 'PRG' for r in records) != 1:
        raise ValueError('Executable overlays are not implemented; only the pinned boot PRG is allowed')
    validate_prg(resident[0]['data'])
    if {r['entry'].zone for r in boot} != {'PRG', 'SPR', 'FIX', 'Z80', 'PCM'}:
        raise ValueError('Boot group must contain all five native CD asset classes')
    return records, closures


def build(manifest_path, out, epoch=946684800):
    manifest_path = Path(manifest_path).resolve()
    manifest = json.loads(manifest_path.read_text())
    records, closures = validate_manifest(manifest, manifest_path.parent)
    game = manifest.get('game', '')
    if not isinstance(game, str) or not re.fullmatch(r'[a-z][a-z0-9_]{0,31}', game):
        raise ValueError('Invalid CD game identifier')
    audio = []
    audio_sources = manifest.get('audio', [])
    if not isinstance(audio_sources, list):
        raise ValueError('Manifest audio must be a list')
    for source in audio_sources:
        path = source_file(manifest_path.parent, source)
        validate_wav(path)
        audio.append(path)
    if len(audio) > 98:
        raise ValueError('CDDA track count exceeds99 tracks including data')
    files = {r['entry'].name: r['data'] for r in records}
    files['IPL.TXT'] = encode_ipl([r['entry'] for r in records if r['group'] == 'boot'])
    files.update(RECOGNITION_FILES)
    iso = build_iso(files, manifest.get('volume', 'EAGLECD'), epoch)
    out = Path(out).resolve()
    if out.exists() and any(out.iterdir()):
        raise ValueError('Disc output must be new or empty; previous images are preserved')
    out.mkdir(parents=True, exist_ok=True)
    (out / (game + '.iso')).write_bytes(iso)
    cue = [f'FILE "{game}.iso" BINARY', '  TRACK 01 MODE1/2048', '    INDEX 01 00:00:00']
    if audio:
        (out / 'audio').mkdir()
    for track, source in enumerate(audio, 2):
        name = f'audio/track{track:02}.wav'
        shutil.copyfile(source, out / name)
        cue += [f'FILE "{name}" WAVE', f'  TRACK {track:02} AUDIO',
                '    PREGAP 00:02:00', '    INDEX 01 00:00:00']
    (out / (game + '.cue')).write_text('\n'.join(cue)+'\n', encoding='ascii')
    audit = audit_cue(out / (game + '.cue'))
    report = dict(manifest, files=[dict(name=r['entry'].name, group=r['group'], bank=r['entry'].bank,
                  offset_bytes=r['entry'].offset, bios_offset=r['entry'].bios_offset,
                  size=len(r['data']), sha256=r['sha256']) for r in records],
                  build_epoch=epoch, image_audit=audit, boot_verified=False)
    (out / 'manifest.json').write_text(json.dumps(report, indent=2)+'\n')
    memory = {g: {z: max((r['end'] for r in records if r['group'] in active and r['entry'].zone == z), default=0)
                     for z in ('PRG', 'SPR', 'FIX', 'Z80', 'PCM')} for g, active in closures.items()}
    (out / 'memory-map.json').write_text(json.dumps(memory, indent=2)+'\n')
    (out / 'boot-files.txt').write_bytes(files['IPL.TXT'])
    sums = [hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.relative_to(out).as_posix()
            for p in sorted(out.rglob('*')) if p.is_file()]
    (out / 'checksums.sha256').write_text('\n'.join(sums)+'\n')
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--manifest', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--epoch', type=int, default=int(os.environ.get('SOURCE_DATE_EPOCH', '946684800')))
    a = p.parse_args()
    report = build(a.manifest, a.out, a.epoch)
    print(json.dumps(report['image_audit'], indent=2))


if __name__ == '__main__':
    main()
