"""Original tournament lettering, drawn directly on the native pixel grid."""
from pathlib import Path
import json
from PIL import Image,ImageDraw
import numpy as np
OUT=Path(__file__).resolve().parents[2]/'assets/authored/rebirth/identity'
PAL=['000000','080c18','181c30','282c48','384858','506878','7890a0','a8c0c8','d0e0e0','f0f0d8','481828','782030','b83040','e85048','e8a850','f8d888']
TOP={
 'S':[[(8,0),(34,0),(30,7),(13,7),(10,12),(28,12),(32,16),(29,28),(23,34),(0,34),(4,27),(20,27),(23,21),(7,21),(2,17),(5,5)]],
 'H':[[(8,0),(17,0),(14,12),(26,12),(29,0),(38,0),(30,34),(21,34),(24,21),(12,21),(9,34),(0,34)]],
 'A':[[(17,0),(28,0),(34,34),(24,34),(23,28),(11,28),(7,34),(0,34)],[(20,9),(15,20),(23,20),(22,9)]],
 'D':[[(8,0),(28,0),(36,8),(32,25),(23,34),(0,34)],[(15,8),(11,26),(19,26),(24,21),(27,11),(24,8)]],
 'O':[[(14,0),(30,0),(36,8),(32,25),(23,34),(7,34),(0,26),(4,9)],[(16,8),(12,12),(9,23),(12,26),(19,26),(23,22),(27,11),(24,8)]],
 'W':[[(7,0),(16,0),(14,21),(25,4),(32,4),(33,21),(42,0),(51,0),(34,34),(26,34),(25,20),(16,34),(7,34),(0,28)]]}
LOW={
 'L':[[(4,0),(11,0),(7,17),(24,17),(22,23),(0,23)]],
 'E':[[(4,0),(27,0),(25,5),(10,5),(9,9),(22,9),(21,14),(8,14),(7,18),(24,18),(23,23),(0,23)]],
 'A':[[(13,0),(22,0),(27,23),(19,23),(18,19),(9,19),(6,23),(0,23)],[(15,6),(12,13),(18,13),(17,6)]],
 'G':[[(10,0),(28,0),(26,6),(13,6),(9,9),(7,16),(10,18),(18,18),(19,14),(14,14),(15,10),(27,10),(25,19),(19,23),(6,23),(0,18),(3,7)]],
 'U':[[(4,0),(11,0),(8,15),(10,18),(16,18),(19,15),(22,0),(29,0),(25,18),(19,23),(7,23),(0,18)]]}

def lettering(im,text,paths,x,y,gap,warm=False):
 mask=Image.new('1',im.size);d=ImageDraw.Draw(mask)
 for c in text:
  for n,points in enumerate(paths[c]):d.polygon([(x+a,y+b) for a,b in points],fill=0 if n else 1)
  x+=max(a for a,b in paths[c][0])+gap
 a=np.asarray(mask);result=np.asarray(im).copy()
 for dx,dy,index in [(4,5,1),(3,4,10 if warm else 2),(2,3,11 if warm else 4),(1,2,12 if warm else 5)]:
  m=np.zeros_like(a);m[dy:,dx:]=a[:-dy,:-dx];result[m]=index
 bands=([15,14,13,12,11,12] if warm else [9,8,7,5,6,8,7]);height=24 if warm else 35
 for yy in range(y,min(y+height,im.height)):
  c=bands[min(len(bands)-1,(yy-y)*len(bands)//height)];result[yy,a[yy]]=c
 top=a & ~np.roll(a,1,axis=0);left=a & ~np.roll(a,1,axis=1)
 result[top]=15 if warm else 9;result[left & ~top]=14 if warm else 7
 im.paste(Image.fromarray(result,'P'))

def main():
 OUT.mkdir(parents=True,exist_ok=True)
 logo=Image.new('P',(288,96));d=ImageDraw.Draw(logo)
 d.polygon([(130,1),(154,1),(167,11),(160,25),(144,32),(126,23),(120,11)],fill=2)
 d.line([(130,1),(154,1),(167,11),(160,25),(144,32),(126,23),(120,11),(130,1)],fill=6)
 d.polygon([(132,9),(146,7),(155,12),(150,21),(139,25),(130,18)],fill=11)
 d.line([(128,22),(153,9),(156,11),(133,26)],fill=14)
 d.line([(133,7),(150,23)],fill=8,width=2)
 lettering(logo,'SHADOW',TOP,12,28,4)
 lettering(logo,'LEAGUE',LOW,56,69,5,True)
 d=ImageDraw.Draw(logo)
 d.line([(9,70),(47,70),(38,76),(4,76)],fill=12)
 d.line([(238,70),(277,70),(270,76),(234,76)],fill=12)
 sub=Image.new('P',(128,16));d=ImageDraw.Draw(sub)
 glyphs={
  'F':['111111','110000','110000','111110','110000','110000','110000'],
  'I':['1111','0110','0110','0110','0110','0110','1111'],
  'G':['011110','110000','110000','110111','110011','110011','011110'],
  'H':['110011','110011','110011','111111','110011','110011','110011'],
  'T':['111111','001100','001100','001100','001100','001100','001100'],
  'N':['110011','111011','111011','110111','110111','110011','110011']}
 x=14
 for c in 'FIGHTING':
  for y,row in enumerate(glyphs[c]):
   for xx,pixel in enumerate(row):
    if pixel=='1':d.point((x+xx,y+4),fill=9 if y<3 else 14)
  x+=len(glyphs[c][0])+6
 for im,name in [(logo,'logo'),(sub,'subtitle')]:
  im.putpalette([v for c in PAL for v in bytes.fromhex(c)]+[0]*720);im.save(OUT/(name+'.png'),transparency=0)
  im.convert('RGBA').resize((im.width*3,im.height*3),Image.Resampling.NEAREST).save(OUT/(name+'-3x.png'))
  im.convert('RGBA').resize((im.width*2,im.height*2),Image.Resampling.NEAREST).save(OUT/(name+'-2x.png'))
 (OUT/'project.json').write_text(json.dumps({'colors':[list(bytes.fromhex(c)) for c in PAL],
   'images':[{'file':n+'.png','canvas':list(im.size)} for im,n in [(logo,'logo'),(sub,'subtitle')]]},indent=2)+'\n')
 print('Saved original 288x96 tournament mark and separate subtitle')
if __name__=='__main__':main()
