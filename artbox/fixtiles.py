#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
#!/usr/bin/env python2
# -*- coding: utf-8 -*-
"""
Created on Fri Sep 14 15:12:19 2018

@author: eagle software
"""

import numpy as np
import os
import struct as st
#import cv2
#from PIL import Image
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
def  fixbyte(lp,rp):

    b0 = lp >> 0 & 1
    b1 = lp >> 1 & 1
    b2 = lp >> 2 & 1
    b3 = lp >> 3 & 1
    b4 = rp >> 0 & 1 
    b5 = rp >> 1 & 1
    b6 = rp >> 2 & 1
    b7 = rp >> 3 & 1
    fixb = np.uint8( b7 << 7 | b6 << 6 | b5 << 5 |  b4 << 4 | b3 << 3 | b2 << 2 | b1 << 1 | b0 )
    print("%x"%fixb)
    print "++++++++++++++++++++++"
    return fixb

def writeBLK(crt,f_s1rom,f_s2rom,f_fix):
    C00_1 = np.uint8(crt[0,4])
    C00_2 = np.uint8(crt[0,5])
    C00 = fixbyte(C00_1,C00_2)
    C01_1 = np.uint8(crt[1,4])
    C01_2 = np.uint8(crt[1,5])
    C01 = fixbyte(C01_1,C01_2)
    C02_1 = np.uint8(crt[2,4])
    C02_2 = np.uint8(crt[2,5])
    C02 = fixbyte(C02_1,C02_2)
    C03_1 = np.uint8(crt[3,4])
    C03_2 = np.uint8(crt[3,5])
    C03 = fixbyte(C03_1,C03_2)
    C04_1 = np.uint8(crt[4,4])
    C04_2 = np.uint8(crt[4,5])
    C04 =  fixbyte(C04_1,C04_2)
    C05_1 = np.uint8(crt[5,4])
    C05_2 = np.uint8(crt[5,5])
    C05 = fixbyte(C05_1,C05_2)
    C06_1 = np.uint8(crt[6,4])
    C06_2 = np.uint8(crt[6,5])
    C06 = fixbyte(C06_1,C06_2)
    C07_1 = np.uint8(crt[7,4])
    C07_2 = np.uint8(crt[7,5])
    C07 = fixbyte(C07_1,C07_2)
    C08_1 = np.uint8(crt[0,6])
    C08_2 = np.uint8(crt[0,7])
    C08 = fixbyte(C08_1,C08_2)
    C09_1 = np.uint8(crt[1,6])
    C09_2 = np.uint8(crt[1,7])
    C09 = fixbyte(C09_1,C09_2)
    C0A_1 = np.uint8(crt[2,6])
    C0A_2 = np.uint8(crt[2,7])
    C0A = fixbyte(C0A_1,C0A_2)
    C0B_1 = np.uint8(crt[3,6])
    C0B_2 = np.uint8(crt[3,7])
    C0B = fixbyte(C0B_1,C0B_2)
    C0C_1 = np.uint8(crt[4,6])
    C0C_2 = np.uint8(crt[4,7])
    C0C =  fixbyte(C0C_1,C0C_2)
    C0D_1 = np.uint8(crt[5,6])
    C0D_2 = np.uint8(crt[5,7])
    C0D = fixbyte(C0D_1,C0D_2)
    C0E_1 = np.uint8(crt[6,6])
    C0E_2 = np.uint8(crt[6,7])
    C0E = fixbyte(C0E_1,C0E_2)
    C0F_1 = np.uint8(crt[7,6])
    C0F_2 = np.uint8(crt[7,7])
    C0F = fixbyte(C0F_1,C0F_2)
    C10_1 = np.uint8(crt[0,0])
    C10_2 = np.uint8(crt[0,1])
    C10 = fixbyte(C10_1,C10_2)
    C11_1 = np.uint8(crt[1,0])
    C11_2 = np.uint8(crt[1,1])
    C11 =  fixbyte(C11_1,C11_2)
    C12_1 = np.uint8(crt[2,0])
    C12_2 = np.uint8(crt[2,1])
    C12 = fixbyte(C12_1,C12_2)
    C13_1 = np.uint8(crt[3,0])
    C13_2 = np.uint8(crt[3,1])
    C13 = fixbyte(C13_1,C13_2)
    C14_1 = np.uint8(crt[4,0])
    C14_2 = np.uint8(crt[4,1])
    C14 = fixbyte(C14_1,C14_2)
    C15_1 = np.uint8(crt[5,0])
    C15_2 = np.uint8(crt[5,1])
    C15 =  fixbyte(C15_1,C15_2)
    C16_1 = np.uint8(crt[6,0])
    C16_2 = np.uint8(crt[6,1])
    C16 = fixbyte(C16_1,C16_2)
    C17_1 = np.uint8(crt[7,0])
    C17_2 = np.uint8(crt[7,1])
    C17 =  fixbyte(C17_1,C17_2)
    C18_1 = np.uint8(crt[0,2])
    C18_2 = np.uint8(crt[0,3])
    C18 = fixbyte(C18_1,C18_2)
    C19_1 = np.uint8(crt[1,2])
    C19_2 = np.uint8(crt[1,3])
    C19 = fixbyte(C19_1, C19_2)
    C1A_1 = np.uint8(crt[2,2])
    C1A_2 = np.uint8(crt[2,3])
    C1A = fixbyte(C1A_1,C1A_2)
    C1B_1 = np.uint8(crt[3,2])
    C1B_2 = np.uint8(crt[3,3])
    C1B = fixbyte(C1B_1,C1B_2)
    C1C_1 = np.uint8(crt[4,2])
    C1C_2 = np.uint8(crt[4,3])
    C1C = fixbyte(C1C_1,C1C_2)
    C1D_1 = np.uint8(crt[5,2])
    C1D_2 = np.uint8(crt[5,3])
    C1D =  fixbyte(C1D_1,C1D_2)
    C1E_1 = np.uint8(crt[6,2])
    C1E_2 = np.uint8(crt[6,3])
    C1E =  fixbyte(C1E_1,C1E_2)
    C1F_1 = np.uint8(crt[7,2])
    C1F_2 = np.uint8(crt[7,3])
    C1F = fixbyte(C1F_1,C1F_2)
 
    f_s1rom.write(st.pack('32B',C01,C00,C03,C02,C05,C04,C07,C06,C09,C08,C0B,C0A,C0D
                          ,C0C,C0F,C0E,C11,C10,C13,C12,C15,C14,C17,C16,C19,C18,C1B,C1A,C1D,C1C,C1F,C1E))
    f_s1rom.flush()

