"""Verify physical ROM alignment and the runtime bank window together."""
import subprocess,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'games/shadowleague/tools'))
from banked_rom import assemble,MIB
sys.path.insert(0,str(ROOT/'tools'))
from pack_neosd import build_image

class BankedRomTests(unittest.TestCase):
    def test_runtime_copy_crosses_fixed_and_switchable_boundaries(self):
        with tempfile.TemporaryDirectory() as folder:
            binary=Path(folder)/'romtest'
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-DSLF_ROM_HOST=1','-I'+str(ROOT),
                str(ROOT/'tests/shadowleague_rom_test.c'),str(ROOT/'games/shadowleague/scenes/slf_rom.c'),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)

    def test_fixed_and_bank_payloads_keep_exact_mib_offsets(self):
        image=assemble(b'FIXED',[b'BANK-ZERO',b'BANK-ONE'])
        raw=bytearray(image);raw[0::2],raw[1::2]=image[1::2],image[0::2]
        self.assertEqual(len(raw),3*MIB)
        for offset,data in [(0,b'FIXED'),(MIB,b'BANK-ZERO'),(2*MIB,b'BANK-ONE')]:
            self.assertEqual(raw[offset:offset+len(data)],data)
            self.assertEqual(raw[offset+len(data):offset+MIB],b'\xff'*(MIB-len(data)))

    def test_overflow_and_wrong_bank_count_are_rejected(self):
        for fixed,banks in [(b'A'*(MIB+1),[b'B',b'C']),(b'A',[b'B']),(b'A',[b'B'*(MIB+1),b'C'])]:
            with self.assertRaises(ValueError):assemble(fixed,banks)

    def test_neosd_preserves_banked_program_bytes_and_region_offsets(self):
        import struct
        p=assemble(b'FIXED',[b'BANK-ZERO',b'BANK-ONE'])
        parts=dict(p1=p,s1=b'S'*65536,m1=b'M'*65536,v1=b'V'*65536,c1=b'C'*65536,c2=b'D'*65536)
        neo=build_image(parts,'Test','Test',2026,5,0x781)
        sizes=struct.unpack_from('<6I',neo,4)
        self.assertEqual(sizes[0],3*MIB);self.assertEqual(neo[4096:4096+3*MIB],p)
        self.assertEqual(neo[4096+3*MIB],ord('S'));self.assertEqual(len(neo),4096+sum(sizes))
        parts['p1']=b'X'*(MIB+2)
        with self.assertRaisesRegex(ValueError,'complete one-MiB'):build_image(parts,'Test','Test',2026,5,0x781)
        parts['p1']=b'X'*(8*MIB+2)
        with self.assertRaisesRegex(ValueError,'capacity'):build_image(parts,'Test','Test',2026,5,0x781)
if __name__=='__main__':unittest.main()
