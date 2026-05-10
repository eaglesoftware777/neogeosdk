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

import numpy as np

SPRITE_PALETTE_BASE = 0x10
MANIFEST_PATH = "assets_manifest.json"


def getpal(image_index):
    start = bsz * image_index + 8
    end = start + bsz - 8
    return struct.unpack("16Q", buffi[start:end])


def load_manifest():
    if not os.path.exists(MANIFEST_PATH):
        return None
    with open(MANIFEST_PATH, "r", encoding="utf-8") as handle:
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
with open("neopal.bin", "rb") as palette_file:
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

sys.stdout = open("screens.c", "wt")

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
    for sprt_index in range(sprt_sz):
        print("SCB2    = setSCB2(xr,yr);")
        if sprt_index == 0:
            print("SCB3    = setSCB3(%s,0,min_crt_sz);" % y)
        else:
            print("SCB3    = setSCB3(%s,1,min_crt_sz);" % y)
        print("SCB4    = setSCB4(%s);" % x)
        print("setBACKDROP(backdrop);")
        print(
            "vram_sprite(sprite_base + 64*%d,1,%d,spriteMapS%d_%d,spal%d_%d,%d,SCB2,SCB3,SCB4);"
            % (sprt_index, sprt_index, image_index, sprt_index + 1, image_index, sprt_index + 1, crt_sz)
        )
        x = "x0+16*%d" % (sprt_index + 1)
    print("}")
