#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
#!/usr/bin/env python3
"""Import all infix/*.png files into the imagefix DB table."""

try:
    import pysqlite3 as sqlite3
    from pysqlite3 import Error
except ImportError:
    import sqlite3
    from sqlite3 import Error

import numpy as np
import png
import io, os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    from PIL import Image
    from img2neo import alpha_bleed, snap_neogeo
    from img2neo_tile import _lab_kmeans_palette_with_anchors, _neo_palette_pop
    HAS_FIX_CONVERTER = True
except ImportError:
    HAS_FIX_CONVERTER = False

def adapt_array(arr):
    out = io.BytesIO()
    np.save(out, arr)
    out.seek(0)
    return sqlite3.Binary(out.read())

DATA_DIR  = os.path.abspath(os.environ.get("ARTBOX_DATA_DIR", os.path.dirname(os.path.abspath(__file__))))
INFIX_DIR = os.environ.get("ARTBOX_INFIX_DIR", os.path.join(DATA_DIR, 'infix'))
DB_PATH   = os.path.join(DATA_DIR, 'neorom.db')


def _pad_to_fix_grid(img):
    width, height = img.size
    target_w = ((width + 7) // 8) * 8
    target_h = ((height + 7) // 8) * 8
    canvas = Image.new("RGBA", (target_w, target_h), (0, 0, 0, 0))
    canvas.alpha_composite(img.convert("RGBA"), (0, 0))
    return canvas


def _nearest_indices(rgb, alpha_mask, palette15):
    indexed = np.zeros(alpha_mask.shape, dtype=np.uint16)
    if not alpha_mask.any():
        return indexed
    px = rgb[alpha_mask].astype(np.int32)
    pal = palette15.astype(np.int32)
    dist = np.sum((px[:, None, :] - pal[None, :, :]) ** 2, axis=2)
    indexed[alpha_mask] = (np.argmin(dist, axis=1) + 1).astype(np.uint16)
    return indexed


def convert_fix_png(path):
    if not HAS_FIX_CONVERTER:
        raise RuntimeError("Pillow is required to convert non-indexed FIX PNGs")

    img = _pad_to_fix_grid(Image.open(path))
    rgba = alpha_bleed(np.array(img, dtype=np.uint8), opaque_alpha=128)
    alpha = rgba[:, :, 3]
    opaque = alpha >= 128

    if opaque.any():
        rgb = _neo_palette_pop(rgba[:, :, :3], opaque, asset_type="sprite")
        solid = alpha >= 240
        palette_source = rgb[solid] if solid.any() else rgb[opaque]
        palette15 = _lab_kmeans_palette_with_anchors(
            palette_source, n_colors=15, asset_type="sprite")
        indexed = _nearest_indices(rgb, opaque, palette15)
    else:
        palette15 = np.zeros((15, 3), dtype=np.uint8)
        indexed = np.zeros(alpha.shape, dtype=np.uint16)

    palette16 = np.zeros((16, 3), dtype=np.uint16)
    palette16[1:] = snap_neogeo(palette15).astype(np.uint16)
    return indexed, palette16

# Collect all numeric PNG files, then any remaining named ones.
# A game with no fix-layer art of its own has no infix directory at all.  That
# is a valid setup - the FIX ROM still gets built from the font and the sfix
# seed - so read a missing directory the same way as an empty one.
if os.path.isdir(INFIX_DIR):
    pngs = sorted(f for f in os.listdir(INFIX_DIR)
                  if f.endswith('.png') and os.path.isfile(os.path.join(INFIX_DIR, f)))
else:
    print(f"No infix directory at {INFIX_DIR}; leaving the fix layer to the font.")
    pngs = []

rows = []
for i, fname in enumerate(pngs):
    path = os.path.join(INFIX_DIR, fname)
    try:
        reader = png.Reader(path)
        w, h, imap1, metadata = reader.read()
        if 'palette' in metadata:
            palettep = np.array(metadata['palette'], dtype=np.uint16)
            indexed = np.vstack(list(map(np.uint16, imap1)))
        else:
            indexed, palettep = convert_fix_png(path)
        rows.append((i, indexed, palettep))
        print(f"  queued {fname} ({w}x{h}, {len(palettep)} colors)")
    except Exception as e:
        print(f"  skip {fname}: {e}")

if not rows:
    print("No valid infix PNGs found.")
    sys.exit(0)

try:
    sqlite3.register_adapter(np.ndarray, adapt_array)
    conn = sqlite3.connect(DB_PATH, detect_types=sqlite3.PARSE_DECLTYPES)
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA synchronous=NORMAL")
    cur = conn.cursor()
    cur.execute("DELETE FROM imagefix")
    cur.executemany(
        "INSERT INTO imagefix (idx, data, palette) VALUES (?,?,?)",
        rows
    )
    conn.commit()
    print(f"Imported {len(rows)} images into imagefix table")
except Error as e:
    print(f"DB error: {e}")
finally:
    conn.close()
