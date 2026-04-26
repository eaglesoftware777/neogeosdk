#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Oct 23 02:07:17 2018

@author: eagle software
"""
import sys
try:
    import pysqlite3 as sqlite3
    from pysqlite3 import Error
except ImportError:
    import sqlite3
    from sqlite3 import Error
import numpy as np
import png
import io

#def adapt_array(arr):
#    out = io.BytesIO()
#    np.save(out, arr)
#    out.seek(0)
#    a = out.read()
#    return buffer(a)
#
#def convert_array(text):
#    out = io.BytesIO(text)
#    out.seek(0)
#    return np.load(out)
def adapt_array(arr):
    out = io.BytesIO()
    np.save(out, arr)
    out.seek(0)
    return sqlite3.Binary(out.read())

def convert_array(text):
    out = io.BytesIO(text)
    out.seek(0)
    return np.load(out)
sz = 256
image_number = int(sys.argv[1:][0])

try:
    conn = sqlite3.connect("neorom.db", detect_types=sqlite3.PARSE_DECLTYPES)
    print(sqlite3.sqlite_version)
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA synchronous=NORMAL")
    sqlite3.register_adapter(np.ndarray, adapt_array)
    sqlite3.register_converter("array", convert_array)
    cur = conn.cursor()
    rows = []
    for i in range(image_number):
        im = png.Reader("in/%d.png"%i)
        w, h, imap1, metadata = im.read()
        palettep = np.array(metadata['palette'],dtype=np.uint16)
        indexed = np.vstack(list(map(np.uint16, imap1)))
        rows.append((i, indexed, palettep))
    cur.executemany("INSERT INTO image (idx,data,palette) VALUES (?,?,?)", rows)
    conn.commit()
except Error as e:
    print(e)
finally:
    conn.close()
