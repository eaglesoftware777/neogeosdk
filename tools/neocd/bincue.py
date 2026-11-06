"""A burnable BIN/CUE image of a Neo Geo CD disc.

Optical drive emulators (SD loaders) and disc burning programs take a .cue
with a raw .bin; they do not read a CHD. This writes one .bin holding every
sector at 2352 bytes -- the data track as MODE1/2352 sectors with their
sync, header, EDC and ECC, then each audio track with its two-second pregap
stored as silence -- and a .cue that points into it. Every track keeps the
disc address it has in the source image.

    python3 tools/neocd/bincue.py --cue build/nosignal.cue --out DIR [--name nosignal]
    python3 tools/neocd/bincue.py --chd disc/nosignal.chd --out DIR
        [--zip dist/release/NoSignal-NeoGeoCD-BinCue_v3.zip --title "No Signal Z Alpha v3"]

The source cue may be the disc build's (an ISO and WAVE files) or chdman
extractcd's (one BINARY file, MODE1/2048 then AUDIO); a .chd goes through
chdman extractcd first. --zip packs the pair with a README, the licence and
SHA-256 sums.
"""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import wave
import zipfile

import numpy as np

RAW = 2352
SIZES = {'MODE1/2048': 2048, 'MODE1/2352': 2352, 'AUDIO': 2352}


def msf(frames):
    return f'{frames // 4500:02}:{frames // 75 % 60:02}:{frames % 75:02}'


def frames_of(text):
    m, s, f = (int(v) for v in text.split(':'))
    return (m * 60 + s) * 75 + f


def parse_cue(path):
    """[{file, kind, track, type, index: {0: f, 1: f}, pregap}] in disc order."""
    path = Path(path)
    tracks, current = [], None
    for line in path.read_text(encoding='ascii').splitlines():
        words = line.strip()
        if not words or words.startswith('REM'):
            continue
        m = re.fullmatch(r'FILE\s+"([^"]+)"\s+(\w+)', words)
        if m:
            current = (path.parent / m.group(1), m.group(2).upper())
            continue
        m = re.fullmatch(r'TRACK\s+(\d+)\s+(\S+)', words)
        if m:
            if current is None or m.group(2) not in SIZES:
                raise ValueError(f'{path}: unsupported track: {words}')
            tracks.append(dict(file=current[0], kind=current[1], track=int(m.group(1)),
                               type=m.group(2), index={}, pregap=0))
            continue
        m = re.fullmatch(r'INDEX\s+(\d+)\s+(\d+:\d+:\d+)', words)
        if m and tracks:
            tracks[-1]['index'][int(m.group(1))] = frames_of(m.group(2))
            continue
        m = re.fullmatch(r'PREGAP\s+(\d+:\d+:\d+)', words)
        if m and tracks:
            tracks[-1]['pregap'] = frames_of(m.group(1))
            continue
        if words.split()[0] in ('POSTGAP', 'FLAGS', 'CATALOG', 'TITLE', 'PERFORMER', 'SONGWRITER', 'ISRC'):
            raise ValueError(f'{path}: {words.split()[0]} is not handled here')
    if not tracks or tracks[0]['type'] not in ('MODE1/2048', 'MODE1/2352') or \
            any(t['type'] != 'AUDIO' for t in tracks[1:]):
        raise ValueError(f'{path}: expected one data track followed by audio tracks')
    if [t['track'] for t in tracks] != list(range(1, len(tracks) + 1)):
        raise ValueError(f'{path}: tracks are not numbered 1..{len(tracks)}')
    return tracks


def file_bytes(path, kind):
    if kind == 'WAVE':
        with wave.open(str(path), 'rb') as w:
            if (w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getcomptype()) != (2, 2, 44100, 'NONE'):
                raise ValueError(f'{path}: CD audio must be 44.1 kHz 16-bit stereo PCM')
            return w.readframes(w.getnframes())
    if kind != 'BINARY':
        raise ValueError(f'{path}: unsupported FILE type {kind}')
    return path.read_bytes()


