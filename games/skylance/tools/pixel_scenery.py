"""Native pixel scenery: integer shapes, fixed ramps and authored route layouts.

352x2048 is the actual tile canvas. No source painting, filtering or resampling
is involved. Water lanes, the road and encounter pads are owned by this layout.
"""
from PIL import Image, ImageDraw
import numpy as np

W, H = 352, 2048
COLORS = {
    'grass': '#526950', 'grass_hi': '#617a58', 'grass_dark': '#405740',
    'tree_dark': '#223c39', 'tree_mid': '#345747', 'tree_hi': '#54755a',
    'soil': '#9b8866', 'sand': '#c0ae80', 'sand_hi': '#d9c599',
    'water': '#22516d', 'water_dark': '#193c59', 'water_hi': '#32738a',
    'foam': '#8fb9b1', 'road': '#48545c', 'road_dark': '#303b47',
    'line': '#c3bb9b', 'wall': '#c1b798', 'wall_dark': '#847d70',
    'roof': '#98664f', 'roof_hi': '#bc8665', 'roof_dark': '#654b47',
    'steel': '#859195', 'steel_hi': '#b7c0b5', 'steel_dark': '#4f636b',
    'glass': '#2d535d', 'glass_hi': '#739b99', 'shadow': '#2e3c3f',
    'rock': '#697877', 'rock_hi': '#8a9690', 'rock_dark': '#485b60',
}


def rect(d, box, color, outline=None):
    d.rectangle(box, fill=COLORS.get(color, color),
                outline=COLORS.get(outline, outline) if outline else None)


