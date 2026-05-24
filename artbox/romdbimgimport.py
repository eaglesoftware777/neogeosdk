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
ROOT = Path(os.environ.get("ARTBOX_DATA_DIR", str(Path(__file__).resolve().parent))).resolve()

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
        alpha_bleed,
    )
    HAS_IMG2NEO = True
except ImportError:
    HAS_IMG2NEO = False
    alpha_bleed = None

# Opt-in CRT-optimised converter (Lab quantisation + horizontal-biased dither
# + gamma/contrast pre-boost).  Enabled by exporting ARTBOX_CRT=1 (or via
# `make art-crt`).  Falls back silently to the legacy
# nearest-neighbour pipeline if the module fails to import.
USE_CRT = os.environ.get("ARTBOX_CRT", "").strip() in ("1", "true", "yes", "on")
try:
    from img2neo_crt import convert_screen_to_indexed as crt_convert_screen
    HAS_CRT = True
except ImportError:
    HAS_CRT = False
    crt_convert_screen = None


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


def load_sprite_asset(spec, shared_palette15=None):
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
    # Inward edge dilation kills white/colour halos before anti-aliased
    # edge pixels get promoted to opaque palette indices.
    if alpha_bleed is not None:
        rgba = alpha_bleed(rgba, opaque_alpha=128)
    alpha_mask = rgba[:, :, 3] >= 16
    rgb = snap_neogeo(rgba[:, :, :3])

    palette15 = shared_palette15 if shared_palette15 is not None else make_sprite_palette(rgb[alpha_mask])
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


def build_shared_sprite_palettes(specs):
    shared = {}
    if not HAS_IMG2NEO or not HAS_PIL:
        return shared

    for group in ("characters", "npcs"):
        group_specs = [
            spec for spec in specs
            if spec["mode"] == "sprite"
            and (spec.get("subdir") == group or spec.get("category") == group)
        ]
        if len(group_specs) <= 1:
            continue

        opaque_chunks = []
        for spec in group_specs:
            img = Image.open(spec["path"])
            canvas, _left, _top, _content_w, _content_h = fit_sprite_rgba(
                img,
                spec["target_width"],
                spec["target_height"],
                spec["anchor"],
            )
            rgba = np.array(canvas, dtype=np.uint8)
            if alpha_bleed is not None:
                rgba = alpha_bleed(rgba, opaque_alpha=128)
            alpha_mask = rgba[:, :, 3] >= 16
            if alpha_mask.any():
                opaque_chunks.append(snap_neogeo(rgba[:, :, :3])[alpha_mask])

        if opaque_chunks:
            opaque_rgb = np.vstack(opaque_chunks)
            if len(opaque_rgb) > 65536:
                rng = np.random.default_rng(0)
                opaque_rgb = opaque_rgb[rng.choice(len(opaque_rgb), 65536, replace=False)]
            shared[group] = make_sprite_palette(opaque_rgb)
    return shared


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

    if USE_CRT and HAS_CRT:
        indexed, palette, meta = crt_convert_screen(
            spec["path"],
            target_w=tw,
            target_h=th,
            fit=spec.get("fit", "contain"),
            anchor=spec.get("anchor", "center"),
            n_colors=15,
        )
        spec["transparent_zero"] = 1
        spec["palette_has_zero"] = 1
        spec["canvas_width"]   = meta["canvas_width"]
        spec["canvas_height"]  = meta["canvas_height"]
        spec["content_left"]   = meta["content_left"]
        spec["content_top"]    = meta["content_top"]
        spec["content_width"]  = meta["content_width"]
        spec["content_height"] = meta["content_height"]
        return indexed, palette

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


def normalize_sequence_bounds(specs):
    for group in ("npcs",):
        group_specs = [
            spec for spec in specs
            if spec["mode"] == "sprite"
            and (spec.get("subdir") == group or spec.get("category") == group)
        ]
        if len(group_specs) <= 1:
            continue

        min_col = min(spec["used_tile_col_start"] for spec in group_specs)
        min_row = min(spec["used_tile_row_start"] for spec in group_specs)
        max_col = max(spec["used_tile_col_start"] + spec["used_tile_cols"] for spec in group_specs)
        max_row = max(spec["used_tile_row_start"] + spec["used_tile_rows"] for spec in group_specs)

        for spec in group_specs:
            spec["used_tile_col_start"] = min_col
            spec["used_tile_row_start"] = min_row
            spec["used_tile_cols"] = max_col - min_col
            spec["used_tile_rows"] = max_row - min_row
            spec["used_tile_count"] = spec["used_tile_cols"] * spec["used_tile_rows"]
            spec["sprite_strips"] = max(1, spec["used_tile_cols"])
            spec["sprite_active_rows"] = max(1, spec["used_tile_rows"])


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
        shared_palettes = build_shared_sprite_palettes(specs)
        for spec in specs:
            print(f"  [{spec['db_index']:3d}] {spec['name']}  mode={spec['mode']}")
            if spec["mode"] == "sprite":
                shared_palette15 = shared_palettes.get(spec.get("subdir"))
                indexed, palette = load_sprite_asset(spec, shared_palette15)
            else:
                indexed, palette = load_screen_asset(spec)

            finalize_spec(spec)
            db_rows.append((spec["db_index"], indexed, palette))

        normalize_sequence_bounds(specs)
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
