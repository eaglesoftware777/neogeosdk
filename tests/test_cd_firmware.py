import importlib.util
from pathlib import Path
import shutil
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
SPEC=importlib.util.spec_from_file_location('original_cd_firmware_builder',ROOT/'bios/cd/tools/build.py')
BUILDER=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(BUILDER)


class FirmwareTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which(BUILDER.compiler_default()),'m68k GCC required')
    def test_exact_rom_vectors_identity_and_service_slots(self):
        with tempfile.TemporaryDirectory() as d:
            folder=Path(d)/'firmware'
            report=BUILDER.build(folder,BUILDER.compiler_default())
            for ident,model in enumerate(('front','top','cdz')):
                data=(folder/('eagle_cd_'+model+'.rom')).read_bytes()
                self.assertEqual(len(data),0x80000)
                self.assertEqual(data[:8],bytes.fromhex('0010f30000c01000'))
                self.assertEqual(data[0x700:0x710],b'EAGLECD\0'+bytes.fromhex('000100070007')+ident.to_bytes(2,'big'))
                self.assertEqual(int.from_bytes(data[0x710:0x714],'big'),1) # diagnostic only
                for slot in (0x402,0x438,0x444,0x44A,0x546,0x552,0x564,0x56A,0x570):
                    self.assertEqual(data[slot:slot+2],b'\x4e\xf9')
                    target=int.from_bytes(data[slot+2:slot+6],'big')
                    self.assertTrue(0xC01000<=target<0xC80000 and not target&1)
                self.assertEqual(data[-128:],b'\xff'*128)
                self.assertEqual(report['profiles'][model]['size'],len(data))
            with self.assertRaises(ValueError):BUILDER.build(folder,BUILDER.compiler_default())


if __name__=='__main__':unittest.main()
