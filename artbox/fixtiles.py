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

# ── collect infix tiles from DB ───────────────────────────────────────────────
infix_tiles = []

try:
    conn = sqlite3.connect(DB_PATH)
    conn.execute("PRAGMA journal_mode=WAL")
    cur = conn.cursor()
    cur.execute("SELECT idx, data FROM imagefix ORDER BY idx")
    for row in cur.fetchall():
        try:
            arr = _np_from_blob(row[1])
            if arr.ndim == 1:
                side = int(np.sqrt(len(arr)))
                arr = arr.reshape(side, -1)
            infix_tiles.extend(image_to_tiles(arr.astype(np.uint8)))
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

# ── write output ───────────────────────────────────────────────────────────────
with open(OUT_S1, 'wb') as f:
    f.write(rom)
print(f"Wrote {ROM_SIZE} bytes → {OUT_S1}")
