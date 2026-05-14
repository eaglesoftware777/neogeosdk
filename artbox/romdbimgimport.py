#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import io
import math
import os
import sys
from pathlib import Path

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
ROOT = Path(__file__).resolve().parent

try:
    from asset_rules import build_asset_specs, save_manifest, write_out_srt
except ImportError:
    build_asset_specs = None
    save_manifest = None
    write_out_srt = None

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


def fit_screen_rgba(img, target_w, target_h, anchor, fit_mode):
    tw = (target_w // 16) * 16
    th = (target_h // 16) * 16
    img = img.convert("RGBA")

    mode = (fit_mode or "contain").lower()
    if mode == "crop":
        w, h = img.size
        if h == 0 or w == 0:
            canvas = Image.new("RGBA", (tw, th), (0, 0, 0, 0))
            return canvas, 0, 0, 0, 0
        if (w / h) > (tw / th):
            nw = int(h * tw / th)
            left = (w - nw) // 2
            img = img.crop((left, 0, left + nw, h))
        else:
            nh = int(w * th / tw)
            top = (h - nh) // 2
            img = img.crop((0, top, w, top + nh))
        img = img.resize((tw, th), Image.LANCZOS)
        canvas = Image.new("RGBA", (tw, th), (0, 0, 0, 0))
        canvas.alpha_composite(img, (0, 0))
        return canvas, 0, 0, tw, th

    # contain / pad / letterbox (non-destructive)
    if img.width > tw or img.height > th:
        img.thumbnail((tw, th), Image.LANCZOS)

    canvas = Image.new("RGBA", (tw, th), (0, 0, 0, 0))
    left = (tw - img.width) // 2
    top = (th - img.height) // 2
    if anchor == "top-center":
        top = 0
    elif anchor == "bottom-center":
        top = th - img.height
    elif anchor == "left-center":
        left = 0
    elif anchor == "right-center":
        left = tw - img.width

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

    src = Image.open(spec["path"]).convert("RGBA")
    canvas, left, top, content_w, content_h = fit_screen_rgba(
        src, tw, th, spec.get("anchor", "center"), spec.get("fit", "contain")
    )
    img = canvas.convert("RGB")

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

    rgba = np.array(canvas, dtype=np.uint8)
    alpha_mask = rgba[:, :, 3] >= 16
    arr = snap_neogeo(np.array(img, dtype=np.uint8))

    palette15 = make_sprite_palette(arr[alpha_mask])
    indexed = index_sprite_pixels(arr, alpha_mask, palette15)
    palette = np.zeros((16, 3), dtype=np.uint16)
    palette[1:] = palette15

    spec["transparent_zero"] = 1
    spec["palette_has_zero"] = 1
    spec["canvas_width"] = tw
    spec["canvas_height"] = th
    spec["content_left"] = left
    spec["content_top"] = top
    spec["content_width"] = content_w
    spec["content_height"] = content_h
    return indexed, palette


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

    if build_asset_specs is None or save_manifest is None or write_out_srt is None:
        raise RuntimeError("asset_rules import failed; ensure artbox/asset_rules.py is present and valid.")

    specs = build_asset_specs(str(ROOT / "in"))
    if not specs:
        print("No PNG files found in in")
        return

    print(f"Importing {len(specs)} images (rule-driven order):")

    conn = None
    try:
        conn = sqlite3.connect(str(ROOT / "neorom.db"), detect_types=sqlite3.PARSE_DECLTYPES)
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
        save_manifest(specs, str(ROOT / "assets_manifest.json"))
        write_out_srt(specs, str(ROOT / "out.srt"))
        print(f"Imported {len(db_rows)} images.")
    except Error as exc:
        print(exc)
        raise
    finally:
        if conn is not None:
            conn.close()


if __name__ == "__main__":
    main()
