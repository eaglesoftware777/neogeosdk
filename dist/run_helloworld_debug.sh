#!/bin/bash
# NeoGeo SDK - Debug Release Launcher (helloworld)
# Place neogeo.zip (BIOS) inside roms/ before running.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec mame neogeo \
    -cart1 helloworld \
    -rompath "$SCRIPT_DIR/roms" \
    -hashpath "$SCRIPT_DIR/hash_eagle/helloworld;$SCRIPT_DIR/hash_eagle;$SCRIPT_DIR/hash" \
    -bios unibios22 \
    -window \
    -console \
    -verbose \
    -debug