def tree(d, x, y, size=14):
    # Stepped crown with three connected shade clusters and a short trunk.
    rect(d, (x+3, y+size-3, x+size+3, y+size+2), 'shadow')
    rect(d, (x+size//2-1, y+size-2, x+size//2+1, y+size+3), 'soil')
    p = [(x+3,y),(x+size-4,y),(x+size-1,y+3),(x+size-1,y+6),
         (x+size+1,y+8),(x+size+1,y+size-3),(x+size-3,y+size),
         (x+2,y+size),(x-1,y+size-3),(x-1,y+4)]
    d.polygon(p, fill=COLORS['tree_dark'])
    d.polygon([(x+3,y+1),(x+size-5,y+1),(x+size-2,y+4),
               (x+size-2,y+size-5),(x+4,y+size-3),(x+1,y+size-6),(x+1,y+4)],
              fill=COLORS['tree_mid'])
    rect(d, (x+4,y+3,x+size-7,y+6), 'tree_hi')
    rect(d, (x+2,y+7,x+5,y+10), 'tree_hi')


def house(d, x, y, w=26, h=22, flat=False):
    rect(d, (x-3,y-3,x+w+6,y+h+6), 'soil')
    rect(d, (x+3,y+3,x+w+3,y+h+3), 'shadow')
    rect(d, (x,y+3,x+w,y+h), 'wall_dark', 'shadow')
    rect(d, (x+1,y+5,x+w-3,y+h-1), 'wall')
    for wx in range(x+4,x+w-4,7):
        rect(d, (wx,y+h-6,wx+3,y+h-3), 'glass')
        rect(d, (wx,y+h-6,wx+3,y+h-6), 'glass_hi')
    roof = 'steel' if flat else 'roof'
    rect(d, (x-1,y-1,x+w,y+h-9), roof, 'roof_dark' if not flat else 'steel_dark')
    rect(d, (x,y,x+w-1,y+1), 'steel_hi' if flat else 'roof_hi')
    if flat:
        rect(d, (x+5,y+4,x+11,y+8), 'steel_dark')
        rect(d, (x+6,y+4,x+10,y+5), 'steel_hi')
    else:
        d.line((x,y+(h-9)//2,x+w,y+(h-9)//2), fill=COLORS['roof_hi'])
        for ry in range(y+3,y+h-9,4):
            d.line((x+2,ry,x+w-2,ry), fill=COLORS['roof_dark'])
        rect(d, (x+w-7,y-3,x+w-4,y+4), 'wall_dark', 'shadow')
    rect(d,(x+w//2,y+h-5,x+w//2+3,y+h),'roof_dark')
    rect(d,(x+w//2-1,y+h+1,x+w//2+4,y+h+4),'sand_hi')


def mountain(d, x, y, w, h):
    # Terraced contours, with a deliberate light face and a dark east face.
    p=[(x,y+h),(x+6,y+h-18),(x+4,y+h-34),(x+18,y+h-43),
       (x+24,y+17),(x+w//2,y),(x+w//2+12,y+8),
       (x+w-16,y+35),(x+w-6,y+h-14),(x+w,y+h)]
    d.polygon(p, fill=COLORS['rock_dark'])
    d.polygon(p[:-3]+[(x+w//2+3,y+h-14),(x+12,y+h)],fill=COLORS['rock'])
    d.polygon([(x+24,y+22),(x+w//2,y+3),(x+w//2+3,y+17),
               (x+34,y+43),(x+24,y+49)],fill=COLORS['rock_hi'])
    for k in range(3):
        sy=y+h-12-k*13
        d.line((x+8+k*5,sy,x+w//2-2,sy-5),fill=COLORS['rock_hi'],width=2)
    for k in range(4):
        sx=x+w//2+8+k*4;sy=y+45+k*19
        d.line((sx,sy,sx+7,sy+18),fill=COLORS['rock'],width=2)
        d.line((sx-9,sy+6,sx-4,sy+14),fill=COLORS['rock_dark'])
    d.line((x+15,y+h-5,x+w-3,y+h-5),fill=COLORS['grass_dark'],width=3)


def field(d, x, y, w, h):
    rect(d,(x,y,x+w,y+h),'soil','grass_dark')
    for sy in range(y+3,y+h-2,6):
        d.line((x+3,sy,x+w-3,sy),fill=COLORS['sand'],width=2)
        d.line((x+5,sy+2,x+w-5,sy+2),fill=COLORS['grass_dark'])
    rect(d,(x-2,y-2,x+w+2,y-1),'grass_dark')


def parked_boat(d, x, y):
    d.polygon([(x,y+6),(x+6,y),(x+12,y+6),(x+12,y+30),(x+2,y+30)],fill=COLORS['water_dark'])
    d.polygon([(x+1,y+6),(x+6,y+1),(x+11,y+6),(x+11,y+28),(x+2,y+28)],fill=COLORS['steel_hi'])
    rect(d,(x+3,y+7,x+9,y+23),'steel_dark')
    rect(d,(x+4,y+12,x+8,y+18),'wall')
    rect(d,(x+4,y+12,x+8,y+13),'glass')
    d.line((x+6,y+5,x+6,y+9),fill=COLORS['shadow'])


def airport(d, x, y, width=112, height=166):
    rect(d, (x,y,x+width,y+height), 'soil')
    rect(d, (x+10,y+3,x+42,y+height-3), 'road_dark')
    rect(d, (x+12,y+3,x+40,y+height-3), 'road')
    for sy in range(y+12,y+height-12,16):
        rect(d, (x+25,sy,x+27,sy+8), 'line')
    for sy in (y+6,y+height-15):
        for sx in (x+16,x+21,x+31,x+36):
            rect(d, (sx,sy,sx+1,sy+9), 'line')
    rect(d, (x+45,y+24,x+width-5,y+height-24), 'steel_dark')
    for sy in range(y+32,y+height-30,38):
        house(d,x+60,sy,36,26,flat=True)
        rect(d,(x+45,sy+14,x+59,sy+20),'road')
    # Tower has its own apron and foundation; it never overlays a roof.
    rect(d,(x+48,y+4,x+58,y+17),'wall_dark','shadow')
    rect(d,(x+47,y+2,x+59,y+9),'glass','steel_hi')


PAINTED = __import__('pathlib').Path(__file__).resolve().parents[1] / 'assets/painted/routes'


def route(stage):
    # A painted map (tools/paint_terrain.py) replaces the drawn one once built.
    painted = PAINTED / f'stage_{stage + 1}.png'
    if stage and painted.exists():               # Stage 1's map is mountain_route's
        with Image.open(painted) as im:
            return im.convert('RGBA')
    naval = stage in (1, 3)
    image = Image.new('RGBA', (W,H), COLORS['water' if naval else 'grass'])
    d = ImageDraw.Draw(image)
    # Large clustered accents, aligned to the native grid, never single-pixel noise.
    for y in range(0,H,32):
        for x in range(0,W,32):
            k=(x//32*7+y//32*3+stage*11)%9
            if naval:
                if k < 3:
                    d.line((x+3,y+8,x+17,y+8), fill=COLORS['water_hi'])
                    d.line((x+8,y+10,x+20,y+10), fill=COLORS['water_dark'])
            elif k < 4:
                rect(d,(x+5,y+9,x+16,y+12),'grass_hi' if k<2 else 'grass_dark')
                rect(d,(x+9,y+13,x+20,y+15),'grass_hi' if k<2 else 'grass_dark')
    if naval:
        # The sea lanes at world X=80 and 192 stay water for all 2048 rows.
        for page in range(8):
            y=page*256
            shore = (248,240,256,244,260,248,240,252)[page]
            rect(d,(shore,y,W-1,y+255),'sand')
            rect(d,(shore-2,y,shore-1,y+255),'foam')
            rect(d,(shore+7,y,W-1,y+255),'grass' if stage==3 else 'soil')
            if stage==1:
                house(d,shore+18,y+36,36,26,True)
                house(d,shore+58,y+90,38,32,True)
                rect(d,(shore-18,y+152,shore+34,y+165),'steel_dark','steel_hi')
                rect(d,(shore-10,y+179,shore+40,y+187),'soil','sand_hi')
                rect(d,(shore+46,y+145,W-1,y+217),'road')
                for sx in range(shore+50,W-10,20):
                    rect(d,(sx,y+154,sx+15,y+183),'roof','roof_hi')
                if page in (0,3,5,7): parked_boat(d,146,y+166)
                for sy in range(y+5,y+250,18):
                    rect(d,(shore+1,sy,shore+5,sy+7),'sand_hi')
                for sy in (y+56,y+104):
                    rect(d,(shore+12,sy,shore+15,sy+3),'steel_dark')
            else:
                mountain(d,shore+18,y+21,66,90)
                for sy in range(y+130,y+238,24): tree(d,shore+18,sy,18)
    else:
        # Traffic and tanks use this continuous road; decorative buildings
        # are drawn only on parcels outside it.
        rect(d,(74,0,110,H-1),'soil')
        rect(d,(78,0,106,H-1),'road_dark')
        rect(d,(80,0,104,H-1),'road')
        for y in range(8,H,24): rect(d,(91,y,92,y+11),'line')
        for page in range(8):
            y=page*256
            if stage==0:
                mountain(d,0,y+14,60,168)
                mountain(d,248,y+82,101,146)
                if page in (0,3,5):
                    house(d,182,y+38,30,24);house(d,217,y+70,28,22)
                if page in (1,4,6):
                    field(d,181,y+140,54,70)
                    field(d,179,y+34,51,72)
                if page in (2,5):
                    for sy in range(y+142,y+220,19):
                        for sx in (186,209,233): tree(d,sx,sy,17)
                if page==7: airport(d,198,y+20,138,200)
            elif stage in (2,5):
                rect(d,(178,y+36,339,y+228),'soil','grass_dark')
                rect(d,(180,y+122,337,y+139),'road')
                if stage==2 and page in (0,2,5,7):
                    for sy in (y+52,y+170):
                        for sx in (184,226,268): house(d,sx,sy,30,24)
                else:
                    for sx in (185,246,305): house(d,sx,y+52,36,48,True)
                    for sx in (198,264):
                        d.ellipse((sx,y+164,sx+30,y+194),fill=COLORS['steel_dark'],outline=COLORS['shadow'])
                        d.ellipse((sx+2,y+164,sx+28,y+189),fill=COLORS['steel'],outline=COLORS['steel_hi'])
                if stage==2 and page in (1,3,6):
                    field(d,7,y+30,56,76);field(d,7,y+136,56,84)
                if stage==5:
                    rect(d,(15,y+6,18,y+248),'steel_dark')
                    rect(d,(42,y+6,45,y+248),'steel_dark')
                    for sy in range(y+12,y+240,9):
                        d.line((10,sy,49,sy),fill=COLORS['roof_dark'],width=2)
                    for sy in (y+50,y+150):
                        rect(d,(11,sy,48,sy+53),'steel_dark','shadow')
                        rect(d,(13,sy+2,46,sy+49),'roof','roof_hi')
                        for sx in range(16,46,6): d.line((sx,sy+4,sx,sy+47),fill=COLORS['roof_dark'])
            elif stage==4:
                if page in (1,5,7): airport(d,208,y+20,132,194)
                else:
                    for sy in range(y+8,y+238,22):
                        for sx in (10,38,190,219,248,278,312): tree(d,sx,sy,18)
                    if page in (0,3): house(d,216,y+74,44,38,True)
            else:
                for sy in (y+26,y+168):
                    for sx in (6,39,186,231,280): house(d,sx,sy,28 if sx<74 else 36,30,True)
                rect(d,(182,y+108,344,y+126),'road')
                if page in (3,7): airport(d,206,y+130,130,116)
            for sy in range(y+16,y+232,34):
                if stage!=4:
                    tree(d,116,sy,12)
                    if page%3==1: tree(d,152,sy+12,14)
            # Small shrubs form coherent clusters along verges, never blur/noise.
            for sy in range(y+12,y+248,25):
                for sx in (67,112):
                    if (sy//25+page)%3:
                        rect(d,(sx,sy,sx+3,sy+2),'grass_dark')
                        rect(d,(sx+2,sy-1,sx+5,sy),'grass_hi')
    # Three deliberately empty military pads. Targets occupy these pads only
    # after the camera reaches travel=512,1024,1536; no building is under them.
    for page in (2,4,6):
        y=page*256+96
        if naval:
            rect(d,(114,y-36,176,y+34),'sand','foam')
        rect(d,(118,y-32,170,y+30),'soil','sand_hi')
        rect(d,(124,y-25,164,y+23),'steel_dark','steel_hi')
        for sy in (y-19,y+17): d.line((128,sy,160,sy),fill=COLORS['line'])
        # A side access spur joins the road/island apron, separate from roofs.
        if not naval: rect(d,(105,y+9,123,y+18),'road')
    return image


def cockpit(pilot, g=0, boost=0):
    """Compact 32x64 console; every display has a live FIX/sprite indicator."""
    im=Image.new('RGBA',(32,64));d=ImageDraw.Draw(im)
    d.polygon([(3,0),(28,0),(31,8),(31,63),(0,63),(0,8)],fill='#182b35',outline='#70817e')
    if pilot==1:  # F-35: single panoramic display.
        rect(d,(3,5,28,40),'glass','steel_hi')
        d.line((16,7,16,38),fill='#547568')
    else:  # F-16/F-15: two framed displays around the horizon instrument.
        rect(d,(3,5,28,28),'glass','steel_hi')
        rect(d,(3,32,14,46),'glass','steel_dark')
        rect(d,(17,32,28,46),'glass','steel_dark')
    for x in (3,27):
        for y in (8,14,20,35,41): rect(d,(x,y,x+1,y+1),'steel_hi')
    rect(d,(3,51,28,61),'shadow','steel_dark')
    if pilot==1:
        for x,value,color in ((5,g+1,'#c6d49e'),(20,3 if boost else 1,'#8db8a1')):
            rect(d,(x,31,x+6,38),'shadow')
            for k in range(value): d.line((x+1,37-k*2,x+5,37-k*2),fill=color)
    else:
        # Working miniature G and engine instruments. Integer needle endpoints.
        for cx,value in ((8,g),(23,3 if boost else 1)):
            d.ellipse((cx-4,34,cx+4,43),fill='#11202c',outline='#87988d')
            for tx,ty in ((cx-3,39),(cx-2,35),(cx+2,35),(cx+3,39)):d.point((tx,ty),fill='#b5c6a7')
            endpoints=((cx-3,40),(cx-2,36),(cx+1,35),(cx+3,36))
            ex,ey=endpoints[value]
            d.line((cx,39,ex,ey),fill='#dfc987')
    return im


def indexed_route(im):
    """Pack exact authored colors into <=16 banks; never fit or dither them."""
    a=np.asarray(im)[:,:,:3]
    H,W=im.height,im.width
    tiles=[]
    for y in range(0,H,16):
        for x in range(0,W,16):
            colors=frozenset(tuple(c) for c in np.unique(a[y:y+16,x:x+16].reshape(-1,3),axis=0))
            if len(colors)>15: raise ValueError(f'Authored tile {x},{y} has {len(colors)} colors')
            tiles.append(colors)
    banks=[];chosen={}
    for colors in sorted(set(tiles),key=lambda c:(-len(c),sorted(c))):
        candidates=[i for i,b in enumerate(banks) if len(b|colors)<=15]
        if candidates:
            i=min(candidates,key=lambda i:(len(colors-banks[i]),i))
            banks[i]|=colors
        else:
            if len(banks)==16:raise ValueError('Authored route exceeds 16 exact banks')
            i=len(banks);banks.append(set(colors))
        chosen[colors]=i
    palettes=np.zeros((len(banks),16,3),dtype=np.uint8)
    lookups=[]
    for i,colors in enumerate(banks):
        ordered=sorted(colors)
        palettes[i,1:len(ordered)+1]=ordered
        lookups.append({c:j+1 for j,c in enumerate(ordered)})
    indices=np.zeros((H,W),dtype=np.uint8)
    mapping=np.zeros((H//16,W//16),dtype=np.uint8)
    for n,colors in enumerate(tiles):
        y,x=(n//(W//16))*16,(n%(W//16))*16
        bank=chosen[colors];mapping[y//16,x//16]=bank
        for yy in range(y,y+16):
            for xx in range(x,x+16):indices[yy,xx]=lookups[bank][tuple(a[yy,xx])]
    reconstructed=palettes[mapping.repeat(16,0).repeat(16,1),indices]
    if not np.array_equal(a,reconstructed):raise ValueError('Authored route pixels changed during indexing')
    return indices,palettes,mapping


def chaff():
    im=Image.new('RGBA',(32,32));d=ImageDraw.Draw(im)
    for x,y in ((8,4),(19,5),(12,10),(24,13),(5,17),(18,19),(11,25),(24,26)):
        d.line((x,y,x+2,y+2),fill='#d9e4d2');d.point((x+3,y+2),fill='#839ca5')
    return im


def cloud(stage):
    """Stepped native cloud clusters matching the authored terrain grid."""
    im=Image.new('RGBA',(96,64));d=ImageDraw.Draw(im)
    ramps=[('#45677c','#688d9b','#a4bbc0','#d4ddd1'),
           ('#537b8e','#7b9eab','#b7ccc8','#e3e6d4'),
           ('#626e82','#8b98ad','#b9c4cc','#dee5df'),
           ('#486776','#729ca2','#aec4be','#dce2cd'),
           ('#4c7476','#7b9c92','#b6c7b0','#e1e2c7'),
           ('#696b75','#999693','#c3bdaf','#e3dac4'),
           ('#685c75','#968299','#c7b2bf','#e6d4ce')]
    dark,mid,light,white=ramps[stage]
    p=[(8,30),(8,25),(12,25),(12,20),(18,20),(18,15),(27,15),
       (27,10),(35,10),(35,8),(48,8),(48,11),(57,11),(57,18),
       (66,18),(66,16),(75,16),(75,22),(81,22),(81,28),(87,28),
       (87,36),(82,36),(82,42),(74,42),(74,47),(60,47),
       (60,51),(40,51),(40,48),(25,48),(25,44),(15,44),(15,38),(8,38)]
    d.polygon(p,fill=dark)
    d.polygon([(x,y-3 if y>30 else y) for x,y in p],fill=mid)
    for x,y,w,h in ((14,24,23,13),(28,12,27,17),(50,22,29,12),(31,29,33,12)):
        d.polygon([(x+4,y),(x+w-5,y),(x+w-5,y+3),(x+w,y+3),
                   (x+w,y+h-4),(x+w-5,y+h-4),(x+w-5,y+h),(x+3,y+h),
                   (x+3,y+h-3),(x,y+h-3),(x,y+4),(x+4,y+4)],fill=light)
        d.line((x+5,y+2,x+w-7,y+2),fill=white,width=2)
    d.line((30,44,43,44),fill=dark,width=2)
    d.line((64,37,76,37),fill=dark,width=2)
    return im


def radar(alert):
    im=Image.new('RGBA',(64,64));d=ImageDraw.Draw(im)
    c='#e78461' if alert else '#7dbba0'
    # Dashed coverage ring with clear interior, not an opaque field.
    for a,b in ((0,32),(58,90),(116,148),(174,206),(232,264),(290,322)):
        d.arc((2,2,61,61),a,b,fill=c)
    d.line((28,32,35,32),fill=c);d.line((32,28,32,35),fill=c)
    return im


def miniboss(pilot):
    im=Image.new('RGBA',(64,48));d=ImageDraw.Draw(im)
    ramps=[('#697c82','#a4b4b5','#334853'),('#6c7456','#a2aa78','#364a41'),('#976e58','#c7a180','#553f42')]
    mid,hi,dark=ramps[pilot]
    # Dedicated broad-wing defender, downward nose and twin engines.
    d.polygon([(29,4),(34,4),(37,18),(58,22),(60,30),(40,28),(37,38),(35,44),(28,44),(26,38),(23,28),(3,30),(5,22),(26,18)],fill=dark)
    d.polygon([(30,5),(33,5),(36,21),(55,24),(57,28),(37,25),(34,39),(31,43),(28,38),(26,25),(7,28),(8,24),(28,20)],fill=mid)
    d.line((31,6,31,41),fill=hi,width=2)
    d.polygon([(29,25),(34,25),(33,34),(30,34)],fill='#203d4d')
    for x in (22,39):
        d.rectangle((x,10,x+3,26),fill=dark)
        d.line((x,11,x+1,24),fill=hi)
        d.rectangle((x,6,x+3,9),fill='#cdad75')
    d.line((8,25,23,23),fill=hi);d.line((40,23,55,25),fill=hi)
    return im
