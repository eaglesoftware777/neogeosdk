from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]


class InputTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('cc'),'Host C compiler required')
    def test_pressed_edges_survive_multiple_game_reads(self):
        with tempfile.TemporaryDirectory() as d:
            f=Path(d);source=f/'input.c';binary=f/'input'
            source.write_text(r'''
#include "ng_cd.h"
#include "neogeo.h"
#include <assert.h>
int main(void) {
 ng_cd_input_sample(0);assert(!poll_joystick_edge());
 ng_cd_input_sample(0x10);assert(poll_joystick()==0x10);
 assert(poll_joystick_edge()==0x10);assert(poll_joystick_edge()==0x10);
 ng_cd_input_sample(0x10);assert(!poll_joystick_edge());
 ng_cd_input_sample(0x118);assert(poll_joystick_edge()==0x108);
 ng_cd_input_sample(0);assert(!poll_joystick());assert(!poll_joystick_edge());
 ng_cd_input_sample(0x10);assert(poll_joystick_edge()==0x10);
 return 0;
}
''')
            subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',
                            '-I'+str(ROOT/'sdk'),'-I'+str(ROOT/'sdk/platform/cd'),
                            str(source),str(ROOT/'sdk/platform/cd/ng_cd_input.c'),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)


if __name__=='__main__':unittest.main()
