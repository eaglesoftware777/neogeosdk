#!/bin/bash
#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
# NeoGeo SDK - Debug Launcher (Linux)
# Place neogeo.zip (BIOS) inside roms/ before running.
# Opens MAME with the built-in debugger and verbose console output.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

python3 "$SCRIPT_DIR/hash_eagle/gen_hash.py" || exit 1

exec mame neogeo \
    -cart1 neogeosdk \
    -rompath "$SCRIPT_DIR/roms" \
    -hashpath "$SCRIPT_DIR/hash_eagle;$SCRIPT_DIR/hash" \
    -bios unibios22 \
    -window \
    -console \
    -verbose \
    -debug
