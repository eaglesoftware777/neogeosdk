#!/usr/bin/env bash
set -e
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE"

echo "============================================================"
echo "  Eagle Software - Flashcart & SD Card Image Downloader"
echo "  NeoSD, Darksoft Multi, and BackBit Platinum Images"
echo "============================================================"
echo ""

PY_BIN=""
for cmd in python3 python; do
    if command -v "$cmd" &>/dev/null; then
        PY_BIN="$cmd"
        break
    fi
done

if [ -n "$PY_BIN" ] && [ -f "$HERE/download_sd.py" ]; then
    exec "$PY_BIN" "$HERE/download_sd.py" "$@"
fi

echo "[INFO] Python 3 not found, using curl / wget..."
mkdir -p "$HERE/sd_images/neosd" "$HERE/sd_images/darksoft" "$HERE/sd_images/backbit"
BASE="https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.2"

DL_CMD="curl -L -o"
command -v curl &>/dev/null || DL_CMD="wget -O"

$DL_CMD "$HERE/sd_images/neosd/Maiya-NeoSD-MVS.neo" "$BASE/Maiya-WIP-NeoSD_MVS_v3.neo"
$DL_CMD "$HERE/sd_images/neosd/Maiya-NeoSD-AES.neo" "$BASE/Maiya-WIP-NeoSD_AES_v3.neo"
$DL_CMD "$HERE/sd_images/darksoft/Maiya-Darksoft-MVS_v3.zip" "$BASE/Maiya-Darksoft-MVS_v3.zip"
$DL_CMD "$HERE/sd_images/darksoft/Maiya-Darksoft-AES_v3.zip" "$BASE/Maiya-Darksoft-AES_v3.zip"
$DL_CMD "$HERE/sd_images/backbit/Maiya-BackBit-MVS_v3.zip" "$BASE/Maiya-BackBit-MVS_v3.zip"
$DL_CMD "$HERE/sd_images/backbit/Maiya-BackBit-AES_v3.zip" "$BASE/Maiya-BackBit-AES_v3.zip"

echo "Flashcart images downloaded to $HERE/sd_images/"
