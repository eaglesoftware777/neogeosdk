"""Recompose edited indexed layer PNGs; generate previews, never redraw pixels."""
import argparse,json
from pathlib import Path
import numpy as np
from PIL import Image
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from native_art_io import indexed

def export(folder):
 folder=Path(folder);q=json.loads((folder/'project.json').read_text());w,h=q['width'],q['height'];colors=np.asarray(q['colors'],np.uint8)
 sheet=Image.new('RGB',(w*4,h*((len(q['frames'])+3)//4)),(24,32,48))
 frames=[]
 for n,fr in enumerate(q['frames']):
  result=np.zeros((h,w),np.uint8)
  for name in q['layers']:
   data,pal=indexed(folder/fr['layers'][name],(w,h))
   if not np.array_equal(pal[:len(colors)],colors):raise ValueError(fr['name']+'/'+name+': source palette changed')
   result[data>0]=data[data>0]
  im=Image.fromarray(result,'P');im.putpalette(colors.flatten().tolist()+[0]*(768-colors.size));im.info['transparency']=0;im.save(folder/fr['file'],transparency=0)
  preview=Image.new('RGB',(w,h),(24,32,48));rgba=im.convert('RGBA');preview.paste(rgba,(0,0),rgba);frames.append(preview)
  sheet.paste(preview,(n%4*w,n//4*h))
 sheet.save(folder/'contact-native.png');sheet.resize((sheet.width*2,sheet.height*2),Image.Resampling.NEAREST).save(folder/'contact-2x.png')
 for tag in q['tags']:
  seq=[im.resize((w*2,h*2),Image.Resampling.NEAREST) for im in frames[tag['first']:tag['last']+1]]
  times=[f['duration_ms'] for f in q['frames'][tag['first']:tag['last']+1]]
  seq[0].save(folder/(tag['name']+'.gif'),save_all=True,append_images=seq[1:],duration=times,loop=0,disposal=2)
 print(folder, 'exported exact layer pixels')
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('folder');export(ap.parse_args().folder)
