#!/usr/bin/env python3
"""
`make neo GAME=<name>`'s second half: after a platform's ROMs are built into
roms/<game>/ (and its software list into hash_eagle/<game>/), put them in
dist/neo/<game>/<platform>/ as a MAME test set (roms/<game>/ and
hash/neogeo.xml) and pack them into a NeoSD image,
dist/neo/<game>/<game>-<platform>.neo (tools/pack_neosd.py: P, S and M
padded to 64 KiB, V to a power of two, C interleaved and padded to
256 KiB). With --manifest, list every file there with its size, CRC32 and
SHA-256 in dist/neo/<game>/MANIFEST.txt.

No system BIOS goes into dist/neo: a NeoSD cart brings its own, and MAME
uses the one in its own rompath.
"""

import argparse
import hashlib
import shutil
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from pack_neosd import build_image  # noqa: E402

PARTS = ("p1", "s1", "m1", "v1", "c1", "c2")

def display(path):
    try:
        return str(Path(path).resolve().relative_to(ROOT))
    except ValueError:
        return str(Path(path).resolve())


def pack(game, game_id, platform, name, output_root=None):
    rom_dir = ROOT / "roms" / game
    out = Path(output_root) if output_root else ROOT / "dist" / "neo" / game
    mame = out / platform
    if mame.exists():
        shutil.rmtree(mame)
    (mame / "roms" / game).mkdir(parents=True)
    (mame / "hash").mkdir(parents=True)
    parts = {}
    for key in PARTS:
        src = rom_dir / f"{game_id}-{key}.{key}"
        parts[key] = src.read_bytes()
        shutil.copy2(src, mame / "roms" / game / src.name)
    shutil.copy2(ROOT / "hash_eagle" / game / "neogeo.xml", mame / "hash" / "neogeo.xml")
    image = build_image(parts, name, "Eagle Software", 2026, 5, int(game_id, 16))
    neo = out / f"{game}-{platform}.neo"
    neo.write_bytes(image)
    print(f"neo: {display(neo)} ({len(image)} bytes), MAME set {display(mame)}")


def manifest(game, output_root=None):
    out = Path(output_root) if output_root else ROOT / "dist" / "neo" / game
    lines = [f"# {display(out)}: built for GAME={game}", "# size  crc32     sha256  path"]
    for path in sorted(p for p in out.rglob("*") if p.is_file() and p.name != "MANIFEST.txt"):
        data = path.read_bytes()
        lines.append(f"{len(data):9d}  {zlib.crc32(data) & 0xffffffff:08x}  "
                     f"{hashlib.sha256(data).hexdigest()}  {path.relative_to(out)}")
    (out / "MANIFEST.txt").write_text("\n".join(lines) + "\n")
    print(f"neo: {display(out / 'MANIFEST.txt')}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", required=True)
    ap.add_argument("--game-id")
    ap.add_argument("--platform", choices=("mvs", "aes"))
    ap.add_argument("--name", help="the title in the NeoSD header (33 characters at most)")
    ap.add_argument("--manifest", action="store_true")
    ap.add_argument("--output-root", type=Path, help="Separate candidate directory; released packages remain intact")
    args = ap.parse_args()
    if args.manifest:
        manifest(args.game, args.output_root)
        return 0
    if not (args.game_id and args.platform and args.name):
        ap.error("--game-id, --platform and --name are needed to pack")
    pack(args.game, args.game_id, args.platform, args.name[:33], args.output_root)
    return 0


if __name__ == "__main__":
    sys.exit(main())
