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

# Screen-converter dispatch.  Both makefiles' `art` target exports
# ARTBOX_TILE=1 so the default route is the tile-local pipeline
# (img2neo_tile): per-tile k-means + per-tile Floyd-Steinberg + greedy
# MAE bank dedup + Lab-nearest pixel remap into a representative
# palette derived from the weighted union of banks.  Preserves the
# per-tile dither micro-detail through to the final indices.
#
# Precedence (highest first):
#   ARTBOX_LEGACY=1  -> original nearest-neighbour-against-global-palette
#                        path (kept for diffing / sanity-check builds).
#   ARTBOX_CRT=1     -> CRT-tuned pipeline (img2neo_crt: Lab k-means +
#                        horizontal-biased FS + gamma 1.20 / contrast
#                        1.10 pre-boost).  Set by `make art-crt`.
#   ARTBOX_TILE=1    -> tile-local pipeline (default; set by `make art`).
#   (none set)       -> tile-local if available, else CRT, else legacy.
USE_CRT    = os.environ.get("ARTBOX_CRT",    "").strip() in ("1", "true", "yes", "on")
USE_TILE   = os.environ.get("ARTBOX_TILE",   "").strip() in ("1", "true", "yes", "on")
USE_LEGACY = os.environ.get("ARTBOX_LEGACY", "").strip() in ("1", "true", "yes", "on")
try:
    from img2neo_crt import convert_screen_to_indexed as crt_convert_screen
    HAS_CRT = True
except ImportError:
    HAS_CRT = False
    crt_convert_screen = None
try:
    # ARTBOX_TILE=1 routes through the consolidated "vivid" pipeline:
    # per-tile CIE-Lab k-means + per-tile RGB-Euclidean Floyd-Steinberg
    # (img2neo_tile.process_vivid_artbox_pipeline), collapsed to a
    # single 15-colour palette for the existing single-bank downstream
    # via img2neo_tile.convert_screen_via_vivid_pipeline.
    from img2neo_tile import convert_screen_via_vivid_pipeline as tile_convert_screen
    # Sprite import also flips to the unified vivid pipeline by default
    # (asset_type="sprite"): one CIE-Lab master palette covering every
    # frame of the group, scanline FS with Teflon Routing + alpha-aware
    # transparent drain.  derive_master_sprite_palette computes the
    # group master; _vivid_pipeline_from_rgba does the quantization
    # from an anchor-aware fit_sprite_rgba + alpha_bleed canvas.
    from img2neo_tile import (
        derive_master_sprite_palette as _derive_master_sprite_palette,
        _vivid_pipeline_from_rgba as _vivid_quantize_canvas,
    )
    HAS_TILE = True
    HAS_VIVID_SPRITES = True
except ImportError:
    HAS_TILE = False
    HAS_VIVID_SPRITES = False
    tile_convert_screen = None
    _derive_master_sprite_palette = None
    _vivid_quantize_canvas = None


def adapt_array(arr):
    out = io.BytesIO()
    np.save(out, arr)
    out.seek(0)
    return sqlite3.Binary(out.read())


def convert_array(text):
    out = io.BytesIO(text)
    out.seek(0)
    return np.load(out)


def strip_halo_edges(rgba, luma_threshold=220):
    """
    Cut the bright outer ring from a sprite's silhouette.

    Many source PNGs were composited against a white matte or carry
    intentional bright outlines that, against a darker in-game backdrop,
    read as a "halo" around the sprite.  This pass kills only the
    pixels that:
        - are opaque (alpha >= 128) in the source, AND
        - have at least one transparent 4-neighbour, AND
        - have luma >= luma_threshold.

    Those pixels become fully transparent (alpha = 0).  alpha_bleed,
    when run afterwards, will then re-bleed the next-inner colour into
    the newly transparent ring so the quantiser never sees stale white
    RGB from the matte.

    Other bright pixels (interior highlights, eyes, magic, metal
    speculars) are NOT touched — only the contour ring.  Controlled
    per-asset via the halo_strip / halo_luma_threshold spec fields so
    sprites that NEED a bright outline (signs, glow effects) can opt
    out.
    """
    rgba = np.asarray(rgba, dtype=np.uint8)
    if rgba.ndim != 3 or rgba.shape[2] != 4:
        return rgba

    alpha = rgba[:, :, 3]
    opaque = alpha >= 128
    if not opaque.any():
        return rgba

    # Edge opaque = opaque pixel with at least one transparent 4-neighbour
    transparent = ~opaque
    edge = np.zeros_like(opaque)
    edge[1:, :]  |= opaque[1:, :]  & transparent[:-1, :]
    edge[:-1, :] |= opaque[:-1, :] & transparent[1:, :]
    edge[:, 1:]  |= opaque[:, 1:]  & transparent[:, :-1]
    edge[:, :-1] |= opaque[:, :-1] & transparent[:, 1:]

    rgb = rgba[:, :, :3].astype(np.int32)
    luma = (299 * rgb[:, :, 0] + 587 * rgb[:, :, 1] + 114 * rgb[:, :, 2]) // 1000
    halo = edge & (luma >= luma_threshold)

    if not halo.any():
        return rgba

    out = rgba.copy()
    out[halo, 3] = 0
    return out


