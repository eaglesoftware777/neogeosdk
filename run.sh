#!/bin/bash
#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
# NeoGeo SDK - MAME launcher (Linux)
# Usage:  ./run.sh [--debug]

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

python3 "$SCRIPT_DIR/hash_eagle/gen_hash.py" || exit 1

MAME_ARGS=(
    neogeo
    -cart1 neogeosdk
    -rompath "$SCRIPT_DIR/roms"
    -hashpath "$SCRIPT_DIR/hash_eagle:$SCRIPT_DIR/hash"
    -bios unibios22
    -window
    -waitvsync
)

if [ "${1:-}" = "--debug" ]; then
    exec mame "${MAME_ARGS[@]}" -debug -verbose
else
    exec mame "${MAME_ARGS[@]}"
fi
