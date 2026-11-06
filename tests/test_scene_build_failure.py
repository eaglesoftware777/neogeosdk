"""A failed first scene must stop compilation before stale objects can link."""
import subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class SceneBuildFailureTests(unittest.TestCase):
    def test_actual_scene_recipe_propagates_first_error(self):
        recipe=next(line for line in(ROOT/'Makefile').read_text().splitlines()if'$(foreach src,$(GAME_SCENE_SRCS)'in line)
        with tempfile.TemporaryDirectory(prefix='scene-compile-')as d:
            folder=Path(d);cc=folder/'cc';cc.write_text('#!/bin/sh\necho "$1" >> called\n[ "$1" != "first.c" ]\n');cc.chmod(0o755)
            (folder/'Makefile').write_text('GAME_SCENE_SRCS = first.c second.c\nGAME_CC = ./cc\nall:\n'+recipe+'\n\t@echo linked > linked\n')
            result=subprocess.run(['make','--no-print-directory'],cwd=folder,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
            self.assertNotEqual(result.returncode,0,result.stdout)
            self.assertEqual((folder/'called').read_text(),'first.c\n')
            self.assertFalse((folder/'linked').exists(),'The build advanced to linking after a failed scene')
if __name__=='__main__':unittest.main()
