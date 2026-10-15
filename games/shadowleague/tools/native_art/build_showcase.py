"""Initial native showcase exports. Normal builds never run this command.

Creates all referenced poses in a fresh directory. It refuses to replace edited
art. Inspect the exported PNGs and real cartridge before accepting the art.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage
from showcase_ink import ORDER, palette, compose, preview
from showcase_poses import DRAW

GAME=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(GAME/'tools'))
from native_art_io import fighter_source, indexed

def engine_box(box):
    x,y,w,h=box
    return [int(x-80),int(142-y-h),int(w),int(h)]

def bounds(image):
    y,x=np.where(np.asarray(image)>0)
    return int(x.min()),int(y.min()),int(x.max()-x.min()+1),int(y.max()-y.min()+1)

def png(path,data,colors):
    im=Image.fromarray(np.asarray(data,dtype=np.uint8),'P')
    im.putpalette(colors.flatten().tolist());im.save(path,transparency=0)
    read,pal=indexed(path,im.size)
    if not np.array_equal(read,data) or not np.array_equal(pal,colors):raise ValueError(str(path)+': PNG write changed pixels/palette')

def alternate(who,colors):
    alt=colors.copy()
    sets={
      'kenji':{10:'182840',11:'386898',12:'70b0d0',13:'284848',14:'487878',15:'88b0a0',16:'d0e8d0'},
      'jack':{10:'182840',11:'305898',12:'6098d8',13:'383040',14:'605068',15:'988898',16:'d0c0c8'},
      'amara':{10:'382048',11:'704088',12:'b070b8'},
    }
    for i,rgb in sets[who].items():alt[i]=list(bytes.fromhex(rgb))
    return alt

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    summary=[];failures=[]
    for who,draw in DRAW.items():
        folder=args.output/who;folder.mkdir()
        q=json.loads((GAME/'fighters'/(who+'.json')).read_text())
        names=list(dict.fromkeys(fr[0] for seq in q['anims'].values() for fr in seq))
        colors=palette(who);frames={};previews={};hashes={};counts=[]
        for name in names:
            parts=draw(name);data=compose(parts)
            paths={}
            for layer in ORDER:
                arr=np.asarray(parts[layer].image)
                if any(edge.any() for edge in (arr[0],arr[-1],arr[:,0],arr[:,-1])):failures.append(who+'/'+name+': clipped '+layer)
                _,components=ndimage.label(arr>0,np.ones((3,3),np.uint8))
                if not 1<=components<=(2 if layer=='head_hair' else 1):failures.append(f'{who}/{name}: {layer} has {components} components')
                filename=name+'-'+layer+'.png';png(folder/filename,arr,colors);paths[layer]=filename
                alphabet='.123456789ABCDEFGHIJKLMNOPQRSTUV'
                text='\n'.join(''.join(alphabet[int(i)] for i in row) for row in arr)+'\n'
                (folder/(name+'-'+layer+'.pixels')).write_text(text)
            contact=[];centres={}
            for side in ('far_leg','near_leg'):
                a=np.asarray(parts[side].image);y,x=np.where(a>0);lowest=int(y.max())
                centres[side]=[round(float(x[y==lowest].mean()),3),lowest]
                if lowest==141:contact.append(side)
            # Landing/support metadata refers to canvas drawings; the engine's
            # airborne height still raises the complete sprite above the floor.
            hits={}
            for side in ('far','near'):
                for target,layer in [('fist','arm'),('ankle','leg')]:
                    box=parts[side+'_'+layer].contact
                    if box is None:raise ValueError(who+'/'+name+': missing contact region')
                    hits[side+'_'+target]=[engine_box(box)]
            if who=='kenji' and name=='j_knee':hits['far_ankle']=[engine_box((83,94,16,20))]
            head_box=parts['head_hair'].hurt
            if head_box is None:raise ValueError('Head needs an authored hurt region')
            legs=(np.asarray(parts['far_leg'].image)>0)|(np.asarray(parts['near_leg'].image)>0)
            hurt=[engine_box(head_box),engine_box(bounds(parts['torso'].image)),engine_box(bounds(Image.fromarray(legs.astype(np.uint8),'P')))]
            png(folder/(name+'.png'),data,colors)
            previews[name]=preview(parts,who)
            hashes[name]=hashlib.sha256(data.tobytes()).hexdigest()
            frames[name]={'file':name+'.png','layers':paths,'hurt':hurt,'hit':hits,'contacts':contact,
                          'foot_centres':centres,'root':[80,141],
                          'silhouette_bounds':list(bounds(Image.fromarray(data,'P')))}
            counts.append(int(np.count_nonzero(data)))
        # Native head crops have their own sixteen-colour indexed artwork.
        # The sole gold earring highlight is explicitly cream in this tiny view.
        from showcase_ink import head
        tiny=np.asarray(head(who).image)[16:48,64:96].copy();tiny[tiny==19]=6
        if tiny.max()>15:raise ValueError('Small portrait exceeds its own bank')
        png(folder/'head.png',tiny,colors);png(folder/'portrait-small.png',tiny,colors)
        manifest={'canvas':[160,160],'anchor':[80,141],'head':'head.png','portrait_small':'portrait-small.png',
                  'alt_palette':alternate(who,colors).tolist(),'frames':frames,
                  'source':'Editable native indexed PNG layers; initial pixel maps retained',
                  'visual_review':'playable proof; professional artistic acceptance pending'}
        (folder/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        # Run the exact production importer, including tile-bank bounds.
        imported=fighter_source(folder,names)
        sheet=Image.new('RGB',(160*8,180*((len(names)+7)//8)),(24,32,48));d=ImageDraw.Draw(sheet)
        for n,name in enumerate(names):
            x=(n%8)*160;y=(n//8)*180;sheet.paste(previews[name],(x,y));d.text((x+2,y+160),name,fill=(224,232,232))
        sheet.save(folder/'contact-native.png')
        for key,seq in q['anims'].items():
            pics=[previews[fr[0]].resize((320,320),Image.Resampling.NEAREST) for fr in seq]
            times=[int(round((fr[1] if fr[1]!=255 else 24)*1000/60)) for fr in seq]
            pics[0].save(folder/(key+'.gif'),save_all=True,append_images=pics[1:],duration=times,loop=0,disposal=2)
        summary.append({'fighter':who,'referenced_poses':len(names),'unique_drawings':len(set(hashes.values())),
                        'animations':len(q['anims']),'palette_banks':len(imported['banks']),
                        'opaque_area_range':[min(counts),max(counts)],'png_and_layer_roundtrip':'pass'})
    (args.output/'SUMMARY.json').write_text(json.dumps({'fighters':summary,'anatomy_failures':failures},indent=2)+'\n')
    if failures:raise ValueError('\n'.join(failures))
    print(json.dumps(summary,indent=2))

if __name__=='__main__':main()
