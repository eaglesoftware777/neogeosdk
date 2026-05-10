#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import glob
import io
import json
import os
import struct as st

import numpy as np

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


def write_palette(palette, std_file, neogeo_file, image_index, packed_palettes, palette_has_zero):
    palette_words = [0x0] * 16
    palette_words[0] = 0x0

    palette_start = 1 if palette_has_zero else 0
    visible_colors = min(len(palette) - palette_start, 15)
    for slot in range(visible_colors):
        rgb = palette[palette_start + slot]
        red_24 = int(rgb[0])
        green_24 = int(rgb[1])
        blue_24 = int(rgb[2])
        color_24 = np.uint32((red_24 << 16) | (green_24 << 8) | blue_24)

        red_16 = (color_24 & 0xF80000) >> 19
        green_16 = (color_24 & 0x00F800) >> 11
        blue_16 = (color_24 & 0x0000F8) >> 3

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
            (0 << 15)
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
        palette_words[slot + 1] = int(ng_word)

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


def encode_block(block, c1_file, c2_file, transparent_zero):
    for row in range(8):
        pixels = block[row, :]
        if transparent_zero:
            colors = [int(pixel) for pixel in pixels]
        else:
            colors = [int(pixel) + 1 for pixel in pixels]

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
    cur.execute("select idx,data,palette from image")
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
    asset_info = manifest.get(int(image_index), {})
    transparent_zero = bool(asset_info.get("transparent_zero", 0))
    palette_has_zero = bool(asset_info.get("palette_has_zero", 0))
    height, width = indexed.shape[:2]
    sprite_count = width // 16
    character_count = height // 16
    sprites = np.uint8(np.vsplit(indexed, sprite_count))

    for sprite in sprites:
        characters = np.hsplit(sprite, character_count)
        for character in characters:
            block3 = character[0:8, 0:8]
            block4 = character[8:16, 0:8]
            block1 = character[0:8, 8:16]
            block2 = character[8:16, 8:16]
            encode_block(block1, f_c1rom, f_c2rom, transparent_zero)
            encode_block(block2, f_c1rom, f_c2rom, transparent_zero)
            encode_block(block3, f_c1rom, f_c2rom, transparent_zero)
            encode_block(block4, f_c1rom, f_c2rom, transparent_zero)

    write_palette(palette, f_std, f_neo, image_index + 1, packed_palettes, palette_has_zero)

os.fsync(f_c1rom)
os.fsync(f_c2rom)
f_c1rom.close()
f_c2rom.close()
f_std.close()
f_neo.close()

with open("neopal.bin", "wb") as pal_file:
    pal_file.write(packed_palettes)
