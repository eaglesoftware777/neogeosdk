#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
#!/usr/bin/env python3
"""
Neo Geo FIX ROM (S1) tile generator.

Builds the 128 KB 777-s1.s1 from:
  1. sfix.sfix system-font ROM  (standard ASCII/FIX tiles)
  2. Existing 777-s1.s1  (non-infix area preserved at original addresses)
  3. infix/*.png images imported via the imagefix DB table at tile 256+

Hardware tile format (after romtool /f or direct from this script):
  Each 8×8 / 4-bpp tile = 32 bytes.
  4 column-pair groups × 8 row bytes.
  Column-pair order: [4,5], [6,7], [0,1], [2,3].
  Each byte: MSNibble = right-column pixel, LSNibble = left-column pixel.
"""

import io, os, sys, struct
import numpy as np
try:
    import pysqlite3 as sqlite3
    from pysqlite3 import Error
except ImportError:
    import sqlite3
    from sqlite3 import Error

# ── constants ──────────────────────────────────────────────────────────────────
TILE_BYTES = 32
ROM_SIZE   = 131072          # 128 KB
NUM_TILES  = ROM_SIZE // TILE_BYTES   # 4096

SCRIPT_DIR  = os.path.dirname(os.path.abspath(__file__))
DATA_DIR    = os.path.abspath(os.environ.get("ARTBOX_DATA_DIR", SCRIPT_DIR))
_GAME_ID    = os.environ.get('GAME_ID', '777')
_GAME       = os.environ.get('GAME', 'demo')
ROMS_DIR    = os.path.join(SCRIPT_DIR, '..', 'roms', _GAME)
GAME_S1     = os.path.join(ROMS_DIR, f'{_GAME_ID}-s1.s1')
SFIX_ROM    = os.path.join(ROMS_DIR, 'sfix.sfix')
SFIX_FALLBACK = os.path.join(SCRIPT_DIR, '..', 'roms', 'neogeo', 'sfix.sfix')
OUT_S1      = os.path.join(DATA_DIR, f'{_GAME_ID}-s1.s1')
DB_PATH     = os.path.join(DATA_DIR, 'neorom.db')
INFIX_DIR   = os.path.join(DATA_DIR, 'infix')
INFIX_REGION_START = int(os.environ.get("INFIX_TILE_BASE", "256"), 0)
INFIX_PAL_HDR = os.path.join(DATA_DIR, 'infix_palettes.h')

# Each infix image gets its own FIX palette bank, starting at this slot.
# Banks 0..3 are reserved (0..2 = text, 3 = padding/future use).
INFIX_PAL_BANK_BASE = 4

def _rgb_to_neogeo_word(r8, g8, b8):
    """Pack an 8-bit RGB triple to a NeoGeo palette word.

    Bit layout: [D][R0][G0][B0][R4..R1][G4..G1][B4..B1]
    where Rn is bit n of the 5-bit channel value.
    """
    r5 = max(0, min(31, int(r8) >> 3))
    g5 = max(0, min(31, int(g8) >> 3))
    b5 = max(0, min(31, int(b8) >> 3))
    return (((r5 & 1) << 14) | ((g5 & 1) << 13) | ((b5 & 1) << 12) |
            ((r5 >> 1) << 8)  | ((g5 >> 1) << 4)  | ((b5 >> 1)))

# ── tile encoding / decoding ───────────────────────────────────────────────────
_COL_PAIRS  = [(4,5),(6,7),(0,1),(2,3)]

def encode_tile(px8x8):
    """8×8 uint8 array  →  32-byte hardware tile."""
    out = bytearray(TILE_BYTES)
    for gi, (cl, cr) in enumerate(_COL_PAIRS):
        for row in range(8):
            lp = int(px8x8[row, cl]) & 0xF
            rp = int(px8x8[row, cr]) & 0xF
            out[gi*8 + row] = (rp << 4) | lp
    return bytes(out)

def decode_tile(raw32):
    """32-byte hardware tile  →  8×8 uint8 array."""
    px = np.zeros((8,8), dtype=np.uint8)
    for gi, (cl, cr) in enumerate(_COL_PAIRS):
        for row in range(8):
            b = raw32[gi*8 + row] if isinstance(raw32, (bytes,bytearray)) else raw32[gi*8+row]
            px[row, cl] = b & 0xF
            px[row, cr] = (b >> 4) & 0xF
    return px

def is_empty_tile(raw32):
    return all(b in (0x00, 0x11) for b in raw32)

def remap_tile_pen(raw32, src_pen, dst_pen):
    px = decode_tile(raw32)
    px[px == src_pen] = dst_pen
    return encode_tile(px)

