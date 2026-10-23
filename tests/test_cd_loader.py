from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from cd_fixtures import package
from tools.neocd.build_disc import build
from tools.neocd.audit_disc import read_iso
from tools.neocd.iso9660 import build_iso

ROOT=Path(__file__).resolve().parents[1]


@unittest.skipUnless(shutil.which('cc'),'host C compiler required')
class LoaderTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory()
        cls.folder=Path(cls.temp.name)
        cls.exe=cls.folder/'loader'
        subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-DECD_HOST_TEST',
                        '-I'+str(ROOT/'bios/cd/include'),str(ROOT/'tests/cd_loader_host.c'),
                        str(ROOT/'bios/cd/src/cd_files.c'),'-o',str(cls.exe)],check=True)
        manifest,_=package(cls.folder)
        path=cls.folder/'source.json'
        import json
        path.write_text(json.dumps(manifest))
        build(path,cls.folder/'build')
        cls.good=(cls.folder/'build/hello.iso').read_bytes()
        cls.files=read_iso(cls.good)

    @classmethod
    def tearDownClass(cls):cls.temp.cleanup()

    def run_image(self,data):
        image=self.folder/'case.iso';image.write_bytes(data)
        return tuple(map(int,subprocess.check_output([str(self.exe),str(image)],text=True).strip().split(',')))

    def test_native_payloads_load_through_production_parser(self):
        self.assertEqual(self.run_image(self.good),(0,0,5,418))

    def test_directory_corruption_rejected_before_payload_writes(self):
        for at in (16*2048+84,16*2048+162,16*2048+170,16*2048+128,20*2048+68):
            bad=bytearray(self.good);bad[at]^=0x40
            with self.subTest(at=at):
                mount,_,loaded,writes=self.run_image(bad)
                self.assertNotEqual(mount,0);self.assertEqual((loaded,writes),(0,0))

    def test_ipl_rejected_before_partial_load(self):
        original=self.files['IPL.TXT']
        cases=[original[:-1],original.replace(b'HELLO.PRG',b'OTHER.PRG'),
               original.replace(b'BOOT.SPR,0,0',b'BOOT.SPR,4,0'),
               original.replace(b'FONT.FIX,0,0',b'FONT.FIX,0,1'),
               original.replace(b'HELLO.PRG,0,0',b'HELLO.PRG,0,100000'),
               b'HELLO.PRG,0,0\r\n\x1a',
               original[:-1]+b'BOOT.SPR,0,0\r\n\x1a',
               original.replace(b'\r\n',b'\n'),
               original+b'trailing']
        for ipl in cases:
            files=dict(self.files);files['IPL.TXT']=ipl
            with self.subTest(ipl=ipl):
                mount,result,loaded,writes=self.run_image(build_iso(files))
                self.assertEqual(mount,0);self.assertNotEqual(result,0)
                self.assertEqual((loaded,writes),(0,0))

    def test_invalid_entry_point_is_never_launched(self):
        for target in (0x100,0x300,0xFFFFFFFE,0x201):
            files=dict(self.files);prg=bytearray(files['HELLO.PRG'])
            prg[0x124:0x128]=target.to_bytes(4,'big');files['HELLO.PRG']=prg
            with self.subTest(target=target):
                mount,result,_,_=self.run_image(build_iso(files))
                self.assertEqual(mount,0);self.assertNotEqual(result,0)


if __name__=='__main__':unittest.main()
