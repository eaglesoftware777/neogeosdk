import importlib.util
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("sound_patch", ROOT / "tools/patch_neosd_sound.py")
patch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)


class SoundPatchTests(unittest.TestCase):
    def make_driver(self):
        m1 = bytearray(b"N" * 0x20000)
        m1[0x90:0x94] = patch.M1_READY_SIGNATURE
        m1[0x200:0x208] = patch.M1_SLOT_SIGNATURE
        m1[0xC0:0xC4] = b"NGP2"
        return bytes(m1)

    def make_base(self):
        p = bytearray(0x80000)
        p[0x1B3A:0x1B3A + len(patch.INIT_SIGNATURE)] = patch.INIT_SIGNATURE
        p[0x1B48:0x1B48 + len(patch.RESET_SIGNATURE)] = patch.RESET_SIGNATURE
        p[0x2598:0x2598 + len(patch.READY_SIGNATURE)] = patch.READY_SIGNATURE
        p[0x1B08:0x1B08 + len(patch.COMMAND_READY_SIGNATURE)] = patch.COMMAND_READY_SIGNATURE
        p[0x1AF0:0x1AF6] = bytes.fromhex("598f4eba0aa4")
        p[-256:] = b"\xff" * 256
        regions = (patch.swap_words(p), b"S" * 0x20000, b"M" * 0x20000,
                   b"V" * 0x800000, b"", b"C" * 0x40000)
        header = bytearray(4096)
        header[:4] = b"NEO\x01"
        struct.pack_into("<6I", header, 4, *(len(r) for r in regions))
        return bytes(header) + b"".join(regions)

    def test_only_sound_regions_change(self):
        base = self.make_base()
        result = patch.patch_image(base, self.make_driver())
        self.assertEqual(result[:4096], base[:4096])
        old, new = patch.read_regions(base), patch.read_regions(result)
        for name in ("s1", "v1", "v2", "c"):
            self.assertEqual(old[name], new[name])
        before, after = patch.swap_words(old["p1"]), patch.swap_words(new["p1"])
        allowed = set(range(0x1B3A, 0x1B48)) | {0x1B4B, 0x25A9, 0x1B13}
        allowed |= set(range(0x1AF0, 0x1AF6)) | set(range(0x7FF00, 0x80000))
        self.assertTrue(all(i in allowed for i, (a, b) in enumerate(zip(before, after)) if a != b))
        self.assertEqual(after[0x1B3A:0x1B40], bytes.fromhex("4ef90007ff00"))
        self.assertEqual(after[0x1B4B], 8)
        self.assertEqual(after[0x25A9], 0x80)
        self.assertEqual(after[0x1B13], 0x80)
        self.assertEqual(after[0x7FF00:0x7FF06], bytes.fromhex("42390010ef00"))
        self.assertEqual(after[0x1AF0:0x1AF6], bytes.fromhex("4ef90007ff40"))

    def test_unrecognized_sender_prologue_is_rejected(self):
        base = self.make_base()
        r = patch.read_regions(base)
        p = bytearray(patch.swap_words(r['p1']))
        p[0x1AF0] = 0
        r['p1'] = patch.swap_words(p)
        with self.assertRaisesRegex(ValueError, 'prologue'):
            patch.patch_image(base[:4096] + b''.join(r.values()), self.make_driver())

    def test_occupied_bootstrap_padding_is_rejected(self):
        base = self.make_base()
        r = patch.read_regions(base)
        p = bytearray(patch.swap_words(r["p1"]))
        p[-256] = 0
        r["p1"] = patch.swap_words(p)
        with self.assertRaises(ValueError):
            patch.patch_image(base[:4096] + b"".join(r.values()), self.make_driver())

    def test_legacy_driver_cannot_be_paired_with_new_program(self):
        with self.assertRaisesRegex(ValueError, "paired ready/slot protocol"):
            patch.patch_image(self.make_base(), b"M" * 0x20000)

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
