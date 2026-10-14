#!/usr/bin/env python3
"""Strict indexed anatomy/animation gate for Maiya's saved Aseprite source.

Checks rendered exports and every visible limb layer. Always writes review
previews before reporting failures. These checks accompany visual art review;
connected components alone cannot prove good anatomy.

Her walk and run share one upper body (head, rose, ponytail and tunic)
from the stride drawing on her original heroine sheet, moved only by
whole-pixel bob; her arms swing between that stride's reach and the same
sheet's passing figure's hanging arms; her legs are native drawings cut
from the WALK/RUN sheets. So: the head and tunic must be that stride's own
pixels in every frame, both boots must keep their weight (thin stick legs
fail), the fists must really swing (and alternate near and far) but each
fist and each foot may move only so far from one frame to the next (no arm
flicking, no leg popping), and the feet must meet the ground line on every
grounded frame.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import sys
import numpy as np
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1]
ART=ROOT/'games/maiya/assets/authored'
LAYERS=('far_arm','far_leg','near_leg','torso','head_hair','near_arm')
BOOT=(5,7,11,13)      # boot browns, near and far ramps
SKIN=(6,9)
BOOT_MIN=45           # visible boot pixels per leg (the earlier thin-legged drawings had 60-88 for both boots)
FIST_STEP=7           # largest fist move between consecutive frames, in pixels (the run's 2 px bob adds to its swing)
FIST_SWING=6          # least distance each fist travels over a step (arms that swing)
FOOT_STEP=16          # largest boot move between consecutive frames, in pixels
AREA_SLACK=.18        # opaque area against the stride (her slim arms fold over the tunic at the down and stop poses; at the run's down, one crosses her back and the other passes behind her chest)
SHOULDER={'far_arm':(45,25),'near_arm':(47,26)}   # where each arm joins her, at bob 0 (compose_gait.py)

def components(mask):
    seen=np.zeros(mask.shape,bool);result=[];h,w=mask.shape
    for y,x in zip(*np.nonzero(mask)):
        if seen[y,x]:continue
        pending=[(int(y),int(x))];seen[y,x]=True;points=[]
        while pending:
            u,v=pending.pop();points.append((u,v))
            for dy in (-1,0,1):
                for dx in (-1,0,1):
                    a,b=u+dy,v+dx
                    if 0<=a<h and 0<=b<w and mask[a,b] and not seen[a,b]:
                        seen[a,b]=True;pending.append((a,b))
        result.append(points)
    return result

def bounds(mask):
    y,x=np.nonzero(mask)
    return (int(x.min()),int(y.min()),int(x.max()),int(y.max())) if len(x) else None

def unbob(a,bob):
    out=np.zeros_like(a)
    if bob>=0:out[:a.shape[0]-bob]=a[bob:]
    else:out[-bob:]=a[:bob]
    return out

def speed(mode):
    # gait clock units per frame: a walk covers 64 px of road per 80 units
    return 800/256 if mode=='run' else 512/256*80/64

def previews(poses,flats,palette):
    dest=ART/'review';dest.mkdir(exist_ok=True)
    cols=max(p['slot'] for p in poses)+1
    sheet=Image.new('RGB',(cols*320,3*280),(36,43,55));draw=ImageDraw.Draw(sheet)
    for p,a in zip(poses,flats):
        im=Image.fromarray(a,'P');im.putpalette(sum(palette,[])+[0]*720);im.info['transparency']=0
        rgba=im.convert('RGBA').resize((320,256),Image.Resampling.NEAREST)
        row={'walk':0,'run':1,'stop':2}[p['mode']];pos=(p['slot']*320,row*280+20)
        sheet.paste(rgba,pos,rgba);draw.text((pos[0]+10,row*280+3),p['name'],fill='white')
    for i,name in enumerate(('reference_stride','reference_idle0')):
        im=Image.open(ART/(name+'.png')).convert('RGBA').resize((320,256),Image.Resampling.NEAREST)
        sheet.paste(im,((i+1)*320,580),im);draw.text(((i+1)*320+10,563),name,fill='white')
    sheet.save(dest/'contact-sheet-4x.png')
    for mode in ('walk','run'):
        frames=[];times=[]
        for p,a in zip(poses,flats):
            if p['mode']!=mode:continue
            im=Image.fromarray(a,'P');im.putpalette(sum(palette,[])+[0]*720)
            frames.append(im.resize((320,256),Image.Resampling.NEAREST))
            times.append(round(p['hold_distance']/speed(mode)*1000/60))
        frames[0].save(dest/(mode+'.gif'),save_all=True,append_images=frames[1:],duration=times,loop=0,transparency=0,disposal=2)

def check(export=True):
    if export:
        spec=importlib.util.spec_from_file_location('maiya_aseprite_pipeline',ROOT/'games/maiya/tools/aseprite/pipeline.py')
        pipeline=importlib.util.module_from_spec(spec);spec.loader.exec_module(pipeline);pipeline.export()
    palette=json.loads((ART/'palette.json').read_text());poses=json.loads((ART/'poses.json').read_text());source=json.loads((ART/'aseprite_source.json').read_text())
    errors=[];metrics=[];flats=[]
    def need(ok,message):
        if not ok:errors.append(message)
    count={m:sum(1 for p in poses if p['mode']==m) for m in ('walk','run','stop')}
    need(count['walk']==8 and count['run']==8 and count['stop']==1,f'poses: expected walk 8, run 8, stop 1, got {count}')
    tags=[];first=1
    for m in ('walk','run','stop'):tags.append({'name':m,'from':first,'to':first+count[m]-1});first+=count[m]
    need([p['mode'] for p in poses]==['walk']*count['walk']+['run']*count['run']+['stop']*count['stop'],'poses: walk, run, stop must be in ROM order')
    need(source['indexed'] and (source['width'],source['height'])==(80,64),'source: indexed 80x64 required')
    need(source['transparentColor']==0,'source: transparentColor must be zero')
    need(source['colors']==[c+[0 if i==0 else 255] for i,c in enumerate(palette['colors'])],'source: embedded palette or alpha differs')
    need([x['name'] for x in source['layers']]==['ref',*LAYERS],'source: incorrect layer order')
    need(all(not x['background'] for x in source['layers']),'source: Background layer forbidden')
    need(not source['layers'][0]['visible'] and not source['layers'][0]['editable'],'source: ref must be hidden and locked')
    need(source['tags']==tags,'source: incorrect tags/frame counts')
    def read(path,size):
        with Image.open(path) as im:
            need(im.mode=='P',f'{path.name}: mode must be P');need(im.size==size,f'{path.name}: canvas mismatch')
            a=np.array(im);need(a.ndim==2,f'{path.name}: indexed image required')
            if a.ndim!=2:return np.zeros((size[1],size[0]),np.uint8)
            need(a.max()<=15,f'{path.name}: palette bank overflow')
            need(im.getpalette()[:48]==sum(palette['colors'],[]),f'{path.name}: off-palette colours')
            alpha=np.array(im.convert('RGBA'))[:,:,3]
            need(np.all((alpha==0)|(alpha==255)),f'{path.name}: partial alpha')
            need(np.all(alpha[a==0]==0),f'{path.name}: index 0 opaque')
            need(np.all(alpha[a!=0]==255),f'{path.name}: opaque index has alpha')
            return a
    for mode,n in count.items():read(ART/f'maiya_{mode}.png',(80*n,64))
    ref=read(ART/'reference_stride.png',(80,64))
    refarea=int(np.count_nonzero(ref))
    shape={}
    for p in poses:
        name=p['name'];bob=p['bob'];layers={l:read(ART/'layers'/f'{name}__{l}.png',(80,64)) for l in LAYERS}
        flat=read(ART/'layers'/f'{name}__flat.png',(80,64));flats.append(flat);merged=np.zeros_like(flat);owner=np.full(flat.shape,-1,np.int8)
        cc={}
        for li,(l,a) in enumerate(layers.items()):
            merged[a!=0]=a[a!=0];owner[a!=0]=li;c=components(a!=0);cc[l]=len(c)
            limit=2 if l=='head_hair' else 1
            need(1<=len(c)<=limit,f'{name}/{l}: {len(c)} connected components (limit {limit})')
            need(all(len(x)>=3 for x in c),f'{name}/{l}: orphan cluster below 3 pixels')
        need(np.array_equal(flat,merged),f'{name}: flattened pixels differ from visible layers')
        need(len(components(flat!=0))==1,f'{name}: her figure is not one connected drawing')
        b=bounds(flat!=0);height=b[3]-b[1]+1
        need(abs(height-palette['run_h'])<=2,f'{name}: drawn height {height}, expected {palette["run_h"]} +/- 2')
        need(abs(bob)<=2,f'{name}: bob {bob} beyond 2 pixels')
        area=int(np.count_nonzero(flat));need(abs(area-refarea)<=refarea*AREA_SLACK,f'{name}: area {area} differs >{AREA_SLACK:.0%} from run0 {refarea}')
        need(not(np.any(flat[0]) or np.any(flat[-1]) or np.any(flat[:,0]) or np.any(flat[:,-1])),f'{name}: clipped at canvas edge')
        # One upper body: her head and tunic are the stride's own pixels,
        # moved only by the pose's bob, and the same pixels in every frame.
        for l in ('head_hair','torso'):
            a=unbob(layers[l],bob);lit=a!=0
            need(np.any(lit) and np.array_equal(a[lit],ref[lit]),f'{name}/{l}: pixels differ from her stride')
            if l in shape:need(np.array_equal(lit,shape[l]),f'{name}/{l}: not the same drawing as the first frame')
            else:shape[l]=lit
        # Legs: two booted legs with their weight, standing on the line.
        boots={}
        for l in ('far_leg','near_leg'):
            visible=(owner==LAYERS.index(l))&np.isin(flat,BOOT)
            boots[l]=int(np.count_nonzero(visible))
            need(boots[l]>=BOOT_MIN,f'{name}/{l}: {boots[l]} boot pixels visible, need {BOOT_MIN} (legs too thin)')
        for l in ('far_leg','near_leg'):
            b_=np.isin(layers[l],BOOT);ys_,xs_=np.nonzero(b_)
            p.setdefault('_boot',{})[l]=float(xs_[ys_>=ys_.max()-6].mean())
        legs=(layers['far_leg']!=0)|(layers['near_leg']!=0)
        sole=bounds(legs)[3]
        if p['grounded']:need(sole==palette['baseline'],f'{name}: soles at y={sole}, ground line is {palette["baseline"]}')
        else:need(sole<=palette['baseline']-3,f'{name}: airborne frame touches down (soles at y={sole})')
        # Arms: the near arm always shows; the far one passes behind her
        # body at the passing pose and, swung forward, mostly behind her
        # chest (she is turned three-quarters), but some of it still shows.
        # A fist is the arm's far end from its shoulder.
        for l in ('far_arm','near_arm'):
            a=layers[l];shown=int(np.count_nonzero((owner==LAYERS.index(l))&(a!=0)))
            if l=='near_arm':need(shown>=12,f'{name}/{l}: arm hidden ({shown} pixels show)')
            elif p['swing']!=2:need(shown>=8,f'{name}/{l}: arm hidden ({shown} pixels show)')
            ys,xs=np.nonzero(a);sx,sy=SHOULDER[l];sy+=bob
            d=np.hypot(xs-sx,ys-sy);keep=d>=d.max()-3
            p.setdefault('_fist',{})[l]=(float(xs[keep].mean()),float(ys[keep].mean()))
        metrics.append({'pose':name,'bounds':b,'height':height,'area':area,'bob':bob,'layer_components':cc,'boots':boots,'boot_x':{k:round(v,1) for k,v in p['_boot'].items()},'fists':{k:[round(v,1) for v in xy] for k,xy in p['_fist'].items()}})
    for mode in ('walk','run'):
        indices=[i for i,p in enumerate(poses) if p['mode']==mode]
        for i,j in zip(indices,indices[1:]+indices[:1]):
            a,b=flats[i]!=0,flats[j]!=0;difference=np.count_nonzero(a^b)/np.count_nonzero(a|b)
            need(difference>=.08,f'{poses[i]["name"]} -> {poses[j]["name"]}: silhouette difference {difference:.1%} below 8%')
            for side in ('far_arm','near_arm'):
                (x1,y1),(x2,y2)=poses[i]['_fist'][side],poses[j]['_fist'][side]
                need(max(abs(x2-x1),abs(y2-y1))<=FIST_STEP,f'{poses[i]["name"]} -> {poses[j]["name"]}: {side} fist jumps {max(abs(x2-x1),abs(y2-y1)):.1f} px (limit {FIST_STEP})')
            # The near layer is always the same leg (and the far layer the
            # other), so each layer's boot traces one foot's path.
            for l in ('far_leg','near_leg'):
                jump=abs(poses[j]['_boot'][l]-poses[i]['_boot'][l])
                need(jump<=FOOT_STEP,f'{poses[i]["name"]} -> {poses[j]["name"]}: {l} boot jumps {jump:.1f} px (limit {FOOT_STEP})')
        half=len(indices)//2
        for side in ('far_arm','near_arm'):
            xs=[poses[k]['_fist'][side][0] for k in indices[:half]];ys=[poses[k]['_fist'][side][1] for k in indices[:half]]
            travel=max(max(xs)-min(xs),max(ys)-min(ys))
            need(travel>=FIST_SWING,f'{mode}: {side} fist travels only {travel:.1f} px over a step (arms hardly swing)')
        for i in range(half):
            a,b=poses[indices[i]],poses[indices[i+half]]
            need(a['near_arm']!=b['near_arm'],f'{mode}: {a["name"]} and {b["name"]} swing the same arm forward')
            need(a['near_leg']==b['far_leg'] and a['far_leg']==b['near_leg'],f'{mode}: {a["name"]} and {b["name"]} do not exchange legs')
        need(sum(p['hold_distance'] for p in poses if p['mode']==mode)==80,f'{mode}: hold distances must total 80 pixels')
    for p in poses:p.pop('_fist',None);p.pop('_boot',None)
    previews(poses,flats,palette['colors'])
    result={'passed':not errors,'errors':errors,'poses':metrics,'review':str(ART/'review')};(ART/'check-result.json').write_text(json.dumps(result,indent=2)+'\n')
    for error in errors:print('FAIL:',error)
    print(f'Hero art gate: {"PASS" if not errors else "FAIL"} ({len(poses)} frames, {len(errors)} failures). Previews: {ART / "review"}')
    return not errors

def check_rom():
    """Decode the actual builder C lanes and compare authored/legacy pixels."""
    import hashlib
    import re
    sys.path.insert(0,str(ROOT/'artbox'))
    from tile_codec import decode_image
    generated=ROOT/'games/maiya/artbox/generated'
    header=(generated/'maiya_assets.h').read_text()
    def array(name):
        body=re.search(name+r'\[.*?\]\s*=\s*\{(.*?)\}',header,re.S).group(1)
        return [int(x,0) for x in re.findall(r'0x[0-9a-fA-F]+|\d+',body)]
    tiles=array('mg_hero_tiles')
    lanes=[(generated/f'780-{c}.{c}').read_bytes() for c in ('c1','c2')]
    old=json.loads((ART/'legacy_manifest.json').read_text())
    for p in old['frames']:
        define=f"#define MG_F_{p['name'].upper()} {p['id']}u"
        if define not in header:raise ValueError('Legacy frame ID changed: '+p['name'])
        png=generated/f"hero_{p['name']}.png"
        if hashlib.sha256(png.read_bytes()).hexdigest()!=p['png_sha256']:
            raise ValueError('Legacy drawing changed: '+p['name'])
        start=tiles[p['id']]*64
        for lane,key in zip(lanes,('c1_sha256','c2_sha256')):
            if hashlib.sha256(lane[start:start+20*64]).hexdigest()!=p[key]:
                raise ValueError('Legacy C tiles changed: '+p['name'])
    for name,words in old['palettes'].items():
        if array(name)!=words:raise ValueError('Existing hero/Luna palette changed: '+name)
    decoded=[]
    for i,p in enumerate(json.loads((ART/'poses.json').read_text()),60):
        mode=p['mode'];slot=p['slot'];name=f'gait_{mode}{slot}'
        if f"#define MG_F_{name.upper()} {i}u" not in header:raise ValueError('Appended gait frame ID differs: '+name)
        start=tiles[i]*64
        # The builder writes final ROM wiring by swapping every byte pair.
        actual=decode_image(lanes[0][start:start+1280],lanes[1][start:start+1280],80,64,swapped=True)
        expected=np.array(Image.open(ART/'layers'/f"{p['name']}__flat.png"))
        if not np.array_equal(actual,expected):raise ValueError('C-ROM pixel indices differ: '+name)
        decoded.append(name)
    result={'passed':True,'legacy_frames_byte_identical':len(old['frames']),
            'unchanged_palettes':list(old['palettes']),'authored_frames_roundtripped':decoded}
    (ART/'rom-check-result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(f'C-ROM gate: PASS ({len(old["frames"])} legacy frames match legacy_manifest.json, {len(decoded)} authored frames pixel-exact, Maiya/Luna palettes unchanged)')
    return True

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--no-export',action='store_true',help='inspect existing evidence without reopening source');parser.add_argument('--rom-only',action='store_true');args=parser.parse_args()
    sys.exit(0 if (check_rom() if args.rom_only else check(not args.no_export)) else 1)
