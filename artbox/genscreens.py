#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import json
import os
import struct
import sys
from pathlib import Path

import numpy as np

SPRITE_PALETTE_BASE = 0x10
ROOT = Path(os.environ.get("ARTBOX_DATA_DIR", str(Path(__file__).resolve().parent))).resolve()
MANIFEST_PATH = ROOT / "assets_manifest.json"
NEOPAL_PATH = ROOT / "neopal.bin"
SCREENS_C_PATH = ROOT / "screens.c"


def getpal(image_index):
    start = bsz * image_index + 8
    end = start + bsz - 8
    return struct.unpack("16Q", buffi[start:end])


def load_manifest():
    if not MANIFEST_PATH.exists():
        return None
    with MANIFEST_PATH.open("r", encoding="utf-8") as handle:
        return json.load(handle)


manifest = load_manifest()
if manifest is not None:
    image_specs = manifest
else:
    imgnb = int(sys.argv[1]) if len(sys.argv) > 1 else 0
    image_specs = [
        {
            "db_index": i,
            "screen_id": i + 1,
            "palette_bank": SPRITE_PALETTE_BASE + i,
            "full_codegen": 1 if i < 10 else 0,
        }
        for i in range(imgnb)
    ]

bsz = 136
with NEOPAL_PATH.open("rb") as palette_file:
    buffi = palette_file.read()

sz = int(sys.argv[2]) if len(sys.argv) > 2 else 16
sprt_sz = int(sys.argv[3]) if len(sys.argv) > 3 else 16
crt_sz = int(sys.argv[4]) if len(sys.argv) > 4 else 16

image_count = len(image_specs)
map_start = 0
maps = []
for _count in range(image_count):
    tile_map = np.uint16(np.zeros((sz, sz)))
    for row in range(sz):
        tile_map[row, :] = np.arange(map_start, map_start + sz, 1, dtype=np.uint16)
        map_start = int(tile_map[row, sz - 1]) + 1
    maps.append(tile_map)

sys.stdout = SCREENS_C_PATH.open("wt", encoding="utf-8")

for spec in image_specs:
    image_i0 = int(spec["db_index"])
    image_index = int(spec["screen_id"])
    palette_bank = int(spec.get("palette_bank", SPRITE_PALETTE_BASE + image_i0))
    full_codegen = int(spec.get("full_codegen", 1 if image_i0 < 10 else 0))
    pal = getpal(image_index)

    print("")
    print("")
    print(
        "void NEOGEO_USER showScreen%d(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {"
        % image_index
    )
    print("/****************************************** screen %d ******************************************/" % image_index)

    if not full_codegen:
        print("uint16_t  pal%d[16];" % image_index)
        print(
            "setpal(pal%d,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x);"
            % (
                image_index,
                pal[0], pal[1], pal[2], pal[3],
                pal[4], pal[5], pal[6], pal[7],
                pal[8], pal[9], pal[10], pal[11],
                pal[12], pal[13], pal[14], pal[15],
            )
        )
        print("load_palettes(pal%d,PALETTES+PALOFFSET*%d);" % (image_index, palette_bank))
        print("}")
        continue

    print("uint16_t  SCB2    = 0x0;")
    print("uint16_t  SCB3    = 0x0;")
    print("uint16_t  SCB4    = 0x0;")
    print("uint16_t  pal%d[16];" % image_index)
    print(
        "setpal(pal%d,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x);"
        % (
            image_index,
            pal[0], pal[1], pal[2], pal[3],
            pal[4], pal[5], pal[6], pal[7],
            pal[8], pal[9], pal[10], pal[11],
            pal[12], pal[13], pal[14], pal[15],
        )
    )
    for sprt_index in range(sprt_sz):
        values = ["0x%x" % maps[image_i0][crt_index, sprt_index] for crt_index in range(crt_sz)]
        print("uint16_t spriteMapS%d_%d[%d] = {%s};" % (
            image_index,
            sprt_index + 1,
            crt_sz,
            ",".join(values),
        ))
    print("load_palettes(pal%d,PALETTES+PALOFFSET*%d);" % (image_index, palette_bank))
    print("uint16_t SCB1_2common = setSCB1_2(%d,0,0,0,0,0);" % palette_bank)
    for sprt_index in range(sprt_sz):
        print(
            "uint16_t spal%d_%d[%d]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};"
            % (image_index, sprt_index + 1, crt_sz)
        )

    x = "x0"
    y = "496-y0"
    # The backdrop is one register holding one colour for the whole
    # screen, and `backdrop` does not change across the strips - so
    # emitting the write inside the loop repeated it once per strip
    # (16+ times) interleaved with the vram_sprite() bursts below.
    # That whole sequence is far too long to fit in vblank, so those
    # repeats land partway down a visible frame: the raster draws the
    # rows above the write in the old colour and the rows below in the
    # new one, which is the black bar that flashes across the top of
    # the screen for a single frame on the way into a scene.  Write it
    # once, before any of the VRAM traffic starts.
    print("setBACKDROP(backdrop);")
    for sprt_index in range(sprt_sz):
        print("SCB2    = setSCB2(xr,yr);")
        if sprt_index == 0:
            print("SCB3    = setSCB3(%s,0,min_crt_sz);" % y)
        else:
            print("SCB3    = setSCB3(%s,1,min_crt_sz);" % y)
        print("SCB4    = setSCB4(%s);" % x)
        print(
            "vram_sprite(sprite_base + 64*%d,1,(sprite_base>>6)+%d,spriteMapS%d_%d,spal%d_%d,%d,SCB2,SCB3,SCB4);"
            % (sprt_index, sprt_index, image_index, sprt_index + 1, image_index, sprt_index + 1, crt_sz)
        )
        x = "x0+16*%d" % (sprt_index + 1)
    print("}")

