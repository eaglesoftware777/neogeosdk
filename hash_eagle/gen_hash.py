#!/usr/bin/env python3
#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
"""
Regenerates hash_eagle/neogeo.xml from the current ROM files in roms/neogeosdk/.
Must be run before launching MAME whenever any ROM is rebuilt.

Usage:
  python3 hash_eagle/gen_hash.py          # update XML only
  python3 hash_eagle/gen_hash.py --dist   # update XML + build dist/ package
"""
import argparse, binascii, hashlib, os, shutil, sys, zipfile

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT   = os.path.dirname(SCRIPT_DIR)
ROM_DIR     = os.path.join(REPO_ROOT, "roms", "neogeosdk")
OUT_XML     = os.path.join(SCRIPT_DIR, "neogeo.xml")

ROM_NAMES = ["777-p1.p1", "777-m1.m1", "777-s1.s1", "777-v1.v1", "777-c1.c1", "777-c2.c2"]

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
    p1_sz, p1_crc, p1_sha = file_info("777-p1.p1")
    m1_sz, m1_crc, m1_sha = file_info("777-m1.m1")
    s1_sz, s1_crc, s1_sha = file_info("777-s1.s1")
    v1_sz, v1_crc, v1_sha = file_info("777-v1.v1")
    c1_sz, c1_crc, c1_sha = file_info("777-c1.c1")
    c2_sz, c2_crc, c2_sha = file_info("777-c2.c2")

    def sz(n): return f"0x{n:06x}" if n else "0x000000"

    # maincpu window is always 1 MB on Neo Geo regardless of P1 size
    maincpu_area = 0x100000
    # sprite area = c1 + c2 interleaved (each ROM covers half the words)
    sprites_area = (c1_sz or 0) + (c2_sz or 0)

    # continue entries fill the gap when sprite ROMs don't cover the full bank set
    # for our ROMs c1 == c2 in size so no gap; include continue block only if needed
    c_continue = sprites_area < maincpu_area  # heuristic: add if sprites < 1 MB

    xml = f"""\
<?xml version="1.0"?>
<!DOCTYPE softwarelist PUBLIC "-//MAME//DTD Software List//EN" "https://raw.githubusercontent.com/mamedev/mame/master/src/mame/mame.dtd">
<softwarelist name="neogeo" description="SNK Neo-Geo cartridges">
    <!--
    ID-0777
    . NGM-777
    NEO-MVS PROG-G2 (SNK-9201) / NEO-MVS CHA 42G-2
    . NGH-777
    -->

    <software name="neogeosdk">
        <description>NeoGeo SDK Demo v1.2.1 / Eagle Software</description>
        <year>2026</year>
        <publisher>Eagle Software</publisher>
        <info name="serial" value="NGM-777 (MVS), NGH-777 (AES)"/>
        <info name="release" value="20260101 (MVS), 20260101 (AES)"/>
        <info name="alt_title" value="Eagle SDK"/>
        <sharedfeat name="release" value="MVS,AES" />
        <sharedfeat name="compatibility" value="MVS,AES" />
        <part name="cart" interface="neo_cart">
            <feature name="slot" value="rom_fatfur2" />
            <dataarea name="maincpu" width="16" endianness="big" size="{sz(maincpu_area)}">
                <rom loadflag="load16_word_swap" name="777-p1.p1" offset="0x000000" size="{sz(p1_sz)}" crc="{p1_crc}" sha1="{p1_sha}" />
            </dataarea>
            <dataarea name="fixed" size="{sz(s1_sz)}">
                <rom offset="0x000000" size="{sz(s1_sz)}" name="777-s1.s1" crc="{s1_crc}" sha1="{s1_sha}" />
            </dataarea>
            <dataarea name="audiocpu" size="{sz(m1_sz)}">
                <rom offset="0x000000" size="{sz(m1_sz)}" name="777-m1.m1" crc="{m1_crc}" sha1="{m1_sha}" />
            </dataarea>
            <dataarea name="ymsnd:adpcma" size="{sz(v1_sz)}">
                <rom name="777-v1.v1" offset="0x000000" size="{sz(v1_sz)}" crc="{v1_crc}" sha1="{v1_sha}" />
            </dataarea>
            <dataarea name="sprites" size="{sz(sprites_area)}">
                <rom loadflag="load16_byte" name="777-c1.c1" offset="0x000000" size="{sz(c1_sz)}" crc="{c1_crc}" sha1="{c1_sha}" />
                <rom loadflag="load16_byte" name="777-c2.c2" offset="0x000001" size="{sz(c2_sz)}" crc="{c2_crc}" sha1="{c2_sha}" />
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
    hash_dir  = os.path.join(dist_dir, "hash_eagle")
    os.makedirs(roms_dir, exist_ok=True)
    os.makedirs(hash_dir, exist_ok=True)

    zip_path = os.path.join(roms_dir, "neogeosdk.zip")
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
    name = "run_neogeosdk_debug.bat" if debug else "run_neogeosdk.bat"
    debug_flags = " ^\n    -debug" if debug else ""
    content = f"""\
@echo off
setlocal
REM NeoGeo SDK - {'Debug ' if debug else ''}Release Launcher
REM Place neogeo.zip (BIOS) inside the roms\\ folder before running.
REM Correct launch: mame neogeo -cart1 neogeosdk (NOT mame neogeosdk)
mame neogeo -cart1 neogeosdk ^
    -rompath "%~dp0roms" ^
    -hashpath "%~dp0hash_eagle;%~dp0hash" ^
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
    name = "run_neogeosdk_debug.sh" if debug else "run_neogeosdk.sh"
    extra = " \\\n    -debug" if debug else ""
    content = f"""\
#!/bin/bash
# NeoGeo SDK - {'Debug ' if debug else ''}Release Launcher (Linux)
# Place neogeo.zip (BIOS) inside roms/ before running.
# Correct launch: mame neogeo -cart1 neogeosdk  (NOT mame neogeosdk)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec mame neogeo \\
    -cart1 neogeosdk \\
    -rompath "$SCRIPT_DIR/roms" \\
    -hashpath "$SCRIPT_DIR/hash_eagle:$SCRIPT_DIR/hash" \\
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