def track_payloads(tracks):
    """Each track's stored sectors (bytes) and its pregap frames, read from the
    source files. Within a file a track runs from its first index to the next
    track's, counted in its own sector size."""
    loaded, out = {}, []
    for k, t in enumerate(tracks):
        data = loaded.setdefault(t['file'], file_bytes(t['file'], t['kind']))
        size = SIZES[t['type']]
        same = [u for u in tracks if u['file'] == t['file']]
        pos = same.index(t)
        start_frame = min(t['index'].values())
        # byte offset of this track's first index within the file
        offset, cursor = 0, 0
        for u in same[:pos]:
            nxt = same[same.index(u) + 1]
            gap = min(nxt['index'].values()) - min(u['index'].values())
            offset += gap * SIZES[u['type']]
        if pos == 0 and start_frame:
            raise ValueError(f"{t['file']}: track {t['track']} does not start the file")
        end = len(data)
        if pos + 1 < len(same):
            nxt = same[pos + 1]
            end = offset + (min(nxt['index'].values()) - start_frame) * size
        body = data[offset:end]
        if t['type'] != 'AUDIO' and len(body) % size:
            raise ValueError(f"track {t['track']}: data is not whole {size}-byte sectors")
        if len(body) % size:
            body += bytes(size - len(body) % size)      # audio: pad to the last sector
        stored_pregap = t['index'][1] - t['index'].get(0, t['index'][1])
        pregap = t['pregap'] + stored_pregap
        if t['type'] == 'AUDIO':
            body = body[stored_pregap * size:]          # silence rewritten below
        out.append(dict(track=t['track'], type=t['type'], size=size, body=body, pregap=pregap))
    return out


# ------------------------------------------------------------ EDC and ECC
# The CD-ROM Mode 1 checks (ECMA-130 annex A and B): a 32-bit EDC over the
# sync, header and data, then the P and Q Reed-Solomon parity over the header,
# data and EDC. Worked for every sector of the track at once.
def _tables():
    f_lut = np.zeros(256, np.uint8)
    b_lut = np.zeros(256, np.uint8)
    edc = np.zeros(256, np.uint32)
    for i in range(256):
        j = ((i << 1) ^ (0x11D if i & 0x80 else 0)) & 0xFF
        f_lut[i] = j
        b_lut[i ^ j] = i
        e = i
        for _ in range(8):
            e = (e >> 1) ^ (0xD8018001 if e & 1 else 0)
        edc[i] = e
    return f_lut, b_lut, edc


F_LUT, B_LUT, EDC_LUT = _tables()


def _parity(src, major_count, minor_count, major_mult, minor_inc):
    """src: (n, bytes) from sector offset 12; returns (n, 2 * major_count)."""
    n = src.shape[0]
    size = major_count * minor_count
    out = np.zeros((n, 2 * major_count), np.uint8)
    for major in range(major_count):
        index = (major >> 1) * major_mult + (major & 1)
        a = np.zeros(n, np.uint8)
        b = np.zeros(n, np.uint8)
        for _ in range(minor_count):
            v = src[:, index]
            index += minor_inc
            if index >= size:
                index -= size
            a ^= v
            b ^= v
            a = F_LUT[a]
        a = B_LUT[F_LUT[a] ^ b]
        out[:, major] = a
        out[:, major + major_count] = a ^ b
    return out


