#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import io
import sys

import numpy as np
import png

try:
    import pysqlite3 as sqlite3
    from pysqlite3 import Error
except ImportError:
    import sqlite3
    from sqlite3 import Error


def adapt_array(arr):
    out = io.BytesIO()
    np.save(out, arr)
    out.seek(0)
    return sqlite3.Binary(out.read())


def convert_array(text):
    out = io.BytesIO(text)
    out.seek(0)
    return np.load(out)


def main():
    image_number = int(sys.argv[1:][0])

    sqlite3.register_adapter(np.ndarray, adapt_array)
    sqlite3.register_converter("array", convert_array)

    try:
        conn = sqlite3.connect("neorom.db", detect_types=sqlite3.PARSE_DECLTYPES)
        print(sqlite3.sqlite_version)
        conn.execute("PRAGMA journal_mode=WAL")
        conn.execute("PRAGMA synchronous=NORMAL")
        cur = conn.cursor()
        cur.execute("DELETE FROM image")

        rows = []
        for image_index in range(image_number):
            reader = png.Reader(f"in/{image_index}.png")
            _, _, image_rows, metadata = reader.read()
            palette = np.array(metadata["palette"], dtype=np.uint16)
            indexed = np.vstack(list(map(np.uint16, image_rows)))
            rows.append((image_index, indexed, palette))

        cur.executemany("INSERT INTO image (idx,data,palette) VALUES (?,?,?)", rows)
        conn.commit()
    except Error as exc:
        print(exc)
        raise
    finally:
        conn.close()


if __name__ == "__main__":
    main()
