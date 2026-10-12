"""Hand-composed native mountain campaign: sixteen distinct 256-pixel sectors."""
from PIL import Image,ImageDraw
from pixel_scenery import COLORS,rect,tree,house,mountain,field,airport
W,H=352,4096
SECTORS=('Dawn farms','Foothill hamlet','Ridge AA apron','Reservoir crossing',
         'Valley radar station','Terraced quarry','Mountain command site','Civil airfield',
         'River industry','Orchard village','Rail cutting','Alpine pass',
         'Forward barracks','Fortress switchback','Citadel outer works','Crimson Keep approach')
def cliff(d,x,y,w,h,left=True):
    # A broad stepped escarpment with a coherent lit cap and its dark cliff wall.
    v=[(x,y+h),(x,y+31),(x+8,y+31),(x+8,y+15),(x+22,y+15),(x+22,y+3),(x+w-21,y+3),(x+w-21,y+14),(x+w-8,y+14),(x+w-8,y+38),(x+w,y+38),(x+w,y+h)]
    d.polygon(v,fill=COLORS['rock_dark']);d.polygon(v[:-2]+[(x+w-9,y+h-19),(x+10,y+h-19)],fill=COLORS['rock'])
    d.line((x+9,y+32,x+23,y+17,x+w-22,y+17,x+w-10,y+40),fill=COLORS['rock_hi'],width=3)
    for yy in range(y+48,y+h-10,19):
        d.line((x+8,yy,x+w-10,yy-7),fill=COLORS['rock_hi'],width=2)
        d.line((x+8,yy+5,x+w-10,yy-2),fill=COLORS['rock_dark'],width=2)
    for xx in range(x+16,x+w-10,17):d.line((xx,y+h-17,xx+3,y+h-2),fill=COLORS['shadow'],width=2)
    rect(d,(x+4,y+h-1,x+w+4,y+h+6),'shadow')
def bridge(d,x,y):
    rect(d,(x,y,x+67,y+66),'water_dark');rect(d,(x+3,y,x+64,y+66),'water')
    for yy in (y+6,y+26,y+52):d.line((x+7,yy,x+25,yy),fill=COLORS['water_hi'])
    rect(d,(x-4,y+23,x+72,y+40),'shadow');rect(d,(x-6,y+18,x+68,y+34),'steel_dark','steel_hi')
    rect(d,(x-5,y+21,x+67,y+30),'road');d.line((x-3,y+25,x+65,y+25),fill=COLORS['line'])
    for xx in (x+4,x+30,x+57):rect(d,(xx,y+31,xx+4,y+44),'rock_dark','rock_hi')
def cows(d,x,y):
    for dx,dy in ((0,0),(16,9),(32,1)):
        rect(d,(x+dx,y+dy,x+dx+6,y+dy+3),'wall');rect(d,(x+dx+5,y+dy-1,x+dx+8,y+dy+2),'shadow')
        rect(d,(x+dx+1,y+dy+4,x+dx+1,y+dy+5),'shadow');rect(d,(x+dx+5,y+dy+4,x+dx+5,y+dy+5),'shadow')
def rail(d,x,y,h):
    rect(d,(x-4,y,x+32,y+h),'soil');rect(d,(x,y,x+3,y+h),'steel_dark');rect(d,(x+22,y,x+25,y+h),'steel_dark')
    for yy in range(y+4,y+h,8):d.line((x-3,yy,x+29,yy),fill=COLORS['roof_dark'],width=2)
    d.line((x+1,y,x+1,y+h),fill=COLORS['steel_hi']);d.line((x+23,y,x+23,y+h),fill=COLORS['steel_hi'])
