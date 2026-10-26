#!/usr/bin/env python3
"""
Install the current build of a game into a MAME test folder, replacing the
copy that was there (no backup is kept: the folder is for trying builds).

    python3 tools/install_wip_mame.py --game sevenwonders --platform mvs --dest /mnt/c/mame/neogeosdk
    python3 tools/install_wip_mame.py --game sevenwonders --platform aes --dest /mnt/c/mame/neogeosdk --label 2026-10-17b

Run it after building that platform (roms/<game> holds the last build). The
game goes to tests/<game>/<platform>/ with its own software list, settings
and saves; EagleBIOS goes to tests/<game>/bios/. tools/mame_bats.py then
writes the folder's launchers and MENU.bat again (and, for a game with level
launchers, records this build's addresses from out/game), and <GAME>-TEST.txt
says which build is installed. --fresh also clears that platform's settings
and saves.
"""

import argparse
import datetime
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

import mame_bats

ROOT = Path(__file__).resolve().parents[1]
PARTS = ("p1", "m1", "s1", "v1", "c1", "c2")


def game_id(game):
    mk = (ROOT / "games" / game / "game.mk").read_text(encoding="utf-8")
    m = re.search(r"^GAME_ID\s*=\s*(\d+)", mk, re.M)
    if not m:
        raise SystemExit(f"install_wip_mame: no GAME_ID in games/{game}/game.mk")
    return m.group(1)


def game_name(game):
    mk = (ROOT / "games" / game / "game.mk").read_text(encoding="utf-8")
    m = re.search(r"^GAME_NAME\s*=\s*(.+)$", mk, re.M)
    return m.group(1).strip() if m else game


def crlf(path, text):
    path.write_bytes(text.replace("\r\n", "\n").replace("\n", "\r\n").encode("ascii"))


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", required=True)
    ap.add_argument("--platform", choices=("mvs", "aes"), required=True)
    ap.add_argument("--dest", required=True, help="the MAME test folder (C:\\mame\\neogeosdk is /mnt/c/mame/neogeosdk in WSL)")
    ap.add_argument("--label", default="", help="a name for this build in the note")
    ap.add_argument("--fresh", action="store_true", help="also clear this platform's cfg, nvram and states")
    args = ap.parse_args(argv)

    gid = game_id(args.game)
    src = ROOT / "roms" / args.game
    files = [src / f"{gid}-{p}.{p}" for p in PARTS]
    missing = [f.name for f in files if not f.is_file()]
    if missing:
        raise SystemExit(f"install_wip_mame: build first, missing {', '.join(missing)} in roms/{args.game}")
    env = dict(os.environ, GAME=args.game, GAME_ID=gid)
    subprocess.run([sys.executable, str(ROOT / "hash_eagle/gen_hash.py")], check=True, env=env, cwd=ROOT,
                   stdout=subprocess.DEVNULL)

    dest = Path(args.dest)
    if not dest.is_dir():
        raise SystemExit(f"install_wip_mame: {dest} is not a folder")
    base = dest / "tests" / args.game
    data = base / args.platform
    for sub in ("roms", "hash") + (("cfg", "nvram", "state") if args.fresh else ()):
        shutil.rmtree(data / sub, ignore_errors=True)
    (data / "roms" / args.game).mkdir(parents=True)
    (data / "hash").mkdir(parents=True)
    for f in files:
        shutil.copyfile(f, data / "roms" / args.game / f.name)
    shutil.copyfile(ROOT / "hash_eagle" / args.game / "neogeo.xml", data / "hash" / "neogeo.xml")
    for system in ("neogeo", "aes"):
        bios = base / "bios" / system
        shutil.rmtree(bios, ignore_errors=True)
        bios.mkdir(parents=True)
        for f in (ROOT / "bios/test_roms" / system).iterdir():
            if f.is_file():
                shutil.copyfile(f, bios / f.name)

    if args.game in mame_bats.PROBES:
        mame_bats.describe(dest, args.game, args.platform, ROOT / "out" / "game", ROOT)
    mame_bats.write_all(dest)

    note = dest / f"{args.game.upper()}-TEST.txt"
    builds = {}
    if note.is_file():
        for line in note.read_text(encoding="ascii", errors="replace").splitlines():
            m = re.match(r"^(MVS|AES) build: (.*)$", line)
            if m:
                builds[m.group(1)] = m.group(2)
    when = datetime.datetime.now().strftime("%Y-%m-%d %H:%M")
    builds[args.platform.upper()] = f"{args.label or 'work in progress'}, installed {when}"
    crlf(note, f"""{game_name(args.game)} - MAME test build
{'=' * (len(game_name(args.game)) + 20)}

MVS build: {builds.get('MVS', 'not installed')}
AES build: {builds.get('AES', 'not installed')}

Windows, from this folder: MENU.bat, or
    run_{args.game}_mvs.bat   MVS cartridge, EagleBIOS (euro)
    run_{args.game}_aes.bat   AES cartridge, EagleBIOS (asia)
    (the _scanlines launchers use the CRT picture; README-LAUNCHERS.txt has the rest)

Each platform keeps its own ROMs, settings and saves in tests\\{args.game}\\mvs and
tests\\{args.game}\\aes. A new build replaces the old one; settings and saves stay.
MAME warns that EagleBIOS's checksums differ from the original BIOS: expected.
This is a work-in-progress build for MAME: physical hardware is untested.
""")
    print(f"install_wip_mame: {args.game} {args.platform} -> {data}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
