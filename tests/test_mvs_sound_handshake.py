"""Execute the assembled driver with Z80Ex; YM ports are recorded, not synthesized."""

import ctypes as c
import ctypes.util
import os
from pathlib import Path
import shutil
import subprocess
import struct
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
LIB = os.environ.get("Z80EX_LIB") or ctypes.util.find_library("z80ex")


class Machine:
    def __init__(self, rom):
        self.lib = c.CDLL(LIB)
        self.mem = bytearray(rom[:0xF800]) + bytearray(0x800)
        self.latch = 0
        self.writes = []
        self.fetches = []
        mr = c.CFUNCTYPE(c.c_uint8, c.c_void_p, c.c_uint16, c.c_int, c.c_void_p)
        mw = c.CFUNCTYPE(None, c.c_void_p, c.c_uint16, c.c_uint8, c.c_void_p)
        pr = c.CFUNCTYPE(c.c_uint8, c.c_void_p, c.c_uint16, c.c_void_p)
        pw = c.CFUNCTYPE(None, c.c_void_p, c.c_uint16, c.c_uint8, c.c_void_p)
        ir = c.CFUNCTYPE(c.c_uint8, c.c_void_p, c.c_void_p)

        def read(cpu, addr, m1, data):
            if m1:
                self.fetches.append(addr)
            return self.mem[addr]

        def write(cpu, addr, value, data):
            if addr >= 0xF800:
                self.mem[addr] = value

        def port_write(cpu, port, value, data):
            self.writes.append((port & 255, value, self.pc))
            if port & 255 == 0:
                self.latch = 0

        self.callbacks = [mr(read), mw(write),
                          pr(lambda cpu, port, data: self.latch if port & 255 == 0 else 0),
                          pw(port_write), ir(lambda cpu, data: 255)]
        self.lib.z80ex_create.argtypes = [mr, c.c_void_p, mw, c.c_void_p,
                                         pr, c.c_void_p, pw, c.c_void_p, ir, c.c_void_p]
        self.lib.z80ex_create.restype = c.c_void_p
        for name in ("step", "nmi", "int", "doing_halt", "destroy"):
            getattr(self.lib, "z80ex_" + name).argtypes = [c.c_void_p]
        self.lib.z80ex_get_reg.argtypes = [c.c_void_p, c.c_int]
        self.lib.z80ex_get_reg.restype = c.c_uint16
        self.cpu = self.lib.z80ex_create(self.callbacks[0], None, self.callbacks[1], None,
                                        self.callbacks[2], None, self.callbacks[3], None,
                                        self.callbacks[4], None)

    @property
    def pc(self):
        return self.lib.z80ex_get_reg(self.cpu, 10)

    def run(self, condition, limit=100000):
        for _ in range(limit):
            self.lib.z80ex_step(self.cpu)
            if condition():
                return
        raise AssertionError(f"Driver did not reach expected state; PC={self.pc:04x}")

    def halt(self):
        self.run(lambda: self.lib.z80ex_doing_halt(self.cpu))

    def command(self, command, sleeping=False):
        self.latch = command
        assert self.lib.z80ex_nmi(self.cpu)
        if sleeping:
            self.run(lambda: self.pc == 0xFF85)
        else:
            self.halt()

    def close(self):
        self.lib.z80ex_destroy(self.cpu)


@unittest.skipUnless(LIB and shutil.which("wla-z80") and shutil.which("wlalink"),
                     "Requires Z80Ex and WLA-DX")
class SlotSwitchTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            obj, link, binary = (tmp / name for name in ("driver.o", "driver.link", "m1.bin"))
            subprocess.run(["wla-z80", "-I", str(ROOT / "sound/driver"), "-o", str(obj),
                            str(ROOT / "sound/driver/driver.asm")], check=True)
            link.write_text(f"[objects]\n{obj}\n")
            subprocess.run(["wlalink", "-r", str(link), str(binary)], check=True)
            cls.rom = binary.read_bytes()

    def setUp(self):
        self.machine = Machine(self.rom)
        self.addCleanup(self.machine.close)
        self.machine.halt()

    def test_acknowledgement_executes_in_ram_and_wait_has_no_rom_fetch(self):
        m = self.machine
        m.command(1, sleeping=True)
        self.assertEqual(m.mem[0xFE42], 1)
        self.assertTrue(any(port == 12 and value == 1 and pc >= 0xF800
                            for port, value, pc in m.writes))
        m.fetches.clear()
        for _ in range(1000):
            m.lib.z80ex_step(m.cpu)
        self.assertTrue(all(addr >= 0xF800 for addr in m.fetches))
        self.assertEqual(m.mem[0xF927] & 15, 0)
        self.assertEqual(m.lib.z80ex_get_reg(m.cpu, 16), 1)
        m.command(3)
        self.assertEqual(m.mem[0xFE42], 0)
        self.assertTrue(m.mem[0xF927] & 2)

    def test_one_remains_a_volume_parameter(self):
        m = self.machine
        m.command(5)
        self.assertNotEqual(m.mem[0xFE0B], 0)
        m.command(1)
        self.assertEqual(m.mem[0xFE42], 0)
        self.assertEqual(m.mem[0xFE0C], 1)
        self.assertEqual(m.mem[0xFE0B], 0)

    def test_game_init_and_soft_reset_keep_timer_running(self):
        m = self.machine
        for command in (9, 3, 4, 9):
            m.command(command)
            self.assertEqual(m.mem[0xFE42], 0)
            self.assertTrue(m.mem[0xF927] & 2)

    def test_parameter_arriving_before_fifo_dispatch(self):
        m = self.machine
        m.latch = 5
        self.assertTrue(m.lib.z80ex_nmi(m.cpu))
        m.run(lambda: m.pc < 0xF800 and m.mem[0xF820] != m.mem[0xF821]
              and m.mem[0xFE43] == 1 and m.writes[-1][:2] == (12, 1))
        m.command(1)
        self.assertEqual(m.mem[0xFE42], 0)
        self.assertEqual(m.mem[0xFE0C], 1)

    def test_playback_register_writes_match_release_driver(self):
        path = ROOT / "dist/release/Maiya-WIP-NeoSD_MVS_v1.neo"
        if not path.exists():
            self.skipTest("Release baseline not present")
        image = path.read_bytes()
        sizes = struct.unpack_from("<6I", image, 4)
        start = 4096 + sum(sizes[:2])
        old = Machine(image[start:start + sizes[2]])
        self.addCleanup(old.close)
        old.halt()
        new = self.machine
        old.command(1)
        new.command(9)
        for m in (old, new):
            m.writes.clear()
            for command in (5, 1, 6, 0xC0, 0x40, 0x80, 0x31, 1, 0x32, 1):
                m.command(command)
            for _ in range(120):
                self.assertTrue(m.lib.z80ex_int(m.cpu))
                m.halt()
            m.command(4)
        ym = lambda m: [(port, value) for port, value, pc in m.writes
                         if port in (4, 5, 6, 7)]
        self.assertEqual(ym(old), ym(new))


if __name__ == "__main__":
    unittest.main()
