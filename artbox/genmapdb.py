import struct
import sys
from sqlite3 import Error
import sqlite3 as db
import io
import numpy as np
def adapt_array(arr):

    out = io.BytesIO()
    np.save(out, arr)
    out.seek(0)
    return db.Binary(out.read())

def convert_array(text):
    out = io.BytesIO(text)
    out.seek(0)
    return np.load(out)

def getpal(image_index):
    i = bsz*image_index + 8
    #print i
    j = i+ bsz - 8 
    
    buff = buffi[i:j]
    #print buff
    pal= struct.unpack('16Q',buff)
    
    return pal
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
bsz = 136
buffi = bytearray()
try:
    bfile =  open("neopal.bin", "rb")
    bfile.seek(0)
    buffi  = bfile.read()
finally:
    bfile.close()  
print len(buffi)

  

lsprts  =  []

with open('map', 'r') as fl:
    lines = fl.readlines()
    
for  line in lines:
        lsprts.append(line.split())



sys.stdout = open('output1.txt','wt')



#counter = 0
#counter1 = 0
#for sprts in lsprts:
#    scnumber =	len(sprts)
#    #print ("pal > scnumber = %d" % scnumber)
#    for i in range(scnumber):
#       #print ("pal > i = %d" % i)
#        pal_index = int(sprts[i])
#        #print ("pal > spal_index= %d" % pal_index)
#
#       #print ("pal > i = %d" % i)
#        counter = counter + 1
#        pal = getpal(pal_index)
#        print("uint16_t  pal_tile%d[16];" % counter)
#        print("setpal_tile(pal_tile%d,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x);" 
#              % (counter,pal[0],pal[1],pal[2],pal[3],pal[4],pal[5],pal[6],pal[7],pal[8],pal[9],pal[10],pal[11],pal[12],pal[13],pal[14],pal[15]))       
#
#        for j in range(16):
#            print("pal_tile%d[%d]  = 0x%x;" %( counter,j, pal[j]))
counter = 0
#counter1 = 0            
for pal_index in range(image_number):
    counter = counter + 1
    pal = getpal(pal_index+1)
    print("uint16_t  pal_screen%d[16];" % counter)
    print("setpal_screen(pal_screen%d,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x);" 
             % (counter,pal[0],pal[1],pal[2],pal[3],pal[4],pal[5],pal[6],pal[7],pal[8],pal[9],pal[10],pal[11],pal[12],pal[13],pal[14],pal[15]))           
    for j in range(16):
       print("pal_screen%d[%d]  = 0x%x;" %( counter,j, pal[j]))

            
                
snumber = len(lsprts)
scnumber = len(lsprts[0])
counter0 = 0
counter1 = 0
counter2 = 1
print("/*************************** screen number : 0 ***************************/")

for sprts in lsprts:
    scnumber =	len(sprts)
    #print ("map > scnumber = %d" % scnumber)
    counter0 = counter0 + 1
    print("uint16_t spriteMap%d[%d];"% (counter0,scnumber))
    for i in range(scnumber):
        print("spriteMap%d[%d] = 0x%s;" %( counter0,i, sprts[i]))
        counter1 = counter1 + 1
    if(counter0==16):
        counter0 = 0
        counter1 = 0
        print("/*************************** screen number : %d ***************************/" % counter2)
        counter2 = counter2 + 1
#tcnumber = counter1
print("load_palettes(pal_tile0,PALETTES);")
for p in range(image_number):
    o = p + 1
    print("load_palettes(pal_tile%d,PALETTES+16*16*%d);" % (o, o))

#
j=0;
for i in range(snumber):
    g = i + 1  
    print("uint16_t spritePals%d[%d];" % (g,scnumber))
    for n in range(scnumber) : 
        l =i+1# n + j + 1             
        print("spritePals%d[%d] = setPal(%d,0,0,0,0,0);" % (g,n,l))
    j =  l 
    
x = 16
y = 496-10
for sp in range(snumber):
    e = sp
    f = sp + 1 
    print("SCB2    = setSCB2(0xF,0xFF);")
    if (sp == 0):
        print("SCB3    = setSCB3(%d,0,%d);" % (y,13)) #496-30
    else:
        print("SCB3    = setSCB3(%d,1,%d);" % (y,13)) #496-30  
    print("SCB4    = setSCB4(%d);" % x)
    print("setBACKDROP(BLACK);")
    print("vram_sprite(64*%d,1,%d,spriteMap%d,spritePals1,%d,SCB2,SCB3,SCB4);" % (e,e,f,scnumber))
    x = x+16 
    #if x == 16 * 16 :
    #    y = y - 16
    #    x = 0
        
#snumber = len(lsprts)
#scnumber = len(lsprts[0])
#    
#j=0;
#for i in range(snumber):
#    g = i + 1  
#    #print("uint16_t spriteMap%d[30] = {" % g)
#    print("uint16_t spriteMap%d[%d];"% (g,scnumber))
#    for n in range(scnumber) : 
#        l = n + j + 1             
#        #if n == 29 :
#        print("spriteMap%d[%d] = 0x%x;" %( g,n, l))
#        j =  l 
#        #else :                
#            #print("0x%x," % l)

#print("load_palettes(pal_screen0,PALETTES);")
#for p in range(image_number):
#    o = p + 1
#    print("load_palettes(pal_screen%d,PALETTES+16*16*%d);" % (o, o))
#
#j=0;
#for i in range(snumber):
#    g = i + 1  
#    print("uint16_t spritePals%d[%d];" % (g,scnumber))
#    for n in range(16) : 
#        l = i+1#n + j + 1             
#        print("spritePals%d[%d] = setPal(%d,0,0,0,0,0);" % (g,n,l))
#    j =  l 




#x = 0
#y = 496-0
#for sp in range(snumber):
#    e = sp
#    f = sp + 1 
#    print("SCB2    = setSCB2(0xF,0xFF);")
#    if (sp == 0):
#        print("SCB3    = setSCB3(%d,0,%d);" % (y,13)) #496-30
#    else:
#        print("SCB3    = setSCB3(%d,1,%d);" % (y,13)) #496-30  
#    print("SCB4    = setSCB4(%d);" % x)
#    print("setBACKDROP(BLACK);")
#    print("vram_sprite(64*%d,1,%d,spriteMap%d,spritePals%d,%d,SCB2,SCB3,SCB4);" % (e,e,f,f,scnumber))
#    x = x + 16 * 16 
#    if x == 16 * 16 :
#        y = y - 16
#        x = 0
        

