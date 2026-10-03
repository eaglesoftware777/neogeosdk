#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "=== NeoGeo SDK - Experimental EagleBIOS Test Launcher (demo) ==="

# 1. Ensure EagleBIOS firmware suite is built
if [ ! -f "$SCRIPT_DIR/bios/sp-s2.sp1" ] || [ ! -f "$SCRIPT_DIR/bios/sm1.sm1" ] || \
   [ ! -f "$SCRIPT_DIR/bios/sfix.sfix" ] || [ ! -f "$SCRIPT_DIR/bios/000-lo.lo" ]; then
    echo "Building EagleBIOS firmware suite..."
    make -C "$SCRIPT_DIR/bios"
fi

# 2. Setup clean test_roms directory with EagleBIOS ROMs for MVS and AES
mkdir -p "$SCRIPT_DIR/bios/test_roms/neogeo" "$SCRIPT_DIR/bios/test_roms/aes"
cp -f "$SCRIPT_DIR/bios/sp-s2.sp1" "$SCRIPT_DIR/bios/test_roms/neogeo/"
cp -f "$SCRIPT_DIR/bios/neo-epo.bin" "$SCRIPT_DIR/bios/test_roms/aes/"
cp -f "$SCRIPT_DIR/bios/sm1.sm1"   "$SCRIPT_DIR/bios/test_roms/neogeo/" "$SCRIPT_DIR/bios/test_roms/aes/"
cp -f "$SCRIPT_DIR/bios/sfix.sfix" "$SCRIPT_DIR/bios/test_roms/neogeo/" "$SCRIPT_DIR/bios/test_roms/aes/"
cp -f "$SCRIPT_DIR/bios/000-lo.lo" "$SCRIPT_DIR/bios/test_roms/neogeo/" "$SCRIPT_DIR/bios/test_roms/aes/"

# 3. Ensure demo cartridge ROMs are built
if [ ! -f "$SCRIPT_DIR/roms/demo/777-p1.p1" ]; then
    echo "Building demo ROMs..."
    make GAME=demo p1
fi

# 4. Generate hash files if needed
python3 "$SCRIPT_DIR/hash_eagle/gen_hash.py" >/dev/null 2>&1 || true

# 5. Launch MAME with EagleBIOS
echo "Launching MAME with experimental EagleBIOS on demo..."
exec mame neogeo \
    -cart1 demo \
    -rompath "$SCRIPT_DIR/bios/test_roms;$SCRIPT_DIR/roms" \
    -hashpath "$SCRIPT_DIR/hash_eagle/demo;$SCRIPT_DIR/hash_eagle;$SCRIPT_DIR/hash" \
    -bios euro \
    -window \
    -waitvsync \
    "$@"
