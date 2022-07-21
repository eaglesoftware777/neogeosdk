#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
#!/usr/bin/env python2
# -*- coding: utf-8 -*-
"""
Created on Tue Oct 23 02:14:02 2018

@author: eagle software
"""

import sqlite3
from sqlite3 import Error
try:
    conn = sqlite3.connect("neorom.db")
    print(sqlite3.version)
    cur = conn.cursor()
    cur.execute("create table image (idx int, data array,palette array)")
    cur.execute("create table imagefix (idx int, data array,palette array)")
    conn.commit()
except Error as e:
    print(e)
finally:
    conn.close()