def make_base_font_transparent():
    end = min(INFIX_REGION_START, 256, NUM_TILES)
    for slot in range(end):
        start = slot * TILE_BYTES
        raw = bytes(rom[start:start + TILE_BYTES])
        rom[start:start + TILE_BYTES] = remap_tile_pen(raw, 2, 0)

# ── image → tile list ──────────────────────────────────────────────────────────
def image_to_tiles(indexed_2d):
    """Slice any ×8 sized indexed pixel array into 32-byte hardware tiles (row-major)."""
    h, w = indexed_2d.shape
    tiles = []
    for ry in range(0, h - h % 8 if h % 8 else h, 8):
        for rx in range(0, w - w % 8 if w % 8 else w, 8):
            block = indexed_2d[ry:ry+8, rx:rx+8].copy()
            if block.shape != (8,8):
                pad = np.zeros((8,8), dtype=np.uint8)
                pad[:block.shape[0],:block.shape[1]] = block
                block = pad
            tiles.append(encode_tile(block))
    return tiles

# ── numpy DB serialisation helpers ────────────────────────────────────────────
def _np_from_blob(blob):
    """Deserialise a numpy array stored as a .npy blob (regardless of column type)."""
    raw = bytes(blob)
    try:
        return np.load(io.BytesIO(raw))
    except Exception:
        return np.frombuffer(raw, dtype=np.uint8)

# ── read all tiles from a ROM file ────────────────────────────────────────────
def read_rom(path):
    if not os.path.exists(path):
        return []
    with open(path,'rb') as f:
        data = f.read()
    n = len(data) // TILE_BYTES
    return [data[i*TILE_BYTES:(i+1)*TILE_BYTES] for i in range(n)]

# ── build the 128-KB ROM buffer ───────────────────────────────────────────────
rom = bytearray(ROM_SIZE)   # transparent = all 0x00

# Step 1: seed with sfix.sfix so standard NeoGeo fonts are available everywhere
sfix_path = SFIX_ROM if os.path.exists(SFIX_ROM) else SFIX_FALLBACK
sfix_tiles = read_rom(sfix_path)
if sfix_tiles:
    for i, t in enumerate(sfix_tiles[:NUM_TILES]):
        rom[i*TILE_BYTES:(i+1)*TILE_BYTES] = t
    print(f"Seeded {len(sfix_tiles[:NUM_TILES])} tiles from {os.path.basename(sfix_path)}")
else:
    print(f"Warning: no sfix.sfix found in {SFIX_ROM} or {SFIX_FALLBACK}")

# Step 2: overlay the existing game S1 ROM outside the generated infix region.
# Tiles 0-255 remain the normal font area.  Tiles 256+ are deterministic
# generated art slots so runtime code can address infix images by fixed tile
# numbers without depending on old ROM contents.
game_tiles = read_rom(GAME_S1)
preserved  = 0
for i, t in enumerate(game_tiles[:NUM_TILES]):
    if i >= INFIX_REGION_START:
        continue
    if not is_empty_tile(t):
        rom[i*TILE_BYTES:(i+1)*TILE_BYTES] = t
        preserved += 1
if preserved:
    print(f"Preserved {preserved} non-empty base-font tiles from {os.path.basename(GAME_S1)}")

make_base_font_transparent()
print("Made base FIX font background transparent")

# ── collect infix tiles from DB ───────────────────────────────────────────────
infix_tiles = []
# Per-image metadata for the generated palette header:
#   (idx, cols, rows, tile_base, [16 NeoGeo palette words])
infix_meta = []

def _pack_image_palette(pal_arr):
    """Convert an imagefix.palette blob (uint16 array) to 16 NeoGeo words.

    The PNG-imported rows store a (256, 4) RGBA palette; convert_fix_png
    rows store a (16, 3) RGB palette pre-snapped to the NeoGeo grid.
    Either way we only need the first 16 entries, and index 0 must be
    transparent (word=0).
    """
    words = [0] * 16
    if pal_arr.ndim != 2 or pal_arr.shape[0] < 1:
        return words
    n = min(16, pal_arr.shape[0])
    for i in range(1, n):  # leave entry 0 transparent
        row = pal_arr[i]
        r = int(row[0]); g = int(row[1]); b = int(row[2])
        words[i] = _rgb_to_neogeo_word(r, g, b)
    return words

