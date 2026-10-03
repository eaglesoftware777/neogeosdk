#!/usr/bin/env python3
"""
A cartridge's ROMs as a Darksoft Neo Geo Multi game folder:

    prom    the P ROM as built
    srom    the S (FIX) ROM
    m1rom   the Z80 M ROM
    vroma0  the V ROM, padded with $FF to a power of two
    crom0   C1 and C2 interleaved two bytes at a time (C1[0..1], C2[0..1], ...)
    fpga    ASCII "10"

plus MANIFEST.json (sizes, CRC32, SHA-256) and README.txt. The layout is the
one of the v1.7.1 release's Maiya-Darksoft zips, whose crom0 this
reproduces byte for byte from the same C ROMs. No system BIOS goes in: the
Multi uses the board's.

    python3 tools/pack_darksoft.py --rom-dir roms/maiya --game-id 780 \\
        --name "Maiya: Super Nature Girl" --platform MVS --out dist/Maiya_MVS [--zip X.zip]

--unpack turns a Darksoft folder (or a zip holding one) back into a MAME
set, to run the very files the Multi gets in an emulator: prom, srom, m1rom
as P1, S1, M1; crom0 split into C1 and C2; vroma0 cut back to the V ROM's
size when MANIFEST.json gives it (only $FF padding may go).

    python3 tools/pack_darksoft.py --unpack Maiya-Darksoft-MVS_v2.zip --game-id 780 --out roms/maiya
"""

import argparse
import hashlib
import json
import sys
import zipfile
import zlib
from pathlib import Path


def pad_pow2(data, fill=b"\xff"):
    size = 1 << max(16, (len(data) - 1).bit_length())
    return data + fill * (size - len(data))


def interleave_pairs(c1, c2):
    if len(c1) != len(c2) or len(c1) % 2:
        raise ValueError("C1 and C2 must be the same even length")
    out = bytearray(len(c1) * 2)
    out[0::4], out[1::4] = c1[0::2], c1[1::2]
    out[2::4], out[3::4] = c2[0::2], c2[1::2]
    return bytes(out)


def build(rom_dir, game_id, platform, name):
    read = lambda part: (rom_dir / f"{game_id}-{part}.{part}").read_bytes()
    files = {
        "prom": read("p1"),
        "srom": read("s1"),
        "m1rom": read("m1"),
        "vroma0": pad_pow2(read("v1")),
        "crom0": interleave_pairs(read("c1"), read("c2")),
        "fpga": b"10",
    }
    # the C ROM must come back out exactly
    c1, c2 = read("c1"), read("c2")
    crom = files["crom0"]
    back1 = bytearray(len(c1)); back2 = bytearray(len(c2))
    back1[0::2], back1[1::2] = crom[0::4], crom[1::4]
    back2[0::2], back2[1::2] = crom[2::4], crom[3::4]
    if bytes(back1) != c1 or bytes(back2) != c2:
        raise SystemExit("crom0 does not round-trip to C1/C2")
    manifest = {
        "game": name,
        "platform": platform,
        "format": "Darksoft Neo Geo Multi",
        "source": f"{rom_dir.as_posix()}/{game_id}-*",
        "files": {k: {"size": len(v), "size_hex": hex(len(v)),
                      "crc32": "%08x" % (zlib.crc32(v) & 0xffffffff),
                      "sha256": hashlib.sha256(v).hexdigest()} for k, v in files.items()},
        "verification": {"crom0_roundtrip_matches_C1_C2": True,
                         "v1_size": hex(len(read("v1"))), "vroma0_size": hex(len(files["vroma0"]))},
    }
    readme = (f"{name} -- Darksoft Neo Geo Multi ({platform})\n\n"
              "Copy this folder to a game folder on the Darksoft Multi SD card.\n"
              "No system BIOS is included: the Multi uses the board's.\n\n"
              "  prom    P ROM\n  srom    S (FIX) ROM\n  m1rom   Z80 M ROM\n"
              "  vroma0  V ROM, padded with $FF to a power of two\n"
              "  crom0   C1/C2 in Darksoft two-byte pair interleave\n  fpga    ASCII \"10\"\n\n"
              "Built by tools/pack_darksoft.py from the cartridge's own ROMs; crom0 is\n"
              "checked to come back out as C1 and C2 byte for byte (MANIFEST.json).\n")
    files["MANIFEST.json"] = (json.dumps(manifest, indent=2) + "\n").encode()
    files["README.txt"] = readme.encode()
    return files


def unpack(files, game_id):
    """A Darksoft folder's files (name -> bytes) as MAME ROM files (name -> bytes)."""
    crom = files["crom0"]
    c1 = bytearray(len(crom) // 2); c2 = bytearray(len(crom) // 2)
    c1[0::2], c1[1::2] = crom[0::4], crom[1::4]
    c2[0::2], c2[1::2] = crom[2::4], crom[3::4]
    v1 = files["vroma0"]
    # this tool's manifests carry the V ROM's own size; the v1 release's
    # (packed from a .neo, V already padded) don't, and vroma0 stays whole
    size = json.loads(files.get("MANIFEST.json", "{}")).get("verification", {}).get("v1_size")
    if size:
        size = int(size, 16)
        if set(v1[size:]) - {0xff}:
            raise SystemExit("vroma0 holds more than $FF padding past the V ROM's size")
        v1 = v1[:size]
    return {f"{game_id}-p1.p1": files["prom"], f"{game_id}-s1.s1": files["srom"],
            f"{game_id}-m1.m1": files["m1rom"], f"{game_id}-v1.v1": v1,
            f"{game_id}-c1.c1": bytes(c1), f"{game_id}-c2.c2": bytes(c2)}


def read_folder(path):
    """A Darksoft folder, or the one folder inside a zip, as name -> bytes."""
    if path.is_dir():
        return {f.name: f.read_bytes() for f in path.iterdir() if f.is_file()}
    with zipfile.ZipFile(path) as z:
        return {Path(n).name: z.read(n) for n in z.namelist() if not n.endswith("/")}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rom-dir", type=Path)
    ap.add_argument("--game-id", required=True)
    ap.add_argument("--name")
    ap.add_argument("--platform", choices=("MVS", "AES"))
    ap.add_argument("--out", type=Path, help="write the folder (or, with --unpack, the MAME ROMs) here")
    ap.add_argument("--zip", type=Path, help="and/or a zip holding the folder")
    ap.add_argument("--unpack", type=Path, help="a Darksoft folder or zip to turn back into a MAME set")
    args = ap.parse_args()
    if args.unpack:
        if not args.out:
            ap.error("--unpack needs --out")
        args.out.mkdir(parents=True, exist_ok=True)
        for k, v in unpack(read_folder(args.unpack), args.game_id).items():
            (args.out / k).write_bytes(v)
            print(f"darksoft: {args.out / k} {len(v)} {zlib.crc32(v) & 0xffffffff:08x}")
        return 0
    if not (args.rom_dir and args.name and args.platform):
        ap.error("--rom-dir, --name and --platform are needed to pack")
    files = build(args.rom_dir, args.game_id, args.platform, args.name)
    folder = (args.out.name if args.out else f"{args.name.split(':')[0]}_{args.platform}")
    if args.out:
        args.out.mkdir(parents=True, exist_ok=True)
        for k, v in files.items():
            (args.out / k).write_bytes(v)
        print(f"darksoft: {args.out}")
    if args.zip:
        with zipfile.ZipFile(args.zip, "w", zipfile.ZIP_DEFLATED) as z:
            for k, v in files.items():
                z.writestr(f"{folder}/{k}", v)
        print(f"darksoft: {args.zip}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
