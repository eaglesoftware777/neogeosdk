#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import io
import os
import sys

import numpy as np
import png

try:
    import pysqlite3 as sqlite3
    from pysqlite3 import Error
except ImportError:
    import sqlite3
    from sqlite3 import Error

try:
    from PIL import Image, ImageFilter, ImageEnhance
    HAS_PIL = True
except ImportError:
    HAS_PIL = False

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
try:
    from img2neo import snap_neogeo, kmeans_palette, floyd_steinberg, open_as_rgb, crop_center
    HAS_IMG2NEO = True
except ImportError:
    HAS_IMG2NEO = False


def adapt_array(arr):
    out = io.BytesIO()
    np.save(out, arr)
    out.seek(0)
    return sqlite3.Binary(out.read())


def convert_array(text):
    out = io.BytesIO(text)
    out.seek(0)
    return np.load(out)


def load_png_as_indexed(path, target_w=256, target_h=256):
    """Load PNG; auto-convert RGBA/RGB to 15-colour indexed if needed."""
    reader = png.Reader(path)
    w, h, rows, metadata = reader.read()

    if metadata.get("palette"):
        palette = np.array(metadata["palette"], dtype=np.uint16)
        indexed = np.vstack(list(map(np.uint16, rows)))
        return indexed, palette

    if not HAS_IMG2NEO or not HAS_PIL:
        raise RuntimeError(
            f"PIL/img2neo not available to auto-convert {os.path.basename(path)}. "
            "Install Pillow: pip install Pillow"
        )

    tw = (target_w // 16) * 16
    th = (target_h // 16) * 16

    img = open_as_rgb(path)
    img = crop_center(img, tw, th)
    img = ImageEnhance.Contrast(img).enhance(1.2)
    img = ImageEnhance.Color(img).enhance(1.3)
    img = img.filter(ImageFilter.UnsharpMask(radius=1.5, percent=150, threshold=2))

    arr = snap_neogeo(np.array(img, dtype=np.uint8))
    pal = kmeans_palette(arr, n=15)
    indexed = floyd_steinberg(arr, pal).astype(np.uint16)
    palette = pal.astype(np.uint16)
    return indexed, palette


def main():
    sqlite3.register_adapter(np.ndarray, adapt_array)
    sqlite3.register_converter("array", convert_array)

    in_dir = "in"
    # Sort all PNGs alphabetically — supports any naming scheme
    png_files = sorted(
        f for f in os.listdir(in_dir) if f.lower().endswith(".png")
    )

    if not png_files:
        print("No PNG files found in", in_dir)
        return

    print(f"Importing {len(png_files)} images (alphabetical order):")

    try:
        conn = sqlite3.connect("neorom.db", detect_types=sqlite3.PARSE_DECLTYPES)
        print(sqlite3.sqlite_version)
        conn.execute("PRAGMA journal_mode=WAL")
        conn.execute("PRAGMA synchronous=NORMAL")
        cur = conn.cursor()
        cur.execute("DELETE FROM image")

        rows = []
        for image_index, fname in enumerate(png_files):
            path = os.path.join(in_dir, fname)
            print(f"  [{image_index:3d}] {fname}")
            indexed, palette = load_png_as_indexed(path)
            rows.append((image_index, indexed, palette))

        cur.executemany("INSERT INTO image (idx,data,palette) VALUES (?,?,?)", rows)
        conn.commit()
        print(f"Imported {len(rows)} images.")
    except Error as exc:
        print(exc)
        raise
    finally:
        conn.close()


if __name__ == "__main__":
    main()
