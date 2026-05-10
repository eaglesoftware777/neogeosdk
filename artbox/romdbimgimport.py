#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import io
import math
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
    from img2neo import (
        snap_neogeo,
        kmeans_palette,
        floyd_steinberg,
        nearest_palette_indices,
        ordered_dither,
        open_as_rgb,
        crop_center,
    )
    from asset_rules import build_asset_specs, save_manifest, write_out_srt
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


def fit_sprite_rgba(img, target_w, target_h, anchor):
    tw = (target_w // 16) * 16
    th = (target_h // 16) * 16
    img = img.convert("RGBA")

    if img.width > tw or img.height > th:
        img.thumbnail((tw, th), Image.LANCZOS)

    canvas = Image.new("RGBA", (tw, th), (0, 0, 0, 0))
    left = (tw - img.width) // 2
    top = (th - img.height) // 2
    if anchor == "bottom-center":
        top = th - img.height
    if left < 0:
        left = 0
    if top < 0:
        top = 0

    canvas.alpha_composite(img, (left, top))
    return canvas, left, top, img.width, img.height


def make_sprite_palette(opaque_rgb):
    if opaque_rgb.size == 0:
        return np.zeros((15, 3), dtype=np.uint16)

    unique = np.unique(opaque_rgb.reshape(-1, 3), axis=0)
    if len(unique) <= 15:
        palette = np.zeros((15, 3), dtype=np.uint16)
        palette[:len(unique)] = unique.astype(np.uint16)
        return palette

    return kmeans_palette(opaque_rgb.reshape(1, -1, 3), n=15).astype(np.uint16)


def index_sprite_pixels(rgb, alpha_mask, palette15):
    h, w = alpha_mask.shape
    indexed = np.zeros((h, w), dtype=np.uint16)
    if not alpha_mask.any():
        return indexed

    pal = palette15.astype(np.int32)
    opaque = rgb[alpha_mask].astype(np.int32)
    diff = opaque[:, None, :] - pal[None, :, :]
    dist = np.sum(diff * diff, axis=2)
    indexed[alpha_mask] = (np.argmin(dist, axis=1) + 1).astype(np.uint16)
    return indexed


def load_sprite_asset(spec):
    if not HAS_IMG2NEO or not HAS_PIL:
        raise RuntimeError(
            f"PIL/img2neo not available to auto-convert {os.path.basename(spec['path'])}. "
            "Install Pillow: pip install Pillow"
        )

    img = Image.open(spec["path"])
    source_w, source_h = img.size
    canvas, left, top, content_w, content_h = fit_sprite_rgba(
        img,
        spec["target_width"],
        spec["target_height"],
        spec["anchor"],
    )
    rgba = np.array(canvas, dtype=np.uint8)
    alpha_mask = rgba[:, :, 3] >= 16
    rgb = snap_neogeo(rgba[:, :, :3])

    palette15 = make_sprite_palette(rgb[alpha_mask])
    indexed = index_sprite_pixels(rgb, alpha_mask, palette15)

    palette = np.zeros((16, 3), dtype=np.uint16)
    palette[1:] = palette15

    spec["source_width"] = source_w
    spec["source_height"] = source_h
    spec["canvas_width"] = canvas.width
    spec["canvas_height"] = canvas.height
    spec["content_left"] = left
    spec["content_top"] = top
    spec["content_width"] = content_w
    spec["content_height"] = content_h
    return indexed, palette


def load_screen_asset(spec):
    reader = png.Reader(spec["path"])
    source_w, source_h, rows, metadata = reader.read()

    spec["source_width"] = source_w
    spec["source_height"] = source_h

    if metadata.get("palette"):
        indexed = np.vstack(list(map(np.uint16, rows)))
        palette = np.array(metadata["palette"], dtype=np.uint16)
        spec["canvas_width"] = indexed.shape[1]
        spec["canvas_height"] = indexed.shape[0]
        spec["content_left"] = 0
        spec["content_top"] = 0
        spec["content_width"] = indexed.shape[1]
        spec["content_height"] = indexed.shape[0]
        return indexed, palette

    if not HAS_IMG2NEO or not HAS_PIL:
        raise RuntimeError(
            f"PIL/img2neo not available to auto-convert {os.path.basename(spec['path'])}. "
            "Install Pillow: pip install Pillow"
        )

    tw = (spec["target_width"] // 16) * 16
    th = (spec["target_height"] // 16) * 16

    img = open_as_rgb(spec["path"])
    img = crop_center(img, tw, th)
    if spec["contrast"] != 1.0:
        img = ImageEnhance.Contrast(img).enhance(spec["contrast"])
    if spec["saturation"] != 1.0:
        img = ImageEnhance.Color(img).enhance(spec["saturation"])
    if spec["sharpen_percent"] > 0 and spec["sharpen_radius"] > 0.0:
        img = img.filter(
            ImageFilter.UnsharpMask(
                radius=spec["sharpen_radius"],
                percent=spec["sharpen_percent"],
                threshold=spec["sharpen_threshold"],
            )
        )

    arr = snap_neogeo(np.array(img, dtype=np.uint8))
    pal = kmeans_palette(
        arr,
        n=15,
        iters=spec["kmeans_iters"],
        sample_limit=spec["kmeans_samples"],
    )
    if spec["dither"] == "floyd":
        indexed = floyd_steinberg(arr, pal).astype(np.uint16)
    elif spec["dither"] == "none":
        indexed = nearest_palette_indices(arr, pal).astype(np.uint16)
    else:
        indexed = ordered_dither(arr, pal).astype(np.uint16)

    spec["canvas_width"] = tw
    spec["canvas_height"] = th
    spec["content_left"] = 0
    spec["content_top"] = 0
    spec["content_width"] = tw
    spec["content_height"] = th
    return indexed, pal.astype(np.uint16)


def finalize_spec(spec):
    used_cols = int(math.ceil(spec["content_width"] / 16.0)) if spec["content_width"] else 0
    used_rows = int(math.ceil(spec["content_height"] / 16.0)) if spec["content_height"] else 0
    used_col_start = int(spec["content_left"] // 16) if spec["content_width"] else 0
    used_row_start = int(spec["content_top"] // 16) if spec["content_height"] else 0

    spec["used_tile_cols"] = used_cols
    spec["used_tile_rows"] = used_rows
    spec["used_tile_col_start"] = used_col_start
    spec["used_tile_row_start"] = used_row_start
    spec["content_left_mod"] = int(spec["content_left"] % 16) if spec["content_width"] else 0
    spec["content_top_mod"] = int(spec["content_top"] % 16) if spec["content_height"] else 0
    spec["used_tile_count"] = used_cols * used_rows
    spec["sprite_strips"] = max(1, used_cols)
    spec["sprite_active_rows"] = max(1, used_rows)


def main():
    sqlite3.register_adapter(np.ndarray, adapt_array)
    sqlite3.register_converter("array", convert_array)

    specs = build_asset_specs("in")
    if not specs:
        print("No PNG files found in in")
        return

    print(f"Importing {len(specs)} images (rule-driven order):")

    try:
        conn = sqlite3.connect("neorom.db", detect_types=sqlite3.PARSE_DECLTYPES)
        print(sqlite3.sqlite_version)
        conn.execute("PRAGMA journal_mode=WAL")
        conn.execute("PRAGMA synchronous=NORMAL")
        cur = conn.cursor()
        cur.execute("DELETE FROM image")

        db_rows = []
        for spec in specs:
            print(f"  [{spec['db_index']:3d}] {spec['name']}  mode={spec['mode']}")
            if spec["mode"] == "sprite":
                indexed, palette = load_sprite_asset(spec)
            else:
                indexed, palette = load_screen_asset(spec)

            finalize_spec(spec)
            db_rows.append((spec["db_index"], indexed, palette))

        cur.executemany("INSERT INTO image (idx,data,palette) VALUES (?,?,?)", db_rows)
        conn.commit()
        save_manifest(specs)
        write_out_srt(specs)
        print(f"Imported {len(db_rows)} images.")
    except Error as exc:
        print(exc)
        raise
    finally:
        conn.close()


if __name__ == "__main__":
    main()
