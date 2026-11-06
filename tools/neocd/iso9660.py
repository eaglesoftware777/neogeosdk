"""Deterministic, flat ISO9660 Level1 authoring (2048-byte MODE1 payloads).

This writer has no game/firmware loader. audit_disc.py parses the resulting
bytes independently rather than accepting this writer's directory inventory.
"""
import datetime
import re
import struct
from .formats import filename

SECTOR = 2048


def both16(n):
    return struct.pack('<H', n) + struct.pack('>H', n)


def both32(n):
    return struct.pack('<I', n) + struct.pack('>I', n)


def directory_record(name, lba, size, stamp, directory=False):
    length = 33 + len(name) + (len(name) % 2 == 0)
    return (bytes((length, 0)) + both32(lba) + both32(size) + stamp +
            bytes((2 if directory else 0, 0, 0)) + both16(1) + bytes((len(name),)) +
            name + bytes(length - 33 - len(name)))


def pack_directory(records):
    result = bytearray()
    for record in records:
        remain = SECTOR - len(result) % SECTOR
        if len(record) > remain:
            result.extend(bytes(remain))
        result.extend(record)
    result.extend(bytes((-len(result)) % SECTOR))
    return result


def build_iso(files, volume='EAGLECD', epoch=946684800):
    if not re.fullmatch(r'[A-Z0-9_]{1,32}', volume):
        raise ValueError('Volume ID must be1..32 uppercase ISO characters')
    if not 1 <= len(files) <= 512:
        raise ValueError('Conservative CD authoring limit is1..512 root files')
    names = sorted(filename(n) for n in files)
    if len({n.casefold() for n in names}) != len(names):
        raise ValueError('ISO filename collision')
    dt = datetime.datetime.fromtimestamp(epoch, datetime.timezone.utc)
    if not 1900 <= dt.year <= 2155:
        raise ValueError('ISO timestamp year cannot be represented')
    stamp = bytes((dt.year-1900, dt.month, dt.day, dt.hour, dt.minute, dt.second, 0))
    root_lba = 20
    provisional = [directory_record(b'\0', root_lba, SECTOR, stamp, True),
                   directory_record(b'\1', root_lba, SECTOR, stamp, True)]
    # Native CD firmware searches unversioned 8.3 names (mkisofs -N).
    provisional += [directory_record(n.encode(), 0, len(files[n]), stamp) for n in names]
    root_bytes = len(pack_directory(provisional))
    next_lba = root_lba + root_bytes // SECTOR
    extents = {}
    for n in names:
        if not files[n]:
            raise ValueError('Empty CD file: ' + n)
        extents[n] = (next_lba, len(files[n]))
        next_lba += (len(files[n]) + SECTOR - 1) // SECTOR
    sectors = next_lba + 150  # conservative readable trailing pad
    result = bytearray(sectors * SECTOR)
    pvd = bytearray(SECTOR)
    pvd[:7] = b'\x01CD001\x01'
    pvd[8:40] = b'EAGLE SOFTWARE'.ljust(32, b' ')
    pvd[40:72] = volume.encode().ljust(32, b' ')
    pvd[80:88] = both32(sectors)
    pvd[120:124] = both16(1)
    pvd[124:128] = both16(1)
    pvd[128:132] = both16(SECTOR)
    pvd[132:140] = both32(10)
    pvd[140:144] = struct.pack('<I', 18)
    pvd[148:152] = struct.pack('>I', 19)
    root = directory_record(b'\0', root_lba, root_bytes, stamp, True)
    pvd[156:190] = root
    for offset in (190, 318, 446, 574):
        pvd[offset:offset+128] = b'NEOGEOSDK ORIGINAL HOMEBREW'.ljust(128, b' ')
    for offset in (813, 830, 847, 864):
        pvd[offset:offset+17] = dt.strftime('%Y%m%d%H%M%S').encode() + b'00\0'
    pvd[847:864] = b'0'*16 + b'\0'  # no expiration date
    pvd[881] = 1
    result[16*SECTOR:17*SECTOR] = pvd
    result[17*SECTOR:17*SECTOR+7] = b'\xffCD001\x01'
    result[18*SECTOR:18*SECTOR+10] = b'\x01\0' + struct.pack('<IH', root_lba, 1) + b'\0\0'
    result[19*SECTOR:19*SECTOR+10] = b'\x01\0' + struct.pack('>IH', root_lba, 1) + b'\0\0'
    records = [root, directory_record(b'\1', root_lba, root_bytes, stamp, True)]
    records += [directory_record(n.encode(), *extents[n], stamp) for n in names]
    result[root_lba*SECTOR:root_lba*SECTOR+root_bytes] = pack_directory(records)
    for n, (lba, size) in extents.items():
        result[lba*SECTOR:lba*SECTOR+size] = files[n]
    return bytes(result)
