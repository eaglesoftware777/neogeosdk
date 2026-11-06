"""Lossless native indexed art import and bounded tile-palette assignment.

This module consumes finished drawings. It does not construct anatomy.
"""
from pathlib import Path
import json
import numpy as np
from PIL import Image

def bounded_palette_cover(sets,limit):
    """Bounded deterministic repacking when first-fit wastes palette banks.

    States contain actual source-colour sets, never approximations. A beam caps
    search cost; failure still rejects the asset at the original hardware limit.
    """
    items=sorted({sum(1<<i for i in used) for used in sets if used},
                 key=lambda bits:(-bits.bit_count(),bits))
    states=[()]
    for item in items:
        candidates=set()
        for state in states:
            if any(item & ~bank == 0 for bank in state):
                candidates.add(state);continue
            seen=set()
            for n,bank in enumerate(state):
                merged=bank|item
                if merged.bit_count()>15 or merged in seen:continue
                seen.add(merged)
                candidates.add(tuple(sorted(state[:n]+state[n+1:]+(merged,))))
            if len(state)<limit:candidates.add(tuple(sorted(state+(item,))))
        if not candidates:return None
        states=sorted(candidates,key=lambda s:(sum(b.bit_count() for b in s)+4*len(s),len(s),s))[:512]
    if not states:return [set()]
    best=states[0]
    return [{i for i in range(256) if bank & (1<<i)} for bank in best]


def indexed(path,size=None):
    with Image.open(path) as im:
        if im.mode!='P' or (size is not None and im.size!=size):
            raise ValueError(f'{path}: expected indexed native canvas {size}')
        transparency=im.info.get('transparency')
        if transparency!=0 and not (isinstance(transparency,bytes) and transparency and transparency[0]==0):
            raise ValueError(f'{path}: index zero must be transparent')
        rgba=np.asarray(im.convert('RGBA'))
        if not np.isin(rgba[:,:,3],[0,255]).all():raise ValueError(f'{path}: partial alpha')
        data=np.asarray(im,dtype=np.uint8).copy()
        if not np.array_equal(rgba[:,:,3]==0,data==0):raise ValueError(f'{path}: transparency must be exactly index zero')
        colors=np.asarray(im.getpalette(),dtype=np.uint8).reshape(-1,3)
        if data.max()>=len(colors):raise ValueError(f'{path}: missing palette color')
        return data,colors

