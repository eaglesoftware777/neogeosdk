#!/usr/bin/env python3
#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
"""
Regenerates hash_eagle/<GAME>/neogeo.xml from the current ROM files in
roms/<GAME>/.  Must be run before launching MAME whenever any ROM is rebuilt.

Each game gets its own subdirectory so multiple builds can coexist on disk:
  hash_eagle/demo/neogeo.xml
  hash_eagle/helloworld/neogeo.xml
  ...

MAME resolves the correct XML via -hashpath which is set to
hash_eagle/<GAME>:hash_eagle:hash (game-specific dir first).

Usage:
  python3 hash_eagle/gen_hash.py          # update XML only
  python3 hash_eagle/gen_hash.py --dist   # update XML + build dist/ package

Environment:
  GAME    - game folder name (default: demo)
  GAME_ID - ROM file prefix  (default: 777)
"""
import argparse, binascii, hashlib, os, shutil, sys, zipfile

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT   = os.path.dirname(SCRIPT_DIR)

GAME    = os.environ.get("GAME",    "demo")
GAME_ID = os.environ.get("GAME_ID", "777")

ROM_DIR    = os.path.join(REPO_ROOT, "roms", GAME)
# Per-game hash directory: hash_eagle/<GAME>/neogeo.xml
HASH_GAME_DIR = os.path.join(SCRIPT_DIR, GAME)
OUT_XML       = os.path.join(HASH_GAME_DIR, "neogeo.xml")

ROM_NAMES = [
    f"{GAME_ID}-p1.p1",
    f"{GAME_ID}-m1.m1",
    f"{GAME_ID}-s1.s1",
    f"{GAME_ID}-v1.v1",
    f"{GAME_ID}-c1.c1",
    f"{GAME_ID}-c2.c2",
]

def file_info(name):
    path = os.path.join(ROM_DIR, name)
    if not os.path.exists(path):
        print(f"WARNING: {name} not found in {ROM_DIR}", file=sys.stderr)
        return None, None, None
    data = open(path, "rb").read()
    crc  = binascii.crc32(data) & 0xFFFFFFFF
    sha  = hashlib.sha1(data).hexdigest()
    return len(data), f"{crc:08x}", sha

def area_size(name):
    path = os.path.join(ROM_DIR, name)
    return os.path.getsize(path) if os.path.exists(path) else 0

def gen_xml():
    os.makedirs(HASH_GAME_DIR, exist_ok=True)

    p1_sz, p1_crc, p1_sha = file_info(f"{GAME_ID}-p1.p1")
    m1_sz, m1_crc, m1_sha = file_info(f"{GAME_ID}-m1.m1")
    s1_sz, s1_crc, s1_sha = file_info(f"{GAME_ID}-s1.s1")
    v1_sz, v1_crc, v1_sha = file_info(f"{GAME_ID}-v1.v1")
    c1_sz, c1_crc, c1_sha = file_info(f"{GAME_ID}-c1.c1")
    c2_sz, c2_crc, c2_sha = file_info(f"{GAME_ID}-c2.c2")

    def sz(n): return f"0x{n:06x}" if n else "0x000000"

    maincpu_area = 0x100000
    sprites_area = (c1_sz or 0) + (c2_sz or 0)

    xml = f"""\
<?xml version="1.0"?>
<!DOCTYPE softwarelist PUBLIC "-//MAME//DTD Software List//EN" "https://raw.githubusercontent.com/mamedev/mame/master/src/mame/mame.dtd">
<softwarelist name="neogeo" description="SNK Neo-Geo cartridges">
    <!--
    ID-0{GAME_ID}
    . NGM-{GAME_ID}
    NEO-MVS PROG-G2 (SNK-9201) / NEO-MVS CHA 42G-2
    . NGH-{GAME_ID}
    -->

    <software name="{GAME}">
        <description>NeoGeo SDK {GAME} v1.2.0 / Eagle Software</description>
        <year>2026</year>
        <publisher>Eagle Software</publisher>
        <info name="serial" value="NGM-{GAME_ID} (MVS), NGH-{GAME_ID} (AES)"/>
        <info name="release" value="20260101 (MVS), 20260101 (AES)"/>
        <info name="alt_title" value="Eagle SDK"/>
        <sharedfeat name="release" value="MVS,AES" />
        <sharedfeat name="compatibility" value="MVS,AES" />
        <part name="cart" interface="neo_cart">
            <feature name="slot" value="rom_fatfur2" />
            <dataarea name="maincpu" width="16" endianness="big" size="{sz(maincpu_area)}">
                <rom loadflag="load16_word_swap" name="{GAME_ID}-p1.p1" offset="0x000000" size="{sz(p1_sz)}" crc="{p1_crc}" sha1="{p1_sha}" />
            </dataarea>
            <dataarea name="fixed" size="{sz(s1_sz)}">
                <rom offset="0x000000" size="{sz(s1_sz)}" name="{GAME_ID}-s1.s1" crc="{s1_crc}" sha1="{s1_sha}" />
            </dataarea>
            <dataarea name="audiocpu" size="{sz(m1_sz)}">
                <rom offset="0x000000" size="{sz(m1_sz)}" name="{GAME_ID}-m1.m1" crc="{m1_crc}" sha1="{m1_sha}" />
            </dataarea>
            <dataarea name="ymsnd:adpcma" size="{sz(v1_sz)}">
                <rom name="{GAME_ID}-v1.v1" offset="0x000000" size="{sz(v1_sz)}" crc="{v1_crc}" sha1="{v1_sha}" />
            </dataarea>
            <dataarea name="sprites" size="{sz(sprites_area)}">
                <rom loadflag="load16_byte" name="{GAME_ID}-c1.c1" offset="0x000000" size="{sz(c1_sz)}" crc="{c1_crc}" sha1="{c1_sha}" />
                <rom loadflag="load16_byte" name="{GAME_ID}-c2.c2" offset="0x000001" size="{sz(c2_sz)}" crc="{c2_crc}" sha1="{c2_sha}" />
            </dataarea>
        </part>
    </software>
</softwarelist>
"""
    with open(OUT_XML, "w", newline="\n") as f:
        f.write(xml)
    print(f"Generated {OUT_XML}")