#def writeBLK(crt,f_s1rom,f_s2rom,f_fix):
#    C00_1 = np.uint8(crt[0,4])
#    C00_2 = np.uint8(crt[0,5])
#    C00 = fixbyte(C00_1,C00_2)
#    C01_1 = np.uint8(crt[1,4])
#    C01_2 = np.uint8(crt[1,5])
#    C01 = fixbyte(C01_1,C01_2)
#    C02_1 = np.uint8(crt[2,4])
#    C02_2 = np.uint8(crt[2,5])
#    C02 = fixbyte(C02_1,C02_2)
#    C03_1 = np.uint8(crt[3,4])
#    C03_2 = np.uint8(crt[3,5])
#    C03 = fixbyte(C03_1,C03_2)
#    C04_1 = np.uint8(crt[4,4])
#    C04_2 = np.uint8(crt[4,5])
#    C04 =  fixbyte(C04_1,C04_2)
#    C05_1 = np.uint8(crt[5,4])
#    C05_2 = np.uint8(crt[5,5])
#    C05 = fixbyte(C05_1,C05_2)
#    C06_1 = np.uint8(crt[6,4])
#    C06_2 = np.uint8(crt[6,5])
#    C06 = fixbyte(C06_1,C06_2)
#    C07_1 = np.uint8(crt[7,4])
#    C07_2 = np.uint8(crt[7,5])
#    C07 = fixbyte(C07_1,C07_2)
#    C08_1 = np.uint8(crt[0,6])
#    C08_2 = np.uint8(crt[0,7])
#    C08 = fixbyte(C08_1,C08_2)
#    C09_1 = np.uint8(crt[1,6])
#    C09_2 = np.uint8(crt[1,7])
#    C09 = fixbyte(C09_1,C09_2)
#    C0A_1 = np.uint8(crt[2,6])
#    C0A_2 = np.uint8(crt[2,7])
#    C0A = fixbyte(C0A_1,C0A_2)
#    C0B_1 = np.uint8(crt[3,6])
#    C0B_2 = np.uint8(crt[3,7])
#    C0B = fixbyte(C0B_1,C0B_2)
#    C0C_1 = np.uint8(crt[4,6])
#    C0C_2 = np.uint8(crt[4,7])
#    C0C =  fixbyte(C0C_1,C0C_2)
#    C0D_1 = np.uint8(crt[5,6])
#    C0D_2 = np.uint8(crt[5,7])
#    C0D = fixbyte(C0D_1,C0D_2)
#    C0E_1 = np.uint8(crt[6,6])
#    C0E_2 = np.uint8(crt[6,7])
#    C0E = fixbyte(C0E_1,C0E_2)
#    C0F_1 = np.uint8(crt[7,6])
#    C0F_2 = np.uint8(crt[7,7])
#    C0F = fixbyte(C0F_1,C0F_2)
#    C10_1 = np.uint8(crt[0,0])
#    C10_2 = np.uint8(crt[0,1])
#    C10 = fixbyte(C10_1,C10_2)
#    C11_1 = np.uint8(crt[1,0])
#    C11_2 = np.uint8(crt[1,1])
#    C11 =  fixbyte(C11_1,C11_2)
#    C12_1 = np.uint8(crt[2,0])
#    C12_2 = np.uint8(crt[2,1])
#    C12 = fixbyte(C12_1,C12_2)
#    C13_1 = np.uint8(crt[3,0])
#    C13_2 = np.uint8(crt[3,1])
#    C13 = fixbyte(C13_1,C13_2)
#    C14_1 = np.uint8(crt[4,0])
#    C14_2 = np.uint8(crt[4,1])
#    C14 = fixbyte(C14_1,C14_2)
#    C15_1 = np.uint8(crt[5,0])
#    C15_2 = np.uint8(crt[5,1])
#    C15 =  fixbyte(C15_1,C15_2)
#    C16_1 = np.uint8(crt[6,0])
#    C16_2 = np.uint8(crt[6,1])
#    C16 = fixbyte(C16_1,C16_2)
#    C17_1 = np.uint8(crt[7,0])
#    C17_2 = np.uint8(crt[7,1])
#    C17 =  fixbyte(C17_1,C17_2)
#    C18_1 = np.uint8(crt[0,2])
#    C18_2 = np.uint8(crt[0,3])
#    C18 = fixbyte(C18_1,C18_2)
#    C19_1 = np.uint8(crt[1,2])
#    C19_2 = np.uint8(crt[1,3])
#    C19 = fixbyte(C19_1, C19_2)
#    C1A_1 = np.uint8(crt[2,2])
#    C1A_2 = np.uint8(crt[2,3])
#    C1A = fixbyte(C1A_1,C1A_2)
#    C1B_1 = np.uint8(crt[3,2])
#    C1B_2 = np.uint8(crt[3,3])
#    C1B = fixbyte(C1B_1,C1B_2)
#    C1C_1 = np.uint8(crt[4,2])
#    C1C_2 = np.uint8(crt[4,3])
#    C1C = fixbyte(C1C_1,C1C_2)
#    C1D_1 = np.uint8(crt[5,2])
#    C1D_2 = np.uint8(crt[5,3])
#    C1D =  fixbyte(C1D_1,C1D_2)
#    C1E_1 = np.uint8(crt[6,2])
#    C1E_2 = np.uint8(crt[6,3])
#    C1E =  fixbyte(C1E_1,C1E_2)
#    C1F_1 = np.uint8(crt[7,2])
#    C1F_2 = np.uint8(crt[7,3])
#    C1F = fixbyte(C1F_1,C1F_2)
# 
#    f_s1rom.write(st.pack('32B',C01,C00,C03,C02,C05,C04,C07,C06,C09,C08,C0B,C0A,C0D
#                          ,C0C,C0F,C0E,C11,C10,C13,C12,C15,C14,C17,C16,C19,C18,C1B,C1A,C1D,C1C,C1F,C1E))
#    f_s1rom.flush()
##    os.fsync(f_s1rom)


