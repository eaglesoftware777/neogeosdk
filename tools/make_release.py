#!/usr/bin/env python3
"""
A game's release files, from `make neo GAME=<game>` (dist/neo/<game>/: the
MVS and AES MAME sets and NeoSD images) and EagleBIOS (bios/), named with
a version suffix into dist/release/:

    <Prefix>-WIP-NeoSD_MVS_<v>.neo       NeoSD images (cartridge data only)
    <Prefix>-WIP-NeoSD_AES_<v>.neo
    <Prefix>-Darksoft-MVS_<v>.zip        Darksoft Multi folders (tools/pack_darksoft.py)
    <Prefix>-Darksoft-AES_<v>.zip
    <Prefix>-WIP-EagleBIOS_<v>.zip       MAME: the MVS cartridge with EagleBIOS
    <Prefix>-WIP-AES-EagleBIOS_<v>.zip   MAME: the AES cartridge with EagleBIOS
    <Prefix>-MANIFEST_<v>.txt            every file's size, CRC32 and SHA-256

The only system ROM that goes in is EagleBIOS, the open firmware built in
bios/: no SNK BIOS and no UniBIOS, whatever sits in roms/neogeo or
roms/aes. An existing file is never overwritten (released files stay as
they were shared).

    python3 tools/make_release.py --game maiya --game-id 780 --prefix Maiya \\
        --name "Maiya: Super Nature Girl" --version v2 [--readme notes.txt]
"""

import argparse
import hashlib
import shutil
import sys
import zipfile
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from pack_darksoft import build as darksoft  # noqa: E402

EAGLE = {"neogeo": ("sp-s2.sp1", "sm1.sm1", "sfix.sfix", "000-lo.lo"), "aes": ("neo-epo.bin", "000-lo.lo")}
PARTS = ("p1", "s1", "m1", "v1", "c1", "c2")


def eagle_zip(dest, game, game_id, mame_set, boards, run_text):
    with zipfile.ZipFile(dest, "w", zipfile.ZIP_DEFLATED) as z:
        for part in PARTS:
            z.write(mame_set / "roms" / game / f"{game_id}-{part}.{part}", f"roms/{game}/{game_id}-{part}.{part}")
        for board in boards:
            for name in EAGLE[board]:
                z.write(ROOT / "bios" / name, f"roms/{board}/{name}")
        z.write(mame_set / "hash" / "neogeo.xml", "hash/neogeo.xml")
        z.write(ROOT / "bios" / "README.md", "EagleBIOS.md")
        z.write(ROOT / "LICENSE", "LICENSE")
        z.writestr("RUN.txt", run_text)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", required=True)
    ap.add_argument("--game-id", required=True)
    ap.add_argument("--prefix", required=True)
    ap.add_argument("--name", required=True)
    ap.add_argument("--version", required=True)
    ap.add_argument("--readme", type=Path, help="release notes to ship as <Prefix>-README_<v>.txt")
    args = ap.parse_args()
    src = ROOT / "dist" / "neo" / args.game
    out = ROOT / "dist" / "release"
    v, p, g, i = args.version, args.prefix, args.game, args.game_id
    names = {
        "neo_mvs": f"{p}-WIP-NeoSD_MVS_{v}.neo", "neo_aes": f"{p}-WIP-NeoSD_AES_{v}.neo",
        "ds_mvs": f"{p}-Darksoft-MVS_{v}.zip", "ds_aes": f"{p}-Darksoft-AES_{v}.zip",
        "eagle_mvs": f"{p}-WIP-EagleBIOS_{v}.zip", "eagle_aes": f"{p}-WIP-AES-EagleBIOS_{v}.zip",
        "manifest": f"{p}-MANIFEST_{v}.txt",
    }
    if args.readme:
        names["readme"] = f"{p}-README_{v}.txt"
    clash = [n for n in names.values() if (out / n).exists()]
    if clash:
        raise SystemExit("make_release: these exist already and stay as released: " + ", ".join(clash))
    for plat in ("mvs", "aes"):
        for part in PARTS:
            if not (src / plat / "roms" / g / f"{i}-{part}.{part}").is_file():
                raise SystemExit(f"make_release: run make neo GAME={g} first ({plat} {part})")
    out.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src / f"{g}-mvs.neo", out / names["neo_mvs"])
    shutil.copy2(src / f"{g}-aes.neo", out / names["neo_aes"])
    for plat, key in (("MVS", "ds_mvs"), ("AES", "ds_aes")):
        files = darksoft(src / plat.lower() / "roms" / g, i, plat, args.name)
        folder = f"{p}_{plat}"
        with zipfile.ZipFile(out / names[key], "w", zipfile.ZIP_DEFLATED) as z:
            for k, data in files.items():
                z.writestr(f"{folder}/{k}", data)
    eagle_zip(out / names["eagle_mvs"], g, i, src / "mvs", ("neogeo", "aes"),
              f"mame neogeo -rompath roms -hashpath hash -bios euro -cart1 {g}\n"
              "EagleBIOS stands in for the system ROM: checksum warnings for it are expected.\n"
              f"For a console, use {p}-WIP-AES-EagleBIOS_{v}.zip (the AES build).\n")
    eagle_zip(out / names["eagle_aes"], g, i, src / "aes", ("aes",),
              f"mame aes -rompath roms -hashpath hash -bios asia -cart1 {g}\n"
              "EagleBIOS stands in for the system ROM: checksum warnings for it are expected.\n"
              "Memory card saves need a system ROM with the CARD routine (EagleBIOS's answers 'no card').\n")
    if args.readme:
        shutil.copy2(args.readme, out / names["readme"])
    # nothing but EagleBIOS may be a system ROM in there
    eagle_crc = {zlib.crc32((ROOT / "bios" / n).read_bytes()) & 0xffffffff for b in EAGLE.values() for n in b}
    for key in ("eagle_mvs", "eagle_aes"):
        with zipfile.ZipFile(out / names[key]) as z:
            for info in z.infolist():
                if info.filename.startswith(("roms/neogeo/", "roms/aes/")) and info.CRC not in eagle_crc:
                    raise SystemExit(f"make_release: {names[key]}: {info.filename} is not EagleBIOS")
    lines = [f"# {p} {v}: dist/release, made by tools/make_release.py", "# size  crc32     sha256  file"]
    for key, n in names.items():
        if key == "manifest":
            continue
        data = (out / n).read_bytes()
        lines.append(f"{len(data):9d}  {zlib.crc32(data) & 0xffffffff:08x}  {hashlib.sha256(data).hexdigest()}  {n}")
    (out / names["manifest"]).write_text("\n".join(lines) + "\n")
    for n in names.values():
        print(f"release: dist/release/{n}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
