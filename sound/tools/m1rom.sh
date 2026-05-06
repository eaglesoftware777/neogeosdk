#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SDK_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
OUT_DIR="$SDK_ROOT/out"
ROM_DIR="$SDK_ROOT/roms/ssideki"
OBJ="$OUT_DIR/driver.o"
OBJ_C="$OUT_DIR/driver_c.o"
ASM_C="$OUT_DIR/driver.gen.asm"
ASM_COMBINED="$OUT_DIR/driver_combined.asm"
LINKFILE="$OUT_DIR/m1.link"
M1ROM="$OUT_DIR/052-m1.m1"

mkdir -p "$OUT_DIR"

: "${WLAZ80:=wla-z80}"
: "${WLALINK:=wlalink}"
: "${USE_Z80C:=0}"
: "${LINK_C_DRIVER:=0}"
: "${Z80CC:=$SDK_ROOT/z80c-special/build/z80cc}"
: "${Z80C_SRC:=$SDK_ROOT/sound/driver/driver.c}"

if [ "$USE_Z80C" = "1" ] && [ ! -f "$Z80CC" ]; then
    echo "Building z80cc..."
    make -C "$SDK_ROOT/z80c-special"
fi

python3 "$SDK_ROOT/sound/tools/fm_patch_compile.py" "$SDK_ROOT/sound/fm/patches.fm" -o "$SDK_ROOT/sound/driver/fm_patch_table.inc"
python3 "$SDK_ROOT/sound/tools/fm_compile.py" "$SDK_ROOT"/sound/fm/*.mml -o "$SDK_ROOT/sound/driver/fm_data.inc"
python3 "$SDK_ROOT/sound/tools/mml_compile.py" "$SDK_ROOT"/sound/mml/*.mml -o "$SDK_ROOT/sound/driver/music_data.inc"
python3 "$SDK_ROOT/sound/tools/ssg_config_compile.py" "$SDK_ROOT/sound/ssg/config.ssg" -o "$SDK_ROOT/sound/driver/ssg_config.inc"
python3 "$SDK_ROOT/sound/tools/ssg_compile.py" "$SDK_ROOT"/sound/ssg/*.mml -o "$SDK_ROOT/sound/driver/ssg_data.inc"

if [ "$USE_Z80C" = "1" ]; then
    echo "Compiling C driver with $Z80CC from $Z80C_SRC"
    "$Z80CC" --target neogeo -DDRIVER_SPLIT_PRELUDE=1 -I"$SDK_ROOT/sound/driver" -S -o "$ASM_C" "$Z80C_SRC"
fi

if [ "$USE_Z80C" = "1" ] && [ "$LINK_C_DRIVER" = "1" ]; then
    echo "Linking experimental C driver core"
    python3 "$SDK_ROOT/sound/tools/combine_split_driver.py" \
        "$SDK_ROOT/sound/driver/driver_prelude.asm" \
        "$ASM_C" \
        "$ASM_COMBINED"
    "$WLAZ80" -I "$SDK_ROOT/sound/driver" -o "$OBJ_C" "$ASM_COMBINED"
    printf "[objects]\n%s\n" "$OBJ_C" > "$LINKFILE"
else
    if [ "$USE_Z80C" = "1" ]; then
        echo "Linking authoritative ASM driver for final M1"
    else
        echo "Assembling ASM driver with $WLAZ80"
    fi
    "$WLAZ80" -I "$SDK_ROOT/sound/driver" -o "$OBJ" "$SDK_ROOT/sound/m1/m1.asm"
    printf "[objects]\n%s\n" "$OBJ" > "$LINKFILE"
fi
"$WLALINK" -r "$LINKFILE" "$M1ROM"

current_size=$(wc -c < "$M1ROM" | tr -d ' ')
target_size=131072
if [ "$current_size" -lt "$target_size" ]; then
  truncate -s "$target_size" "$M1ROM"
fi

mkdir -p "$ROM_DIR"
cp "$M1ROM" "$ROM_DIR/052-m1.m1"
cp "$M1ROM" "$ROM_DIR/sm1.sm1"
rm -f "$OBJ" "$OBJ_C" "$ASM_C" "$ASM_COMBINED" "$LINKFILE"

echo "Built $M1ROM"
