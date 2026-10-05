#!/usr/bin/env python3
"""
A cartridge's ROMs as a BackBit Platinum game folder, named after the game:

    <game>/<game>.p1   P ROM
    <game>/<game>.s1   S (FIX) ROM
    <game>/<game>.m1   Z80 M ROM
    <game>/<game>.v1   V ROM
    <game>/<game>.c1   C ROM, planes 0 and 1
    <game>/<game>.c2   C ROM, planes 2 and 3
    <game>/README.txt

From the cartridge's own ROMs (a MAME set: <id>-p1.p1 ...), or from a
Darksoft Neo Geo Multi folder or zip (tools/pack_darksoft.py). Either way
the six ROM files come out byte for byte the MAME set's. No system BIOS
goes in: the BackBit uses the board's.

    python3 tools/pack_backbit.py --rom-dir roms/maiya --game-id 780 --game maiya \\
        --name "Maiya: Super Nature Girl" --platform MVS --zip Maiya-BackBit-MVS.zip
    python3 tools/pack_backbit.py --darksoft Maiya-Darksoft-MVS_v2.zip --game-id 780 --game maiya \\
        --name "Maiya: Super Nature Girl" --platform MVS --zip Maiya-BackBit-MVS_v2.zip
"""

import argparse
import sys
import zipfile
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from pack_darksoft import read_folder, unpack  # noqa: E402

PARTS = ("p1", "s1", "m1", "v1", "c1", "c2")


def mame_set(rom_dir, game_id):
    """A MAME set's six ROM files, by part."""
    return {part: (rom_dir / f"{game_id}-{part}.{part}").read_bytes() for part in PARTS}


def from_darksoft(path, game_id):
    """A Darksoft folder's ROMs, by part."""
    roms = unpack(read_folder(path), game_id)
    return {part: roms[f"{game_id}-{part}.{part}"] for part in PARTS}


def build(roms, game, name, platform):
    """The BackBit folder's files: path inside the zip -> bytes."""
    files = {f"{game}/{game}.{part}": roms[part] for part in PARTS}
    lines = "\n".join(f"  {game}.{part}  {len(roms[part]):>8}  {zlib.crc32(roms[part]) & 0xffffffff:08x}"
                      for part in PARTS)
    files[f"{game}/README.txt"] = (
        f"{name} -- BackBit Platinum ({platform})\n\n"
        f"Copy the {game} folder to the BackBit Platinum's microSD card.\n"
        "No system BIOS is included: the BackBit uses the board's.\n\n"
        f"{lines}\n").encode()
    return files


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rom-dir", type=Path, help="the MAME set's ROMs")
    ap.add_argument("--darksoft", type=Path, help="or a Darksoft folder or zip")
    ap.add_argument("--game-id", required=True)
    ap.add_argument("--game", required=True, help="the folder's and the files' name (maiya)")
    ap.add_argument("--name", required=True)
    ap.add_argument("--platform", choices=("MVS", "AES"), required=True)
    ap.add_argument("--out", type=Path, help="write the folder under this directory")
    ap.add_argument("--zip", type=Path, help="and/or a zip holding the folder")
    args = ap.parse_args()
    if bool(args.rom_dir) == bool(args.darksoft):
        ap.error("give one of --rom-dir and --darksoft")
    roms = mame_set(args.rom_dir, args.game_id) if args.rom_dir else from_darksoft(args.darksoft, args.game_id)
    files = build(roms, args.game, args.name, args.platform)
    if args.out:
        for k, v in files.items():
            (args.out / k).parent.mkdir(parents=True, exist_ok=True)
            (args.out / k).write_bytes(v)
        print(f"backbit: {args.out / args.game}")
    if args.zip:
        with zipfile.ZipFile(args.zip, "w", zipfile.ZIP_DEFLATED) as z:
            for k, v in files.items():
                z.writestr(k, v)
        print(f"backbit: {args.zip}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
