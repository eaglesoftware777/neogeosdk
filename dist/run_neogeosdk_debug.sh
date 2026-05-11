#!/bin/bash
# NeoGeo SDK - Debug Release Launcher (Linux)
# Place neogeo.zip (BIOS) inside roms/ before running.
# Correct launch: mame neogeo -cart1 neogeosdk  (NOT mame neogeosdk)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec mame neogeo \
    -cart1 neogeosdk \
    -rompath "$SCRIPT_DIR/roms" \
    -hashpath "$SCRIPT_DIR/hash_eagle:$SCRIPT_DIR/hash" \
    -bios unibios22 \
    -window \
    -console \
    -verbose \
    -debug
