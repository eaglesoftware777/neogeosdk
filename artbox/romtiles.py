#!/usr/bin/env python2
# -*- coding: utf-8 -*-
"""
Created on Fri Sep 14 15:12:19 2018

@author: eagle software
"""
#import cv2
import struct as st
#import png
import numpy as np
#import itertools
import os
from sqlite3 import Error
import sqlite3 as db
import io
bsz = 136


def adapt_array(arr):

    out = io.BytesIO()
    np.save(out, arr)
    out.seek(0)
    return db.Binary(out.read())

def convert_array(text):
    out = io.BytesIO(text)
    out.seek(0)
    return np.load(out)


def writePLT(palettep,f_std,f_neogeo,image_index,palbuff):
    pal = [0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0]
    print ("***************  %s ******************" % image_index)
    pal[0] = 0x0
    pal[1] = 0x0
    pal[2] = 0x0
    pal[3] = 0x0
    pal[4] = 0x0
    pal[5] = 0x0
    pal[6] = 0x0
    pal[7] = 0x0
    pal[8] = 0x0
    pal[9] = 0x0
    pal[10] = 0x0
    pal[11] = 0x0
    pal[12] = 0x0
    pal[13] = 0x0
    pal[14] = 0x0
    pal[15] = 0x0
    sz = len(palettep)-1
    print sz
    print ("0x%x"%pal[0])
    for i in range(sz):
        color24_ = palettep[i]
        r24 = color24_[0]
        g24 = color24_[1]
        b24 = color24_[2]
        color24 = np.uint32((r24 << 16) | (g24 << 8)  | (b24)) 
        r16 = (color24 & 0xF80000) >> 19
        g16 = (color24 & 0x00F800)  >> 12
        b16 = (color24 & 0x0000F8) >> 3 
        color16 =  np.uint16( r16 << 10 | g16 << 5 | b16)
        color16.tofile(f_std)
        d0  = 0
        b4 = b16 >> 0 & 1
        b3 = b16 >> 1 & 1
        b2 = b16 >> 2 & 1
        b1 = b16 >> 3 & 1
        b0 = b16 >> 4 & 1
        g4 = g16 >> 0 & 1
        g3 = g16 >> 1 & 1
        g2 = g16 >> 2 & 1
        g1 = g16 >> 3 & 1
        g0 = g16 >> 4 & 1
        r4 = r16 >> 0 & 1
        r3 = r16 >> 1 & 1
        r2 = r16 >> 2 & 1
        r1 = r16 >> 3 & 1
        r0 = r16 >> 4 & 1
        color_neogeo16 = d0 << 15 | r4 << 14 | g4  << 13 | b4  << 12 | r0 << 11 | r1 << 10 | r2 << 9 | r3 << 8 | g0 << 7 | g1 << 6 | g2 << 5 | g3 << 4 | b0 << 3 | b1 << 2 | b2 << 1 | b3 
        color_neogeo16.tofile(f_neogeo)
        
        pal[i+1] = color_neogeo16
        print ("0x%x" % pal[i+1])
    st.pack_into('i16Q',palbuff, bsz*image_index, image_index, pal[0] , pal[1] , pal[2] , pal[3] , pal[4] , pal[5] , pal[6] , pal[7] , pal[8] , pal[9] , pal[10] , pal[11] , pal[12] , pal[13] , pal[14] , pal[15])

def writeBLK(BLK_,blk,f_c1rom,f_c2rom,f_spr):
	for i in range(8):
		BLK = BLK_[i, :]
		BLK_1 = BLK[0] + 1
		BPD1 = BLK_1 >> 0 & 1 
		BPC1 = BLK_1 >> 1 & 1 
		BPB1 = BLK_1 >> 2 & 1 
		BPA1 = BLK_1 >> 3 & 1 
		BLK_2 = BLK[1] + 1
		BPD2 = BLK_2 >> 0 & 1 
		BPC2 = BLK_2 >> 1 & 1 
		BPB2 = BLK_2 >> 2 & 1 
		BPA2 = BLK_2 >> 3 & 1 
		BLK_3 = BLK[2] + 1 
		BPD3 = BLK_3 >> 0 & 1 
		BPC3 = BLK_3 >> 1 & 1 
		BPB3 = BLK_3 >> 2 & 1 
		BPA3 = BLK_3 >> 3 & 1 
		BLK_4 = BLK[3] + 1
		BPD4 = BLK_4 >> 0 & 1 
		BPC4 = BLK_4 >> 1 & 1 
		BPB4 = BLK_4 >> 2 & 1 
		BPA4 = BLK_4 >> 3 & 1 
		BLK_5 = BLK[4] + 1
		BPD5 = BLK_5 >> 0 & 1 
		BPC5 = BLK_5 >> 1 & 1 
		BPB5 = BLK_5 >> 2 & 1 
		BPA5 = BLK_5 >> 3 & 1 
		BLK_6 = BLK[5] + 1
		BPD6 = BLK_6 >> 0 & 1 
		BPC6 = BLK_6 >> 1 & 1 
		BPB6 = BLK_6 >> 2 & 1 
		BPA6 = BLK_6 >> 3 & 1 
		BLK_7 = BLK[6] + 1
		BPD7 = BLK_7 >> 0 & 1 
		BPC7 = BLK_7 >> 1 & 1 
		BPB7 = BLK_7 >> 2 & 1 
		BPA7 = BLK_7 >> 3 & 1 
		BLK_8 = BLK[7] + 1
		BPD8 = BLK_8 >> 0 & 1 
		BPC8 = BLK_8 >> 1 & 1 
		BPB8 = BLK_8 >> 2 & 1 
		BPA8 = BLK_8 >> 3 & 1 
		BPD = np.uint8( BPA8 << 7 | BPA7 << 6 | BPA6 << 5 |  BPA5 << 4 | BPA4 << 3 | BPA3 << 2 | BPA2 << 1 | BPA1 )
		BPC = np.uint8( BPB8 << 7 | BPB7 << 6 | BPB6 << 5 |  BPB5 << 4 | BPB4 << 3 | BPB3 << 2 | BPB2 << 1 | BPB1 )
		BPB = np.uint8( BPC8 << 7 | BPC7 << 6 | BPC6 << 5 |  BPC5 << 4 | BPC4 << 3 | BPC3 << 2 | BPC2 << 1 | BPC1 )
		BPA = np.uint8( BPD8 << 7 | BPD7 << 6 | BPD6 << 5 |  BPD5 << 4 | BPD4 << 3 | BPD3 << 2 | BPD2 << 1 | BPD1 )
		f_c1rom.write(st.pack("2B",BPB,BPA))
		f_c1rom.flush()
		f_c2rom.write(st.pack("2B",BPD,BPC))
		f_c2rom.flush()

