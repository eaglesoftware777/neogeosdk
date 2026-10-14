"""Native indexed sources, exact hardware indices, stable pivots and append-only IDs."""
import json
from pathlib import Path
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]/'assets/authored'

def indexed(path,banks=1):
    with Image.open(path) as im:
        if im.mode!='P' or im.info.get('transparency')!=0:
            raise ValueError(f'{path}: native source must be indexed with transparent slot zero')
        indices=np.asarray(im).copy()
        if indices.max()>15:raise ValueError(f'{path}: exceeds one hardware palette bank')
        colors=np.array(im.getpalette()[:48],np.uint8).reshape(16,3)
        rgba=im.convert('RGBA')
    h,w=indices.shape
    if h%16 or w%16:raise ValueError(f'{path}: incomplete tile canvas')
    return rgba,(indices,np.array([colors.copy() for _ in range(banks)],np.uint8),np.zeros((h//16,w//16),np.uint8))

PAINTED=ROOT.parent/'painted'

def shadow_source(path):
    """A painted unit or jet brings its own ground shadow."""
    name=path.name
    for alt in (PAINTED/'sprites'/name,):
        if alt.exists():return alt
    return path

def load():
    overrides={};extra=[];pivots={};provenance={};native_banks={}
    models=json.loads((ROOT/'aircraft_models.json').read_text())
    def put(name,path,bank,pivot=(24,24),banks=1,append=False):
        image,table=indexed(path,banks)
        overrides[name]=(image,table);native_banks[name]=bank
        pivots[name]=pivot;provenance[name]=str(path.relative_to(ROOT.parent.parent))
        if append:extra.append((name,image,bank,banks))
    for pilot,m in enumerate(models):
        bank=44+pilot*4;frames=m['frames']
        def pose(n):return ROOT/'frames'/f"aircraft_{m['name']}__{frames[n]['name']}.png"
        # All legacy IDs remain fixed, including the four special-maneuver slots.
        prefix=f'p{pilot+1}_plane'
        for name,n in ((prefix,3),(prefix+'_left',0),(prefix+'_right',6),
                       (prefix+'_left_mid',1),(prefix+'_left_soft',2),
                       (prefix+'_right_soft',4),(prefix+'_right_mid',5)):
            put(name,pose(n),bank,banks=4)
        for i in range(4):put(f'maneuver_{pilot}_{i}',pose(23+i),bank,banks=4)
        for i in range(7):put(f'native_damage_{pilot}_{i}',pose(12+i),bank,banks=4,append=True)
        for i in range(2):put(f'native_boost_{pilot}_{i}',pose(10+i),bank,banks=4,append=True)
        for i in range(4):put(f'native_death_{pilot}_{i}',pose(19+i),bank,banks=4,append=True)
        # Export the explicitly authored hidden shadow layer; do not synthesize
        # a runtime alpha-mask or modify aircraft pixels in the importer.
        for i in range(7):
            path=ROOT/'frames'/f"shadow_{m['name']}_{i}.png"
            painted=PAINTED/'jets'/m['name']/f'shadow_{i}.png'
            if painted.exists():path=painted
            if path.exists():put(f'native_shadow_{pilot}_{i}',path,114,append=True)
    content=ROOT/'production_assets.json'
    if content.exists():
        for a in json.loads(content.read_text()):
            put(a['name'],ROOT/a['png'],a['bank'],a['pivot'],a.get('banks',1),a.get('append',False))
            if a['frame']==1 and a['kind'] in ('enemy','miniboss','boss'):
                put('native_unit_shadow_'+a['name'],shadow_source(ROOT/'frames'/('shadow_'+a['name']+'.png')),114,a['pivot'],append=True)
    return overrides,extra,pivots,provenance,native_banks
