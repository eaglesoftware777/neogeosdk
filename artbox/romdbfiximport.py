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

def adapt_array(arr):
    out = io.BytesIO()
    np.save(out, arr)
    out.seek(0)
    return sqlite3.Binary(out.read())

DATA_DIR  = os.path.abspath(os.environ.get("ARTBOX_DATA_DIR", os.path.dirname(os.path.abspath(__file__))))
INFIX_DIR = os.path.join(DATA_DIR, 'infix')
DB_PATH   = os.path.join(DATA_DIR, 'neorom.db')

# Collect all numeric PNG files, then any remaining named ones
pngs = sorted(f for f in os.listdir(INFIX_DIR)
              if f.endswith('.png') and os.path.isfile(os.path.join(INFIX_DIR, f)))

rows = []
for i, fname in enumerate(pngs):
    path = os.path.join(INFIX_DIR, fname)
    try:
        reader = png.Reader(path)
        w, h, imap1, metadata = reader.read()
        if 'palette' not in metadata:
            print(f"  skip {fname}: not indexed-palette PNG")
            continue
        palettep = np.array(metadata['palette'], dtype=np.uint16)
        indexed  = np.vstack(list(map(np.uint16, imap1)))
        rows.append((i, indexed, palettep))
        print(f"  queued {fname} ({w}×{h}, {len(metadata['palette'])} colors)")
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
