"""ROM packaging invariants, independent of the cross compiler and emulator."""

import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("bios_build", Path(__file__).parents[1] / "tools/build.py")
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)


class PackagingTest(unittest.TestCase):
    def test_pad_and_byte_order(self):
        self.assertEqual(build.rom_image(b"\x12\x34", True)[:4], b"\x34\x12\xff\xff")
        self.assertEqual(len(build.rom_image(b"x")), 0x20000)
        for data in (b"", bytes(0x20001)):
            with self.assertRaises(ValueError):
                build.rom_image(data)

    def test_bundle_does_not_include_installed_system_roms(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            bios = root / "bios"
            bios.mkdir()
            (bios / "README.md").write_text("Firmware manual")
            (root / "LICENSE").write_text("License")
            for filenames in build.FIRMWARE.values():
                for name in filenames:
                    (bios / name).write_bytes(bytes(0x20000))
            cart = root / "roms/test"
            cart.mkdir(parents=True)
            for part in ("p1", "s1", "m1", "v1", "c1", "c2"):
                (cart / f"777-{part}.{part}").write_bytes(b"cartridge")
            hashes = root / "hash_eagle/test"
            hashes.mkdir(parents=True)
            (hashes / "neogeo.xml").write_text("<softwarelist/>")
            installed = root / "roms/neogeo"
            installed.mkdir()
            (installed / "sp-s2.sp1").write_bytes(b"do not replace or distribute")
            output = root / "dist/test.zip"
            with patch.object(build, "ROOT", root), patch.object(build, "BIOS", bios):
                build.install(root / "isolated")
                build.package("test", "777", output)
            self.assertEqual((installed / "sp-s2.sp1").read_bytes(), b"do not replace or distribute")
            with zipfile.ZipFile(output) as archive:
                self.assertEqual(archive.read("roms/neogeo/sp-s2.sp1"), bytes(0x20000))
                self.assertNotIn("roms/aes/sm1.sm1", archive.namelist())
                self.assertIn("hash/neogeo.xml", archive.namelist())


if __name__ == "__main__":
    unittest.main()
