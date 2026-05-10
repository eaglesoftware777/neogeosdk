#!/usr/bin/env python3
#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
"""
Regenerates hash_eagle/neogeo.xml from the current ROM files in roms/neogeosdk/.
Must be run before launching MAME whenever any ROM is rebuilt.
"""
import binascii, hashlib, os, sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT   = os.path.dirname(SCRIPT_DIR)
ROM_DIR     = os.path.join(REPO_ROOT, "roms", "neogeosdk")
OUT_XML     = os.path.join(SCRIPT_DIR, "neogeo.xml")

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

def main():
    p1_sz = area_size("777-p1.p1")
    m1_sz = area_size("777-m1.m1")
    s1_sz = area_size("777-s1.s1")
    v1_sz = area_size("777-v1.v1")
    c1_sz = area_size("777-c1.c1")
    c2_sz = area_size("777-c2.c2")

    xml = f'''\
<?xml version="1.0"?>
<!DOCTYPE softwarelist PUBLIC "-//MAME//DTD Software List//EN" "https://raw.githubusercontent.com/mamedev/mame/master/src/mame/mame.dtd">
<softwarelist name="neogeo" description="Eagle Software NeoGeo SDK">
    <software name="neogeosdk">
        <description>NeoGeo SDK Demo</description>
        <year>2026</year>
        <publisher>Eagle Software</publisher>
        <part name="cart" interface="neo_cart">
            <dataarea name="maincpu" size="{p1_sz:#010x}">
{rom_tag("777-p1.p1")}
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

if __name__ == "__main__":
    main()