def audit_sprite_transparency(spec, indexed, label="post-convert"):
    """
    Verify the transparency contract on a converted sprite.

    Required invariants:
      (1) every canvas pixel where the source PNG's alpha was below
          the opaque threshold must be palette index 0;
      (2) every canvas pixel outside the source PNG's placement
          rectangle must be palette index 0;
      (3) tiles outside the used_tile_cols x used_tile_rows render
          rectangle (within the asset's 16x16 tile allocation) must
          be entirely palette index 0.

    Raises AssertionError on the first violation with enough context
    to identify the offending sprite.
    """
    indexed = np.asarray(indexed, dtype=np.uint16)
    if indexed.ndim != 2:
        return

    # Reload the source's alpha to drive checks 1 and 2 (no resize -
    # spec records the canvas placement directly).
    if not HAS_PIL:
        return
    img = Image.open(spec["path"]).convert("RGBA")
    src_alpha = np.array(img)[:, :, 3]
    src_op = src_alpha >= 128

    ch, cw = indexed.shape
    left = int(spec.get("content_left", 0))
    top  = int(spec.get("content_top",  0))
    sh = min(int(src_op.shape[0]), ch - top)
    sw = min(int(src_op.shape[1]), cw - left)

    # Check 1 + 2: pixel-level transparency.
    src_in_canvas_op = np.zeros((ch, cw), dtype=bool)
    if sh > 0 and sw > 0:
        src_in_canvas_op[top:top + sh, left:left + sw] = src_op[:sh, :sw]
    must_be_zero = ~src_in_canvas_op
    leaks = int(((indexed != 0) & must_be_zero).sum())
    if leaks:
        ys, xs = np.where((indexed != 0) & must_be_zero)
        first = list(zip(xs[:3].tolist(), ys[:3].tolist()))
        raise AssertionError(
            f"{label} audit: spec {spec.get('name', '?')}: {leaks} canvas "
            f"pixels are non-zero where the source was transparent or padding "
            f"(first: {first})"
        )

    # Check 3: tile padding outside the render rectangle.
    s = int(spec.get("sprite_strips", 0))
    r = int(spec.get("sprite_active_rows", 0))
    cs = int(spec.get("used_tile_col_start", 0))
    rs = int(spec.get("used_tile_row_start", 0))
    if s and r:
        # The render rectangle the engine reads is rows [rs..rs+r),
        # cols [cs..cs+s) within the 16x16 asset-tile allocation.
        tile_cols_total = cw // 16
        bad_tiles = []
        for tr in range(16):
            for tc in range(16):
                inside = (rs <= tr < rs + r) and (cs <= tc < cs + s)
                if inside:
                    continue
                y0, x0 = tr * 16, tc * 16
                if y0 >= ch or x0 >= cw:
                    continue
                block = indexed[y0:y0 + 16, x0:x0 + 16]
                if (block != 0).any():
                    bad_tiles.append((tr, tc))
                    if len(bad_tiles) > 5:
                        break
            if len(bad_tiles) > 5:
                break
        if bad_tiles:
            # Padding-tile leaks aren't visible (the engine never reads
            # those tiles) but they hint at a packer / canvas leak we
            # want to know about.  Warn only — do not abort the build.
            print(f"    WARN {label} audit: spec {spec.get('name', '?')}: "
                  f"{len(bad_tiles)}+ non-zero tiles outside the strips x rows "
                  f"render rect (first: {bad_tiles[:5]})")


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
    # Optional outer-ring halo strip BEFORE alpha_bleed so alpha_bleed
    # then re-bleeds the next-inner colour into the freshly transparent
    # ring.  Drives the matte-cleanup for sprites whose artist painted
    # the silhouette edge in a near-white colour against a white
    # background; configured per-asset via the assets.cfg rule.
    if spec.get("halo_strip"):
        rgba = strip_halo_edges(
            rgba,
            luma_threshold=int(spec.get("halo_luma_threshold", 220)),
        )
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