def palette_tiles(data,colors,limit=4,with_indices=False):
    """Map each 16x16 tile without changing a single source color or pixel.

    Reject an overfull tile or palette budget; never quantize to force a fit.
    Bank creation is deterministic across identical inputs.
    """
    if data.ndim!=2 or any(n%16 for n in data.shape):raise ValueError('Native art dimensions must be multiples of 16')
    sets=[];positions=[]
    for y in range(0,data.shape[0],16):
        for x in range(0,data.shape[1],16):
            used=set(int(i) for i in np.unique(data[y:y+16,x:x+16]) if i)
            if len(used)>15:raise ValueError(f'Tile ({x//16},{y//16}) has {len(used)} opaque colors')
            sets.append(used);positions.append((y,x))
    banks=[]
    for used in sorted(sets,key=lambda s:(-len(s),tuple(sorted(s)))):
        if any(used<=b for b in banks):continue
        candidates=[(len(b|used),i) for i,b in enumerate(banks) if len(b|used)<=15]
        if candidates:
            _,i=min(candidates);banks[i]|=used
        else:banks.append(set(used))
    if len(banks)>limit:
        repacked=bounded_palette_cover(sets,limit)
        if repacked is None:raise ValueError(f'Artwork needs {len(banks)} palette banks; limit is {limit}')
        banks=repacked
    if not banks:banks=[set()]
    result=np.zeros_like(data);assignment=np.zeros((data.shape[0]//16,data.shape[1]//16),np.uint8);palettes=[]
    for bank in banks:
        pal=np.zeros((16,3),np.uint8)
        for i,index in enumerate(sorted(bank),1):pal[i]=colors[index]
        palettes.append(pal)
    for (y,x),used in zip(positions,sets):
        b=next(i for i,bank in enumerate(banks) if used<=bank);assignment[y//16,x//16]=b
        lookup=np.zeros(256,np.uint8)
        for i,index in enumerate(sorted(banks[b]),1):lookup[index]=i
        result[y:y+16,x:x+16]=lookup[data[y:y+16,x:x+16]]
    output=(result,np.asarray(palettes),assignment)
    return output+([([0]+sorted(bank)+[0]*(15-len(bank))) for bank in banks],) if with_indices else output


def fighter_source(folder,names):
    folder=Path(folder);manifest=json.loads((folder/'manifest.json').read_text())
    frames=manifest['frames'];result=[];metadata=[];colors=None
    canvas=manifest.get('canvas');anchor=manifest.get('anchor')
    if canvas not in ([160,160],[192,192]) or anchor!=[canvas[0]//2,canvas[1]-19]:raise ValueError('Authored fighter canvas/anchor mismatch')
    width,height=canvas;baseline=anchor[1]
    from scipy import ndimage
    from PIL import Image
    required_layers={'far_arm','far_leg','torso','head_hair','near_leg','near_arm'}
    for name in names:
        if name not in frames:raise ValueError(f'{folder}: missing authored pose {name}; procedural fallback forbidden')
        fr=frames[name];data,pal=indexed(folder/fr['file'],tuple(canvas))
        if colors is not None and not np.array_equal(pal,colors):raise ValueError('Fighter palette changes between poses')
        colors=pal
        if any(edge.any() for edge in (data[0],data[-1],data[:,0],data[:,-1])):raise ValueError(name+': clipped source')
        if not fr.get('hurt') or 'hit' not in fr:raise ValueError(name+': missing authored collision metadata')
        if 'contacts' not in fr:raise ValueError(name+': missing foot contact metadata')
        root=fr.get('root',anchor)
        if len(root)!=2 or not (0<=root[0]<width and 0<=root[1]<height):raise ValueError(name+': frame anchor outside canvas')
        for layer in fr['contacts']:
            if layer not in ('far_leg','near_leg'):raise ValueError(name+': invalid planted-foot layer')
        if 'layers' in fr:
            # Layered drawings: each limb is one piece and the layers flatten to the export.
            if set(fr['layers'])!=required_layers:raise ValueError(name+': six anatomy layers required')
            flat=np.zeros_like(data)
            for layer in ('far_arm','far_leg','torso','head_hair','near_leg','near_arm'):
                part,part_pal=indexed(folder/fr['layers'][layer],tuple(canvas))
                if not np.array_equal(part_pal,pal):raise ValueError(name+': layer palette mismatch')
                _,components=ndimage.label(part>0,np.ones((3,3),np.uint8))
                if not 1<=components<=(2 if layer=='head_hair' else 1):raise ValueError(name+': disconnected '+layer)
                flat[part>0]=part[part>0]
            if not np.array_equal(flat,data):raise ValueError(name+': flattening differs from source layers')
            for layer in fr['contacts']:
                leg,_=indexed(folder/fr['layers'][layer],tuple(canvas))
                if np.where(leg>0)[0].max()!=baseline:raise ValueError(name+f': sole misses baseline {baseline}')
        elif fr['contacts'] and np.where(data>0)[0].max()!=baseline:
            # Single-layer drawings: a planted pose stands on the baseline.
            raise ValueError(name+f': sole misses baseline {baseline}')
        for boxes in [fr['hurt']]+list(fr['hit'].values()):
            if len(boxes)>3:raise ValueError(name+': more than three collision boxes')
            for box in boxes:
                if len(box)!=4 or not (-128<=box[0]<=127 and -128<=box[1]<=127 and 1<=box[2]<=255 and 1<=box[3]<=255):raise ValueError(name+': invalid hardware collision rectangle')
        result.append(data);metadata.append(fr)
    local,banks,tile_maps,indices=palette_tiles(np.vstack(result),colors,limit=4,with_indices=True)
    return {'canvas':canvas,'anchor':anchor,'images':[local[i*height:(i+1)*height] for i in range(len(result))],
            'source_images':result,'source_palette':colors,'metadata':metadata,
            'banks':banks,'bank_indices':indices,
            'tile_maps':[tile_maps[i*(height//16):(i+1)*(height//16)] for i in range(len(result))]}
