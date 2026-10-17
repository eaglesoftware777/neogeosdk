"""Pack completed native exports, preserving every indexed source pixel."""
import json,struct
from pathlib import Path
import numpy as np
from native_art_io import indexed,palette_tiles
from palette_banks import palette_words,quantize
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]/'assets/authored/rebirth'
GENERATED=Path(__file__).resolve().parents[1]/'artbox/generated'
CARD_BYTES=920
CARD_BANKS=16

def tables(place):
    lines=[];packed=[]
    for name,size in [('top',(320,32)),('bottom',(320,16))]:
        data,colors=indexed(ROOT/'hud'/f'{name}.png',size)
        if data.max()>15:raise ValueError('HUD plate bank overflow')
        tile=place(data);lines.append(f'#define SLF_HUD_{name.upper()}_TILE {tile}u')
        packed.append(('hud-'+name,data,tile))
        if name=='top':lines.append('static const uint16_t slf_hud_plate_pal[16] = {'+','.join(str(v) for v in palette_words(colors[:16]))+'};')
    for name,size in [('logo',(288,96)),('subtitle',(128,16))]:
        data,colors=indexed(ROOT/'identity'/f'{name}.png',size)
        if data.max()>15:raise ValueError(name+': identity bank overflow')
        tile=place(data);lines.append(f'#define SLF_REBIRTH_{name.upper()}_TILE {tile}u')
        packed.append((name,data,tile))
        if name=='logo':lines.append('static const uint16_t slf_rebirth_identity_pal[16] = {'+','.join(str(v) for v in palette_words(colors[:16]))+'};')
    tiles=[];palettes=[]
    for name in ('kenji','jack','amara'):
        data,colors=indexed(ROOT/'portraits'/f'{name}.png',(80,96));tile=place(data)
        if data.max()>15:raise ValueError(name+': portrait bank overflow')
        tiles.append(tile);palettes.append(palette_words(colors[:16]));packed.append((name+'-portrait',data,tile))
    lines.append('static const uint32_t slf_rebirth_portrait_tiles[3] = {'+','.join(str(v) for v in tiles)+'};')
    lines.append('static const uint16_t slf_rebirth_portrait_pals[3][16] = {'+','.join('{'+','.join(str(v) for v in p)+'}' for p in palettes)+'};')
    project=json.loads((ROOT/'cinema/project.json').read_text())
    cards=[{'file':project['title'],'duration':360,'caption':['','']}]+project['cards']
    story=len(cards);backdrops=project.get('backdrops',[]);cards+=backdrops
    payloads=[bytearray(),bytearray()];offsets=[];descriptors=[]
    for number,card in enumerate(cards):
        painted=ROOT/'cinema/sources'/card['file']
        if painted.exists():
            # The full-colour painting, sixteen palettes chosen tile by tile;
            # the indexed card beside it is rewritten from the result.
            rgba=np.asarray(Image.open(painted).convert('RGBA'))
            if rgba.shape[:2]!=(224,352):raise ValueError(card['file']+': painted card must be 352x224')
            data,banks,mapping=quantize(rgba,bank_limit=CARD_BANKS)
            glob=np.where(data>0,mapping.repeat(16,0).repeat(16,1).astype(np.int32)*15+data,0).astype(np.uint8)
            flat=[0,0,0]+[int(v) for b in banks for v in b[1:].flatten()]
            out=Image.fromarray(glob,'P');out.putpalette(flat+[0]*(768-len(flat)))
            out.save(ROOT/'cinema'/card['file'],transparency=0)
        source,colors=indexed(ROOT/'cinema'/card['file'],(352,224))
        if not painted.exists():data,banks,mapping=palette_tiles(source,colors,limit=CARD_BANKS)
        tile=place(data);packed.append((card['file'],data,tile))
        words=[word for bank in banks for word in palette_words(bank)];words += [0]*(CARD_BANKS*16-len(words))
        caption=b''
        if len(card['caption'])!=2 or not 1<=card['duration']<=600:raise ValueError('Invalid story timing/caption')
        for text in card['caption']:
            encoded=text.encode('ascii')
            if len(encoded)>38:raise ValueError('Story caption exceeds FIX safe width')
            caption+=encoded.ljust(40,b'\0')
        descriptor=struct.pack('>IHBB',tile,card['duration'],len(banks),22)
        descriptor+=struct.pack('>%dH'%(CARD_BANKS*16),*words)+mapping.tobytes().ljust(320,b'\0')+caption
        if len(descriptor)!=CARD_BYTES:raise ValueError('Presentation descriptor layout mismatch')
        bank=number&1;offsets.append(0x100000+bank*0x100000+len(payloads[bank]));payloads[bank].extend(descriptor)
        descriptors.append({'file':card['file'],'tile':tile,'palette_banks':len(banks),'P_offset':offsets[-1]})
        actual=np.zeros((224,352,3),np.uint8)
        for y in range(0,224,16):
            for x in range(0,352,16):actual[y:y+16,x:x+16]=banks[mapping[y//16,x//16]][data[y:y+16,x:x+16]]
        expected=colors[source].copy();expected[source==0]=0
        if not np.array_equal(actual,expected):raise ValueError(card['file']+': palette mapping changed source colors')
    GENERATED.mkdir(parents=True,exist_ok=True)
    for n,payload in enumerate(payloads):(GENERATED/f'presentation-bank-{n}.bin').write_bytes(payload)
    (GENERATED/'presentation-layout.json').write_text(json.dumps(descriptors,indent=2)+'\n')
    lines.append(f'#define SLF_PRESENTATION_CARDS {len(cards)}u')
    lines.append(f'#define SLF_STORY_CARDS {story}u')
    for n,card in enumerate(backdrops):
        lines.append(f"#define SLF_CARD_{card['file'][:-4].upper()} {story+n}u")
    lines.append('static const uint32_t slf_presentation_offsets[] = {'+','.join(str(v) for v in offsets)+'};')
    return lines,packed

def roundtrip(packed,decoded):
    for name,data,base in packed:
        actual=np.zeros_like(data);tile=base
        for y in range(0,data.shape[0],16):
            for x in range(0,data.shape[1],16):actual[y:y+16,x:x+16]=decoded[tile];tile+=1
        if not np.array_equal(actual,data):raise ValueError(name+': encoded C-ROM pixels differ from source')
