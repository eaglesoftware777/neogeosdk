#!/usr/bin/env python3
"""
Check a game's release files in dist/release/ for the hardware fixes every
set must carry, before they are published:

  ROMs        P 512 KiB (or whole 1 MiB banks), S and M 128 KiB, C1/C2 equal lanes, hardware blank
              tile $FFFF empty
  boot        cartridge id at $108, eye-catcher flag MVS 1 / AES 2, the
              Universe BIOS logo bank $49 with its tiles drawn in C, the
              game's own FIX font in its private bank ($D00 on)
  MVS sound   the sound driver's NGP2 protocol in M1 (slot-switch wait,
              BIOS commands apart from the game's)
  NeoSD       the hardware alignment: P/S/M on 64 KiB, V padded with $FF to
              a power of two, C interleaved and on 256 KiB; every region the
              MAME set's bytes
  Darksoft    unpacks to the MAME set byte for byte
  BackBit     the MAME set's six files byte for byte
  MAME zips   EagleBIOS only (as in bios/), software list CRC/SHA-1 matching
  packages    no system BIOS in the NeoSD, Darksoft or BackBit files
  manifest    every file's size, CRC32 and SHA-256

    python3 tools/audit_release.py --prefix Maiya --game maiya --game-id 780 --version v4
"""

import argparse
import hashlib
import struct
import sys
import xml.etree.ElementTree as ET
import zipfile
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from pack_darksoft import read_folder, unpack  # noqa: E402

PARTS = ("p1", "s1", "m1", "v1", "c1", "c2")
EAGLE = {"neogeo": ("sp-s2.sp1", "sm1.sm1", "sfix.sfix", "000-lo.lo"), "aes": ("neo-epo.bin", "000-lo.lo")}
BIOS_NAMES = ("sp-s2", "sp1", "sm1", "sfix", "000-lo", "neo-epo", "neo-po", "uni-bios", "asia-s3", "vs-bios")
BOOT_C_BANK, GAME_FIX_BASE = 0x49, 0xD00


class Audit:
    def __init__(self):
        self.failures = []

    def check(self, ok, what):
        print(("  ok    " if ok else "  FAIL  ") + what)
        if not ok:
            self.failures.append(what)


def rom_checks(a, roms, platform, game_id):
    p = roms["p1"]
    # P is 512 KiB, or a banked image of whole 1 MiB banks (the fixed first
    # MiB, then the windows switched at $2FFFF0), up to the NeoSD's 8 MiB.
    p_ok = len(p) == 0x80000 or (len(p) % 0x100000 == 0 and 0x100000 <= len(p) <= 0x800000)
    a.check(p_ok and len(roms["s1"]) == 0x20000 and len(roms["m1"]) == 0x20000,
            f"{platform}: P 512 KiB or whole 1 MiB banks, S and M 128 KiB")
    a.check(len(roms["c1"]) == len(roms["c2"]) and len(roms["c1"]) % 0x20000 == 0,
            f"{platform}: C1/C2 equal lanes ({len(roms['c1']) // 1024} KiB each)")
    a.check(all(roms[c][-64:] == bytes(64) for c in ("c1", "c2")) if len(roms["c1"]) == 0x400000 else True,
            f"{platform}: hardware blank tile $FFFF empty")
    program = bytearray(len(p))
    program[0::2], program[1::2] = p[1::2], p[0::2]
    a.check(program[0x100:0x107] == b"NEO-GEO", f"{platform}: NEO-GEO header")
    ngh = int(game_id, 16)
    a.check(program[0x108:0x10A] == ngh.to_bytes(2, "big"), f"{platform}: cartridge id {game_id} at $108")
    flag = 2 if platform == "aes" else 1
    a.check(program[0x114] == flag, f"{platform}: eye-catcher flag {program[0x114]} (expected {flag})")
    a.check(program[0x115] == BOOT_C_BANK, f"{platform}: Universe BIOS logo bank ${program[0x115]:02X} (expected $49)")
    first = BOOT_C_BANK * 256 * 64
    drawn = sum(1 for t in range(256) if any(roms[c][first + t * 64:first + t * 64 + 64] != bytes(64)
                                               for c in ("c1", "c2")))
    a.check(drawn > 0, f"{platform}: boot logo tiles drawn in C bank $49 ({drawn})")
    fix = roms["s1"]
    font = sum(1 for t in range(GAME_FIX_BASE, 0x1000) if fix[t * 32:t * 32 + 32] != bytes(32))
    a.check(font > 0, f"{platform}: game FIX font in its own bank $D00-$FFF ({font} tiles)")
    a.check(b"NGP2" in roms["m1"], f"{platform}: MVS sound protocol (NGP2) in M1")


