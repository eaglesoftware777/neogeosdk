"""Reconstruct the actual cartridge FIX words and compare native source pixels."""
from pathlib import Path
import json
import re
import unittest

import numpy as np
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
GAME=ROOT/"games/shadowleague"

def decode_fix_tile(raw):
    # Hardware column pairs are 4/5, 6/7, 0/1, 2/3, low nibble first.
    # Keep this independent of the encoder and the DB-backed asset script.
    columns=np.frombuffer(raw,dtype=np.uint8).reshape(4,8).T
    result=np.zeros((8,8),np.uint8)
    result[:,[4,6,0,2]]=columns & 15
    result[:,[5,7,1,3]]=columns >> 4
    return result


class AnnouncementArtTests(unittest.TestCase):
    def test_native_words_survive_actual_s_rom_encoding(self):
        folder=GAME/"assets/authored/rebirth/announcements"
        project=json.loads((folder/"project.json").read_text())
        header=(GAME/"artbox/generated/slf_assets.h").read_text()
        rom=(ROOT/"roms/shadowleague/781-s1.s1").read_bytes()
        entries=project["images"]+[{'name':'DIGITS','file':'../hud/digits.png','canvas':[160,16]}]
        for entry in entries:
            with self.subTest(word=entry["name"]):
                source=Image.open(folder/entry["file"])
                self.assertEqual(source.mode,"P")
                self.assertEqual(source.info.get("transparency"),0)
                self.assertEqual(source.size,tuple(entry["canvas"]))
                expected=np.asarray(source)
                self.assertLessEqual(int(expected.max()),7)
                tile=int(re.search(r"#define SLF_BIG_"+entry["name"]+r" (\d+)u",header)[1])
                width=int(re.search(r"#define SLF_BIG_"+entry["name"]+r"_W (\d+)u",header)[1])
                height=int(re.search(r"#define SLF_BIG_"+entry["name"]+r"_H (\d+)u",header)[1])
                self.assertEqual((width*8,height*8),source.size)
                decoded=np.zeros_like(expected)
                for y in range(height):
                    for x in range(width):
                        index=tile+y*width+x
                        decoded[y*8:y*8+8,x*8:x*8+8]=decode_fix_tile(rom[index*32:(index+1)*32])
                np.testing.assert_array_equal(decoded,expected)


if __name__=="__main__":
    unittest.main()
