"""Native indexed brand plate: exact pixel lettering and original mountain contours."""
import json,sys
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1]/'assets/authored'
sys.path.insert(0,str(ROOT.parents[3]/'artbox'))
from boot_assets import text_pixels

def build():
    colors=json.loads((ROOT/'aircraft_models.json').read_text())[0]['palette']
    im=Image.new('P',(320,224),1);im.putpalette(sum(colors,[])+[0]*720);d=ImageDraw.Draw(im)
    d.polygon(((0,100),(24,76),(44,82),(74,54),(91,60),(122,100),(145,69),(172,74),(202,39),(224,58),(257,98),(278,77),(305,90),(319,87),(319,180),(0,180)),fill=2)
    d.polygon(((0,145),(30,113),(53,126),(78,101),(103,121),(119,151),(151,118),(171,131),(202,99),(228,128),(256,121),(282,142),(320,108),(320,184),(0,184)),fill=3)
    for x,y in ((23,113),(76,101),(151,118),(202,99),(256,121),(303,119)):
        d.line((x,y,x+10,y+14,x+15,y+14),fill=4,width=2)
    d.rectangle((0,184,319,223),fill=1);d.line((28,183,291,183),fill=9)
    d.line((24,35,295,35),fill=3);d.line((31,36,289,36),fill=5)
    mask=text_pixels('SKY LANCE',4).astype(bool);ox=(320-mask.shape[1])//2;oy=48
    a=np.array(im)
    # Crisp one-pixel outlines and an offset metal extrusion, all on-grid.
    yy,xx=np.nonzero(mask)
    for dx,dy in ((0,4),(1,4),(2,4),(0,3)):
        a[oy+yy+dy,ox+xx+dx]=2
    for dx,dy in ((-1,0),(1,0),(0,-1),(0,1)):
        a[oy+yy+dy,ox+xx+dx]=13
    bands=(7,14,14,13,13,7,9)
    a[oy+yy,ox+xx]=np.array([bands[min(int(y)//4,6)] for y in yy],np.uint8)
    def text(label,y,color):
        raw=text_pixels(label).astype(bool);ty,tx=np.nonzero(raw);a[y+ty,(320-raw.shape[1])//2+tx]=color
    text('SKY LANCE SQUADRON',89,7);text('EAGLE SOFTWARE',196,5);text('(C)2026 EAGLE SOFTWARE',210,14)
    # The hero remains exactly 48x48; the source aircraft is never enlarged.
    with Image.open(ROOT/'frames/aircraft_f16__bank_04.png') as plane:p=np.array(plane)
    region=a[119:167,136:184];region[p!=0]=p[p!=0]
    # Mission flight-path marks are purposeful composition, not a grid.
    for x,y in ((104,154),(110,150),(116,146),(203,154),(209,150),(215,146)):
        a[y:y+2,x:x+5]=8
    out=Image.fromarray(a,'P');out.putpalette(sum(colors,[])+[0]*720);out.save(ROOT/'title-native.png',transparency=0)
    (ROOT/'title-native_indices.bin').write_bytes(a.tobytes())
    return out
if __name__=='__main__':build()
