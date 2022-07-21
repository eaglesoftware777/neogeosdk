#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
#!/usr/bin/env python2
# -*- coding: utf-8 -*-
"""
Created on Wed Oct 24 01:22:23 2018

@author: eagle software
"""

import struct
import sys
import numpy as np

def getpal(image_index):
    i = bsz*image_index + 8
    #print i
    j = i+ bsz - 8 
    
    buff = buffi[i:j]
    #print buff
    pal= struct.unpack('16Q',buff)
    
    return pal
imgnb =  int(sys.argv[1:][0])
image_screens = range(imgnb)
bsz = 136
buffi = bytearray()
try:
    bfile =  open("neopal.bin", "rb")
    bfile.seek(0)
    buffi  = bfile.read()
finally:
    bfile.close()  

sz = int(sys.argv[1:][1])
img_count = len(image_screens)
mapl1_start = 0
mapl1_end = mapl1_start+sz
L=[]
for count in range(img_count):

    A=np.uint16(np.zeros((sz,sz)))

    for j in range(sz):
            for i in range(sz):
                A[i,:] = np.arange(mapl1_start,mapl1_end,1,dtype=np.uint16)+i*sz
    mapl1_start = A[sz-1,sz-1]+1
    mapl1_end = mapl1_start+sz
    L.append(A)


sys.stdout = open('screens.c','wt') 
pal_sz = 16
sprt_sz =  int(sys.argv[1:][2])
crt_sz =  int(sys.argv[1:][3])
s=""
for image_i0 in image_screens:
    image_index = image_i0+1
    print("")
    print("")
    print("void NEOGEO_USER showScreen%d(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop) {" % image_index)
    print("/****************************************** screen %d ******************************************/" % image_index)
    print("uint16_t  SCB2    = 0x0;")
    print("uint16_t  SCB3    = 0x0;") 
    print("uint16_t  SCB4    = 0x0;")
    print("uint16_t  pal%d[16];" % image_index)
    pal = getpal(image_index)
    print("setpal(pal%d,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x);" 
             % (image_index,pal[0],pal[1],pal[2],pal[3],pal[4],pal[5],pal[6],pal[7],pal[8],pal[9],pal[10],pal[11],pal[12],pal[13],pal[14],pal[15]))           
    # for pal_index in range(pal_sz):
        # print("pal%d[%d]  = 0x%x;" %( image_index,pal_index,pal[pal_index]))
    for sprt_index in range(sprt_sz):
        s="uint16_t spriteMapS%d_%d[%d] = {" %(image_index,sprt_index+1,16)
        for crt_index in range(crt_sz-1):
			s=s+"0x%x,"%L[image_i0][crt_index,sprt_index]
        s=s+"0x%x};"%L[image_i0][crt_sz-1,sprt_index]
        print(s)
        s=""
    print("load_palettes(pal%d,PALETTES+PALOFFSET*%d);" % (image_index,image_index+1))
    print("uint16_t SCB1_2common = setSCB1_2(%d,0,0,0,0,0);" %(image_index+1))
    for sprt_index in range(sprt_sz):
        print("uint16_t spal%d_%d[%d]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};" % (image_index,sprt_index+1,crt_sz))
        # for crt_index in range(crt_sz) : 
            # pal_id =1           
            # print("spal%d_%d[%d] = setPal(%d,0,0,0,0,0);" % (image_index,sprt_index+1,crt_index,pal_id))
    # x0 = 16
    # y0 = 10
    x = "x0"
    y = "496-y0"
    min_crt_sz = "min_crt_sz"#13
    xr = "xr"#"0xF"
    yr = "yr"#"0xCF"
    link = 1
    unlink = 0
    backdrop = "backdrop"
    for sprt_index in range(sprt_sz):
        print("SCB2    = setSCB2(%s,%s);" %(xr,yr))
        if (sprt_index == 0):
            print("SCB3    = setSCB3(%s,%d,%s);" % (y,unlink,min_crt_sz)) 
        else:
            print("SCB3    = setSCB3(%s,%d,%s);" % (y,link,min_crt_sz)) 
        print("SCB4    = setSCB4(%s);" % x)
        print("setBACKDROP(%s);" % backdrop)
        print("vram_sprite(64*%d,1,%d,spriteMapS%d_%d,spal%d_%d,%d,SCB2,SCB3,SCB4);" % (sprt_index,sprt_index,image_index,sprt_index+1,image_index,sprt_index+1,crt_sz))
        x = "x0+16*%d"%(sprt_index+1)
    print("}")
