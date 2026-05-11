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

def rom_tag(name, offset=None, loadflag=None):
    size, crc, sha = file_info(name)
    if size is None:
        return f'            <!-- {name}: MISSING -->'
    attrs = f'name="{name}" size="{size:#010x}" crc="{crc}" sha1="{sha}"'
    if offset is not None:
        attrs += f' offset="{offset:#010x}"'
    if loadflag:
        attrs += f' loadflag="{loadflag}"'
    return f'            <rom {attrs}/>'

def area_size(name):
    path = os.path.join(ROM_DIR, name)
    return os.path.getsize(path) if os.path.exists(path) else 0

def gen_xml():
    p1_sz = area_size("777-p1.p1")
    m1_sz = area_size("777-m1.m1")
    s1_sz = area_size("777-s1.s1")
    v1_sz = area_size("777-v1.v1")
    c1_sz = area_size("777-c1.c1")
    c2_sz = area_size("777-c2.c2")

    # maincpu maps a 1 MB window on Neo Geo even when P1 is 512 KB
    maincpu_area = max(p1_sz, 0x100000)

    xml = f'''\
<?xml version="1.0"?>
<!DOCTYPE softwarelist PUBLIC "-//MAME//DTD Software List//EN" "https://raw.githubusercontent.com/mamedev/mame/master/src/mame/mame.dtd">
<softwarelist name="neogeo" description="Eagle Software NeoGeo SDK">
    <software name="neogeosdk">
        <description>NeoGeo SDK Demo</description>
        <year>2026</year>
        <publisher>Eagle Software</publisher>
        <part name="cart" interface="neo_cart">
            <dataarea name="maincpu" size="{maincpu_area:#010x}">
{rom_tag("777-p1.p1", loadflag="load16_word_swap")}
            </dataarea>
            <dataarea name="audiocpu" size="{m1_sz:#010x}">
{rom_tag("777-m1.m1")}
            </dataarea>
            <dataarea name="fixed" size="{s1_sz:#010x}">
{rom_tag("777-s1.s1")}
            </dataarea>
            <dataarea name="ymsnd:adpcma" size="{v1_sz:#010x}">
{rom_tag("777-v1.v1")}
            </dataarea>
            <dataarea name="sprites" size="{c1_sz + c2_sz:#010x}">
{rom_tag("777-c1.c1", offset=0, loadflag="load16_byte")}
{rom_tag("777-c2.c2", offset=1, loadflag="load16_byte")}
            </dataarea>
        </part>
    </software>
</softwarelist>
'''
    with open(OUT_XML, "w", newline="\n") as f:
        f.write(xml)
    print(f"Generated {OUT_XML}")

def build_dist():
    dist_dir  = os.path.join(REPO_ROOT, "dist")
    roms_dir  = os.path.join(dist_dir, "roms")
    hash_dir  = os.path.join(dist_dir, "hash_eagle")
    os.makedirs(roms_dir, exist_ok=True)
    os.makedirs(hash_dir, exist_ok=True)

    # ROM ZIP — files at archive root, no subfolder
    zip_path = os.path.join(roms_dir, "neogeosdk.zip")
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_STORED) as zf:
        for rom in ROM_NAMES:
            src = os.path.join(ROM_DIR, rom)
            if os.path.exists(src):
                zf.write(src, rom)
            else:
                print(f"WARNING: {rom} missing — skipped in ZIP", file=sys.stderr)
    print(f"Created {zip_path}")

    # Copy XML
    shutil.copy2(OUT_XML, os.path.join(hash_dir, "neogeo.xml"))

    # Dist launchers — no gen_hash.py call, standalone for end users
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
