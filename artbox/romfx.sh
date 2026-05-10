#!/bin/bash
#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
# Copy the generated S1 ROM to the roms directory.
# fixtiles.py already outputs the final hardware-format 777-s1.s1 directly.
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$REPO_ROOT/roms/neogeosdk"
cp 777-s1.s1 "$REPO_ROOT/roms/neogeosdk/777-s1.s1"
echo "Copied 777-s1.s1 → $REPO_ROOT/roms/neogeosdk/"
