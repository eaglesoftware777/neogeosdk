"""Reject stale Maiya graphics and verify installed NeoSD lane conversion."""
import importlib.util
import hashlib
from pathlib import Path
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
spec = importlib.util.spec_from_file_location("maiya_installer", ROOT / "tools/build_maiya_mame.py")
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)


class InstallTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.generated = self.root / "games/maiya/artbox/generated"
        self.roms = self.root / "roms"
        self.generated.mkdir(parents=True)
        self.roms.mkdir()
        listing = ET.Element("softwarelist")
        for part in ("c1", "c2"):
            name = f"780-{part}.{part}"
            data = b"X" + bytes(0x400000-1)
            (self.generated / name).write_bytes(data)
            (self.roms / name).write_bytes(data)
            ET.SubElement(listing, "rom", name=name, size="0x400000", sha1=hashlib.sha1(data).hexdigest())
        self.hash = self.root / "neogeo.xml"
        ET.ElementTree(listing).write(self.hash)

    def test_matching_graphics(self):
        installer.verify_set(self.root, self.roms, self.hash)

    def test_program_only_rebuild_cannot_reuse_stale_lanes(self):
        for part in ("c1", "c2"):
            path = self.roms / f"780-{part}.{part}"
            data = path.read_bytes()
            path.write_bytes(b"Y" + data[1:])
            with self.assertRaisesRegex(ValueError, "Stale Maiya graphics"):
                installer.verify_set(self.root, self.roms, self.hash)
            path.write_bytes(data)

    def test_matching_files_still_need_a_blank_hardware_tile(self):
        for folder in (self.generated, self.roms):
            path = folder / "780-c1.c1"
            path.write_bytes(path.read_bytes()[:-1] + b"X")
        with self.assertRaisesRegex(ValueError, "tile reservation"):
            installer.verify_set(self.root, self.roms, self.hash)

    def test_manifest_must_describe_actual_roms(self):
        tree = ET.parse(self.hash)
        tree.find("rom").set("sha1", "0"*40)
        tree.write(self.hash)
        with self.assertRaisesRegex(ValueError, "Software list"):
            installer.verify_set(self.root, self.roms, self.hash)

    @unittest.skipUnless(Path('/mnt/c/mame/neogeosdk/tests/aes/Maiya-AES.neo').exists(), 'No installed cartridge')
    def test_installed_aes_and_mvs_images_preserve_every_lane(self):
        for mode in ("aes", "mvs"):
            folder = Path('/mnt/c/mame/neogeosdk/tests') / mode
            parts = {part: (folder / "roms/maiya" / f"780-{part}.{part}").read_bytes()
                     for part in installer.PARTS}
            installer.verify_image((folder / f"Maiya-{mode.upper()}.neo").read_bytes(), parts)
            installer.verify_set(ROOT, folder / "roms/maiya", folder / "hash/neogeo.xml")


if __name__ == "__main__":
    unittest.main()
