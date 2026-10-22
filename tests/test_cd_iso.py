import json
from pathlib import Path
import tempfile
import unittest
import wave
from cd_fixtures import package
from tools.neocd.iso9660 import build_iso
from tools.neocd.audit_disc import read_iso,audit_image,audit_cue
from tools.neocd.build_disc import build


class ISOTests(unittest.TestCase):
    def test_multisector_directory_and_reproducibility(self):
        files={'F%07d.SPR'%i:bytes([i%256])*128 for i in range(512)}
        image=build_iso(files)
        self.assertEqual(read_iso(image),files)
        self.assertEqual(build_iso(dict(reversed(list(files.items())))),image)
        with self.assertRaises(ValueError):build_iso(dict(files,**{'EXTRA.SPR':b'x'}))

    def test_bad_iso_metadata(self):
        good=build_iso({'A.SPR':b'A'*128,'B.SPR':b'B'*128})
        # Independent dual-endian, signature, root extent, record and padding checks.
        for offset in (16*2048,16*2048+84,16*2048+158,20*2048,20*2048+1,20*2048+2047):
            bad=bytearray(good);bad[offset]^=0x40
            with self.subTest(offset=offset),self.assertRaises(ValueError):read_iso(bad)
        for trim in (1,2048):
            with self.assertRaises(ValueError):read_iso(good[:-trim])

    def test_overlapping_extents_rejected(self):
        image=bytearray(build_iso({'A.SPR':b'A'*128,'B.SPR':b'B'*128}))
        cursor=20*2048+68
        first=cursor;second=cursor+image[cursor]
        image[second+2:second+10]=image[first+2:first+10]
        with self.assertRaises(ValueError):read_iso(image)

    def test_package_and_payload_roundtrip(self):
        with tempfile.TemporaryDirectory() as d:
            folder=Path(d);manifest,payload=package(folder)
            source=folder/'manifest.json';source.write_text(json.dumps(manifest))
            result=build(source,folder/'out')
            extracted=read_iso((folder/'out/hello.iso').read_bytes())
            self.assertEqual({n:extracted[n] for n in payload},payload)
            self.assertTrue(result['image_audit']['passed'])
            self.assertFalse(result['boot_verified'])
            self.assertEqual(audit_cue(folder/'out/hello.cue')['files'],result['image_audit']['files'])
            with self.assertRaises(ValueError):build(source,folder/'out')

    def test_missing_boot_asset_fails(self):
        with tempfile.TemporaryDirectory() as d:
            folder=Path(d);_,files=package(folder)
            files['IPL.TXT']=b'MISSING.PRG,0,0\r\n\x1a'
            iso=folder/'bad.iso';iso.write_bytes(build_iso(files))
            with self.assertRaises(ValueError):audit_image(iso)

    def test_cue_track_association_and_paths(self):
        with tempfile.TemporaryDirectory() as d:
            f=Path(d);manifest,_=package(f)
            with wave.open(str(f/'music.wav'),'wb') as w:
                w.setparams((2,2,44100,0,'NONE',''));w.writeframes(bytes(588*4))
            manifest['audio']=['music.wav']
            src=f/'m.json';src.write_text(json.dumps(manifest));build(src,f/'disc')
            cue=f/'disc/hello.cue';good=cue.read_text()
            self.assertEqual(len(audit_cue(cue)['audio']),1)
            for bad in (good.replace('TRACK 02','TRACK 03'),good.replace('MODE1/2048','AUDIO'),
                        good.replace('PREGAP 00:02:00','PREGAP 00:02:00\n    PREGAP 00:02:00'),
                        good+'FILE "hello.iso" BINARY\n',good.replace('audio/track02.wav','../music.wav'),
                        good.replace('audio/track02.wav','C:/music.wav'),
                        good.replace('FILE "audio/track02.wav" WAVE\n','')):
                cue.write_text(bad)
                with self.subTest(cue=bad),self.assertRaises(ValueError):audit_cue(cue)


if __name__=='__main__':unittest.main()
