"""No Signal CD conversion preserves original pixels within real RAM limits."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from tools.neocd import nosignal
from tools.neocd.formats import Entry


class NoSignalCDTests(unittest.TestCase):
    def test_source_and_media_reference(self):
        self.assertEqual(nosignal.verify_v2()['version'], 2)

    def test_every_sprite_byte_and_palette_survives_relocation(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            payload, include = root/'payload', root/'include'
            payload.mkdir(); include.mkdir()
            result = nosignal.convert_graphics(payload, include)
            self.assertLessEqual(result['resident_tiles'], nosignal.BG_BASE)
            self.assertEqual(len(result['backgrounds']), 13)
            resident = b''.join((payload/f'SPR{i}.SPR').read_bytes() for i in range(4))
            self.assertEqual(len(resident), nosignal.BG_BASE*128)
            art = nosignal.GAME/'artbox'
            source = [(art/f'784-{lane}.{lane}').read_bytes() for lane in ('c1','c2')]
            assets = json.loads((art/'generated/assets.json').read_text())['assets']
            for a in assets:
                start = a['tile_base']*64; end = start+a['tiles']*64
                dest = result['tile_mapping'][a['id']]*128
                if a['kind']=='background':
                    native = (payload/f"BG{a['id']:02}.SPR").read_bytes()
                    Entry(f"BG{a['id']:02}.SPR", 3, 0x80000).validate(len(native))
                    self.assertLessEqual(dest+len(native), 0x7FFD*128)
                else:
                    native = resident[dest:dest+a['tiles']*128]
                for lane, offsets in enumerate(((1,0),(3,2))):
                    restored = bytearray(a['tiles']*64)
                    restored[0::2], restored[1::2] = native[offsets[0]::4], native[offsets[1]::4]
                    self.assertEqual(restored, source[lane][start:end], a['name'])
            utility = (payload/'UTIL.SPR').read_bytes()
            for lane, offsets in enumerate(((1,0),(3,2))):
                restored = bytearray(192)
                restored[0::2], restored[1::2] = utility[offsets[0]::4], utility[offsets[1]::4]
                self.assertEqual(restored, source[lane][0xFFFD*64:])
            original = (art/'generated/ns_assets.h').read_text()
            converted = (include/'games/nosignal/artbox/generated/ns_assets.h').read_text()
            self.assertEqual(original.split('static const NGArtAsset')[0], converted.split('static const NGArtAsset')[0])
            self.assertEqual((payload/'NS.FIX').read_bytes(), (art/'784-s1.s1').read_bytes())

    def test_descriptor_address_units_and_terminator(self):
        data = nosignal.descriptor('BG01.SPR',3,0x80000)
        self.assertEqual(data[:10], b'BG01.SPR\0\x03')
        self.assertEqual(int.from_bytes(data[10:14], 'big'), 0x80000)
        self.assertEqual(data[14:], b'\0')

    def test_source_changes_are_rejected(self):
        with patch.object(nosignal.hashlib, 'sha256') as sha:
            sha.return_value.hexdigest.return_value = 'changed'
            with self.assertRaisesRegex(ValueError, 'V2 source or artwork differs'):
                nosignal.verify_v2()


if __name__=='__main__':
    unittest.main()