try:
    os.system("rm ./tiles/fix_*")
except OSError:
    pass
try:
    os.system("rm ./tiles_dth/fix_dth_*")
except OSError:
    pass
try:
    os.system("rm *.bin")
except OSError:
    pass
try:
    os.system("rm *.s1 *.s2")
except OSError:
    pass
try:
    os.system("rm s1.txt")
except OSError:
    pass
try:
    os.system("rm s2.txt")
except OSError:
    pass
try:
    os.system("rm neofix.pal")
except OSError:
    pass
try:
    os.system("rm stdfix.pal")
except OSError:
    pass
try:
    os.system("rm neo.fix")
except OSError:
    pass


f_s1rom = open("./1c.s1","wb")
data = []
try:
    db.register_adapter(np.ndarray, adapt_array)
    db.register_converter("array", convert_array)
    conn = db.connect("neorom.db", detect_types=db.PARSE_DECLTYPES)
    cur = conn.cursor()
    cur.execute("select idx,data,palette from imagefix")
    data = cur.fetchall()
except Error as e:
    print(e)
finally:
    conn.close()
if len(data) <= 0 :
    print("Error : len(data) = 0 ")
    exit()
#img = cv2.imread("raw15.png")
#im = Image.open("chr0.png") # Replace with your image name here
#indexed = np.array(im) # Convert to NumPy array to easier access
#img1 = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
#h = indexed.shape[0]
#w = indexed.shape[1]
#img0 = np.uint8(np.zeros((h,w)))
#for i in range(h):
#    for j in range(w):
#        if img1[i,j][0] == 255 :
#            img0[i,j] = 0
#        else :
#            img0[i,j] = 1
#        
#print img0.shape
#h, w = indexed.shape[:2]
#snumber = w /8
#print snumber
#scnumber = h /8
#print scnumber
#tc =  snumber * scnumber
#sprts = np.uint8(np.vsplit(indexed,snumber))
#scount = 0
#crcount = 0

image_number = len(data)
#buff = bytearray(bsz*(image_number+1))
#st.pack_into('i16Q',buff,0, 0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0)

for i in range(image_number):
    print i
    
    indexed = data[i][1]
    palettep = data[i][2]
    h, w = indexed.shape[:2]

    snumber = w /16
    #print snumber
    scnumber = h / 16
    print snumber
    print scnumber
    #print scnumber
    tc =  snumber * scnumber
    sprts = np.uint8(np.vsplit(indexed,snumber))
    scount = 0
    crcount = 0
    
    for sprt in sprts:
        scount = scount + 1
        #print scount
        crts = np.uint8(np.hsplit(sprt,scnumber))
        for crt in crts:
            crcount = crcount + 1
            writeBLK(crt,f_s1rom,None,None)

#f_s1rom.flush()
os.fsync(f_s1rom)
f_s1rom.close()




#try:
#    os.system("./romtool /p 1c.s1 1p.s1 2048 0")
#except OSError:
#    pass

#try:
#    os.system("./romtool /f 1p.s1 052-s1.s1")
#except OSError:
#    pass

#try:
#    os.system("cp 052-s1.s1 ~/neogeosdk/roms")
#except OSError:
#    pass

