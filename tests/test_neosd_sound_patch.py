import importlib.util
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("sound_patch", ROOT / "tools/patch_neosd_sound.py")
patch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)


class SoundPatchTests(unittest.TestCase):
    def make_base(self):
        p = bytearray(0x80000)
        p[0x1B3A:0x1B3A + len(patch.INIT_SIGNATURE)] = patch.INIT_SIGNATURE
        regions = (patch.swap_words(p), b"S" * 0x20000, b"M" * 0x20000,
                   b"V" * 0x800000, b"", b"C" * 0x40000)
        header = bytearray(4096)
        header[:4] = b"NEO\x01"
        struct.pack_into("<6I", header, 4, *(len(r) for r in regions))
        return bytes(header) + b"".join(regions)

    def test_only_sound_regions_change(self):
        base = self.make_base()
        result = patch.patch_image(base, b"N" * 0x20000)
        self.assertEqual(result[:4096], base[:4096])
        old, new = patch.read_regions(base), patch.read_regions(result)
        for name in ("s1", "v1", "v2", "c"):
            self.assertEqual(old[name], new[name])
        self.assertEqual(sum(a != b for a, b in zip(old["p1"], new["p1"])), 1)
        self.assertEqual(patch.swap_words(new["p1"])[0x1B3D], 9)

    def test_wrong_signature_and_size_are_rejected(self):
        base = self.make_base()
        with self.assertRaises(ValueError):
            patch.patch_image(base, b"M")
        with self.assertRaises(ValueError):
            patch.patch_image(base[:4096] + bytes(len(base) - 4096), b"M" * 0x20000)
        with self.assertRaises(ValueError):
            patch.read_regions(base[:-1])


if __name__ == "__main__":
    unittest.main()
