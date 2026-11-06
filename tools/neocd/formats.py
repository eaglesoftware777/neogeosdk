"""Explicit CD file/address contracts; cartridge filenames are not conversions."""
from dataclasses import dataclass
import hashlib
import re
import wave

NAME = re.compile(r'[A-Z0-9_]{1,8}\.[A-Z0-9_]{1,3}\Z')
ZONES = {'PRG': (1, 0x100000, 2), 'SPR': (4, 0x100000, 128),
         'FIX': (1, 0x20000, 32), 'Z80': (1, 0x10000, 2),
         'PCM': (2, 0x80000, 256)}
BYTE_ZONES = ('FIX', 'Z80', 'PCM')


def filename(name):
    if not isinstance(name, str) or not NAME.fullmatch(name):
        raise ValueError('CD filename must be uppercase 8.3: ' + repr(name))
    return name


def integer(value, label, maximum=0xFFFFFFFF):
    if type(value) is not int or not 0 <= value <= maximum:
        raise ValueError(label + ' is not an unsigned integer in range')
    return value


@dataclass(frozen=True)
class Entry:
    name: str
    bank: int = 0
    offset: int = 0  # physical data bytes, NOT the BIOS byte-lane address

    @property
    def zone(self):
        return filename(self.name).rsplit('.', 1)[1]

    def validate(self, size):
        zone = self.zone
        if zone not in ZONES:
            raise ValueError('Unsupported CD asset class: ' + zone)
        banks, capacity, alignment = ZONES[zone]
        integer(self.bank, 'bank', banks - 1)
        integer(self.offset, 'offset', capacity)
        integer(size, 'asset size', capacity)
        if not size or self.offset % alignment or size % alignment or size > capacity - self.offset:
            raise ValueError(f'{self.name}: empty, misaligned, or crosses its upload bank')
        return self.bank * capacity + self.offset, self.bank * capacity + self.offset + size

    @property
    def bios_offset(self):
        return self.offset * (2 if self.zone in BYTE_ZONES else 1)


def encode_ipl(entries):
    if not 1 <= len(entries) <= 32 or len({e.name for e in entries}) != len(entries):
        raise ValueError('IPL requires1..32 uniquely named files')
    lines = []
    for e in entries:
        filename(e.name)
        if e.zone not in ZONES:
            raise ValueError('Unsupported IPL extension')
        integer(e.bank, 'bank', ZONES[e.zone][0] - 1)
        e.validate(ZONES[e.zone][2])
        lines.append(f'{e.name},{e.bank},{e.bios_offset:X}\r\n')
    return ''.join(lines).encode('ascii') + b'\x1a'


def parse_ipl(data):
    if not data.endswith(b'\x1a') or b'\x1a' in data[:-1]:
        raise ValueError('IPL must end with one DOS EOF byte')
    content = data[:-1]
    if not content.endswith(b'\r\n'):
        raise ValueError('IPL records need CRLF')
    result = []
    for raw in content[:-2].split(b'\r\n'):
        try:
            text = raw.decode('ascii')
        except UnicodeDecodeError as e:
            raise ValueError('IPL is not ASCII') from e
        match = re.fullmatch(r'([A-Z0-9_]{1,8}\.[A-Z0-9_]{1,3}),([0-9]),([0-9A-F]{1,8})', text)
        if not match:
            raise ValueError('Malformed IPL record')
        name, bank, address = match.groups()
        zone = name.rsplit('.', 1)[1]
        if zone not in ZONES:
            raise ValueError('Unsupported IPL extension')
        scale = 2 if zone in BYTE_ZONES else 1
        address = int(address, 16)
        if address % scale:
            raise ValueError('Unaligned byte-lane BIOS address')
        entry = Entry(name, int(bank), address // scale)
        entry.validate(ZONES[zone][2])
        result.append(entry)
    if not 1 <= len(result) <= 32 or len({e.name for e in result}) != len(result):
        raise ValueError('Invalid IPL list length or duplicate entry')
    return result


def validate_prg(data):
    if len(data) < 0x140 or len(data) > 0x100000 or len(data) % 2:
        raise ValueError('Invalid resident CD PRG size')
    if data[0x100:0x107] != b'NEO-GEO' or data[0x107] != 2:
        raise ValueError('Not a native CD PRG header (do not byte-swap cartridge P-ROM)')
    if data[0x122:0x124] != b'\x4e\xf9':
        raise ValueError('CD USER entry must be an absolute jump')
    entry = int.from_bytes(data[0x124:0x128], 'big')
    if entry & 1 or not 0x140 <= entry < len(data):
        raise ValueError('CD USER entry is outside resident PRG')
    if data[0x13A:0x13C] != b'\0\0':
        raise ValueError('This backend requires explicit CDDA calls, not Z80 polling')
    return entry


def cartridge_sprites_to_spr(c1, c2):
    """Final cartridge lanes (bp0/1 and bp2/3) -> native CD bp1/0/3/2."""
    if not c1 or len(c1) != len(c2) or len(c1) % 64:
        raise ValueError('C-ROM lanes need equal, complete64-byte tile records')
    result = bytearray(len(c1) * 2)
    for i in range(0, len(c1), 2):
        result[i*2:i*2+4] = bytes((c1[i+1], c1[i], c2[i+1], c2[i]))
    return bytes(result)


def decode_spr(tile):
    if len(tile) != 128:
        raise ValueError('SPR tile must be128 bytes')
    pixels = [[0] * 16 for _ in range(16)]
    for half, left in enumerate((8, 0)):
        for y in range(16):
            at = half * 64 + y * 4
            planes = (tile[at+1], tile[at], tile[at+3], tile[at+2])
            for x in range(8):
                pixels[y][left+x] = sum(((plane >> x) & 1) << bit for bit, plane in enumerate(planes))
    return pixels


def encode_spr(pixels):
    if len(pixels) != 16 or any(len(row) != 16 for row in pixels):
        raise ValueError('Expected16x16 native pixels')
    if any(type(p) is not int or not 0 <= p <= 15 for row in pixels for p in row):
        raise ValueError('Sprite indices must be4bpp')
    result = bytearray()
    for left in (8, 0):
        for row in pixels:
            for bit in (1, 0, 3, 2):
                result.append(sum(((row[left+x] >> bit) & 1) << x for x in range(8)))
    return bytes(result)


def decode_fix(tile):
    if len(tile) != 32:
        raise ValueError('FIX tile must be32 bytes')
    result = [[0]*8 for _ in range(8)]
    for pair, offset in enumerate((16, 24, 0, 8)):
        for y in range(8):
            value = tile[offset+y]
            result[y][pair*2:pair*2+2] = [value & 15, value >> 4]
    return result


def validate_wav(path):
    with wave.open(str(path), 'rb') as w:
        if (w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getcomptype()) != (2, 2, 44100, 'NONE'):
            raise ValueError('CDDA requires44.1kHz16-bit stereo PCM WAV')
        count = w.getnframes()
        data = w.readframes(count)
    if not count or count % 588 or len(data) != count * 4:
        raise ValueError('CDDA audio must contain complete1/75-second sectors')
    return dict(frames=count, sectors=count//588, sha256=hashlib.sha256(data).hexdigest())


def bcd_track(track):
    integer(track, 'audio track', 99)
    if track < 2:
        raise ValueError('Track1 is data, not CDDA')
    return (track // 10 << 4) | track % 10
