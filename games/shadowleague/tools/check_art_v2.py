"""Reject clipped, disconnected or off-palette fighter artwork before packaging."""
import json
from pathlib import Path
import numpy as np
from scipy import ndimage
from PIL import Image

GAME=Path(__file__).resolve().parents[1]
CONNECT=np.ones((3,3),dtype=np.uint8)


def decode_tiles(c1,c2):
    """Decode the final byte-swapped cartridge lanes, not the source PNGs."""
    a=np.frombuffer(c1,dtype=np.uint8).reshape(-1,4,8,2)
    b=np.frombuffer(c2,dtype=np.uint8).reshape(-1,4,8,2)
    shifts=np.arange(8,dtype=np.uint8)
    values=((a[...,0,None]>>shifts)&1)|(((a[...,1,None]>>shifts)&1)<<1)
    values|=(((b[...,0,None]>>shifts)&1)<<2)|(((b[...,1,None]>>shifts)&1)<<3)
    tiles=np.zeros((len(a),16,16),dtype=np.uint8)
    for k,(x,y) in enumerate(((8,0),(8,8),(0,0),(0,8))):tiles[:,y:y+8,x:x+8]=values[:,k]
    return tiles


def check(fighters,layout=None,c1=None,c2=None):
    failures=[];frames=0;palettes=0;limbs=0;contacts=0
    decoded=decode_tiles(c1,c2) if c1 is not None else None
    layouts={f['id']:f for f in layout} if layout is not None else {}
    for f in fighters:
        if len(f.sprite_names)>255:failures.append(f'{f.id}: sprite index overflow')
        normal,alt=f.palettes()
        if normal.shape!=(16,3) or alt.shape!=(16,3):failures.append(f'{f.id}: palette size')
        if f.authored is None:
            for group in ('heads','portraits'):
                p=GAME/'assets/authored'/group/(f.id+'.png');im=Image.open(p)
                if im.mode!='P' or im.info.get('transparency')!=0 or max(im.tobytes())>15:
                    failures.append(f'{f.id}: invalid indexed {group}')
                if im.getpalette()[:48]!=normal.flatten().tolist():failures.append(f'{f.id}: {group} palette differs')
                palettes+=1
            # The two contacts exchange which side reaches forward, without mirroring the face.
            a=f.poses['v2_step0'];b=f.poses['v2_step3']
            if not(a['far_foot'][0]>a['near_foot'][0] and b['near_foot'][0]>b['far_foot'][0]):
                failures.append(f'{f.id}: lead foot did not alternate')
        else:
            from native_art_io import indexed
            manifest=json.loads((f.authored_folder/'manifest.json').read_text())
            for key,size in [('head',(32,32)),('portrait_small',(32,32))]:
                data,pal=indexed(f.authored_folder/manifest[key],size)
                if data.max()>15 or not np.array_equal(pal[:16],normal):failures.append(f.id+': authored '+key+' palette differs')
                palettes+=1
        for i,(name,image,j) in enumerate(zip(f.sprite_names,f.images,f.joints)):
            label=f'{f.id}/{name}';frames+=1
            if image.shape!=tuple(reversed(f.canvas)) or image.dtype!=np.uint8 or image.max()>15:
                failures.append(label+': native indexed canvas')
            if image[0].any() or image[-1].any() or image[:,0].any() or image[:,-1].any():
                failures.append(label+': clipped canvas edge')
            lab,n=ndimage.label(image>0,CONNECT);sizes=np.bincount(lab.ravel())[1:]
            if (sizes<3).any():failures.append(label+': detached small pixel cluster')
            layers = j['art_layers'] if f.authored is None else {
                ('head' if part=='head_hair' else part):np.asarray(Image.open(f.authored_folder/path))>0
                for part,path in f.authored[i].get('layers',{}).items()}
            for part,mask in layers.items():
                _,n=ndimage.label(mask,CONNECT)
                if n<1 or n>(2 if part=='head' else 1):failures.append(label+': disconnected '+part)
                if part in ('far_arm','near_arm','far_leg','near_leg'):limbs+=1
            if f.authored is None and name in ('stance','v2_step0','v2_step3'):
                side='far' if name=='v2_step0' else 'near'
                ys=np.where(j['art_layers'][side+'_leg'])[0]
                if max(ys)!=141:failures.append(label+': planted sole missed baseline 141')
                contacts+=1
            if decoded is not None:
                actual=np.zeros_like(image)
                for slot,tile,bits in layouts[f.id]['frames'][i]['strips']:
                    tile |= (bits >> 4) << 16
                    row,col=divmod(slot,f.canvas[0]//16)
                    actual[row*32:row*32+16,col*16:col*16+16]=decoded[tile]
                    actual[row*32+16:row*32+32,col*16:col*16+16]=decoded[tile+1]
                if not np.array_equal(actual,image):failures.append(label+': C-ROM pixel mismatch')
                if f.authored is not None:
                    banks,_ = f.palette_banks()
                    rgb=np.zeros((*image.shape,3),np.uint8)
                    for slot,tile,bits in layouts[f.id]['frames'][i]['strips']:
                        tile |= (bits >> 4) << 16
                        row,col=divmod(slot,f.canvas[0]//16)
                        for half,bank in enumerate((bits&3,(bits>>2)&3)):
                            y=row*32+half*16;x=col*16
                            if bank>=len(banks):failures.append(label+': palette assignment overflow');continue
                            if bank!=f.tile_banks[i][y//16,x//16]:failures.append(label+': tile palette assignment differs')
                            rgb[y:y+16,x:x+16]=banks[bank][decoded[tile+half]]
                    expected=f.source_palette[f.source_images[i]].copy();expected[f.source_images[i]==0]=0
                    if not np.array_equal(rgb,expected):failures.append(label+': C-ROM source-color mismatch')
    report={'passed':not failures,'fighters':len(fighters),'frames':frames,'limb_layers_checked':limbs,
            'indexed_assets_checked':palettes,'ground_contacts_checked':contacts,
            'canvas':[160,160],'baseline':141,'C_ROM_round_trip':decoded is not None,'failures':failures}
    (GAME/'artbox/generated/art-check-v2.json').write_text(json.dumps(report,indent=2)+'\n')
    if failures:raise ValueError('\n'.join(failures[:30])+f'\n{len(failures)} artwork gate failures')
    print(f'Art gates PASS: {len(fighters)} fighters, {frames} frames, {limbs} intact limb layers; C-ROM round trip {decoded is not None}')
    return report


if __name__=='__main__':
    import fighter_cook
    art=GAME/'artbox'
    check(fighter_cook.load_all(),json.loads((art/'generated/fighter-layout.json').read_text()),
          (art/'781-c1.c1').read_bytes(),(art/'781-c2.c2').read_bytes())
