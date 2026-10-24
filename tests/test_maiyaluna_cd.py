"""V2 artwork and gameplay preservation across native CD conversion."""
import json,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
from tools.neocd import maiyaluna


class MaiyaLunaCDTests(unittest.TestCase):
    def test_original_v2_source_and_asset_lock(self):
        reference=maiyaluna.verify_v2()
        self.assertEqual(reference['version'],2)
        self.assertEqual(len(reference['files']),16)

    def test_sprite_conversion_preserves_every_used_byte_and_utility_tile(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder=Path(tmp);report=maiyaluna.convert_graphics(folder)
            self.assertEqual(report['last_asset_tile'],21340)
            native=b''.join((folder/f'SPR{i}.SPR').read_bytes() for i in range(4))
            for start,end,dest in ((0,21341*64,0),(0xFFFD*64,0x10000*64,0x7FFD*128)):
                spr=native[dest:dest+(end-start)*2]
                for lane,offsets in (('c1',(1,0)),('c2',(3,2))):
                    restored=bytearray(end-start)
                    restored[0::2]=spr[offsets[0]::4]
                    restored[1::2]=spr[offsets[1]::4]
                    source=(maiyaluna.GAME/'artbox'/f'783-{lane}.{lane}').read_bytes()
                    self.assertEqual(restored,source[start:end])
            self.assertEqual((folder/'ML.FIX').read_bytes(),(maiyaluna.GAME/'artbox/783-s1.s1').read_bytes())
            self.assertEqual(native[21341*128:0x7FFD*128],bytes((0x7FFD-21341)*128))

    def test_changed_source_is_rejected(self):
        with patch.object(maiyaluna.hashlib,'sha256') as sha:
            sha.return_value.hexdigest.return_value='bad'
            with self.assertRaisesRegex(ValueError,'V2 source or artwork differs'):
                maiyaluna.verify_v2()

    def test_changed_reference_must_not_weaken_sprite_memory_bounds(self):
        data=json.loads((maiyaluna.GAME/'artbox/generated/assets.json').read_text())
        data['tiles_used']=0x7FFE
        with patch.object(maiyaluna,'verify_v2'),patch.object(maiyaluna.json,'loads',return_value=data):
            with self.assertRaisesRegex(ValueError,'exceeds CD sprite RAM'):
                maiyaluna.convert_graphics(Path('/unused'))
