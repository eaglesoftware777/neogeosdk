#!/bin/bash
# NeoGeo SDK - Release Launcher (maiya)
# Place neogeo.zip (BIOS) inside roms/ before running.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec mame neogeo \
    -cart1 maiya \
    -rompath "$SCRIPT_DIR/roms" \
    -hashpath "$SCRIPT_DIR/hash_eagle/maiya;$SCRIPT_DIR/hash_eagle;$SCRIPT_DIR/hash" \
    -bios unibios22 \
    -window \
    -console \
    -verbose
