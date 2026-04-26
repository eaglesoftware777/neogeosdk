#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Oct 23 02:14:02 2018

@author: eagle software
"""

try:
    import pysqlite3 as sqlite3
    from pysqlite3 import Error
except ImportError:
    import sqlite3
    from sqlite3 import Error

try:
    conn = sqlite3.connect("neorom.db")
    print(sqlite3.sqlite_version)
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA synchronous=NORMAL")
    conn.execute("CREATE TABLE IF NOT EXISTS image (idx INTEGER, data BLOB, palette BLOB)")
    conn.execute("CREATE TABLE IF NOT EXISTS imagefix (idx INTEGER, data BLOB, palette BLOB)")
    conn.commit()
except Error as e:
    print(e)
finally:
    conn.close()
