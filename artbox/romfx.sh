#!/bin/bash
#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
# Copy the generated S1 ROM to the roms directory.
# fixtiles.py already outputs the final hardware-format ${GAME_ID}-s1.s1 directly.
: "${GAME_ID:=777}"
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$REPO_ROOT/roms/neogeosdk"
cp ${GAME_ID}-s1.s1 "$REPO_ROOT/roms/neogeosdk/${GAME_ID}-s1.s1"
echo "Copied ${GAME_ID}-s1.s1 → $REPO_ROOT/roms/neogeosdk/"