def neosd_checks(a, image, roms, platform, game_id):
    a.check(image[:4] == b"NEO\x01", f"{platform} NeoSD: header")
    sizes = struct.unpack_from("<6I", image, 4)
    p, s, m, v, v2, c = sizes
    a.check(len(image) == 4096 + sum(sizes), f"{platform} NeoSD: size matches header")
    a.check(all(x % 0x10000 == 0 for x in (p, s, m)), f"{platform} NeoSD: P/S/M on 64 KiB")
    a.check(v >= 0x10000 and v & (v - 1) == 0, f"{platform} NeoSD: V padded to a power of two ({v // 1024} KiB)")
    a.check(c % 0x40000 == 0 and v2 == 0, f"{platform} NeoSD: C on 256 KiB")
    a.check(struct.unpack_from("<I", image, 40)[0] == int(game_id, 16), f"{platform} NeoSD: NGH {game_id}")
    offset = 4096
    for part, size in zip(("p1", "s1", "m1", "v1"), (p, s, m, v)):
        region = image[offset:offset + size]
        a.check(region[:len(roms[part])] == roms[part] and set(region[len(roms[part]):]) <= {0xFF},
                f"{platform} NeoSD: {part.upper()} is the MAME set's, padded with $FF")
        offset += size
    sprites = image[offset:offset + c]
    a.check(sprites[0::2][:len(roms["c1"])] == roms["c1"] and sprites[1::2][:len(roms["c2"])] == roms["c2"],
            f"{platform} NeoSD: C interleaved from C1/C2 byte for byte")


def no_bios(a, names, what):
    found = [n for n in names if any(b in Path(n).name.lower() for b in BIOS_NAMES)]
    a.check(not found, f"{what}: no system BIOS inside" + (f" ({found})" if found else ""))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--prefix", required=True)
    ap.add_argument("--game", required=True)
    ap.add_argument("--game-id", required=True)
    ap.add_argument("--version", required=True)
    ap.add_argument("--dir", type=Path, default=ROOT / "dist/release")
    args = ap.parse_args()
    d, P, v, gid = args.dir, args.prefix, args.version, args.game_id
    a = Audit()
    sets = {}
    for platform, mame in (("mvs", f"{P}-WIP-EagleBIOS_{v}.zip"), ("aes", f"{P}-WIP-AES-EagleBIOS_{v}.zip")):
        print(f"{mame}")
        with zipfile.ZipFile(d / mame) as z:
            names = z.namelist()
            roms = {part: z.read(f"roms/{args.game}/{gid}-{part}.{part}") for part in PARTS}
            # the MVS zip carries both boards' EagleBIOS (tools/make_release.py)
            boards = ("aes",) if platform == "aes" else ("neogeo", "aes")
            expected = [f"roms/{b}/{f}" for b in boards for f in EAGLE[b]]
            bios = [n for n in names if n.startswith("roms/") and not n.startswith(f"roms/{args.game}/")]
            a.check(sorted(bios) == sorted(expected) and
                    all(z.read(n) == (ROOT / "bios" / Path(n).name).read_bytes() for n in expected),
                    f"{platform}: EagleBIOS only, as built in bios/")
            soft = ET.fromstring(z.read("hash/neogeo.xml"))
            listed = {r.get("name"): r for r in soft.iter("rom")}
            ok = True
            for part in PARTS:
                r = listed.get(f"{gid}-{part}.{part}")
                data = roms[part]
                ok &= r is not None and int(r.get("size"), 0) == len(data) \
                    and r.get("crc") == f"{zlib.crc32(data) & 0xffffffff:08x}" \
                    and r.get("sha1") == hashlib.sha1(data).hexdigest()
            a.check(ok, f"{platform}: software list size/CRC/SHA-1 match the ROMs")
        rom_checks(a, roms, platform, gid)
        sets[platform] = roms
        upper = platform.upper()
        print(f"{P}-WIP-NeoSD_{upper}_{v}.neo")
        neosd_checks(a, (d / f"{P}-WIP-NeoSD_{upper}_{v}.neo").read_bytes(), roms, platform, gid)
        dark = d / f"{P}-Darksoft-{upper}_{v}.zip"
        print(dark.name)
        files = read_folder(dark)
        back = unpack(files, gid)
        a.check(all(back[f"{gid}-{part}.{part}"] == roms[part] for part in PARTS),
                f"{platform} Darksoft: unpacks to the MAME set byte for byte")
        no_bios(a, files, f"{platform} Darksoft")
        bb = d / f"{P}-BackBit-{upper}_{v}.zip"
        print(bb.name)
        with zipfile.ZipFile(bb) as z:
            a.check(all(z.read(f"{args.game}/{args.game}.{part}") == roms[part] for part in PARTS),
                    f"{platform} BackBit: the MAME set's six files byte for byte")
            no_bios(a, z.namelist(), f"{platform} BackBit")
    a.check(sets["mvs"]["p1"] != sets["aes"]["p1"], "MVS and AES are separate builds (P1 differs)")
    manifest = d / f"{P}-MANIFEST_{v}.txt"
    print(manifest.name)
    rows = [line.split() for line in manifest.read_text().splitlines() if line and not line.startswith("#")]
    for size, crc, sha, name in rows:
        data = (d / name).read_bytes()
        a.check(len(data) == int(size) and f"{zlib.crc32(data) & 0xffffffff:08x}" == crc
                and hashlib.sha256(data).hexdigest() == sha, f"manifest: {name}")
    print(f"\n{P} {v}: {'PASS' if not a.failures else 'FAIL'} ({len(a.failures)} failures)")
    return 1 if a.failures else 0


if __name__ == "__main__":
    sys.exit(main())