def build_master_sprite_palettes(specs):
    """
    Vivid-pipeline replacement for build_shared_sprite_palettes.
    Groups sprite specs by subdir / category (characters, npcs) and
    derives ONE CIE-Lab k-means++ master palette per group from the
    union of every group sprite's opaque pixels.  Used by
    load_sprite_asset_vivid: every frame of one character renders
    against the same 15-colour palette, so animations cannot flicker
    or shift hues between frames.
    """
    shared = {}
    if not HAS_VIVID_SPRITES or not HAS_PIL:
        return shared
    for group in ("characters", "npcs"):
        group_specs = [
            spec for spec in specs
            if spec["mode"] == "sprite"
            and (spec.get("subdir") == group or spec.get("category") == group)
        ]
        if len(group_specs) <= 1:
            continue
        paths = [spec["path"] for spec in group_specs]
        shared[group] = _derive_master_sprite_palette(paths, n_colors=15)
    return shared


def load_sprite_asset_vivid(spec, shared_master=None):
    """
    Vivid-pipeline sprite import.  Anchor-aware positioning via
    fit_sprite_rgba (preserves the spec's bottom-center / center
    anchor for sprite placement), inward-edge dilation via
    alpha_bleed (kills white halo on Lanczos-AA contours), then
    quantisation through img2neo_tile._vivid_pipeline_from_rgba in
    sprite mode (single palette across the whole image, no per-tile
    drift).  When `shared_master` is supplied (output of
    build_master_sprite_palettes) every frame of the group renders
    against the same 15-colour palette so animations are pixel-stable.

    Returns the same (indexed_u16, palette16_u16) tuple shape that
    load_sprite_asset produces so call sites stay identical.
    """
    if not HAS_IMG2NEO or not HAS_PIL or not HAS_VIVID_SPRITES:
        raise RuntimeError(
            f"img2neo_tile not available to auto-convert "
            f"{os.path.basename(spec['path'])}; install Pillow + numpy."
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
    # Outer-ring halo strip before alpha_bleed (so the bleed re-fills
    # the freshly-transparent ring with the next-inner colour).
    if spec.get("halo_strip"):
        rgba = strip_halo_edges(
            rgba,
            luma_threshold=int(spec.get("halo_luma_threshold", 220)),
        )
    if alpha_bleed is not None:
        rgba = alpha_bleed(rgba, opaque_alpha=128)

    master = None
    if shared_master is not None:
        master = np.asarray(shared_master, dtype=np.uint8).reshape(-1, 3)
        if master.shape[0] >= 15:
            master = master[:15]

    ready_tiles, final_palettes = _vivid_quantize_canvas(
        rgba.astype(np.float32),
        asset_type="sprite",
        master_palette=master,
    )

    canvas_h, canvas_w = rgba.shape[:2]
    tile_cols = canvas_w // 16

    indexed = np.zeros((canvas_h, canvas_w), dtype=np.uint16)
    for ti, tile_row in enumerate(ready_tiles):
        ty, tx = divmod(ti, tile_cols)
        y0, x0 = ty * 16, tx * 16
        tile = np.array(tile_row, dtype=np.uint8).reshape(16, 16)
        indexed[y0:y0 + 16, x0:x0 + 16] = tile.astype(np.uint16)

    sprite_palette15 = np.array(final_palettes[0],
                                  dtype=np.uint8).reshape(15, 3)
    palette = np.zeros((16, 3), dtype=np.uint16)
    palette[1:] = sprite_palette15.astype(np.uint16)

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

    # Dispatch (see module-level header for the full precedence table):
    #   LEGACY  -> fall through to legacy nearest-neighbour
    #   CRT     -> img2neo_crt
    #   TILE    -> img2neo_tile (Makefile default)
    #   nothing -> tile if available, else CRT, else legacy
    convert_fn = None
    if USE_LEGACY:
        convert_fn = None
    elif USE_CRT and HAS_CRT:
        convert_fn = crt_convert_screen
    elif USE_TILE and HAS_TILE:
        convert_fn = tile_convert_screen
    elif HAS_TILE:
        convert_fn = tile_convert_screen        # default
    elif HAS_CRT:
        convert_fn = crt_convert_screen         # fallback

    if convert_fn is not None:
        indexed, palette, meta = convert_fn(
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
    used_col_start = int(spec["content_left"] // 16) if spec["content_width"] else 0
    used_row_start = int(spec["content_top"] // 16) if spec["content_height"] else 0
    left_mod = int(spec["content_left"] % 16) if spec["content_width"] else 0
    top_mod  = int(spec["content_top"]  % 16) if spec["content_height"] else 0

    # When the content's painted bounding box starts mid-tile, the rightmost
    # used tile column has its leftmost (left_mod) pixels worth of art and the
    # leftmost used tile column has its rightmost pixels worth of art.  Naively
    # taking ceil(content_width / 16) ignores the sub-tile offset and
    # under-counts the columns whenever (left_mod + content_width) crosses an
    # extra tile boundary, leaving the rightmost N pixels of the art in a tile
    # column the engine never reads.  Same shape for rows.
    used_cols = (int(math.ceil((left_mod + spec["content_width"]) / 16.0))
                 if spec["content_width"] else 0)
    used_rows = (int(math.ceil((top_mod  + spec["content_height"]) / 16.0))
                 if spec["content_height"] else 0)

    spec["used_tile_cols"] = used_cols
    spec["used_tile_rows"] = used_rows
    spec["used_tile_col_start"] = used_col_start
    spec["used_tile_row_start"] = used_row_start
    spec["content_left_mod"] = left_mod
    spec["content_top_mod"]  = top_mod
    spec["used_tile_count"] = used_cols * used_rows
    spec["sprite_strips"] = max(1, used_cols)
    spec["sprite_active_rows"] = max(1, used_rows)

    # Asset audit: catch any spec that still has the painted bbox extending
    # past the chosen used_cols x used_rows rectangle.  With the corrected
    # formula above this should never trip; the assert is a guardrail against
    # future refactors that touch finalize_spec.
    if spec["content_width"] and left_mod + spec["content_width"] > used_cols * 16:
        raise AssertionError(
            f"asset {spec.get('path', '?')}: "
            f"content_left_mod={left_mod} + content_width={spec['content_width']} "
            f"exceeds used_tile_cols * 16 = {used_cols * 16}"
        )
    if spec["content_height"] and top_mod + spec["content_height"] > used_rows * 16:
        raise AssertionError(
            f"asset {spec.get('path', '?')}: "
            f"content_top_mod={top_mod} + content_height={spec['content_height']} "
            f"exceeds used_tile_rows * 16 = {used_rows * 16}"
        )


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
        # Sprite import dispatch matches the screen-import dispatch:
        # ARTBOX_LEGACY=1 -> legacy build_shared_sprite_palettes +
        # load_sprite_asset (naive nearest-colour, no dither).
        # default       -> vivid pipeline: build_master_sprite_palettes
        # derives ONE CIE-Lab master per group (characters / npcs) and
        # load_sprite_asset_vivid quantises every frame against that
        # master with scanline FS + Teflon Routing.  Animations are
        # bit-stable across frames by construction (single palette).
        if USE_LEGACY or not HAS_VIVID_SPRITES:
            shared_palettes = build_shared_sprite_palettes(specs)
            sprite_loader = load_sprite_asset
        else:
            shared_palettes = build_master_sprite_palettes(specs)
            sprite_loader = load_sprite_asset_vivid
        for spec in specs:
            print(f"  [{spec['db_index']:3d}] {spec['name']}  mode={spec['mode']}")
            if spec["mode"] == "sprite":
                shared = shared_palettes.get(spec.get("subdir"))
                indexed, palette = sprite_loader(spec, shared)
            else:
                indexed, palette = load_screen_asset(spec)

            finalize_spec(spec)

            # Verify the transparency contract after the spec is final
            # (finalize_spec sets sprite_strips / sprite_active_rows /
            # used_tile_col_start / used_tile_row_start which the audit
            # needs to bound check 3).  Sprite-only — backgrounds tile
            # the entire canvas by design.
            if spec["mode"] == "sprite":
                audit_sprite_transparency(spec, indexed,
                                          label=spec.get("rule_name", "?"))

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
