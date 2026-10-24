"""Native-resolution V5 environment artwork and lossless C-ROM installation.

The drawings use whole-pixel contours and embedded, hardware-rounded palettes.
Only scenery reservations are replaced; cast tiles and frame IDs are immutable.
No character source sheet, geometry, resampling or palette fitting is involved.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import sys

import numpy as np
from PIL import Image, ImageDraw

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / 'artbox'))
from tile_codec import encode_image, decode_image
from palette_banks import palette_words

AUTHORED = GAME / 'assets/authored/v5'
GENERATED = GAME / 'artbox/generated'
COLS = (70, 64, 64, 64, 64, 70, 70, 64, 64, 64, 73, 73)
# Sky, haze, bark/stone, foliage/metal, warm accent, water/light.
THEMES = (
    ('emerald', (31, 73, 83), (105, 151, 132), (135, 98, 63), (66, 122, 67), (233, 193, 105), (100, 199, 190)),
    ('falls', (48, 91, 115), (139, 183, 188), (125, 132, 113), (64, 126, 101), (221, 207, 153), (120, 215, 226)),
    ('coast', (55, 133, 171), (161, 205, 211), (155, 112, 68), (70, 139, 93), (246, 209, 136), (88, 199, 213)),
    ('autumn', (98, 75, 110), (215, 145, 111), (144, 86, 67), (183, 89, 57), (250, 201, 112), (233, 132, 79)),
    ('crystal', (29, 37, 73), (101, 99, 151), (93, 85, 127), (77, 111, 139), (194, 160, 233), (116, 222, 228)),
    ('worldtree', (41, 62, 75), (105, 143, 133), (131, 101, 69), (85, 140, 90), (222, 198, 127), (127, 192, 169)),
    ('foundry', (40, 41, 59), (130, 99, 93), (119, 91, 81), (111, 127, 125), (239, 164, 83), (247, 105, 63)),
    ('reef', (24, 59, 96), (82, 137, 159), (122, 140, 148), (77, 139, 142), (228, 165, 144), (114, 216, 226)),
    ('ice', (39, 58, 102), (137, 173, 207), (112, 139, 174), (125, 175, 195), (209, 197, 236), (184, 238, 244)),
    ('savanna', (102, 91, 115), (221, 178, 139), (169, 120, 83), (126, 139, 88), (246, 213, 134), (224, 178, 111)),
    ('skyroad', (65, 77, 119), (186, 159, 164), (136, 108, 91), (128, 148, 157), (243, 190, 117), (177, 216, 232)),
    ('citadel', (33, 34, 60), (109, 95, 133), (106, 90, 112), (90, 119, 120), (227, 145, 104), (120, 215, 178)),
)


def mix(a, b, amount):
    return tuple(round(x*(1-amount)+y*amount) for x, y in zip(a, b))


def palettes(stage, count=6):
    _, sky, haze, stone, leaf, gold, water = THEMES[stage]
    colors = []
    for bank in range(count):
        fog = (0.27, 0.20, 0.14, 0.07, 0.0, 0.0)[bank] if count==6 else 0.0
        ramp = [(0, 0, 0), (13, 20, 29), mix(stone, (8, 17, 29), .66),
                mix(leaf, sky, .55), mix(stone, (13, 21, 29), .34), stone,
                mix(stone, gold, .38), mix(leaf, (12, 30, 34), .42), leaf,
                mix(leaf, gold, .30), sky, mix(sky, haze, .46), haze,
                gold, water, mix(haze, (255, 247, 218), .80)]
        if bank==4 and stage in (1,2,4,6,7,8,11):
            glow=water if stage not in (6,) else (246,128,59)
            ramp[12:16]=[mix(glow,sky,.55),mix(glow,sky,.25),glow,mix(glow,(255,250,229),.64)]
        for i, rgb in enumerate(ramp):
            rgb = mix(rgb, haze, fog) if i not in (0, 10, 11, 12, 15) else rgb
            # Exact five-bit channels; decoded ROM previews match the source.
            colors.append(tuple(round(round(c*31/255)*255/31) for c in rgb))
    return colors


class Drawing:
    def __init__(self, width, height):
        self.image = Image.new('L', (width, height), 0)
        self.pen = ImageDraw.Draw(self.image)
        self.w, self.h = width, height

    def box(self, box, color): self.pen.rectangle(box, fill=color)
    def line(self, points, color, width=1): self.pen.line(points, fill=color, width=width)
    def poly(self, points, color): self.pen.polygon(points, fill=color)

    def cloud(self, x, y, width=94):
        self.poly([(x,y+12),(x+9,y+12),(x+9,y+6),(x+24,y+6),(x+24,y),
                   (x+42,y),(x+42,y+4),(x+59,y+4),(x+59,y+9),
                   (x+width-8,y+9),(x+width-8,y+14),(x+width,y+14),
                   (x+width,y+18),(x,y+18)],12)
        self.line([(x+12,y+18),(x+width-4,y+18)],11,2)
        self.line([(x+25,y+2),(x+39,y+2)],15)

    def foliage(self, x, y, width, color=8):
        self.poly([(x,y+19),(x+3,y+10),(x+12,y+10),(x+12,y+5),(x+23,y+5),
                   (x+23,y+1),(x+width//2+7,y+1),(x+width//2+7,y+5),
                   (x+width-14,y+5),(x+width-14,y+11),(x+width-4,y+11),
                   (x+width-4,y+18),(x+width,y+18),(x+width,y+29),
                   (x+width-8,y+29),(x+width-8,y+35),(x+width-20,y+35),
                   (x+width-20,y+39),(x+17,y+39),(x+17,y+35),
                   (x+5,y+35),(x+5,y+29),(x,y+29)],7)
        self.poly([(x+4,y+18),(x+12,y+12),(x+23,y+9),(x+width//2,y+5),
                   (x+width-15,y+10),(x+width-6,y+20),(x+width-11,y+31),
                   (x+width-24,y+35),(x+20,y+34),(x+8,y+28)],color)
        for k,(yy,offset) in enumerate(((11,13),(21,6),(30,19))):
            for xx in range(x+offset,x+width-12,23):
                self.poly([(xx,yy+y+3),(xx+3,yy+y),(xx+12,yy+y),
                           (xx+16,yy+y+3),(xx+8,yy+y+2),(xx+6,yy+y+5)],9 if k<2 else 8)
        self.line([(x+23,y+7),(x+width//2+4,y+7)],9,2)
        self.line([(x+7,y+25),(x+15,y+27),(x+11,y+29)],2)

    def tree(self, x, y=67, width=28, crown=True):
        self.poly([(x,y),(x+width-7,y),(x+width-5,166),(x+width+7,191),
                   (x+width-1,191),(x+width-11,178),(x+9,178),(x-2,191),
                   (x-14,191),(x+2,163)],4)
        self.poly([(x+4,y),(x+10,y),(x+10,166),(x+3,180),(x-5,183),
                   (x+5,162)],6)
        self.line([(x+width-8,y+10),(x+width-6,159),(x+width,181)],2,2)
        for a,b in ((y+19,y+34),(y+48,y+60),(y+74,y+85)):
            self.line([(x+12,a),(x+15,a+4),(x+13,b)],2)
        for yy in range(max(8,y+5),171,19):
            self.line([(x+5,yy),(x+8,yy+4),(x+6,yy+10)],5)
            self.line([(x+width-13,yy+7),(x+width-10,yy+2)],6)
        self.poly([(x+width//2,155),(x+width+13,172),(x+width+31,187),
                   (x+width+16,184),(x+width-1,174),(x+width//2,170)],4)
        self.line([(x+width//2+3,158),(x+width+10,174),(x+width+22,181)],6)
        if crown:
            self.foliage(x-41,y-30,99,7)
            self.foliage(x-58,y-12,75)
            self.foliage(x+5,y-17,81)

    def masonry(self, x, y, width, height, color=5):
        self.box((x,y,x+width,y+height),4)
        for row, yy in enumerate(range(y,y+height,12)):
            offset=-(row & 1)*12
            for xx in range(x+offset,x+width,27):
                a,b=max(x,xx+1),min(x+width,xx+25)
                self.box((a,yy+1,b,min(yy+10,y+height)),color)
                self.line([(a,yy+1),(b,yy+1)],6)
                self.line([(b,yy+3),(b,min(yy+10,y+height))],2)

    def pillar(self,x,y,height=80):
        self.box((x,y,x+23,y+height),4)
        self.box((x+4,y+3,x+8,y+height-5),6)
        self.box((x+10,y+3,x+17,y+height-5),5)
        self.box((x-4,y,x+27,y+6),6)
        self.box((x-5,y+height-7,x+28,y+height),4)
        self.line([(x-4,y+height-7),(x+27,y+height-7)],6,2)

    def crystal(self,x,y,height=46,width=18):
        self.poly([(x,y),(x+width//2,y-height),(x+width,y-8),(x+width-3,y),
                   (x+3,y+4)],7)
        self.poly([(x+3,y),(x+width//2,y-height+5),(x+width//2,y+2)],14)
        self.poly([(x+width//2,y-height),(x+width//2+2,y-height+3),
                   (x+width//2+3,y-4),(x+width//2,y)],15)


def indexed(drawing, stage, banks=6, transparent=False):
    shades=np.asarray(drawing.image,dtype=np.uint8)
    h,w=shades.shape
    if banks==6:
        row_bank=np.asarray([min(5,(0 if y<32 else 1 if y<64 else 2 if y<112 else 3 if y<144 else 4 if y<192 else 5)) for y in range(h)])
    else:
        row_bank=np.minimum(banks-1,np.arange(h)//64)
    indices=shades+row_bank[:,None].astype(np.uint8)*16
    if transparent:indices[shades==0]=0
    elif np.any(shades==0):raise ValueError('Opaque environment has unpainted pixels')
    image=Image.fromarray(indices,'P');colors=palettes(stage,banks)
    image.putpalette([c for rgb in colors for c in rgb]+[0]*(768-len(colors)*3))
    image.info['transparency']=0
    return image


def panorama(stage, width):
    d=Drawing(width,224)
    for a,b,c in ((0,31,10),(32,63,11),(64,111,12),(112,143,3),(144,191,7),(192,223,4)):
        d.box((0,a,width-1,b),c)
    # Deliberately uneven skyline silhouettes; no stochastic texture or dithering.
    for x in range(-50,width,268):
        d.poly([(x,132),(x+37,103),(x+63,109),(x+108,66),(x+131,80),
                (x+148,74),(x+205,119),(x+246,104),(x+285,143)],3)
        d.line([(x+65,109),(x+108,66),(x+126,89),(x+138,87)],11,2)
    if stage not in (4,5,6,7,8,11):
        d.pen.ellipse((236,12,261,37),fill=13)
        d.pen.ellipse((239,13,258,32),fill=15)
        for x,y in ((49,18),(159,37),(654,16)):
            d.line([(x,y),(x+4,y+3),(x+8,y)],2)
        for x,y,w in ((72,31,94),(380,19,118),(710,41,76),(998,23,106)):
            d.cloud(x,y,w)
    if stage in (0,3,5):
        # Receding trees, distant woodland floor and a distinct ruin island.
        for x,h in ((66,79),(279,102),(455,70),(740,93),(947,77),(1110,111)):
            d.poly([(x,190),(x+10,190),(x+6,190-h),(x+2,190-h)],3)
            for yy in range(190-h,162,24):d.foliage(x-28,yy-8,65,3)
        for x,y in ((-20,160),(109,146),(291,163),(465,150),(651,155),(872,143),(1048,161)):
            d.foliage(x,y,92,7)
            d.foliage(x+41,y+8,78,8)
        if stage==0:
            d.masonry(259,127,87,64)
            d.masonry(363,157,55,34)
            d.pillar(255,111,80);d.pillar(340,111,80)
            d.box((251,104,367,116),6)
            d.box((282,144,327,191),2)
            d.poly([(278,145),(289,133),(320,133),(332,145)],4)
            d.foliage(250,94,61,7);d.foliage(320,104,63)
            d.line([(297,117),(295,141),(303,155),(299,179)],8,2)
        for x,y,w in ((15,78,23),(180,51,32),(386,80,21),(590,66,35),(832,48,28),(1011,73,34)):
            d.tree(x,y,w)
        if stage==0:
            for x,y in ((91,21),(478,-9),(692,4),(1051,12)):
                d.foliage(x,y,149,7);d.foliage(x+45,y+17,91)
            for x in (117,489,714,1081):
                d.line([(x,36),(x-3,63),(x+3,89),(x-1,116)],7,2)
                for yy in (51,77,100):d.poly([(x-1,yy),(x-9,yy+2),(x-4,yy+8)],8)
        if stage==5:
            for x in (62,485,934):
                d.tree(x,0,105,False)
                d.poly([(x-40,124),(x+39,95),(x+110,101),(x+166,122),(x+165,130),
                        (x+104,111),(x+36,108),(x-40,135)],5)
        if stage==3:
            for x in (235,699):
                d.masonry(x,113,88,73)
                for yy,ww in ((102,118),(121,104)):
                    d.poly([(x-17,yy+9),(x+44,yy-13),(x+ww-17,yy+9),
                            (x+ww-25,yy+12),(x-9,yy+12)],2)
                    d.line([(x-13,yy+8),(x+44,yy-9),(x+ww-22,yy+8)],13,2)
                for xx in range(x+12,x+80,21):d.box((xx,142,xx+7,162),2)
    elif stage==1:
        for x,top,w in ((30,78,110),(251,112,155),(534,64,188),(815,96,103),(1010,80,100)):
            d.masonry(x,top,w,191-top)
            d.foliage(x-7,top-16,w+14)
            d.box((x+w-32,top+8,x+w-8,190),14)
            for xx in (x+w-29,x+w-21,x+w-13):
                d.line([(xx,top+12),(xx+2,top+43),(xx,188)],15)
            d.cloud(x+w-65,169,90)
    elif stage==2:
        d.box((0,145,width,183),14)
        for yy in (148,157,172,181):
            for x in range((yy*5)%49,width,81):d.line([(x,yy),(x+27,yy)],15)
        for x in (32,404,903):
            d.poly([(x,160),(x+24,176),(x+133,176),(x+157,156)],4)
            d.line([(x+70,80),(x+70,161)],2,3)
            d.poly([(x+73,84),(x+73,137),(x+113,137),(x+105,109)],12)
        for x in (218,717):
            d.tree(x,102,14,False)
            for dx,dy in ((-41,-7),(-24,-21),(27,-17),(42,-3)):
                d.poly([(x+6,103),(x+dx,103+dy),(x+dx-5,113+dy),(x+dx+7,109+dy)],8)
    elif stage in (4,8):
        d.poly([(0,0),(width,0),(width,29),(0,24)],2)
        for x,hh in ((18,49),(153,83),(318,37),(477,61),(688,86),(904,41),(1080,67)):
            d.poly([(x,12),(x+24,hh),(x+47,16)],4)
            d.line([(x+9,18),(x+24,hh-5)],6,2)
        for x,h in ((27,62),(135,41),(330,80),(502,43),(681,75),(913,54),(1096,84)):
            d.crystal(x,189,h,26)
            d.crystal(x+28,190,h//2,17)
        for x in (89,276,563,851):
            d.box((x-8,105,x+38,117),4)
            d.line([(x-8,105),(x+38,105)],6,2)
            d.crystal(x,104,61,16);d.crystal(x+18,106,34,12)
        for x,y in ((104,57),(311,96),(762,52),(1002,87)):
            d.line([(x,y),(x+5,y+2),(x+9,y-1),(x+15,y+1)],13)
        if stage==8:
            for x in (198,752):
                d.pillar(x,101,89);d.pillar(x+80,109,81)
                d.poly([(x-4,110),(x+16,75),(x+65,79),(x+107,116),
                        (x+94,121),(x+62,92),(x+24,90),(x+7,120)],6)
    elif stage in (6,11):
        for x in (132,480,936):
            d.box((x,77,x+55,116),4)
            d.box((x+5,83,x+49,114),13)
            d.box((x+9,89,x+45,114),14)
            d.line([(x+13,112),(x+17,100),(x+23,108),(x+28,96),(x+38,112)],15,3)
        for x,h in ((24,107),(173,76),(392,123),(624,82),(833,113),(1046,94)):
            d.masonry(x,190-h,104,h)
            d.box((x+17,66-h//3,x+38,190-h),4)
            d.box((x+13,63-h//3,x+42,69-h//3),6)
            for xx in (x+14,x+43,x+72):
                d.box((xx,201-h,xx+13,216-h),2)
                d.box((xx+2,202-h,xx+10,214-h),13 if stage==6 else 14)
            for yy in (143,168):
                d.line([(x-20,yy),(x+119,yy)],2,5);d.line([(x-20,yy-2),(x+119,yy-2)],6)
        if stage==11:
            d.masonry(464,50,142,140)
            d.poly([(451,60),(535,16),(620,60)],2)
            d.box((519,83,550,136),14)
            d.line([(535,83),(535,136)],4,3);d.line([(520,109),(550,109)],4,3)
    elif stage==7:
        for x,y in ((89,36),(372,62),(651,95),(983,49)):
            for k in range(4):
                xx=x+k*12;yy=y+(k&1)*4
                d.poly([(xx,yy),(xx+5,yy-2),(xx+9,yy),(xx+5,yy+2)],13)
                d.poly([(xx+8,yy),(xx+12,yy-3),(xx+12,yy+3)],8)
        for x in (40,203,507,778,1036):
            d.pillar(x,96+(x%3)*8,90-(x%3)*8)
            for yy in (37,71,122):d.box((x-9,yy,x-7,yy+2),15)
        for x in (120,371,652,925):
            d.line([(x,190),(x-4,163),(x+9,143),(x+5,126)],8,3)
            d.line([(x,173),(x+18,158),(x+21,139)],9,2)
            d.line([(x-2,162),(x-19,147),(x-14,136)],13,2)
    elif stage==9:
        for x,y,w in ((45,121,105),(349,140,138),(765,114,119),(1040,139,96)):
            d.poly([(x,y+27),(x+w//3,y),(x+w,y+19),(x+w,y+62),(x,y+66)],5)
            d.line([(x+4,y+26),(x+w//3,y+3),(x+w-3,y+21)],13,2)
        for x in (191,596,959):
            d.pillar(x,106,82);d.pillar(x+59,120,68)
            d.box((x-7,101,x+92,113),6)
            d.foliage(x-29,83,86,7)
    elif stage==10:
        for x,y in ((57,121),(348,97),(738,128),(1033,92)):
            d.poly([(x,y),(x+98,y-6),(x+131,y+6),(x+98,y+15),(x+17,y+15)],4)
            d.box((x+46,y-25,x+74,y-4),5)
            d.box((x+52,y-30,x+69,y-25),6)
            d.line([(x+87,y),(x+113,y-20)],6,3)
            for xx in range(x+25,x+99,16):d.box((xx,y+5,xx+5,y+8),13)
            d.box((x+4,y+5,x+11,y+8),14)
            d.line([(x+3,y+6),(x-6,y+6)],15,2)
        d.cloud(170,161,151);d.cloud(598,169,138)
    # The floor is a stable, shaded surface with readable contact at y192.
    d.line([(0,192),(width,192)],9 if stage not in (6,8,11) else 6,2)
    d.line([(0,195),(width,195)],2,2)
    for x in range(0,width,32):
        d.line([(x+4,200),(x+21,200),(x+25,204)],5)
        d.line([(x+8,217),(x+13,212),(x+23,212)],2)
        d.line([(x+25,207),(x+29,207)],6)
        if stage in (0,1,3,5,9):
            d.poly([(x+1,193),(x+4,185),(x+6,192),(x+10,188),(x+10,196)],8)
            d.line([(x+14,193),(x+16,201),(x+12,207)],2)
            if x%96==0:
                d.poly([(x+16,190),(x+11,178),(x+19,183),(x+24,169),
                        (x+25,184),(x+32,179),(x+27,191)],9)
        if stage in (6,11):
            d.box((x,196,x+29,217),5)
            d.line([(x,197),(x+28,197),(x+28,216)],6)
            for xx in (x+3,x+25):d.box((xx,200,xx+1,201),13)
    return indexed(d,stage)


def midground(stage):
    d=Drawing(32,112)
    if stage in (0,3,5):
        d.tree(8,-30,18,False);d.foliage(-7,12,43,7);d.foliage(3,38,38)
    elif stage in (1,7,9):d.pillar(5,9,102)
    elif stage in (4,8):d.crystal(2,110,92,26)
    elif stage in (6,11):
        d.box((7,5,24,111),4);d.box((10,7,14,111),6)
        for y in (12,39,76):d.box((3,y,28,y+6),5);d.line([(4,y),(26,y)],6)
    else:
        d.line([(17,6),(17,109)],5,5)
        d.poly([(19,8),(19,56),(31,56),(27,21)],12)
    image=indexed(d,stage,1,True)
    return image


def landmark(stage):
    d=Drawing(192,176)
    if stage in (0,3,5,9):
        d.poly([(44,4),(145,4),(151,126),(179,171),(129,171),(114,153),
                (72,153),(54,173),(10,173),(37,129)],4)
        d.poly([(54,8),(76,8),(67,133),(46,158),(29,163),(49,128)],6)
        d.line([(133,11),(137,116),(158,155)],2,4)
        for x in (52,68,119,130):
            for y in (30,61,91):d.line([(x,y),(x+3,y+8),(x,y+19)],5)
        d.foliage(8,-7,154,7);d.foliage(-5,10,91);d.foliage(98,7,98)
        d.foliage(7,115,56,7);d.foliage(142,125,49)
    elif stage in (4,8):
        for x,h,w in ((12,98,45),(39,141,43),(119,139,43),(151,91,38)):
            d.crystal(x,167,h,w)
        d.masonry(56,103,82,65)
        d.poly([(54,109),(67,83),(124,83),(140,109)],6)
    elif stage in (6,11):
        d.masonry(26,33,141,137)
        d.box((19,27,174,38),6)
        for x in (28,151):
            d.box((x,8,x+13,167),4)
            d.line([(x+3,10),(x+3,165)],6,2)
            for y in (24,59,128):d.box((x-4,y,x+17,y+6),5)
        for x in (49,116):
            d.box((x,51,x+26,84),2)
            d.box((x+3,54,x+23,81),14 if stage==11 else 13)
            d.line([(x+13,53),(x+13,82)],4,2)
        d.line([(54,93),(138,93)],13,2)
    else:
        for x in (30,139):d.pillar(x,30,137)
        d.masonry(54,70,87,98)
        d.poly([(20,44),(54,15),(139,15),(175,44)],4)
        d.line([(26,41),(56,19),(137,19),(170,41)],6,3)
        for x in (64,97,129):d.box((x,37,x+7,52),13)
        if stage==7:
            for x in (7,159):
                d.line([(x+12,168),(x+8,146),(x+19,130)],8,3)
                d.line([(x+9,147),(x,135)],13,2)
    # Door centre remains at local x96 / world gate+16. It is rendered by
    # the unchanged gate sprite; only its architectural surround is new.
    d.poly([(73,167),(73,104),(81,92),(109,92),(120,105),(120,167)],2)
    d.line([(76,165),(76,105),(84,96),(108,96),(116,106),(116,165)],6,3)
    d.box((81,106,110,167),0)
    d.box((58,169,137,175),4);d.line([(60,170),(135,170)],6,2)
    return indexed(d,stage,3,True)


def arrays(text):
    return {m[1]:(m[0],m[2]) for m in re.findall(r'static const (uint8_t|uint16_t) (mg_\w+)\[\d+\] = \{(.*?)\};',text,re.S)}


def replace_array(text,name,values,ctype='uint16_t'):
    data='static const '+ctype+' '+name+'['+str(len(values))+'] = { '+', '.join(hex(int(v)) for v in values)+' };'
    pattern=r'static const (?:uint8_t|uint16_t) '+re.escape(name)+r'\[\d+\] = \{.*?\};'
    if re.search(pattern,text,re.S):return re.sub(pattern,lambda _:data,text,flags=re.S)
    return text.replace('#endif',data+'\n#endif')


def install(author=False):
    AUTHORED.mkdir(parents=True,exist_ok=True)
    def authored(name,painter):
        path=AUTHORED/(name+'.png')
        if author or not path.exists():painter().save(path,transparency=0)
        image=Image.open(path)
        if image.mode!='P' or image.info.get('transparency')!=0:
            raise ValueError('Authored scenery must be indexed with transparent index zero: '+name)
        return image
    manifest=json.loads((GENERATED/'assets.json').read_text())
    header=(GENERATED/'maiya_assets.h').read_text()
    lanes=[bytearray((GENERATED/f'780-{c}.{c}').read_bytes()) for c in ('c1','c2')]
    original={a['name']:a for a in manifest}
    protected={a['name']:[hashlib.sha256(b[a['tile_base']*64:(a['tile_last']+1)*64]).hexdigest() for b in lanes]
               for a in manifest if not a['name'].startswith(('bg','ground','arena','arenaground','block','landmark','v5_'))}
    def native(image):
        idx=np.asarray(image,dtype=np.uint8)
        full=np.asarray(image.getpalette(),dtype=np.uint8).reshape(-1,3)
        used=np.flatnonzero(np.any(full!=0,axis=1))
        bank_count=int(used[-1]//16+1) if len(used) else 1
        pal=full[:bank_count*16]
        h,w=idx.shape;assign=np.empty((h//16,w//16),dtype=np.uint8)
        for y in range(h//16):
            for x in range(w//16):
                t=idx[y*16:y*16+16,x*16:x*16+16];used=np.unique(t[t!=0]//16)
                if len(used)>1:raise ValueError('A tile crosses palette banks')
                assign[y,x]=int(used[0]) if len(used) else 0
        lo,hi=encode_image(idx%16,h*w//256)
        if not np.array_equal(decode_image(lo,hi,w,h),idx%16):raise ValueError('Native graphics roundtrip failed')
        swapped=[np.frombuffer(b,dtype=np.uint8).reshape(-1,2)[:,::-1].tobytes() for b in (lo,hi)]
        return swapped,pal,assign
    def patch(name,image,bank_base=16):
        nonlocal header
        a=original[name];encoded,pal,assign=native(image)
        if image.size!=(a['strips']*16,a['rows']*16):raise ValueError('Scenery reservation changed: '+name)
        first,last=a['tile_base']*64,(a['tile_last']+1)*64
        for b,pixels in zip(lanes,encoded):b[first:last]=pixels
        a['palette_count']=len(pal)//16
        header=replace_array(header,'mg_'+name+'_map',(assign+bank_base).flatten(),'uint8_t')
        image.convert('RGBA').save(GENERATED/(name+'.png'))
        return pal
    mid_tiles=[];mid_pal=[]
    end=max(a['tile_last'] for a in manifest if not a['name'].startswith('v5_'))+1
    for stage,cols in enumerate(COLS):
        path=AUTHORED/(THEMES[stage][0]+'.png')
        if author or not path.exists():panorama(stage,cols*16).save(path,transparency=0)
        image=Image.open(path)
        if image.mode!='P' or image.size!=(cols*16,224):raise ValueError('Invalid authored environment: '+str(path))
        pal=patch('bg'+str(stage),image.crop((0,0,cols*16,192)))
        patch('ground'+str(stage),image.crop((0,192,512,224)))
        words=[v for p in pal.reshape(6,16,3) for v in palette_words(p)]
        header=replace_array(header,f'mg_bg{stage}_pal',words)
        header=replace_array(header,f'mg_bg{stage}_blight_pal',words)
        header=re.sub(rf'#define MG_BG{stage}_BANKS \d+u',f'#define MG_BG{stage}_BANKS 6u',header)
        mid=authored(THEMES[stage][0]+'_mid',lambda:midground(stage))
        if mid.size!=(32,112):raise ValueError('Midground canvas must stay 32x112')
        encoded,mp,_=native(mid);mid_tiles.append(end)
        for b,pixels in zip(lanes,encoded):b[end*64:(end+14)*64]=pixels
        mid_pal.extend(palette_words(mp[:16]));end+=14
    # Keep each ledge's accepted alpha/collision silhouette, author its pixels
    # as discrete grass, timber, masonry or plate clusters inside that outline.
    terrain_stages={'grass':0,'moss':1,'sand':2,'autumn':3,'snow':4,
                    'bark':5,'rust':6,'coral':7,'stone':8,'savanna':9}
    for material,stage in terrain_stages.items():
        for name,a in original.items():
            if not name.startswith('block_'+material+'_'):continue
            path=GENERATED/(name+'.png')
            alpha=np.asarray(Image.open(path).convert('RGBA'))[:,:,3]>=128
            d=Drawing(alpha.shape[1],alpha.shape[0])
            d.box((0,0,d.w-1,d.h-1),4)
            if material in ('grass','moss','autumn','bark','savanna'):
                d.box((0,0,d.w-1,4),8);d.line([(0,1),(d.w,1)],9)
                for x in (2,11,24):
                    d.poly([(x,4),(x+4,4),(x+3,11),(x+6,15),(x+2,13)],7)
                for x,y in ((3,18),(18,13),(23,24)):
                    d.line([(x,y),(x+5,y-1),(x+8,y+3)],5)
                    d.line([(x+4,y+4),(x+9,y+4)],2)
            else:
                d.box((0,0,d.w-1,3),6 if material!='snow' else 14)
                for y in (7,18,29):
                    offset=8 if y==18 else -6
                    for x in range(offset,d.w,21):
                        d.box((x,y,x+18,min(y+8,d.h-2)),5)
                        d.line([(x,y),(x+18,y)],6)
                        d.line([(x+18,y+2),(x+18,y+8)],2)
                if material=='rust':
                    for x,y in ((4,7),(25,7),(4,25),(25,25)):d.box((x,y,x+1,y+1),13)
            shade=np.asarray(d.image).copy();shade[~alpha]=0
            # Ink follows the underside rather than putting an arbitrary box
            # around the transparent end caps.
            for y in range(1,d.h-1):
                for x in range(1,d.w-1):
                    if alpha[y,x] and not alpha[y+1,x]:shade[y,x]=2
            d.image=Image.fromarray(shade,'L')
            image=authored(name,lambda:indexed(d,stage,1,True))
            patch(name,image)
        header=replace_array(header,'mg_block_'+material+'_pal',palette_words(np.asarray(palettes(stage,1),dtype=np.uint8)))
    for name,stage in zip(('forest','falls','coast','autumn','ice','worldtree','works','reef','mountain','baobab','citadel'),(0,1,2,3,4,5,6,7,8,9,11)):
        image=authored('landmark_'+name,lambda:landmark(stage))
        pal=patch('landmark_'+name,image,71)
        header=replace_array(header,'mg_landmark_'+name+'_pal',[v for p in pal.reshape(3,16,3) for v in palette_words(p)])
    header=replace_array(header,'mg_landmark_banks',[3]*11,'uint8_t')
    manifest=[a for a in manifest if not a['name'].startswith('v5_')]
    for stage,tile in enumerate(mid_tiles):manifest.append(dict(name='v5_mid_'+str(stage),tile_base=tile,tile_last=tile+13,strips=2,rows=7,stride=2,palette_count=1))
    header=replace_array(header,'mg_v5_mid_tiles',mid_tiles)
    header=replace_array(header,'mg_v5_mid_pal',mid_pal)
    def extra(name,image,banks=1):
        nonlocal end,header
        expected=image.size
        image=authored(name,lambda:image)
        if image.size!=expected:raise ValueError('Authored scenery canvas changed: '+name)
        encoded,pal,_=native(image);count=image.width*image.height//256;tile=end
        for b,pixels in zip(lanes,encoded):b[end*64:(end+count)*64]=pixels
        manifest.append(dict(name=name,tile_base=end,tile_last=end+count-1,strips=image.width//16,
                             rows=image.height//16,stride=image.width//16,palette_count=banks))
        end+=count
        macro='MG_'+name.upper()+'_TILE'
        header=re.sub(r'#define '+macro+r' \d+u\n','',header)
        header=header.replace('#endif',f'#define {macro} {tile}u\n#endif')
        return pal[:banks*16]
    cart=Drawing(64,32)
    cart.box((2,0,61,5),6);cart.box((5,6,58,20),4)
    cart.line([(6,8),(56,8)],13,2)
    for x in (8,26,44):
        cart.box((x,11,x+12,18),5);cart.line([(x,11),(x+12,11)],6)
    cart.line([(5,22),(58,22)],2,3)
    for x in (13,49):
        cart.pen.ellipse((x-7,19,x+7,31),fill=2)
        cart.pen.ellipse((x-4,22,x+4,28),fill=6)
        cart.box((x-1,24,x+1,26),1)
    cp=extra('v5_cart',indexed(cart,6,1,True))
    header=replace_array(header,'mg_v5_cart_pal',palette_words(cp))
    spark=Drawing(96,32)
    for i in range(3):
        x=i*32;c=x+16;spread=(13,15,10)[i]
        spark.poly([(c,2),(c+3,12),(x+30,5),(c+8,16),(x+31,23),
                    (c+3,20),(c,31),(c-3,20),(x+1,27),(c-8,16),
                    (x+1,7),(c-3,12)],13)
        spark.poly([(c,6),(c+2,14),(x+26,11),(c+5,17),(x+26,22),
                    (c+1,19),(c,27),(c-2,19),(x+6,22),(c-5,16),
                    (x+7,11),(c-2,14)],15)
        spark.line([(x+2,spread),(x+6,spread-3)],14,2)
    sp=extra('v5_impact',indexed(spark,0,1,True))
    header=replace_array(header,'mg_v5_impact_pal',palette_words(sp))
    arena_tiles=[];arena_colors=[];arena_maps=[]
    for stage in range(12):
        image=panorama(stage,512)
        arena_tiles.append(end)
        pal=extra('v5_arena_'+str(stage),image,6)
        arena_colors.extend(v for p in pal.reshape(6,16,3) for v in palette_words(p))
        _,_,mapping=native(image)
        arena_maps.extend((mapping+16).flatten())
    header=replace_array(header,'mg_v5_arena_tiles',arena_tiles)
    header=replace_array(header,'mg_v5_arena_pal',arena_colors)
    header=replace_array(header,'mg_v5_arena_maps',arena_maps,'uint8_t')
    if end&3:raise ValueError('Hardware-animation reservation must be four-tile aligned')
    water=Drawing(64,96)
    for frame in range(4):
        x=frame*16
        water.box((x+4,0,x+11,95),14)
        for yy in range(-8,96,19):
            water.line([(x+5,yy+frame*4),(x+5,yy+frame*4+9)],15)
            water.line([(x+9,yy+frame*4+5),(x+9,yy+frame*4+14)],12)
    extra('v5_water',indexed(water,1,1,True))
    for name,expected in protected.items():
        a=original[name];actual=[hashlib.sha256(b[a['tile_base']*64:(a['tile_last']+1)*64]).hexdigest() for b in lanes]
        if actual!=expected:raise ValueError('Protected sprite changed: '+name)
    if end>0x7FFD:raise ValueError('V5 art exceeds native CD sprite memory')
    for part,data in zip(('c1','c2'),lanes):(GENERATED/f'780-{part}.{part}').write_bytes(data)
    (GENERATED/'maiya_assets.h').write_text(header)
    (GENERATED/'assets.json').write_text(json.dumps(manifest,indent=2)+'\n')
    (GENERATED/'v5-art-check.json').write_text(json.dumps({'protected':protected,'last_tile':end-1,'background_banks':6},indent=2)+'\n')
    print('V5: twelve indexed panoramas; twelve midground sources;',len(protected),'sprite reservations unchanged; last tile',end-1)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--author',action='store_true');a=p.parse_args();install(a.author)
