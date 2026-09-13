#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
GAME="${1:-}"
if [ -z "$GAME" ]; then
    echo "Usage: $0 <game>"
    echo "Example: $0 demo"
    exit 1
fi
if [ ! -f "$SCRIPT_DIR/hash_eagle/$GAME/neogeo.xml" ]; then
    echo "ERROR: missing $SCRIPT_DIR/hash_eagle/$GAME/neogeo.xml"
    exit 1
fi
exec mame neogeo \
    -cart1 "$GAME" \
    -rompath "$SCRIPT_DIR/roms" \
    -hashpath "$SCRIPT_DIR/hash_eagle/$GAME;$SCRIPT_DIR/hash_eagle;$SCRIPT_DIR/hash" \
    -bios unibios22 \
    -window \
    -console \
    -verbose
