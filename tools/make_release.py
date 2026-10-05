#!/usr/bin/env python3
"""
A game's release files, from `make neo GAME=<game>` (dist/neo/<game>/: the
MVS and AES MAME sets and NeoSD images) and EagleBIOS (bios/), named with
a version suffix into dist/release/:

    <Prefix>-WIP-NeoSD_MVS_<v>.neo       NeoSD images (cartridge data only)
    <Prefix>-WIP-NeoSD_AES_<v>.neo
    <Prefix>-Darksoft-MVS_<v>.zip        Darksoft Multi folders (tools/pack_darksoft.py)
    <Prefix>-Darksoft-AES_<v>.zip
    <Prefix>-BackBit-MVS_<v>.zip         BackBit Platinum folders (tools/pack_backbit.py)
    <Prefix>-BackBit-AES_<v>.zip
    <Prefix>-WIP-EagleBIOS_<v>.zip       MAME: the MVS cartridge with EagleBIOS
    <Prefix>-WIP-AES-EagleBIOS_<v>.zip   MAME: the AES cartridge with EagleBIOS
    <Prefix>-MANIFEST_<v>.txt            every file's size, CRC32 and SHA-256

The only system ROM that goes in is EagleBIOS, the open firmware built in
bios/: no SNK BIOS and no UniBIOS, whatever sits in roms/neogeo or
roms/aes. An existing file is never overwritten (released files stay as
they were shared), unless --replace asks to rebuild this version's own
files: then only those whose content changes are rewritten, and the
script lists them. Files of other versions are never touched.

    python3 tools/make_release.py --game maiya --game-id 780 --prefix Maiya \\
        --name "Maiya: Super Nature Girl" --version v2 [--readme notes.txt]
"""

import argparse
import hashlib
import shutil
import sys
import tempfile
import zipfile
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from pack_darksoft import build as darksoft  # noqa: E402
from pack_backbit import build as backbit, mame_set  # noqa: E402

EAGLE = {"neogeo": ("sp-s2.sp1", "sm1.sm1", "sfix.sfix", "000-lo.lo"), "aes": ("neo-epo.bin", "000-lo.lo")}
PARTS = ("p1", "s1", "m1", "v1", "c1", "c2")


def eagle_zip(dest, game, game_id, mame_set, boards, run_text, extra=None):
    with zipfile.ZipFile(dest, "w", zipfile.ZIP_DEFLATED) as z:
        for name, data in (extra or {}).items():
            z.writestr(name, data)
        for part in PARTS:
            z.write(mame_set / "roms" / game / f"{game_id}-{part}.{part}", f"roms/{game}/{game_id}-{part}.{part}")
        for board in boards:
            for name in EAGLE[board]:
                z.write(ROOT / "bios" / name, f"roms/{board}/{name}")
        z.write(mame_set / "hash" / "neogeo.xml", "hash/neogeo.xml")
        z.write(ROOT / "bios" / "README.md", "EagleBIOS.md")
        z.write(ROOT / "LICENSE", "LICENSE")
        z.writestr("RUN.txt", run_text)


def members(path):
    """A zip's (name, size, CRC) list: equal lists mean the same files inside."""
    with zipfile.ZipFile(path) as z:
        return sorted((info.filename, info.file_size, info.CRC) for info in z.infolist())