def build_dist():
    dist_dir  = os.path.join(REPO_ROOT, "dist")
    roms_dir  = os.path.join(dist_dir, "roms")
    # Per-game hash directory in dist mirrors the hash_eagle layout
    hash_dir  = os.path.join(dist_dir, "hash_eagle", GAME)
    os.makedirs(roms_dir, exist_ok=True)
    os.makedirs(hash_dir, exist_ok=True)

    zip_path = os.path.join(roms_dir, f"{GAME}.zip")
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_STORED) as zf:
        for rom in ROM_NAMES:
            src = os.path.join(ROM_DIR, rom)
            if os.path.exists(src):
                zf.write(src, rom)
            else:
                print(f"WARNING: {rom} missing — skipped in ZIP", file=sys.stderr)
    print(f"Created {zip_path}")

    shutil.copy2(OUT_XML, os.path.join(hash_dir, "neogeo.xml"))

    _write_dist_bat(dist_dir, debug=False)
    _write_dist_bat(dist_dir, debug=True)
    _write_dist_sh(dist_dir, debug=False)
    _write_dist_sh(dist_dir, debug=True)
    print(f"Dist ready: {dist_dir}")

def _write_dist_bat(dist_dir, debug):
    name = f"run_{GAME}_debug.bat" if debug else f"run_{GAME}.bat"
    debug_flags = " ^\n    -debug" if debug else ""
    content = f"""\
@echo off
setlocal
REM NeoGeo SDK - {'Debug ' if debug else ''}Release Launcher ({GAME})
REM Place neogeo.zip (BIOS) inside the roms\\ folder before running.
mame neogeo -cart1 {GAME} ^
    -rompath "%~dp0roms" ^
    -hashpath "%~dp0hash_eagle\\{GAME};%~dp0hash_eagle;%~dp0hash" ^
    -bios unibios22 ^
    -window ^
    -console ^
    -verbose{debug_flags}
endlocal
"""
    path = os.path.join(dist_dir, name)
    with open(path, "w", newline="\r\n") as f:
        f.write(content)
    print(f"Written {path}")

def _write_dist_sh(dist_dir, debug):
    name = f"run_{GAME}_debug.sh" if debug else f"run_{GAME}.sh"
    extra = " \\\n    -debug" if debug else ""
    content = f"""\
#!/bin/bash
# NeoGeo SDK - {'Debug ' if debug else ''}Release Launcher ({GAME})
# Place neogeo.zip (BIOS) inside roms/ before running.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec mame neogeo \\
    -cart1 {GAME} \\
    -rompath "$SCRIPT_DIR/roms" \\
    -hashpath "$SCRIPT_DIR/hash_eagle/{GAME}:$SCRIPT_DIR/hash_eagle:$SCRIPT_DIR/hash" \\
    -bios unibios22 \\
    -window \\
    -console \\
    -verbose{extra}
"""
    path = os.path.join(dist_dir, name)
    with open(path, "w", newline="\n") as f:
        f.write(content)
    os.chmod(path, 0o755)
    print(f"Written {path}")

def main():
    ap = argparse.ArgumentParser(description="Regenerate neogeo.xml and optionally build dist/")
    ap.add_argument("--dist", action="store_true", help="also build the dist/ release package")
    args = ap.parse_args()

    gen_xml()
    if args.dist:
        build_dist()

if __name__ == "__main__":
    main()