print("")
print("static const NGPaletteAsset ng_screen_palette_assets[] = {")
for spec in image_specs:
    image_i0 = int(spec["db_index"])
    image_index = int(spec["screen_id"])
    palette_bank = int(spec.get("palette_bank", SPRITE_PALETTE_BASE + image_i0))
    pal = getpal(image_index)
    values = ",".join("0x%x" % color for color in pal)
    print("    {%d,%d,{%s}}," % (image_index, palette_bank, values))
print("};")
print("const uint16_t ng_screen_palette_count = %d;" % image_count)
print("")
print("uint8_t NEOGEO_USER ng_load_screen_palette(uint16_t screen_id) {")
print("    return ng_palette_load_asset(ng_screen_palette_assets, ng_screen_palette_count, screen_id);")
print("}")

def art_type_for(spec):
    mode = str(spec.get("mode", "screen"))
    category = str(spec.get("category", ""))
    if category == "backgrounds" or category == "background":
        return "NG_ART_TYPE_BACKGROUND"
    if mode == "sprite":
        return "NG_ART_TYPE_SPRITE"
    return "NG_ART_TYPE_SCREEN"

print("")
print("const NGArtAsset ng_screen_art_assets[] = {")
for spec in image_specs:
    image_index = int(spec["screen_id"])
    tile_base = int(spec.get("tile_base", int(spec["db_index"]) * 256))
    tile_col = int(spec.get("used_tile_col_start", 0))
    tile_row = int(spec.get("used_tile_row_start", 0))
    tile_stride = int(spec.get("target_width", 256)) // 16
    if tile_stride < 1:
        tile_stride = 16
    active_tile = tile_base + tile_row * tile_stride + tile_col
    print(
        "    {%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d},"
        % (
            image_index,
            art_type_for(spec),
            int(spec.get("palette_bank", SPRITE_PALETTE_BASE + int(spec["db_index"]))),
            active_tile,
            int(spec.get("tile_reserved_last", active_tile)),
            int(spec.get("sprite_strips", 1)),
            int(spec.get("used_tile_rows", spec.get("sprite_active_rows", 1))),
            int(spec.get("sprite_active_rows", 1)),
            tile_stride,
            int(spec.get("used_tile_col_start", 0)) * 16,
            int(spec.get("used_tile_row_start", 0)) * 16,
            int(spec.get("content_width", 0)),
            int(spec.get("content_height", 0)),
        )
    )
print("};")
print("const uint16_t ng_screen_art_asset_count = %d;" % image_count)
print("")
print("const NGArtAsset * NEOGEO_USER ng_screen_art_asset(uint16_t screen_id) {")
print("    return ng_art_asset_find(ng_screen_art_assets, ng_screen_art_asset_count, screen_id);")
print("}")

# --- Screen dispatch table (consumed by ng_bg.c / ng_level.c) ---
# Emitted once after all showScreenN functions.
# ng_screen_table[i] == showScreen_i (1-based).  Index 0 is NULL (unused).
max_screen_id = max((int(spec["screen_id"]) for spec in image_specs), default=0)
print("")
print("const NGShowScreenFn ng_screen_table[NG_SCREEN_TABLE_MAX] = {")
print("    0, /* index 0 unused */")
for sid in range(1, max_screen_id + 1):
    exists = any(int(spec["screen_id"]) == sid for spec in image_specs)
    if exists:
        print("    showScreen%d," % sid)
    else:
        print("    0, /* %d not present */" % sid)
# Pad remaining slots to NG_SCREEN_TABLE_MAX with 0
print("};")
print("const uint16_t ng_screen_count = %d;" % image_count)
