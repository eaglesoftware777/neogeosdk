"""Independent, bounds-checked ISO/IPL/package reader. Never invokes the writer."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

if __package__ in (None, ''):
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.neocd.formats import filename, parse_ipl, validate_prg, validate_wav


def dual(data, at, width):
    if at + 2*width > len(data):
        raise ValueError('Truncated ISO integer')
    little = int.from_bytes(data[at:at+width], 'little')
    big = int.from_bytes(data[at+width:at+2*width], 'big')
    if little != big:
        raise ValueError('ISO endian copies disagree')
    return little


def read_iso(data):
    data = bytes(data)
    if len(data) % 2048 or len(data) < 21*2048:
        raise ValueError('Not a complete MODE1/2048 ISO')
    pvd = data[16*2048:17*2048]
    if pvd[:7] != b'\x01CD001\x01' or pvd[881] != 1:
        raise ValueError('Invalid primary volume descriptor')
    if data[17*2048:17*2048+7] != b'\xffCD001\x01':
        raise ValueError('Unsupported/missing volume descriptor terminator')
    if dual(pvd, 80, 4)*2048 != len(data) or dual(pvd, 128, 2) != 2048:
        raise ValueError('ISO length or logical sector size is inconsistent')
    root, size = dual(pvd, 158, 4), dual(pvd, 166, 4)
    if root < 20 or not size or size % 2048 or root*2048+size > len(data):
        raise ValueError('Root directory extent is invalid')
    if dual(pvd, 132, 4) != 10:
        raise ValueError('This disc profile requires a flat root path table')
    for at, byteorder in ((140, 'little'), (148, 'big')):
        lba = int.from_bytes(pvd[at:at+4], byteorder)
        if not 18 <= lba < root:
            raise ValueError('Path table lies outside the metadata area')
        table = data[lba*2048:lba*2048+10]
        parent = (1).to_bytes(2, byteorder) + b'\0\0'
        if table[:2] != b'\x01\0' or int.from_bytes(table[2:6], byteorder) != root or table[6:] != parent:
            raise ValueError('Invalid root path table')
    directory = data[root*2048:root*2048+size]
    result, intervals, cursor, links = {}, [], 0, set()
    while cursor < size:
        length = directory[cursor]
        if not length:
            end = min(size, ((cursor // 2048)+1)*2048)
            if any(directory[cursor:end]):
                raise ValueError('Nonzero bytes after directory sector padding')
            cursor = end
            continue
        if length < 34 or length > 2048-cursor%2048 or cursor+length > size:
            raise ValueError('Directory record crosses a sector or is truncated')
        record = directory[cursor:cursor+length]
        count = record[32]
        if 33+count > length or record[1] or record[26] or record[27] or dual(record, 28, 2) != 1:
            raise ValueError('Unsupported directory record encoding')
        extent, n = dual(record, 2, 4), dual(record, 10, 4)
        name = record[33:33+count]
        if name in (b'\0', b'\1'):
            if record[25] != 2 or extent != root or n != size or name in links:
                raise ValueError('Invalid root self/parent directory link')
            links.add(name)
        else:
            if record[25] != 0:
                raise ValueError('Subdirectories/multiextent files are outside the CD profile')
            try:
                text = name.decode('ascii')
            except UnicodeDecodeError as e:
                raise ValueError('Non-ASCII ISO filename') from e
            if not text.endswith(';1'):
                raise ValueError('Missing ISO9660 version1 suffix')
            text = filename(text[:-2])
            end = extent*2048+n
            padded_end = ((end+2047)//2048)*2048
            if text in result or n == 0 or extent < root+size//2048 or end > len(data):
                raise ValueError('Duplicate, empty, overlapping metadata or out-of-range file')
            if any(extent*2048 < b and a < padded_end for a, b in intervals):
                raise ValueError('ISO file extents overlap')
            intervals.append((extent*2048, padded_end))
            result[text] = data[extent*2048:end]
        cursor += length
    if links != {b'\0', b'\1'} or not 1 <= len(result) <= 512:
        raise ValueError('Invalid root directory links/file count')
    return result


def audit_image(path):
    data = Path(path).read_bytes()
    files = read_iso(data)
    if 'IPL.TXT' not in files:
        raise ValueError('Missing IPL.TXT')
    entries = parse_ipl(files['IPL.TXT'])
    seen = {}
    for entry in entries:
        if entry.name not in files:
            raise ValueError('IPL references missing file: ' + entry.name)
        interval = entry.validate(len(files[entry.name]))
        for a, b in seen.setdefault(entry.zone, []):
            if interval[0] < b and a < interval[1]:
                raise ValueError('Initial CD memory destinations overlap')
        seen[entry.zone].append(interval)
    if set(seen) != {'PRG', 'SPR', 'FIX', 'Z80', 'PCM'}:
        raise ValueError('Initial native asset class is missing')
    boot = [e for e in entries if e.zone == 'PRG' and e.offset == 0]
    if len(boot) != 1:
        raise ValueError('Exactly one resident native PRG is required')
    entry = validate_prg(files[boot[0].name])
    return dict(passed=True, format='ISO9660 Level1 MODE1/2048', size=len(data),
                sha256=hashlib.sha256(data).hexdigest(), entry=entry,
                files={n: dict(size=len(v), sha256=hashlib.sha256(v).hexdigest()) for n, v in files.items()},
                memory_high_water={z: max(b for _, b in intervals) for z, intervals in seen.items()},
                boot_verified=False)


def audit_cue(path):
    path = Path(path)
    lines = path.read_text(encoding='ascii').splitlines()
    path = path.resolve()
    current_file, current_track, tracks, files, indexed = None, None, [], [], False
    pending_file, pregap = False, False
    for raw in lines:
        line = raw.strip()
        match = re.fullmatch(r'FILE "([^"\\]+)" (BINARY|WAVE)', line)
        if match:
            if pending_file or (current_track is not None and not indexed):
                raise ValueError('CUE FILE interrupts an unfinished track')
            relative = Path(match[1])
            resolved = (path.parent / relative).resolve()
            if (relative.is_absolute() or '..' in relative.parts or ':' in match[1] or
                    not resolved.is_relative_to(path.parent)):
                raise ValueError('CUE path escapes package')
            current_file = resolved
            if not current_file.is_file():
                raise ValueError('CUE references missing file')
            if any(current_file == previous for previous, _ in files):
                raise ValueError('Each CUE track needs its own file')
            files.append((current_file, match[2]))
            pending_file = True
            continue
        match = re.fullmatch(r'TRACK (\d\d) (MODE1/2048|AUDIO)', line)
        if match:
            if current_track is not None and not indexed:
                raise ValueError('CUE track lacks INDEX01')
            number = int(match[1]); kind = match[2]
            if not pending_file or number != len(tracks)+1 or number > 99:
                raise ValueError('CUE tracks must be consecutively numbered from1')
            if number == 1 and (kind != 'MODE1/2048' or files[-1][1] != 'BINARY'):
                raise ValueError('CUE track1 must be the data ISO')
            if number > 1 and (kind != 'AUDIO' or files[-1][1] != 'WAVE'):
                raise ValueError('CUE audio track must reference a WAV')
            tracks.append((number, kind, current_file));current_track=number;indexed=False
            pending_file = False
            pregap = False
            continue
        if line == 'PREGAP 00:02:00' and current_track and current_track > 1 and not indexed and not pregap:
            pregap = True
            continue
        if line == 'INDEX 01 00:00:00' and current_track and not indexed:
            indexed=True
            continue
        raise ValueError('Unsupported or malformed CUE statement: ' + line)
    if not tracks or not indexed or pending_file or len(files) != len(tracks):
        raise ValueError('CUE requires one file and INDEX01 per track')
    report = audit_image(tracks[0][2])
    report['audio'] = [validate_wav(file) for _, _, file in tracks[1:]]
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('image', type=Path)
    a = p.parse_args()
    print(json.dumps(audit_cue(a.image) if a.image.suffix.lower() == '.cue' else audit_image(a.image), indent=2))


if __name__ == '__main__':
    main()
