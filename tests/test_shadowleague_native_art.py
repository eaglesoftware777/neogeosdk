"""Exercise lossless artist exports and strict hardware palette limits."""
import sys,tempfile,unittest,json
from pathlib import Path
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'games/shadowleague/tools'))
from native_art_io import indexed,palette_tiles,fighter_source
from strip_pack import StripPack

class NativeArtTests(unittest.TestCase):
    def test_native_title_repacking_keeps_card_bank_limit_and_exact_colors(self):
        sys.path.insert(0,str(ROOT/'artbox'))
        from rebirth_assets import CARD_BANKS
        folder=ROOT/'games/shadowleague/assets/authored/rebirth/cinema'
        project=json.loads((folder/'project.json').read_text())
        source,colors=indexed(folder/project['title'],(352,224))
        local,banks,mapping=palette_tiles(source,colors,limit=CARD_BANKS)
        self.assertLessEqual(len(banks),CARD_BANKS)
        decoded=np.zeros((*source.shape,3),np.uint8)
        for y in range(0,224,16):
            for x in range(0,352,16):decoded[y:y+16,x:x+16]=banks[mapping[y//16,x//16]][local[y:y+16,x:x+16]]
        expected=colors[source].copy();expected[source==0]=0
        np.testing.assert_array_equal(decoded,expected)

    def test_playable_showcase_has_all_referenced_pose_exports(self):
        game=ROOT/'games/shadowleague'
        for who in ('kenji','jack','amara'):
            with self.subTest(fighter=who):
                data=json.loads((game/'fighters'/(who+'.json')).read_text())
                source=game/'assets/authored/fighters'/who
                manifest=json.loads((source/'manifest.json').read_text())
                names={fr[0] for seq in data['anims'].values() for fr in seq}
                self.assertTrue(names <= set(manifest['frames']))
                source_data=fighter_source(source,names)
                self.assertEqual(len(source_data['images']),len(names))
                for name in names:
                    pose=manifest['frames'][name]
                    self.assertTrue((source/pose['file']).is_file())
                    if 'layers' in pose:self.assertEqual(len(pose['layers']),6)
                    pixels,_=indexed(source/pose['file'],tuple(manifest['canvas']))
                    self.assertFalse(pixels[0].any() or pixels[-1].any() or pixels[:,0].any() or pixels[:,-1].any())
                self.assertEqual(len(data['anims']['walk_f']),6)

    def test_large_canvas_keeps_last_tile_and_palette_bits(self):
        packed=[]
        def place(image):
            packed.append(image.copy());return 0x20000
        pack=StripPack(place)
        image=np.zeros((192,192),np.uint8);image[160:191,176:191]=7
        banks=np.zeros((12,12),np.uint8);banks[10,11]=3;banks[11,11]=2
        self.assertEqual(pack.frame(image,banks),[(71,0,0x2b)])
        np.testing.assert_array_equal(packed[0],image[160:192,176:192])
        with self.assertRaisesRegex(ValueError,'160x160 or 192x192'):pack.frame(np.zeros((176,176),np.uint8))

    def test_large_flat_frame_has_explicit_stable_root(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder=Path(temporary);data=np.zeros((192,192),np.uint8)
            data[40:174,20:172]=1
            im=Image.fromarray(data,'P');im.putpalette([0,0,0,239,156,90]+[0]*762);im.save(folder/'guard.png',transparency=0)
            d={'canvas':[192,192],'anchor':[96,173],'frames':{'guard':{'file':'guard.png','root':[101,173],'hurt':[[-20,0,40,100]],'hit':{},'contacts':[]}}}
            (folder/'manifest.json').write_text(json.dumps(d))
            source=fighter_source(folder,['guard'])
            self.assertEqual(source['canvas'],[192,192]);self.assertEqual(source['anchor'],[96,173])
            self.assertEqual(source['metadata'][0]['root'],[101,173])

    def test_shared_strip_retains_independent_palette_and_address_bits(self):
        pack=StripPack(lambda image:0x10002)
        image=np.zeros((160,160),np.uint8);image[32:64,48:64]=1
        banks=np.zeros((10,10),np.uint8);banks[2,3]=2;banks[3,3]=3
        self.assertEqual(pack.frame(image,banks),[(13,2,0x1e)])
        banks[2,3]=1;banks[3,3]=0
        self.assertEqual(pack.frame(image,banks),[(13,2,0x11)])
        self.assertEqual(len(pack.lookup),1)
        high=StripPack(lambda image:0xf0002)
        self.assertEqual(high.frame(image,banks),[(13,2,0xf1)])
    def test_multi_bank_pixel_roundtrip(self):
        data=np.zeros((32,32),np.uint8)
        for y in range(32):
            for x in range(32):data[y,x]=1+(x%14)+(14 if y>=16 else 0)
        colors=np.asarray([[i*8%256,i*16%256,i*24%256] for i in range(256)],np.uint8)
        local,banks,mapping=palette_tiles(data,colors)
        self.assertEqual(len(banks),2)
        decoded=np.zeros((32,32,3),np.uint8)
        for y in range(0,32,16):
            for x in range(0,32,16):decoded[y:y+16,x:x+16]=banks[mapping[y//16,x//16]][local[y:y+16,x:x+16]]
        np.testing.assert_array_equal(decoded,colors[data])
    def test_overfull_tile_fails(self):
        data=np.tile(np.arange(16,dtype=np.uint8)+1,(16,1))
        with self.assertRaisesRegex(ValueError,'16 opaque'):palette_tiles(data,np.zeros((256,3),np.uint8))
    def test_unused_index_zero_must_still_be_transparent(self):
        with tempfile.TemporaryDirectory() as folder:
            p=Path(folder)/'opaque.png';im=Image.new('P',(16,16),1);im.putpalette([0]*768);im.save(p)
            with self.assertRaisesRegex(ValueError,'index zero must be transparent'):indexed(p)
    def test_partial_alpha_fails(self):
        with tempfile.TemporaryDirectory() as folder:
            p=Path(folder)/'bad.png';im=Image.new('P',(16,16),1);im.putpalette([0]*768);im.save(p,transparency=bytes([0,128]+[255]*254))
            with self.assertRaisesRegex(ValueError,'partial alpha'):indexed(p)
    def test_missing_authored_frame_never_falls_back(self):
        with tempfile.TemporaryDirectory() as folder:
            (Path(folder)/'manifest.json').write_text(json.dumps({'canvas':[160,160],'anchor':[80,141],'frames':{}}))
            with self.assertRaisesRegex(ValueError,'fallback forbidden'):fighter_source(folder,['heavy_impact'])

    def test_authored_anatomy_layers_keep_global_colors_across_banks(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder=Path(temporary)
            colors=np.asarray([[i*8%256,i*16%256,i*24%256] for i in range(256)],np.uint8)
            def save(data,name):
                im=Image.fromarray(data,'P');im.putpalette(colors.ravel().tolist());im.save(folder/name,transparency=0)
            layers={};flat=np.zeros((160,160),np.uint8)
            for n,part in enumerate(['far_arm','far_leg','torso','head_hair','near_leg','near_arm']):
                data=np.zeros_like(flat);x=16+n*16
                # Two disjoint color ramps on connected native limb fixtures.
                data[32:142,x:x+14]=np.arange(1,15,dtype=np.uint8)+(14 if n&1 else 0)
                name=part+'.png';save(data,name);layers[part]=name;flat[data>0]=data[data>0]
            save(flat,'guard.png')
            manifest={'canvas':[160,160],'anchor':[80,141], 'frames':{'guard':{
                'file':'guard.png','layers':layers,'hurt':[[-12,0,24,80]],'hit':{},'contacts':['far_leg','near_leg']}}}
            (folder/'manifest.json').write_text(json.dumps(manifest))
            source=fighter_source(folder,['guard'])
            self.assertEqual(len(source['banks']),2)
            actual=np.zeros((160,160,3),np.uint8)
            for y in range(0,160,16):
                for x in range(0,160,16):
                    bank=source['tile_maps'][0][y//16,x//16]
                    actual[y:y+16,x:x+16]=source['banks'][bank][source['images'][0][y:y+16,x:x+16]]
            np.testing.assert_array_equal(actual,colors[flat])
if __name__=='__main__':unittest.main()