PAINTED=__import__('pathlib').Path(__file__).resolve().parents[1]/'assets/painted/routes/stage_1.png'
def route():
    # The painted map (tools/paint_terrain.py) replaces the drawn one once built.
    if PAINTED.exists():
        with Image.open(PAINTED) as painted:return painted.convert('RGBA')
    im=Image.new('RGBA',(W,H),COLORS['grass']);d=ImageDraw.Draw(im)
    # Connected terrain clusters instead of noise or a tiled photograph.
    for y in range(18,H,64):
        for x in (12,135,197,292):
            d.polygon(((x,y),(x+19,y),(x+23,y+6),(x+9,y+11),(x-2,y+7)),fill=COLORS['grass_hi' if (y//64+x)%3 else 'grass_dark'])
    rect(d,(74,0,110,H-1),'soil');rect(d,(78,0,106,H-1),'road_dark');rect(d,(80,0,104,H-1),'road')
    for y in range(8,H,24):rect(d,(91,y,92,y+10),'line')
    for sector in range(16):
        y=sector*256
        if sector in (0,1,5,10):cliff(d,0,y+12,63,170-(sector%3)*17)
        elif sector in (2,6,11,14):cliff(d,0,y+8,66,230)
        else:
            for yy in (y+28,y+101,y+189):tree(d,18,yy,24);tree(d,44,yy+13,18)
        if sector in (0,1):
            field(d,190,y+145,56,79);field(d,258,y+121,62,99)
            for xx,yy in ((187,y+30),(229,y+61),(285,y+29)):house(d,xx,yy,31,25)
            rect(d,(180,y+89,324,y+95),'soil');rect(d,(189,y+103,233,y+135),'grass_dark','soil');cows(d,193,y+110)
        elif sector in (2,4,6):
            cliff(d,228,y+15,116,161);house(d,189,y+192,38,26,True)
            for xx in (181,208):tree(d,xx,y+20,18)
        elif sector==3:
            bridge(d,183,y+57);house(d,268,y+37,35,28);house(d,278,y+157,39,27)
            rect(d,(176,y+137,273,y+146),'soil');field(d,184,y+176,57,59)
        elif sector==5:
            for n in range(3):
                rect(d,(184+n*15,y+28+n*16,338-n*10,y+190-n*22),'rock_dark','rock_hi')
                rect(d,(188+n*15,y+32+n*16,334-n*10,y+175-n*22),'soil')
            house(d,204,y+207,44,27,True);rect(d,(173,y+197,341,y+205),'road')
        elif sector==7:
            airport(d,188,y+18,151,218);house(d,149,y+22,32,24);house(d,149,y+90,35,26)
        elif sector==8:
            bridge(d,181,y+46);rail(d,306,y+7,226)
            for xx,yy in ((190,y+140),(238,y+172)):house(d,xx,yy,42,39,True)
        elif sector==9:
            for yy in (y+26,y+91,y+151):
                for xx in (190,223,258,294):tree(d,xx,yy,20)
            house(d,188,y+207,39,26);field(d,246,y+201,79,37)
        elif sector==10:
            rail(d,184,y+5,240);cliff(d,237,y+25,107,191)
            for n in range(3):house(d,181,y+27+n*59,34,42,True)
        elif sector==11:
            cliff(d,205,y+5,140,237)
            for yy in (y+37,y+127):tree(d,142,yy,22);house(d,154,yy+37,29,25)
        elif sector==12:
            for yy in (y+30,y+103,y+176):
                house(d,187,yy,46,37,True);house(d,260,yy,54,37,True)
            rect(d,(173,y+15,178,y+241),'steel_dark');rect(d,(327,y+15,332,y+241),'steel_dark')
        elif sector==13:
            cliff(d,243,y+15,101,220);rect(d,(183,y+12,202,y+242),'road_dark');rect(d,(185,y+12,199,y+242),'road')
            for yy in (y+42,y+108,y+178):house(d,215,yy,24,28,True)
        elif sector==14:
            rect(d,(187,y+22,341,y+235),'steel_dark','shadow')
            for xx in (190,257):
                for yy in (y+28,y+151):house(d,xx,yy,60,62,True)
            rect(d,(184,y+117,341,y+134),'road');d.line((186,y+125,339,y+125),fill=COLORS['line'])
        else:
            cliff(d,6,y+3,60,185);cliff(d,247,y+3,98,185)
            rect(d,(125,y+30,230,y+231),'road_dark','steel_hi');rect(d,(142,y+47,212,y+209),'steel_dark')
            for yy in (y+58,y+187):
                for xx in (151,197):rect(d,(xx,yy,xx+6,yy+12),'line')
            house(d,190,y+4,47,28,True)
        # Trees are placed beside the valley road, on their own parcels.
        for yy in (y+23,y+113,y+207):tree(d,119,yy,15)
    for page in (2,4,6):
        y=page*256+96
        rect(d,(118,y-34,170,y+32),'soil','sand_hi');rect(d,(124,y-26,164,y+24),'steel_dark','steel_hi')
        rect(d,(105,y+9,123,y+17),'road')
        for yy in (y-19,y+17):d.line((128,yy,160,yy),fill=COLORS['line'])
    return im
