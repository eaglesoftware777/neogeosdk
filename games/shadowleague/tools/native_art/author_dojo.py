"""Native 512x224 dojo study: integer architectural contours and pixel clusters.
This authoring command is explicit; builds consume the edited PNG export.
"""
from pathlib import Path
from PIL import Image,ImageDraw
import json
OUT=Path(__file__).resolve().parents[2]/'assets/authored/rebirth/arena'
COLORS=['000000','181828','302028','683830','985838','d09048','f8d088','304070','988088','505880','708898','486868','203838','385040','a89880','e8e0c0']
def main():
 OUT.mkdir(parents=True,exist_ok=True);im=Image.new('P',(512,224),7);d=ImageDraw.Draw(im)
 def p(points,c):d.polygon(points,fill=c)
 def r(box,c):d.rectangle(box,fill=c)
 def l(points,c,w=1):d.line(points,fill=c,width=w)
 # Clear bands of dawn color, no interpolated gradient or noisy painting.
 r((0,47,511,93),8);r((0,94,511,153),10)
 p([(55,138),(101,93),(130,104),(186,63),(223,88),(260,56),(305,91),(339,72),(405,120),(454,140)],9)
 p([(157,88),(186,63),(207,77),(196,76),(189,82),(182,76),(173,85)],15)
 p([(232,80),(260,56),(287,77),(271,72),(264,82),(258,72),(248,80)],15)
 p([(97,147),(153,111),(202,126),(236,98),(279,123),(316,109),(365,145),(421,160)],11)
 p([(154,115),(171,119),(167,125),(149,130),(137,136),(119,137)],10)
 p([(287,139),(316,109),(335,124),(323,122),(311,132),(300,132)],10)
 r((0,148,511,172),12)
 # Deliberate distant forest crowns, differing silhouettes at fixed positions.
 for x,y,h in [(108,131,19),(131,137,15),(166,139,20),(215,145,13),(284,141,18),(348,137,25),(372,145,16),(401,132,23)]:
  p([(x,y-h),(x-3,y-h+6),(x-7,y-9),(x-4,y-9),(x-10,y-1),(x-3,y-1),(x-11,y+6),(x+10,y+6),(x+4,y-2),(x+8,y-2),(x+3,y-10)],12)
  l([(x,y-h+7),(x-3,y-7),(x+1,y-5)],13)
 # Curved roof eave and traditional timber frame, shadow mass under the lip.
 p([(0,0),(512,0),(512,24),(448,25),(364,37),(250,39),(147,35),(70,23),(0,21)],1)
 p([(0,5),(78,12),(151,21),(257,24),(364,21),(448,12),(511,4),(511,13),(454,21),(364,29),(252,32),(144,27),(71,17),(0,13)],3)
 l([(0,7),(74,15),(146,24),(252,29),(365,25),(452,17),(511,8)],5,2)
 l([(0,19),(74,23),(147,34),(252,38),(364,34),(451,24),(511,19)],4,2)
 # Paper walls, frames and carefully placed lattice shadows.
 for left,right in [(0,90),(434,511)]:
  r((left,36,right,163),14);r((left,41,right,148),15)
  for x in range(left+8,right,19):r((x,38,x+2,162),3);l([(x,42),(x,146)],4)
  for y in (62,88,116,143):r((left,y,right,y+2),3);l([(left,y),(right,y)],4)
  r((left,149,right,163),2);l([(left,151),(right,151)],4)
 # Two heavy pillars. Upper-left light, deep architectural cast shadows.
 for x in (90,412):
  r((x,29,x+20,166),1);r((x+3,31,x+17,163),3);r((x+4,33,x+7,162),5)
  r((x+8,33,x+11,162),4);r((x+17,31,x+19,166),2)
  for y in (48,74,102,127,154):l([(x+10,y),(x+15,y+2),(x+15,y+5)],2)
  r((x-4,156,x+24,167),2);l([(x-2,158),(x+22,158)],4)
 # Crest banners; an original graphic, not imitation text or real insignia.
 for x in (131,360):
  l([(x+9,29),(x+9,44)],2,2);r((x-3,42,x+24,46),2);r((x,46,x+21,107),2)
  p([(x+2,46),(x+19,46),(x+19,101),(x+10,109),(x+2,101)],3)
  l([(x+3,48),(x+3,98)],5)
  p([(x+10,59),(x+16,66),(x+10,74),(x+5,66)],6)
  l([(x+6,83),(x+14,83),(x+10,87),(x+6,92),(x+14,92)],5,2)
 # Low rail and pale wood floor, with perspective seams and selective shine.
 r((0,163,511,169),1);r((0,164,511,165),5);r((0,170,511,223),4)
 for top,bottom,c in [(170,181,3),(182,196,4),(197,213,5),(214,223,4)]:r((0,top,511,bottom),c)
 for y in (181,196,213):l([(0,y),(511,y)],2);l([(0,y+1),(511,y+1)],5)
 for x in (-90,-10,68,144,220,296,372,448,526,602):
  l([(256+(x-256)//3,170),(x,223)],3)
 # Long restrained reflections, with crisp ends rather than blanket noise.
 for x,y,length in [(114,178,26),(172,179,34),(263,177,45),(341,180,22),(51,189,41),(143,191,61),(260,188,56),(389,192,49),(113,205,42),(227,204,71),(326,207,51)]:
  l([(x,y),(x+length,y)],6);l([(x+5,y+1),(x+length-8,y+1)],5)
 p([(105,170),(121,170),(147,223),(128,223)],2)
 p([(428,170),(439,170),(478,223),(462,223)],3)
 # Foreground plants belong to side alcoves, never the fighting floor.
 for x in (29,471):
  r((x-12,144,x+12,166),2);l([(x-12,145),(x+12,145)],4)
  for dx,dy in [(-11,0),(-5,-15),(1,-6),(8,-19),(15,-5)]:
   l([(x,145),(x+dx,120+dy)],12,2)
   p([(x+dx,120+dy),(x+dx-7,115+dy),(x+dx-3,112+dy),(x+dx+1,116+dy),(x+dx+8,112+dy),(x+dx+4,118+dy)],13)
 im.putpalette(sum([list(bytes.fromhex(c)) for c in COLORS],[])+[0]*720);im.save(OUT/'dojo.png',transparency=0)
 im.resize((1024,448),Image.Resampling.NEAREST).save(OUT/'dojo-2x.png',transparency=0)
 (OUT/'palette.json').write_text(json.dumps([list(bytes.fromhex(c)) for c in COLORS],indent=2)+'\n')
 print('Native dojo study: 512x224, one palette, no resized painting')
if __name__=='__main__':main()