try:
	os.system("rm neopal.bin")
except OSError:
    pass


try:
	os.system("rm *.c1 *.c2")
except OSError:
    pass

try:
	os.system("rm neo.pal")
except OSError:
    pass
try:
	os.system("rm std.pal")
except OSError:
    pass
try:
	os.system("rm neo.spr")
except OSError:
    pass


f_c1rom = open("1c.c1","a+b")
f_c2rom = open("2c.c2","a+b")
f_std = open("std.pal","a+b")
f_neo = open("neo.pal","a+b")
f_spr = open("neo.spr","a+b")
data = []
try:
    db.register_adapter(np.ndarray, adapt_array)
    db.register_converter("array", convert_array)
    conn = db.connect("neorom.db", detect_types=db.PARSE_DECLTYPES)
    cur = conn.cursor()
    cur.execute("select idx,data,palette from image")
    data = cur.fetchall()
except Error as e:
    print(e)
finally:
    conn.close()
if len(data) <= 0 :
    print("Error : len(data) = 0 ")
    exit()

image_number = len(data)
buff = bytearray(bsz*(image_number+1))
st.pack_into('i16Q',buff,0, 0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0)

for i in range(image_number):
    #im = png.Reader("in/%d.png"%i)
#    im = Image.open("in/%d.png"%i) # Replace with your image name here
#    indexed = np.array(im) # Convert to NumPy array to easier access
    #w, h, imap1, metadata = im.read()
    #palettep = metadata['palette']
    #indexed = np.vstack(itertools.imap(np.uint16, imap1))
   # print i
    indexed = data[i][1]
    palettep = data[i][2]
    h, w = indexed.shape[:2]
    snumber = w /16
    #print snumber
    scnumber = h / 16
    #print scnumber
    tc =  snumber * scnumber
    sprts = np.uint8(np.vsplit(indexed,snumber))
    scount = 0
    crcount = 0
    for sprt in sprts:
        scount = scount + 1
    #print scount
        crts = np.hsplit(sprt,scnumber)
        for crt in crts:
            crcount = crcount + 1
            #print crcount
            BLK3 = crt[0:8, 0:8]
            BLK4 = crt[8:16, 0:8]
            BLK1 = crt[0:8, 8:16]
            BLK2 = crt[8:16, 8:16]
            writeBLK(BLK1,1,f_c1rom,f_c2rom,f_spr)
            writeBLK(BLK2,2,f_c1rom,f_c2rom,f_spr)
            writeBLK(BLK3,3,f_c1rom,f_c2rom,f_spr)
            writeBLK(BLK4,4,f_c1rom,f_c2rom,f_spr)
    writePLT(palettep,f_std,f_neo,i+1,buff)
os.fsync(f_c1rom)
os.fsync(f_c2rom)
f_c1rom.close()
f_c2rom.close()
f_std.close()
f_neo.close()
f_spr.close()


try:
    bfile =  open("neopal.bin", "wb")
    bfile.write(buff)
finally:
    bfile.close()  


    
#try:
#	os.system("./romtool /p 1c.c1 1p.c1 2048 0")
#except OSError:
#	pass
#try:
#	os.system("./romtool /p 2c.c2 2p.c2 2048 0")
#except OSError:
#	pass
#try:
#	os.system("./romtool /f 1p.c1 052-c1.c1")
#except OSError:
#	pass
#try:
#	os.system("./romtool /f 2p.c2 052-c2.c2")
#except OSError:
#	pass
#try:
#	os.system("cp 052-c1.c1 052-c2.c2 ~/neogeosdk/roms")
#except OSError:
#	pass
