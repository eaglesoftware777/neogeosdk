"""Header-only helpers survive the cartridge linker's discarded .text section."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class HeaderRomSectionTests(unittest.TestCase):
    def test_helpers_stay_in_the_callers_rom_section(self):
        sdkhome = Path(os.environ.get('SDKHOME', ROOT.parent))
        compiler = next((sdkhome / d / 'm68k-unknown-elf/bin/m68k-unknown-elf-gcc'
                         for d in ('x-tools-v3', 'x-tools-v2', 'x-tools')
                         if (sdkhome / d / 'm68k-unknown-elf/bin/m68k-unknown-elf-gcc').is_file()), None)
        if compiler is None:
            self.skipTest('no m68k toolchain')
        objdump = str(compiler).removesuffix('gcc') + 'objdump'
        program = '''
#include "sdk/macro.h"
#include "sdk/ng_input_repeat.h"
#include "sdk/2d_engine/ng_frame_sync.h"
uint16_t NEOGEO_USER probe(NGInputRepeat *r, NGFrameSync *s,
                          uint16_t held, uint8_t delay, uint8_t interval) {
    uint16_t result = ng_input_repeat_step(r, held, delay, interval);
    result += ng_frame_sync_publish(s);
    result += ng_frame_sync_take(s);
    return result;
}
'''
        with tempfile.TemporaryDirectory() as folder:
            source, obj = Path(folder) / 'probe.c', Path(folder) / 'probe.o'
            source.write_text(program)
            for optimization in ('-O0', '-O2'):
                with self.subTest(optimization=optimization):
                    subprocess.run([str(compiler), '-std=c99', optimization, '-fno-inline',
                                    '-march=68000', '-ffreestanding', '-Wall', '-Wextra', '-Werror',
                                    '-I', str(ROOT), '-c', str(source), '-o', str(obj)], check=True)
                    sections = subprocess.check_output([objdump, '-h', str(obj)], text=True)
                    sizes = {name: int(size, 16) for name, size in
                             re.findall(r'^\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s', sections, re.M)}
                    self.assertEqual(sizes.get('.text', 0), 0, sections)
                    self.assertGreater(sizes.get('neogeo_user', 0), 0, sections)
                    relocations = subprocess.check_output([objdump, '-r', str(obj)], text=True)
                    self.assertNotRegex(relocations, r'\s\.text(?:\+|\s|$)')


if __name__ == '__main__':
    unittest.main()