def mode1_sectors(user, first_lba=0):
    """Raw 2352-byte Mode 1 sectors for 2048-byte user blocks starting at first_lba."""
    if len(user) % 2048:
        raise ValueError('Mode 1 user data must be whole 2048-byte blocks')
    n = len(user) // 2048
    s = np.zeros((n, RAW), np.uint8)
    s[:, 1:11] = 0xFF
    lba = np.arange(n) + first_lba + 150
    for k, v in enumerate((lba // 4500, lba // 75 % 60, lba % 75)):
        s[:, 12 + k] = (v // 10 << 4) | (v % 10)
    s[:, 15] = 1
    s[:, 16:2064] = np.frombuffer(user, np.uint8).reshape(n, 2048)
    edc = np.zeros(n, np.uint32)
    for k in range(2064):
        edc = (edc >> 8) ^ EDC_LUT[(edc ^ s[:, k]) & 0xFF]
    s[:, 0x810:0x814] = edc.astype('<u4').view(np.uint8).reshape(n, 4)
    s[:, 0x81C:0x8C8] = _parity(s[:, 12:12 + 2064], 86, 24, 2, 86)
    s[:, 0x8C8:0x930] = _parity(s[:, 12:12 + 2236], 52, 43, 86, 88)
    return s.tobytes()


# ------------------------------------------------------------ writer
def write(tracks, out, name):
    """Write <name>.bin and <name>.cue; returns (bin path, cue path, layout)."""
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    payloads = track_payloads(tracks)
    data = payloads[0]
    user = data['body'] if data['size'] == 2048 else None
    raw = mode1_sectors(user) if user is not None else data['body']
    cue = [f'FILE "{name}.bin" BINARY', '  TRACK 01 MODE1/2352', '    INDEX 01 00:00:00']
    layout = [dict(track=1, type='MODE1/2352', lba=0, frames=len(raw) // RAW)]
    with open(out / f'{name}.bin', 'wb') as f:
        f.write(raw)
        at = len(raw) // RAW
        for p in payloads[1:]:
            cue.append(f"  TRACK {p['track']:02} AUDIO")
            if p['pregap']:
                cue.append(f'    INDEX 00 {msf(at)}')
                f.write(bytes(p['pregap'] * RAW))
                at += p['pregap']
            cue.append(f'    INDEX 01 {msf(at)}')
            layout.append(dict(track=p['track'], type='AUDIO', lba=at, frames=len(p['body']) // RAW,
                               pregap=p['pregap']))
            f.write(p['body'])
            at += len(p['body']) // RAW
    (out / f'{name}.cue').write_text('\r\n'.join(cue) + '\r\n', encoding='ascii')
    return out / f'{name}.bin', out / f'{name}.cue', layout


def disc_layout(tracks):
    """Each track's disc address (index 1) and length, as the source image lays them out."""
    out, at = [], 0
    for p in track_payloads(tracks):
        at += p['pregap'] if p['type'] == 'AUDIO' else 0
        out.append((p['track'], at, len(p['body']) // p['size']))
        at += len(p['body']) // p['size']
    return out


README = """{title} - Neo Geo CD disc image (BIN/CUE)

{cue} and {bin} are the whole disc: track 1 is the program data
(MODE1/2352, full raw sectors with EDC/ECC), tracks 2-{last} are CD audio.
Keep both files in the same folder; the .cue names the .bin.

- Optical drive emulators / SD loaders: copy both files to the card.
- Burning a CD-R: open the .cue (not the .bin) in a burner that writes
  mixed-mode discs in disc-at-once mode (ImgBurn, CDRWIN, cdrdao) and
  burn at a low speed.
- Emulators: open the .cue.

Same disc as {source}:
every data sector and audio sample matches it, and every track starts at
the same disc address. No system ROM is on the disc or in this archive.

Checked in MAME: the .cue boots and shows the same screens as the original
image. Burnt discs, drive emulators and real Neo Geo CD hardware have not
been tested with this image.

Tracks:
{tracks}
"""


def package(zip_path, binary, sheet, layout, title, source):
    """Pack the .bin/.cue with a README, the licence and SHA-256 sums."""
    licence = Path(__file__).resolve().parents[2] / 'LICENSE'
    rows = []
    for t in layout:
        kind = 'data, MODE1/2352' if t['track'] == 1 else 'audio'
        rows.append(f"  {t['track']:02}  {kind:17s} starts {msf(t['lba'] + 150)}  {t['frames']} sectors")
    readme = README.format(title=title, cue=sheet.name, bin=binary.name, last=len(layout),
                           source=source, tracks='\n'.join(rows)).replace('\n', '\r\n')
    files = {sheet.name: sheet.read_bytes(), binary.name: None, 'README.txt': readme.encode('ascii'),
             'LICENSE.txt': licence.read_bytes()}
    sums = []
    for name, data in files.items():
        h = hashlib.sha256(data if data is not None else binary.read_bytes()).hexdigest()
        sums.append(f'{h}  {name}')
    files['SHA256SUMS.txt'] = ('\r\n'.join(sums) + '\r\n').encode('ascii')
    zip_path = Path(zip_path)
    zip_path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name, data in files.items():
            info = zipfile.ZipInfo(f'{zip_path.stem}/{name}', date_time=(2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            if data is None:
                with open(binary, 'rb') as src, z.open(info, 'w') as dst:
                    while chunk := src.read(1 << 22):
                        dst.write(chunk)
            else:
                z.writestr(info, data)
    return zip_path


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    src = ap.add_mutually_exclusive_group(required=True)
    src.add_argument('--cue', type=Path)
    src.add_argument('--chd', type=Path)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--name', help='base name of the .bin and .cue (default: the source name)')
    ap.add_argument('--chdman', default='chdman')
    ap.add_argument('--zip', type=Path, help='also pack the pair into this archive')
    ap.add_argument('--title', help="the README's first line, e.g. the game and version")
    ap.add_argument('--source-label', help='the README\'s name for the source image, e.g. its package')
    a = ap.parse_args()
    name = a.name or (a.cue or a.chd).stem
    with tempfile.TemporaryDirectory() as tmp:
        cue = a.cue
        if a.chd:
            cue = Path(tmp) / f'{name}.cue'
            subprocess.run([a.chdman, 'extractcd', '-i', str(a.chd), '-o', str(cue)],
                           check=True, stdout=subprocess.DEVNULL)
        tracks = parse_cue(cue)
        before = disc_layout(tracks)
        binary, sheet, layout = write(tracks, a.out, name)
    after = [(t['track'], t['lba'], t['frames']) for t in layout]
    if before != after:
        raise SystemExit(f'track layout changed: {before} -> {after}')
    digest = hashlib.sha256(binary.read_bytes()).hexdigest()
    print(f'{sheet.name} + {binary.name}: {len(layout)} tracks, {binary.stat().st_size} bytes, sha256 {digest}')
    if a.zip:
        if not a.title or not a.source_label:
            raise SystemExit('--zip needs --title and --source-label')
        print('packed', package(a.zip, binary, sheet, layout, a.title, a.source_label))


if __name__ == '__main__':
    sys.exit(main())
