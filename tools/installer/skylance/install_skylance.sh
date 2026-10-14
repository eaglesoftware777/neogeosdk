#!/usr/bin/env bash
# Sky Lance v1 setup for Linux and macOS: the MVS and AES MAME sets of the
# NeoGeoSDK v1.7.3 pre-release side by side, EagleBIOS and run_skylance.sh,
# in the same layout as the Windows setup (install_skylance.ps1).
#
#   ./install_skylance.sh [folder]     default: ~/mame/neogeosdk
#
# The release zips are taken from beside this script, or downloaded, and
# checked against SkyLance-MANIFEST_v1.txt first. Replaced files go to
# backups/skylance-setup-<time>; settings and saves stay.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TAG=v1.7.3
VERSION=v1
RELEASE="https://github.com/eaglesoftware777/neogeosdk/releases/download/$TAG"
TARGET="${1:-$HOME/mame/neogeosdk}"
mkdir -p "$TARGET"
TARGET="$(cd "$TARGET" && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

for tool in unzip; do
    command -v "$tool" >/dev/null || { echo "Please install $tool first."; exit 1; }
done
sha256() { if command -v sha256sum >/dev/null; then sha256sum "$1"; else shasum -a 256 "$1"; fi | cut -d' ' -f1; }

echo "Sky Lance $VERSION setup (NeoGeoSDK $TAG pre-release) -> $TARGET"
for platform in mvs aes; do
    if [ "$platform" = mvs ]; then name="SkyLance-WIP-EagleBIOS_$VERSION.zip"; else name="SkyLance-WIP-AES-EagleBIOS_$VERSION.zip"; fi
    zip="$HERE/$name"
    if [ ! -f "$zip" ]; then
        zip="$WORK/$name"
        echo "Downloading $name ..."
        if command -v curl >/dev/null; then curl -fL -o "$zip" "$RELEASE/$name"; else wget -O "$zip" "$RELEASE/$name"; fi
    fi
    want="$(awk -v n="$name" '$4 == n { print $3 }' "$HERE/SkyLance-MANIFEST_$VERSION.txt")"
    [ "$(sha256 "$zip")" = "$want" ] || { echo "$name does not match SkyLance-MANIFEST_$VERSION.txt"; exit 1; }
    echo "  $name verified"
    unzip -q -o "$zip" -d "$WORK/$platform"
done

backup="$TARGET/backups/skylance-setup-$(date +%Y%m%d-%H%M%S)"
for item in tests/skylance/mvs/roms tests/skylance/mvs/hash tests/skylance/aes/roms tests/skylance/aes/hash \
            tests/skylance/bios run_skylance.sh SKYLANCE-README.txt; do
    if [ -e "$TARGET/$item" ]; then
        mkdir -p "$(dirname "$backup/$item")"
        mv "$TARGET/$item" "$backup/$item"
    fi
done
for platform in mvs aes; do
    mkdir -p "$TARGET/tests/skylance/$platform/roms/skylance" "$TARGET/tests/skylance/$platform/hash"
    cp "$WORK/$platform/roms/skylance/"* "$TARGET/tests/skylance/$platform/roms/skylance/"
    cp "$WORK/$platform/hash/neogeo.xml" "$TARGET/tests/skylance/$platform/hash/"
done
for board in neogeo aes; do
    mkdir -p "$TARGET/tests/skylance/bios/$board"
    cp "$WORK/mvs/roms/$board/"* "$TARGET/tests/skylance/bios/$board/"
done
cp "$HERE/run_skylance.sh" "$HERE/SKYLANCE-README.txt" "$TARGET/"
chmod +x "$TARGET/run_skylance.sh"
echo
echo "Sky Lance $VERSION is installed in $TARGET"
echo "  ./run_skylance.sh mvs    arcade (MVS), EagleBIOS"
echo "  ./run_skylance.sh aes    console (AES), EagleBIOS"
[ -d "$backup" ] && echo "Earlier files: $backup"
exit 0
