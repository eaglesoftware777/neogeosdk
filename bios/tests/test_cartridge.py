"""Retail compatibility tests must preserve identity and cartridge wiring."""

import argparse
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zipfile
import zlib

spec = importlib.util.spec_from_file_location("bios_test", Path(__file__).parents[1] / "test_bios.py")
bios_test = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bios_test)


class CartridgeTest(unittest.TestCase):
    def test_verified_layout_and_rejection(self):
        data = b"test cartridge"
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory)
            cart = out / "cart"
            cart.mkdir()
            listing = out / "reference.xml"
            listing.write_text(
                '<softwarelist><software name="ssideki"><part name="cart" interface="neo_cart">'
                '<feature name="slot" value="rom_fatfur2"/>'
                '<dataarea name="sprites" size="0x600000">'
                f'<rom name="test.bin" offset="0" size="{len(data)}" '
                f'crc="{zlib.crc32(data):08x}" sha1="{hashlib.sha1(data).hexdigest()}"/>'
                '<rom offset="0x400000" size="0x100000" loadflag="continue"/>'
                '</dataarea></part></software></softwarelist>'
            )
            archive = out / "test.zip"
            args = argparse.Namespace(cartridge=archive, software_list=listing)
            with zipfile.ZipFile(archive, "w") as zipped:
                zipped.writestr("renamed.bin", data)
            bios_test.prepare_retail(args, out, cart)
            result = ET.parse(out / "hash/neogeo.xml")
            self.assertEqual(result.find(".//feature").get("value"), "rom_fatfur2")
            self.assertEqual(result.find(".//rom[@loadflag='continue']").get("offset"), "0x400000")
            self.assertEqual((cart / "test.bin").read_bytes(), data)
            with zipfile.ZipFile(archive, "w") as zipped:
                zipped.writestr("test.bin", b"replacement program")
            with self.assertRaisesRegex(SystemExit, "CRC mismatch"):
                bios_test.prepare_retail(args, out, cart)


if __name__ == "__main__":
    unittest.main()
