#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
#!/usr/bin/env python2
# -*- coding: utf-8 -*-
"""
Created on Tue Oct 23 02:07:17 2018

@author: eagle software
"""
import sys
import sqlite3
import numpy as np
import png
import io
import itertools
from sqlite3 import Error

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
    print(sqlite3.version)
    A=np.ndarray((sz,sz),np.uint16)
    sqlite3.register_adapter(np.ndarray, adapt_array)
    sqlite3.register_converter("array", convert_array)
    cur = conn.cursor()
    for i in range(image_number):
        im = png.Reader("in/%d.png"%i)
        #    im = Image.open("in/%d.png"%i) # Replace with your image name here
        #    indexed = np.array(im) # Convert to NumPy array to easier access
        w, h, imap1, metadata = im.read()
        palettep = np.array(metadata['palette'],dtype=np.uint16)
        indexed = np.vstack(itertools.imap(np.uint16, imap1))
        cur.execute("insert into image (idx,data,palette) values (?, ?, ?)", (i, indexed,palettep))
        conn.commit()
except Error as e:
    print(e)
finally:
    conn.close()
