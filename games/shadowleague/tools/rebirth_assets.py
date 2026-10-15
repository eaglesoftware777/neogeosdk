"""Pack completed native exports, preserving every indexed source pixel."""
import json,struct
from pathlib import Path
import numpy as np
from native_art_io import indexed,palette_tiles
from palette_banks import palette_words
ROOT=Path(__file__).resolve().parents[1]/'assets/authored/rebirth'
GENERATED=Path(__file__).resolve().parents[1]/'artbox/generated'
CARD_BYTES=664

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
    payloads=[bytearray(),bytearray()];offsets=[];descriptors=[]
    for number,card in enumerate(cards):
        source,colors=indexed(ROOT/'cinema'/card['file'],(352,224))
        data,banks,mapping=palette_tiles(source,colors,limit=8)
        tile=place(data);packed.append((card['file'],data,tile))
        words=[word for bank in banks for word in palette_words(bank)];words += [0]*(128-len(words))
        caption=b''
        if len(card['caption'])!=2 or not 1<=card['duration']<=600:raise ValueError('Invalid story timing/caption')
        for text in card['caption']:
            encoded=text.encode('ascii')
            if len(encoded)>38:raise ValueError('Story caption exceeds FIX safe width')
            caption+=encoded.ljust(40,b'\0')
        descriptor=struct.pack('>IHBB',tile,card['duration'],len(banks),22)
        descriptor+=struct.pack('>128H',*words)+mapping.tobytes().ljust(320,b'\0')+caption
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
    lines.append('static const uint32_t slf_presentation_offsets[] = {'+','.join(str(v) for v in offsets)+'};')
    return lines,packed

def roundtrip(packed,decoded):
    for name,data,base in packed:
        actual=np.zeros_like(data);tile=base
        for y in range(0,data.shape[0],16):
            for x in range(0,data.shape[1],16):actual[y:y+16,x:x+16]=decoded[tile];tile+=1
        if not np.array_equal(actual,data):raise ValueError(name+': encoded C-ROM pixels differ from source')