try:
    conn = sqlite3.connect(DB_PATH)
    conn.execute("PRAGMA journal_mode=WAL")
    cur = conn.cursor()
    cur.execute("SELECT idx, data, palette FROM imagefix ORDER BY idx")
    for row in cur.fetchall():
        try:
            arr = _np_from_blob(row[1])
            if arr.ndim == 1:
                side = int(np.sqrt(len(arr)))
                arr = arr.reshape(side, -1)
            arr = arr.astype(np.uint8)
            tile_base_for_image = INFIX_REGION_START + len(infix_tiles)
            cols = arr.shape[1] // 8
            rows_ = arr.shape[0] // 8
            tiles = image_to_tiles(arr)
            infix_tiles.extend(tiles)

            pal_arr = _np_from_blob(row[2]) if row[2] is not None else np.zeros((16,3), np.uint16)
            words = _pack_image_palette(pal_arr)
            infix_meta.append((int(row[0]), cols, rows_, tile_base_for_image, words))
        except Exception as e:
            print(f"  skip DB row {row[0]}: {e}")
    conn.close()
    print(f"Loaded {len(infix_tiles)} tiles from imagefix DB")
except Exception as e:
    print(f"DB error: {e}")

# ── fallback: read PNG files directly from infix/ dir ─────────────────────────
if not infix_tiles:
    try:
        import png as pypng
        pngs = sorted(f for f in os.listdir(INFIX_DIR) if f.endswith('.png'))
        for fname in pngs:
            try:
                reader = pypng.Reader(os.path.join(INFIX_DIR, fname))
                w, h, rows, meta = reader.read()
                indexed = np.vstack([np.frombuffer(r, dtype=np.uint8) for r in rows])
                infix_tiles.extend(image_to_tiles(indexed))
                print(f"  read {fname} ({w}×{h})")
            except Exception as e:
                print(f"  skip {fname}: {e}")
        print(f"Loaded {len(infix_tiles)} tiles directly from infix PNGs")
    except ImportError:
        print("pypng not available; no infix tiles loaded")

# ── write infix tiles at deterministic tile numbers ───────────────────────────
# The demo and generated docs expect the first infix tile to start at tile 256.
# This region deliberately overwrites the system font copy above tile 255.
written = 0
max_infix_tiles = NUM_TILES - INFIX_REGION_START
for tile_data in infix_tiles:
    if written >= max_infix_tiles:
        print(f"Warning: ROM full ({max_infix_tiles} infix slots), "
              f"{len(infix_tiles) - written} infix tiles not written")
        break
    slot = INFIX_REGION_START + written
    rom[slot*TILE_BYTES:(slot+1)*TILE_BYTES] = tile_data
    written += 1

if written:
    print(f"Wrote {written} infix tiles starting at tile {INFIX_REGION_START}")

# ── emit per-image palette header ─────────────────────────────────────────────
if infix_meta:
    lines = []
    lines.append("/* Auto-generated by artbox/fixtiles.py — do not edit. */")
    lines.append("#ifndef INFIX_PALETTES_H")
    lines.append("#define INFIX_PALETTES_H")
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append(f"#define INFIX_IMAGE_COUNT     {len(infix_meta)}u")
    lines.append(f"#define INFIX_PAL_BANK_BASE   {INFIX_PAL_BANK_BASE}u")
    lines.append("")
    lines.append("typedef struct {")
    lines.append("    uint16_t tile_base;")
    lines.append("    uint8_t  cols;")
    lines.append("    uint8_t  rows;")
    lines.append("    uint8_t  pal_bank;   /* FIX palette bank index */")
    lines.append("} InfixImage;")
    lines.append("")
    lines.append("static const InfixImage INFIX_IMAGES[INFIX_IMAGE_COUNT] = {")
    for n, (idx, cols, rows_, tb, _w) in enumerate(infix_meta):
        bank = INFIX_PAL_BANK_BASE + n
        lines.append(f"    {{ {tb:4d}u, {cols:3d}u, {rows_:2d}u, {bank:2d}u }},  /* idx {idx} */")
    lines.append("};")
    lines.append("")
    lines.append("static const uint16_t INFIX_PALETTES[INFIX_IMAGE_COUNT][16] = {")
    for n, (idx, _c, _r, _tb, words) in enumerate(infix_meta):
        body = ", ".join(f"0x{w:04X}" for w in words)
        lines.append(f"    {{ {body} }},  /* idx {idx} */")
    lines.append("};")
    lines.append("")
    lines.append("#endif /* INFIX_PALETTES_H */")
    with open(INFIX_PAL_HDR, 'w') as f:
        f.write("\n".join(lines) + "\n")
    print(f"Wrote {len(infix_meta)} infix palettes → {INFIX_PAL_HDR}")

# ── write output ───────────────────────────────────────────────────────────────
with open(OUT_S1, 'wb') as f:
    f.write(rom)
print(f"Wrote {ROM_SIZE} bytes → {OUT_S1}")
