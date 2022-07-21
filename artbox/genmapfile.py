#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
#!/usr/bin/env python2
# -*- coding: utf-8 -*-
"""
Created on Mon Oct 22 19:56:57 2018

@author: eagle software
"""
import sys
import numpy as np
#aa = os.
#print aa
sz = 16
img_count = int(sys.argv[1:][0])
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
         
with open('map','wt') as fmap:
    for l in L:
        for j in range(sz):
            for i in range(sz):
                fmap.write("%x "%l[i,j])
            fmap.write("\n")



       
    

