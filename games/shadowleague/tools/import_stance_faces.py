"""Crop original high-resolution stance heads for explicit selection portraits.

This tool preserves drawn pixels with nearest-neighbor export. Normal builds
consume the checked artwork and never refresh it from a machine's cache.
"""
import argparse,json,sys
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw
TOOLS=Path(__file__).resolve().parent
sys.path.insert(0,str(TOOLS/'painted_import'))
import sprites as sp
GAME=TOOLS.parent
GROUND=np.asarray((28,24,34),np.uint8)
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--jack-sources',type=Path,required=True);a=ap.parse_args()
    faces=GAME/'assets/authored/faces';faces.mkdir(exist_ok=True)
    ids=[p.stem for p in (GAME/'fighters').glob('*.json') if not (faces/(p.stem+'.png')).exists() or p.stem=='jack']
    records=[]
    for fid in ids:
        source=(a.jack_sources if fid=='jack' else GAME/'assets/authored/sdk177/sources'/fid)/(fid+'_s1.png')
        rgb=sp.load(source);figures,_=sp.figures(rgb,2);box,figure,mask=figures[0]
        h,w=mask.shape;ys,xs=np.nonzero(mask[:max(1,int(h*.09))]);cx=int(np.median(xs));side=max(32,round(h*.30));cy=round(h*.13)
        x0=cx-side//2;y0=cy-side//2
        out=np.tile(GROUND,(side,side,1));clean=figure.copy();clean[~mask]=GROUND
        left,top=max(0,x0),max(0,y0);right,bottom=min(w,x0+side),min(h,y0+side)
        out[top-y0:bottom-y0,left-x0:right-x0]=clean[top:bottom,left:right]
        Image.fromarray(out).resize((128,128),Image.Resampling.NEAREST).save(faces/(fid+'.png'))
        records.append({'fighter':fid,'source':str(source.resolve()),'figure_box':list(map(int,box)),'face_box':[x0,y0,side,side],'resampling':'nearest'})
    (GAME/'assets/authored/sdk177/face-crops.json').write_text(json.dumps(records,indent=2)+'\n')
    sheet=Image.new('RGB',(8*128,((len(ids)+7)//8)*150),(20,22,30));draw=ImageDraw.Draw(sheet)
    for k,fid in enumerate(ids):
        x=k%8*128;y=k//8*150;sheet.paste(Image.open(faces/(fid+'.png')),(x,y));draw.text((x,y+130),fid,fill='white')
    sheet.save(GAME/'assets/review/sdk177/new-faces-review.png');print(len(ids),'face sources exported')
if __name__=='__main__':main()
