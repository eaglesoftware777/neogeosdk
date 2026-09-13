#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import glob
import io
import json
import os
import struct as st

import numpy as np
from tile_codec import encode_image, write_utility_tiles

try:
    import pysqlite3 as db
    from pysqlite3 import Error
except ImportError:
    import sqlite3 as db
    from sqlite3 import Error


PALETTE_RECORD_SIZE = 136
MANIFEST_PATH = "assets_manifest.json"


def adapt_array(arr):
    out = io.BytesIO()
    np.save(out, arr)
    out.seek(0)
    return db.Binary(out.read())


def convert_array(text):
    out = io.BytesIO(text)
    out.seek(0)
    return np.load(out)


def write_palette(palette, std_file, neogeo_file, image_index, packed_palettes):
    palette_words = [0x0] * 16
    palette_words[0] = 0x0

    visible_colors = min(len(palette), 16)
    for slot in range(1, visible_colors):
        rgb = palette[slot]
        red_24 = int(rgb[0])
        green_24 = int(rgb[1])
        blue_24 = int(rgb[2])
        color_24 = np.uint32((red_24 << 16) | (green_24 << 8) | blue_24)

        # Six bits per channel, not five: the word holds five, and the top
        # bit supplies a sixth, least significant one that all three
        # channels share, inverted -
        #     channel6 = (channel5 << 1) | (1 - dark)
        # so the darkest colour a palette can express is 0 rather than the
        # 4/255 that leaving the bit clear pins it to, and the reachable
        # colours are two interleaved lattices instead of one.
        #
        # The quantiser only emits colours that are already on that lattice,
        # where the three channels agree on parity.  Anything else - a
        # hand-authored palette, or one from an older tool - is placed on
        # whichever parity reproduces it more closely instead of being
        # truncated onto the odd one.
        chan6 = [(c * 63 + 127) // 255 for c in (red_24, green_24, blue_24)]
        best = None
        for parity in (0, 1):
            cand = [min(62 + parity, max(parity,
                                         int(round((c - parity) / 2.0)) * 2 + parity))
                    for c in chan6]
            err = sum(w * ((v << 2 | v >> 4) - c8) ** 2
                      for w, v, c8 in zip((0.299, 0.587, 0.114), cand,
                                          (red_24, green_24, blue_24)))
            if best is None or err < best[0]:
                best = (err, cand)
        red_6, green_6, blue_6 = best[1]

        dark = 1 - (red_6 & 1)
        red_16 = red_6 >> 1
        green_16 = green_6 >> 1
        blue_16 = blue_6 >> 1

        std_word = np.uint16((red_16 << 10) | (green_16 << 5) | blue_16)
        std_word.tofile(std_file)

        red_lsb = (red_16 >> 0) & 1
        red_3 = (red_16 >> 1) & 1
        red_2 = (red_16 >> 2) & 1
        red_1 = (red_16 >> 3) & 1
        red_0 = (red_16 >> 4) & 1

        green_lsb = (green_16 >> 0) & 1
        green_3 = (green_16 >> 1) & 1
        green_2 = (green_16 >> 2) & 1
        green_1 = (green_16 >> 3) & 1
        green_0 = (green_16 >> 4) & 1

        blue_lsb = (blue_16 >> 0) & 1
        blue_3 = (blue_16 >> 1) & 1
        blue_2 = (blue_16 >> 2) & 1
        blue_1 = (blue_16 >> 3) & 1
        blue_0 = (blue_16 >> 4) & 1

        ng_word = np.uint16(
            (dark << 15)
            | (red_lsb << 14)
            | (green_lsb << 13)
            | (blue_lsb << 12)
            | (red_0 << 11)
            | (red_1 << 10)
            | (red_2 << 9)
            | (red_3 << 8)
            | (green_0 << 7)
            | (green_1 << 6)
            | (green_2 << 5)
            | (green_3 << 4)
            | (blue_0 << 3)
            | (blue_1 << 2)
            | (blue_2 << 1)
            | blue_3
        )
        ng_word.tofile(neogeo_file)
        palette_words[slot] = int(ng_word)

    st.pack_into(
        "i16Q",
        packed_palettes,
        PALETTE_RECORD_SIZE * image_index,
        image_index,
        palette_words[0],
        palette_words[1],
        palette_words[2],
        palette_words[3],
        palette_words[4],
        palette_words[5],
        palette_words[6],
        palette_words[7],
        palette_words[8],
        palette_words[9],
        palette_words[10],
        palette_words[11],
        palette_words[12],
        palette_words[13],
        palette_words[14],
        palette_words[15],
    )


def encode_block(block, c1_file, c2_file):
    for row in range(8):
        pixels = block[row, :]
        colors = [int(pixel) & 0x0F for pixel in pixels]

        plane_d = (
            ((colors[7] >> 3) & 1) << 7
            | ((colors[6] >> 3) & 1) << 6
            | ((colors[5] >> 3) & 1) << 5
            | ((colors[4] >> 3) & 1) << 4
            | ((colors[3] >> 3) & 1) << 3
            | ((colors[2] >> 3) & 1) << 2
            | ((colors[1] >> 3) & 1) << 1
            | ((colors[0] >> 3) & 1)
        )
        plane_c = (
            ((colors[7] >> 2) & 1) << 7
            | ((colors[6] >> 2) & 1) << 6
            | ((colors[5] >> 2) & 1) << 5
            | ((colors[4] >> 2) & 1) << 4
            | ((colors[3] >> 2) & 1) << 3
            | ((colors[2] >> 2) & 1) << 2
            | ((colors[1] >> 2) & 1) << 1
            | ((colors[0] >> 2) & 1)
        )
        plane_b = (
            ((colors[7] >> 1) & 1) << 7
            | ((colors[6] >> 1) & 1) << 6
            | ((colors[5] >> 1) & 1) << 5
            | ((colors[4] >> 1) & 1) << 4
            | ((colors[3] >> 1) & 1) << 3
            | ((colors[2] >> 1) & 1) << 2
            | ((colors[1] >> 1) & 1) << 1
            | ((colors[0] >> 1) & 1)
        )
        plane_a = (
            ((colors[7] >> 0) & 1) << 7
            | ((colors[6] >> 0) & 1) << 6
            | ((colors[5] >> 0) & 1) << 5
            | ((colors[4] >> 0) & 1) << 4
            | ((colors[3] >> 0) & 1) << 3
            | ((colors[2] >> 0) & 1) << 2
            | ((colors[1] >> 0) & 1) << 1
            | ((colors[0] >> 0) & 1)
        )

        c1_file.write(st.pack("2B", plane_b, plane_a))
        c2_file.write(st.pack("2B", plane_d, plane_c))


def remove_if_exists(pattern):
    for path in glob.glob(pattern):
        try:
            os.remove(path)
        except OSError:
            pass


remove_if_exists("neopal.bin")
remove_if_exists("*.c1")
remove_if_exists("*.c2")
remove_if_exists("neo.pal")
remove_if_exists("std.pal")
f_c1rom = open("1c.c1", "wb")
f_c2rom = open("2c.c2", "wb")
f_std = open("std.pal", "wb")
f_neo = open("neo.pal", "wb")
data = []
manifest = {}

try:
    db.register_adapter(np.ndarray, adapt_array)
    db.register_converter("array", convert_array)
    conn = db.connect("neorom.db", detect_types=db.PARSE_DECLTYPES)
    conn.execute("PRAGMA journal_mode=WAL")
    cur = conn.cursor()
    cur.execute("select idx,data,palette from image order by idx")
    raw = cur.fetchall()
    for row in raw:
        idx = row[0]
        arr = convert_array(bytes(row[1])) if isinstance(row[1], (bytes, bytearray, memoryview)) else row[1]
        pal = convert_array(bytes(row[2])) if isinstance(row[2], (bytes, bytearray, memoryview)) else row[2]
        data.append((idx, arr, pal))
except Error as exc:
    print(exc)
finally:
    conn.close()

if os.path.exists(MANIFEST_PATH):
    with open(MANIFEST_PATH, "r", encoding="utf-8") as manifest_file:
        manifest = {
            int(entry["db_index"]): entry
            for entry in json.load(manifest_file)
        }

if not data:
    print("Error: len(data) = 0")
    raise SystemExit(1)

image_number = len(data)
packed_palettes = bytearray(PALETTE_RECORD_SIZE * (image_number + 1))
st.pack_into(
    "i16Q",
    packed_palettes,
    0,
    0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
)

for image_index, indexed, palette in data:
    spec = manifest.get(image_index, {})
    first_byte = int(spec.get("tile_base", image_index * 256)) * 64
    if f_c1rom.tell() > first_byte:
        raise ValueError(f"Asset {image_index} overlaps the previous tile reservation")
    gap = bytes(first_byte - f_c1rom.tell())
    f_c1rom.write(gap)
    f_c2rom.write(gap)
    c1, c2 = encode_image(indexed, int(spec.get("tile_reserved_count", 256)))
    f_c1rom.write(c1)
    f_c2rom.write(c2)

    write_palette(palette, f_std, f_neo, image_index + 1, packed_palettes)

write_utility_tiles(f_c1rom, f_c2rom)
os.fsync(f_c1rom)
os.fsync(f_c2rom)
f_c1rom.close()
f_c2rom.close()
f_std.close()
f_neo.close()

with open("neopal.bin", "wb") as pal_file:
    pal_file.write(packed_palettes)