def place(new, dest):
    """Move a staged file into place unless dest already holds the same content."""
    if dest.exists():
        same = (members(new) == members(dest)) if dest.suffix == ".zip" else (new.read_bytes() == dest.read_bytes())
        if same:
            return "unchanged"
        shutil.move(str(new), str(dest))
        return "REPLACED"
    shutil.move(str(new), str(dest))
    return "new"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", required=True)
    ap.add_argument("--game-id", required=True)
    ap.add_argument("--prefix", required=True)
    ap.add_argument("--name", required=True)
    ap.add_argument("--version", required=True)
    ap.add_argument("--readme", type=Path, help="release notes to ship as <Prefix>-README_<v>.txt")
    ap.add_argument("--changelog", type=Path, help="a changelog to ship as <Prefix>-CHANGELOG_<v>.txt")
    ap.add_argument("--replace", action="store_true", help="rebuild this version's files, rewriting changed ones")
    args = ap.parse_args()
    src = ROOT / "dist" / "neo" / args.game
    out = ROOT / "dist" / "release"
    v, p, g, i = args.version, args.prefix, args.game, args.game_id
    names = {
        "neo_mvs": f"{p}-WIP-NeoSD_MVS_{v}.neo", "neo_aes": f"{p}-WIP-NeoSD_AES_{v}.neo",
        "ds_mvs": f"{p}-Darksoft-MVS_{v}.zip", "ds_aes": f"{p}-Darksoft-AES_{v}.zip",
        "bb_mvs": f"{p}-BackBit-MVS_{v}.zip", "bb_aes": f"{p}-BackBit-AES_{v}.zip",
        "eagle_mvs": f"{p}-WIP-EagleBIOS_{v}.zip", "eagle_aes": f"{p}-WIP-AES-EagleBIOS_{v}.zip",
        "manifest": f"{p}-MANIFEST_{v}.txt",
    }
    if args.readme:
        names["readme"] = f"{p}-README_{v}.txt"
    if args.changelog:
        names["changelog"] = f"{p}-CHANGELOG_{v}.txt"
    clash = [n for n in names.values() if (out / n).exists()]
    if clash and not args.replace:
        raise SystemExit("make_release: these exist already and stay as released: " + ", ".join(clash))
    for plat in ("mvs", "aes"):
        for part in PARTS:
            if not (src / plat / "roms" / g / f"{i}-{part}.{part}").is_file():
                raise SystemExit(f"make_release: run make neo GAME={g} first ({plat} {part})")
    out.mkdir(parents=True, exist_ok=True)
    stage = Path(tempfile.mkdtemp(prefix=".stage-", dir=out))
    try:
        shutil.copy2(src / f"{g}-mvs.neo", stage / names["neo_mvs"])
        shutil.copy2(src / f"{g}-aes.neo", stage / names["neo_aes"])
        for plat, key in (("MVS", "ds_mvs"), ("AES", "ds_aes")):
            files = darksoft(src / plat.lower() / "roms" / g, i, plat, args.name)
            folder = f"{p}_{plat}"
            with zipfile.ZipFile(stage / names[key], "w", zipfile.ZIP_DEFLATED) as z:
                for k, data in files.items():
                    z.writestr(f"{folder}/{k}", data)
        for plat, key in (("MVS", "bb_mvs"), ("AES", "bb_aes")):
            files = backbit(mame_set(src / plat.lower() / "roms" / g, i), g, args.name, plat)
            with zipfile.ZipFile(stage / names[key], "w", zipfile.ZIP_DEFLATED) as z:
                for k, data in files.items():
                    z.writestr(k, data)
        eagle_zip(stage / names["eagle_mvs"], g, i, src / "mvs", ("neogeo", "aes"),
                  f"mame neogeo -rompath roms -hashpath hash -bios euro -cart1 {g}\n"
                  "EagleBIOS stands in for the system ROM: checksum warnings for it are expected.\n"
                  f"For a console, use {p}-WIP-AES-EagleBIOS_{v}.zip (the AES build).\n")
        eagle_zip(stage / names["eagle_aes"], g, i, src / "aes", ("aes",),
                  f"mame aes -rompath roms -hashpath hash -bios asia -cart1 {g} -memc card.bin\n"
                  "EagleBIOS stands in for the system ROM: checksum warnings for it are expected.\n"
                  "card.bin is a blank 2 KiB memory card: the game's saves are kept on it.\n",
                  {"card.bin": bytes(2048)})
        if args.readme:
            shutil.copy2(args.readme, stage / names["readme"])
        if args.changelog:
            (stage / names["changelog"]).write_bytes(
                args.changelog.read_bytes().replace(b"\r\n", b"\n").replace(b"\n", b"\r\n"))
        # nothing but EagleBIOS may be a system ROM in there
        eagle_crc = {zlib.crc32((ROOT / "bios" / n).read_bytes()) & 0xffffffff for b in EAGLE.values() for n in b}
        for key in ("eagle_mvs", "eagle_aes"):
            with zipfile.ZipFile(stage / names[key]) as z:
                for info in z.infolist():
                    if info.filename.startswith(("roms/neogeo/", "roms/aes/")) and info.CRC not in eagle_crc:
                        raise SystemExit(f"make_release: {names[key]}: {info.filename} is not EagleBIOS")
        # a released file whose content is the same stays byte for byte as it was shared
        state = {}
        for key, n in names.items():
            if key == "manifest":
                continue
            state[n] = place(stage / n, out / n)
        lines = [f"# {p} {v}: dist/release, made by tools/make_release.py", "# size  crc32     sha256  file"]
        for key, n in names.items():
            if key == "manifest":
                continue
            data = (out / n).read_bytes()
            lines.append(f"{len(data):9d}  {zlib.crc32(data) & 0xffffffff:08x}  {hashlib.sha256(data).hexdigest()}  {n}")
        (stage / names["manifest"]).write_text("\n".join(lines) + "\n")
        state[names["manifest"]] = place(stage / names["manifest"], out / names["manifest"])
    finally:
        shutil.rmtree(stage, ignore_errors=True)
    for n in names.values():
        print(f"release: dist/release/{n} ({state[n]})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
