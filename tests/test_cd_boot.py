import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from cd_fixtures import program
from tools.neocd.formats import validate_prg
from tools.neocd.build import build_native,default_compiler
from tools.neocd.audit_disc import audit_cue
from tools.neocd.qa import firmware_bytes

ROOT=Path(__file__).resolve().parents[1]


class NativeBootTests(unittest.TestCase):
    def test_prg_is_native_not_cartridge_order(self):
        raw=program();self.assertEqual(validate_prg(raw),0x200)
        swapped=bytearray(raw);swapped[0::2],swapped[1::2]=raw[1::2],raw[0::2]
        with self.assertRaises(ValueError):validate_prg(swapped)
        for offset,value in ((0x107,0),(0x122,0),(0x126,0xFF),(0x13A,1)):
            bad=bytearray(raw);bad[offset]=value
            with self.subTest(offset=offset),self.assertRaises(ValueError):validate_prg(bad)

    def test_no_false_firmware_success(self):
        with tempfile.TemporaryDirectory() as d:
            f=Path(d)/'firmware.bin'
            with self.assertRaises(ValueError):firmware_bytes(f,'stock')
            f.write_bytes(bytes(0x20000))
            with self.assertRaises(ValueError):firmware_bytes(f,'stock')
            raw=bytearray(0x80000);raw[:8]=bytes.fromhex('0010f30000c01000')
            raw[0x700:0x708]=b'EAGLECD\0';f.write_bytes(raw)
            for mode in ('stock','eagle'):
                with self.assertRaises(ValueError):firmware_bytes(f,mode)

    def test_unported_game_is_rejected_before_output(self):
        with tempfile.TemporaryDirectory() as d:
            out=Path(d)/'unported'
            with self.assertRaises(ValueError):build_native('nosignal',out,default_compiler())
            self.assertFalse(out.exists())

    @unittest.skipUnless(shutil.which(default_compiler()) and shutil.which('wla-z80') and shutil.which('wlalink'),
                         'm68k GCC and WLA-DX required for native build test')
    def test_real_native_program_and_disc_build(self):
        with tempfile.TemporaryDirectory() as d:
            out=Path(d)/'native'
            report=build_native('helloworld',out,default_compiler(),cdda=False)
            self.assertTrue(audit_cue(out/'disc/helloworld.cue')['passed'])
            self.assertEqual(report['image_audit']['audio'],[])
            self.assertEqual((out/'payload/HELLO.PRG').read_bytes()[0x100:0x108],b'NEO-GEO\x02')
            raw=(out/'payload/HELLO.PRG').read_bytes()
            entry=int.from_bytes(raw[0x124:0x128],'big')
            self.assertEqual(raw[entry:entry+8],bytes.fromhex('0c3900020010fdae'))
            self.assertEqual(raw[entry+8],0x65) # carry: initial USER requests
            target=entry+10+int.from_bytes(raw[entry+9:entry+10],'big',signed=True)
            self.assertEqual(raw[target:target+6],bytes.fromhex('4ef900c00444'))
            # Drive IRQs can arrive during IPL, before USER has repaired RAM
            # vectors. Each must already chain to its matching firmware vector.
            exception=int.from_bytes(raw[8:12],'big')
            for vector in (0x54,0x58,0x5C):
                handler=int.from_bytes(raw[vector:vector+4],'big')
                self.assertNotEqual(handler,exception)
                self.assertEqual(raw[handler:handler+6],b'\x2f\x39'+(0xC00000+vector).to_bytes(4,'big'))
                self.assertEqual(raw[handler+6:handler+8],b'\x4e\x75')
            with self.assertRaises(ValueError):build_native('helloworld',out,default_compiler())

    def test_make_dispatch_preserves_cartridge_recipes(self):
        for name,newline in (('Makefile',b'\n'),('MakefileWin32.mak',b'\r\n')):
            raw=(ROOT/name).read_bytes()
            self.assertTrue(raw.startswith(b'ifeq ($(PLATFORM),cd)'+newline))
            self.assertIn(b'include sdk/platform/cd/cd.mk'+newline+b'else'+newline,raw)
            self.assertTrue(raw.endswith(b'include sdk/platform/cd/targets.mk'+newline))
            if newline==b'\r\n':self.assertNotIn(b'\n',raw.replace(b'\r\n',b''))


if __name__=='__main__':unittest.main()
