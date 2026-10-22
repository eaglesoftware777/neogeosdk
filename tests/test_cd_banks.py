from pathlib import Path
import random
import shutil
import subprocess
import tempfile
import unittest
from tools.neocd.formats import Entry,ZONES,encode_spr,decode_spr,cartridge_sprites_to_spr,decode_fix

ROOT=Path(__file__).resolve().parents[1]


class BankTests(unittest.TestCase):
    def test_exact_boundaries_all_banks(self):
        for zone,(banks,capacity,alignment) in ZONES.items():
            for bank in range(banks):
                e=Entry('TEST.'+zone,bank,capacity-alignment)
                self.assertEqual(e.validate(alignment),(bank*capacity+capacity-alignment,(bank+1)*capacity))
                for bad in (Entry(e.name,banks),Entry(e.name,bank,capacity),Entry(e.name,bank,1)):
                    with self.subTest(entry=bad),self.assertRaises(ValueError):bad.validate(alignment)
                for size in (0,alignment-1,alignment*2,-1,True):
                    with self.subTest(zone=zone,bank=bank,size=size),self.assertRaises(ValueError):e.validate(size)

    def test_sprite_plane_and_half_fixtures(self):
        # Independent one-pixel fixtures pin plane order, bit order, and right/left halves.
        for x in range(16):
            for y in (0,15):
                for plane,wire in ((0,1),(1,0),(2,3),(3,2)):
                    image=[[0]*16 for _ in range(16)];image[y][x]=1<<plane
                    expected=bytearray(128);expected[(0 if x>=8 else 64)+y*4+wire]=1<<(x%8)
                    self.assertEqual(encode_spr(image),expected)
                    self.assertEqual(decode_spr(expected),image)

    def test_cart_final_lane_conversion(self):
        c1,c2=bytearray(64),bytearray(64)
        c1[0]=1;c1[1]=2;c2[0]=4;c2[1]=8
        native=cartridge_sprites_to_spr(c1,c2)
        self.assertEqual(native[:4],bytes((2,1,8,4)))
        pixels=decode_spr(native)
        self.assertEqual(pixels[0][8:12],[1,2,4,8])
        for a,b in ((b'',b''),(bytes(63),bytes(63)),(bytes(64),bytes(128))):
            with self.assertRaises(ValueError):cartridge_sprites_to_spr(a,b)

    def test_sprite_random_roundtrip_and_fix_columns(self):
        rng=random.Random(177)
        for _ in range(20):
            pixels=[[rng.randrange(16) for x in range(16)] for y in range(16)]
            self.assertEqual(decode_spr(encode_spr(pixels)),pixels)
        for column,offset in enumerate((16,24,0,8)):
            tile=bytearray(32);tile[offset+7]=0xA5
            pixels=decode_fix(tile)
            self.assertEqual(pixels[7][column*2:column*2+2],[5,10])
            self.assertEqual(sum(p!=0 for row in pixels for p in row),2)

    @unittest.skipUnless(shutil.which('cc') and shutil.which('c++'),'Host C and C++ compilers required')
    def test_c_and_cpp14_abi_and_runtime_rejections(self):
        with tempfile.TemporaryDirectory() as d:
            f=Path(d);source=f/'probe.c'
            source.write_text(r'''
#include "ng_cd.h"
#include <assert.h>
int main(void) {
 const unsigned capacity[5]={0x100000,0x20000,0x100000,0x10000,0x80000};
 const unsigned alignment[5]={2,32,128,2,256};
 const unsigned banks[5]={1,1,4,1,2};
 unsigned z,b;
 for(z=0;z<5;z++) for(b=0;b<banks[z];b++) {
  NGCDRange r={capacity[z]-alignment[z],alignment[z],(uint16_t)b,(uint8_t)z};
  assert(ng_cd_range_validate(&r)==NG_CD_OK);
  r.bytes+=alignment[z];assert(ng_cd_range_validate(&r)==NG_CD_BOUNDS);
  r.offset=0;r.bytes=alignment[z];r.bank=(uint16_t)banks[z];
  assert(ng_cd_range_validate(&r)==NG_CD_BOUNDS);
  r.bank=0;r.offset=0xFFFFFFFFu;assert(ng_cd_range_validate(&r)==NG_CD_BOUNDS);
 }
 {
  NGCDRange r={0,128,3,NG_CD_SPR};
  assert(ng_cd_load_file("ASSET.SPR",&r)==NG_CD_UNSUPPORTED);
  assert(ng_cd_load_file("ASSET.PCM",&r)==NG_CD_INVALID);
  assert(ng_cd_load_file("../A.SPR",&r)==NG_CD_INVALID);
  assert(ng_cd_load_file("TOOLONGXX.SPR",&r)==NG_CD_INVALID);
  r.zone=NG_CD_PRG;r.bank=0;
  assert(ng_cd_load_file("CODE.PRG",&r)==NG_CD_UNSUPPORTED);
 }
 assert(ng_cd_range_validate(0)==NG_CD_INVALID);
 assert(ng_platform_capabilities()->platform==NG_PLATFORM_CD_ID);
 assert(!ng_platform_capabilities()->has_adpcm_b);
 return 0;
}
''')
            objects=[]
            for name in ('ng_cd_memory','ng_cd_io','ng_cd_system'):
                obj=f/(name+'.o');objects.append(str(obj))
                subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-I'+str(ROOT/'sdk/platform/cd'),
                                '-c',str(ROOT/'sdk/platform/cd'/(name+'.c')),'-o',str(obj)],check=True)
            for compiler,language in (('cc',['-std=c99']),('c++',['-x','c++','-std=c++14'])):
                executable=f/('probe-'+compiler.replace('+','p'))
                main=f/('main-'+compiler.replace('+','p')+'.o')
                subprocess.run([compiler,*language,'-Wall','-Wextra','-Werror',
                                '-I'+str(ROOT/'sdk/platform/cd'),'-c',str(source),'-o',str(main)],check=True)
                subprocess.run([compiler,str(main),*objects,'-o',str(executable)],check=True)
                subprocess.run([str(executable)],check=True)


if __name__=='__main__':unittest.main()
