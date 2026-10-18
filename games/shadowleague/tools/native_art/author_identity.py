"""Original identity drawings at native resolution; explicit integer contours.
Never invoked by a normal game build. Editable projects/exports are authoritative.
"""
from pathlib import Path
import json
import numpy as np
from PIL import Image,ImageDraw

OUT=Path(__file__).resolve().parents[2]/'assets/authored/rebirth/identity'
PALETTE=['000000','080818','181830','282848','383858','485878','687898','90a8b8','c0d8e0','f0f0e0','481828','882838','c84840','f88058','b89050','f0d080']
# Letter paths individually drawn for this mark, in a 24x26 design cell.
PATHS={
'S':[[(3,0),(23,0),(21,5),(7,5),(6,10),(19,10),(23,14),(21,23),(18,26),(0,26),(1,21),(15,21),(16,16),(4,16),(0,12),(1,4)]],
'H':[[(3,0),(9,0),(8,10),(17,10),(18,0),(24,0),(21,26),(15,26),(16,16),(7,16),(6,26),(0,26)]],
'A':[[(10,0),(20,0),(24,26),(17,26),(16,21),(8,21),(6,26),(0,26)],[(12,6),(10,15),(15,15),(14,6)]],
'D':[[(3,0),(17,0),(24,6),(21,20),(15,26),(0,26)],[(9,6),(7,20),(13,20),(16,16),(18,9),(15,6)]],
'O':[[(8,0),(19,0),(24,6),(21,20),(15,26),(4,26),(0,20),(3,6)],[(10,6),(7,10),(6,18),(9,20),(13,20),(16,16),(18,8),(15,6)]],
'W':[[(0,0),(6,0),(6,17),(12,3),(16,3),(18,17),(22,0),(28,0),(22,26),(16,26),(13,15),(8,26),(2,26)]],
'L':[[(3,0),(10,0),(7,20),(23,20),(21,26),(0,26)]],
'E':[[(3,0),(24,0),(23,6),(9,6),(8,10),(20,10),(19,16),(7,16),(6,20),(21,20),(20,26),(0,26)]],
'G':[[(8,0),(24,0),(22,6),(10,6),(7,10),(6,18),(9,20),(16,20),(17,16),(12,16),(13,11),(24,11),(22,21),(17,26),(4,26),(0,20),(3,6)]],
'U':[[(3,0),(10,0),(7,18),(9,20),(14,20),(17,17),(19,0),(25,0),(22,20),(16,26),(5,26),(0,21)]],
'F':[[(3,0),(24,0),(23,6),(9,6),(8,11),(19,11),(18,17),(7,17),(6,26),(0,26)]],
'I':[[(3,0),(10,0),(7,26),(0,26)]],
'T':[[(0,0),(25,0),(24,6),(15,6),(13,26),(6,26),(8,6),(0,6)]],
'N':[[(3,0),(9,0),(16,15),(18,0),(24,0),(21,26),(15,26),(8,12),(6,26),(0,26)]]}

def save(im,name):
 im.putpalette(sum([list(bytes.fromhex(c)) for c in PALETTE],[])+[0]*720);im.save(OUT/(name+'.png'),transparency=0)
 im.convert('RGBA').resize((im.width*2,im.height*2),Image.Resampling.NEAREST).save(OUT/(name+'-2x.png'))

def word(target,text,x,y,gap=3,warm=False):
 mask=Image.new('1',target.size);d=ImageDraw.Draw(mask)
 for c in text:
  paths=PATHS[c]
  for i,p in enumerate(paths):d.polygon([(x+a,y+b) for a,b in p],fill=0 if i else 1)
  x+=max(a for a,b in paths[0])+gap
 a=np.asarray(mask);base=np.asarray(target).copy()
 # Explicit bevel bands and stepped depth; no utility-font expansion.
 for dx,dy,c in [(4,5,1),(3,4,2),(2,3,5),(1,2,6)]:
  shifted=np.zeros_like(a);shifted[dy:,dx:]=a[:-dy,:-dx];base[shifted]=c
 for yy in range(target.height):
  value=([15,15,14,12,11,12,13] if warm else [9,8,7,6,5,6,7])[min(6,max(0,(yy-y)//4))]
  base[yy,a[yy]]=value
 # Thin upper facing edges and hollow letter counters survive unchanged.
 upper=a & ~np.roll(a,1,axis=0);base[upper]=15 if warm else 9
 target.paste(Image.fromarray(base,'P'))

def main():
 OUT.mkdir(parents=True,exist_ok=True)
 logo=Image.new('P',(256,112));d=ImageDraw.Draw(logo)
 # Hexagonal tournament crest, broken ring and watchful eye motif.
 d.polygon([(119,3),(135,3),(146,12),(142,24),(127,31),(113,24),(109,12)],fill=2)
 d.line([(119,3),(135,3),(146,12),(142,24),(127,31),(113,24),(109,12),(119,3)],fill=7,width=1)
 d.polygon([(115,14),(122,10),(131,10),(139,14),(132,18),(122,18)],fill=8)
 d.polygon([(123,11),(130,11),(132,14),(128,18),(124,17),(122,14)],fill=11)
 d.rectangle((125,12,128,16),fill=15)
 word(logo,'SHADOW',45,35,gap=5)
 word(logo,'LEAGUE',44,69,gap=7,warm=True)
 # Subtitle is authored with narrow design cells, separate source contours.
 sub=Image.new('P',(128,16));sd=ImageDraw.Draw(sub);xx=6
 for cc in 'FIGHTING':
  for ii,pp in enumerate(PATHS[cc]):sd.polygon([(xx+a//2,1+b//2) for a,b in pp],fill=0 if ii else 13)
  xx+=max(a for a,b in PATHS[cc][0])//2+2
 # Exact integer reduction would drop pixels, so retain original letter size
 # and put subtitle in a separate 256x32 graphic below the principal mark.
 save(logo,'logo');save(sub,'subtitle')
 background=Image.new('P',(320,224),1);d=ImageDraw.Draw(background)
 d.polygon([(0,112),(46,78),(86,92),(126,57),(172,73),(207,48),(252,85),(283,73),(319,101),(319,190),(0,190)],fill=2)
 d.polygon([(0,143),(45,117),(84,128),(126,98),(160,130),(192,119),(233,131),(271,102),(319,132),(319,223),(0,223)],fill=3)
 # Dark tournament hall structural frame, intentional directional light.
 d.polygon([(0,0),(320,0),(320,17),(240,28),(72,22),(0,12)],fill=2)
 d.line([(0,15),(76,27),(237,32),(320,22)],fill=5,width=2)
 for x in (12,296):
  d.rectangle((x,20,x+11,223),fill=2);d.rectangle((x,21,x+2,222),fill=5)
 d.polygon([(24,185),(296,185),(320,224),(0,224)],fill=2)
 for x in (0,48,94,143,189,233,272,319):d.line([(160+(x-160)//3,185),(x,223)],fill=4)
 for y in (189,199,213):d.line([(24-(y-185)//2,y),(296+(y-185)//2,y)],fill=3)
 for x,y in [(35,47),(77,53),(249,42),(284,59),(206,35),(97,36)]:d.line([(x,y),(x+1,y)],fill=6)
 save(background,'title-background')
 (OUT/'project.json').write_text(json.dumps({'colors':[list(bytes.fromhex(c)) for c in PALETTE],'images':['logo','subtitle','title-background']},indent=2)+'\n')
 print('Native identity drawings saved; logo uses original individual lettering contours')
if __name__=='__main__':main()
