import importlib.util
from pathlib import Path
import struct
import unittest

spec = importlib.util.spec_from_file_location(
    "pack_neosd", Path(__file__).resolve().parents[1] / "tools/pack_neosd.py")
packer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(packer)


class NeoSDPackingTests(unittest.TestCase):
    def test_unaligned_audio_does_not_shift_sprite_region(self):
        parts = dict(p1=b"P" * 65536, s1=b"S" * 65536,
                     m1=b"M" * 65536, v1=b"V" * 0x79AB00,
                     c1=bytes(range(256)) * 512,
                     c2=bytes(reversed(range(256))) * 512)
        data = packer.build_image(parts, "Test", "Test", 2026, 5, 0x780)
        sizes = struct.unpack_from("<6I", data, 4)
        self.assertEqual(sizes, (65536, 65536, 65536, 0x7A0000, 0, 0x40000))
        start = 4096 + sum(sizes[:3])
        self.assertEqual(data[start:start + len(parts["v1"])], parts["v1"])
        self.assertEqual(data[start + len(parts["v1"]):start + sizes[3]], b"\xff" * 0x5500)
        sprites = data[4096 + sum(sizes[:5]):]
        self.assertEqual(sprites[::2], parts["c1"])
        self.assertEqual(sprites[1::2], parts["c2"])
        self.assertEqual(len(data), 4096 + sum(sizes))

    def test_aligned_region_is_not_extended(self):
        self.assertEqual(packer.pad(b"a" * 65536, 65536), b"a" * 65536)


if __name__ == "__main__":
    unittest.main()
